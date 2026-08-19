#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

namespace {

TEST(ControlledMovementProtocol, SubmittingIntentDoesNotMutateAuthoritativeWorldState) {
    worldsim::protocol::Simulation simulation{42};
    const auto spatial_before = simulation.controlled_actor_spatial_projection();
    const auto observed_before = simulation.observed_world_projection();

    const auto submitted = simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
    });

    ASSERT_TRUE(submitted.has_value());
    EXPECT_EQ(simulation.controlled_actor_spatial_projection(), spatial_before);
    EXPECT_EQ(simulation.observed_world_projection(), observed_before);
}

TEST(ControlledMovementProtocol, LocomotionTickAppliesStoredSemanticIntentAuthoritatively) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
    }).has_value());

    const auto result = simulation.advance_locomotion_tick();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->entity_id, 1);
    EXPECT_EQ(result->x_mm, 96);
    EXPECT_EQ(result->y_mm, 0);
    EXPECT_EQ(result->z_mm, 0);
    EXPECT_EQ(result->velocity_x_mm_per_second, 5800);
    EXPECT_EQ(result->velocity_y_mm_per_second, 0);
    EXPECT_EQ(result->velocity_z_mm_per_second, 0);
    EXPECT_EQ(result->spatial_epoch, 1);
    EXPECT_EQ(result->tick, 1);
    EXPECT_EQ(result->revision, 2);
    EXPECT_EQ(result->protocol_version, worldsim::protocol::kProtocolVersion);
}

TEST(ControlledMovementProtocol, InvalidIntentDoesNotReplaceLastAcceptedIntent) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
    }).has_value());

    const auto invalid = simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 1000,
    });

    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(
        invalid.error(),
        worldsim::protocol::ControlledActorMovementError::invalid_intent
    );

    const auto result = simulation.advance_locomotion_tick();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->x_mm, 96);
    EXPECT_EQ(result->velocity_x_mm_per_second, 5800);
}

TEST(ControlledMovementProtocol, ZeroIntentStopsGroundedMotionOnNextWorldTick) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
    }).has_value());
    ASSERT_TRUE(simulation.advance_locomotion_tick().has_value());

    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
    const auto stopped = simulation.advance_locomotion_tick();

    ASSERT_TRUE(stopped.has_value());
    EXPECT_EQ(stopped->x_mm, 96);
    EXPECT_EQ(stopped->velocity_x_mm_per_second, 0);
    EXPECT_EQ(stopped->tick, 2);
    EXPECT_EQ(stopped->revision, 3);
    EXPECT_EQ(stopped->spatial_epoch, 1);
}

TEST(ControlledMovementProtocol, RepeatedTicksPreserveExactAuthoritativeIntegration) {
    worldsim::protocol::Simulation first{42};
    worldsim::protocol::Simulation second{42};
    const worldsim::protocol::ControlledActorMoveIntent intent{
        .x = 1000,
        .z = 0,
    };
    ASSERT_TRUE(first.submit_controlled_actor_move_intent(intent).has_value());
    ASSERT_TRUE(second.submit_controlled_actor_move_intent(intent).has_value());

    for (int tick = 0; tick < 60; ++tick) {
        const auto first_result = first.advance_locomotion_tick();
        const auto second_result = second.advance_locomotion_tick();
        ASSERT_TRUE(first_result.has_value());
        ASSERT_TRUE(second_result.has_value());
        EXPECT_EQ(*first_result, *second_result);
    }

    const auto spatial = first.controlled_actor_spatial_projection();
    EXPECT_EQ(spatial.x_mm, 5800);
    EXPECT_EQ(spatial.velocity_x_mm_per_second, 5800);
    EXPECT_EQ(spatial.tick, 60);
    EXPECT_EQ(spatial.revision, 61);
    EXPECT_EQ(spatial.spatial_epoch, 1);
}

} // namespace
