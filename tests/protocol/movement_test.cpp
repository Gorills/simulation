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

TEST(ControlledMovementProtocol, LocomotionTickReturnsPostTransitionAuthoritativeSampleBatch) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
    }).has_value());

    const auto result = simulation.advance_locomotion_tick();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->samples.size(), 1U);
    EXPECT_EQ(result->tick, 1);
    EXPECT_EQ(result->revision, 2);
    EXPECT_EQ(result->protocol_version, worldsim::protocol::kProtocolVersion);

    const auto &sample = result->samples.front();
    EXPECT_EQ(sample.entity_id, 1);
    EXPECT_EQ(sample.x_mm, 96);
    EXPECT_EQ(sample.y_mm, 0);
    EXPECT_EQ(sample.z_mm, 0);
    EXPECT_EQ(sample.velocity_x_mm_per_second, 5800);
    EXPECT_EQ(sample.velocity_y_mm_per_second, 0);
    EXPECT_EQ(sample.velocity_z_mm_per_second, 0);
    EXPECT_EQ(sample.spatial_epoch, 1);

    const auto projection = simulation.controlled_actor_spatial_projection();
    EXPECT_EQ(projection.entity_id, sample.entity_id);
    EXPECT_EQ(projection.x_mm, sample.x_mm);
    EXPECT_EQ(projection.y_mm, sample.y_mm);
    EXPECT_EQ(projection.z_mm, sample.z_mm);
    EXPECT_EQ(projection.velocity_x_mm_per_second, sample.velocity_x_mm_per_second);
    EXPECT_EQ(projection.velocity_y_mm_per_second, sample.velocity_y_mm_per_second);
    EXPECT_EQ(projection.velocity_z_mm_per_second, sample.velocity_z_mm_per_second);
    EXPECT_EQ(projection.spatial_epoch, sample.spatial_epoch);
    EXPECT_EQ(projection.tick, result->tick);
    EXPECT_EQ(projection.revision, result->revision);
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
    ASSERT_EQ(result->samples.size(), 1U);
    EXPECT_EQ(result->samples.front().x_mm, 96);
    EXPECT_EQ(result->samples.front().velocity_x_mm_per_second, 5800);
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
    ASSERT_EQ(stopped->samples.size(), 1U);
    EXPECT_EQ(stopped->samples.front().x_mm, 96);
    EXPECT_EQ(stopped->samples.front().velocity_x_mm_per_second, 0);
    EXPECT_EQ(stopped->tick, 2);
    EXPECT_EQ(stopped->revision, 3);
    EXPECT_EQ(stopped->samples.front().spatial_epoch, 1);
}

TEST(ControlledMovementProtocol, RepeatedBatchesAreStrictlyOrderedByTickAndRevision) {
    worldsim::protocol::Simulation first{42};
    worldsim::protocol::Simulation second{42};
    const worldsim::protocol::ControlledActorMoveIntent intent{
        .x = 1000,
        .z = 0,
    };
    ASSERT_TRUE(first.submit_controlled_actor_move_intent(intent).has_value());
    ASSERT_TRUE(second.submit_controlled_actor_move_intent(intent).has_value());

    worldsim::protocol::ProtocolInteger previous_tick = 0;
    worldsim::protocol::ProtocolInteger previous_revision = 1;
    for (int tick = 0; tick < 60; ++tick) {
        const auto first_result = first.advance_locomotion_tick();
        const auto second_result = second.advance_locomotion_tick();
        ASSERT_TRUE(first_result.has_value());
        ASSERT_TRUE(second_result.has_value());
        EXPECT_EQ(*first_result, *second_result);
        ASSERT_EQ(first_result->samples.size(), 1U);
        EXPECT_GT(first_result->tick, previous_tick);
        EXPECT_GT(first_result->revision, previous_revision);
        previous_tick = first_result->tick;
        previous_revision = first_result->revision;
    }

    const auto spatial = first.controlled_actor_spatial_projection();
    EXPECT_EQ(spatial.x_mm, 5800);
    EXPECT_EQ(spatial.velocity_x_mm_per_second, 5800);
    EXPECT_EQ(spatial.tick, 60);
    EXPECT_EQ(spatial.revision, 61);
    EXPECT_EQ(spatial.spatial_epoch, 1);
}

} // namespace
