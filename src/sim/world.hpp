#pragma once

#include "protocol/move.hpp"

#include <cstdint>

namespace worldsim::sim {

class World final {
public:
    explicit World(std::uint64_t seed = 1) noexcept;

    [[nodiscard]] protocol::MoveOutcome move(const protocol::MoveIntent &intent) noexcept;
    [[nodiscard]] protocol::PlayerProjection player_projection() const noexcept;

private:
    std::int32_t player_x_{};
    std::int32_t player_y_{};
    std::uint64_t tick_{};
    std::uint64_t seed_{};
};

} // namespace worldsim::sim
