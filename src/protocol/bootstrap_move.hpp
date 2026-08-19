#pragma once

#include "protocol/integer.hpp"
#include "protocol/version.hpp"

#include <cstdint>
#include <expected>

namespace worldsim::protocol {

// Milestone 0 transport probe only. Production third-person locomotion must use
// a real authoritative actor-location/movement contract.
struct BootstrapMoveIntent final {
    std::int32_t dx{};
    std::int32_t dy{};

    constexpr bool operator==(const BootstrapMoveIntent &) const = default;
};

enum class BootstrapMoveError : std::uint8_t {
    invalid_delta,
    controlled_actor_missing,
    protocol_integer_exhausted,
};

// Milestone 0 projection only. Grid coordinates are deliberately not exposed as
// the generic production actor-projection contract.
struct BootstrapActorProjection final {
    std::int64_t entity_id{};
    std::int32_t x{};
    std::int32_t y{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    ProtocolInteger seed{};
    std::uint32_t protocol_version{kProtocolVersion};

    constexpr bool operator==(const BootstrapActorProjection &) const = default;
};

struct BootstrapMoveResult final {
    BootstrapActorProjection actor{};

    constexpr bool operator==(const BootstrapMoveResult &) const = default;
};

using BootstrapMoveOutcome = std::expected<BootstrapMoveResult, BootstrapMoveError>;

} // namespace worldsim::protocol
