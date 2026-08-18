#pragma once

#include <cstdint>

namespace simulation::protocol {

inline constexpr std::uint32_t kProtocolVersion = 1;

enum class MoveDirection : std::uint8_t {
    north = 0,
    south = 1,
    west = 2,
    east = 3,
};

struct MoveIntent {
    MoveDirection direction;
};

struct PlayerProjection {
    std::int32_t x;
    std::int32_t y;
    std::uint64_t tick;
};

struct CommandResult {
    bool accepted;
    PlayerProjection player;
};

}  // namespace simulation::protocol
