#pragma once

#include "protocol/protocol.hpp"

#include <cstdint>

namespace simulation {

class Simulation final {
public:
    explicit Simulation(std::uint32_t seed) noexcept;

    [[nodiscard]] protocol::CommandResult execute(protocol::MoveIntent intent) noexcept;
    [[nodiscard]] protocol::PlayerProjection player_projection() const noexcept;
    [[nodiscard]] std::uint32_t seed() const noexcept;

private:
    struct WorldState {
        std::uint32_t seed;
        std::uint64_t tick{0};
        std::int32_t player_x{0};
        std::int32_t player_y{0};
    };

    WorldState state_;
};

}  // namespace simulation
