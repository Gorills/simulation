class_name ThirdPersonPlayer
extends CharacterBody3D

@export var profile: LocomotionProfile

@onready var visual_root: Node3D = $VisualRoot


func _ready() -> void:
    assert(profile != null, "ThirdPersonPlayer requires a LocomotionProfile resource")


# Position and velocity are presentation replicas of Simulation samples. Only the
# visual child turns per rendered frame; the physics root transform is moved solely
# by WorldPresentation on authoritative fixed ticks.
func _process(delta: float) -> void:
    var horizontal_velocity := Vector3(velocity.x, 0.0, velocity.z)
    if horizontal_velocity.is_zero_approx():
        return

    var desired_direction := horizontal_velocity.normalized()
    var target_yaw := atan2(-desired_direction.x, -desired_direction.z)
    var blend := 1.0 - exp(-profile.turn_response * delta)
    visual_root.rotation.y = lerp_angle(visual_root.rotation.y, target_yaw, blend)
