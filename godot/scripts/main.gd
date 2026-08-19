extends Node3D

const DEBUG_REFRESH_INTERVAL_SECONDS := 0.25
const MOVE_INTENT_SCALE := 1000
const LOCOMOTION_PACE_WALK := 0
const LOCOMOTION_PACE_RUN := 1
const LOCOMOTION_PACE_SPRINT := 2

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
var _last_movement_batch: Dictionary = {}
var _debug_refresh_elapsed := 0.0
var _scenario_name := "interactive"
var _locomotion_runtime_enabled := false
var _smoke_movement_requested := false
var _smoke_movement_finished := false
var _smoke_movement_succeeded := false
var _duplicate_movement_batch_rejected := false


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
    controls.input_device_changed.connect(_on_input_device_changed)
    Localization.locale_changed.connect(_on_locale_changed)
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
        return

    _locomotion_runtime_enabled = true


func _physics_process(_delta: float) -> void:
    if _smoke_movement_requested:
        _smoke_movement_requested = false
        _smoke_movement_succeeded = _advance_authoritative_locomotion(
            Vector2i(MOVE_INTENT_SCALE, 0),
            LOCOMOTION_PACE_RUN
        )
        _smoke_movement_finished = true
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
    return true


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

    # int() truncates toward zero, so quantization cannot push a valid analog
    # vector outside the protocol's unit-circle bound.
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


func _refresh_debug_hud() -> void:
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


func _active_input_device_text() -> String:
    match controls.active_device():
        PlayerControls.InputDevice.GAMEPAD:
            return tr(&"UI_INPUT_GAMEPAD")
        _:
            return tr(&"UI_INPUT_KEYBOARD_MOUSE")


func _scenario_text() -> String:
    match _scenario_name:
        "smoke":
            return tr(&"UI_SCENARIO_SMOKE")
        _:
            return tr(&"UI_SCENARIO_INTERACTIVE")


func _run_smoke(artifact_dir: String) -> void:
    await get_tree().process_frame
    var response: Dictionary = sim.bootstrap_submit_move(1, 0)
    if not bool(response.get("ok", false)):
        push_error("native bootstrap move failed")
        get_tree().quit(3)
        return

    _set_bootstrap_projection(response.get("projection", {}))
    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to apply observed-world projection after bootstrap move")
        get_tree().quit(6)
        return

    _smoke_movement_requested = true
    while not _smoke_movement_finished:
        await get_tree().physics_frame
    if not _smoke_movement_succeeded:
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
    var movement_evidence := _movement_batch_evidence(_last_movement_batch)
    if spatial_evidence.is_empty() or movement_evidence.is_empty():
        push_error("authoritative spatial movement is not serializable smoke evidence")
        return false

    var evidence := {
        "bootstrap_projection": _bootstrap_projection,
        "observed_world_projection": _observed_world_projection,
        "controlled_actor_spatial_projection": spatial_evidence,
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
        },
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
