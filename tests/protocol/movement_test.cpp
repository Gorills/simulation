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
        .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
    });

    ASSERT_TRUE(submitted.has_value());
    EXPECT_EQ(simulation.controlled_actor_spatial_projection(), spatial_before);
    EXPECT_EQ(simulation.observed_world_projection(), observed_before);
}

TEST(ControlledMovementProtocol, LocomotionTickReturnsResolvedPlayerAndNpcSamples) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
        .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
    }).has_value());

    const auto result = simulation.advance_locomotion_tick();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->samples.size(), 2U);
    EXPECT_EQ(result->tick, 1);
    EXPECT_EQ(result->revision, 3);
    EXPECT_EQ(result->protocol_version, worldsim::protocol::kProtocolVersion);

    const auto &controlled_sample = result->samples[0];
    EXPECT_EQ(controlled_sample.entity_id, 1);
    EXPECT_EQ(controlled_sample.x_mm, 1);
    EXPECT_EQ(controlled_sample.y_mm, 0);
    EXPECT_EQ(controlled_sample.z_mm, 0);
    EXPECT_EQ(controlled_sample.velocity_x_mm_per_second, 100);
    EXPECT_EQ(controlled_sample.velocity_y_mm_per_second, 0);
    EXPECT_EQ(controlled_sample.velocity_z_mm_per_second, 0);
    EXPECT_EQ(controlled_sample.spatial_epoch, 1);

    const auto &npc_sample = result->samples[1];
    EXPECT_EQ(npc_sample.entity_id, 2);
    EXPECT_EQ(npc_sample.x_mm, 2'999);
    EXPECT_EQ(npc_sample.y_mm, 0);
    EXPECT_EQ(npc_sample.z_mm, -3'000);
    EXPECT_EQ(npc_sample.velocity_x_mm_per_second, -100);
    EXPECT_EQ(npc_sample.velocity_y_mm_per_second, 0);
    EXPECT_EQ(npc_sample.velocity_z_mm_per_second, 0);
    EXPECT_EQ(npc_sample.spatial_epoch, 1);

    const auto projection = simulation.controlled_actor_spatial_projection();
    EXPECT_EQ(projection.entity_id, controlled_sample.entity_id);
    EXPECT_EQ(projection.x_mm, controlled_sample.x_mm);
    EXPECT_EQ(projection.y_mm, controlled_sample.y_mm);
    EXPECT_EQ(projection.z_mm, controlled_sample.z_mm);
    EXPECT_EQ(projection.velocity_x_mm_per_second, controlled_sample.velocity_x_mm_per_second);
    EXPECT_EQ(projection.velocity_y_mm_per_second, controlled_sample.velocity_y_mm_per_second);
    EXPECT_EQ(projection.velocity_z_mm_per_second, controlled_sample.velocity_z_mm_per_second);
    EXPECT_EQ(projection.spatial_epoch, controlled_sample.spatial_epoch);
    EXPECT_EQ(projection.tick, result->tick);
    EXPECT_EQ(projection.revision, result->revision);
}

TEST(ControlledMovementProtocol, InvalidIntentDoesNotReplaceLastAcceptedIntent) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
        .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
    }).has_value());

    const auto invalid = simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 1000,
        .pace = worldsim::protocol::ControlledActorLocomotionPace::sprint,
    });

    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error(), worldsim::protocol::ControlledActorMovementError::invalid_intent);

    const auto result = simulation.advance_locomotion_tick();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->samples.size(), 2U);
    EXPECT_EQ(result->samples[0].entity_id, 1);
    EXPECT_EQ(result->samples[0].x_mm, 1);
    EXPECT_EQ(result->samples[0].velocity_x_mm_per_second, 100);
}

TEST(ControlledMovementProtocol, InvalidPaceDoesNotReplaceLastAcceptedIntent) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
        .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
    }).has_value());

    const auto invalid = simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
        .pace = static_cast<worldsim::protocol::ControlledActorLocomotionPace>(99),
    });
    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error(), worldsim::protocol::ControlledActorMovementError::invalid_pace);

    const auto result = simulation.advance_locomotion_tick();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->samples[0].velocity_x_mm_per_second, 100);
}

TEST(ControlledMovementProtocol, ZeroIntentBrakesGroundedMotionBeforeStopping) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .x = 1000,
        .z = 0,
        .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
    }).has_value());
    for (int tick = 0; tick < 30; ++tick) {
        ASSERT_TRUE(simulation.advance_locomotion_tick().has_value());
    }

    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
    }).has_value());
    const auto braking = simulation.advance_locomotion_tick();
    ASSERT_TRUE(braking.has_value());
    EXPECT_EQ(braking->samples[0].entity_id, 1);
    EXPECT_EQ(braking->samples[0].x_mm, 822);
    EXPECT_EQ(braking->samples[0].velocity_x_mm_per_second, 2'867);

    worldsim::protocol::AuthoritativeMovementSampleBatch stopped{};
    for (int tick = 0; tick < 22; ++tick) {
        const auto next = simulation.advance_locomotion_tick();
        ASSERT_TRUE(next.has_value());
        stopped = *next;
    }
    EXPECT_EQ(stopped.samples[0].x_mm, 1'312);
    EXPECT_EQ(stopped.samples[0].velocity_x_mm_per_second, 0);
    EXPECT_EQ(stopped.tick, 53);
    EXPECT_EQ(stopped.revision, 55);
    EXPECT_EQ(stopped.samples[0].spatial_epoch, 1);
}

TEST(ControlledMovementProtocol, RepeatedBatchesAreStrictlyOrderedByTickAndRevision) {
    worldsim::protocol::Simulation first{42};
    worldsim::protocol::Simulation second{42};
    const worldsim::protocol::ControlledActorMoveIntent intent{
        .x = 1000,
        .z = 0,
        .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
    };
    ASSERT_TRUE(first.submit_controlled_actor_move_intent(intent).has_value());
    ASSERT_TRUE(second.submit_controlled_actor_move_intent(intent).has_value());

    worldsim::protocol::ProtocolInteger previous_tick = 0;
    worldsim::protocol::ProtocolInteger previous_revision = 2;
    for (int tick = 0; tick < 60; ++tick) {
        const auto first_result = first.advance_locomotion_tick();
        const auto second_result = second.advance_locomotion_tick();
        ASSERT_TRUE(first_result.has_value());
        ASSERT_TRUE(second_result.has_value());
        EXPECT_EQ(*first_result, *second_result);
        ASSERT_EQ(first_result->samples.size(), 2U);
        EXPECT_EQ(first_result->samples[0].entity_id, 1);
        EXPECT_EQ(first_result->samples[1].entity_id, 2);
        EXPECT_GT(first_result->tick, previous_tick);
        EXPECT_GT(first_result->revision, previous_revision);
        previous_tick = first_result->tick;
        previous_revision = first_result->revision;
    }

    const auto spatial = first.controlled_actor_spatial_projection();
    EXPECT_EQ(spatial.x_mm, 2'275);
    EXPECT_EQ(spatial.velocity_x_mm_per_second, 3'000);
    EXPECT_EQ(spatial.tick, 60);
    EXPECT_EQ(spatial.revision, 62);
    EXPECT_EQ(spatial.spatial_epoch, 1);
}

TEST(ControlledMovementProtocol, LivingNeedNpcWalksAndBrakesInsideAssignedRestTolerance) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({
        .pace = worldsim::protocol::ControlledActorLocomotionPace::run,
    }).has_value());

    worldsim::protocol::AuthoritativeMovementSampleBatch last{};
    for (int tick = 0; tick < 364; ++tick) {
        const auto advanced = simulation.advance_locomotion_tick();
        ASSERT_TRUE(advanced.has_value());
        ASSERT_EQ(advanced->samples.size(), 2U);
        EXPECT_EQ(advanced->samples[0].entity_id, 1);
        EXPECT_EQ(advanced->samples[1].entity_id, 2);
        last = *advanced;
    }

    EXPECT_EQ(last.tick, 364);
    EXPECT_EQ(last.revision, 366);
    EXPECT_EQ(last.samples[1].x_mm, -2'912);
    EXPECT_EQ(last.samples[1].z_mm, -3'000);
    EXPECT_EQ(last.samples[1].velocity_x_mm_per_second, 0);
    EXPECT_EQ(last.samples[1].velocity_z_mm_per_second, 0);
    EXPECT_EQ(last.samples[1].spatial_epoch, 1);
}

} // namespace
