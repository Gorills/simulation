#include "adapters/gdextension/sim_facade.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <stdexcept>

namespace worldsim::gdextension {
namespace {

constexpr double kMillimetersPerMeter = 1000.0;

[[nodiscard]] godot::String bootstrap_move_error_name(const protocol::BootstrapMoveError error) {
    switch (error) {
    case protocol::BootstrapMoveError::invalid_delta:
        return godot::String("invalid_delta");
    case protocol::BootstrapMoveError::controlled_actor_missing:
        return godot::String("controlled_actor_missing");
    case protocol::BootstrapMoveError::protocol_integer_exhausted:
        return godot::String("protocol_integer_exhausted");
    }
    return godot::String("unknown_bootstrap_move_error");
}

[[nodiscard]] godot::String movement_error_name(
    const protocol::ControlledActorMovementError error
) {
    switch (error) {
    case protocol::ControlledActorMovementError::invalid_intent:
        return godot::String("invalid_intent");
    case protocol::ControlledActorMovementError::protocol_integer_exhausted:
        return godot::String("protocol_integer_exhausted");
    case protocol::ControlledActorMovementError::controlled_actor_missing:
        return godot::String("controlled_actor_missing");
    case protocol::ControlledActorMovementError::controlled_actor_spatial_state_missing:
        return godot::String("controlled_actor_spatial_state_missing");
    case protocol::ControlledActorMovementError::world_rejected:
        return godot::String("world_rejected");
    }
    return godot::String("unknown_movement_error");
}

[[nodiscard]] godot::Dictionary protocol_integer_error_dictionary() {
    godot::Dictionary result;
    result["bridge_error"] = godot::String("protocol_integer_out_of_range");
    return result;
}

[[nodiscard]] godot::real_t meters_from_millimeters(const std::int64_t millimeters) {
    return static_cast<godot::real_t>(static_cast<double>(millimeters) / kMillimetersPerMeter);
}

[[nodiscard]] godot::Vector3 meters_vector(
    const std::int64_t x,
    const std::int64_t y,
    const std::int64_t z
) {
    return godot::Vector3{
        meters_from_millimeters(x),
        meters_from_millimeters(y),
        meters_from_millimeters(z),
    };
}

} // namespace

SimFacade::SimFacade() : simulation_(1) {}

void SimFacade::_bind_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("bootstrap_submit_move", "dx", "dy"),
        &SimFacade::bootstrap_submit_move
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("bootstrap_debug_projection"),
        &SimFacade::bootstrap_debug_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("observed_world_projection"),
        &SimFacade::observed_world_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_spatial_projection"),
        &SimFacade::controlled_actor_spatial_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_submit_move_intent", "x", "z"),
        &SimFacade::controlled_actor_submit_move_intent
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("advance_locomotion_tick"),
        &SimFacade::advance_locomotion_tick
    );
}

godot::Dictionary SimFacade::bootstrap_submit_move(const std::int32_t dx, const std::int32_t dy) {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.bootstrap_move({.dx = dx, .dy = dy});
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = bootstrap_move_error_name(outcome.error());
            response["projection"] = to_dictionary(simulation_.bootstrap_controlled_actor_projection());
            return response;
        }

        response["ok"] = true;
        response["projection"] = to_dictionary(outcome->actor);
        return response;
    } catch (const std::overflow_error &) {
        response["ok"] = false;
        response["error"] = godot::String("protocol_integer_out_of_range");
        return response;
    }
}

godot::Dictionary SimFacade::bootstrap_debug_projection() const {
    try {
        return to_dictionary(simulation_.bootstrap_controlled_actor_projection());
    } catch (const std::overflow_error &) {
        return protocol_integer_error_dictionary();
    }
}

godot::Dictionary SimFacade::observed_world_projection() const {
    try {
        return to_dictionary(simulation_.observed_world_projection());
    } catch (const std::overflow_error &) {
        return protocol_integer_error_dictionary();
    }
}

godot::Dictionary SimFacade::controlled_actor_spatial_projection() const {
    try {
        return to_dictionary(simulation_.controlled_actor_spatial_projection());
    } catch (const std::overflow_error &) {
        return protocol_integer_error_dictionary();
    }
}

godot::Dictionary SimFacade::controlled_actor_submit_move_intent(
    const std::int32_t x,
    const std::int32_t z
) {
    godot::Dictionary response;
    const auto outcome = simulation_.submit_controlled_actor_move_intent({.x = x, .z = z});
    if (!outcome.has_value()) {
        response["ok"] = false;
        response["error"] = movement_error_name(outcome.error());
        return response;
    }

    response["ok"] = true;
    return response;
}

godot::Dictionary SimFacade::advance_locomotion_tick() {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.advance_locomotion_tick();
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = movement_error_name(outcome.error());
            return response;
        }

        response["ok"] = true;
        response["batch"] = to_dictionary(*outcome);
        return response;
    } catch (const std::overflow_error &) {
        response["ok"] = false;
        response["error"] = godot::String("protocol_integer_out_of_range");
        return response;
    }
}

godot::Dictionary SimFacade::to_dictionary(const protocol::BootstrapActorProjection &projection) {
    godot::Dictionary result;
    result["entity_id"] = projection.entity_id;
    result["x"] = projection.x;
    result["y"] = projection.y;
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["seed"] = projection.seed;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

godot::Dictionary SimFacade::to_dictionary(const protocol::ObservedWorldProjection &projection) {
    godot::Array entities;
    for (const auto &entity : projection.entities) {
        godot::Dictionary item;
        item["entity_id"] = entity.entity_id;
        entities.push_back(item);
    }

    godot::Dictionary result;
    result["controlled_actor_id"] = projection.controlled_actor_id;
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    result["entities"] = entities;
    return result;
}

godot::Dictionary SimFacade::to_dictionary(
    const protocol::ControlledActorSpatialProjection &projection
) {
    godot::Dictionary result;
    result["entity_id"] = projection.entity_id;
    result["position_m"] = meters_vector(projection.x_mm, projection.y_mm, projection.z_mm);
    result["velocity_mps"] = meters_vector(
        projection.velocity_x_mm_per_second,
        projection.velocity_y_mm_per_second,
        projection.velocity_z_mm_per_second
    );
    result["spatial_epoch"] = projection.spatial_epoch;
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

godot::Dictionary SimFacade::to_dictionary(
    const protocol::AuthoritativeMovementSampleBatch &batch
) {
    godot::Array samples;
    for (const auto &sample : batch.samples) {
        godot::Dictionary item;
        item["entity_id"] = sample.entity_id;
        item["position_m"] = meters_vector(sample.x_mm, sample.y_mm, sample.z_mm);
        item["velocity_mps"] = meters_vector(
            sample.velocity_x_mm_per_second,
            sample.velocity_y_mm_per_second,
            sample.velocity_z_mm_per_second
        );
        item["spatial_epoch"] = sample.spatial_epoch;
        samples.push_back(item);
    }

    godot::Dictionary result;
    result["tick"] = batch.tick;
    result["revision"] = batch.revision;
    result["protocol_version"] = static_cast<std::int64_t>(batch.protocol_version);
    result["samples"] = samples;
    return result;
}

} // namespace worldsim::gdextension
