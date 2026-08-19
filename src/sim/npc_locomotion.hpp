#pragma once

#include "sim/world.hpp"

#include <cstdint>
#include <expected>

namespace worldsim::sim {

enum class NpcLocomotionDecisionError : std::uint8_t {
    invalid_waypoint,
    unknown_actor,
    missing_spatial_state,
};

// A local steering input, not a high-level NPC need/task or a navigation route.
// Another causal system chooses why this actor should reach the waypoint. This
// primitive only turns that already-selected local planar waypoint into
// PlanarMoveIntent for the same actor-generic World locomotion transition used
// by human control.
struct NpcLocalWaypoint final {
    EntityId actor{};
    Millimeters x{};
    Millimeters z{};
    // Per-axis dead band around the waypoint. It is supplied by the caller so
    // this locomotion primitive does not invent a universal arrival distance.
    Millimeters axis_arrival_tolerance{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return actor.is_valid() && axis_arrival_tolerance.value >= 0;
    }

    constexpr bool operator==(const NpcLocalWaypoint &) const = default;
};

// Produces deterministic coarse local steering from authoritative actor state.
// It deliberately does not perform pathfinding, obstacle queries, scheduling,
// need selection, or world mutation. Collision and resulting SpatialState remain
// owned by World::advance_grounded_locomotion_tick().
[[nodiscard]] std::expected<ActorGroundedMoveIntent, NpcLocomotionDecisionError>
decide_npc_local_move_toward_waypoint(
    const World &world,
    const NpcLocalWaypoint &waypoint
) noexcept;

} // namespace worldsim::sim
