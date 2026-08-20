extends SceneTree

const MOVE_INTENT_SCALE := 1000
const LOCOMOTION_PACE_WALK := 0
const LOCOMOTION_PACE_RUN := 1
const OPPORTUNITY_LIMIT := 480


func _initialize() -> void:
    call_deferred("_run")


func _run() -> void:
    if not _run_helped_branch():
        quit(31)
        return
    if not _run_control_branch():
        quit(32)
        return
    print("M3 reciprocal aid bridge: helped/control passed")
    quit(0)


func _run_helped_branch() -> bool:
    var sim := SimFacade.new()
    var target_id := _target_household_id(sim)
    if target_id <= 0:
        return _fail("helped: cannot resolve target household")

    var memory_before := _projection(sim, target_id)
    if memory_before.is_empty() or bool(memory_before.get("remembered_for_controlled_actor", true)):
        return _fail("helped: target unexpectedly remembers actor at startup")

    var draw: Dictionary = sim.controlled_actor_draw_grain()
    if not bool(draw.get("ok", false)):
        return _fail("helped: draw failed")
    if not _wait_for_shortage(sim, target_id):
        return false
    if not _reach_store(sim, target_id):
        return false

    var gift: Dictionary = sim.controlled_actor_gift_grain(target_id)
    if not bool(gift.get("ok", false)):
        return _fail("helped: qualifying gift failed")
    var memory_after_gift := _projection(sim, target_id)
    if memory_after_gift.is_empty() or not bool(
        memory_after_gift.get("remembered_for_controlled_actor", false)
    ):
        return _fail("helped: qualifying gift is not visible as remembered aid")

    var aid: Dictionary = sim.controlled_actor_request_reciprocal_aid(target_id)
    if not bool(aid.get("ok", false)):
        return _fail("helped: reciprocal aid request failed: %s" % str(aid.get("error", "")))
    var result_value = aid.get("result", null)
    if typeof(result_value) != TYPE_DICTIONARY:
        return _fail("helped: reciprocal aid has no result dictionary")
    var result: Dictionary = result_value
    if int(result.get("received_grain_units", 0)) != 1:
        return _fail("helped: reciprocal aid moved an unexpected amount")
    if int(result.get("remaining_household_grain_stock_units", -1)) != 2:
        return _fail("helped: repayment did not preserve target shortage threshold")

    var memory_after_aid := _projection(sim, target_id)
    if memory_after_aid.is_empty() or bool(
        memory_after_aid.get("remembered_for_controlled_actor", true)
    ):
        return _fail("helped: reciprocal aid did not consume remembered favour")

    var second: Dictionary = sim.controlled_actor_request_reciprocal_aid(target_id)
    if bool(second.get("ok", true)) or str(second.get("error", "")) != "no_remembered_aid":
        return _fail("helped: consumed favour did not refuse a second request")
    return true


func _run_control_branch() -> bool:
    var sim := SimFacade.new()
    var target_id := _target_household_id(sim)
    if target_id <= 0:
        return _fail("control: cannot resolve target household")
    if not _wait_for_shortage(sim, target_id):
        return false

    var transfer: Dictionary = sim.controlled_actor_execute_household_transfer_pledge()
    if not bool(transfer.get("ok", false)):
        return _fail("control: standing household transfer failed")
    if not _reach_store(sim, target_id):
        return false

    var memory := _projection(sim, target_id)
    if memory.is_empty() or bool(memory.get("remembered_for_controlled_actor", true)):
        return _fail("control: household transfer incorrectly created personal remembered aid")

    var target := _household_by_id(sim.village_household_resource_projection(), target_id)
    var carry: Dictionary = sim.controlled_actor_carry_projection()
    if target.is_empty():
        return _fail("control: target household disappeared")
    if int(target.get("grain_stock_units", 0)) <= int(target.get("shortage_threshold_units", 0)):
        return _fail("control: target is not materially able to spare grain")
    if int(carry.get("carried_grain_units", -1)) >= int(carry.get("grain_carry_capacity_units", -1)):
        return _fail("control: actor has no free carry capacity")

    var refused: Dictionary = sim.controlled_actor_request_reciprocal_aid(target_id)
    if bool(refused.get("ok", true)) or str(refused.get("error", "")) != "no_remembered_aid":
        return _fail("control: materially feasible request was not refused for social reason")
    return true


func _target_household_id(sim: SimFacade) -> int:
    var carry: Dictionary = sim.controlled_actor_carry_projection()
    var member_household_id := int(carry.get("member_household_id", 0))
    var projection: Dictionary = sim.village_household_resource_projection()
    var households_value = projection.get("households", null)
    if member_household_id <= 0 or typeof(households_value) != TYPE_ARRAY:
        return 0
    for household_value in households_value:
        if typeof(household_value) != TYPE_DICTIONARY:
            continue
        var household: Dictionary = household_value
        var household_id := int(household.get("household_id", 0))
        if household_id > 0 and household_id != member_household_id:
            return household_id
    return 0


func _projection(sim: SimFacade, household_id: int) -> Dictionary:
    var response: Dictionary = sim.reciprocal_aid_projection(household_id)
    if not bool(response.get("ok", false)):
        _fail("projection failed: %s" % str(response.get("error", "")))
        return {}
    var projection_value = response.get("projection", null)
    if typeof(projection_value) != TYPE_DICTIONARY:
        _fail("projection result is not a dictionary")
        return {}
    return projection_value


func _household_by_id(projection: Dictionary, household_id: int) -> Dictionary:
    var households_value = projection.get("households", null)
    if typeof(households_value) != TYPE_ARRAY:
        return {}
    for household_value in households_value:
        if typeof(household_value) != TYPE_DICTIONARY:
            continue
        var household: Dictionary = household_value
        if int(household.get("household_id", 0)) == household_id:
            return household
    return {}


func _wait_for_shortage(sim: SimFacade, household_id: int) -> bool:
    for _opportunity in range(OPPORTUNITY_LIMIT):
        var submit: Dictionary = sim.controlled_actor_submit_move_intent(0, 0, LOCOMOTION_PACE_WALK)
        if not bool(submit.get("ok", false)):
            return _fail("wait: idle movement intent failed")
        var movement: Dictionary = sim.advance_locomotion_tick()
        if not bool(movement.get("ok", false)):
            return _fail("wait: locomotion tick failed")
        var household := _household_by_id(sim.village_household_resource_projection(), household_id)
        if household.is_empty():
            return _fail("wait: target household disappeared")
        if str(household.get("status", "")) == "shortage":
            return true
    return _fail("wait: target household did not become short")


func _reach_store(sim: SimFacade, household_id: int) -> bool:
    for _opportunity in range(OPPORTUNITY_LIMIT):
        var household := _household_by_id(sim.village_household_resource_projection(), household_id)
        if household.is_empty():
            return _fail("reach: target household disappeared")
        var spatial: Dictionary = sim.controlled_actor_spatial_projection()
        if _at_store(spatial, household):
            return true
        var intent := _intent_toward(spatial, household)
        var submit: Dictionary = sim.controlled_actor_submit_move_intent(
            int(intent.x),
            int(intent.y),
            LOCOMOTION_PACE_RUN
        )
        if not bool(submit.get("ok", false)):
            return _fail("reach: movement intent failed")
        var movement: Dictionary = sim.advance_locomotion_tick()
        if not bool(movement.get("ok", false)):
            return _fail("reach: locomotion tick failed")
    return _fail("reach: actor did not reach target store")


func _intent_toward(spatial: Dictionary, household: Dictionary) -> Vector2i:
    var position_value = spatial.get("position_m", null)
    var store_value = household.get("store_position_m", null)
    if typeof(position_value) != TYPE_VECTOR3 or typeof(store_value) != TYPE_VECTOR3:
        return Vector2i.ZERO
    var position: Vector3 = position_value
    var store: Vector3 = store_value
    var tolerance := float(household.get("store_axis_tolerance_m", 0.0))
    var dx := store.x - position.x
    var dz := store.z - position.z
    var x := 0 if absf(dx) <= tolerance else (-1 if dx < 0.0 else 1)
    var z := 0 if absf(dz) <= tolerance else (-1 if dz < 0.0 else 1)
    if x != 0 and z != 0:
        return Vector2i(x * 707, z * 707)
    return Vector2i(x * MOVE_INTENT_SCALE, z * MOVE_INTENT_SCALE)


func _at_store(spatial: Dictionary, household: Dictionary) -> bool:
    var position_value = spatial.get("position_m", null)
    var store_value = household.get("store_position_m", null)
    if typeof(position_value) != TYPE_VECTOR3 or typeof(store_value) != TYPE_VECTOR3:
        return false
    var position: Vector3 = position_value
    var store: Vector3 = store_value
    var tolerance := float(household.get("store_axis_tolerance_m", -1.0))
    return tolerance >= 0.0 \
        and absf(position.x - store.x) <= tolerance \
        and absf(position.z - store.z) <= tolerance


func _fail(message: String) -> bool:
    push_error("M3 reciprocal aid bridge: %s" % message)
    return false
