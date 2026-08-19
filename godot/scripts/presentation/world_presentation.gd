class_name WorldPresentation
extends Node3D

var _bindings: Dictionary = {}
var _observed_entity_ids: Dictionary = {}
var _controlled_entity_id: int = 0
var _last_tick: int = -1
var _last_revision: int = -1
var _protocol_version: int = 0
var _controlled_spatial_initialized: bool = false
var _controlled_spatial_epoch: int = 0
var _controlled_spatial_tick: int = -1
var _controlled_spatial_revision: int = -1


func apply_observed_world_projection(
    projection: Dictionary,
    controlled_binding: EntityBinding
) -> bool:
    if controlled_binding == null:
        push_error("WorldPresentation requires the controlled EntityBinding")
        return false

    var tick := _read_nonnegative_int(projection, "tick")
    var revision := _read_nonnegative_int(projection, "revision")
    var protocol_version := _read_positive_int(projection, "protocol_version")
    var controlled_entity_id := _read_positive_int(projection, "controlled_actor_id")
    if tick < 0 or revision < 0 or protocol_version <= 0 or controlled_entity_id <= 0:
        push_error("WorldPresentation received an invalid observed-world header")
        return false
    if revision < _last_revision:
        push_warning(
            "WorldPresentation ignored stale revision %d after %d"
            % [revision, _last_revision]
        )
        return false

    var entities_value = projection.get("entities", null)
    if typeof(entities_value) != TYPE_ARRAY:
        push_error("WorldPresentation observed-world projection requires an entities array")
        return false

    var entities: Array = entities_value
    var next_observed: Dictionary = {}
    for entity_value in entities:
        if typeof(entity_value) != TYPE_DICTIONARY:
            push_error("WorldPresentation observed entity must be a Dictionary")
            return false
        var entity: Dictionary = entity_value
        var entity_id := _read_positive_int(entity, "entity_id")
        if entity_id <= 0 or next_observed.has(entity_id):
            push_error("WorldPresentation received an invalid or duplicate EntityId")
            return false
        next_observed[entity_id] = true

    if not next_observed.has(controlled_entity_id):
        push_error("WorldPresentation controlled actor is absent from observed entities")
        return false

    var presentation_root := controlled_binding.get_parent() as Node3D
    if presentation_root == null:
        push_error("EntityBinding must be a child of its Node3D presentation root")
        return false
    if not is_ancestor_of(presentation_root):
        push_error("EntityBinding presentation root must live under WorldPresentation")
        return false

    var existing_value = _bindings.get(controlled_entity_id, null)
    if existing_value != null and existing_value != controlled_binding:
        push_error("WorldPresentation already has a different binding for this EntityId")
        return false

    var previous_entity_id := controlled_binding.entity_id()
    if previous_entity_id > 0 and previous_entity_id != controlled_entity_id:
        if _bindings.get(previous_entity_id, null) == controlled_binding:
            _bindings.erase(previous_entity_id)
        controlled_binding._clear_entity_id()

    if not controlled_binding._assign_entity_id(controlled_entity_id):
        return false

    _bindings[controlled_entity_id] = controlled_binding
    _observed_entity_ids = next_observed
    _controlled_entity_id = controlled_entity_id
    _last_tick = tick
    _last_revision = revision
    _protocol_version = protocol_version
    return true


# Initial placement only. Continuous authoritative movement must buffer ordered
# Simulation samples and interpolate/reconcile them; repeatedly calling this
# function would turn every update into a teleport.
func initialize_controlled_spatial_presentation(
    projection: Dictionary,
    controlled_binding: EntityBinding
) -> bool:
    if controlled_binding == null or not controlled_binding.is_bound():
        push_error("controlled spatial initialization requires a bound EntityBinding")
        return false

    var entity_id := _read_positive_int(projection, "entity_id")
    var spatial_epoch := _read_positive_int(projection, "spatial_epoch")
    var tick := _read_nonnegative_int(projection, "tick")
    var revision := _read_nonnegative_int(projection, "revision")
    var protocol_version := _read_positive_int(projection, "protocol_version")
    var position_value = projection.get("position_m", null)
    var velocity_value = projection.get("velocity_mps", null)

    if (
        entity_id <= 0
        or spatial_epoch <= 0
        or tick < 0
        or revision < 0
        or protocol_version <= 0
        or typeof(position_value) != TYPE_VECTOR3
        or typeof(velocity_value) != TYPE_VECTOR3
    ):
        push_error("WorldPresentation received an invalid controlled spatial projection")
        return false
    if entity_id != controlled_binding.entity_id() or entity_id != _controlled_entity_id:
        push_error("controlled spatial projection EntityId does not match the bound presentation")
        return false
    if protocol_version != _protocol_version:
        push_error("controlled spatial projection protocol version does not match observed world")
        return false
    if tick != _last_tick or revision != _last_revision:
        push_error("controlled spatial projection is not from the applied observed-world revision")
        return false

    var presentation_root := controlled_binding.get_parent() as Node3D
    if presentation_root == null or not is_ancestor_of(presentation_root):
        push_error("controlled spatial presentation root must live under WorldPresentation")
        return false

    var position: Vector3 = position_value
    presentation_root.global_position = position
    presentation_root.reset_physics_interpolation()

    _controlled_spatial_initialized = true
    _controlled_spatial_epoch = spatial_epoch
    _controlled_spatial_tick = tick
    _controlled_spatial_revision = revision
    return true


func controlled_entity_id() -> int:
    return _controlled_entity_id


func last_tick() -> int:
    return _last_tick


func last_revision() -> int:
    return _last_revision


func protocol_version() -> int:
    return _protocol_version


func is_observed(entity_id: int) -> bool:
    return _observed_entity_ids.has(entity_id)


func presentation_for(entity_id: int) -> Node3D:
    var binding_value = _bindings.get(entity_id, null)
    if binding_value == null:
        return null
    var binding := binding_value as EntityBinding
    if binding == null:
        return null
    return binding.get_parent() as Node3D


func debug_snapshot() -> Dictionary:
    var observed_ids: Array = _observed_entity_ids.keys()
    observed_ids.sort()
    var bound_ids: Array = _bindings.keys()
    bound_ids.sort()
    return {
        "controlled_entity_id": _controlled_entity_id,
        "last_tick": _last_tick,
        "last_revision": _last_revision,
        "protocol_version": _protocol_version,
        "observed_entity_ids": observed_ids,
        "bound_entity_ids": bound_ids,
        "controlled_spatial_initialized": _controlled_spatial_initialized,
        "controlled_spatial_epoch": _controlled_spatial_epoch,
        "controlled_spatial_tick": _controlled_spatial_tick,
        "controlled_spatial_revision": _controlled_spatial_revision,
    }


func _read_nonnegative_int(source: Dictionary, key: String) -> int:
    var value = source.get(key, null)
    if typeof(value) != TYPE_INT:
        return -1
    var result := int(value)
    if result < 0:
        return -1
    return result


func _read_positive_int(source: Dictionary, key: String) -> int:
    var result := _read_nonnegative_int(source, key)
    if result <= 0:
        return -1
    return result
