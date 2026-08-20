extends Node3D

const DEBUG_REFRESH_INTERVAL_SECONDS := 0.25
const MOVE_INTENT_SCALE := 1000
const LOCOMOTION_PACE_WALK := 0
const LOCOMOTION_PACE_RUN := 1
const LOCOMOTION_PACE_SPRINT := 2
const REST_HOLD_TOLERANCE_M := 0.04
const REST_RUN_SWITCH_DISTANCE_M := 0.75
const REST_INTERFERENCE_APPROACH_LIMIT_TICKS := 480
const REST_INTERFERENCE_RELEASE_LIMIT_TICKS := 120
const SHORTAGE_SCENARIO_LIMIT_TICKS := 480

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
@onready var debug_living_need_status: Label = %DebugLivingNeedStatus
@onready var debug_household_resource_status: Label = %DebugHouseholdResourceStatus
@onready var debug_authority_position: Label = %DebugAuthorityPosition
@onready var debug_epoch: Label = %DebugEpoch
@onready var debug_presentation_position: Label = %DebugPresentationPosition
@onready var debug_divergence: Label = %DebugDivergence

var sim := SimFacade.new()
var _bootstrap_projection: Dictionary = {}
var _initial_village_household_resource_projection: Dictionary = {}
var _village_household_resource_projection: Dictionary = {}
var _observed_world_projection: Dictionary = {}
var _controlled_actor_spatial_projection: Dictionary = {}
var _living_need_projection: Dictionary = {}
var _last_movement_batch: Dictionary = {}
var _offscreen_evidence: Dictionary = {}
var _rest_interference_evidence: Dictionary = {}
var _shortage_evidence: Dictionary = {}
var _living_need_actor_id := 0
var _tracked_household_id := 0
var _rest_target_m := Vector2.ZERO
var _debug_refresh_elapsed := 0.0
var _scenario_name := "interactive"
var _locomotion_runtime_enabled := false
var _scripted_movement_requested := false
var _scripted_movement_finished := false
var _scripted_movement_succeeded := false
var _scripted_movement_intent := Vector2i.ZERO
var _scripted_movement_pace := LOCOMOTION_PACE_RUN
var _duplicate_movement_batch_rejected := false


func _ready() -> void:
    _initial_village_household_resource_projection = sim.village_household_resource_projection()
    _village_household_resource_projection = _initial_village_household_resource_projection.duplicate(true)

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

    _living_need_projection = sim.living_need_projection()
    if not _validate_startup_projection_revision() or not _resolve_acceptance_projection_bindings():
        get_tree().quit(12)
        return

    camera_rig.configure(controls, player)
    controls.input_device_changed.connect(_on_input_device_changed)
    Localization.locale_changed.connect(_on_locale_changed)
    controls.capture_pointer()

    _set_bootstrap_projection(sim.bootstrap_debug_projection())
    var scenario := _user_arg_value("--scenario")
    if not scenario.is_empty():
        _scenario_name = scenario
    _refresh_debug_hud()

    if scenario in ["smoke", "offscreen", "rest_interference", "shortage"]:
        var artifact_dir := _user_arg_value("--artifact-dir")
        if artifact_dir.is_empty():
            push_error("bounded playtest scenario requires --artifact-dir")
            get_tree().quit(2)
            return
        match scenario:
            "offscreen":
                call_deferred("_run_offscreen", artifact_dir)
            "rest_interference":
                call_deferred("_run_rest_interference", artifact_dir)
            "shortage":
                call_deferred("_run_shortage", artifact_dir)
            _:
                call_deferred("_run_smoke", artifact_dir)
        return

    _locomotion_runtime_enabled = true


func _physics_process(_delta: float) -> void:
    if _scripted_movement_requested:
        _scripted_movement_requested = false
        _scripted_movement_succeeded = _advance_authoritative_locomotion(
            _scripted_movement_intent,
            _scripted_movement_pace
        )
        _scripted_movement_finished = true
        return

    if not _locomotion_runtime_enabled:
        return

    var intent := _camera_relative_move_intent()
    var pace := LOCOMOTION_PACE_SPRINT if controls.is_sprinting() else LOCOMOTION_PACE_RUN
    if not _advance_authoritative_locomotion(intent, pace):
        _locomotion_runtime_enabled = false
        push_error("authoritative locomotion disabled after bridge failure")


func _process(delta: float) -> void:
    _debug_refresh_elapsed += delta
    if _debug_refresh_elapsed < DEBUG_REFRESH_INTERVAL_SECONDS:
        return
    _debug_refresh_elapsed = 0.0
    _refresh_debug_hud()


func _on_input_device_changed(_device: int) -> void:
    _refresh_debug_hud()


func _on_locale_changed(_locale: String) -> void:
    _refresh_debug_hud()


func _set_bootstrap_projection(projection: Dictionary) -> void:
    _bootstrap_projection = projection


func _validate_startup_projection_revision() -> bool:
    var projections := [
        _initial_village_household_resource_projection,
        _observed_world_projection,
        _controlled_actor_spatial_projection,
        _living_need_projection,
    ]
    var expected_tick := int(_initial_village_household_resource_projection.get("tick", -1))
    var expected_revision := int(_initial_village_household_resource_projection.get("revision", -1))
    var expected_protocol := int(
        _initial_village_household_resource_projection.get("protocol_version", 0)
    )
    if expected_tick < 0 or expected_revision < 0 or expected_protocol <= 0:
        push_error("initial village resource projection has an invalid temporal header")
        return false
    for projection_value in projections:
        if typeof(projection_value) != TYPE_DICTIONARY:
            push_error("startup projection is not a Dictionary")
            return false
        var projection: Dictionary = projection_value
        if (
            int(projection.get("tick", -1)) != expected_tick
            or int(projection.get("revision", -1)) != expected_revision
            or int(projection.get("protocol_version", 0)) != expected_protocol
        ):
            push_error("startup projections are not from one unchanged world revision")
            return false
    return true


func _resolve_acceptance_projection_bindings() -> bool:
    _living_need_actor_id = int(_living_need_projection.get("entity_id", 0))
    var target_value = _living_need_projection.get("target_position_m", null)
    if _living_need_actor_id <= 0 or typeof(target_value) != TYPE_VECTOR3:
        push_error("living-need projection cannot resolve the acceptance actor/target")
        return false

    var household := _household_for_member(
        _initial_village_household_resource_projection,
        _living_need_actor_id
    )
    if household.is_empty():
        push_error("living-need actor is not present in authoritative household discovery")
        return false

    var store_value = household.get("store_position_m", null)
    if typeof(store_value) != TYPE_VECTOR3:
        push_error("tracked household has no authoritative store position")
        return false
    var target: Vector3 = target_value
    var store: Vector3 = store_value
    if not is_equal_approx(target.x, store.x) or not is_equal_approx(target.z, store.z):
        push_error("RestNeed target and household store are not the same Core content fact")
        return false

    _tracked_household_id = int(household.get("household_id", 0))
    if _tracked_household_id <= 0:
        push_error("tracked household has an invalid EntityId")
        return false
    _rest_target_m = Vector2(target.x, target.z)
    return true


func _household_for_member(projection: Dictionary, actor_id: int) -> Dictionary:
    var households_value = projection.get("households", null)
    if typeof(households_value) != TYPE_ARRAY or actor_id <= 0:
        return {}
    for household_value in households_value:
        if typeof(household_value) != TYPE_DICTIONARY:
            return {}
        var household: Dictionary = household_value
        var members_value = household.get("member_actor_ids", null)
        if typeof(members_value) != TYPE_ARRAY:
            return {}
        for member_value in members_value:
            if int(member_value) == actor_id:
                return household
    return {}


func _household_by_id(projection: Dictionary, household_id: int) -> Dictionary:
    var households_value = projection.get("households", null)
    if typeof(households_value) != TYPE_ARRAY or household_id <= 0:
        return {}
    for household_value in households_value:
        if typeof(household_value) != TYPE_DICTIONARY:
            return {}
        var household: Dictionary = household_value
        if int(household.get("household_id", 0)) == household_id:
            return household
    return {}


func _refresh_village_household_resource_projection() -> bool:
    var projection: Dictionary = sim.village_household_resource_projection()
    if typeof(projection.get("households", null)) != TYPE_ARRAY:
        push_error("village household resource bridge read is invalid")
        return false
    _village_household_resource_projection = projection
    return true


func _advance_authoritative_locomotion(intent: Vector2i, pace: int) -> bool:
    var submitted: Dictionary = sim.controlled_actor_submit_move_intent(intent.x, intent.y, pace)
    if not bool(submitted.get("ok", false)):
        push_error(
            "controlled move intent rejected: %s"
            % str(submitted.get("error", "unknown_error"))
        )
        return false

    var advanced: Dictionary = sim.advance_locomotion_tick()
    if not bool(advanced.get("ok", false)):
        push_error(
            "authoritative locomotion tick failed: %s"
            % str(advanced.get("error", "unknown_error"))
        )
        return false

    var batch_value = advanced.get("batch", null)
    if typeof(batch_value) != TYPE_DICTIONARY:
        push_error("authoritative locomotion response is missing a movement batch")
        return false
    var batch: Dictionary = batch_value
    if not world_presentation.apply_authoritative_movement_sample_batch(batch):
        push_error("WorldPresentation rejected the authoritative movement batch")
        return false

    var controlled_projection := _controlled_projection_from_batch(batch)
    if controlled_projection.is_empty():
        push_error("movement batch does not expose the controlled actor sample")
        return false

    _last_movement_batch = batch
    _controlled_actor_spatial_projection = controlled_projection
    _living_need_projection = sim.living_need_projection()
    return true


func _run_one_scripted_movement(intent: Vector2i, pace: int) -> bool:
    _scripted_movement_finished = false
    _scripted_movement_succeeded = false
    _scripted_movement_intent = intent
    _scripted_movement_pace = pace
    _scripted_movement_requested = true
    while not _scripted_movement_finished:
        await get_tree().physics_frame
    return _scripted_movement_succeeded


func _run_one_smoke_movement() -> bool:
    return await _run_one_scripted_movement(
        Vector2i(MOVE_INTENT_SCALE, 0),
        LOCOMOTION_PACE_RUN
    )


func _camera_relative_move_intent() -> Vector2i:
    var input_axis := controls.move_axis()
    if input_axis.is_zero_approx():
        return Vector2i.ZERO

    var camera_basis := camera_rig.get_camera().global_transform.basis
    var camera_forward := -camera_basis.z
    var camera_right := camera_basis.x
    camera_forward.y = 0.0
    camera_right.y = 0.0
    if camera_forward.is_zero_approx() or camera_right.is_zero_approx():
        return Vector2i.ZERO
    camera_forward = camera_forward.normalized()
    camera_right = camera_right.normalized()

    var world_direction := camera_right * input_axis.x + camera_forward * -input_axis.y
    var planar := Vector2(world_direction.x, world_direction.z)
    if planar.length() > 1.0:
        planar = planar.normalized()

    return Vector2i(
        int(planar.x * MOVE_INTENT_SCALE),
        int(planar.y * MOVE_INTENT_SCALE)
    )


func _controlled_projection_from_batch(batch: Dictionary) -> Dictionary:
    var samples_value = batch.get("samples", null)
    if typeof(samples_value) != TYPE_ARRAY:
        return {}

    var controlled_entity_id := world_presentation.controlled_entity_id()
    for sample_value in samples_value:
        if typeof(sample_value) != TYPE_DICTIONARY:
            return {}
        var sample: Dictionary = sample_value
        if int(sample.get("entity_id", 0)) != controlled_entity_id:
            continue
        return {
            "entity_id": controlled_entity_id,
            "position_m": sample.get("position_m", Vector3.ZERO),
            "velocity_mps": sample.get("velocity_mps", Vector3.ZERO),
            "spatial_epoch": int(sample.get("spatial_epoch", 0)),
            "tick": int(batch.get("tick", -1)),
            "revision": int(batch.get("revision", -1)),
            "protocol_version": int(batch.get("protocol_version", 0)),
        }
    return {}


func _controlled_planar_position() -> Vector2:
    var position_value = _controlled_actor_spatial_projection.get("position_m", Vector3.ZERO)
    if typeof(position_value) != TYPE_VECTOR3:
        return Vector2.ZERO
    var position: Vector3 = position_value
    return Vector2(position.x, position.z)


func _intent_toward_rest_target() -> Vector2i:
    var delta := _rest_target_m - _controlled_planar_position()
    if abs(delta.x) <= REST_HOLD_TOLERANCE_M and abs(delta.y) <= REST_HOLD_TOLERANCE_M:
        return Vector2i.ZERO
    if delta.is_zero_approx():
        return Vector2i.ZERO
    var direction := delta.normalized()
    return Vector2i(
        int(direction.x * MOVE_INTENT_SCALE),
        int(direction.y * MOVE_INTENT_SCALE)
    )


func _pace_toward_rest_target() -> int:
    var distance := (_rest_target_m - _controlled_planar_position()).length()
    return LOCOMOTION_PACE_RUN if distance > REST_RUN_SWITCH_DISTANCE_M else LOCOMOTION_PACE_WALK


func _refresh_debug_hud() -> void:
    _living_need_projection = sim.living_need_projection()
    if not _refresh_village_household_resource_projection():
        debug_household_resource_status.text = "—"
    var authoritative_position := world_presentation.controlled_authoritative_position()
    var presentation_position := player.get_global_transform_interpolated().origin
    var fps := int(Performance.get_monitor(Performance.TIME_FPS))
    var process_ms := float(Performance.get_monitor(Performance.TIME_PROCESS)) * 1000.0
    var physics_ms := float(Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS)) * 1000.0

    debug_fps.text = "%d" % fps
    debug_process.text = "%.2f ms" % process_ms
    debug_physics.text = "%.2f ms" % physics_ms
    debug_input.text = _active_input_device_text()

    debug_entity.text = "#%d" % world_presentation.controlled_entity_id()
    debug_tick.text = str(world_presentation.last_tick())
    debug_revision.text = str(world_presentation.last_revision())
    debug_protocol.text = str(world_presentation.protocol_version())
    debug_seed.text = str(int(_bootstrap_projection.get("seed", 0)))
    debug_scenario.text = _scenario_text()
    debug_living_need_status.text = _living_need_status_text(
        str(_living_need_projection.get("status", "unknown"))
    )
    debug_household_resource_status.text = _household_resource_status_text(
        _household_by_id(_village_household_resource_projection, _tracked_household_id)
    )

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


func _living_need_status_text(status: String) -> String:
    match status:
        "traveling":
            return tr(&"UI_NEED_TRAVELING")
        "blocked":
            return tr(&"UI_NEED_BLOCKED")
        "satisfied":
            return tr(&"UI_NEED_SATISFIED")
        _:
            return "—"


func _household_resource_status_text(household: Dictionary) -> String:
    if household.is_empty():
        return "—"
    var status_text := "—"
    match str(household.get("status", "unknown")):
        "adequate":
            status_text = tr(&"UI_RESOURCE_ADEQUATE")
        "shortage":
            status_text = tr(&"UI_RESOURCE_SHORTAGE")
        _:
            return "—"
    return "%s · %d / %d" % [
        status_text,
        int(household.get("grain_stock_units", 0)),
        int(household.get("shortage_threshold_units", 0)),
    ]


func _active_input_device_text() -> String:
    match controls.active_device():
        PlayerControls.InputDevice.GAMEPAD:
            return tr(&"UI_INPUT_GAMEPAD")
        _:
            return tr(&"UI_INPUT_KEYBOARD_MOUSE")


func _scenario_text() -> String:
    match _scenario_name:
        "smoke", "offscreen":
            return tr(&"UI_SCENARIO_SMOKE")
        "rest_interference":
            return tr(&"UI_SCENARIO_REST_INTERFERENCE")
        "shortage":
            return tr(&"UI_SCENARIO_SHORTAGE")
        _:
            return tr(&"UI_SCENARIO_INTERACTIVE")


func _apply_smoke_bootstrap() -> bool:
    var response: Dictionary = sim.bootstrap_submit_move(1, 0)
    if not bool(response.get("ok", false)):
        push_error("native bootstrap move failed")
        return false

    _set_bootstrap_projection(response.get("projection", {}))
    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to apply observed-world projection after bootstrap move")
        return false
    return true


func _run_smoke(artifact_dir: String) -> void:
    await get_tree().process_frame
    if not _apply_smoke_bootstrap():
        get_tree().quit(3)
        return

    if not await _run_one_smoke_movement():
        get_tree().quit(8)
        return

    _duplicate_movement_batch_rejected = not world_presentation.apply_authoritative_movement_sample_batch(
        _last_movement_batch
    )
    if not _duplicate_movement_batch_rejected:
        push_error("WorldPresentation accepted a duplicate authoritative movement batch")
        get_tree().quit(9)
        return

    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to refresh observed-world projection after authoritative movement")
        get_tree().quit(6)
        return
    _controlled_actor_spatial_projection = sim.controlled_actor_spatial_projection()
    _living_need_projection = sim.living_need_projection()
    _refresh_debug_hud()

    await RenderingServer.frame_post_draw
    if not _write_debug_artifact(artifact_dir):
        get_tree().quit(4)
        return
    if not _write_screenshot(artifact_dir):
        get_tree().quit(5)
        return

    get_tree().quit(0)


func _run_offscreen(artifact_dir: String) -> void:
    await get_tree().process_frame
    if not _apply_smoke_bootstrap():
        get_tree().quit(3)
        return

    if not await _run_one_smoke_movement():
        get_tree().quit(8)
        return
    var first_batch := _last_movement_batch.duplicate(true)
    var first_batch_evidence := _movement_batch_evidence(first_batch)
    if first_batch_evidence.is_empty():
        push_error("offscreen scenario could not serialize its first authoritative batch")
        get_tree().quit(10)
        return

    var initial_npc_presentation := world_presentation.presentation_for(_living_need_actor_id)
    if initial_npc_presentation == null or not initial_npc_presentation.visible:
        push_error("offscreen scenario requires an initially materialized visible NPC")
        get_tree().quit(10)
        return
    if not world_presentation.dematerialize_observed_non_controlled(_living_need_actor_id):
        get_tree().quit(10)
        return
    await get_tree().process_frame

    var observed_while_absent := world_presentation.is_observed(_living_need_actor_id)
    var absent_before_tick := world_presentation.presentation_for(_living_need_actor_id) == null
    if not observed_while_absent or not absent_before_tick:
        push_error("NPC must remain observed while its presentation is absent")
        get_tree().quit(10)
        return

    if not await _run_one_smoke_movement():
        get_tree().quit(8)
        return
    var offscreen_batch := _last_movement_batch.duplicate(true)
    var offscreen_batch_evidence := _movement_batch_evidence(offscreen_batch)
    var absent_after_tick := world_presentation.presentation_for(_living_need_actor_id) == null
    if offscreen_batch_evidence.is_empty() or not absent_after_tick:
        push_error("authoritative offscreen tick must succeed without rematerializing the NPC")
        get_tree().quit(10)
        return

    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to rematerialize NPC from fresh observed-world projection")
        get_tree().quit(10)
        return
    var rematerialized := world_presentation.presentation_for(_living_need_actor_id)
    var hidden_before_sample := rematerialized != null and not rematerialized.visible
    if not hidden_before_sample:
        push_error("rematerialized NPC must wait hidden for a fresh authoritative sample")
        get_tree().quit(10)
        return

    if not await _run_one_smoke_movement():
        get_tree().quit(8)
        return
    var rematerialized_after_sample := world_presentation.presentation_for(_living_need_actor_id)
    var visible_after_sample := (
        rematerialized_after_sample != null and rematerialized_after_sample.visible
    )
    if not visible_after_sample:
        push_error("fresh authoritative sample must reveal the rematerialized NPC")
        get_tree().quit(10)
        return

    _offscreen_evidence = {
        "entity_id": _living_need_actor_id,
        "observed_while_absent": observed_while_absent,
        "presentation_absent_before_tick": absent_before_tick,
        "presentation_absent_after_tick": absent_after_tick,
        "rematerialized_hidden_before_sample": hidden_before_sample,
        "rematerialized_visible_after_sample": visible_after_sample,
        "first_batch": first_batch_evidence,
        "offscreen_batch": offscreen_batch_evidence,
    }

    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to refresh observed-world projection after rematerialization")
        get_tree().quit(10)
        return
    _controlled_actor_spatial_projection = sim.controlled_actor_spatial_projection()
    _living_need_projection = sim.living_need_projection()
    _refresh_debug_hud()

    await RenderingServer.frame_post_draw
    if not _write_debug_artifact(artifact_dir):
        get_tree().quit(4)
        return
    if not _write_screenshot(artifact_dir):
        get_tree().quit(5)
        return

    get_tree().quit(0)


func _run_rest_interference(artifact_dir: String) -> void:
    await get_tree().process_frame
    if not _apply_smoke_bootstrap():
        get_tree().quit(3)
        return

    _living_need_projection = sim.living_need_projection()
    var initial_status := str(_living_need_projection.get("status", "unknown"))
    if initial_status != "traveling":
        push_error("rest interference scenario must start with a traveling need")
        get_tree().quit(11)
        return

    var blocked_projection: Dictionary = {}
    for _tick in range(REST_INTERFERENCE_APPROACH_LIMIT_TICKS):
        var intent := _intent_toward_rest_target()
        var pace := _pace_toward_rest_target()
        if not await _run_one_scripted_movement(intent, pace):
            get_tree().quit(8)
            return
        var status := str(_living_need_projection.get("status", "unknown"))
        if status == "blocked":
            blocked_projection = _living_need_projection.duplicate(true)
            break
        if status == "satisfied":
            push_error("NPC satisfied RestNeed before controlled actor produced the blocking condition")
            get_tree().quit(11)
            return

    if blocked_projection.is_empty():
        push_error("controlled actor did not produce the RestNeed blocked outcome before the deadline")
        get_tree().quit(11)
        return

    var blocked_position := _controlled_planar_position()
    _refresh_debug_hud()
    var blocked_hud_text := debug_living_need_status.text
    await RenderingServer.frame_post_draw
    if not _write_named_screenshot(artifact_dir, "blocked.png"):
        get_tree().quit(5)
        return

    var satisfied_projection: Dictionary = {}
    for _tick in range(REST_INTERFERENCE_RELEASE_LIMIT_TICKS):
        if not await _run_one_scripted_movement(
            Vector2i(MOVE_INTENT_SCALE, 0),
            LOCOMOTION_PACE_RUN
        ):
            get_tree().quit(8)
            return
        var status := str(_living_need_projection.get("status", "unknown"))
        if status == "satisfied":
            satisfied_projection = _living_need_projection.duplicate(true)
            break

    if satisfied_projection.is_empty():
        push_error("controlled actor did not release the blocking condition before the deadline")
        get_tree().quit(11)
        return

    var satisfied_position := _controlled_planar_position()
    _rest_interference_evidence = {
        "entity_id": _living_need_actor_id,
        "initial_status": initial_status,
        "blocked_projection": blocked_projection,
        "satisfied_projection": satisfied_projection,
        "blocked_player_position_m": [blocked_position.x, blocked_position.y],
        "satisfied_player_position_m": [satisfied_position.x, satisfied_position.y],
        "blocked_hud_text": blocked_hud_text,
    }

    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to refresh observed world after rest interference scenario")
        get_tree().quit(11)
        return
    _controlled_actor_spatial_projection = sim.controlled_actor_spatial_projection()
    _living_need_projection = sim.living_need_projection()
    _refresh_debug_hud()

    await RenderingServer.frame_post_draw
    if not _write_debug_artifact(artifact_dir):
        get_tree().quit(4)
        return
    if not _write_screenshot(artifact_dir):
        get_tree().quit(5)
        return

    get_tree().quit(0)


func _run_shortage(artifact_dir: String) -> void:
    await get_tree().process_frame
    var initial_household := _household_by_id(
        _initial_village_household_resource_projection,
        _tracked_household_id
    )
    if initial_household.is_empty() or str(initial_household.get("status", "unknown")) != "adequate":
        push_error("autonomous shortage scenario must begin with an adequate tracked household")
        get_tree().quit(13)
        return

    var shortage_projection: Dictionary = {}
    for _tick in range(SHORTAGE_SCENARIO_LIMIT_TICKS):
        if not await _run_one_scripted_movement(Vector2i.ZERO, LOCOMOTION_PACE_WALK):
            get_tree().quit(8)
            return
        if not _refresh_village_household_resource_projection():
            get_tree().quit(13)
            return
        var household := _household_by_id(
            _village_household_resource_projection,
            _tracked_household_id
        )
        if household.is_empty():
            push_error("tracked household disappeared from authoritative discovery")
            get_tree().quit(13)
            return
        if str(household.get("status", "unknown")) == "shortage":
            shortage_projection = _village_household_resource_projection.duplicate(true)
            break

    if shortage_projection.is_empty():
        push_error("autonomous NPC Consume did not produce shortage before the deadline")
        get_tree().quit(13)
        return

    _village_household_resource_projection = shortage_projection
    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to reconcile presentation to the post-Consume world revision")
        get_tree().quit(13)
        return

    _refresh_debug_hud()
    var final_household := _household_by_id(
        _village_household_resource_projection,
        _tracked_household_id
    )
    if final_household.is_empty() or str(final_household.get("status", "unknown")) != "shortage":
        push_error("localized shortage feedback lost the authoritative shortage household")
        get_tree().quit(13)
        return

    _shortage_evidence = {
        "actor_id": _living_need_actor_id,
        "household_id": _tracked_household_id,
        "player_economic_intent_submitted": false,
        "initial_status": str(initial_household.get("status", "unknown")),
        "final_status": str(final_household.get("status", "unknown")),
        "initial_stock_units": int(initial_household.get("grain_stock_units", -1)),
        "final_stock_units": int(final_household.get("grain_stock_units", -1)),
        "shortage_threshold_units": int(final_household.get("shortage_threshold_units", -1)),
        "movement_tick": int(_last_movement_batch.get("tick", -1)),
        "movement_revision": int(_last_movement_batch.get("revision", -1)),
        "resource_tick": int(_village_household_resource_projection.get("tick", -1)),
        "resource_revision": int(_village_household_resource_projection.get("revision", -1)),
        "shortage_hud_text": debug_household_resource_status.text,
    }

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
    var movement_evidence := _movement_batch_evidence(_last_movement_batch)
    var initial_resource_evidence := _resource_projection_evidence(
        _initial_village_household_resource_projection
    )
    var resource_evidence := _resource_projection_evidence(
        _village_household_resource_projection
    )
    if (
        spatial_evidence.is_empty()
        or movement_evidence.is_empty()
        or initial_resource_evidence.is_empty()
        or resource_evidence.is_empty()
    ):
        push_error("authoritative runtime state is not serializable playtest evidence")
        return false

    var evidence := {
        "bootstrap_projection": _bootstrap_projection,
        "initial_village_household_resource_projection": initial_resource_evidence,
        "village_household_resource_projection": resource_evidence,
        "observed_world_projection": _observed_world_projection,
        "controlled_actor_spatial_projection": spatial_evidence,
        "living_need_projection": _living_need_projection,
        "movement_stream": {
            "batch": movement_evidence,
            "duplicate_batch_rejected": _duplicate_movement_batch_rejected,
        },
        "presentation": world_presentation.debug_snapshot(),
        "localization": {
            "locale": Localization.current_locale(),
            "supported_locales": Array(Localization.supported_locales()),
            "hud_title": tr(&"UI_DEBUG_TITLE"),
            "controls_hint": tr(&"UI_DEBUG_CONTROLS_HINT"),
            "living_need_status_text": debug_living_need_status.text,
            "household_resource_status_text": debug_household_resource_status.text,
            "scenario_text": debug_scenario.text,
        },
    }
    if not _offscreen_evidence.is_empty():
        evidence["offscreen_continuation"] = _offscreen_evidence
    if not _rest_interference_evidence.is_empty():
        evidence["rest_interference"] = _rest_interference_evidence
    if not _shortage_evidence.is_empty():
        evidence["autonomous_shortage"] = _shortage_evidence
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


func _movement_batch_evidence(batch: Dictionary) -> Dictionary:
    var samples_value = batch.get("samples", null)
    if typeof(samples_value) != TYPE_ARRAY:
        return {}

    var serialized_samples: Array = []
    for sample_value in samples_value:
        if typeof(sample_value) != TYPE_DICTIONARY:
            return {}
        var sample: Dictionary = sample_value
        var position_value = sample.get("position_m", null)
        var velocity_value = sample.get("velocity_mps", null)
        if typeof(position_value) != TYPE_VECTOR3 or typeof(velocity_value) != TYPE_VECTOR3:
            return {}
        var position: Vector3 = position_value
        var velocity: Vector3 = velocity_value
        serialized_samples.append({
            "entity_id": int(sample.get("entity_id", 0)),
            "position_m": [position.x, position.y, position.z],
            "velocity_mps": [velocity.x, velocity.y, velocity.z],
            "spatial_epoch": int(sample.get("spatial_epoch", 0)),
        })

    return {
        "tick": int(batch.get("tick", -1)),
        "revision": int(batch.get("revision", -1)),
        "protocol_version": int(batch.get("protocol_version", 0)),
        "samples": serialized_samples,
    }


func _resource_projection_evidence(projection: Dictionary) -> Dictionary:
    var households_value = projection.get("households", null)
    if typeof(households_value) != TYPE_ARRAY:
        return {}

    var serialized_households: Array = []
    for household_value in households_value:
        if typeof(household_value) != TYPE_DICTIONARY:
            return {}
        var household: Dictionary = household_value
        var members_value = household.get("member_actor_ids", null)
        var store_value = household.get("store_position_m", null)
        if typeof(members_value) != TYPE_ARRAY or typeof(store_value) != TYPE_VECTOR3:
            return {}
        var store: Vector3 = store_value
        serialized_households.append({
            "household_id": int(household.get("household_id", 0)),
            "member_actor_ids": members_value,
            "store_place_id": int(household.get("store_place_id", 0)),
            "store_position_m": [store.x, store.y, store.z],
            "store_axis_tolerance_m": float(household.get("store_axis_tolerance_m", -1.0)),
            "grain_stock_units": int(household.get("grain_stock_units", -1)),
            "shortage_threshold_units": int(household.get("shortage_threshold_units", -1)),
            "status": str(household.get("status", "unknown")),
        })

    return {
        "tick": int(projection.get("tick", -1)),
        "revision": int(projection.get("revision", -1)),
        "protocol_version": int(projection.get("protocol_version", 0)),
        "households": serialized_households,
    }


func _write_screenshot(artifact_dir: String) -> bool:
    return _write_named_screenshot(artifact_dir, "final.png")


func _write_named_screenshot(artifact_dir: String, filename: String) -> bool:
    var image := get_viewport().get_texture().get_image()
    var save_error := image.save_png(artifact_dir.path_join(filename))
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
