#include "sim/living_need.hpp"

#include <cstdint>

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

// Occupancy envelope (arrival + body radius) plus this NPC's body radius. The
// NPC may approach an occupied rest place, but must not walk its body onto the
// stand-here footprint.
[[nodiscard]] constexpr std::uint64_t occupied_footprint_keep_out(
    const Millimeters axis_arrival_tolerance
) noexcept {
    return static_cast<std::uint64_t>(axis_arrival_tolerance.value)
        + 2U * static_cast<std::uint64_t>(kFirstPlayableBody.radius.value);
}

[[nodiscard]] bool body_would_enter_occupied_footprint(
    const SpatialState &spatial,
    const RestNeedState &need
) noexcept {
    const auto keep_out = occupied_footprint_keep_out(need.axis_arrival_tolerance);
    return unsigned_distance(spatial.position.x.value, need.rest_x.value) <= keep_out
        && unsigned_distance(spatial.position.z.value, need.rest_z.value) <= keep_out;
}

} // namespace

std::expected<NpcRestNeedDecision, NpcRestNeedDecisionError>
decide_npc_rest_need(const World &world, const EntityId actor) noexcept {
    if (!actor.is_valid()) {
        return std::unexpected(NpcRestNeedDecisionError::invalid_entity_id);
    }
    if (!world.contains_actor(actor)) {
        return std::unexpected(NpcRestNeedDecisionError::unknown_actor);
    }

    const auto need = world.actor_rest_need(actor);
    if (!need.has_value()) {
        return std::unexpected(NpcRestNeedDecisionError::rest_need_missing);
    }
    if (!need->is_valid()) {
        return std::unexpected(NpcRestNeedDecisionError::invalid_rest_need_state);
    }

    // Rest travel is ordinary walking intent. The decision does not author a
    // numeric speed; World resolves the actor's current walk capability.
    const auto movement = decide_npc_local_move_toward_waypoint(
        world,
        NpcLocalWaypoint{
            .actor = actor,
            .x = need->rest_x,
            .z = need->rest_z,
            .axis_arrival_tolerance = need->axis_arrival_tolerance,
            .pace = LocomotionPace::walk,
        }
    );
    if (!movement.has_value()) {
        switch (movement.error()) {
        case NpcLocomotionDecisionError::invalid_waypoint:
            return std::unexpected(NpcRestNeedDecisionError::invalid_rest_need_state);
        case NpcLocomotionDecisionError::unknown_actor:
            return std::unexpected(NpcRestNeedDecisionError::unknown_actor);
        case NpcLocomotionDecisionError::missing_spatial_state:
            return std::unexpected(NpcRestNeedDecisionError::missing_spatial_state);
        }
        return std::unexpected(NpcRestNeedDecisionError::invalid_rest_need_state);
    }

    const auto spatial = world.actor_spatial_state(actor);
    if (!spatial.has_value()) {
        return std::unexpected(NpcRestNeedDecisionError::missing_spatial_state);
    }

    const bool occupied = world.is_planar_position_occupied_by_other_actor(
        actor,
        need->rest_x,
        need->rest_z,
        need->axis_arrival_tolerance
    );
    if (occupied && body_would_enter_occupied_footprint(*spatial, *need)) {
        auto halted = *movement;
        halted.move = {};
        return NpcRestNeedDecision{
            .satisfied = false,
            .blocked_by_other_actor = true,
            .movement = halted,
        };
    }

    const bool at_assigned_rest_point = movement->move == PlanarMoveIntent{};
    return NpcRestNeedDecision{
        .satisfied = at_assigned_rest_point,
        .blocked_by_other_actor = false,
        .movement = *movement,
    };
}

} // namespace worldsim::sim
