class_name ControlProfile
extends Resource

@export_category("Movement input")
@export_range(0.0, 0.5, 0.01) var move_deadzone: float = 0.20

@export_category("Mouse camera")
@export_range(0.0001, 0.01, 0.0001) var mouse_sensitivity: float = 0.0020
@export var invert_mouse_x: bool = false
@export var invert_mouse_y: bool = false

@export_category("Gamepad camera")
@export_range(0.0, 0.5, 0.01) var look_deadzone: float = 0.20
@export_range(0.5, 8.0, 0.1) var gamepad_look_speed: float = 3.0
@export_range(0.5, 3.0, 0.05) var gamepad_look_response: float = 1.0
@export var invert_gamepad_x: bool = false
@export var invert_gamepad_y: bool = false

@export_category("Third-person camera")
@export_range(-89.0, 0.0, 1.0) var pitch_min_degrees: float = -65.0
@export_range(0.0, 89.0, 1.0) var pitch_max_degrees: float = 70.0
@export_range(0.5, 3.0, 0.05) var target_height: float = 1.55
@export_range(1.5, 8.0, 0.1) var camera_distance: float = 4.2
@export_range(0.0, 0.5, 0.01) var camera_collision_margin: float = 0.08
@export_range(35.0, 100.0, 1.0) var field_of_view: float = 68.0
