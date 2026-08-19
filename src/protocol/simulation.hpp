#pragma once

#include "protocol/bootstrap_move.hpp"
#include "protocol/observed_world.hpp"
#include "sim/world.hpp"

#include <cstdint>

namespace worldsim::protocol {

class Simulation final {
public:
    explicit Simulation(std::uint64_t seed = 1);

    [[nodiscard]] BootstrapMoveOutcome bootstrap_move(const BootstrapMoveIntent &intent) noexcept;
    [[nodiscard]] BootstrapActorProjection bootstrap_controlled_actor_projection() const noexcept;
    [[nodiscard]] ObservedWorldProjection observed_world_projection() const;

private:
    sim::World world_;

    // Milestone 0 scenario/session binding only. Runtime entity identity is
    // Simulation/content-owned; clients must not choose authoritative IDs.
    sim::EntityId controlled_actor_{1};
};

} // namespace worldsim::protocol
