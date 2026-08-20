#pragma once

#include "protocol/integer.hpp"
#include "protocol/version.hpp"

#include <cstdint>

namespace worldsim::protocol {

// Purpose-built read-only presentation state for the first Milestone 1 need.
// It exposes the current derived outcome plus the observable rest-place footprint
// required for the player to understand and physically interfere with the same
// authoritative world rule. It is not a generic need/task framework and grants
// no mutation authority.
enum class LivingNeedStatus : std::uint8_t {
    traveling,
    blocked,
    satisfied,
};

struct LivingNeedProjection final {
    ProtocolInteger entity_id{};
    LivingNeedStatus status{LivingNeedStatus::traveling};
    ProtocolInteger target_x_mm{};
    ProtocolInteger target_z_mm{};
    ProtocolInteger axis_arrival_tolerance_mm{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    constexpr bool operator==(const LivingNeedProjection &) const = default;
};

} // namespace worldsim::protocol
