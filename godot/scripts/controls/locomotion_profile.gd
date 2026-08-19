class_name LocomotionProfile
extends Resource

# Presentation-only facing response. Authoritative movement speed, pace limits,
# acceleration, braking, gravity, slope and step rules belong to Simulation.
@export_category("Presentation")
@export_range(1.0, 30.0, 0.5) var turn_response: float = 14.0
