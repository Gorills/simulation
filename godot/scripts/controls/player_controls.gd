class_name PlayerControls
extends Node

enum InputDevice {
    KEYBOARD_MOUSE,
    GAMEPAD,
}

signal input_device_changed(device: InputDevice)

@export var profile: ControlProfile

var _active_device: InputDevice = InputDevice.KEYBOARD_MOUSE
var _gameplay_enabled: bool = true
var _mouse_look_delta: Vector2 = Vector2.ZERO


func _ready() -> void:
    assert(profile != null, "PlayerControls requires a ControlProfile resource")


func set_gameplay_enabled(enabled: bool) -> void:
    if _gameplay_enabled == enabled:
        return
    _gameplay_enabled = enabled
    if not enabled:
        _mouse_look_delta = Vector2.ZERO


func is_gameplay_enabled() -> bool:
    return _gameplay_enabled


func move_axis() -> Vector2:
    if not _gameplay_enabled:
        return Vector2.ZERO
    return Input.get_vector(
        &"move_left",
        &"move_right",
        &"move_forward",
        &"move_back",
        profile.move_deadzone
    )


func gamepad_look_axis() -> Vector2:
    if not _gameplay_enabled:
        return Vector2.ZERO
    var look := Input.get_vector(
        &"look_left",
        &"look_right",
        &"look_up",
        &"look_down",
        profile.look_deadzone
    )
    return _apply_radial_response(look, profile.gamepad_look_response)


func consume_mouse_look() -> Vector2:
    if not _gameplay_enabled:
        _mouse_look_delta = Vector2.ZERO
        return Vector2.ZERO
    var delta := _mouse_look_delta
    _mouse_look_delta = Vector2.ZERO
    return delta


func is_sprinting() -> bool:
    return _gameplay_enabled and Input.is_action_pressed(&"sprint")


func active_device() -> InputDevice:
    return _active_device


func active_device_name() -> String:
    match _active_device:
        InputDevice.GAMEPAD:
            return "gamepad"
        _:
            return "keyboard/mouse"


func capture_pointer() -> void:
    Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func release_pointer() -> void:
    Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
    _mouse_look_delta = Vector2.ZERO


func _input(event: InputEvent) -> void:
    _track_active_device(event)

    if (
        _gameplay_enabled
        and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED
        and event is InputEventMouseMotion
        and not event.screen_relative.is_zero_approx()
    ):
        _mouse_look_delta += event.screen_relative


func _unhandled_input(event: InputEvent) -> void:
    if event.is_action_pressed(&"pointer_release") and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
        release_pointer()
        get_viewport().set_input_as_handled()
        return

    if (
        _gameplay_enabled
        and Input.mouse_mode != Input.MOUSE_MODE_CAPTURED
        and event is InputEventMouseButton
        and event.pressed
        and event.button_index == MOUSE_BUTTON_LEFT
    ):
        capture_pointer()
        get_viewport().set_input_as_handled()


func _track_active_device(event: InputEvent) -> void:
    if event is InputEventMouseMotion:
        if not event.screen_relative.is_zero_approx():
            _set_active_device(InputDevice.KEYBOARD_MOUSE)
        return

    if event is InputEventMouseButton:
        if event.pressed:
            _set_active_device(InputDevice.KEYBOARD_MOUSE)
        return

    if event is InputEventKey:
        if event.pressed and not event.echo:
            _set_active_device(InputDevice.KEYBOARD_MOUSE)
        return

    if event is InputEventJoypadButton:
        if event.pressed:
            _set_active_device(InputDevice.GAMEPAD)
        return

    if event is InputEventJoypadMotion and _is_meaningful_joypad_motion(event):
        _set_active_device(InputDevice.GAMEPAD)


func _is_meaningful_joypad_motion(event: InputEventJoypadMotion) -> bool:
    if event.axis == JOY_AXIS_LEFT_X or event.axis == JOY_AXIS_LEFT_Y:
        return _joypad_stick(event.device, JOY_AXIS_LEFT_X, JOY_AXIS_LEFT_Y).length() > profile.move_deadzone
    if event.axis == JOY_AXIS_RIGHT_X or event.axis == JOY_AXIS_RIGHT_Y:
        return _joypad_stick(event.device, JOY_AXIS_RIGHT_X, JOY_AXIS_RIGHT_Y).length() > profile.look_deadzone
    return false


func _joypad_stick(device: int, x_axis: JoyAxis, y_axis: JoyAxis) -> Vector2:
    return Vector2(
        Input.get_joy_axis(device, x_axis),
        Input.get_joy_axis(device, y_axis)
    )


func _set_active_device(device: InputDevice) -> void:
    if device == _active_device:
        return
    _active_device = device
    input_device_changed.emit(device)


func _apply_radial_response(value: Vector2, exponent: float) -> Vector2:
    var magnitude := value.length()
    if magnitude <= 0.0 or is_equal_approx(exponent, 1.0):
        return value

    var shaped_magnitude := pow(magnitude, exponent)
    return value / magnitude * shaped_magnitude
