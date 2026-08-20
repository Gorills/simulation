class_name ThirdPersonPlayer
extends CharacterBody3D

@export var profile: LocomotionProfile

@onready var visual_root: Node3D = $VisualRoot

var _planar_move_intent: Vector2i = Vector2i.ZERO


func _ready() -> void:
    assert(profile != null, "ThirdPersonPlayer requires a LocomotionProfile resource")
    # Local yaw is written every rendered frame. Inherited physics interpolation
    # would fight those writes; the parent physics root still interpolates
    # translation from authoritative samples.
    visual_root.set_physics_interpolation_mode(Node.PHYSICS_INTERPOLATION_MODE_OFF)


# Position and velocity are presentation replicas of Simulation samples. Only the
# visual child turns per rendered frame; the physics root transform is moved solely
# by WorldPresentation on authoritative fixed ticks. Facing follows the submitted
# planar intent, not replica velocity: independent-axis braking changes velocity
# direction while stopping, which would yaw the mesh after input is released.
func set_planar_move_intent(intent: Vector2i) -> void:
    _planar_move_intent = intent


func _process(delta: float) -> void:
    if _planar_move_intent == Vector2i.ZERO:
        return

    var target_yaw := atan2(-float(_planar_move_intent.x), -float(_planar_move_intent.y))
    var blend := 1.0 - exp(-profile.turn_response * delta)
    visual_root.rotation.y = lerp_angle(visual_root.rotation.y, target_yaw, blend)
