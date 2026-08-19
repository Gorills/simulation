#include "adapters/gdextension/sim_facade.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace worldsim::gdextension {
namespace {

[[nodiscard]] godot::String bootstrap_move_error_name(const protocol::BootstrapMoveError error) {
    switch (error) {
    case protocol::BootstrapMoveError::invalid_delta:
        return godot::String("invalid_delta");
    case protocol::BootstrapMoveError::controlled_actor_missing:
        return godot::String("controlled_actor_missing");
    }
    return godot::String("unknown_bootstrap_move_error");
}

} // namespace

SimFacade::SimFacade() : simulation_(1) {}

void SimFacade::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("submit_move", "dx", "dy"), &SimFacade::submit_move);
    godot::ClassDB::bind_method(godot::D_METHOD("debug_projection"), &SimFacade::debug_projection);
}

godot::Dictionary SimFacade::submit_move(const std::int32_t dx, const std::int32_t dy) {
    const auto outcome = simulation_.bootstrap_move({.dx = dx, .dy = dy});

    godot::Dictionary response;
    if (!outcome.has_value()) {
        response["ok"] = false;
        response["error"] = bootstrap_move_error_name(outcome.error());
        response["projection"] = to_dictionary(simulation_.bootstrap_controlled_actor_projection());
        return response;
    }

    response["ok"] = true;
    response["projection"] = to_dictionary(outcome->actor);
    return response;
}

godot::Dictionary SimFacade::debug_projection() const {
    return to_dictionary(simulation_.bootstrap_controlled_actor_projection());
}

godot::Dictionary SimFacade::to_dictionary(const protocol::BootstrapActorProjection &projection) {
    godot::Dictionary result;
    result["entity_id"] = static_cast<std::int64_t>(projection.entity_id);
    result["x"] = projection.x;
    result["y"] = projection.y;
    result["tick"] = static_cast<std::int64_t>(projection.tick);
    result["revision"] = static_cast<std::int64_t>(projection.revision);
    result["seed"] = static_cast<std::int64_t>(projection.seed);
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

} // namespace worldsim::gdextension
