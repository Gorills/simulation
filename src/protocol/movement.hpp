#pragma once

#include "protocol/spatial.hpp"

#include <cstdint>
#include <expected>

namespace worldsim::protocol {

inline constexpr std::int32_t kPlanarMoveIntentScale = 1000;

struct ControlledActorMoveIntent final {
    std::int32_t x{};
    std::int32_t z{};

    constexpr bool operator==(const ControlledActorMoveIntent &) const = default;
};

enum class ControlledActorMovementError : std::uint8_t {
    invalid_intent,
    protocol_integer_exhausted,
    controlled_actor_missing,
    controlled_actor_spatial_state_missing,
    world_rejected,
};

using ControlledActorMoveIntentOutcome =
    std::expected<void, ControlledActorMovementError>;
using ControlledActorLocomotionTickOutcome =
    std::expected<ControlledActorSpatialProjection, ControlledActorMovementError>;

} // namespace worldsim::protocol
