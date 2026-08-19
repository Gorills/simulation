#pragma once

#include "protocol/bootstrap_move.hpp"
#include "protocol/integer.hpp"
#include "protocol/observed_world.hpp"
#include "protocol/spatial.hpp"
#include "sim/world.hpp"

namespace worldsim::protocol {

class Simulation final {
public:
    explicit Simulation(ProtocolInteger seed = 1);

    [[nodiscard]] BootstrapMoveOutcome bootstrap_move(const BootstrapMoveIntent &intent);
    [[nodiscard]] BootstrapActorProjection bootstrap_controlled_actor_projection() const;
    [[nodiscard]] ObservedWorldProjection observed_world_projection() const;
    [[nodiscard]] ControlledActorSpatialProjection controlled_actor_spatial_projection() const;

private:
    sim::World world_;

    // Milestone 0 scenario/session binding only. Runtime entity identity is
    // Simulation/content-owned; clients must not choose authoritative IDs.
    sim::EntityId controlled_actor_{1};
};

} // namespace worldsim::protocol
