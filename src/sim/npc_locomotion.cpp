#include "sim/npc_locomotion.hpp"

#include <cstdint>

namespace worldsim::sim {
namespace {

// Equal-axis eight-way steering must remain inside PlanarMoveIntent's unit
// circle. 707 is the largest integer component that satisfies that bound;
// 708/708 would already be invalid.
inline constexpr std::int32_t kDiagonalIntentComponent = 707;
static_assert(
    2LL * kDiagonalIntentComponent * kDiagonalIntentComponent <=
    static_cast<std::int64_t>(kIntentScale) * kIntentScale
);
static_assert(
    2LL * (kDiagonalIntentComponent + 1) * (kDiagonalIntentComponent + 1) >
    static_cast<std::int64_t>(kIntentScale) * kIntentScale
);

[[nodiscard]] constexpr std::uint64_t unsigned_distance(
    const std::int64_t first,
    const std::int64_t second
) noexcept {
    if (first >= second) {
        return static_cast<std::uint64_t>(first) - static_cast<std::uint64_t>(second);
    }
    return static_cast<std::uint64_t>(second) - static_cast<std::uint64_t>(first);
}

[[nodiscard]] constexpr std::int32_t axis_direction(
    const std::int64_t current,
    const std::int64_t target,
    const std::uint64_t tolerance
) noexcept {
    if (unsigned_distance(current, target) <= tolerance) {
        return 0;
    }
    return target > current ? 1 : -1;
}

} // namespace

std::expected<ActorGroundedMoveIntent, NpcLocomotionDecisionError>
decide_npc_local_move_toward_waypoint(
    const World &world,
    const NpcLocalWaypoint &waypoint
) noexcept {
    if (!waypoint.is_valid()) {
        return std::unexpected(NpcLocomotionDecisionError::invalid_waypoint);
    }

    const auto spatial = world.actor_spatial_state(waypoint.actor);
    if (!spatial.has_value()) {
        if (!world.contains_actor(waypoint.actor)) {
            return std::unexpected(NpcLocomotionDecisionError::unknown_actor);
        }
        return std::unexpected(NpcLocomotionDecisionError::missing_spatial_state);
    }

    const auto tolerance = static_cast<std::uint64_t>(waypoint.axis_arrival_tolerance.value);
    const auto x_direction = axis_direction(
        spatial->position.x.value,
        waypoint.x.value,
        tolerance
    );
    const auto z_direction = axis_direction(
        spatial->position.z.value,
        waypoint.z.value,
        tolerance
    );

    PlanarMoveIntent move{};
    if (x_direction != 0 && z_direction != 0) {
        move.x = x_direction * kDiagonalIntentComponent;
        move.z = z_direction * kDiagonalIntentComponent;
    } else {
        move.x = x_direction * kIntentScale;
        move.z = z_direction * kIntentScale;
    }

    return ActorGroundedMoveIntent{
        .actor = waypoint.actor,
        .move = move,
    };
}

} // namespace worldsim::sim
