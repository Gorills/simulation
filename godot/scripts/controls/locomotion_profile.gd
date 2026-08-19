class_name LocomotionProfile
extends Resource

@export_category("Ground movement")
@export_range(0.5, 12.0, 0.1) var move_speed: float = 5.8
@export_range(0.5, 15.0, 0.1) var sprint_speed: float = 8.2
@export_range(1.0, 40.0, 0.5) var acceleration: float = 18.0
@export_range(1.0, 50.0, 0.5) var deceleration: float = 24.0
@export_range(1.0, 60.0, 0.5) var direction_change_acceleration: float = 24.0
@export_range(1.0, 30.0, 0.5) var turn_response: float = 14.0
@export_range(0.0, 1.0, 0.05) var sprint_minimum_input: float = 0.65

@export_category("CharacterBody3D")
@export_range(0.0, 1.0, 0.01) var floor_snap_length: float = 0.30
@export_range(0.0, 89.0, 1.0) var floor_max_angle_degrees: float = 50.0
@export var constant_speed_on_slopes: bool = true
