extends Node2D

const TILE_PIXELS := 48.0
const ORIGIN := Vector2(480.0, 270.0)

@onready var player_view: Polygon2D = $Player
@onready var debug_label: Label = $HUD/Debug

var sim := SimFacade.new()

func _ready() -> void:
    _render_projection(sim.debug_projection())

    var scenario := _user_arg_value("--scenario")
    if scenario == "smoke":
        var artifact_dir := _user_arg_value("--artifact-dir")
        if artifact_dir.is_empty():
            push_error("smoke scenario requires --artifact-dir")
            get_tree().quit(2)
            return
        call_deferred("_run_smoke", artifact_dir)


func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventKey and event.echo:
        return
    if event.is_action_pressed("move_left"):
        _submit_move(-1, 0)
    elif event.is_action_pressed("move_right"):
        _submit_move(1, 0)
    elif event.is_action_pressed("move_up"):
        _submit_move(0, -1)
    elif event.is_action_pressed("move_down"):
        _submit_move(0, 1)


func _submit_move(dx: int, dy: int) -> Dictionary:
    var response: Dictionary = sim.submit_move(dx, dy)
    var projection: Dictionary = response.get("projection", {})
    _render_projection(projection)
    return response


func _render_projection(projection: Dictionary) -> void:
    var x := int(projection.get("x", 0))
    var y := int(projection.get("y", 0))
    player_view.position = ORIGIN + Vector2(float(x), float(y)) * TILE_PIXELS
    debug_label.text = "native projection\n%s" % JSON.stringify(projection)


func _run_smoke(artifact_dir: String) -> void:
    await get_tree().process_frame
    var response := _submit_move(1, 0)
    if not bool(response.get("ok", false)):
        push_error("native smoke move failed")
        get_tree().quit(3)
        return

    await RenderingServer.frame_post_draw
    var projection: Dictionary = response["projection"]
    if not _write_debug_artifact(artifact_dir, projection):
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
