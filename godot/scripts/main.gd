extends Node3D

const DEBUG_REFRESH_INTERVAL_SECONDS := 0.25

@onready var controls: PlayerControls = %PlayerControls
@onready var world_presentation: WorldPresentation = %WorldPresentation
@onready var player: ThirdPersonPlayer = %Player
@onready var player_entity_binding: EntityBinding = %PlayerEntityBinding
@onready var camera_rig: ThirdPersonCameraRig = %CameraRig
@onready var debug_label: Label = %Debug

var sim := SimFacade.new()
var _bootstrap_projection: Dictionary = {}
var _bootstrap_projection_text := "{}"
var _observed_world_projection: Dictionary = {}
var _controlled_actor_spatial_projection: Dictionary = {}
var _debug_refresh_elapsed := 0.0


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
    _refresh_debug_text()

    var scenario := _user_arg_value("--scenario")
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
    _refresh_debug_text()


func _on_input_device_changed(_device: int) -> void:
    _refresh_debug_text()


func _set_bootstrap_projection(projection: Dictionary) -> void:
    _bootstrap_projection = projection
    _bootstrap_projection_text = JSON.stringify(projection)


func _refresh_debug_text() -> void:
    var authoritative_position := _spatial_position_or_zero(_controlled_actor_spatial_projection)
    var fps := int(Performance.get_monitor(Performance.TIME_FPS))
    var process_ms := float(Performance.get_monitor(Performance.TIME_PROCESS)) * 1000.0
    var physics_ms := float(Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS)) * 1000.0
    debug_label.text = (
        "controls: %s\n"
        + "performance: %d fps · process %.2f ms · physics %.2f ms\n"
        + "authoritative entity: %d · tick: %d · revision: %d · protocol: %d\n"
        + "authoritative spatial: (%.2f, %.2f, %.2f)m · epoch: %d\n"
        + "player presentation: (%.2f, %.2f, %.2f)\n"
        + "native bootstrap projection: %s\n"
        + "WASD / left stick move · mouse / right stick look · Shift / L3 sprint · Esc releases mouse"
    ) % [
        controls.active_device_name(),
        fps,
        process_ms,
        physics_ms,
        world_presentation.controlled_entity_id(),
        world_presentation.last_tick(),
        world_presentation.last_revision(),
        world_presentation.protocol_version(),
        authoritative_position.x,
        authoritative_position.y,
        authoritative_position.z,
        int(_controlled_actor_spatial_projection.get("spatial_epoch", 0)),
        player.global_position.x,
        player.global_position.y,
        player.global_position.z,
        _bootstrap_projection_text,
    ]


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
    _refresh_debug_text()

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
