extends Node

const DEBUG_REFRESH_INTERVAL_SECONDS := 0.25
const GIFT_SCENARIO_LIMIT_TICKS := 480
const LOCOMOTION_PACE_WALK := 0

var _main
var _sim
var _carry_label: Label
var _refresh_elapsed := 0.0
var _scenario_name := "interactive"
var _target_household_id := 0


func _ready() -> void:
    call_deferred("_bind_main")


func _bind_main() -> void:
    _main = get_tree().current_scene
    if _main == null or not _main.has_method("_run_one_scripted_movement"):
        return

    _sim = _main.get("sim")
    _target_household_id = int(_main.get("_tracked_household_id"))
    _scenario_name = _user_arg_value("--scenario")
    if _scenario_name.is_empty():
        _scenario_name = "interactive"

    _install_carry_hud()
    _refresh_carry_hud()

    if _scenario_name == "gift":
        _main.set("_locomotion_runtime_enabled", false)
        var artifact_dir := _user_arg_value("--artifact-dir")
        if artifact_dir.is_empty():
            push_error("gift scenario requires --artifact-dir")
            get_tree().quit(14)
            return
        call_deferred("_run_gift_scenario", artifact_dir)


func _process(delta: float) -> void:
    if _main == null or _sim == null:
        return
    _refresh_elapsed += delta
    if _refresh_elapsed < DEBUG_REFRESH_INTERVAL_SECONDS:
        return
    _refresh_elapsed = 0.0
    _refresh_carry_hud()


func _unhandled_input(event: InputEvent) -> void:
    if _main == null or _sim == null or _scenario_name != "interactive":
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
    else:
        return

    if bool(response.get("ok", false)):
        if not _reconcile_after_resource_action():
            push_error("resource action committed but presentation reconciliation failed")
    _refresh_carry_hud()
    get_viewport().set_input_as_handled()


func _install_carry_hud() -> void:
    var resource_status = _main.get("debug_household_resource_status")
    if resource_status == null or not resource_status is Label:
        return
    _carry_label = Label.new()
    _carry_label.name = "DebugCarryStatus"
    _carry_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
    resource_status.get_parent().add_child(_carry_label)


func _refresh_carry_hud() -> void:
    if _carry_label == null or _sim == null:
        return
    var carry: Dictionary = _sim.controlled_actor_carry_projection()
    var carried := int(carry.get("carried_grain_units", -1))
    var capacity := int(carry.get("grain_carry_capacity_units", -1))
    if carried < 0 or capacity < 0:
        _carry_label.text = "—"
        return
    var scenario_text := ""
    if _scenario_name == "gift":
        scenario_text = " · %s" % tr(&"UI_SCENARIO_GIFT")
    _carry_label.text = "%s: %d / %d · %s%s" % [
        tr(&"UI_DEBUG_GRAIN_CARRY"),
        carried,
        capacity,
        tr(&"UI_RESOURCE_ACTION_HINT"),
        scenario_text,
    ]


func _reconcile_after_resource_action() -> bool:
    var observed: Dictionary = _sim.observed_world_projection()
    var world_presentation = _main.get("world_presentation")
    var player_entity_binding = _main.get("player_entity_binding")
    if not world_presentation.apply_observed_world_projection(observed, player_entity_binding):
        return false
    _main.set("_observed_world_projection", observed)
    _main.set("_village_household_resource_projection", _sim.village_household_resource_projection())
    _main.set("_living_need_projection", _sim.living_need_projection())
    _main.call("_refresh_debug_hud")
    return true


func _run_gift_scenario(artifact_dir: String) -> void:
    await get_tree().process_frame

    var initial_carry: Dictionary = _sim.controlled_actor_carry_projection()
    var source_household_id := int(initial_carry.get("member_household_id", 0))
    if (
        source_household_id <= 0
        or _target_household_id <= 0
        or source_household_id == _target_household_id
        or int(initial_carry.get("carried_grain_units", -1)) != 0
        or int(initial_carry.get("grain_carry_capacity_units", 0)) <= 0
    ):
        _fail_gift("gift scenario has invalid initial carry/household bindings")
        return

    var initial_resources: Dictionary = _sim.village_household_resource_projection()
    var initial_target := _household_by_id(initial_resources, _target_household_id)
    if initial_target.is_empty():
        _fail_gift("gift target missing from initial household discovery")
        return

    var draw_response: Dictionary = _sim.controlled_actor_draw_grain()
    if not bool(draw_response.get("ok", false)):
        _fail_gift("controlled Draw failed at the source household store")
        return
    var draw_result: Dictionary = draw_response.get("result", {})
    if int(draw_result.get("moved_grain_units", 0)) <= 0:
        _fail_gift("controlled Draw did not move authoritative grain")
        return

    var shortage_resources: Dictionary = {}
    for _tick in range(GIFT_SCENARIO_LIMIT_TICKS):
        if not await _main._run_one_scripted_movement(Vector2i.ZERO, LOCOMOTION_PACE_WALK):
            _fail_gift("locomotion failed while waiting for autonomous shortage")
            return
        var resources: Dictionary = _sim.village_household_resource_projection()
        var target := _household_by_id(resources, _target_household_id)
        if target.is_empty():
            _fail_gift("gift target disappeared while waiting for shortage")
            return
        if str(target.get("status", "unknown")) == "shortage":
            shortage_resources = resources
            break

    if shortage_resources.is_empty():
        _fail_gift("autonomous shortage did not occur before Gift approach")
        return

    var resources_before_gift: Dictionary = {}
    var gift_result: Dictionary = {}
    var living_at_gift: Dictionary = {}
    var movement_before_gift: Dictionary = {}

    for _tick in range(GIFT_SCENARIO_LIMIT_TICKS):
        var intent: Vector2i = _main._intent_toward_rest_target()
        var pace: int = _main._pace_toward_rest_target()
        if not await _main._run_one_scripted_movement(intent, pace):
            _fail_gift("locomotion failed while approaching Gift target")
            return

        resources_before_gift = _sim.village_household_resource_projection()
        living_at_gift = _sim.living_need_projection()
        var movement_value = _main.get("_last_movement_batch")
        if typeof(movement_value) != TYPE_DICTIONARY:
            _fail_gift("Gift approach lost the authoritative movement batch")
            return
        movement_before_gift = movement_value.duplicate(true)

        var gift_response: Dictionary = _sim.controlled_actor_gift_grain(_target_household_id)
        if not bool(gift_response.get("ok", false)):
            if str(gift_response.get("error", "unknown")) == "outside_store":
                continue
            _fail_gift("Gift failed with %s" % str(gift_response.get("error", "unknown")))
            return

        gift_result = gift_response.get("result", {})
        break

    if gift_result.is_empty():
        _fail_gift("controlled actor did not reach exact Gift store tolerance")
        return
    if str(living_at_gift.get("status", "unknown")) != "blocked":
        _fail_gift("Gift occupancy did not preserve M1 rest interference")
        return

    var after_resources: Dictionary = _sim.village_household_resource_projection()
    var after_carry: Dictionary = _sim.controlled_actor_carry_projection()
    var after_target := _household_by_id(after_resources, _target_household_id)
    var before_target := _household_by_id(resources_before_gift, _target_household_id)
    if before_target.is_empty() or after_target.is_empty():
        _fail_gift("Gift target missing from before/after resource reads")
        return

    var moved := int(gift_result.get("moved_grain_units", -1))
    if (
        moved <= 0
        or int(after_carry.get("carried_grain_units", -1)) != 0
        or int(after_target.get("grain_stock_units", -1))
            != int(before_target.get("grain_stock_units", -1)) + moved
        or int(gift_result.get("tick", -1)) != int(resources_before_gift.get("tick", -2))
        or int(gift_result.get("revision", -1))
            != int(resources_before_gift.get("revision", -2)) + 1
    ):
        _fail_gift("Gift result does not match authoritative conservation/temporal state")
        return
    if str(before_target.get("status", "unknown")) != "shortage":
        _fail_gift("Gift target was no longer short before the transfer")
        return
    if str(after_target.get("status", "unknown")) != "adequate":
        _fail_gift("Gift did not relieve the bounded shortage target")
        return

    _main.set("_village_household_resource_projection", after_resources)
    _main.set("_living_need_projection", _sim.living_need_projection())
    if not _reconcile_after_resource_action():
        _fail_gift("failed to reconcile presentation to Gift revision")
        return
    _refresh_carry_hud()

    var resource_status_value = _main.get("debug_household_resource_status")
    var resource_status_text := ""
    if resource_status_value is Label:
        resource_status_text = resource_status_value.text

    var evidence := {
        "scenario": "gift",
        "client_supplied_amount": false,
        "source_household_id": source_household_id,
        "target_household_id": _target_household_id,
        "initial_carry": _carry_evidence(initial_carry),
        "initial_resource_header": _resource_header(initial_resources),
        "draw_result": draw_result,
        "shortage_before_approach": _household_evidence(
            _household_by_id(shortage_resources, _target_household_id)
        ),
        "shortage_resource_header": _resource_header(shortage_resources),
        "target_before_gift": _household_evidence(before_target),
        "resource_before_gift": _resource_header(resources_before_gift),
        "gift_result": gift_result,
        "target_after_gift": _household_evidence(after_target),
        "resource_after_gift": _resource_header(after_resources),
        "carry_after_gift": _carry_evidence(after_carry),
        "living_need_at_gift": {
            "entity_id": int(living_at_gift.get("entity_id", 0)),
            "status": str(living_at_gift.get("status", "unknown")),
            "tick": int(living_at_gift.get("tick", -1)),
            "revision": int(living_at_gift.get("revision", -1)),
            "protocol_version": int(living_at_gift.get("protocol_version", 0)),
        },
        "movement_before_gift": {
            "tick": int(movement_before_gift.get("tick", -1)),
            "revision": int(movement_before_gift.get("revision", -1)),
            "protocol_version": int(movement_before_gift.get("protocol_version", 0)),
        },
        "localization": {
            "locale": Localization.current_locale(),
            "scenario_text": tr(&"UI_SCENARIO_GIFT"),
            "carry_hud_text": _carry_label.text if _carry_label != null else "",
            "household_resource_status_text": resource_status_text,
        },
    }

    await RenderingServer.frame_post_draw
    if not _write_debug_artifact(artifact_dir, evidence):
        _fail_gift("failed to write Gift debug artifact")
        return
    if not _write_screenshot(artifact_dir):
        _fail_gift("failed to write Gift screenshot")
        return
    get_tree().quit(0)


func _household_by_id(projection: Dictionary, household_id: int) -> Dictionary:
    var households = projection.get("households", [])
    if typeof(households) != TYPE_ARRAY:
        return {}
    for value in households:
        if typeof(value) != TYPE_DICTIONARY:
            return {}
        var household: Dictionary = value
        if int(household.get("household_id", 0)) == household_id:
            return household
    return {}


func _household_evidence(household: Dictionary) -> Dictionary:
    return {
        "household_id": int(household.get("household_id", 0)),
        "grain_stock_units": int(household.get("grain_stock_units", -1)),
        "shortage_threshold_units": int(household.get("shortage_threshold_units", -1)),
        "status": str(household.get("status", "unknown")),
    }


func _resource_header(projection: Dictionary) -> Dictionary:
    return {
        "tick": int(projection.get("tick", -1)),
        "revision": int(projection.get("revision", -1)),
        "protocol_version": int(projection.get("protocol_version", 0)),
    }


func _carry_evidence(carry: Dictionary) -> Dictionary:
    return {
        "entity_id": int(carry.get("entity_id", 0)),
        "carried_grain_units": int(carry.get("carried_grain_units", -1)),
        "grain_carry_capacity_units": int(carry.get("grain_carry_capacity_units", -1)),
        "member_household_id": int(carry.get("member_household_id", 0)),
        "member_household_grain_stock_units": int(
            carry.get("member_household_grain_stock_units", -1)
        ),
        "tick": int(carry.get("tick", -1)),
        "revision": int(carry.get("revision", -1)),
        "protocol_version": int(carry.get("protocol_version", 0)),
    }


func _write_debug_artifact(artifact_dir: String, evidence: Dictionary) -> bool:
    var mkdir_error := DirAccess.make_dir_recursive_absolute(artifact_dir)
    if mkdir_error != OK:
        return false
    var file := FileAccess.open(artifact_dir.path_join("debug.json"), FileAccess.WRITE)
    if file == null:
        return false
    file.store_string(JSON.stringify(evidence, "  "))
    return true


func _write_screenshot(artifact_dir: String) -> bool:
    var image := get_viewport().get_texture().get_image()
    return image.save_png(artifact_dir.path_join("final.png")) == OK


func _fail_gift(message: String) -> void:
    push_error(message)
    get_tree().quit(14)


func _user_arg_value(name: String) -> String:
    var args := OS.get_cmdline_user_args()
    for index in range(args.size() - 1):
        if args[index] == name:
            return args[index + 1]
    return ""
