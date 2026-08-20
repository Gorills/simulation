#include "adapters/gdextension/sim_facade.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

#include <stdexcept>

namespace worldsim::gdextension {
namespace {

[[nodiscard]] godot::String transfer_error_name(
    const protocol::ControlledActorTransferError error
) {
    switch (error) {
    case protocol::ControlledActorTransferError::protocol_integer_exhausted:
        return godot::String("protocol_integer_exhausted");
    case protocol::ControlledActorTransferError::controlled_actor_missing:
        return godot::String("controlled_actor_missing");
    case protocol::ControlledActorTransferError::actor_without_household:
        return godot::String("actor_without_household");
    case protocol::ControlledActorTransferError::invalid_household_state:
        return godot::String("invalid_household_state");
    case protocol::ControlledActorTransferError::invalid_pledge_state:
        return godot::String("invalid_pledge_state");
    case protocol::ControlledActorTransferError::destination_household_missing:
        return godot::String("destination_household_missing");
    case protocol::ControlledActorTransferError::self_destination:
        return godot::String("self_destination");
    case protocol::ControlledActorTransferError::controlled_actor_spatial_state_missing:
        return godot::String("controlled_actor_spatial_state_missing");
    case protocol::ControlledActorTransferError::outside_store:
        return godot::String("outside_store");
    case protocol::ControlledActorTransferError::pledge_zero:
        return godot::String("pledge_zero");
    case protocol::ControlledActorTransferError::insufficient_stock:
        return godot::String("insufficient_stock");
    case protocol::ControlledActorTransferError::stock_overflow:
        return godot::String("stock_overflow");
    }
    return godot::String("unknown_transfer_error");
}

[[nodiscard]] godot::Dictionary protocol_integer_error_dictionary() {
    godot::Dictionary result;
    result["bridge_error"] = godot::String("protocol_integer_out_of_range");
    return result;
}

} // namespace

void SimFacade::bind_transfer_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("standing_transfer_pledge_projection"),
        &SimFacade::standing_transfer_pledge_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_execute_household_transfer_pledge"),
        &SimFacade::controlled_actor_execute_household_transfer_pledge
    );
    bind_reciprocal_aid_methods();
}

godot::Dictionary SimFacade::standing_transfer_pledge_projection() const {
    try {
        return to_dictionary(simulation_.standing_transfer_pledge_projection());
    } catch (const std::overflow_error &) {
        return protocol_integer_error_dictionary();
    }
}

godot::Dictionary SimFacade::controlled_actor_execute_household_transfer_pledge() {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.controlled_actor_execute_household_transfer_pledge();
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = transfer_error_name(outcome.error());
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
    const protocol::StandingTransferPledgeProjection &projection
) {
    godot::Dictionary result;
    result["source_household_id"] = projection.source_household_id;
    result["destination_household_id"] = projection.destination_household_id;
    result["remaining_grain_units"] = projection.remaining_grain_units;
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

godot::Dictionary SimFacade::to_dictionary(
    const protocol::ControlledActorTransferResult &transfer
) {
    godot::Dictionary result;
    result["entity_id"] = transfer.entity_id;
    result["source_household_id"] = transfer.source_household_id;
    result["destination_household_id"] = transfer.destination_household_id;
    result["transferred_grain_units"] = transfer.transferred_grain_units;
    result["source_household_grain_stock_units"] = transfer.source_household_grain_stock_units;
    result["destination_household_grain_stock_units"] =
        transfer.destination_household_grain_stock_units;
    result["remaining_pledge_grain_units"] = transfer.remaining_pledge_grain_units;
    result["tick"] = transfer.tick;
    result["revision"] = transfer.revision;
    result["protocol_version"] = static_cast<std::int64_t>(transfer.protocol_version);
    return result;
}

} // namespace worldsim::gdextension
