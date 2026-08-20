#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace {

[[nodiscard]] const worldsim::protocol::HouseholdResourceProjection *find_household(
    const worldsim::protocol::VillageHouseholdResourceProjection &projection,
    const worldsim::protocol::ProtocolInteger household_id
) {
    for (const auto &household : projection.households) {
        if (household.household_id == household_id) {
            return &household;
        }
    }
    return nullptr;
}

[[nodiscard]] worldsim::protocol::ProtocolInteger target_household_id(
    worldsim::protocol::Simulation &simulation
) {
    const auto carry = simulation.controlled_actor_carry_projection();
    if (!carry.member_household_id.has_value()) {
        return 0;
    }
    for (const auto &household : simulation.village_household_resource_projection().households) {
        if (household.household_id != *carry.member_household_id) {
            return household.household_id;
        }
    }
    return 0;
}

[[nodiscard]] worldsim::protocol::ControlledActorMoveIntent intent_toward(
    const worldsim::protocol::ControlledActorSpatialProjection &actor,
    const worldsim::protocol::HouseholdResourceProjection &household
) {
    const auto dx = household.store_x_mm - actor.x_mm;
    const auto dz = household.store_z_mm - actor.z_mm;
    const auto tolerance = household.store_axis_tolerance_mm;

    const auto x = std::abs(dx) <= tolerance ? 0 : (dx < 0 ? -1 : 1);
    const auto z = std::abs(dz) <= tolerance ? 0 : (dz < 0 ? -1 : 1);
    if (x != 0 && z != 0) {
        return worldsim::protocol::ControlledActorMoveIntent{
            .x = x * 707,
            .z = z * 707,
            .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
        };
    }
    return worldsim::protocol::ControlledActorMoveIntent{
        .x = x * worldsim::protocol::kPlanarMoveIntentScale,
        .z = z * worldsim::protocol::kPlanarMoveIntentScale,
        .pace = worldsim::protocol::ControlledActorLocomotionPace::walk,
    };
}

[[nodiscard]] bool at_store(
    const worldsim::protocol::ControlledActorSpatialProjection &actor,
    const worldsim::protocol::HouseholdResourceProjection &household
) {
    return std::abs(actor.x_mm - household.store_x_mm) <= household.store_axis_tolerance_mm
        && std::abs(actor.z_mm - household.store_z_mm) <= household.store_axis_tolerance_mm;
}

[[nodiscard]] bool wait_for_shortage(
    worldsim::protocol::Simulation &simulation,
    const worldsim::protocol::ProtocolInteger household_id
) {
    for (int opportunity = 0; opportunity < 480; ++opportunity) {
        if (!simulation.submit_controlled_actor_move_intent({}).has_value()) {
            return false;
        }
        if (!simulation.advance_locomotion_tick().has_value()) {
            return false;
        }
        const auto resources = simulation.village_household_resource_projection();
        const auto *target = find_household(resources, household_id);
        if (
            target != nullptr
            && target->status == worldsim::protocol::HouseholdResourceStatus::shortage
        ) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool reach_store(
    worldsim::protocol::Simulation &simulation,
    const worldsim::protocol::ProtocolInteger household_id
) {
    for (int opportunity = 0; opportunity < 480; ++opportunity) {
        const auto resources = simulation.village_household_resource_projection();
        const auto *target = find_household(resources, household_id);
        if (target == nullptr) {
            return false;
        }
        const auto spatial = simulation.controlled_actor_spatial_projection();
        if (at_store(spatial, *target)) {
            return true;
        }
        if (!simulation.submit_controlled_actor_move_intent(intent_toward(spatial, *target)).has_value()) {
            return false;
        }
        if (!simulation.advance_locomotion_tick().has_value()) {
            return false;
        }
    }
    return false;
}

} // namespace

TEST(M3ReciprocalAidProtocol, QualifyingGiftCreatesVisibleMemoryAndOneReciprocalTransfer) {
    worldsim::protocol::Simulation simulation{42};
    const auto target_id = target_household_id(simulation);
    ASSERT_GT(target_id, 0);

    const auto before_memory = simulation.reciprocal_aid_projection(target_id);
    ASSERT_TRUE(before_memory.has_value());
    EXPECT_FALSE(before_memory->remembered_for_controlled_actor);
    EXPECT_EQ(before_memory->protocol_version, 11U);

    const auto draw = simulation.controlled_actor_draw_household_grain();
    ASSERT_TRUE(draw.has_value());
    ASSERT_EQ(draw->moved_grain_units, 2);
    ASSERT_TRUE(wait_for_shortage(simulation, target_id));
    ASSERT_TRUE(reach_store(simulation, target_id));

    const auto resources_before_gift = simulation.village_household_resource_projection();
    const auto *target_before_gift = find_household(resources_before_gift, target_id);
    ASSERT_NE(target_before_gift, nullptr);
    ASSERT_EQ(target_before_gift->status, worldsim::protocol::HouseholdResourceStatus::shortage);

    const auto gift = simulation.controlled_actor_gift_household_grain(target_id);
    ASSERT_TRUE(gift.has_value());
    EXPECT_EQ(gift->moved_grain_units, 2);
    EXPECT_EQ(gift->carried_grain_units, 0);

    const auto remembered = simulation.reciprocal_aid_projection(target_id);
    ASSERT_TRUE(remembered.has_value());
    EXPECT_TRUE(remembered->remembered_for_controlled_actor);
    EXPECT_EQ(remembered->revision, gift->revision);
    EXPECT_EQ(remembered->protocol_version, worldsim::protocol::kProtocolVersion);

    const auto resources_before_aid = simulation.village_household_resource_projection();
    const auto *target_before_aid = find_household(resources_before_aid, target_id);
    ASSERT_NE(target_before_aid, nullptr);
    ASSERT_GT(target_before_aid->grain_stock_units, target_before_aid->shortage_threshold_units);

    const auto aid = simulation.controlled_actor_request_reciprocal_aid(target_id);
    ASSERT_TRUE(aid.has_value());
    EXPECT_EQ(aid->entity_id, 1);
    EXPECT_EQ(aid->household_id, target_id);
    EXPECT_EQ(aid->received_grain_units, 1);
    EXPECT_EQ(aid->carried_grain_units, 1);
    EXPECT_EQ(
        aid->remaining_household_grain_stock_units,
        target_before_aid->shortage_threshold_units
    );
    EXPECT_EQ(aid->tick, resources_before_aid.tick);
    EXPECT_EQ(aid->revision, resources_before_aid.revision + 1);
    EXPECT_EQ(aid->protocol_version, worldsim::protocol::kProtocolVersion);

    const auto after_memory = simulation.reciprocal_aid_projection(target_id);
    ASSERT_TRUE(after_memory.has_value());
    EXPECT_FALSE(after_memory->remembered_for_controlled_actor);
    EXPECT_EQ(after_memory->revision, aid->revision);

    const auto second = simulation.controlled_actor_request_reciprocal_aid(target_id);
    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(
        second.error(),
        worldsim::protocol::ControlledActorReciprocalAidError::no_remembered_aid
    );
    EXPECT_EQ(simulation.reciprocal_aid_projection(target_id), after_memory);
}

TEST(M3ReciprocalAidProtocol, StandingTransferControlIsMateriallyFeasibleButSociallyRefused) {
    worldsim::protocol::Simulation simulation{43};
    const auto target_id = target_household_id(simulation);
    ASSERT_GT(target_id, 0);
    ASSERT_TRUE(wait_for_shortage(simulation, target_id));

    const auto pledge = simulation.standing_transfer_pledge_projection();
    ASSERT_EQ(pledge.destination_household_id, target_id);
    ASSERT_GT(pledge.remaining_grain_units, 0);
    const auto transfer = simulation.controlled_actor_execute_household_transfer_pledge();
    ASSERT_TRUE(transfer.has_value());

    ASSERT_TRUE(reach_store(simulation, target_id));
    const auto resources = simulation.village_household_resource_projection();
    const auto *target = find_household(resources, target_id);
    ASSERT_NE(target, nullptr);
    const auto carry = simulation.controlled_actor_carry_projection();
    const auto spatial = simulation.controlled_actor_spatial_projection();
    const auto memory = simulation.reciprocal_aid_projection(target_id);
    ASSERT_TRUE(memory.has_value());
    ASSERT_FALSE(memory->remembered_for_controlled_actor);

    ASSERT_TRUE(at_store(spatial, *target));
    ASSERT_GT(target->grain_stock_units, target->shortage_threshold_units);
    ASSERT_LT(carry.carried_grain_units, carry.grain_carry_capacity_units);

    const auto before_resources = resources;
    const auto before_carry = carry;
    const auto before_memory = memory;
    const auto refused = simulation.controlled_actor_request_reciprocal_aid(target_id);
    ASSERT_FALSE(refused.has_value());
    EXPECT_EQ(
        refused.error(),
        worldsim::protocol::ControlledActorReciprocalAidError::no_remembered_aid
    );
    EXPECT_EQ(simulation.village_household_resource_projection(), before_resources);
    EXPECT_EQ(simulation.controlled_actor_carry_projection(), before_carry);
    EXPECT_EQ(simulation.reciprocal_aid_projection(target_id), before_memory);
}
