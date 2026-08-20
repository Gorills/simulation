#include "protocol/simulation.hpp"

#include <cstdint>
#include <stdexcept>

namespace worldsim::protocol {
namespace {

[[nodiscard]] ProtocolInteger checked_protocol_integer(const std::uint64_t value) {
    const auto converted = to_protocol_integer(value);
    if (!converted.has_value()) {
        throw std::overflow_error("authoritative value exceeds protocol integer range");
    }
    return *converted;
}

[[nodiscard]] ControlledActorReciprocalAidError map_reciprocal_aid_error(
    const sim::HouseholdReciprocalAidError error
) noexcept {
    switch (error) {
    case sim::HouseholdReciprocalAidError::invalid_entity_id:
    case sim::HouseholdReciprocalAidError::unknown_household:
        return ControlledActorReciprocalAidError::target_household_missing;
    case sim::HouseholdReciprocalAidError::unknown_actor:
        return ControlledActorReciprocalAidError::controlled_actor_missing;
    case sim::HouseholdReciprocalAidError::invalid_actor_grain_carry_state:
        return ControlledActorReciprocalAidError::invalid_actor_carry_state;
    case sim::HouseholdReciprocalAidError::invalid_household_state:
        return ControlledActorReciprocalAidError::invalid_household_state;
    case sim::HouseholdReciprocalAidError::missing_spatial_state:
        return ControlledActorReciprocalAidError::controlled_actor_spatial_state_missing;
    case sim::HouseholdReciprocalAidError::outside_store:
        return ControlledActorReciprocalAidError::outside_store;
    case sim::HouseholdReciprocalAidError::no_remembered_aid:
        return ControlledActorReciprocalAidError::no_remembered_aid;
    case sim::HouseholdReciprocalAidError::remembered_for_other_actor:
        return ControlledActorReciprocalAidError::remembered_for_other_actor;
    case sim::HouseholdReciprocalAidError::carry_full:
        return ControlledActorReciprocalAidError::carry_full;
    case sim::HouseholdReciprocalAidError::insufficient_surplus:
        return ControlledActorReciprocalAidError::insufficient_surplus;
    }
    return ControlledActorReciprocalAidError::invalid_household_state;
}

[[nodiscard]] ControlledActorReciprocalAidResult reciprocal_aid_result(
    const sim::HouseholdReciprocalAidResult &result
) {
    return ControlledActorReciprocalAidResult{
        .entity_id = result.actor.value,
        .household_id = result.household.value,
        .received_grain_units = result.received_grain_units,
        .carried_grain_units = result.carried_grain_units,
        .remaining_household_grain_stock_units = result.remaining_grain_stock_units,
        .tick = checked_protocol_integer(result.tick.value),
        .revision = checked_protocol_integer(result.revision.value),
        .protocol_version = kProtocolVersion,
    };
}

} // namespace

ControlledActorReciprocalAidOutcome Simulation::controlled_actor_request_reciprocal_aid(
    const ProtocolInteger household_id
) {
    if (world_.revision().value >= kMaxProtocolInteger) {
        return std::unexpected(ControlledActorReciprocalAidError::protocol_integer_exhausted);
    }

    const auto outcome = world_.request_household_reciprocal_aid(
        controlled_actor_,
        sim::EntityId{household_id}
    );
    if (!outcome.has_value()) {
        return std::unexpected(map_reciprocal_aid_error(outcome.error()));
    }
    return reciprocal_aid_result(*outcome);
}

ControlledActorReciprocalAidProjectionOutcome Simulation::reciprocal_aid_projection(
    const ProtocolInteger household_id
) const {
    if (
        world_.tick().value > kMaxProtocolInteger ||
        world_.revision().value > kMaxProtocolInteger
    ) {
        return std::unexpected(ControlledActorReciprocalAidError::protocol_integer_exhausted);
    }
    if (!world_.contains_actor(controlled_actor_)) {
        return std::unexpected(ControlledActorReciprocalAidError::controlled_actor_missing);
    }

    const sim::EntityId target{household_id};
    if (!target.is_valid()) {
        return std::unexpected(ControlledActorReciprocalAidError::target_household_missing);
    }
    const auto household = world_.household_state(target);
    if (!household.has_value()) {
        return std::unexpected(ControlledActorReciprocalAidError::target_household_missing);
    }
    if (!household->has_valid_resource_state() || !household->has_valid_social_state()) {
        return std::unexpected(ControlledActorReciprocalAidError::invalid_household_state);
    }

    return ControlledActorReciprocalAidProjection{
        .household_id = household->id.value,
        .remembered_for_controlled_actor =
            household->remembered_material_aid_actor == controlled_actor_,
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };
}

} // namespace worldsim::protocol
