#pragma once

#include "sim/npc_locomotion.hpp"

#include <cstdint>
#include <expected>

namespace worldsim::sim {

enum class NpcRestNeedDecisionError : std::uint8_t {
    invalid_entity_id,
    unknown_actor,
    rest_need_missing,
    invalid_rest_need_state,
    missing_spatial_state,
};

// One bounded causal decision for Milestone 1: an NPC with RestNeedState either
// approaches its assigned rest point, waits outside an occupied rest footprint
// because another exact-spatial actor currently occupies it, or satisfies the
// need there. This is not a generic task planner, reservation system, schedule,
// behavior tree, actor-body collision or navigation route.
struct NpcRestNeedDecision final {
    bool satisfied{};
    bool blocked_by_other_actor{};
    ActorGroundedMoveIntent movement{};

    constexpr bool operator==(const NpcRestNeedDecision &) const = default;
};

[[nodiscard]] std::expected<NpcRestNeedDecision, NpcRestNeedDecisionError>
decide_npc_rest_need(const World &world, EntityId actor) noexcept;

} // namespace worldsim::sim
