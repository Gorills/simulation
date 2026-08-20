#include "adapters/gdextension/sim_facade.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <optional>
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
    case protocol::ControlledActorMovementError::invalid_pace:
        return godot::String("invalid_pace");
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

[[nodiscard]] godot::String resource_error_name(
    const protocol::ControlledActorResourceError error
) {
    switch (error) {
    case protocol::ControlledActorResourceError::protocol_integer_exhausted:
        return godot::String("protocol_integer_exhausted");
    case protocol::ControlledActorResourceError::controlled_actor_missing:
        return godot::String("controlled_actor_missing");
    case protocol::ControlledActorResourceError::actor_without_household:
        return godot::String("actor_without_household");
    case protocol::ControlledActorResourceError::invalid_actor_carry_state:
        return godot::String("invalid_actor_carry_state");
    case protocol::ControlledActorResourceError::invalid_household_state:
        return godot::String("invalid_household_state");
    case protocol::ControlledActorResourceError::controlled_actor_spatial_state_missing:
        return godot::String("controlled_actor_spatial_state_missing");
    case protocol::ControlledActorResourceError::outside_store:
        return godot::String("outside_store");
    case protocol::ControlledActorResourceError::carry_full:
        return godot::String("carry_full");
    case protocol::ControlledActorResourceError::store_empty:
        return godot::String("store_empty");
    case protocol::ControlledActorResourceError::carry_empty:
        return godot::String("carry_empty");
    case protocol::ControlledActorResourceError::target_household_missing:
        return godot::String("target_household_missing");
    case protocol::ControlledActorResourceError::own_household:
        return godot::String("own_household");
    case protocol::ControlledActorResourceError::stock_overflow:
        return godot::String("stock_overflow");
    }
    return godot::String("unknown_resource_error");
}

[[nodiscard]] godot::String living_need_status_name(const protocol::LivingNeedStatus status) {
    switch (status) {
    case protocol::LivingNeedStatus::traveling:
        return godot::String("traveling");
    case protocol::LivingNeedStatus::blocked:
        return godot::String("blocked");
    case protocol::LivingNeedStatus::satisfied:
        return godot::String("satisfied");
    }
    return godot::String("unknown");
}

[[nodiscard]] godot::String household_resource_status_name(
    const protocol::HouseholdResourceStatus status
) {
    switch (status) {
    case protocol::HouseholdResourceStatus::adequate:
        return godot::String("adequate");
    case protocol::HouseholdResourceStatus::shortage:
        return godot::String("shortage");
    }
    return godot::String("unknown");
}

[[nodiscard]] std::optional<protocol::ControlledActorLocomotionPace> movement_pace(
    const std::int32_t value
) noexcept {
    switch (value) {
    case 0:
        return protocol::ControlledActorLocomotionPace::walk;
    case 1:
        return protocol::ControlledActorLocomotionPace::run;
    case 2:
        return protocol::ControlledActorLocomotionPace::sprint;
    default:
        return std::nullopt;
    }
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
        godot::D_METHOD("living_need_projection"),
        &SimFacade::living_need_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("village_household_resource_projection"),
        &SimFacade::village_household_resource_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_carry_projection"),
        &SimFacade::controlled_actor_carry_projection
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_draw_grain"),
        &SimFacade::controlled_actor_draw_grain
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_deposit_grain"),
        &SimFacade::controlled_actor_deposit_grain
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_gift_grain", "receiving_household_id"),
        &SimFacade::controlled_actor_gift_grain
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("controlled_actor_submit_move_intent", "x", "z", "pace"),
        &SimFacade::controlled_actor_submit_move_intent
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("advance_locomotion_tick"),
        &SimFacade::advance_locomotion_tick
    );
    bind_work_methods();
    bind_transfer_methods();
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

godot::Dictionary SimFacade::living_need_projection() const {
    try {
        return to_dictionary(simulation_.living_need_projection());
    } catch (const std::overflow_error &) {
        return protocol_integer_error_dictionary();
    }
}

godot::Dictionary SimFacade::village_household_resource_projection() const {
    try {
        return to_dictionary(simulation_.village_household_resource_projection());
    } catch (const std::overflow_error &) {
        return protocol_integer_error_dictionary();
    }
}

godot::Dictionary SimFacade::controlled_actor_carry_projection() const {
    try {
        return to_dictionary(simulation_.controlled_actor_carry_projection());
    } catch (const std::overflow_error &) {
        return protocol_integer_error_dictionary();
    }
}

godot::Dictionary SimFacade::controlled_actor_draw_grain() {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.controlled_actor_draw_household_grain();
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = resource_error_name(outcome.error());
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

godot::Dictionary SimFacade::controlled_actor_deposit_grain() {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.controlled_actor_deposit_household_grain();
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = resource_error_name(outcome.error());
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

godot::Dictionary SimFacade::controlled_actor_gift_grain(
    const std::int64_t receiving_household_id
) {
    godot::Dictionary response;
    try {
        const auto outcome = simulation_.controlled_actor_gift_household_grain(
            receiving_household_id
        );
        if (!outcome.has_value()) {
            response["ok"] = false;
            response["error"] = resource_error_name(outcome.error());
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

godot::Dictionary SimFacade::controlled_actor_submit_move_intent(
    const std::int32_t x,
    const std::int32_t z,
    const std::int32_t pace
) {
    godot::Dictionary response;
    const auto semantic_pace = movement_pace(pace);
    if (!semantic_pace.has_value()) {
        response["ok"] = false;
        response["error"] = godot::String("invalid_pace");
        return response;
    }

    const auto outcome = simulation_.submit_controlled_actor_move_intent({
        .x = x,
        .z = z,
        .pace = *semantic_pace,
    });
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

godot::Dictionary SimFacade::to_dictionary(const protocol::LivingNeedProjection &projection) {
    godot::Dictionary result;
    result["entity_id"] = projection.entity_id;
    result["status"] = living_need_status_name(projection.status);
    result["target_position_m"] = meters_vector(projection.target_x_mm, 0, projection.target_z_mm);
    result["axis_arrival_tolerance_m"] = meters_from_millimeters(projection.axis_arrival_tolerance_mm);
    result["axis_occupancy_tolerance_m"] = meters_from_millimeters(projection.axis_occupancy_tolerance_mm);
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

godot::Dictionary SimFacade::to_dictionary(
    const protocol::VillageHouseholdResourceProjection &projection
) {
    godot::Array households;
    for (const auto &household : projection.households) {
        godot::Array member_actor_ids;
        for (const auto member : household.member_actor_ids) {
            member_actor_ids.push_back(member);
        }

        godot::Dictionary item;
        item["household_id"] = household.household_id;
        item["member_actor_ids"] = member_actor_ids;
        item["store_place_id"] = household.store_place_id;
        item["store_position_m"] = meters_vector(household.store_x_mm, 0, household.store_z_mm);
        item["store_axis_tolerance_m"] = meters_from_millimeters(
            household.store_axis_tolerance_mm
        );
        item["grain_stock_units"] = household.grain_stock_units;
        item["shortage_threshold_units"] = household.shortage_threshold_units;
        item["status"] = household_resource_status_name(household.status);
        households.push_back(item);
    }

    godot::Dictionary result;
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    result["households"] = households;
    return result;
}

godot::Dictionary SimFacade::to_dictionary(
    const protocol::ControlledActorCarryProjection &projection
) {
    godot::Dictionary result;
    result["entity_id"] = projection.entity_id;
    result["carried_grain_units"] = projection.carried_grain_units;
    result["grain_carry_capacity_units"] = projection.grain_carry_capacity_units;
    if (projection.member_household_id.has_value()) {
        result["member_household_id"] = *projection.member_household_id;
    }
    if (projection.member_household_grain_stock_units.has_value()) {
        result["member_household_grain_stock_units"] =
            *projection.member_household_grain_stock_units;
    }
    result["tick"] = projection.tick;
    result["revision"] = projection.revision;
    result["protocol_version"] = static_cast<std::int64_t>(projection.protocol_version);
    return result;
}

godot::Dictionary SimFacade::to_dictionary(
    const protocol::ControlledActorResourceResult &resource
) {
    godot::Dictionary result;
    result["entity_id"] = resource.entity_id;
    result["affected_household_id"] = resource.affected_household_id;
    result["moved_grain_units"] = resource.moved_grain_units;
    result["carried_grain_units"] = resource.carried_grain_units;
    result["affected_household_grain_stock_units"] =
        resource.affected_household_grain_stock_units;
    result["tick"] = resource.tick;
    result["revision"] = resource.revision;
    result["protocol_version"] = static_cast<std::int64_t>(resource.protocol_version);
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
