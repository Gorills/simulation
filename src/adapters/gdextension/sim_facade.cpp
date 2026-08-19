#include "adapters/gdextension/sim_facade.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace worldsim::gdextension {

SimFacade::SimFacade() : world_(1) {}

void SimFacade::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("submit_move", "dx", "dy"), &SimFacade::submit_move);
    godot::ClassDB::bind_method(godot::D_METHOD("debug_projection"), &SimFacade::debug_projection);
}

godot::Dictionary SimFacade::submit_move(const std::int32_t dx, const std::int32_t dy) {
    const auto outcome = world_.move({.dx = dx, .dy = dy});

    godot::Dictionary response;
    if (!outcome.has_value()) {
        response["ok"] = false;
        response["error"] = godot::String("invalid_delta");
        response["projection"] = to_dictionary(world_.player_projection());
        return response;
    }

    response["ok"] = true;
    response["projection"] = to_dictionary(outcome->player);
    return response;
}

godot::Dictionary SimFacade::debug_projection() const {
    return to_dictionary(world_.player_projection());
}

godot::Dictionary SimFacade::to_dictionary(const protocol::PlayerProjection &projection) {
    godot::Dictionary result;
    result["x"] = projection.x;
    result["y"] = projection.y;
    result["tick"] = static_cast<std::int64_t>(projection.tick);
    result["seed"] = static_cast<std::int64_t>(projection.seed);
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

} // namespace worldsim::gdextension
