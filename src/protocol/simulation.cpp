#include "protocol/simulation.hpp"

#include "sim/acceptance_village.hpp"
#include "sim/living_need.hpp"

#include <cassert>
#include <cstddef>
#include <expected>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

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

[[nodiscard]] HouseholdResourceStatus household_resource_status(const bool shortage) noexcept {
    return shortage ? HouseholdResourceStatus::shortage : HouseholdResourceStatus::adequate;
}

[[nodiscard]] std::optional<sim::EntityId> first_living_need_actor(const sim::World &world) noexcept {
    for (const auto actor : world.actor_ids()) {
        if (world.actor_rest_need(actor).has_value()) {
            return actor;
        }
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
    const auto bindings = sim::populate_household_resource_acceptance_village(world_);
    if (!bindings.has_value()) {
        throw std::logic_error("household resource acceptance village is invalid");
    }
    controlled_actor_ = bindings->controlled_actor;
    assert(world_.contains_actor(controlled_actor_));
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

    // Reuse this one bounded actor view for movement collection and post-movement
    // autonomy. No snapshot copy, named NPC field or second entity enumeration.
    const auto actor_ids = world_.actor_ids();
    std::vector<sim::ActorGroundedMoveIntent> intents;
    intents.reserve(actor_ids.size());

    for (const auto actor : actor_ids) {
        if (actor == controlled_actor_) {
            intents.push_back(sim::ActorGroundedMoveIntent{
                .actor = actor,
                .move = sim::PlanarMoveIntent{
                    .x = controlled_move_intent_.x,
                    .z = controlled_move_intent_.z,
                },
                .pace = *controlled_pace,
            });
            continue;
        }

        if (world_.actor_rest_need(actor).has_value()) {
            const auto decision = sim::decide_npc_rest_need(world_, actor);
            if (!decision.has_value()) {
                return std::unexpected(ControlledActorMovementError::world_rejected);
            }
            intents.push_back(decision->movement);
            continue;
        }

        // Exact-spatial actors without a current movement-producing behavior still
        // join the authoritative batch with idle intent so presentation can
        // materialize them without a feature-named branch.
        if (world_.actor_spatial_state(actor).has_value()) {
            intents.push_back(sim::ActorGroundedMoveIntent{
                .actor = actor,
                .move = {},
                .pace = sim::LocomotionPace::walk,
            });
        }
    }

    // Allocate the protocol result before mutating World. Human and NPC intents
    // enter one fixed authoritative batch; result ordering is supplied by World
    // and is independent of collection order.
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

    // Movement is already committed and its batch keeps that exact revision.
    // Then bounded NPC policy may apply the same actor-generic Consume law against
    // post-movement World state. Infeasible proposals are suppressed; an ordinary
    // refusal can never retroactively turn successful locomotion into failure.
    for (const auto actor : actor_ids) {
        if (actor == controlled_actor_ || world_.revision().value >= kMaxProtocolInteger) {
            continue;
        }
        if (!world_.can_consume_household_grain(actor)) {
            continue;
        }
        (void)world_.consume_household_grain(actor);
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
    assert(controlled_actor_exists);

    ObservedWorldProjection result{
        .controlled_actor_id = controlled_actor_exists ? controlled_actor_.value : 0,
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };

    result.entities.reserve(world_.actor_ids().size());
    for (const auto actor : world_.actor_ids()) {
        result.entities.push_back(ObservedEntityProjection{.entity_id = actor.value});
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
    const auto actor = first_living_need_actor(world_);
    assert(actor.has_value());
    if (!actor.has_value()) {
        return {};
    }

    const auto decision = sim::decide_npc_rest_need(world_, *actor);
    const auto need = world_.actor_rest_need(*actor);
    assert(decision.has_value());
    assert(need.has_value());
    if (!decision.has_value() || !need.has_value()) {
        return {};
    }

    return LivingNeedProjection{
        .entity_id = actor->value,
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

VillageHouseholdResourceProjection Simulation::village_household_resource_projection() const {
    VillageHouseholdResourceProjection result{
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };
    result.households.reserve(world_.household_ids().size());

    for (const auto household_id : world_.household_ids()) {
        const auto household = world_.household_state(household_id);
        const auto shortage = world_.household_is_short(household_id);
        assert(household.has_value());
        assert(shortage.has_value());
        if (!household.has_value() || !shortage.has_value()) {
            continue;
        }

        const auto store = world_.place_state(household->store_place);
        assert(store.has_value());
        if (!store.has_value()) {
            continue;
        }

        HouseholdResourceProjection projection{
            .household_id = household->id.value,
            .store_place_id = store->id.value,
            .store_x_mm = store->x.value,
            .store_z_mm = store->z.value,
            .store_axis_tolerance_mm = store->axis_occupancy_tolerance.value,
            .grain_stock_units = household->grain_stock_units,
            .shortage_threshold_units = household->shortage_threshold_units,
            .status = household_resource_status(*shortage),
        };
        projection.member_actor_ids.reserve(household->members.size());
        for (const auto member : household->members) {
            projection.member_actor_ids.push_back(member.value);
        }
        result.households.push_back(std::move(projection));
    }

    return result;
}

} // namespace worldsim::protocol
