#include "sim/world.hpp"

#include <cstdint>
#include <limits>

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

bool World::can_consume_household_grain(const EntityId actor) const noexcept {
    if (!actor.is_valid()) {
        return false;
    }

    const auto actor_index_value = actor_index(actor);
    if (!actor_index_value.has_value() || !actors_[*actor_index_value].spatial.has_value()) {
        return false;
    }

    const auto household_index_value = actor_household_index(actor);
    if (!household_index_value.has_value()) {
        return false;
    }

    const auto &household = households_[*household_index_value];
    return household.has_valid_resource_state()
        && household.remaining_consume_budget > 0
        && household.grain_stock_units >= household.consume_amount_units
        && is_actor_inside_place(actor, household.store_place);
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

    const auto household_index_value = actor_household_index(actor);
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

std::expected<HouseholdDrawResult, HouseholdDrawError>
World::draw_household_grain(const EntityId actor) noexcept {
    if (!actor.is_valid()) {
        return std::unexpected(HouseholdDrawError::invalid_entity_id);
    }

    const auto actor_index_value = actor_index(actor);
    if (!actor_index_value.has_value()) {
        return std::unexpected(HouseholdDrawError::unknown_actor);
    }
    const auto household_index_value = actor_household_index(actor);
    if (!household_index_value.has_value()) {
        return std::unexpected(HouseholdDrawError::actor_without_household);
    }

    auto &actor_state = actors_[*actor_index_value];
    auto &household = households_[*household_index_value];
    if (!actor_state.grain_carry.is_valid()) {
        return std::unexpected(HouseholdDrawError::invalid_actor_grain_carry_state);
    }
    if (!household.has_valid_resource_state()) {
        return std::unexpected(HouseholdDrawError::invalid_household_state);
    }
    if (!actor_state.spatial.has_value()) {
        return std::unexpected(HouseholdDrawError::missing_spatial_state);
    }
    if (!is_actor_inside_place(actor, household.store_place)) {
        return std::unexpected(HouseholdDrawError::outside_store);
    }
    if (
        actor_state.grain_carry.carried_grain_units
        == actor_state.grain_carry.grain_carry_capacity_units
    ) {
        return std::unexpected(HouseholdDrawError::carry_full);
    }
    if (household.grain_stock_units == 0) {
        return std::unexpected(HouseholdDrawError::store_empty);
    }

    const auto free_capacity =
        actor_state.grain_carry.grain_carry_capacity_units
        - actor_state.grain_carry.carried_grain_units;
    const auto moved = household.grain_stock_units < free_capacity
        ? household.grain_stock_units
        : free_capacity;

    actor_state.grain_carry.carried_grain_units += moved;
    household.grain_stock_units -= moved;
    ++revision_.value;

    return HouseholdDrawResult{
        .actor = actor,
        .household = household.id,
        .moved_grain_units = moved,
        .carried_grain_units = actor_state.grain_carry.carried_grain_units,
        .remaining_grain_stock_units = household.grain_stock_units,
        .tick = tick_,
        .revision = revision_,
    };
}

std::expected<HouseholdDepositResult, HouseholdDepositError>
World::deposit_household_grain(const EntityId actor) noexcept {
    if (!actor.is_valid()) {
        return std::unexpected(HouseholdDepositError::invalid_entity_id);
    }

    const auto actor_index_value = actor_index(actor);
    if (!actor_index_value.has_value()) {
        return std::unexpected(HouseholdDepositError::unknown_actor);
    }
    const auto household_index_value = actor_household_index(actor);
    if (!household_index_value.has_value()) {
        return std::unexpected(HouseholdDepositError::actor_without_household);
    }

    auto &actor_state = actors_[*actor_index_value];
    auto &household = households_[*household_index_value];
    if (!actor_state.grain_carry.is_valid()) {
        return std::unexpected(HouseholdDepositError::invalid_actor_grain_carry_state);
    }
    if (!household.has_valid_resource_state()) {
        return std::unexpected(HouseholdDepositError::invalid_household_state);
    }
    if (!actor_state.spatial.has_value()) {
        return std::unexpected(HouseholdDepositError::missing_spatial_state);
    }
    if (!is_actor_inside_place(actor, household.store_place)) {
        return std::unexpected(HouseholdDepositError::outside_store);
    }
    if (actor_state.grain_carry.carried_grain_units == 0) {
        return std::unexpected(HouseholdDepositError::carry_empty);
    }

    std::int64_t updated_stock{};
    if (!checked_add_nonnegative(
            household.grain_stock_units,
            actor_state.grain_carry.carried_grain_units,
            updated_stock
        )) {
        return std::unexpected(HouseholdDepositError::stock_overflow);
    }

    const auto deposited = actor_state.grain_carry.carried_grain_units;
    household.grain_stock_units = updated_stock;
    actor_state.grain_carry.carried_grain_units = 0;
    ++revision_.value;

    return HouseholdDepositResult{
        .actor = actor,
        .household = household.id,
        .deposited_grain_units = deposited,
        .carried_grain_units = actor_state.grain_carry.carried_grain_units,
        .remaining_grain_stock_units = household.grain_stock_units,
        .tick = tick_,
        .revision = revision_,
    };
}

std::expected<HouseholdGiftResult, HouseholdGiftError>
World::gift_household_grain(
    const EntityId actor,
    const EntityId receiving_household
) noexcept {
    if (!actor.is_valid() || !receiving_household.is_valid()) {
        return std::unexpected(HouseholdGiftError::invalid_entity_id);
    }

    const auto actor_index_value = actor_index(actor);
    if (!actor_index_value.has_value()) {
        return std::unexpected(HouseholdGiftError::unknown_actor);
    }
    const auto receiving_index = household_index(receiving_household);
    if (!receiving_index.has_value()) {
        return std::unexpected(HouseholdGiftError::unknown_household);
    }

    auto &actor_state = actors_[*actor_index_value];
    auto &household = households_[*receiving_index];
    if (!actor_state.grain_carry.is_valid()) {
        return std::unexpected(HouseholdGiftError::invalid_actor_grain_carry_state);
    }
    if (!household.has_valid_resource_state() || !household.has_valid_social_state()) {
        return std::unexpected(HouseholdGiftError::invalid_household_state);
    }
    if (actor_state.grain_carry.carried_grain_units == 0) {
        return std::unexpected(HouseholdGiftError::carry_empty);
    }

    const auto own_household = actor_household_index(actor);
    if (own_household.has_value() && *own_household == *receiving_index) {
        return std::unexpected(HouseholdGiftError::own_household);
    }
    if (!actor_state.spatial.has_value()) {
        return std::unexpected(HouseholdGiftError::missing_spatial_state);
    }
    if (!is_actor_inside_place(actor, household.store_place)) {
        return std::unexpected(HouseholdGiftError::outside_store);
    }

    std::int64_t updated_stock{};
    if (!checked_add_nonnegative(
            household.grain_stock_units,
            actor_state.grain_carry.carried_grain_units,
            updated_stock
        )) {
        return std::unexpected(HouseholdGiftError::stock_overflow);
    }

    const auto gifted = actor_state.grain_carry.carried_grain_units;
    const bool remember_personal_aid =
        household.grain_stock_units < household.shortage_threshold_units
        && !household.remembered_material_aid_actor.is_valid();

    household.grain_stock_units = updated_stock;
    actor_state.grain_carry.carried_grain_units = 0;
    if (remember_personal_aid) {
        household.remembered_material_aid_actor = actor;
    }
    ++revision_.value;

    return HouseholdGiftResult{
        .actor = actor,
        .receiving_household = household.id,
        .gifted_grain_units = gifted,
        .carried_grain_units = actor_state.grain_carry.carried_grain_units,
        .receiving_grain_stock_units = household.grain_stock_units,
        .tick = tick_,
        .revision = revision_,
        .remembered_aid_created = remember_personal_aid,
    };
}

std::expected<HouseholdReciprocalAidResult, HouseholdReciprocalAidError>
World::request_household_reciprocal_aid(
    const EntityId actor,
    const EntityId household_id
) noexcept {
    if (!actor.is_valid() || !household_id.is_valid()) {
        return std::unexpected(HouseholdReciprocalAidError::invalid_entity_id);
    }

    const auto actor_index_value = actor_index(actor);
    if (!actor_index_value.has_value()) {
        return std::unexpected(HouseholdReciprocalAidError::unknown_actor);
    }
    const auto household_index_value = household_index(household_id);
    if (!household_index_value.has_value()) {
        return std::unexpected(HouseholdReciprocalAidError::unknown_household);
    }

    auto &actor_state = actors_[*actor_index_value];
    auto &household = households_[*household_index_value];
    if (!actor_state.grain_carry.is_valid()) {
        return std::unexpected(HouseholdReciprocalAidError::invalid_actor_grain_carry_state);
    }
    if (!household.has_valid_resource_state() || !household.has_valid_social_state()) {
        return std::unexpected(HouseholdReciprocalAidError::invalid_household_state);
    }
    if (!actor_state.spatial.has_value()) {
        return std::unexpected(HouseholdReciprocalAidError::missing_spatial_state);
    }
    if (!is_actor_inside_place(actor, household.store_place)) {
        return std::unexpected(HouseholdReciprocalAidError::outside_store);
    }
    if (!household.remembered_material_aid_actor.is_valid()) {
        return std::unexpected(HouseholdReciprocalAidError::no_remembered_aid);
    }
    if (household.remembered_material_aid_actor != actor) {
        return std::unexpected(HouseholdReciprocalAidError::remembered_for_other_actor);
    }
    if (
        actor_state.grain_carry.carried_grain_units
        == actor_state.grain_carry.grain_carry_capacity_units
    ) {
        return std::unexpected(HouseholdReciprocalAidError::carry_full);
    }
    if (household.grain_stock_units <= household.shortage_threshold_units) {
        return std::unexpected(HouseholdReciprocalAidError::insufficient_surplus);
    }

    const auto free_capacity =
        actor_state.grain_carry.grain_carry_capacity_units
        - actor_state.grain_carry.carried_grain_units;
    const auto surplus = household.grain_stock_units - household.shortage_threshold_units;
    const auto moved = surplus < free_capacity ? surplus : free_capacity;

    actor_state.grain_carry.carried_grain_units += moved;
    household.grain_stock_units -= moved;
    household.remembered_material_aid_actor = EntityId{};
    ++revision_.value;

    return HouseholdReciprocalAidResult{
        .actor = actor,
        .household = household.id,
        .received_grain_units = moved,
        .carried_grain_units = actor_state.grain_carry.carried_grain_units,
        .remaining_grain_stock_units = household.grain_stock_units,
        .tick = tick_,
        .revision = revision_,
    };
}

} // namespace worldsim::sim
