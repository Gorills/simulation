#include "protocol/simulation.hpp"

#include <cassert>
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

[[nodiscard]] ControlledActorWorkError map_work_error(const sim::FieldWorkError error) noexcept {
    switch (error) {
    case sim::FieldWorkError::invalid_entity_id:
    case sim::FieldWorkError::unknown_actor:
        return ControlledActorWorkError::controlled_actor_missing;
    case sim::FieldWorkError::field_work_assignment_missing:
        return ControlledActorWorkError::field_work_unavailable;
    case sim::FieldWorkError::invalid_field_work_assignment_state:
    case sim::FieldWorkError::unknown_work_place:
    case sim::FieldWorkError::unknown_work_destination_household:
    case sim::FieldWorkError::invalid_household_state:
        return ControlledActorWorkError::invalid_field_work_state;
    case sim::FieldWorkError::missing_spatial_state:
        return ControlledActorWorkError::controlled_actor_spatial_state_missing;
    case sim::FieldWorkError::outside_field:
        return ControlledActorWorkError::outside_field;
    case sim::FieldWorkError::work_exhausted:
        return ControlledActorWorkError::work_exhausted;
    case sim::FieldWorkError::stock_overflow:
        return ControlledActorWorkError::stock_overflow;
    }
    return ControlledActorWorkError::invalid_field_work_state;
}

[[nodiscard]] ControlledActorWorkResult work_result(const sim::FieldWorkResult &result) {
    return ControlledActorWorkResult{
        .entity_id = result.actor.value,
        .work_place_id = result.work_place.value,
        .destination_household_id = result.destination_household.value,
        .produced_grain_units = result.produced_grain_units,
        .destination_household_grain_stock_units = result.destination_grain_stock_units,
        .remaining_work_completions = checked_protocol_integer(
            result.remaining_work_completions
        ),
        .tick = checked_protocol_integer(result.tick.value),
        .revision = checked_protocol_integer(result.revision.value),
        .protocol_version = kProtocolVersion,
    };
}

} // namespace

ControlledActorWorkOutcome Simulation::controlled_actor_complete_field_work() {
    if (world_.revision().value >= kMaxProtocolInteger) {
        return std::unexpected(ControlledActorWorkError::protocol_integer_exhausted);
    }

    const auto outcome = world_.complete_field_work(controlled_actor_);
    if (!outcome.has_value()) {
        return std::unexpected(map_work_error(outcome.error()));
    }
    return work_result(*outcome);
}

FieldWorkProjection Simulation::field_work_projection() const {
    const auto assignment = world_.field_work_assignment();
    assert(assignment.has_value());
    if (!assignment.has_value()) {
        return {};
    }

    const auto place = world_.place_state(assignment->work_place);
    assert(place.has_value());
    if (!place.has_value()) {
        return {};
    }

    return FieldWorkProjection{
        .work_place_id = place->id.value,
        .work_x_mm = place->x.value,
        .work_z_mm = place->z.value,
        .work_axis_tolerance_mm = place->axis_occupancy_tolerance.value,
        .destination_household_id = assignment->destination_household.value,
        .yield_grain_units = assignment->yield_grain_units,
        .remaining_work_completions = checked_protocol_integer(
            assignment->remaining_work_completions
        ),
        .tick = checked_protocol_integer(world_.tick().value),
        .revision = checked_protocol_integer(world_.revision().value),
        .protocol_version = kProtocolVersion,
    };
}

} // namespace worldsim::protocol
