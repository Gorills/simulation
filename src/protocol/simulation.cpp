#include "protocol/simulation.hpp"

#include "sim/living_need.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <expected>
#include <optional>
#include <stdexcept>

namespace worldsim::protocol {
namespace {

static_assert(kPlanarMoveIntentScale == sim::kIntentScale);

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

[[nodiscard]] bool is_valid_move_intent(const ControlledActorMoveIntent &intent) noexcept {
    if (
        intent.x < -kPlanarMoveIntentScale || intent.x > kPlanarMoveIntentScale ||
        intent.z < -kPlanarMoveIntentScale || intent.z > kPlanarMoveIntentScale
    ) {
        return false;
    }
    const auto x = static_cast<std::int64_t>(intent.x);
    const auto z = static_cast<std::int64_t>(intent.z);
    const auto scale = static_cast<std::int64_t>(kPlanarMoveIntentScale);
    return x * x + z * z <= scale * scale;
}

[[nodiscard]] std::optional<sim::LocomotionPace> simulation_pace(
    const ControlledActorLocomotionPace pace
) noexcept {
    switch (pace) {
    case ControlledActorLocomotionPace::walk:
        return sim::LocomotionPace::walk;
    case ControlledActorLocomotionPace::run:
        return sim::LocomotionPace::run;
    case ControlledActorLocomotionPace::sprint:
        return sim::LocomotionPace::sprint;
    }
    return std::nullopt;
}

[[nodiscard]] LivingNeedStatus living_need_status(
    const sim::NpcRestNeedDecision &decision
) noexcept {
    if (decision.satisfied) {
        return LivingNeedStatus::satisfied;
    }
    if (decision.blocked_by_other_actor) {
        return LivingNeedStatus::blocked;
    }
    return LivingNeedStatus::traveling;
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

[[nodiscard]] ControlledActorMovementError map_world_locomotion_error(
    const sim::GroundedLocomotionTickError error
) noexcept {
    switch (error) {
    case sim::GroundedLocomotionTickError::unknown_entity:
    case sim::GroundedLocomotionTickError::invalid_entity_id:
        return ControlledActorMovementError::controlled_actor_missing;
    case sim::GroundedLocomotionTickError::missing_spatial_state:
        return ControlledActorMovementError::controlled_actor_spatial_state_missing;
    case sim::GroundedLocomotionTickError::invalid_pace:
        return ControlledActorMovementError::invalid_pace;
    case sim::GroundedLocomotionTickError::invalid_context:
    case sim::GroundedLocomotionTickError::duplicate_actor_intent:
    case sim::GroundedLocomotionTickError::invalid_locomotion_capability:
    case sim::GroundedLocomotionTickError::invalid_continuation_state:
    case sim::GroundedLocomotionTickError::incompatible_tick_rate:
    case sim::GroundedLocomotionTickError::invalid_intent:
    case sim::GroundedLocomotionTickError::no_ground_support:
    case sim::GroundedLocomotionTickError::arithmetic_overflow:
        return ControlledActorMovementError::world_rejected;
    }
    return ControlledActorMovementError::world_rejected;
}

[[nodiscard]] AuthoritativeMovementSample movement_sample(
    const sim::GroundedLocomotionSample &sample
) {
    return AuthoritativeMovementSample{
        .entity_id = sample.actor.value,
        .x_mm = sample.spatial.position.x.value,
        .y_mm = sample.spatial.position.y.value,
        .z_mm = sample.spatial.position.z.value,
        .velocity_x_mm_per_second = sample.spatial.velocity.x.value,
        .velocity_y_mm_per_second = sample.spatial.velocity.y.value,
        .velocity_z_mm_per_second = sample.spatial.velocity.z.value,
        .spatial_epoch = checked_protocol_integer(sample.spatial.epoch.value),
    };
}

} // namespace

Simulation::Simulation(const ProtocolInteger seed)
    : world_(checked_world_seed(seed)),
      locomotion_context_(sim::make_flat_locomotion_acceptance_context()) {
    const auto controlled_spawned = world_.spawn_actor(
        controlled_actor_,
        sim::ActorSpawnState{
            .spatial = sim::SpatialState{
                .position = {},
                .velocity = {},
                .epoch = sim::SpatialEpoch{1},
            },
        }
    );
    assert(controlled_spawned.has_value());
    (void)controlled_spawned;

    // First Milestone 1 acceptance scenario: the NPC starts six meters away
    // from its assigned local rest point. RestNeedState is authoritative actor
    // state; the visible Godot shell merely presents the resulting samples.
    const auto npc_spawned = world_.spawn_actor(
        living_need_npc_,
        sim::ActorSpawnState{
            .spatial = sim::SpatialState{
                .position = {
                    .x = sim::Millimeters{3'000},
                    .y = sim::Millimeters{0},
                    .z = sim::Millimeters{-3'000},
                },
                .velocity = {},
                .epoch = sim::SpatialEpoch{1},
            },
            .rest_need = sim::RestNeedState{
                .rest_x = sim::Millimeters{-3'000},
                .rest_z = sim::Millimeters{-3'000},
                .axis_arrival_tolerance = sim::Millimeters{150},
            },
        }
    );
    assert(npc_spawned.has_value());
    (void)npc_spawned;
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

ControlledActorMoveIntentOutcome Simulation::submit_controlled_actor_move_intent(
    const ControlledActorMoveIntent &intent
) noexcept {
    if (!is_valid_move_intent(intent)) {
        return std::unexpected(ControlledActorMovementError::invalid_intent);
    }
    if (!is_valid_controlled_actor_locomotion_pace(intent.pace)) {
        return std::unexpected(ControlledActorMovementError::invalid_pace);
    }
    controlled_move_intent_ = intent;
    return {};
}

ControlledActorLocomotionTickOutcome Simulation::advance_locomotion_tick() {
    if (
        world_.tick().value >= kMaxProtocolInteger ||
        world_.revision().value >= kMaxProtocolInteger
    ) {
        return std::unexpected(ControlledActorMovementError::protocol_integer_exhausted);
    }

    const auto controlled_pace = simulation_pace(controlled_move_intent_.pace);
    if (!controlled_pace.has_value()) {
        return std::unexpected(ControlledActorMovementError::invalid_pace);
    }

    const auto npc_decision = sim::decide_npc_rest_need(world_, living_need_npc_);
    if (!npc_decision.has_value()) {
        return std::unexpected(ControlledActorMovementError::world_rejected);
    }

    const std::array intents{
        sim::ActorGroundedMoveIntent{
            .actor = controlled_actor_,
            .move = sim::PlanarMoveIntent{
                .x = controlled_move_intent_.x,
                .z = controlled_move_intent_.z,
            },
            .pace = *controlled_pace,
        },
        npc_decision->movement,
    };

    // Allocate the protocol result before mutating World. Both human and NPC
    // intents enter one fixed authoritative batch; result ordering is supplied
    // by World and is independent of collection order.
    AuthoritativeMovementSampleBatch result{};
    result.samples.resize(intents.size());

    const auto advanced = world_.advance_grounded_locomotion_tick(
        locomotion_context_,
        intents
    );
    if (!advanced.has_value()) {
        return std::unexpected(map_world_locomotion_error(advanced.error()));
    }

    assert(advanced->samples.size() == result.samples.size());
    result.tick = checked_protocol_integer(advanced->tick.value);
    result.revision = checked_protocol_integer(advanced->revision.value);
    result.protocol_version = kProtocolVersion;
    for (std::size_t index = 0; index < advanced->samples.size(); ++index) {
        result.samples[index] = movement_sample(advanced->samples[index]);
    }
    return result;
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
    const bool npc_exists = world_.contains_actor(living_need_npc_);
    assert(controlled_actor_exists);
    assert(npc_exists);

    ObservedWorldProjection result{
        .controlled_actor_id = controlled_actor_exists ? controlled_actor_.value : 0,
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };

    if (controlled_actor_exists) {
        result.entities.push_back(ObservedEntityProjection{.entity_id = controlled_actor_.value});
    }
    if (npc_exists) {
        result.entities.push_back(ObservedEntityProjection{.entity_id = living_need_npc_.value});
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

LivingNeedProjection Simulation::living_need_projection() const {
    const auto decision = sim::decide_npc_rest_need(world_, living_need_npc_);
    const auto need = world_.actor_rest_need(living_need_npc_);
    assert(decision.has_value());
    assert(need.has_value());
    if (!decision.has_value() || !need.has_value()) {
        return {};
    }

    return LivingNeedProjection{
        .entity_id = living_need_npc_.value,
        .status = living_need_status(*decision),
        .target_x_mm = need->rest_x.value,
        .target_z_mm = need->rest_z.value,
        .axis_arrival_tolerance_mm = need->axis_arrival_tolerance.value,
        .axis_occupancy_tolerance_mm =
            need->axis_arrival_tolerance.value + sim::kFirstPlayableBody.radius.value,
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };
}

} // namespace worldsim::protocol
