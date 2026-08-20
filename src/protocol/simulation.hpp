#pragma once

#include "protocol/bootstrap_move.hpp"
#include "protocol/household_resource.hpp"
#include "protocol/integer.hpp"
#include "protocol/living_need.hpp"
#include "protocol/movement.hpp"
#include "protocol/observed_world.hpp"
#include "protocol/spatial.hpp"
#include "sim/world.hpp"

namespace worldsim::protocol {

class Simulation final {
public:
    explicit Simulation(ProtocolInteger seed = 1);

    [[nodiscard]] BootstrapMoveOutcome bootstrap_move(const BootstrapMoveIntent &intent);
    [[nodiscard]] ControlledActorMoveIntentOutcome submit_controlled_actor_move_intent(
        const ControlledActorMoveIntent &intent
    ) noexcept;
    [[nodiscard]] ControlledActorLocomotionTickOutcome advance_locomotion_tick();

    [[nodiscard]] ControlledActorResourceOutcome controlled_actor_draw_household_grain();
    [[nodiscard]] ControlledActorResourceOutcome controlled_actor_deposit_household_grain();
    [[nodiscard]] ControlledActorResourceOutcome controlled_actor_gift_household_grain(
        ProtocolInteger receiving_household_id
    );

    [[nodiscard]] BootstrapActorProjection bootstrap_controlled_actor_projection() const;
    [[nodiscard]] ObservedWorldProjection observed_world_projection() const;
    [[nodiscard]] ControlledActorSpatialProjection controlled_actor_spatial_projection() const;
    [[nodiscard]] LivingNeedProjection living_need_projection() const;
    [[nodiscard]] VillageHouseholdResourceProjection village_household_resource_projection() const;
    [[nodiscard]] ControlledActorCarryProjection controlled_actor_carry_projection() const;

private:
    sim::World world_;
    sim::GroundedLocomotionContext locomotion_context_;

    // Session/control binding only. Runtime entity identity and composition are
    // Simulation Core content-owned; clients must not choose authoritative IDs.
    sim::EntityId controlled_actor_{};
    // Session/control state, not world truth. Submission validates and replaces
    // the desired planar intent; the authoritative World consumes it only when
    // the fixed locomotion tick advances.
    ControlledActorMoveIntent controlled_move_intent_{};
};

} // namespace worldsim::protocol
