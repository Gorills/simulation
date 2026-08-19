#include "protocol/simulation.hpp"

#include <cassert>
#include <expected>
#include <optional>
#include <stdexcept>

namespace worldsim::protocol {
namespace {

[[nodiscard]] std::optional<sim::CardinalDirection> cardinal_direction(
    const BootstrapMoveIntent &intent
) noexcept {
    if (intent.dx == 0 && intent.dy == -1) {
        return sim::CardinalDirection::north;
    }
    if (intent.dx == 1 && intent.dy == 0) {
        return sim::CardinalDirection::east;
    }
    if (intent.dx == 0 && intent.dy == 1) {
        return sim::CardinalDirection::south;
    }
    if (intent.dx == -1 && intent.dy == 0) {
        return sim::CardinalDirection::west;
    }
    return std::nullopt;
}

[[nodiscard]] sim::WorldSeed checked_world_seed(const ProtocolInteger seed) {
    if (seed < 0) {
        throw std::invalid_argument("protocol seed must be non-negative");
    }
    return sim::WorldSeed{static_cast<std::uint64_t>(seed)};
}

[[nodiscard]] ProtocolInteger checked_protocol_integer(const std::uint64_t value) {
    const auto converted = to_protocol_integer(value);
    if (!converted.has_value()) {
        throw std::overflow_error("authoritative value exceeds protocol integer range");
    }
    return *converted;
}

} // namespace

Simulation::Simulation(const ProtocolInteger seed) : world_(checked_world_seed(seed)) {
    const auto spawned = world_.spawn_actor(
        controlled_actor_,
        sim::ActorSpawnState{
            .spatial = sim::SpatialState{
                .position = {},
                .velocity = {},
                .epoch = sim::SpatialEpoch{1},
            },
        }
    );
    assert(spawned.has_value());
    (void)spawned;
}

BootstrapMoveOutcome Simulation::bootstrap_move(const BootstrapMoveIntent &intent) {
    const auto direction = cardinal_direction(intent);
    if (!direction.has_value()) {
        return std::unexpected(BootstrapMoveError::invalid_delta);
    }
    if (world_.revision().value >= kMaxProtocolInteger) {
        return std::unexpected(BootstrapMoveError::protocol_integer_exhausted);
    }

    const auto moved = world_.apply_bootstrap_step(controlled_actor_, *direction);
    if (!moved.has_value()) {
        return std::unexpected(BootstrapMoveError::controlled_actor_missing);
    }

    return BootstrapMoveResult{.actor = bootstrap_controlled_actor_projection()};
}

BootstrapActorProjection Simulation::bootstrap_controlled_actor_projection() const {
    const auto position = world_.actor_bootstrap_position(controlled_actor_);
    assert(position.has_value());
    if (!position.has_value()) {
        return {};
    }

    return BootstrapActorProjection{
        .entity_id = controlled_actor_.value,
        .x = position->x,
        .y = position->y,
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .seed = checked_protocol_integer(world_.seed().value),
        .protocol_version = kProtocolVersion,
    };
}

ObservedWorldProjection Simulation::observed_world_projection() const {
    const bool controlled_actor_exists = world_.contains_actor(controlled_actor_);
    assert(controlled_actor_exists);

    ObservedWorldProjection result{
        .controlled_actor_id = controlled_actor_exists ? controlled_actor_.value : 0,
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };

    if (controlled_actor_exists) {
        result.entities.push_back(ObservedEntityProjection{.entity_id = controlled_actor_.value});
    }
    return result;
}

ControlledActorSpatialProjection Simulation::controlled_actor_spatial_projection() const {
    const auto spatial = world_.actor_spatial_state(controlled_actor_);
    assert(spatial.has_value());
    if (!spatial.has_value()) {
        return {};
    }

    return ControlledActorSpatialProjection{
        .entity_id = controlled_actor_.value,
        .x_mm = spatial->position.x.value,
        .y_mm = spatial->position.y.value,
        .z_mm = spatial->position.z.value,
        .velocity_x_mm_per_second = spatial->velocity.x.value,
        .velocity_y_mm_per_second = spatial->velocity.y.value,
        .velocity_z_mm_per_second = spatial->velocity.z.value,
        .spatial_epoch = checked_protocol_integer(spatial->epoch.value),
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };
}

} // namespace worldsim::protocol
