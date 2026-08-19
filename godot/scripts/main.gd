extends Node3D

@onready var controls: PlayerControls = %PlayerControls
@onready var player: ThirdPersonPlayer = %Player
@onready var camera_rig: ThirdPersonCameraRig = %CameraRig
@onready var debug_label: Label = %Debug

var sim := SimFacade.new()
var _native_projection: Dictionary = {}


func _ready() -> void:
    camera_rig.configure(controls, player)
    player.configure(controls, camera_rig.get_camera())
    controls.input_device_changed.connect(_on_input_device_changed)
    controls.capture_pointer()

    _native_projection = sim.debug_projection()
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
        + "player presentation: (%.2f, %.2f, %.2f)\n"
        + "native smoke projection: %s\n"
        + "WASD / left stick move · mouse / right stick look · Shift / L3 sprint · Esc releases mouse"
    ) % [
        controls.active_device_name(),
        player.global_position.x,
        player.global_position.y,
        player.global_position.z,
        JSON.stringify(_native_projection),
    ]


func _run_smoke(artifact_dir: String) -> void:
    await get_tree().process_frame
    var response: Dictionary = sim.submit_move(1, 0)
    if not bool(response.get("ok", false)):
        push_error("native smoke move failed")
        get_tree().quit(3)
        return

    _native_projection = response.get("projection", {})
    _refresh_debug_text()

    await RenderingServer.frame_post_draw
    if not _write_debug_artifact(artifact_dir, _native_projection):
        get_tree().quit(4)
        return
    if not _write_screenshot(artifact_dir):
        get_tree().quit(5)
        return

    get_tree().quit(0)


func _write_debug_artifact(artifact_dir: String, projection: Dictionary) -> bool:
    var mkdir_error := DirAccess.make_dir_recursive_absolute(artifact_dir)
    if mkdir_error != OK:
        push_error("failed to create artifact directory: %s" % error_string(mkdir_error))
        return false

    var file := FileAccess.open(artifact_dir.path_join("debug.json"), FileAccess.WRITE)
    if file == null:
        push_error("failed to open debug artifact: %s" % error_string(FileAccess.get_open_error()))
        return false
    file.store_string(JSON.stringify(projection, "  "))
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
