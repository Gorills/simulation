#pragma once

#include "protocol/move.hpp"
#include "sim/world.hpp"

#include <cstdint>

namespace worldsim::protocol {

class Simulation final {
public:
    explicit Simulation(std::uint64_t seed = 1) noexcept;

    [[nodiscard]] MoveOutcome move(const MoveIntent &intent) noexcept;
    [[nodiscard]] PlayerProjection player_projection() const noexcept;

private:
    sim::World world_;
};

} // namespace worldsim::protocol
