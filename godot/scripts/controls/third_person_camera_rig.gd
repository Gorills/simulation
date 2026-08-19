class_name ThirdPersonCameraRig
extends Node3D

@onready var pitch_pivot: Node3D = %PitchPivot
@onready var spring_arm: SpringArm3D = %SpringArm
@onready var camera_node: Camera3D = %Camera

var _controls: PlayerControls
var _target: Node3D
var _yaw: float = 0.0
var _pitch: float = deg_to_rad(-12.0)


func _ready() -> void:
    # The rig is updated every rendered frame and follows an explicitly
    # interpolated target, so automatic physics interpolation would add a
    # second interpolation layer and delay mouse-driven camera rotation.
    set_physics_interpolation_mode(Node.PHYSICS_INTERPOLATION_MODE_OFF)


func configure(controls: PlayerControls, target: Node3D) -> void:
    _controls = controls
    _target = target

    var profile := _controls.profile
    spring_arm.spring_length = profile.camera_distance
    spring_arm.margin = profile.camera_collision_margin
    camera_node.fov = profile.field_of_view

    if _target is CollisionObject3D:
        spring_arm.add_excluded_object((_target as CollisionObject3D).get_rid())

    _yaw = _target.global_rotation.y
    _pitch = clampf(
        _pitch,
        deg_to_rad(profile.pitch_min_degrees),
        deg_to_rad(profile.pitch_max_degrees)
    )
    _apply_rotation()
    global_position = _target.global_position + Vector3.UP * profile.target_height
    # Prime the interpolation pump before later gameplay can perform a
    # teleport/reset on the target.
    _target.get_global_transform_interpolated()
    camera_node.make_current()


func get_camera() -> Camera3D:
    return camera_node


func _process(delta: float) -> void:
    if _controls == null or _target == null:
        return

    _follow_target()

    var profile := _controls.profile
    var mouse_delta := _controls.consume_mouse_look()
    if not mouse_delta.is_zero_approx():
        var mouse_yaw := -mouse_delta.x if profile.invert_mouse_x else mouse_delta.x
        var mouse_pitch := mouse_delta.y if profile.invert_mouse_y else -mouse_delta.y
        _yaw -= mouse_yaw * profile.mouse_sensitivity
        _pitch += mouse_pitch * profile.mouse_sensitivity

    var stick := _controls.gamepad_look_axis()
    if not stick.is_zero_approx():
        var stick_yaw := -stick.x if profile.invert_gamepad_x else stick.x
        var stick_pitch := stick.y if profile.invert_gamepad_y else -stick.y
        _yaw -= stick_yaw * profile.gamepad_look_speed * delta
        _pitch += stick_pitch * profile.gamepad_look_speed * delta

    _pitch = clampf(
        _pitch,
        deg_to_rad(profile.pitch_min_degrees),
        deg_to_rad(profile.pitch_max_degrees)
    )
    _apply_rotation()


func _follow_target() -> void:
    var target_position := _target.get_global_transform_interpolated().origin
    global_position = target_position + Vector3.UP * _controls.profile.target_height


func _apply_rotation() -> void:
    rotation = Vector3(0.0, _yaw, 0.0)
    pitch_pivot.rotation = Vector3(_pitch, 0.0, 0.0)
