#include "sim/living_need.hpp"

namespace worldsim::sim {

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

    const bool at_assigned_rest_point = movement->move == PlanarMoveIntent{};
    const bool blocked_by_other_actor = at_assigned_rest_point
        && world.is_planar_position_occupied_by_other_actor(
            actor,
            need->rest_x,
            need->rest_z,
            need->axis_arrival_tolerance
        );

    return NpcRestNeedDecision{
        .satisfied = at_assigned_rest_point && !blocked_by_other_actor,
        .blocked_by_other_actor = blocked_by_other_actor,
        .movement = *movement,
    };
}

} // namespace worldsim::sim
