#pragma once

#include "protocol/integer.hpp"
#include "protocol/version.hpp"

#include <cstdint>

namespace worldsim::protocol {

// Purpose-built read-only presentation state for the first Milestone 1 need.
// It reports the current derived outcome only; it is not a generic need/task
// framework and does not expose RestNeedState internals or mutation authority.
enum class LivingNeedStatus : std::uint8_t {
    traveling,
    blocked,
    satisfied,
};

struct LivingNeedProjection final {
    ProtocolInteger entity_id{};
    LivingNeedStatus status{LivingNeedStatus::traveling};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    constexpr bool operator==(const LivingNeedProjection &) const = default;
};

} // namespace worldsim::protocol
