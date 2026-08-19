#pragma once

#include "sim/types.hpp"

namespace worldsim::sim {

class World final {
public:
    explicit World(WorldSeed seed = WorldSeed{1}) noexcept;

    void move(CardinalDirection direction) noexcept;

    [[nodiscard]] GridPosition player_position() const noexcept;
    [[nodiscard]] SimulationTick tick() const noexcept;
    [[nodiscard]] WorldSeed seed() const noexcept;

private:
    GridPosition player_position_{};
    SimulationTick tick_{};
    WorldSeed seed_{};
};

} // namespace worldsim::sim
