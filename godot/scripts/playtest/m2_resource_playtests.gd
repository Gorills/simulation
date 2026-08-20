class_name M2ResourcePlaytests
extends Node

const MOVE_INTENT_SCALE := 1000
const LOCOMOTION_PACE_WALK := 0
const LOCOMOTION_PACE_RUN := 1
const SHORTAGE_SCENARIO_LIMIT_TICKS := 480
const APPROACH_LIMIT_TICKS := 480
const ARRIVAL_HOLD_TICKS := 24

var _host: Node = null
var _resource_ui: ResourceInteraction = null


func configure(host: Node, resource_ui: ResourceInteraction) -> void:
    _host = host
    _resource_ui = resource_ui


func run(artifact_dir: String) -> void:
    await get_tree().process_frame
    if _host == null or _resource_ui == null:
        push_error("M2 resource playtests are not bound to the ordinary gameplay scene")
        get_tree().quit(14)
        return

    _host.disable_interactive_locomotion()
    _resource_ui.set_interactive(false)

    var scenario := _scenario_name()
    match scenario:
        "gift":
            await _run_gift(artifact_dir)
        "work":
            await _run_work(artifact_dir)
        "transfer":
            await _run_transfer(artifact_dir)
        _:
            push_error("M2 resource playtests received an unsupported scenario")
            get_tree().quit(14)


func _run_gift(artifact_dir: String) -> void:
    var sim: SimFacade = _host.simulation_facade()
    var source_household_id := int(
        sim.controlled_actor_carry_projection().get("member_household_id", 0)
    )
    var target_household_id: int = _host.tracked_neighbour_household_id()
    if source_household_id <= 0 or target_household_id <= 0 or source_household_id == target_household_id:
        push_error("Gift scenario cannot resolve distinct source/target households")
        get_tree().quit(14)
        return

    var initial_carry: Dictionary = _jsonable(sim.controlled_actor_carry_projection())
    var initial_header := _resource_header(_host.village_resource_projection())
    var draw_response: Dictionary = sim.controlled_actor_draw_grain()
    _resource_ui.show_command_outcome(draw_response)
    var draw := _accepted_result(draw_response, "Draw")
    if draw.is_empty():
        get_tree().quit(14)
        return
    if not _host.refresh_resource_views():
        get_tree().quit(14)
        return

    var draw_full := _refused_result(sim.controlled_actor_draw_grain(), "carry_full", "Draw")
    if draw_full.is_empty():
        get_tree().quit(14)
        return
    var own_household_gift := _refused_result(
        sim.controlled_actor_gift_grain(source_household_id),
        "own_household",
        "Gift"
    )
    if own_household_gift.is_empty():
        get_tree().quit(14)
        return

    var shortage := await _wait_for_target_shortage(target_household_id)
    if shortage.is_empty():
        get_tree().quit(14)
        return

    if not await _approach_rest_store():
        get_tree().quit(14)
        return
    if not _host.refresh_resource_views():
        get_tree().quit(14)
        return

    var living: Dictionary = _jsonable(_host.living_need_projection())
    if str(living.get("status", "unknown")) != "blocked":
        push_error("Gift at the short-household store did not preserve M1 rest interference")
        get_tree().quit(14)
        return

    var target_before := _household_evidence(_host.household_by_id(target_household_id))
    var before_header := _resource_header(_host.village_resource_projection())
    var movement: Dictionary = _host.movement_batch_evidence(_host.last_authoritative_movement_batch())
    var gift_response: Dictionary = sim.controlled_actor_gift_grain(target_household_id)
    _resource_ui.show_command_outcome(gift_response)
    if not _host.apply_latest_resource_projections():
        get_tree().quit(14)
        return
    var gift := _accepted_result(gift_response, "Gift")
    if gift.is_empty():
        get_tree().quit(14)
        return

    var target_after := _household_evidence(_host.household_by_id(target_household_id))
    var after_header := _resource_header(_host.village_resource_projection())
    var carry_after: Dictionary = _jsonable(sim.controlled_actor_carry_projection())
    var deposit_outside := _refused_result(
        sim.controlled_actor_deposit_grain(),
        "outside_store",
        "Deposit"
    )
    if deposit_outside.is_empty():
        get_tree().quit(14)
        return
    var gift_empty := _refused_result(
        sim.controlled_actor_gift_grain(target_household_id),
        "carry_empty",
        "Gift"
    )
    if gift_empty.is_empty():
        get_tree().quit(14)
        return
    _host.refresh_debug_overlay()
    _resource_ui.refresh()

    var evidence := {
        "scenario": "gift",
        "client_supplied_amount": false,
        "source_household_id": source_household_id,
        "target_household_id": target_household_id,
        "initial_carry": initial_carry,
        "initial_resource_header": initial_header,
        "draw_result": draw,
        "draw_full_refusal": draw_full,
        "own_household_gift_refusal": own_household_gift,
        "shortage_before_approach": shortage.get("household", {}),
        "shortage_resource_header": shortage.get("header", {}),
        "target_before_gift": target_before,
        "resource_before_gift": before_header,
        "gift_result": gift,
        "target_after_gift": target_after,
        "resource_after_gift": after_header,
        "carry_after_gift": carry_after,
        "deposit_outside_refusal": deposit_outside,
        "gift_empty_refusal": gift_empty,
        "living_need_at_gift": living,
        "movement_before_gift": movement,
        "localization": _localization_evidence(
            "%s · %s" % [_resource_ui.carry_hud_text(), _host.current_scenario_text()],
            "",
            "",
            _resource_ui.refusal_hud_text()
        ),
    }
    await _finish(artifact_dir, evidence)


func _run_work(artifact_dir: String) -> void:
    var sim: SimFacade = _host.simulation_facade()
    var initial_field: Dictionary = _jsonable(sim.field_work_projection())
    var target_household_id := int(initial_field.get("destination_household_id", 0))
    if target_household_id <= 0:
        push_error("Work scenario cannot resolve the durable destination household")
        get_tree().quit(15)
        return

    var initial_header := _resource_header(_host.village_resource_projection())
    var shortage := await _wait_for_target_shortage(target_household_id)
    if shortage.is_empty():
        get_tree().quit(15)
        return

    var field_position_value = sim.field_work_projection().get("work_position_m", null)
    if typeof(field_position_value) != TYPE_VECTOR3:
        push_error("Work scenario field projection has no authoritative work position")
        get_tree().quit(15)
        return
    var field_position: Vector3 = field_position_value
    if not await _approach_planar_target(Vector2(field_position.x, field_position.z)):
        get_tree().quit(15)
        return
    if not _host.refresh_resource_views():
        get_tree().quit(15)
        return

    var field_before: Dictionary = _jsonable(sim.field_work_projection())
    var target_before := _household_evidence(_host.household_by_id(target_household_id))
    var before_header := _resource_header(_host.village_resource_projection())
    var movement: Dictionary = _host.movement_batch_evidence(_host.last_authoritative_movement_batch())
    var work_response: Dictionary = sim.controlled_actor_complete_field_work()
    _resource_ui.show_command_outcome(work_response)
    if not _host.apply_latest_resource_projections():
        get_tree().quit(15)
        return
    var work := _accepted_result(work_response, "Work")
    if work.is_empty():
        get_tree().quit(15)
        return

    var field_after: Dictionary = _jsonable(sim.field_work_projection())
    var target_after := _household_evidence(_host.household_by_id(target_household_id))
    var after_header := _resource_header(_host.village_resource_projection())
    var exhausted: Dictionary = sim.controlled_actor_complete_field_work()
    _resource_ui.show_command_outcome(exhausted)
    if not _host.apply_latest_resource_projections():
        get_tree().quit(15)
        return
    var field_after_refusal: Dictionary = _jsonable(sim.field_work_projection())
    var resource_after_refusal := _resource_header(_host.village_resource_projection())
    _host.refresh_debug_overlay()
    _resource_ui.refresh()

    var evidence := {
        "scenario": "work",
        "client_supplied_amount": false,
        "client_supplied_yield": false,
        "client_supplied_destination": false,
        "target_household_id": target_household_id,
        "initial_field_work": initial_field,
        "initial_resource_header": initial_header,
        "shortage_before_approach": shortage.get("household", {}),
        "shortage_resource_header": shortage.get("header", {}),
        "field_before_work": field_before,
        "target_before_work": target_before,
        "resource_before_work": before_header,
        "movement_before_work": movement,
        "work_result": work,
        "field_after_work": field_after,
        "target_after_work": target_after,
        "resource_after_work": after_header,
        "exhausted_refusal": {
            "ok": bool(exhausted.get("ok", true)),
            "error": str(exhausted.get("error", "")),
        },
        "field_after_exhausted_refusal": field_after_refusal,
        "resource_after_exhausted_refusal": resource_after_refusal,
        "localization": _localization_evidence(
            "",
            "%s · %s" % [_resource_ui.work_hud_text(), _host.current_scenario_text()],
            _resource_ui.field_cue_text(),
            _resource_ui.refusal_hud_text()
        ),
    }
    await _finish(artifact_dir, evidence)


func _run_transfer(artifact_dir: String) -> void:
    var sim: SimFacade = _host.simulation_facade()
    var initial_pledge: Dictionary = _jsonable(sim.standing_transfer_pledge_projection())
    var source_household_id := int(initial_pledge.get("source_household_id", 0))
    var destination_household_id := int(initial_pledge.get("destination_household_id", 0))
    if source_household_id <= 0 or destination_household_id <= 0:
        push_error("Transfer scenario cannot resolve the standing pledge")
        get_tree().quit(16)
        return

    var initial_header := _resource_header(_host.village_resource_projection())
    var shortage := await _wait_for_target_shortage(destination_household_id)
    if shortage.is_empty():
        get_tree().quit(16)
        return
    if not await _host.run_one_scripted_movement(Vector2i.ZERO, LOCOMOTION_PACE_WALK):
        get_tree().quit(16)
        return
    if not _host.refresh_resource_views():
        get_tree().quit(16)
        return

    var pledge_before: Dictionary = _jsonable(sim.standing_transfer_pledge_projection())
    var source_before := _household_evidence(_host.household_by_id(source_household_id))
    var destination_before := _household_evidence(_host.household_by_id(destination_household_id))
    var before_header := _resource_header(_host.village_resource_projection())
    var movement: Dictionary = _host.movement_batch_evidence(_host.last_authoritative_movement_batch())
    var transfer_response: Dictionary = sim.controlled_actor_execute_household_transfer_pledge()
    _resource_ui.show_command_outcome(transfer_response)
    if not _host.apply_latest_resource_projections():
        get_tree().quit(16)
        return
    var transferred := _accepted_result(transfer_response, "Household transfer")
    if transferred.is_empty():
        get_tree().quit(16)
        return

    var pledge_after: Dictionary = _jsonable(sim.standing_transfer_pledge_projection())
    var source_after := _household_evidence(_host.household_by_id(source_household_id))
    var destination_after := _household_evidence(_host.household_by_id(destination_household_id))
    var after_header := _resource_header(_host.village_resource_projection())
    var exhausted: Dictionary = sim.controlled_actor_execute_household_transfer_pledge()
    _resource_ui.show_command_outcome(exhausted)
    if not _host.apply_latest_resource_projections():
        get_tree().quit(16)
        return
    var pledge_after_refusal: Dictionary = _jsonable(sim.standing_transfer_pledge_projection())
    var resource_after_refusal := _resource_header(_host.village_resource_projection())
    _host.refresh_debug_overlay()
    _resource_ui.refresh()

    var evidence := {
        "scenario": "transfer",
        "client_supplied_amount": false,
        "source_household_id": source_household_id,
        "destination_household_id": destination_household_id,
        "initial_pledge": initial_pledge,
        "initial_resource_header": initial_header,
        "shortage_before_execution": shortage.get("household", {}),
        "shortage_resource_header": shortage.get("header", {}),
        "pledge_before": pledge_before,
        "source_before": source_before,
        "destination_before": destination_before,
        "resource_before": before_header,
        "movement_before": movement,
        "transfer_result": transferred,
        "pledge_after": pledge_after,
        "source_after": source_after,
        "destination_after": destination_after,
        "resource_after": after_header,
        "exhausted_refusal": {
            "ok": bool(exhausted.get("ok", true)),
            "error": str(exhausted.get("error", "")),
        },
        "pledge_after_refusal": pledge_after_refusal,
        "resource_after_refusal": resource_after_refusal,
        "localization": _localization_evidence(
            "",
            "",
            "",
            _resource_ui.refusal_hud_text(),
            "%s · %s" % [_resource_ui.pledge_hud_text(), _host.current_scenario_text()]
        ),
    }
    await _finish(artifact_dir, evidence)


func _wait_for_target_shortage(target_household_id: int) -> Dictionary:
    for _tick in range(SHORTAGE_SCENARIO_LIMIT_TICKS):
        if not await _host.run_one_scripted_movement(Vector2i.ZERO, LOCOMOTION_PACE_WALK):
            push_error("M2 playtest locomotion failed while waiting for shortage")
            return {}
        if not _host.refresh_village_resources():
            return {}
        var household: Dictionary = _host.household_by_id(target_household_id)
        if household.is_empty():
            push_error("tracked household disappeared from authoritative discovery")
            return {}
        if str(household.get("status", "unknown")) == "shortage":
            return {
                "household": _household_evidence(household),
                "header": _resource_header(_host.village_resource_projection()),
            }
    push_error("autonomous NPC Consume did not produce shortage before the deadline")
    return {}


func _approach_rest_store() -> bool:
    var arrived := false
    for _tick in range(APPROACH_LIMIT_TICKS):
        var intent: Vector2i = _host.intent_toward_rest_target()
        var pace: int = _host.pace_toward_rest_target()
        if not await _host.run_one_scripted_movement(intent, pace):
            push_error("Gift approach locomotion failed")
            return false
        if intent == Vector2i.ZERO:
            arrived = true
            if str(_host.living_need_projection().get("status", "unknown")) == "blocked":
                return true
        elif arrived:
            arrived = false
    push_error("Gift scenario did not occupy the receiving store with RestNeed blocked")
    return false


func _approach_planar_target(target: Vector2) -> bool:
    for _tick in range(APPROACH_LIMIT_TICKS):
        var intent: Vector2i = _host.intent_toward_planar_target(target)
        var pace: int = _host.pace_toward_planar_target(target)
        if not await _host.run_one_scripted_movement(intent, pace):
            push_error("Work approach locomotion failed")
            return false
        if intent == Vector2i.ZERO:
            for _hold in range(ARRIVAL_HOLD_TICKS):
                if not await _host.run_one_scripted_movement(Vector2i.ZERO, LOCOMOTION_PACE_WALK):
                    return false
                if _host.intent_toward_planar_target(target) == Vector2i.ZERO:
                    return true
            return true
    push_error("Work scenario did not reach the projected field")
    return false


func _accepted_result(response: Dictionary, label: String) -> Dictionary:
    if not bool(response.get("ok", false)):
        push_error("%s refused: %s" % [label, str(response.get("error", "unknown"))])
        return {}
    var result_value = response.get("result", null)
    if typeof(result_value) != TYPE_DICTIONARY:
        push_error("%s result is missing" % label)
        return {}
    return _jsonable(result_value)


func _refused_result(response: Dictionary, expected_error: String, label: String) -> Dictionary:
    _resource_ui.show_command_outcome(response)
    if bool(response.get("ok", false)):
        push_error("%s unexpectedly succeeded" % label)
        return {}
    var error := str(response.get("error", ""))
    if error != expected_error:
        push_error("%s expected %s, got %s" % [label, expected_error, error])
        return {}
    return {
        "ok": false,
        "error": error,
        "refusal_hud_text": _resource_ui.refusal_hud_text(),
    }


func _resource_header(projection: Dictionary) -> Dictionary:
    return {
        "tick": int(projection.get("tick", -1)),
        "revision": int(projection.get("revision", -1)),
        "protocol_version": int(projection.get("protocol_version", 0)),
    }


func _household_evidence(household: Dictionary) -> Dictionary:
    if household.is_empty():
        return {}
    var store_value = household.get("store_position_m", null)
    var store := [0.0, 0.0, 0.0]
    if typeof(store_value) == TYPE_VECTOR3:
        var position: Vector3 = store_value
        store = [position.x, position.y, position.z]
    elif typeof(store_value) == TYPE_ARRAY:
        store = store_value
    return {
        "household_id": int(household.get("household_id", 0)),
        "member_actor_ids": household.get("member_actor_ids", []),
        "store_place_id": int(household.get("store_place_id", 0)),
        "store_position_m": store,
        "store_axis_tolerance_m": float(household.get("store_axis_tolerance_m", -1.0)),
        "grain_stock_units": int(household.get("grain_stock_units", -1)),
        "shortage_threshold_units": int(household.get("shortage_threshold_units", -1)),
        "status": str(household.get("status", "unknown")),
    }


func _localization_evidence(
    carry_hud_text: String,
    work_hud_text: String,
    field_cue_text: String,
    refusal_hud_text: String,
    pledge_hud_text: String = ""
) -> Dictionary:
    var evidence := {
        "locale": Localization.current_locale(),
        "supported_locales": Array(Localization.supported_locales()),
        "scenario_text": _host.current_scenario_text(),
        "household_resource_status_text": _host.household_resource_status_text(),
        "carry_hud_text": carry_hud_text,
        "work_hud_text": work_hud_text,
        "field_cue_text": field_cue_text,
        "refusal_hud_text": refusal_hud_text,
        "pledge_hud_text": pledge_hud_text,
    }
    return evidence


func _jsonable(value: Variant) -> Variant:
    match typeof(value):
        TYPE_VECTOR3:
            var vector: Vector3 = value
            return [vector.x, vector.y, vector.z]
        TYPE_DICTIONARY:
            var converted := {}
            for key in value:
                converted[key] = _jsonable(value[key])
            return converted
        TYPE_ARRAY:
            var converted: Array = []
            for item in value:
                converted.append(_jsonable(item))
            return converted
        _:
            return value


func _finish(artifact_dir: String, evidence: Dictionary) -> void:
    await RenderingServer.frame_post_draw
    if not _write_debug_artifact(artifact_dir, evidence):
        get_tree().quit(4)
        return
    if not _write_screenshot(artifact_dir):
        get_tree().quit(5)
        return
    get_tree().quit(0)


func _write_debug_artifact(artifact_dir: String, evidence: Dictionary) -> bool:
    var mkdir_error := DirAccess.make_dir_recursive_absolute(artifact_dir)
    if mkdir_error != OK:
        push_error("failed to create artifact directory: %s" % error_string(mkdir_error))
        return false
    var file := FileAccess.open(artifact_dir.path_join("debug.json"), FileAccess.WRITE)
    if file == null:
        push_error("failed to open debug artifact: %s" % error_string(FileAccess.get_open_error()))
        return false
    file.store_string(JSON.stringify(evidence, "  "))
    return true


func _write_screenshot(artifact_dir: String) -> bool:
    var image := get_viewport().get_texture().get_image()
    var save_error := image.save_png(artifact_dir.path_join("final.png"))
    if save_error != OK:
        push_error("failed to save screenshot: %s" % error_string(save_error))
        return false
    return true


func _scenario_name() -> String:
    var args := OS.get_cmdline_user_args()
    for index in range(args.size() - 1):
        if args[index] == "--scenario":
            return str(args[index + 1])
    return ""
