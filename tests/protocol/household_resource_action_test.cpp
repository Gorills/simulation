#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

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

TEST(ResourceProtocol, ControlledCarryProjectionSharesAuthoritativeStartupRevision) {
    worldsim::protocol::Simulation simulation{42};

    const auto carry = simulation.controlled_actor_carry_projection();
    const auto resources = simulation.village_household_resource_projection();

    EXPECT_EQ(carry.entity_id, 1);
    EXPECT_EQ(carry.carried_grain_units, 0);
    EXPECT_EQ(carry.grain_carry_capacity_units, 2);
    ASSERT_TRUE(carry.member_household_id.has_value());
    ASSERT_TRUE(carry.member_household_grain_stock_units.has_value());
    EXPECT_EQ(*carry.member_household_id, 20);
    EXPECT_EQ(*carry.member_household_grain_stock_units, 8);
    EXPECT_EQ(carry.tick, resources.tick);
    EXPECT_EQ(carry.revision, resources.revision);
    EXPECT_EQ(carry.protocol_version, worldsim::protocol::kProtocolVersion);
    EXPECT_EQ(carry.protocol_version, 8U);
}

TEST(ResourceProtocol, DrawAndDepositAreSemanticRevisionOnlyCommands) {
    worldsim::protocol::Simulation simulation{42};
    const auto before = simulation.controlled_actor_carry_projection();

    const auto drawn = simulation.controlled_actor_draw_household_grain();
    ASSERT_TRUE(drawn.has_value());
    EXPECT_EQ(drawn->entity_id, before.entity_id);
    EXPECT_EQ(drawn->affected_household_id, *before.member_household_id);
    EXPECT_EQ(drawn->moved_grain_units, 2);
    EXPECT_EQ(drawn->carried_grain_units, 2);
    EXPECT_EQ(drawn->affected_household_grain_stock_units, 6);
    EXPECT_EQ(drawn->tick, before.tick);
    EXPECT_EQ(drawn->revision, before.revision + 1);
    EXPECT_EQ(drawn->protocol_version, worldsim::protocol::kProtocolVersion);

    const auto after_draw = simulation.controlled_actor_carry_projection();
    EXPECT_EQ(after_draw.carried_grain_units, 2);
    EXPECT_EQ(*after_draw.member_household_grain_stock_units, 6);
    EXPECT_EQ(after_draw.tick, drawn->tick);
    EXPECT_EQ(after_draw.revision, drawn->revision);

    const auto deposited = simulation.controlled_actor_deposit_household_grain();
    ASSERT_TRUE(deposited.has_value());
    EXPECT_EQ(deposited->moved_grain_units, 2);
    EXPECT_EQ(deposited->carried_grain_units, 0);
    EXPECT_EQ(deposited->affected_household_grain_stock_units, 8);
    EXPECT_EQ(deposited->tick, drawn->tick);
    EXPECT_EQ(deposited->revision, drawn->revision + 1);
}

TEST(ResourceProtocol, GiftRefusesOwnHouseholdWithoutMutation) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.controlled_actor_draw_household_grain().has_value());
    const auto before = simulation.controlled_actor_carry_projection();
    ASSERT_TRUE(before.member_household_id.has_value());

    const auto gifted = simulation.controlled_actor_gift_household_grain(
        *before.member_household_id
    );

    ASSERT_FALSE(gifted.has_value());
    EXPECT_EQ(
        gifted.error(),
        worldsim::protocol::ControlledActorResourceError::own_household
    );
    EXPECT_EQ(simulation.controlled_actor_carry_projection(), before);
}

TEST(ResourceProtocol, GiftChangesAuthoritativeStockAndPreservesM1RestInterference) {
    worldsim::protocol::Simulation simulation{42};
    const auto initial_resources = simulation.village_household_resource_projection();
    const auto initial_carry = simulation.controlled_actor_carry_projection();
    ASSERT_TRUE(initial_carry.member_household_id.has_value());

    const worldsim::protocol::HouseholdResourceProjection *target = nullptr;
    for (const auto &household : initial_resources.households) {
        if (household.household_id != *initial_carry.member_household_id) {
            target = &household;
            break;
        }
    }
    ASSERT_NE(target, nullptr);
    const auto target_id = target->household_id;

    const auto drawn = simulation.controlled_actor_draw_household_grain();
    ASSERT_TRUE(drawn.has_value());
    ASSERT_GT(drawn->moved_grain_units, 0);
    EXPECT_EQ(drawn->tick, initial_carry.tick);

    // Match the real-client Gift vertical: the neighbour first becomes short
    // through ordinary post-movement NPC Consume while the controlled actor
    // holds position at its own store. This ensures the RestNeed NPC has reached
    // the shared target/store before the controlled actor approaches to Gift.
    bool shortage_seen = false;
    for (int opportunity = 0; opportunity < 480; ++opportunity) {
        ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
        const auto movement = simulation.advance_locomotion_tick();
        ASSERT_TRUE(movement.has_value());

        const auto resources = simulation.village_household_resource_projection();
        const auto *current_target = find_household(resources, target_id);
        ASSERT_NE(current_target, nullptr);
        if (current_target->status == worldsim::protocol::HouseholdResourceStatus::shortage) {
            shortage_seen = true;
            break;
        }
    }
    ASSERT_TRUE(shortage_seen);

    bool gifted = false;
    worldsim::protocol::ControlledActorResourceResult gift_result{};
    worldsim::protocol::VillageHouseholdResourceProjection before_gift{};

    for (int opportunity = 0; opportunity < 480; ++opportunity) {
        const auto spatial = simulation.controlled_actor_spatial_projection();
        const auto resources = simulation.village_household_resource_projection();
        const auto *current_target = find_household(resources, target_id);
        ASSERT_NE(current_target, nullptr);

        ASSERT_TRUE(simulation.submit_controlled_actor_move_intent(
            intent_toward(spatial, *current_target)
        ).has_value());
        const auto movement = simulation.advance_locomotion_tick();
        ASSERT_TRUE(movement.has_value());

        before_gift = simulation.village_household_resource_projection();
        const auto attempt = simulation.controlled_actor_gift_household_grain(target_id);
        if (!attempt.has_value()) {
            ASSERT_EQ(
                attempt.error(),
                worldsim::protocol::ControlledActorResourceError::outside_store
            );
            continue;
        }

        gifted = true;
        gift_result = *attempt;
        break;
    }

    ASSERT_TRUE(gifted);
    const auto *target_before = find_household(before_gift, target_id);
    ASSERT_NE(target_before, nullptr);
    EXPECT_EQ(gift_result.entity_id, initial_carry.entity_id);
    EXPECT_EQ(gift_result.affected_household_id, target_id);
    EXPECT_EQ(gift_result.moved_grain_units, drawn->moved_grain_units);
    EXPECT_EQ(gift_result.carried_grain_units, 0);
    EXPECT_EQ(
        gift_result.affected_household_grain_stock_units,
        target_before->grain_stock_units + gift_result.moved_grain_units
    );
    EXPECT_EQ(gift_result.tick, before_gift.tick);
    EXPECT_EQ(gift_result.revision, before_gift.revision + 1);

    const auto after_carry = simulation.controlled_actor_carry_projection();
    EXPECT_EQ(after_carry.carried_grain_units, 0);
    EXPECT_EQ(after_carry.tick, gift_result.tick);
    EXPECT_EQ(after_carry.revision, gift_result.revision);

    const auto living = simulation.living_need_projection();
    EXPECT_EQ(living.status, worldsim::protocol::LivingNeedStatus::blocked);
    EXPECT_EQ(living.tick, gift_result.tick);
    EXPECT_EQ(living.revision, gift_result.revision);

    // A resource revision between locomotion batches must not poison the next
    // movement batch's temporal guard.
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
    const auto next_movement = simulation.advance_locomotion_tick();
    ASSERT_TRUE(next_movement.has_value());
    EXPECT_EQ(next_movement->tick, gift_result.tick + 1);
    EXPECT_GT(next_movement->revision, gift_result.revision);
}

} // namespace
