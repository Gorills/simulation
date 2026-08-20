#include "adapters/gdextension/sim_facade.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

#include <stdexcept>

namespace worldsim::gdextension {
namespace {

[[nodiscard]] godot::String reciprocal_aid_error_name(
    const protocol::ControlledActorReciprocalAidError error
) {
    switch (error) {
    case protocol::ControlledActorReciprocalAidError::protocol_integer_exhausted:
        return godot::String("protocol_integer_exhausted");
    case protocol::ControlledActorReciprocalAidError::controlled_actor_missing:
        return godot::String("controlled_actor_missing");
    case protocol::ControlledActorReciprocalAidError::target_household_missing:
        return godot::String("target_household_missing");
    case protocol::ControlledActorReciprocalAidError::invalid_actor_carry_state:
        return godot::String("invalid_actor_carry_state");
    case protocol::ControlledActorReciprocalAidError::invalid_household_state:
        return godot::String("invalid_household_state");
    case protocol::ControlledActorReciprocalAidError::controlled_actor_spatial_state_missing:
        return godot::String("controlled_actor_spatial_state_missing");
    case protocol::ControlledActorReciprocalAidError::outside_store:
        return godot::String("outside_store");
    case protocol::ControlledActorReciprocalAidError::no_remembered_aid:
        return godot::String("no_remembered_aid");
    case protocol::ControlledActorReciprocalAidError::remembered_for_other_actor:
        return godot::String("remembered_for_other_actor");
    case protocol::ControlledActorReciprocalAidError::carry_full:
        return godot::String("carry_full");
    case protocol::ControlledActorReciprocalAidError::insufficient_surplus:
        return godot::String("insufficient_surplus");
    }
    return godot::String("unknown_reciprocal_aid_error");
}

[[nodiscard]] godot::Dictionary protocol_integer_error_dictionary() {
    godot::Dictionary result;
    result["bridge_error"] = godot::String("protocol_integer_out_of_range");
    return result;
}

} // namespace

void SimFacade::bind_reciprocal_aid_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("reciprocal_aid_projection", "household_id"),
        &SimFacade::reciprocal_aid_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_request_reciprocal_aid", "household_id"),
        &SimFacade::controlled_actor_request_reciprocal_aid
    );
}

godot::Dictionary SimFacade::reciprocal_aid_projection(const std::int64_t household_id) const {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.reciprocal_aid_projection(household_id);
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = reciprocal_aid_error_name(outcome.error());
            return response;
        }
        response["ok"] = true;
        response["projection"] = to_dictionary(*outcome);
        return response;
    } catch (const std::overflow_error &) {
        response["ok"] = false;
        response["error"] = godot::String("protocol_integer_out_of_range");
        return response;
    }
}

godot::Dictionary SimFacade::controlled_actor_request_reciprocal_aid(
    const std::int64_t household_id
) {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.controlled_actor_request_reciprocal_aid(household_id);
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = reciprocal_aid_error_name(outcome.error());
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

godot::Dictionary SimFacade::to_dictionary(
    const protocol::ControlledActorReciprocalAidProjection &projection
) {
    godot::Dictionary result;
    result["household_id"] = projection.household_id;
    result["remembered_for_controlled_actor"] = projection.remembered_for_controlled_actor;
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

godot::Dictionary SimFacade::to_dictionary(
    const protocol::ControlledActorReciprocalAidResult &aid
) {
    godot::Dictionary result;
    result["entity_id"] = aid.entity_id;
    result["household_id"] = aid.household_id;
    result["received_grain_units"] = aid.received_grain_units;
    result["carried_grain_units"] = aid.carried_grain_units;
    result["remaining_household_grain_stock_units"] = aid.remaining_household_grain_stock_units;
    result["tick"] = aid.tick;
    result["revision"] = aid.revision;
    result["protocol_version"] = static_cast<std::int64_t>(aid.protocol_version);
    return result;
}

} // namespace worldsim::gdextension
