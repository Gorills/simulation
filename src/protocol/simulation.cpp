#include "protocol/simulation.hpp"

#include <cassert>
#include <expected>
#include <optional>

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

} // namespace

Simulation::Simulation(const std::uint64_t seed) : world_(sim::WorldSeed{seed}) {
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

BootstrapMoveOutcome Simulation::bootstrap_move(const BootstrapMoveIntent &intent) noexcept {
    const auto direction = cardinal_direction(intent);
    if (!direction.has_value()) {
        return std::unexpected(BootstrapMoveError::invalid_delta);
    }

    const auto moved = world_.apply_bootstrap_step(controlled_actor_, *direction);
    if (!moved.has_value()) {
        return std::unexpected(BootstrapMoveError::controlled_actor_missing);
    }

    return BootstrapMoveResult{.actor = bootstrap_controlled_actor_projection()};
}

BootstrapActorProjection Simulation::bootstrap_controlled_actor_projection() const noexcept {
    const auto *actor = world_.actor(controlled_actor_);
    assert(actor != nullptr);
    if (actor == nullptr) {
        return {};
    }

    return BootstrapActorProjection{
        .entity_id = actor->id.value,
        .x = actor->bootstrap_position.x,
        .y = actor->bootstrap_position.y,
        .tick = world_.tick().value,
        .revision = world_.revision().value,
        .seed = world_.seed().value,
        .protocol_version = kProtocolVersion,
    };
}

ObservedWorldProjection Simulation::observed_world_projection() const {
    const auto *actor = world_.actor(controlled_actor_);
    assert(actor != nullptr);

    ObservedWorldProjection result{
        .controlled_actor_id = actor == nullptr ? 0 : actor->id.value,
        .tick = world_.tick().value,
        .revision = world_.revision().value,
        .protocol_version = kProtocolVersion,
    };

    if (actor != nullptr) {
        result.entities.push_back(ObservedEntityProjection{.entity_id = actor->id.value});
    }
    return result;
}

ControlledActorSpatialProjection Simulation::controlled_actor_spatial_projection() const noexcept {
    const auto *actor = world_.actor(controlled_actor_);
    const auto *spatial = world_.actor_spatial_state(controlled_actor_);
    assert(actor != nullptr);
    assert(spatial != nullptr);
    if (actor == nullptr || spatial == nullptr) {
        return {};
    }

    return ControlledActorSpatialProjection{
        .entity_id = actor->id.value,
        .x_mm = spatial->position.x.value,
        .y_mm = spatial->position.y.value,
        .z_mm = spatial->position.z.value,
        .velocity_x_mm_per_second = spatial->velocity.x.value,
        .velocity_y_mm_per_second = spatial->velocity.y.value,
        .velocity_z_mm_per_second = spatial->velocity.z.value,
        .spatial_epoch = spatial->epoch.value,
        .tick = world_.tick().value,
        .revision = world_.revision().value,
        .protocol_version = kProtocolVersion,
    };
}

} // namespace worldsim::protocol
