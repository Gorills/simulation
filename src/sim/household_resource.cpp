#include "sim/world.hpp"

#include <cstdint>

namespace worldsim::sim {
namespace {

[[nodiscard]] constexpr std::uint64_t unsigned_distance(
    const std::int64_t first,
    const std::int64_t second
) noexcept {
    if (first >= second) {
        return static_cast<std::uint64_t>(first) - static_cast<std::uint64_t>(second);
    }
    return static_cast<std::uint64_t>(second) - static_cast<std::uint64_t>(first);
}

} // namespace

bool World::is_actor_inside_place(const EntityId actor, const EntityId place) const noexcept {
    const auto actor_index_value = actor_index(actor);
    const auto place_index_value = place_index(place);
    if (!actor_index_value.has_value() || !place_index_value.has_value()) {
        return false;
    }

    const auto &actor_state = actors_[*actor_index_value];
    if (!actor_state.spatial.has_value()) {
        return false;
    }

    const auto &place_state_value = places_[*place_index_value];
    const auto tolerance =
        static_cast<std::uint64_t>(place_state_value.axis_occupancy_tolerance.value);
    return unsigned_distance(
               actor_state.spatial->position.x.value,
               place_state_value.x.value
           ) <= tolerance
        && unsigned_distance(
               actor_state.spatial->position.z.value,
               place_state_value.z.value
           ) <= tolerance;
}

std::optional<bool> World::household_is_short(const EntityId id) const noexcept {
    const auto index = household_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    const auto &household = households_[*index];
    if (!household.has_valid_resource_state()) {
        return std::nullopt;
    }
    return household.grain_stock_units < household.shortage_threshold_units;
}

std::expected<HouseholdConsumeResult, HouseholdConsumeError>
World::consume_household_grain(const EntityId actor) noexcept {
    if (!actor.is_valid()) {
        return std::unexpected(HouseholdConsumeError::invalid_entity_id);
    }

    const auto actor_index_value = actor_index(actor);
    if (!actor_index_value.has_value()) {
        return std::unexpected(HouseholdConsumeError::unknown_actor);
    }

    std::optional<std::size_t> household_index_value;
    for (
        std::size_t index = 0;
        index < households_.size() && !household_index_value.has_value();
        ++index
    ) {
        for (const auto member : households_[index].members) {
            if (member == actor) {
                household_index_value = index;
                break;
            }
        }
    }
    if (!household_index_value.has_value()) {
        return std::unexpected(HouseholdConsumeError::actor_without_household);
    }

    auto &household = households_[*household_index_value];
    if (!household.has_valid_resource_state()) {
        return std::unexpected(HouseholdConsumeError::invalid_household_state);
    }
    if (!actors_[*actor_index_value].spatial.has_value()) {
        return std::unexpected(HouseholdConsumeError::missing_spatial_state);
    }
    if (!is_actor_inside_place(actor, household.store_place)) {
        return std::unexpected(HouseholdConsumeError::outside_store);
    }
    if (household.remaining_consume_budget == 0) {
        return std::unexpected(HouseholdConsumeError::consume_budget_exhausted);
    }
    if (household.grain_stock_units < household.consume_amount_units) {
        return std::unexpected(HouseholdConsumeError::insufficient_stock);
    }

    household.grain_stock_units -= household.consume_amount_units;
    --household.remaining_consume_budget;
    ++revision_.value;

    return HouseholdConsumeResult{
        .actor = actor,
        .household = household.id,
        .consumed_grain_units = household.consume_amount_units,
        .remaining_grain_stock_units = household.grain_stock_units,
        .remaining_consume_budget = household.remaining_consume_budget,
        .shortage = household.grain_stock_units < household.shortage_threshold_units,
        .tick = tick_,
        .revision = revision_,
    };
}

} // namespace worldsim::sim
