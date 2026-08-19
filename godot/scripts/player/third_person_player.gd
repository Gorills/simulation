class_name ThirdPersonPlayer
extends CharacterBody3D

@export var profile: LocomotionProfile

var _controls: PlayerControls
var _movement_camera: Camera3D


func _ready() -> void:
    assert(profile != null, "ThirdPersonPlayer requires a LocomotionProfile resource")
    floor_snap_length = profile.floor_snap_length
    floor_max_angle = deg_to_rad(profile.floor_max_angle_degrees)
    floor_constant_speed = profile.constant_speed_on_slopes


func configure(controls: PlayerControls, movement_camera: Camera3D) -> void:
    _controls = controls
    _movement_camera = movement_camera


func _physics_process(delta: float) -> void:
    if _controls == null or _movement_camera == null:
        return

    var input_axis := _controls.move_axis()
    var input_strength := input_axis.length()
    var desired_direction := _camera_relative_direction(input_axis)

    var target_speed := profile.move_speed * input_strength
    if _controls.is_sprinting() and input_strength >= profile.sprint_minimum_input:
        target_speed = profile.sprint_speed * input_strength

    var target_horizontal_velocity := desired_direction * target_speed
    var horizontal_velocity := Vector3(velocity.x, 0.0, velocity.z)
    var response_rate := _horizontal_response_rate(
        horizontal_velocity,
        desired_direction,
        target_speed
    )
    horizontal_velocity = horizontal_velocity.move_toward(
        target_horizontal_velocity,
        response_rate * delta
    )

    velocity.x = horizontal_velocity.x
    velocity.z = horizontal_velocity.z

    if is_on_floor():
        if velocity.y < 0.0:
            velocity.y = 0.0
    else:
        velocity += get_gravity() * delta

    if not desired_direction.is_zero_approx():
        var target_yaw := atan2(-desired_direction.x, -desired_direction.z)
        var blend := 1.0 - exp(-profile.turn_response * delta)
        rotation.y = lerp_angle(rotation.y, target_yaw, blend)

    move_and_slide()


func _horizontal_response_rate(
    horizontal_velocity: Vector3,
    desired_direction: Vector3,
    target_speed: float
) -> float:
    if desired_direction.is_zero_approx():
        return profile.deceleration
    if horizontal_velocity.is_zero_approx():
        return profile.acceleration

    var current_speed := horizontal_velocity.length()
    var current_direction := horizontal_velocity / current_speed
    if not current_direction.is_equal_approx(desired_direction):
        return profile.direction_change_acceleration
    if target_speed < current_speed:
        return profile.deceleration
    return profile.acceleration


func _camera_relative_direction(input_axis: Vector2) -> Vector3:
    if input_axis.is_zero_approx():
        return Vector3.ZERO

    var camera_basis := _movement_camera.global_transform.basis
    var camera_forward := -camera_basis.z
    var camera_right := camera_basis.x
    camera_forward.y = 0.0
    camera_right.y = 0.0
    camera_forward = camera_forward.normalized()
    camera_right = camera_right.normalized()

    var direction := camera_right * input_axis.x + camera_forward * -input_axis.y
    if direction.length_squared() <= 0.0:
        return Vector3.ZERO
    return direction.normalized()
