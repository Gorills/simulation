#include "adapters/gdextension/sim_facade.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <stdexcept>

namespace worldsim::gdextension {
namespace {

constexpr double kMillimetersPerMeter = 1000.0;

[[nodiscard]] godot::real_t meters_from_millimeters(const std::int64_t millimeters) {
    return static_cast<godot::real_t>(static_cast<double>(millimeters) / kMillimetersPerMeter);
}

[[nodiscard]] godot::String work_error_name(const protocol::ControlledActorWorkError error) {
    switch (error) {
    case protocol::ControlledActorWorkError::protocol_integer_exhausted:
        return godot::String("protocol_integer_exhausted");
    case protocol::ControlledActorWorkError::controlled_actor_missing:
        return godot::String("controlled_actor_missing");
    case protocol::ControlledActorWorkError::field_work_unavailable:
        return godot::String("field_work_unavailable");
    case protocol::ControlledActorWorkError::invalid_field_work_state:
        return godot::String("invalid_field_work_state");
    case protocol::ControlledActorWorkError::controlled_actor_spatial_state_missing:
        return godot::String("controlled_actor_spatial_state_missing");
    case protocol::ControlledActorWorkError::outside_field:
        return godot::String("outside_field");
    case protocol::ControlledActorWorkError::work_exhausted:
        return godot::String("work_exhausted");
    case protocol::ControlledActorWorkError::stock_overflow:
        return godot::String("stock_overflow");
    }
    return godot::String("unknown_work_error");
}

[[nodiscard]] godot::Dictionary protocol_integer_error_dictionary() {
    godot::Dictionary result;
    result["bridge_error"] = godot::String("protocol_integer_out_of_range");
    return result;
}

} // namespace

void SimFacade::bind_work_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("field_work_projection"),
        &SimFacade::field_work_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_complete_field_work"),
        &SimFacade::controlled_actor_complete_field_work
    );
}

godot::Dictionary SimFacade::field_work_projection() const {
    try {
        return to_dictionary(simulation_.field_work_projection());
    } catch (const std::overflow_error &) {
        return protocol_integer_error_dictionary();
    }
}

godot::Dictionary SimFacade::controlled_actor_complete_field_work() {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.controlled_actor_complete_field_work();
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = work_error_name(outcome.error());
            return response;
        }
        response["ok"] = true;
        response["result"] = to_dictionary(*outcome);
        return response;
    } catch (const std::overflow_error &) {
        response["ok"] = false;
        response["error"] = godot::String("protocol_integer_out_of_range");
        return response;
    }
}

godot::Dictionary SimFacade::to_dictionary(const protocol::FieldWorkProjection &projection) {
    godot::Dictionary result;
    result["work_place_id"] = projection.work_place_id;
    result["work_position_m"] = godot::Vector3{
        meters_from_millimeters(projection.work_x_mm),
        0,
        meters_from_millimeters(projection.work_z_mm),
    };
    result["work_axis_tolerance_m"] = meters_from_millimeters(
        projection.work_axis_tolerance_mm
    );
    result["destination_household_id"] = projection.destination_household_id;
    result["yield_grain_units"] = projection.yield_grain_units;
    result["remaining_work_completions"] = projection.remaining_work_completions;
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

godot::Dictionary SimFacade::to_dictionary(const protocol::ControlledActorWorkResult &work) {
    godot::Dictionary result;
    result["entity_id"] = work.entity_id;
    result["work_place_id"] = work.work_place_id;
    result["destination_household_id"] = work.destination_household_id;
    result["produced_grain_units"] = work.produced_grain_units;
    result["destination_household_grain_stock_units"] =
        work.destination_household_grain_stock_units;
    result["remaining_work_completions"] = work.remaining_work_completions;
    result["tick"] = work.tick;
    result["revision"] = work.revision;
    result["protocol_version"] = static_cast<std::int64_t>(work.protocol_version);
    return result;
}

} // namespace worldsim::gdextension
