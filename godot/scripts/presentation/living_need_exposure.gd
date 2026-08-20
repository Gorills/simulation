extends Node

const TEST_VALIDATION_FRAME_LIMIT := 120
const FOOTPRINT_HEIGHT_M := 0.04
const LABEL_HEIGHT_M := 1.25

var _scene: Node = null
var _sim = null
var _world_presentation: Node = null
var _marker_root: Node3D = null
var _footprint: MeshInstance3D = null
var _footprint_mesh: BoxMesh = null
var _target_label: Label3D = null
var _npc_label: Label3D = null
var _npc_label_parent: Node3D = null
var _test_scenario := false
var _test_validated := false
var _validation_frames := 0


func _ready() -> void:
    _test_scenario = _scenario_name() in ["smoke", "offscreen", "rest_interference"]


func _process(_delta: float) -> void:
    var current_scene := get_tree().current_scene
    if current_scene != _scene:
        _bind_scene(current_scene)

    if _scene == null or _sim == null or _world_presentation == null:
        _record_validation_wait()
        return

    var projection_value = _sim.living_need_projection()
    if typeof(projection_value) != TYPE_DICTIONARY:
        _record_validation_wait()
        return
    var projection: Dictionary = projection_value
    if not _apply_projection(projection):
        _record_validation_wait()
        return

    if _test_scenario:
        _test_validated = true


func _bind_scene(scene: Node) -> void:
    _clear_runtime_nodes()
    _scene = scene
    _sim = null
    _world_presentation = null
    _validation_frames = 0
    _test_validated = false

    if _scene == null:
        return

    _sim = _scene.get("sim")
    _world_presentation = _scene.get_node_or_null("WorldPresentation")
    if _sim == null or _world_presentation == null:
        return

    _marker_root = Node3D.new()
    _marker_root.name = "LivingNeedRestExposure"
    _scene.add_child(_marker_root)

    _footprint = MeshInstance3D.new()
    _footprint.name = "AuthoritativeRestFootprint"
    _footprint.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    _marker_root.add_child(_footprint)

    _footprint_mesh = BoxMesh.new()
    _footprint.mesh = _footprint_mesh

    var material := StandardMaterial3D.new()
    material.albedo_color = Color(0.20, 0.75, 1.0, 1.0)
    material.roughness = 0.45
    _footprint.material_override = material

    _target_label = Label3D.new()
    _target_label.name = "RestInteractionHint"
    _target_label.position = Vector3(0.0, LABEL_HEIGHT_M, 0.0)
    _target_label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
    _target_label.fixed_size = true
    _target_label.font_size = 30
    _target_label.outline_size = 8
    _marker_root.add_child(_target_label)


func _apply_projection(projection: Dictionary) -> bool:
    var entity_id := int(projection.get("entity_id", 0))
    var target_value = projection.get("target_position_m", null)
    var tolerance := float(projection.get("axis_arrival_tolerance_m", -1.0))
    var status := str(projection.get("status", "unknown"))
    if entity_id <= 0 or typeof(target_value) != TYPE_VECTOR3 or tolerance < 0.0:
        return false
    if _marker_root == null or _footprint_mesh == null or _target_label == null:
        return false

    var target: Vector3 = target_value
    _marker_root.position = Vector3(target.x, FOOTPRINT_HEIGHT_M * 0.5, target.z)
    var footprint_size := max(tolerance * 2.0, 0.05)
    _footprint_mesh.size = Vector3(footprint_size, FOOTPRINT_HEIGHT_M, footprint_size)
    _target_label.text = tr(&"UI_WORLD_REST_TARGET_HINT")

    _refresh_npc_label(entity_id, status)
    return true


func _refresh_npc_label(entity_id: int, status: String) -> void:
    var presentation_value = _world_presentation.call("presentation_for", entity_id)
    var presentation := presentation_value as Node3D
    if presentation != _npc_label_parent:
        if _npc_label != null and is_instance_valid(_npc_label):
            _npc_label.queue_free()
        _npc_label = null
        _npc_label_parent = presentation
        if _npc_label_parent != null:
            _npc_label = Label3D.new()
            _npc_label.name = "LivingNeedStatus"
            _npc_label.position = Vector3(0.0, 2.2, 0.0)
            _npc_label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
            _npc_label.fixed_size = true
            _npc_label.font_size = 26
            _npc_label.outline_size = 8
            _npc_label_parent.add_child(_npc_label)

    if _npc_label == null:
        return
    match status:
        "traveling":
            _npc_label.text = tr(&"UI_NEED_TRAVELING")
        "blocked":
            _npc_label.text = tr(&"UI_NEED_BLOCKED")
        "satisfied":
            _npc_label.text = tr(&"UI_NEED_SATISFIED")
        _:
            _npc_label.text = ""


func _record_validation_wait() -> void:
    if not _test_scenario or _test_validated:
        return
    _validation_frames += 1
    if _validation_frames <= TEST_VALIDATION_FRAME_LIMIT:
        return
    push_error("living-need interaction exposure did not materialize from authoritative projection")
    get_tree().quit(12)


func _clear_runtime_nodes() -> void:
    if _marker_root != null and is_instance_valid(_marker_root):
        _marker_root.queue_free()
    if _npc_label != null and is_instance_valid(_npc_label):
        _npc_label.queue_free()
    _marker_root = null
    _footprint = null
    _footprint_mesh = null
    _target_label = null
    _npc_label = null
    _npc_label_parent = null


func _scenario_name() -> String:
    var args := OS.get_cmdline_user_args()
    for index in range(args.size() - 1):
        if args[index] == "--scenario":
            return str(args[index + 1])
    return "interactive"
