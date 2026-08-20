class_name ResourceInteraction
extends Node

const DEBUG_REFRESH_INTERVAL_SECONDS := 0.25
const CUE_HEIGHT_M := 0.05
const LABEL_HEIGHT_M := 1.25
const OWN_STORE_COLOR := Color(0.92, 0.72, 0.18, 1.0)
const NEIGHBOUR_STORE_COLOR := Color(0.28, 0.78, 0.38, 1.0)
const FIELD_COLOR := Color(0.62, 0.48, 0.22, 1.0)

signal resource_state_changed

var _sim: SimFacade = null
var _target_household_id := 0
var _carry_label: Label
var _work_label: Label
var _pledge_label: Label
var _refusal_label: Label
var _prompt_label: Label
var _field_label: Label3D
var _world_root: Node3D = null
var _cue_root: Node3D = null
var _store_cues: Dictionary = {}
var _field_mesh: BoxMesh = null
var _field_material: StandardMaterial3D = null
var _field_footprint: MeshInstance3D = null
var _refresh_elapsed := 0.0
var _interactive := true
var _field_work_projection: Dictionary = {}
var _village_projection: Dictionary = {}
var _carry_projection: Dictionary = {}
var _spatial_projection: Dictionary = {}


func configure(
    sim_facade: SimFacade,
    target_household_id: int,
    carry_label: Label,
    work_label: Label,
    pledge_label: Label,
    refusal_label: Label,
    field_cue: Label3D,
    interaction_prompt: Label,
    world_root: Node3D
) -> void:
    _clear_cues()
    _sim = sim_facade
    _target_household_id = target_household_id
    _carry_label = carry_label
    _work_label = work_label
    _pledge_label = pledge_label
    _refusal_label = refusal_label
    _field_label = field_cue
    _prompt_label = interaction_prompt
    _world_root = world_root
    _interactive = _user_arg_value("--scenario").is_empty() or _user_arg_value("--scenario") == "interactive"
    if _sim == null:
        return
    if not Localization.locale_changed.is_connected(_on_locale_changed):
        Localization.locale_changed.connect(_on_locale_changed)
    _ensure_cue_root()
    _refresh_field_work_projection()
    _refresh_all_hud()


func set_interactive(enabled: bool) -> void:
    _interactive = enabled


func refresh() -> void:
    _refresh_field_work_projection()
    _refresh_all_hud()


func show_command_outcome(response: Dictionary) -> void:
    if bool(response.get("ok", false)):
        _show_refusal("")
        resource_state_changed.emit()
    else:
        _show_refusal(str(response.get("error", "unknown")))
    _refresh_field_work_projection()
    _refresh_all_hud()


func carry_hud_text() -> String:
    return _carry_label.text if _carry_label != null else ""


func work_hud_text() -> String:
    return _work_label.text if _work_label != null else ""


func pledge_hud_text() -> String:
    return _pledge_label.text if _pledge_label != null else ""


func refusal_hud_text() -> String:
    return _refusal_label.text if _refusal_label != null else ""


func field_cue_text() -> String:
    return _field_label.text if _field_label != null else ""


func _process(delta: float) -> void:
    if _sim == null:
        return
    _refresh_elapsed += delta
    if _refresh_elapsed < DEBUG_REFRESH_INTERVAL_SECONDS:
        return
    _refresh_elapsed = 0.0
    _refresh_field_work_projection()
    _refresh_all_hud()


func _unhandled_input(event: InputEvent) -> void:
    if _sim == null or not _interactive:
        return

    var response: Dictionary = {}
    if event.is_action_pressed(&"resource_draw"):
        response = _sim.controlled_actor_draw_grain()
    elif event.is_action_pressed(&"resource_deposit"):
        response = _sim.controlled_actor_deposit_grain()
    elif event.is_action_pressed(&"resource_gift"):
        if _target_household_id <= 0:
            return
        response = _sim.controlled_actor_gift_grain(_target_household_id)
    elif event.is_action_pressed(&"resource_work"):
        response = _sim.controlled_actor_complete_field_work()
    elif event.is_action_pressed(&"resource_transfer"):
        response = _sim.controlled_actor_execute_household_transfer_pledge()
    else:
        return

    if bool(response.get("ok", false)):
        _show_refusal("")
        resource_state_changed.emit()
    else:
        _show_refusal(str(response.get("error", "unknown")))
    _refresh_field_work_projection()
    _refresh_all_hud()
    get_viewport().set_input_as_handled()


func _on_locale_changed(_locale: String) -> void:
    _refresh_all_hud()


func _refresh_all_hud() -> void:
    if _sim == null:
        return
    _carry_projection = _sim.controlled_actor_carry_projection()
    _village_projection = _sim.village_household_resource_projection()
    _spatial_projection = _sim.controlled_actor_spatial_projection()
    _refresh_carry_hud()
    _refresh_work_hud()
    _refresh_pledge_hud()
    _refresh_field_cue()
    _refresh_place_cues()
    _refresh_interaction_prompt()


func _refresh_carry_hud() -> void:
    if _carry_label == null:
        return
    var carried := int(_carry_projection.get("carried_grain_units", -1))
    var capacity := int(_carry_projection.get("grain_carry_capacity_units", -1))
    if carried < 0 or capacity < 0:
        _carry_label.text = "—"
        return
    _carry_label.text = "%s %d/%d" % [
        tr(&"UI_DEBUG_GRAIN_CARRY"),
        carried,
        capacity,
    ]
    _carry_label.visible = true


func _refresh_work_hud() -> void:
    if _work_label == null or _sim == null:
        return
    if _field_work_projection.is_empty():
        _work_label.text = ""
        _work_label.visible = false
        return
    var remaining := int(_field_work_projection.get("remaining_work_completions", -1))
    if remaining < 0:
        _work_label.text = ""
        _work_label.visible = false
        return
    _work_label.text = "%s: %d" % [
        tr(&"UI_DEBUG_FIELD_WORK"),
        remaining,
    ]
    _work_label.visible = true


func _refresh_pledge_hud() -> void:
    if _pledge_label == null or _sim == null:
        return
    var pledge: Dictionary = _sim.standing_transfer_pledge_projection()
    var remaining := int(pledge.get("remaining_grain_units", -1))
    if remaining < 0 or int(pledge.get("source_household_id", 0)) <= 0:
        _pledge_label.text = ""
        _pledge_label.visible = false
        return
    _pledge_label.text = "%s: %d" % [
        tr(&"UI_DEBUG_STANDING_TRANSFER"),
        remaining,
    ]
    _pledge_label.visible = true


func _refresh_field_cue() -> void:
    if _field_label == null:
        return
    var position_value = _field_work_projection.get("work_position_m", null)
    if typeof(position_value) == TYPE_VECTOR3:
        var position: Vector3 = position_value
        _field_label.position = position + Vector3(0.0, 0.8, 0.0)
    _field_label.text = tr(&"UI_FIELD_WORK_CUE")


func _refresh_place_cues() -> void:
    if not _ensure_cue_root():
        return
    var households_value = _village_projection.get("households", null)
    if typeof(households_value) != TYPE_ARRAY:
        return
    var member_household_id := int(_carry_projection.get("member_household_id", 0))
    var seen: Dictionary = {}
    for household_value in households_value:
        if typeof(household_value) != TYPE_DICTIONARY:
            continue
        var household: Dictionary = household_value
        var household_id := int(household.get("household_id", 0))
        var store_value = household.get("store_position_m", null)
        var tolerance := float(household.get("store_axis_tolerance_m", -1.0))
        if household_id <= 0 or typeof(store_value) != TYPE_VECTOR3 or tolerance < 0.0:
            continue
        var store: Vector3 = store_value
        var is_own := household_id == member_household_id
        var cue: Dictionary = _store_cue(household_id, is_own)
        _place_footprint(cue, store, tolerance, is_own)
        seen[household_id] = true
    _prune_store_cues(seen)
    _refresh_field_footprint()


func _refresh_field_footprint() -> void:
    var position_value = _field_work_projection.get("work_position_m", null)
    var tolerance := float(_field_work_projection.get("work_axis_tolerance_m", -1.0))
    if typeof(position_value) != TYPE_VECTOR3 or tolerance < 0.0 or _cue_root == null:
        return
    if _field_footprint == null or not is_instance_valid(_field_footprint):
        _field_footprint = MeshInstance3D.new()
        _field_footprint.name = "FieldWorkFootprint"
        _field_footprint.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
        _field_mesh = BoxMesh.new()
        _field_footprint.mesh = _field_mesh
        _field_material = _make_material(FIELD_COLOR)
        _field_footprint.material_override = _field_material
        _cue_root.add_child(_field_footprint)
    var position: Vector3 = position_value
    _field_footprint.position = Vector3(position.x, CUE_HEIGHT_M * 0.5, position.z)
    var size := maxf(tolerance * 2.0, 0.05)
    _field_mesh.size = Vector3(size, CUE_HEIGHT_M, size)
    _set_occupied_color(_field_material, FIELD_COLOR, _occupies(position, tolerance))


func _refresh_interaction_prompt() -> void:
    if _prompt_label == null:
        return
    var households_value = _village_projection.get("households", null)
    if typeof(households_value) != TYPE_ARRAY:
        _prompt_label.text = tr(&"UI_RESOURCE_PROMPT_SEEK")
        return
    var member_household_id := int(_carry_projection.get("member_household_id", 0))
    var field_position_value = _field_work_projection.get("work_position_m", null)
    var field_tolerance := float(_field_work_projection.get("work_axis_tolerance_m", -1.0))
    if typeof(field_position_value) == TYPE_VECTOR3 and field_tolerance >= 0.0:
        var field_position: Vector3 = field_position_value
        if _occupies(field_position, field_tolerance):
            _prompt_label.text = tr(&"UI_RESOURCE_PROMPT_FIELD")
            return
    for household_value in households_value:
        if typeof(household_value) != TYPE_DICTIONARY:
            continue
        var household: Dictionary = household_value
        var store_value = household.get("store_position_m", null)
        var tolerance := float(household.get("store_axis_tolerance_m", -1.0))
        if typeof(store_value) != TYPE_VECTOR3 or tolerance < 0.0:
            continue
        var store: Vector3 = store_value
        if not _occupies(store, tolerance):
            continue
        if int(household.get("household_id", 0)) == member_household_id:
            _prompt_label.text = tr(&"UI_RESOURCE_PROMPT_OWN_STORE")
        else:
            _prompt_label.text = tr(&"UI_RESOURCE_PROMPT_NEIGHBOUR_STORE")
        return
    _prompt_label.text = tr(&"UI_RESOURCE_PROMPT_SEEK")


func _occupies(center: Vector3, tolerance: float) -> bool:
    var position_value = _spatial_projection.get("position_m", null)
    if typeof(position_value) != TYPE_VECTOR3 or tolerance < 0.0:
        return false
    var position: Vector3 = position_value
    return absf(position.x - center.x) <= tolerance and absf(position.z - center.z) <= tolerance


func _store_cue(household_id: int, is_own: bool) -> Dictionary:
    if _store_cues.has(household_id):
        return _store_cues[household_id]
    var root := Node3D.new()
    root.name = "HouseholdStoreCue_%d" % household_id
    _cue_root.add_child(root)
    var footprint := MeshInstance3D.new()
    footprint.name = "Footprint"
    footprint.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    var mesh := BoxMesh.new()
    footprint.mesh = mesh
    var material := _make_material(OWN_STORE_COLOR if is_own else NEIGHBOUR_STORE_COLOR)
    footprint.material_override = material
    root.add_child(footprint)
    var label := Label3D.new()
    label.name = "CueLabel"
    label.position = Vector3(0.0, LABEL_HEIGHT_M, 0.0)
    label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
    label.font_size = 30
    label.outline_size = 8
    root.add_child(label)
    var cue := {
        "root": root,
        "mesh": mesh,
        "material": material,
        "label": label,
    }
    _store_cues[household_id] = cue
    return cue


func _place_footprint(cue: Dictionary, store: Vector3, tolerance: float, is_own: bool) -> void:
    var root: Node3D = cue["root"]
    var mesh: BoxMesh = cue["mesh"]
    var material: StandardMaterial3D = cue["material"]
    var label: Label3D = cue["label"]
    root.position = Vector3(store.x, CUE_HEIGHT_M * 0.5, store.z)
    var size := maxf(tolerance * 2.0, 0.05)
    mesh.size = Vector3(size, CUE_HEIGHT_M, size)
    var base := OWN_STORE_COLOR if is_own else NEIGHBOUR_STORE_COLOR
    _set_occupied_color(material, base, _occupies(store, tolerance))
    label.text = tr(&"UI_STORE_OWN_CUE") if is_own else tr(&"UI_STORE_NEIGHBOUR_CUE")


func _prune_store_cues(seen: Dictionary) -> void:
    var stale: Array[int] = []
    for household_id in _store_cues.keys():
        if not seen.has(household_id):
            stale.append(int(household_id))
    for household_id in stale:
        var cue: Dictionary = _store_cues[household_id]
        var root: Node3D = cue["root"]
        if is_instance_valid(root):
            root.queue_free()
        _store_cues.erase(household_id)


func _ensure_cue_root() -> bool:
    if _cue_root != null and is_instance_valid(_cue_root):
        return true
    if _world_root == null:
        return false
    _cue_root = Node3D.new()
    _cue_root.name = "ResourcePlaceExposure"
    _world_root.add_child(_cue_root)
    return true


func _clear_cues() -> void:
    for household_id in _store_cues.keys():
        var cue: Dictionary = _store_cues[household_id]
        var root: Node3D = cue["root"]
        if is_instance_valid(root):
            root.queue_free()
    _store_cues.clear()
    if _cue_root != null and is_instance_valid(_cue_root):
        _cue_root.queue_free()
    _cue_root = null
    _field_footprint = null
    _field_mesh = null
    _field_material = null


func _make_material(color: Color) -> StandardMaterial3D:
    var material := StandardMaterial3D.new()
    material.albedo_color = color
    material.roughness = 0.45
    return material


func _set_occupied_color(material: StandardMaterial3D, base: Color, occupied: bool) -> void:
    if material == null:
        return
    material.albedo_color = base.lerp(Color.WHITE, 0.35) if occupied else base


func _refresh_field_work_projection() -> bool:
    if _sim == null:
        return false
    var projection: Dictionary = _sim.field_work_projection()
    var position_value = projection.get("work_position_m", null)
    if (
        int(projection.get("work_place_id", 0)) <= 0
        or typeof(position_value) != TYPE_VECTOR3
        or float(projection.get("work_axis_tolerance_m", -1.0)) < 0.0
        or int(projection.get("destination_household_id", 0)) <= 0
        or int(projection.get("yield_grain_units", 0)) <= 0
        or int(projection.get("remaining_work_completions", -1)) < 0
    ):
        return false
    _field_work_projection = projection
    return true


func _show_refusal(error: String) -> void:
    if _refusal_label == null:
        return
    if error.is_empty():
        _refusal_label.text = ""
        _refusal_label.visible = false
        return
    _refusal_label.text = "%s: %s" % [tr(&"UI_DEBUG_RESOURCE_REFUSAL"), _refusal_text(error)]
    _refusal_label.visible = true


func _refusal_text(error: String) -> String:
    match error:
        "outside_store":
            return tr(&"UI_RESOURCE_REFUSAL_OUTSIDE_STORE")
        "carry_full":
            return tr(&"UI_RESOURCE_REFUSAL_CARRY_FULL")
        "store_empty":
            return tr(&"UI_RESOURCE_REFUSAL_STORE_EMPTY")
        "carry_empty":
            return tr(&"UI_RESOURCE_REFUSAL_CARRY_EMPTY")
        "own_household":
            return tr(&"UI_RESOURCE_REFUSAL_OWN_HOUSEHOLD")
        "outside_field":
            return tr(&"UI_RESOURCE_REFUSAL_OUTSIDE_FIELD")
        "work_exhausted":
            return tr(&"UI_RESOURCE_REFUSAL_WORK_EXHAUSTED")
        "pledge_zero":
            return tr(&"UI_RESOURCE_REFUSAL_PLEDGE_ZERO")
        "insufficient_stock":
            return tr(&"UI_RESOURCE_REFUSAL_INSUFFICIENT_STOCK")
        "stock_overflow":
            return tr(&"UI_RESOURCE_REFUSAL_STOCK_OVERFLOW")
        _:
            return tr(&"UI_RESOURCE_REFUSAL_UNAVAILABLE")


func _user_arg_value(name: String) -> String:
    var args := OS.get_cmdline_user_args()
    for index in range(args.size() - 1):
        if args[index] == name:
            return args[index + 1]
    return ""
