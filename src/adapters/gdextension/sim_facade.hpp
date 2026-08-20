#pragma once

#include "protocol/simulation.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>

namespace worldsim::gdextension {

class SimFacade final : public godot::RefCounted {
    GDCLASS(SimFacade, godot::RefCounted)

public:
    SimFacade();

    [[nodiscard]] godot::Dictionary bootstrap_submit_move(std::int32_t dx, std::int32_t dy);
    [[nodiscard]] godot::Dictionary bootstrap_debug_projection() const;
    [[nodiscard]] godot::Dictionary observed_world_projection() const;
    [[nodiscard]] godot::Dictionary controlled_actor_spatial_projection() const;
    [[nodiscard]] godot::Dictionary living_need_projection() const;
    [[nodiscard]] godot::Dictionary village_household_resource_projection() const;
    [[nodiscard]] godot::Dictionary controlled_actor_carry_projection() const;
    [[nodiscard]] godot::Dictionary field_work_projection() const;
    [[nodiscard]] godot::Dictionary standing_transfer_pledge_projection() const;
    [[nodiscard]] godot::Dictionary controlled_actor_draw_grain();
    [[nodiscard]] godot::Dictionary controlled_actor_deposit_grain();
    [[nodiscard]] godot::Dictionary controlled_actor_gift_grain(std::int64_t receiving_household_id);
    [[nodiscard]] godot::Dictionary controlled_actor_complete_field_work();
    [[nodiscard]] godot::Dictionary controlled_actor_execute_household_transfer_pledge();
    [[nodiscard]] godot::Dictionary controlled_actor_submit_move_intent(
        std::int32_t x,
        std::int32_t z,
        std::int32_t pace
    );
    [[nodiscard]] godot::Dictionary advance_locomotion_tick();

protected:
    static void _bind_methods();

private:
    static void bind_work_methods();
    static void bind_transfer_methods();

    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::BootstrapActorProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::ObservedWorldProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::ControlledActorSpatialProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::LivingNeedProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::VillageHouseholdResourceProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::ControlledActorCarryProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::FieldWorkProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::StandingTransferPledgeProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::ControlledActorResourceResult &result
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::ControlledActorWorkResult &result
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::ControlledActorTransferResult &result
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::AuthoritativeMovementSampleBatch &batch
    );

    protocol::Simulation simulation_;
};

} // namespace worldsim::gdextension
