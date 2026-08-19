class_name EntityBinding
extends Node

var _entity_id: int = 0


func entity_id() -> int:
    return _entity_id


func is_bound() -> bool:
    return _entity_id > 0


# WorldPresentation owns assignment. Presentation scripts may read identity but
# must not choose or mutate authoritative EntityIds themselves.
func _assign_entity_id(value: int) -> bool:
    if value <= 0:
        push_error("EntityBinding requires a positive authoritative EntityId")
        return false
    _entity_id = value
    return true


func _clear_entity_id() -> void:
    _entity_id = 0
