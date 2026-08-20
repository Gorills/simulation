extends Node

const DEBUG_REFRESH_INTERVAL_SECONDS := 0.25
const GIFT_SCENARIO_LIMIT_TICKS := 480
const WORK_SCENARIO_LIMIT_TICKS := 480
const MOVE_INTENT_SCALE := 1000
const LOCOMOTION_PACE_WALK := 0
const LOCOMOTION_PACE_RUN := 1
const FIELD_HOLD_TOLERANCE_M := 0.04
const FIELD_RUN_SWITCH_DISTANCE_M := 0.75

var _main
var _sim
var _carry_label: Label
var _work_label: Label
var _field_label: Label3D
var _refresh_elapsed := 0.0
var _scenario_name := "interactive"
var _target_household_id := 0
var _field_work_projection: Dictionary = {}
var _field_target_m := Vector2.ZERO


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

    if not _refresh_field_work_projection():
        push_error("field Work projection is unavailable")
        if _scenario_name == "work":
            get_tree().quit(15)
        return

    _install_carry_hud()
    _install_work_hud()
    _install_field_cue()
    _refresh_carry_hud()
    _refresh_work_hud()
    _refresh_field_cue()

    if not Localization.locale_changed.is_connected(_on_locale_changed):
        Localization.locale_changed.connect(_on_locale_changed)

    if _scenario_name in ["gift", "work"]:
        _main.set("_locomotion_runtime_enabled", false)
        var artifact_dir := _user_arg_value("--artifact-dir")
        if artifact_dir.is_empty():
            push_error("resource scenario requires --artifact-dir")
            get_tree().quit(14 if _scenario_name == "gift" else 15)
            return
        if _scenario_name == "gift":
            call_deferred("_run_gift_scenario", artifact_dir)
        else:
            call_deferred("_run_work_scenario", artifact_dir)


func _process(delta: float) -> void:
    if _main == null or _sim == null:
        return
    _refresh_elapsed += delta
    if _refresh_elapsed < DEBUG_REFRESH_INTERVAL_SECONDS:
        return
    _refresh_elapsed = 0.0
    _refresh_carry_hud()
    _refresh_work_hud()


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
    elif event.is_action_pressed(&"resource_work"):
        response = _sim.controlled_actor_complete_field_work()
    else:
        return

    if bool(response.get("ok", false)):
        if not _reconcile_after_resource_action():
            push_error("resource action committed but presentation reconciliation failed")
    _refresh_carry_hud()
    _refresh_work_hud()
    get_viewport().set_input_as_handled()


func _on_locale_changed(_locale: String) -> void:
    _refresh_carry_hud()
    _refresh_work_hud()
    _refresh_field_cue()


func _install_carry_hud() -> void:
    var resource_status = _main.get("debug_household_resource_status")
    if resource_status == null or not resource_status is Label:
        return
    _carry_label = Label.new()
    _carry_label.name = "DebugCarryStatus"
    _carry_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
    resource_status.get_parent().add_child(_carry_label)


func _install_work_hud() -> void:
    var resource_status = _main.get("debug_household_resource_status")
    if resource_status == null or not resource_status is Label:
        return
    _work_label = Label.new()
    _work_label.name = "DebugFieldWorkStatus"
    _work_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
    resource_status.get_parent().add_child(_work_label)


func _install_field_cue() -> void:
    if _main == null or _field_work_projection.is_empty():
        return
    var position_value = _field_work_projection.get("work_position_m", null)
    if typeof(position_value) != TYPE_VECTOR3:
        return
    _field_label = Label3D.new()
    _field_label.name = "FieldWorkCue"
    _field_label.position = position_value + Vector3(0.0, 0.8, 0.0)
    _field_label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
    _field_label.font_size = 32
    _field_label.outline_size = 8
    _main.add_child(_field_label)


func _refresh_field_cue() -> void:
    if _field_label == null:
        return
    _field_label.text = tr(&"UI_FIELD_WORK_CUE")


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


func _refresh_work_hud() -> void:
    if _work_label == null or _sim == null:
        return
    if not _refresh_field_work_projection():
        _work_label.text = "—"
        return
    var remaining := int(_field_work_projection.get("remaining_work_completions", -1))
    if remaining < 0:
        _work_label.text = "—"
        return
    var scenario_text := ""
    if _scenario_name == "work":
        scenario_text = " · %s" % tr(&"UI_SCENARIO_WORK")
    _work_label.text = "%s: %d · %s%s" % [
        tr(&"UI_DEBUG_FIELD_WORK"),
        remaining,
        tr(&"UI_WORK_ACTION_HINT"),
        scenario_text,
    ]


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
    var position: Vector3 = position_value
    _field_target_m = Vector2(position.x, position.z)
    return true


func _reconcile_after_resource_action() -> bool:
    var observed: Dictionary = _sim.observed_world_projection()
    var world_presentation = _main.get("world_presentation")
    var player_entity_binding = _main.get("player_entity_binding")
    if not world_presentation.apply_observed_world_projection(observed, player_entity_binding):
        return false
    _main.set("_observed_world_projection", observed)
    _main.set("_village_household_resource_projection", _sim.village_household_resource_projection())
    _main.set("_living_need_projection", _sim.living_need_projection())
    if not _refresh_field_work_projection():
        return false
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
    _refresh_work_hud()

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


func _run_work_scenario(artifact_dir: String) -> void:
    await get_tree().process_frame

    if not _refresh_field_work_projection():
        _fail_work("Work scenario cannot read authoritative field assignment")
        return
    var initial_field := _field_work_projection.duplicate(true)
    if (
        int(initial_field.get("destination_household_id", 0)) != _target_household_id
        or int(initial_field.get("yield_grain_units", 0)) <= 0
        or int(initial_field.get("remaining_work_completions", 0)) <= 0
    ):
        _fail_work("Work scenario field assignment does not target the configured short household")
        return

    var initial_resources: Dictionary = _sim.village_household_resource_projection()
    if _household_by_id(initial_resources, _target_household_id).is_empty():
        _fail_work("Work destination household missing from initial discovery")
        return

    var shortage_resources: Dictionary = {}
    for _tick in range(WORK_SCENARIO_LIMIT_TICKS):
        if not await _main._run_one_scripted_movement(Vector2i.ZERO, LOCOMOTION_PACE_WALK):
            _fail_work("locomotion failed while waiting for autonomous shortage")
            return
        var resources: Dictionary = _sim.village_household_resource_projection()
        var target := _household_by_id(resources, _target_household_id)
        if target.is_empty():
            _fail_work("Work destination disappeared while waiting for shortage")
            return
        if str(target.get("status", "unknown")) == "shortage":
            shortage_resources = resources
            break

    if shortage_resources.is_empty():
        _fail_work("autonomous shortage did not occur before Work approach")
        return

    var field_before_work: Dictionary = {}
    var resources_before_work: Dictionary = {}
    var work_result: Dictionary = {}
    var movement_before_work: Dictionary = {}

    for _tick in range(WORK_SCENARIO_LIMIT_TICKS):
        var intent := _intent_toward_field()
        var pace := _pace_toward_field()
        if not await _main._run_one_scripted_movement(intent, pace):
            _fail_work("locomotion failed while approaching field")
            return

        if not _refresh_field_work_projection():
            _fail_work("field projection failed during Work approach")
            return
        field_before_work = _field_work_projection.duplicate(true)
        resources_before_work = _sim.village_household_resource_projection()
        var movement_value = _main.get("_last_movement_batch")
        if typeof(movement_value) != TYPE_DICTIONARY:
            _fail_work("Work approach lost the authoritative movement batch")
            return
        movement_before_work = movement_value.duplicate(true)

        var work_response: Dictionary = _sim.controlled_actor_complete_field_work()
        if not bool(work_response.get("ok", false)):
            if str(work_response.get("error", "unknown")) == "outside_field":
                continue
            _fail_work("Work failed with %s" % str(work_response.get("error", "unknown")))
            return
        work_result = work_response.get("result", {})
        break

    if work_result.is_empty():
        _fail_work("controlled actor did not reach exact field tolerance")
        return

    var after_resources: Dictionary = _sim.village_household_resource_projection()
    if not _refresh_field_work_projection():
        _fail_work("field projection failed after Work")
        return
    var field_after_work := _field_work_projection.duplicate(true)
    var before_target := _household_by_id(resources_before_work, _target_household_id)
    var after_target := _household_by_id(after_resources, _target_household_id)
    if before_target.is_empty() or after_target.is_empty():
        _fail_work("Work destination missing from before/after resource reads")
        return

    var produced := int(work_result.get("produced_grain_units", -1))
    if (
        produced <= 0
        or produced != int(field_before_work.get("yield_grain_units", -1))
        or int(work_result.get("destination_household_id", 0)) != _target_household_id
        or int(after_target.get("grain_stock_units", -1))
            != int(before_target.get("grain_stock_units", -1)) + produced
        or int(field_after_work.get("remaining_work_completions", -1))
            != int(field_before_work.get("remaining_work_completions", -1)) - 1
        or int(work_result.get("remaining_work_completions", -1))
            != int(field_after_work.get("remaining_work_completions", -2))
        or int(work_result.get("tick", -1)) != int(field_before_work.get("tick", -2))
        or int(work_result.get("revision", -1))
            != int(field_before_work.get("revision", -2)) + 1
    ):
        _fail_work("Work result does not match authoritative yield/availability/temporal state")
        return
    if str(before_target.get("status", "unknown")) != "shortage":
        _fail_work("Work destination was not short immediately before production")
        return
    if str(after_target.get("status", "unknown")) != "adequate":
        _fail_work("Work did not relieve the bounded shortage destination")
        return
    if int(field_after_work.get("remaining_work_completions", -1)) != 0:
        _fail_work("single acceptance Work completion did not exhaust availability")
        return

    var exhausted_response: Dictionary = _sim.controlled_actor_complete_field_work()
    if (
        bool(exhausted_response.get("ok", false))
        or str(exhausted_response.get("error", "unknown")) != "work_exhausted"
    ):
        _fail_work("second Work did not refuse exhausted authoritative availability")
        return
    var after_exhausted_resources: Dictionary = _sim.village_household_resource_projection()
    var after_exhausted_field: Dictionary = _sim.field_work_projection()
    if (
        int(after_exhausted_field.get("revision", -1))
            != int(field_after_work.get("revision", -2))
        or int(after_exhausted_resources.get("revision", -1))
            != int(after_resources.get("revision", -2))
        or int(after_exhausted_field.get("remaining_work_completions", -1)) != 0
    ):
        _fail_work("exhausted Work refusal mutated authoritative state")
        return

    _main.set("_village_household_resource_projection", after_resources)
    _main.set("_living_need_projection", _sim.living_need_projection())
    if not _reconcile_after_resource_action():
        _fail_work("failed to reconcile presentation to Work revision")
        return
    _refresh_carry_hud()
    _refresh_work_hud()
    _refresh_field_cue()

    var resource_status_value = _main.get("debug_household_resource_status")
    var resource_status_text := ""
    if resource_status_value is Label:
        resource_status_text = resource_status_value.text

    var evidence := {
        "scenario": "work",
        "client_supplied_amount": false,
        "client_supplied_yield": false,
        "client_supplied_destination": false,
        "target_household_id": _target_household_id,
        "initial_field_work": _field_evidence(initial_field),
        "initial_resource_header": _resource_header(initial_resources),
        "shortage_before_approach": _household_evidence(
            _household_by_id(shortage_resources, _target_household_id)
        ),
        "shortage_resource_header": _resource_header(shortage_resources),
        "field_before_work": _field_evidence(field_before_work),
        "target_before_work": _household_evidence(before_target),
        "resource_before_work": _resource_header(resources_before_work),
        "movement_before_work": {
            "tick": int(movement_before_work.get("tick", -1)),
            "revision": int(movement_before_work.get("revision", -1)),
            "protocol_version": int(movement_before_work.get("protocol_version", 0)),
        },
        "work_result": work_result,
        "field_after_work": _field_evidence(field_after_work),
        "target_after_work": _household_evidence(after_target),
        "resource_after_work": _resource_header(after_resources),
        "exhausted_refusal": exhausted_response,
        "field_after_exhausted_refusal": _field_evidence(after_exhausted_field),
        "resource_after_exhausted_refusal": _resource_header(after_exhausted_resources),
        "localization": {
            "locale": Localization.current_locale(),
            "scenario_text": tr(&"UI_SCENARIO_WORK"),
            "work_hud_text": _work_label.text if _work_label != null else "",
            "field_cue_text": _field_label.text if _field_label != null else "",
            "household_resource_status_text": resource_status_text,
        },
    }

    await RenderingServer.frame_post_draw
    if not _write_debug_artifact(artifact_dir, evidence):
        _fail_work("failed to write Work debug artifact")
        return
    if not _write_screenshot(artifact_dir):
        _fail_work("failed to write Work screenshot")
        return
    get_tree().quit(0)


func _controlled_planar_position() -> Vector2:
    var spatial: Dictionary = _sim.controlled_actor_spatial_projection()
    var position_value = spatial.get("position_m", null)
    if typeof(position_value) != TYPE_VECTOR3:
        return Vector2.ZERO
    var position: Vector3 = position_value
    return Vector2(position.x, position.z)


func _intent_toward_field() -> Vector2i:
    var delta := _field_target_m - _controlled_planar_position()
    if abs(delta.x) <= FIELD_HOLD_TOLERANCE_M and abs(delta.y) <= FIELD_HOLD_TOLERANCE_M:
        return Vector2i.ZERO
    if delta.is_zero_approx():
        return Vector2i.ZERO
    var direction := delta.normalized()
    return Vector2i(
        int(direction.x * MOVE_INTENT_SCALE),
        int(direction.y * MOVE_INTENT_SCALE)
    )


func _pace_toward_field() -> int:
    var distance := (_field_target_m - _controlled_planar_position()).length()
    return LOCOMOTION_PACE_RUN if distance > FIELD_RUN_SWITCH_DISTANCE_M else LOCOMOTION_PACE_WALK


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


func _field_evidence(field: Dictionary) -> Dictionary:
    var position_value = field.get("work_position_m", Vector3.ZERO)
    var position := Vector3.ZERO
    if typeof(position_value) == TYPE_VECTOR3:
        position = position_value
    return {
        "work_place_id": int(field.get("work_place_id", 0)),
        "work_position_m": [position.x, position.y, position.z],
        "work_axis_tolerance_m": float(field.get("work_axis_tolerance_m", -1.0)),
        "destination_household_id": int(field.get("destination_household_id", 0)),
        "yield_grain_units": int(field.get("yield_grain_units", -1)),
        "remaining_work_completions": int(field.get("remaining_work_completions", -1)),
        "tick": int(field.get("tick", -1)),
        "revision": int(field.get("revision", -1)),
        "protocol_version": int(field.get("protocol_version", 0)),
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


func _fail_work(message: String) -> void:
    push_error(message)
    get_tree().quit(15)


func _user_arg_value(name: String) -> String:
    var args := OS.get_cmdline_user_args()
    for index in range(args.size() - 1):
        if args[index] == name:
            return args[index + 1]
    return ""
