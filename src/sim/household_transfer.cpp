#include "sim/world.hpp"

#include <cstdint>
#include <limits>

namespace worldsim::sim {
namespace {

[[nodiscard]] constexpr bool checked_add_nonnegative(
    const std::int64_t left,
    const std::int64_t right,
    std::int64_t &out
) noexcept {
    if (left < 0 || right < 0) {
        return false;
    }
    if (left > std::numeric_limits<std::int64_t>::max() - right) {
        return false;
    }
    out = left + right;
    return true;
}

} // namespace

std::expected<HouseholdTransferResult, HouseholdTransferError>
World::execute_household_transfer_pledge(const EntityId actor) noexcept {
    if (!actor.is_valid()) {
        return std::unexpected(HouseholdTransferError::invalid_entity_id);
    }

    const auto actor_index_value = actor_index(actor);
    if (!actor_index_value.has_value()) {
        return std::unexpected(HouseholdTransferError::unknown_actor);
    }

    const auto source_index = actor_household_index(actor);
    if (!source_index.has_value()) {
        return std::unexpected(HouseholdTransferError::actor_without_household);
    }

    auto &source = households_[*source_index];
    if (!source.has_valid_resource_state()) {
        return std::unexpected(HouseholdTransferError::invalid_household_state);
    }

    const auto &pledge = source.standing_transfer_pledge;
    if (pledge.remaining_grain_units == 0) {
        return std::unexpected(HouseholdTransferError::pledge_zero);
    }
    if (!pledge.destination_household.is_valid()) {
        return std::unexpected(HouseholdTransferError::invalid_pledge_state);
    }
    if (pledge.destination_household == source.id) {
        return std::unexpected(HouseholdTransferError::self_destination);
    }

    const auto destination_index = household_index(pledge.destination_household);
    if (!destination_index.has_value()) {
        return std::unexpected(HouseholdTransferError::unknown_destination_household);
    }
    auto &destination = households_[*destination_index];
    if (!destination.has_valid_resource_state()) {
        return std::unexpected(HouseholdTransferError::invalid_household_state);
    }

    if (!actors_[*actor_index_value].spatial.has_value()) {
        return std::unexpected(HouseholdTransferError::missing_spatial_state);
    }
    if (!is_actor_inside_place(actor, source.store_place)) {
        return std::unexpected(HouseholdTransferError::outside_store);
    }
    if (source.grain_stock_units < pledge.remaining_grain_units) {
        return std::unexpected(HouseholdTransferError::insufficient_stock);
    }

    std::int64_t updated_destination_stock{};
    if (!checked_add_nonnegative(
            destination.grain_stock_units,
            pledge.remaining_grain_units,
            updated_destination_stock
        )) {
        return std::unexpected(HouseholdTransferError::stock_overflow);
    }

    const auto transferred = pledge.remaining_grain_units;
    source.grain_stock_units -= transferred;
    destination.grain_stock_units = updated_destination_stock;
    source.standing_transfer_pledge.remaining_grain_units = 0;
    ++revision_.value;

    return HouseholdTransferResult{
        .actor = actor,
        .source_household = source.id,
        .destination_household = destination.id,
        .transferred_grain_units = transferred,
        .source_grain_stock_units = source.grain_stock_units,
        .destination_grain_stock_units = destination.grain_stock_units,
        .remaining_pledge_grain_units = source.standing_transfer_pledge.remaining_grain_units,
        .tick = tick_,
        .revision = revision_,
    };
}

} // namespace worldsim::sim
