#include "sim/world.hpp"

#include <cassert>
#include <expected>
#include <utility>

namespace worldsim::sim {

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

    const auto [index_entry, inserted] = actor_index_by_id_.emplace(id.value, actors_.size());
    if (!inserted) {
        return std::unexpected(WorldError::duplicate_entity);
    }

    try {
        actors_.push_back(ActorState{
            .id = id,
            .bootstrap_position = initial.bootstrap_position,
            .spatial = initial.spatial,
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
