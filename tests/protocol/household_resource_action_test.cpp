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

[[nodiscard]] worldsim::protocol::ControlledActorMoveIntent intent_toward(
    const worldsim::protocol::ControlledActorSpatialProjection &actor,
    const worldsim::protocol::FieldWorkProjection &field
) {
    const auto dx = field.work_x_mm - actor.x_mm;
    const auto dz = field.work_z_mm - actor.z_mm;
    const auto tolerance = field.work_axis_tolerance_mm;

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

TEST(ResourceProtocol, WorkUsesAuthoritativeFieldContentAndExhaustsOneCompletion) {
    worldsim::protocol::Simulation simulation{42};
    const auto initial_field = simulation.field_work_projection();
    const auto initial_resources = simulation.village_household_resource_projection();

    EXPECT_EQ(initial_field.work_place_id, 12);
    EXPECT_EQ(initial_field.destination_household_id, 21);
    EXPECT_EQ(initial_field.yield_grain_units, 2);
    EXPECT_EQ(initial_field.remaining_work_completions, 1);
    EXPECT_EQ(initial_field.tick, initial_resources.tick);
    EXPECT_EQ(initial_field.revision, initial_resources.revision);
    EXPECT_EQ(initial_field.protocol_version, worldsim::protocol::kProtocolVersion);

    bool worked = false;
    worldsim::protocol::ControlledActorWorkResult work_result{};
    worldsim::protocol::FieldWorkProjection before_work{};
    worldsim::protocol::VillageHouseholdResourceProjection resources_before_work{};

    for (int opportunity = 0; opportunity < 480; ++opportunity) {
        const auto spatial = simulation.controlled_actor_spatial_projection();
        ASSERT_TRUE(simulation.submit_controlled_actor_move_intent(
            intent_toward(spatial, initial_field)
        ).has_value());
        const auto movement = simulation.advance_locomotion_tick();
        ASSERT_TRUE(movement.has_value());

        before_work = simulation.field_work_projection();
        resources_before_work = simulation.village_household_resource_projection();
        const auto attempt = simulation.controlled_actor_complete_field_work();
        if (!attempt.has_value()) {
            ASSERT_EQ(attempt.error(), worldsim::protocol::ControlledActorWorkError::outside_field);
            continue;
        }

        worked = true;
        work_result = *attempt;
        break;
    }

    ASSERT_TRUE(worked);
    const auto *destination_before = find_household(
        resources_before_work,
        before_work.destination_household_id
    );
    ASSERT_NE(destination_before, nullptr);

    EXPECT_EQ(work_result.entity_id, 1);
    EXPECT_EQ(work_result.work_place_id, before_work.work_place_id);
    EXPECT_EQ(work_result.destination_household_id, before_work.destination_household_id);
    EXPECT_EQ(work_result.produced_grain_units, before_work.yield_grain_units);
    EXPECT_EQ(
        work_result.destination_household_grain_stock_units,
        destination_before->grain_stock_units + work_result.produced_grain_units
    );
    EXPECT_EQ(work_result.remaining_work_completions, 0);
    EXPECT_EQ(work_result.tick, before_work.tick);
    EXPECT_EQ(work_result.revision, before_work.revision + 1);
    EXPECT_EQ(work_result.protocol_version, worldsim::protocol::kProtocolVersion);

    const auto after_field = simulation.field_work_projection();
    const auto after_resources = simulation.village_household_resource_projection();
    const auto *destination_after = find_household(
        after_resources,
        work_result.destination_household_id
    );
    ASSERT_NE(destination_after, nullptr);
    EXPECT_EQ(after_field.work_place_id, before_work.work_place_id);
    EXPECT_EQ(after_field.destination_household_id, before_work.destination_household_id);
    EXPECT_EQ(after_field.yield_grain_units, before_work.yield_grain_units);
    EXPECT_EQ(after_field.remaining_work_completions, 0);
    EXPECT_EQ(after_field.tick, work_result.tick);
    EXPECT_EQ(after_field.revision, work_result.revision);
    EXPECT_EQ(destination_after->grain_stock_units, work_result.destination_household_grain_stock_units);

    const auto exhausted_before = simulation.field_work_projection();
    const auto exhausted = simulation.controlled_actor_complete_field_work();
    ASSERT_FALSE(exhausted.has_value());
    EXPECT_EQ(exhausted.error(), worldsim::protocol::ControlledActorWorkError::work_exhausted);
    EXPECT_EQ(simulation.field_work_projection(), exhausted_before);

    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
    const auto next_movement = simulation.advance_locomotion_tick();
    ASSERT_TRUE(next_movement.has_value());
    EXPECT_EQ(next_movement->tick, work_result.tick + 1);
    EXPECT_GT(next_movement->revision, work_result.revision);
}

TEST(ResourceProtocol, StandingTransferPledgeIsSemanticRevisionOnlyCommand) {
    worldsim::protocol::Simulation simulation{42};
    const auto initial_pledge = simulation.standing_transfer_pledge_projection();
    const auto initial_carry = simulation.controlled_actor_carry_projection();
    const auto initial_resources = simulation.village_household_resource_projection();

    EXPECT_EQ(initial_pledge.source_household_id, *initial_carry.member_household_id);
    EXPECT_EQ(initial_pledge.destination_household_id, 21);
    EXPECT_EQ(initial_pledge.remaining_grain_units, 4);
    EXPECT_EQ(initial_pledge.tick, initial_resources.tick);
    EXPECT_EQ(initial_pledge.revision, initial_resources.revision);
    EXPECT_EQ(initial_pledge.protocol_version, worldsim::protocol::kProtocolVersion);

    bool shortage_seen = false;
    for (int opportunity = 0; opportunity < 480; ++opportunity) {
        ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
        ASSERT_TRUE(simulation.advance_locomotion_tick().has_value());
        const auto resources = simulation.village_household_resource_projection();
        const auto *target = find_household(resources, initial_pledge.destination_household_id);
        ASSERT_NE(target, nullptr);
        if (target->status == worldsim::protocol::HouseholdResourceStatus::shortage) {
            shortage_seen = true;
            break;
        }
    }
    ASSERT_TRUE(shortage_seen);

    const auto before_pledge = simulation.standing_transfer_pledge_projection();
    const auto before_resources = simulation.village_household_resource_projection();
    const auto *source_before = find_household(before_resources, before_pledge.source_household_id);
    const auto *destination_before = find_household(
        before_resources,
        before_pledge.destination_household_id
    );
    ASSERT_NE(source_before, nullptr);
    ASSERT_NE(destination_before, nullptr);

    const auto transferred = simulation.controlled_actor_execute_household_transfer_pledge();
    ASSERT_TRUE(transferred.has_value());
    EXPECT_EQ(transferred->entity_id, 1);
    EXPECT_EQ(transferred->source_household_id, before_pledge.source_household_id);
    EXPECT_EQ(transferred->destination_household_id, before_pledge.destination_household_id);
    EXPECT_EQ(transferred->transferred_grain_units, before_pledge.remaining_grain_units);
    EXPECT_EQ(
        transferred->source_household_grain_stock_units,
        source_before->grain_stock_units - transferred->transferred_grain_units
    );
    EXPECT_EQ(
        transferred->destination_household_grain_stock_units,
        destination_before->grain_stock_units + transferred->transferred_grain_units
    );
    EXPECT_EQ(transferred->remaining_pledge_grain_units, 0);
    EXPECT_EQ(transferred->tick, before_resources.tick);
    EXPECT_EQ(transferred->revision, before_resources.revision + 1);
    EXPECT_EQ(transferred->protocol_version, worldsim::protocol::kProtocolVersion);

    const auto after_pledge = simulation.standing_transfer_pledge_projection();
    const auto after_resources = simulation.village_household_resource_projection();
    const auto *destination_after = find_household(
        after_resources,
        transferred->destination_household_id
    );
    ASSERT_NE(destination_after, nullptr);
    EXPECT_EQ(after_pledge.remaining_grain_units, 0);
    EXPECT_EQ(after_pledge.destination_household_id, before_pledge.destination_household_id);
    EXPECT_EQ(after_pledge.tick, transferred->tick);
    EXPECT_EQ(after_pledge.revision, transferred->revision);
    EXPECT_EQ(destination_after->status, worldsim::protocol::HouseholdResourceStatus::adequate);

    const auto exhausted = simulation.controlled_actor_execute_household_transfer_pledge();
    ASSERT_FALSE(exhausted.has_value());
    EXPECT_EQ(exhausted.error(), worldsim::protocol::ControlledActorTransferError::pledge_zero);
    EXPECT_EQ(simulation.standing_transfer_pledge_projection(), after_pledge);
    EXPECT_EQ(simulation.village_household_resource_projection().revision, after_resources.revision);

    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
    const auto next_movement = simulation.advance_locomotion_tick();
    ASSERT_TRUE(next_movement.has_value());
    EXPECT_EQ(next_movement->tick, transferred->tick + 1);
    EXPECT_GT(next_movement->revision, transferred->revision);
}

} // namespace
