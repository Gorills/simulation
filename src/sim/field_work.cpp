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

std::expected<void, WorldError> World::add_field_work_assignment(
    const FieldWorkAssignmentState assignment
) {
    if (!assignment.is_valid()) {
        return std::unexpected(WorldError::invalid_field_work_assignment_state);
    }
    if (field_work_assignment_.has_value()) {
        return std::unexpected(WorldError::field_work_assignment_already_exists);
    }
    if (!contains_place(assignment.work_place)) {
        return std::unexpected(WorldError::unknown_work_place);
    }
    if (!contains_household(assignment.destination_household)) {
        return std::unexpected(WorldError::unknown_work_destination_household);
    }

    field_work_assignment_ = assignment;
    ++revision_.value;
    return {};
}

std::optional<FieldWorkAssignmentState> World::field_work_assignment() const noexcept {
    return field_work_assignment_;
}

std::expected<FieldWorkResult, FieldWorkError>
World::complete_field_work(const EntityId actor) noexcept {
    if (!actor.is_valid()) {
        return std::unexpected(FieldWorkError::invalid_entity_id);
    }

    const auto actor_index_value = actor_index(actor);
    if (!actor_index_value.has_value()) {
        return std::unexpected(FieldWorkError::unknown_actor);
    }
    if (!field_work_assignment_.has_value()) {
        return std::unexpected(FieldWorkError::field_work_assignment_missing);
    }

    auto &assignment = *field_work_assignment_;
    if (!assignment.is_valid()) {
        return std::unexpected(FieldWorkError::invalid_field_work_assignment_state);
    }
    if (!contains_place(assignment.work_place)) {
        return std::unexpected(FieldWorkError::unknown_work_place);
    }

    const auto destination_index = household_index(assignment.destination_household);
    if (!destination_index.has_value()) {
        return std::unexpected(FieldWorkError::unknown_work_destination_household);
    }
    auto &destination = households_[*destination_index];
    if (!destination.has_valid_resource_state()) {
        return std::unexpected(FieldWorkError::invalid_household_state);
    }

    if (!actors_[*actor_index_value].spatial.has_value()) {
        return std::unexpected(FieldWorkError::missing_spatial_state);
    }
    if (!is_actor_inside_place(actor, assignment.work_place)) {
        return std::unexpected(FieldWorkError::outside_field);
    }
    if (assignment.remaining_work_completions == 0) {
        return std::unexpected(FieldWorkError::work_exhausted);
    }

    std::int64_t updated_stock{};
    if (!checked_add_nonnegative(
            destination.grain_stock_units,
            assignment.yield_grain_units,
            updated_stock
        )) {
        return std::unexpected(FieldWorkError::stock_overflow);
    }

    destination.grain_stock_units = updated_stock;
    --assignment.remaining_work_completions;
    ++revision_.value;

    return FieldWorkResult{
        .actor = actor,
        .work_place = assignment.work_place,
        .destination_household = destination.id,
        .produced_grain_units = assignment.yield_grain_units,
        .destination_grain_stock_units = destination.grain_stock_units,
        .remaining_work_completions = assignment.remaining_work_completions,
        .tick = tick_,
        .revision = revision_,
    };
}

} // namespace worldsim::sim
