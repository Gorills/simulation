class_name WorldPresentation
extends Node3D

const NPC_PRESENTATION_SCENE: PackedScene = preload("res://scenes/npc_presentation.tscn")

var _bindings: Dictionary = {}
var _observed_entity_ids: Dictionary = {}
var _spatial_epoch_by_entity: Dictionary = {}
var _controlled_entity_id: int = 0
var _last_tick: int = -1
var _last_revision: int = -1
var _protocol_version: int = 0
var _controlled_spatial_initialized: bool = false
var _controlled_spatial_epoch: int = 0
var _controlled_spatial_tick: int = -1
var _controlled_spatial_revision: int = -1
var _controlled_authoritative_position: Vector3 = Vector3.ZERO
var _controlled_authoritative_velocity: Vector3 = Vector3.ZERO
var _movement_batches_applied: int = 0


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

    var controlled_root := controlled_binding.get_parent() as Node3D
    if controlled_root == null:
        push_error("EntityBinding must be a child of its Node3D presentation root")
        return false
    if not is_ancestor_of(controlled_root):
        push_error("EntityBinding presentation root must live under WorldPresentation")
        return false

    var existing_controlled = _bindings.get(controlled_entity_id, null)
    if existing_controlled != null and existing_controlled != controlled_binding:
        push_error("WorldPresentation already has a different binding for this EntityId")
        return false

    # Prepare every new non-controlled presentation while detached. All scene and
    # binding validation therefore completes before the observed-world commit.
    var pending_materializations: Array = []
    var ordered_ids: Array = next_observed.keys()
    ordered_ids.sort()
    for entity_id_value in ordered_ids:
        var entity_id := int(entity_id_value)
        if entity_id == controlled_entity_id:
            continue

        var existing_value = _bindings.get(entity_id, null)
        if existing_value != null:
            var existing_binding := existing_value as EntityBinding
            if existing_binding == null or existing_binding.entity_id() != entity_id:
                _free_detached_materializations(pending_materializations)
                push_error("WorldPresentation has an invalid existing non-controlled binding")
                return false
            var existing_root := existing_binding.get_parent() as Node3D
            if existing_root == null or not is_ancestor_of(existing_root):
                _free_detached_materializations(pending_materializations)
                push_error("non-controlled presentation root must live under WorldPresentation")
                return false
            continue

        var presentation_root := NPC_PRESENTATION_SCENE.instantiate() as Node3D
        if presentation_root == null:
            _free_detached_materializations(pending_materializations)
            push_error("NPC presentation scene must instantiate a Node3D root")
            return false
        var binding := presentation_root.get_node_or_null("EntityBinding") as EntityBinding
        if binding == null:
            presentation_root.free()
            _free_detached_materializations(pending_materializations)
            push_error("NPC presentation scene requires an EntityBinding child")
            return false
        if not binding._assign_entity_id(entity_id):
            presentation_root.free()
            _free_detached_materializations(pending_materializations)
            return false
        presentation_root.visible = false
        pending_materializations.append({
            "entity_id": entity_id,
            "presentation_root": presentation_root,
            "binding": binding,
        })

    var previous_entity_id := controlled_binding.entity_id()
    if previous_entity_id > 0 and previous_entity_id != controlled_entity_id:
        if _bindings.get(previous_entity_id, null) == controlled_binding:
            _bindings.erase(previous_entity_id)
        controlled_binding._clear_entity_id()

    if not controlled_binding._assign_entity_id(controlled_entity_id):
        _free_detached_materializations(pending_materializations)
        return false

    # Projection is valid and all future materializations are prepared. Commit
    # presence/binding changes before accepting later movement samples.
    var existing_ids: Array = _bindings.keys()
    for entity_id_value in existing_ids:
        var entity_id := int(entity_id_value)
        if entity_id == controlled_entity_id or next_observed.has(entity_id):
            continue
        var stale_binding := _bindings.get(entity_id, null) as EntityBinding
        if stale_binding != null:
            var stale_root := stale_binding.get_parent() as Node3D
            stale_binding._clear_entity_id()
            if stale_root != null:
                stale_root.queue_free()
        _bindings.erase(entity_id)
        _spatial_epoch_by_entity.erase(entity_id)

    _bindings[controlled_entity_id] = controlled_binding
    for pending_value in pending_materializations:
        var pending: Dictionary = pending_value
        var entity_id: int = pending["entity_id"]
        var presentation_root := pending["presentation_root"] as Node3D
        var binding := pending["binding"] as EntityBinding
        add_child(presentation_root)
        _bindings[entity_id] = binding

    _observed_entity_ids = next_observed
    _controlled_entity_id = controlled_entity_id
    _last_tick = tick
    _last_revision = revision
    _protocol_version = protocol_version
    return true


# Initial placement only. Continuous authoritative movement arrives through
# apply_authoritative_movement_sample_batch().
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
    var velocity: Vector3 = velocity_value
    presentation_root.global_position = position
    if presentation_root is CharacterBody3D:
        (presentation_root as CharacterBody3D).velocity = velocity
    presentation_root.reset_physics_interpolation()

    _spatial_epoch_by_entity[entity_id] = spatial_epoch
    _controlled_spatial_initialized = true
    _controlled_spatial_epoch = spatial_epoch
    _controlled_spatial_tick = tick
    _controlled_spatial_revision = revision
    _controlled_authoritative_position = position
    _controlled_authoritative_velocity = velocity
    return true


# Applies one fixed authoritative movement batch during a Godot physics tick.
# Every sample and bound presentation root is validated before the first transform
# write. Godot physics interpolation then smooths same-epoch transforms for render.
func apply_authoritative_movement_sample_batch(batch: Dictionary) -> bool:
    if not _controlled_spatial_initialized:
        push_error("movement samples require initialized controlled spatial presentation")
        return false

    var tick := _read_nonnegative_int(batch, "tick")
    var revision := _read_nonnegative_int(batch, "revision")
    var protocol_version := _read_positive_int(batch, "protocol_version")
    if tick < 0 or revision < 0 or protocol_version <= 0:
        push_error("WorldPresentation received an invalid movement batch header")
        return false
    if protocol_version != _protocol_version:
        push_error("movement batch protocol version does not match observed world")
        return false
    if tick != _controlled_spatial_tick + 1:
        push_warning(
            "WorldPresentation rejected non-consecutive movement tick %d after %d"
            % [tick, _controlled_spatial_tick]
        )
        return false
    if revision <= _controlled_spatial_revision or revision <= _last_revision:
        push_warning(
            "WorldPresentation rejected stale movement revision %d after %d"
            % [revision, _last_revision]
        )
        return false

    var samples_value = batch.get("samples", null)
    if typeof(samples_value) != TYPE_ARRAY:
        push_error("movement batch requires a samples array")
        return false

    var samples: Array = samples_value
    if samples.is_empty():
        push_error("movement batch must contain the controlled actor sample")
        return false

    var validated_samples: Array = []
    var previous_entity_id := 0
    var controlled_sample_found := false
    for sample_value in samples:
        if typeof(sample_value) != TYPE_DICTIONARY:
            push_error("movement sample must be a Dictionary")
            return false
        var sample: Dictionary = sample_value
        var entity_id := _read_positive_int(sample, "entity_id")
        var spatial_epoch := _read_positive_int(sample, "spatial_epoch")
        var position_value = sample.get("position_m", null)
        var velocity_value = sample.get("velocity_mps", null)
        if (
            entity_id <= previous_entity_id
            or spatial_epoch <= 0
            or typeof(position_value) != TYPE_VECTOR3
            or typeof(velocity_value) != TYPE_VECTOR3
        ):
            push_error("movement samples must be strictly EntityId-ordered and spatially valid")
            return false
        if not _observed_entity_ids.has(entity_id):
            push_error("movement sample references an entity outside the observed world")
            return false

        var binding_value = _bindings.get(entity_id, null)
        var binding := binding_value as EntityBinding
        if binding == null or binding.entity_id() != entity_id:
            push_error("movement sample has no valid observed presentation binding")
            return false
        var presentation_root := binding.get_parent() as Node3D
        if presentation_root == null or not is_ancestor_of(presentation_root):
            push_error("movement sample presentation root must live under WorldPresentation")
            return false

        var position: Vector3 = position_value
        var velocity: Vector3 = velocity_value
        validated_samples.append({
            "entity_id": entity_id,
            "position_m": position,
            "velocity_mps": velocity,
            "spatial_epoch": spatial_epoch,
            "presentation_root": presentation_root,
        })
        if entity_id == _controlled_entity_id:
            controlled_sample_found = true
        previous_entity_id = entity_id

    if not controlled_sample_found:
        push_error("movement batch is missing the controlled actor sample")
        return false

    for sample_value in validated_samples:
        var sample: Dictionary = sample_value
        var entity_id: int = sample["entity_id"]
        var presentation_root := sample["presentation_root"] as Node3D
        var position: Vector3 = sample["position_m"]
        var velocity: Vector3 = sample["velocity_mps"]
        var spatial_epoch: int = sample["spatial_epoch"]
        var previous_epoch := int(_spatial_epoch_by_entity.get(entity_id, 0))

        presentation_root.global_position = position
        if presentation_root is CharacterBody3D:
            (presentation_root as CharacterBody3D).velocity = velocity
        if previous_epoch <= 0 or previous_epoch != spatial_epoch:
            presentation_root.reset_physics_interpolation()
        _spatial_epoch_by_entity[entity_id] = spatial_epoch
        presentation_root.visible = true

        if entity_id == _controlled_entity_id:
            _controlled_spatial_epoch = spatial_epoch
            _controlled_spatial_tick = tick
            _controlled_spatial_revision = revision
            _controlled_authoritative_position = position
            _controlled_authoritative_velocity = velocity

    _last_tick = tick
    _last_revision = revision
    _movement_batches_applied += 1
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


func controlled_authoritative_position() -> Vector3:
    return _controlled_authoritative_position


func controlled_authoritative_velocity() -> Vector3:
    return _controlled_authoritative_velocity


func debug_snapshot() -> Dictionary:
    var observed_ids: Array = _observed_entity_ids.keys()
    observed_ids.sort()
    var bound_ids: Array = _bindings.keys()
    bound_ids.sort()
    var visible_bound_ids: Array = []
    for entity_id_value in bound_ids:
        var entity_id := int(entity_id_value)
        var presentation_root := presentation_for(entity_id)
        if presentation_root != null and presentation_root.visible:
            visible_bound_ids.append(entity_id)
    return {
        "controlled_entity_id": _controlled_entity_id,
        "last_tick": _last_tick,
        "last_revision": _last_revision,
        "protocol_version": _protocol_version,
        "observed_entity_ids": observed_ids,
        "bound_entity_ids": bound_ids,
        "visible_bound_entity_ids": visible_bound_ids,
        "controlled_spatial_initialized": _controlled_spatial_initialized,
        "controlled_spatial_epoch": _controlled_spatial_epoch,
        "controlled_spatial_tick": _controlled_spatial_tick,
        "controlled_spatial_revision": _controlled_spatial_revision,
        "controlled_authoritative_position_m": [
            _controlled_authoritative_position.x,
            _controlled_authoritative_position.y,
            _controlled_authoritative_position.z,
        ],
        "controlled_authoritative_velocity_mps": [
            _controlled_authoritative_velocity.x,
            _controlled_authoritative_velocity.y,
            _controlled_authoritative_velocity.z,
        ],
        "movement_batches_applied": _movement_batches_applied,
    }


func _free_detached_materializations(pending_materializations: Array) -> void:
    for pending_value in pending_materializations:
        var pending: Dictionary = pending_value
        var presentation_root := pending.get("presentation_root", null) as Node3D
        if presentation_root != null and not presentation_root.is_inside_tree():
            presentation_root.free()


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
