extends Node3D

const DEBUG_REFRESH_INTERVAL_SECONDS := 0.25

@onready var controls: PlayerControls = %PlayerControls
@onready var world_presentation: WorldPresentation = %WorldPresentation
@onready var player: ThirdPersonPlayer = %Player
@onready var player_entity_binding: EntityBinding = %PlayerEntityBinding
@onready var camera_rig: ThirdPersonCameraRig = %CameraRig
@onready var debug_fps: Label = %DebugFps
@onready var debug_process: Label = %DebugProcess
@onready var debug_physics: Label = %DebugPhysics
@onready var debug_input: Label = %DebugInput
@onready var debug_entity: Label = %DebugEntity
@onready var debug_tick: Label = %DebugTick
@onready var debug_revision: Label = %DebugRevision
@onready var debug_protocol: Label = %DebugProtocol
@onready var debug_seed: Label = %DebugSeed
@onready var debug_scenario: Label = %DebugScenario
@onready var debug_authority_position: Label = %DebugAuthorityPosition
@onready var debug_epoch: Label = %DebugEpoch
@onready var debug_presentation_position: Label = %DebugPresentationPosition
@onready var debug_divergence: Label = %DebugDivergence

var sim := SimFacade.new()
var _bootstrap_projection: Dictionary = {}
var _observed_world_projection: Dictionary = {}
var _controlled_actor_spatial_projection: Dictionary = {}
var _debug_refresh_elapsed := 0.0
var _scenario_name := "interactive"


func _ready() -> void:
    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to bind controlled presentation to authoritative EntityId")
        get_tree().quit(6)
        return

    _controlled_actor_spatial_projection = sim.controlled_actor_spatial_projection()
    if not world_presentation.initialize_controlled_spatial_presentation(
        _controlled_actor_spatial_projection,
        player_entity_binding
    ):
        push_error("failed to initialize controlled presentation from authoritative spatial state")
        get_tree().quit(7)
        return

    camera_rig.configure(controls, player)
    player.configure(controls, camera_rig.get_camera())
    controls.input_device_changed.connect(_on_input_device_changed)
    controls.capture_pointer()

    _set_bootstrap_projection(sim.bootstrap_debug_projection())
    var scenario := _user_arg_value("--scenario")
    if not scenario.is_empty():
        _scenario_name = scenario
    _refresh_debug_hud()

    if scenario == "smoke":
        var artifact_dir := _user_arg_value("--artifact-dir")
        if artifact_dir.is_empty():
            push_error("smoke scenario requires --artifact-dir")
            get_tree().quit(2)
            return
        call_deferred("_run_smoke", artifact_dir)


func _process(delta: float) -> void:
    _debug_refresh_elapsed += delta
    if _debug_refresh_elapsed < DEBUG_REFRESH_INTERVAL_SECONDS:
        return
    _debug_refresh_elapsed = 0.0
    _refresh_debug_hud()


func _on_input_device_changed(_device: int) -> void:
    _refresh_debug_hud()


func _set_bootstrap_projection(projection: Dictionary) -> void:
    _bootstrap_projection = projection


func _refresh_debug_hud() -> void:
    var authoritative_position := _spatial_position_or_zero(_controlled_actor_spatial_projection)
    var presentation_position := player.global_position
    var fps := int(Performance.get_monitor(Performance.TIME_FPS))
    var process_ms := float(Performance.get_monitor(Performance.TIME_PROCESS)) * 1000.0
    var physics_ms := float(Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS)) * 1000.0

    debug_fps.text = "%d" % fps
    debug_process.text = "%.2f ms" % process_ms
    debug_physics.text = "%.2f ms" % physics_ms
    debug_input.text = controls.active_device_name()

    debug_entity.text = "#%d" % world_presentation.controlled_entity_id()
    debug_tick.text = str(world_presentation.last_tick())
    debug_revision.text = str(world_presentation.last_revision())
    debug_protocol.text = str(world_presentation.protocol_version())
    debug_seed.text = str(int(_bootstrap_projection.get("seed", 0)))
    debug_scenario.text = _scenario_name

    debug_authority_position.text = "(%.2f, %.2f, %.2f) m" % [
        authoritative_position.x,
        authoritative_position.y,
        authoritative_position.z,
    ]
    debug_epoch.text = str(int(_controlled_actor_spatial_projection.get("spatial_epoch", 0)))
    debug_presentation_position.text = "(%.2f, %.2f, %.2f) m" % [
        presentation_position.x,
        presentation_position.y,
        presentation_position.z,
    ]
    debug_divergence.text = "%.2f m" % presentation_position.distance_to(authoritative_position)


func _run_smoke(artifact_dir: String) -> void:
    await get_tree().process_frame
    var response: Dictionary = sim.bootstrap_submit_move(1, 0)
    if not bool(response.get("ok", false)):
        push_error("native bootstrap move failed")
        get_tree().quit(3)
        return

    _set_bootstrap_projection(response.get("projection", {}))
    _observed_world_projection = sim.observed_world_projection()
    _controlled_actor_spatial_projection = sim.controlled_actor_spatial_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to apply observed-world projection after bootstrap move")
        get_tree().quit(6)
        return
    _refresh_debug_hud()

    await RenderingServer.frame_post_draw
    if not _write_debug_artifact(artifact_dir):
        get_tree().quit(4)
        return
    if not _write_screenshot(artifact_dir):
        get_tree().quit(5)
        return

    get_tree().quit(0)


func _write_debug_artifact(artifact_dir: String) -> bool:
    var mkdir_error := DirAccess.make_dir_recursive_absolute(artifact_dir)
    if mkdir_error != OK:
        push_error("failed to create artifact directory: %s" % error_string(mkdir_error))
        return false

    var file := FileAccess.open(artifact_dir.path_join("debug.json"), FileAccess.WRITE)
    if file == null:
        push_error("failed to open debug artifact: %s" % error_string(FileAccess.get_open_error()))
        return false

    var spatial_evidence := _spatial_evidence(_controlled_actor_spatial_projection)
    if spatial_evidence.is_empty():
        push_error("controlled spatial projection is not serializable smoke evidence")
        return false

    var evidence := {
        "bootstrap_projection": _bootstrap_projection,
        "observed_world_projection": _observed_world_projection,
        "controlled_actor_spatial_projection": spatial_evidence,
        "presentation": world_presentation.debug_snapshot(),
    }
    file.store_string(JSON.stringify(evidence, "  "))
    return true


func _spatial_evidence(projection: Dictionary) -> Dictionary:
    var position_value = projection.get("position_m", null)
    var velocity_value = projection.get("velocity_mps", null)
    if typeof(position_value) != TYPE_VECTOR3 or typeof(velocity_value) != TYPE_VECTOR3:
        return {}

    var position: Vector3 = position_value
    var velocity: Vector3 = velocity_value
    return {
        "entity_id": int(projection.get("entity_id", 0)),
        "position_m": [position.x, position.y, position.z],
        "velocity_mps": [velocity.x, velocity.y, velocity.z],
        "spatial_epoch": int(projection.get("spatial_epoch", 0)),
        "tick": int(projection.get("tick", -1)),
        "revision": int(projection.get("revision", -1)),
        "protocol_version": int(projection.get("protocol_version", 0)),
    }


func _spatial_position_or_zero(projection: Dictionary) -> Vector3:
    var value = projection.get("position_m", null)
    if typeof(value) != TYPE_VECTOR3:
        return Vector3.ZERO
    var position: Vector3 = value
    return position


func _write_screenshot(artifact_dir: String) -> bool:
    var image := get_viewport().get_texture().get_image()
    var save_error := image.save_png(artifact_dir.path_join("final.png"))
    if save_error != OK:
        push_error("failed to save screenshot: %s" % error_string(save_error))
        return false
    return true


func _user_arg_value(name: String) -> String:
    var args := OS.get_cmdline_user_args()
    for index in range(args.size() - 1):
        if args[index] == name:
            return args[index + 1]
    return ""
