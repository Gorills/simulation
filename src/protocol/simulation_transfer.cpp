#include "protocol/simulation.hpp"

#include <cassert>
#include <cstdint>
#include <optional>
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

[[nodiscard]] std::optional<sim::HouseholdState> household_for_actor(
    const sim::World &world,
    const sim::EntityId actor
) {
    for (const auto household_id : world.household_ids()) {
        const auto household = world.household_state(household_id);
        if (!household.has_value()) {
            continue;
        }
        for (const auto member : household->members) {
            if (member == actor) {
                return household;
            }
        }
    }
    return std::nullopt;
}

[[nodiscard]] ControlledActorTransferError map_transfer_error(
    const sim::HouseholdTransferError error
) noexcept {
    switch (error) {
    case sim::HouseholdTransferError::invalid_entity_id:
    case sim::HouseholdTransferError::unknown_actor:
        return ControlledActorTransferError::controlled_actor_missing;
    case sim::HouseholdTransferError::actor_without_household:
        return ControlledActorTransferError::actor_without_household;
    case sim::HouseholdTransferError::invalid_household_state:
        return ControlledActorTransferError::invalid_household_state;
    case sim::HouseholdTransferError::invalid_pledge_state:
        return ControlledActorTransferError::invalid_pledge_state;
    case sim::HouseholdTransferError::unknown_destination_household:
        return ControlledActorTransferError::destination_household_missing;
    case sim::HouseholdTransferError::self_destination:
        return ControlledActorTransferError::self_destination;
    case sim::HouseholdTransferError::missing_spatial_state:
        return ControlledActorTransferError::controlled_actor_spatial_state_missing;
    case sim::HouseholdTransferError::outside_store:
        return ControlledActorTransferError::outside_store;
    case sim::HouseholdTransferError::pledge_zero:
        return ControlledActorTransferError::pledge_zero;
    case sim::HouseholdTransferError::insufficient_stock:
        return ControlledActorTransferError::insufficient_stock;
    case sim::HouseholdTransferError::stock_overflow:
        return ControlledActorTransferError::stock_overflow;
    }
    return ControlledActorTransferError::invalid_pledge_state;
}

[[nodiscard]] ControlledActorTransferResult transfer_result(
    const sim::HouseholdTransferResult &result
) {
    return ControlledActorTransferResult{
        .entity_id = result.actor.value,
        .source_household_id = result.source_household.value,
        .destination_household_id = result.destination_household.value,
        .transferred_grain_units = result.transferred_grain_units,
        .source_household_grain_stock_units = result.source_grain_stock_units,
        .destination_household_grain_stock_units = result.destination_grain_stock_units,
        .remaining_pledge_grain_units = result.remaining_pledge_grain_units,
        .tick = checked_protocol_integer(result.tick.value),
        .revision = checked_protocol_integer(result.revision.value),
        .protocol_version = kProtocolVersion,
    };
}

} // namespace

ControlledActorTransferOutcome Simulation::controlled_actor_execute_household_transfer_pledge() {
    if (world_.revision().value >= kMaxProtocolInteger) {
        return std::unexpected(ControlledActorTransferError::protocol_integer_exhausted);
    }

    const auto outcome = world_.execute_household_transfer_pledge(controlled_actor_);
    if (!outcome.has_value()) {
        return std::unexpected(map_transfer_error(outcome.error()));
    }
    return transfer_result(*outcome);
}

StandingTransferPledgeProjection Simulation::standing_transfer_pledge_projection() const {
    const auto household = household_for_actor(world_, controlled_actor_);
    assert(household.has_value());
    if (!household.has_value()) {
        return {};
    }

    return StandingTransferPledgeProjection{
        .source_household_id = household->id.value,
        .destination_household_id =
            household->standing_transfer_pledge.destination_household.value,
        .remaining_grain_units = household->standing_transfer_pledge.remaining_grain_units,
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };
}

} // namespace worldsim::protocol
