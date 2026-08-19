extends Node3D

@onready var controls: PlayerControls = %PlayerControls
@onready var world_presentation: WorldPresentation = %WorldPresentation
@onready var player: ThirdPersonPlayer = %Player
@onready var player_entity_binding: EntityBinding = %PlayerEntityBinding
@onready var camera_rig: ThirdPersonCameraRig = %CameraRig
@onready var debug_label: Label = %Debug

var sim := SimFacade.new()
var _bootstrap_projection: Dictionary = {}
var _observed_world_projection: Dictionary = {}


func _ready() -> void:
    _observed_world_projection = sim.observed_world_projection()
    if not world_presentation.apply_observed_world_projection(
        _observed_world_projection,
        player_entity_binding
    ):
        push_error("failed to bind controlled presentation to authoritative EntityId")
        get_tree().quit(6)
        return

    camera_rig.configure(controls, player)
    player.configure(controls, camera_rig.get_camera())
    controls.input_device_changed.connect(_on_input_device_changed)
    controls.capture_pointer()

    _bootstrap_projection = sim.bootstrap_debug_projection()
    _refresh_debug_text()

    var scenario := _user_arg_value("--scenario")
    if scenario == "smoke":
        var artifact_dir := _user_arg_value("--artifact-dir")
        if artifact_dir.is_empty():
            push_error("smoke scenario requires --artifact-dir")
            get_tree().quit(2)
            return
        call_deferred("_run_smoke", artifact_dir)


func _process(_delta: float) -> void:
    _refresh_debug_text()


func _on_input_device_changed(_device: int) -> void:
    _refresh_debug_text()


func _refresh_debug_text() -> void:
    debug_label.text = (
        "controls: %s\n"
        + "authoritative entity: %d · tick: %d · revision: %d · protocol: %d\n"
        + "player presentation: (%.2f, %.2f, %.2f)\n"
        + "native bootstrap projection: %s\n"
        + "WASD / left stick move · mouse / right stick look · Shift / L3 sprint · Esc releases mouse"
    ) % [
        controls.active_device_name(),
        world_presentation.controlled_entity_id(),
        world_presentation.last_tick(),
        world_presentation.last_revision(),
        world_presentation.protocol_version(),
        player.global_position.x,
        player.global_position.y,
        player.global_position.z,
        JSON.stringify(_bootstrap_projection),
    ]


func _run_smoke(artifact_dir: String) -> void:
    await get_tree().process_frame
    var response: Dictionary = sim.bootstrap_submit_move(1, 0)
    if not bool(response.get("ok", false)):
        push_error("native bootstrap move failed")
        get_tree().quit(3)
        return

    _bootstrap_projection = response.get("projection", {})
    _observed_world_projection = sim.observed_world_projection()
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

    var evidence := {
        "bootstrap_projection": _bootstrap_projection,
        "observed_world_projection": _observed_world_projection,
        "presentation": world_presentation.debug_snapshot(),
    }
    file.store_string(JSON.stringify(evidence, "  "))
    return true


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
