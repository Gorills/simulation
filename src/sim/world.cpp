#include "sim/world.hpp"

#include <algorithm>
#include <cassert>
#include <expected>
#include <utility>
#include <vector>

namespace worldsim::sim {
namespace {

struct PendingGroundedMove final {
    std::size_t actor_index{};
    GroundedStepState next{};
};

[[nodiscard]] constexpr std::uint64_t unsigned_distance(
    const std::int64_t first,
    const std::int64_t second
) noexcept {
    if (first >= second) {
        return static_cast<std::uint64_t>(first) - static_cast<std::uint64_t>(second);
    }
    return static_cast<std::uint64_t>(second) - static_cast<std::uint64_t>(first);
}

[[nodiscard]] GroundedLocomotionTickError map_step_error(const GroundedStepError error) noexcept {
    switch (error) {
    case GroundedStepError::invalid_environment:
    case GroundedStepError::invalid_body:
    case GroundedStepError::invalid_config:
        return GroundedLocomotionTickError::invalid_context;
    case GroundedStepError::invalid_state:
        return GroundedLocomotionTickError::invalid_continuation_state;
    case GroundedStepError::invalid_intent:
        return GroundedLocomotionTickError::invalid_intent;
    case GroundedStepError::no_ground_support:
        return GroundedLocomotionTickError::no_ground_support;
    case GroundedStepError::arithmetic_overflow:
        return GroundedLocomotionTickError::arithmetic_overflow;
    }
    return GroundedLocomotionTickError::arithmetic_overflow;
}

// This is the single actor-state -> solver-limits seam. Today it resolves only
// the actor's stored base locomotion capability plus semantic pace. When a real
// mechanic later introduces wounds, carried load, progression or a concrete
// magical effect, its authoritative state belongs here rather than in input,
// Godot, an NPC-only multiplier, or a duplicate movement solver.
[[nodiscard]] std::expected<GroundedStepConfig, GroundedLocomotionTickError>
resolve_grounded_step_config(
    const GroundedStepConfig &world_config,
    const ActorState &actor,
    const LocomotionPace pace
) noexcept {
    if (!actor.locomotion_capability.is_valid()) {
        return std::unexpected(GroundedLocomotionTickError::invalid_locomotion_capability);
    }
    if (!is_valid_locomotion_pace(pace)) {
        return std::unexpected(GroundedLocomotionTickError::invalid_pace);
    }

    GroundedStepConfig resolved = world_config;
    switch (pace) {
    case LocomotionPace::walk:
        resolved.move_speed = actor.locomotion_capability.walk_speed;
        break;
    case LocomotionPace::run:
        resolved.move_speed = actor.locomotion_capability.run_speed;
        break;
    case LocomotionPace::sprint:
        resolved.move_speed = actor.locomotion_capability.sprint_speed;
        break;
    }
    resolved.acceleration = actor.locomotion_capability.acceleration;
    resolved.braking = actor.locomotion_capability.braking;
    return resolved;
}

} // namespace

World::World(const WorldSeed seed) noexcept : seed_(seed) {}

std::expected<void, WorldError> World::spawn_actor(
    const EntityId id,
    const ActorSpawnState initial
) {
    if (!id.is_valid()) {
        return std::unexpected(WorldError::invalid_entity_id);
    }
    if (initial.spatial.has_value() && !initial.spatial->is_valid()) {
        return std::unexpected(WorldError::invalid_spatial_state);
    }
    if (!initial.locomotion_capability.is_valid()) {
        return std::unexpected(WorldError::invalid_locomotion_capability);
    }
    if (initial.rest_need.has_value() && !initial.rest_need->is_valid()) {
        return std::unexpected(WorldError::invalid_rest_need_state);
    }

    const auto [index_entry, inserted] = actor_index_by_id_.emplace(id.value, actors_.size());
    if (!inserted) {
        return std::unexpected(WorldError::duplicate_entity);
    }

    try {
        actors_.push_back(ActorState{
            .id = id,
            .bootstrap_position = initial.bootstrap_position,
            .spatial = initial.spatial,
            .locomotion_capability = initial.locomotion_capability,
            .rest_need = initial.rest_need,
        });
    } catch (...) {
        actor_index_by_id_.erase(index_entry);
        throw;
    }

    ++revision_.value;
    return {};
}

std::expected<void, WorldError> World::apply_bootstrap_step(
    const EntityId id,
    const CardinalDirection direction
) noexcept {
    if (!id.is_valid()) {
        return std::unexpected(WorldError::invalid_entity_id);
    }

    auto *state = find_actor(id);
    if (state == nullptr) {
        return std::unexpected(WorldError::unknown_entity);
    }

    switch (direction) {
    case CardinalDirection::north:
        --state->bootstrap_position.y;
        break;
    case CardinalDirection::east:
        ++state->bootstrap_position.x;
        break;
    case CardinalDirection::south:
        ++state->bootstrap_position.y;
        break;
    case CardinalDirection::west:
        --state->bootstrap_position.x;
        break;
    }

    ++revision_.value;
    return {};
}

std::expected<GroundedLocomotionTickResult, GroundedLocomotionTickError>
World::advance_grounded_locomotion_tick(
    const GroundedLocomotionContext &context,
    const std::span<const ActorGroundedMoveIntent> intents
) {
    if (!context.is_valid()) {
        return std::unexpected(GroundedLocomotionTickError::invalid_context);
    }

    std::vector<PendingGroundedMove> pending;
    pending.reserve(intents.size());

    for (const auto &intent : intents) {
        if (!intent.actor.is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_entity_id);
        }
        if (!intent.move.is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_intent);
        }
        if (!is_valid_locomotion_pace(intent.pace)) {
            return std::unexpected(GroundedLocomotionTickError::invalid_pace);
        }

        const auto index = actor_index(intent.actor);
        if (!index.has_value()) {
            return std::unexpected(GroundedLocomotionTickError::unknown_entity);
        }
        for (const auto &already_pending : pending) {
            if (already_pending.actor_index == *index) {
                return std::unexpected(GroundedLocomotionTickError::duplicate_actor_intent);
            }
        }

        const auto &actor = actors_[*index];
        if (!actor.spatial.has_value()) {
            return std::unexpected(GroundedLocomotionTickError::missing_spatial_state);
        }
        if (!actor.locomotion_capability.is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_locomotion_capability);
        }
        if (!actor.grounded_locomotion.is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_continuation_state);
        }
        if (!actor.grounded_locomotion.is_compatible(context.config.ticks_per_second)) {
            return std::unexpected(GroundedLocomotionTickError::incompatible_tick_rate);
        }

        const auto resolved_config = resolve_grounded_step_config(context.config, actor, intent.pace);
        if (!resolved_config.has_value()) {
            return std::unexpected(resolved_config.error());
        }
        if (!resolved_config->is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_context);
        }

        const auto step = step_grounded(
            context.environment,
            context.body,
            *resolved_config,
            GroundedStepState{
                .spatial = *actor.spatial,
                .remainder = actor.grounded_locomotion.remainder,
            },
            intent.move
        );
        if (!step.has_value()) {
            return std::unexpected(map_step_error(step.error()));
        }

        pending.push_back(PendingGroundedMove{
            .actor_index = *index,
            .next = *step,
        });
    }

    // Build and order the complete public result before mutating World state.
    // Allocation failure therefore cannot leave a successful subset committed.
    std::vector<GroundedLocomotionSample> samples;
    samples.reserve(pending.size());
    for (const auto &update : pending) {
        samples.push_back(GroundedLocomotionSample{
            .actor = actors_[update.actor_index].id,
            .spatial = update.next.spatial,
        });
    }
    std::sort(
        samples.begin(),
        samples.end(),
        [](const GroundedLocomotionSample &left, const GroundedLocomotionSample &right) {
            return left.actor.value < right.actor.value;
        }
    );

    for (const auto &update : pending) {
        auto &actor = actors_[update.actor_index];
        actor.spatial = update.next.spatial;
        actor.grounded_locomotion = GroundedLocomotionContinuation{
            .remainder = update.next.remainder,
            .ticks_per_second = context.config.ticks_per_second,
        };
    }

    ++tick_.value;
    ++revision_.value;
    return GroundedLocomotionTickResult{
        .tick = tick_,
        .revision = revision_,
        .samples = std::move(samples),
    };
}

void World::advance_one_tick() noexcept {
    ++tick_.value;
    ++revision_.value;
}

WorldSnapshot World::snapshot() const {
    return WorldSnapshot{
        .schema_version = kWorldSnapshotSchemaVersion,
        .seed = seed_,
        .tick = tick_,
        .revision = revision_,
        .actors = actors_,
    };
}

std::expected<void, WorldSnapshotError> World::restore(const WorldSnapshot &snapshot_state) {
    if (snapshot_state.schema_version != kWorldSnapshotSchemaVersion) {
        return std::unexpected(WorldSnapshotError::unsupported_schema_version);
    }

    // Build into a temporary World so malformed snapshots or allocation failure
    // never partially mutate the current authoritative state.
    World restored{snapshot_state.seed};
    restored.actors_.reserve(snapshot_state.actors.size());
    restored.actor_index_by_id_.reserve(snapshot_state.actors.size());

    for (const auto &actor : snapshot_state.actors) {
        if (!actor.id.is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_entity_id);
        }
        if (actor.spatial.has_value() && !actor.spatial->is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_spatial_state);
        }
        if (!actor.locomotion_capability.is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_locomotion_capability);
        }
        if (actor.rest_need.has_value() && !actor.rest_need->is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_rest_need_state);
        }
        if (
            !actor.grounded_locomotion.is_valid() ||
            (!actor.spatial.has_value() && !actor.grounded_locomotion.is_pristine())
        ) {
            return std::unexpected(WorldSnapshotError::invalid_grounded_locomotion_state);
        }

        const bool inserted = restored.actor_index_by_id_
                                  .emplace(actor.id.value, restored.actors_.size())
                                  .second;
        if (!inserted) {
            return std::unexpected(WorldSnapshotError::duplicate_entity);
        }
        restored.actors_.push_back(actor);
    }

    restored.tick_ = snapshot_state.tick;
    restored.revision_ = snapshot_state.revision;
    *this = std::move(restored);
    return {};
}

bool World::contains_actor(const EntityId id) const noexcept {
    return id.is_valid() && actor_index_by_id_.contains(id.value);
}

std::optional<GridPosition> World::actor_bootstrap_position(const EntityId id) const noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return actors_[*index].bootstrap_position;
}

std::optional<SpatialState> World::actor_spatial_state(const EntityId id) const noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return actors_[*index].spatial;
}

std::optional<ActorLocomotionCapability> World::actor_locomotion_capability(const EntityId id) const noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return actors_[*index].locomotion_capability;
}

std::optional<RestNeedState> World::actor_rest_need(const EntityId id) const noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return actors_[*index].rest_need;
}

bool World::is_planar_position_occupied_by_other_actor(
    const EntityId excluded_actor,
    const Millimeters x,
    const Millimeters z,
    const Millimeters axis_tolerance
) const noexcept {
    if (!excluded_actor.is_valid() || axis_tolerance.value < 0) {
        return false;
    }

    const auto tolerance = static_cast<std::uint64_t>(axis_tolerance.value);
    for (const auto &actor : actors_) {
        if (actor.id == excluded_actor || !actor.spatial.has_value()) {
            continue;
        }
        if (
            unsigned_distance(actor.spatial->position.x.value, x.value) <= tolerance &&
            unsigned_distance(actor.spatial->position.z.value, z.value) <= tolerance
        ) {
            return true;
        }
    }
    return false;
}

SimulationTick World::tick() const noexcept {
    return tick_;
}

WorldRevision World::revision() const noexcept {
    return revision_;
}

WorldSeed World::seed() const noexcept {
    return seed_;
}

std::optional<std::size_t> World::actor_index(const EntityId id) const noexcept {
    if (!id.is_valid()) {
        return std::nullopt;
    }

    const auto entry = actor_index_by_id_.find(id.value);
    if (entry == actor_index_by_id_.end()) {
        return std::nullopt;
    }

    assert(entry->second < actors_.size());
    assert(actors_[entry->second].id == id);
    return entry->second;
}

ActorState *World::find_actor(const EntityId id) noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return nullptr;
    }
    return &actors_[*index];
}

} // namespace worldsim::sim
