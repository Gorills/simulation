#include "sim/world.hpp"

#include <expected>

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
    if (actor(id) != nullptr) {
        return std::unexpected(WorldError::duplicate_entity);
    }

    actors_.push_back(ActorState{
        .id = id,
        .bootstrap_position = initial.bootstrap_position,
        .spatial = initial.spatial,
    });
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

const ActorState *World::actor(const EntityId id) const noexcept {
    if (!id.is_valid()) {
        return nullptr;
    }
    for (const auto &state : actors_) {
        if (state.id == id) {
            return &state;
        }
    }
    return nullptr;
}

const SpatialState *World::actor_spatial_state(const EntityId id) const noexcept {
    const auto *state = actor(id);
    if (state == nullptr || !state->spatial.has_value()) {
        return nullptr;
    }
    return &*state->spatial;
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

ActorState *World::find_actor(const EntityId id) noexcept {
    if (!id.is_valid()) {
        return nullptr;
    }
    for (auto &state : actors_) {
        if (state.id == id) {
            return &state;
        }
    }
    return nullptr;
}

} // namespace worldsim::sim
