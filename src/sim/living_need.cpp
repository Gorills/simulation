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

    const auto movement = decide_npc_local_move_toward_waypoint(
        world,
        NpcLocalWaypoint{
            .actor = actor,
            .x = need->rest_x,
            .z = need->rest_z,
            .axis_arrival_tolerance = need->axis_arrival_tolerance,
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

    return NpcRestNeedDecision{
        .satisfied = movement->move == PlanarMoveIntent{},
        .movement = *movement,
    };
}

} // namespace worldsim::sim
