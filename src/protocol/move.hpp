#pragma once

#include <cstdint>
#include <expected>

namespace worldsim::protocol {

inline constexpr std::uint32_t kProtocolVersion = 1;

struct MoveIntent {
    std::int32_t dx{};
    std::int32_t dy{};

    constexpr bool operator==(const MoveIntent &) const = default;
};

enum class MoveError : std::uint8_t {
    invalid_delta,
};

struct PlayerProjection {
    std::int32_t x{};
    std::int32_t y{};
    std::uint64_t tick{};
    std::uint64_t seed{};
    std::uint32_t protocol_version{kProtocolVersion};

    constexpr bool operator==(const PlayerProjection &) const = default;
};

struct MoveResult {
    PlayerProjection player{};

    constexpr bool operator==(const MoveResult &) const = default;
};

using MoveOutcome = std::expected<MoveResult, MoveError>;

} // namespace worldsim::protocol
