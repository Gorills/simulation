#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace {

TEST(ProtocolInteger, MatchesSignedApplicationBoundaryWithoutUnsignedWrap) {
    using worldsim::protocol::ProtocolInteger;

    const auto zero = worldsim::protocol::to_protocol_integer(0);
    const auto maximum = worldsim::protocol::to_protocol_integer(
        worldsim::protocol::kMaxProtocolInteger
    );
    const auto too_large = worldsim::protocol::to_protocol_integer(
        worldsim::protocol::kMaxProtocolInteger + 1U
    );

    ASSERT_TRUE(zero.has_value());
    EXPECT_EQ(*zero, 0);
    ASSERT_TRUE(maximum.has_value());
    EXPECT_EQ(*maximum, std::numeric_limits<ProtocolInteger>::max());
    EXPECT_FALSE(too_large.has_value());
}

TEST(SimulationProtocol, SeedUsesTheRepresentableProtocolIntegerDomain) {
    using worldsim::protocol::ProtocolInteger;

    worldsim::protocol::Simulation maximum_seed{
        std::numeric_limits<ProtocolInteger>::max()
    };
    EXPECT_EQ(
        maximum_seed.bootstrap_controlled_actor_projection().seed,
        std::numeric_limits<ProtocolInteger>::max()
    );
    EXPECT_THROW((worldsim::protocol::Simulation{-1}), std::invalid_argument);
}

TEST(SimulationProtocol, ObservedWorldStartsWithControlledActorAndLivingNeedNpc) {
    worldsim::protocol::Simulation simulation{42};

    const auto projection = simulation.observed_world_projection();

    EXPECT_EQ(projection.controlled_actor_id, 1);
    EXPECT_EQ(projection.tick, 0);
    EXPECT_EQ(projection.revision, 2);
    EXPECT_EQ(projection.protocol_version, worldsim::protocol::kProtocolVersion);
    ASSERT_EQ(projection.entities.size(), 2U);
    EXPECT_EQ(projection.entities[0].entity_id, 1);
    EXPECT_EQ(projection.entities[1].entity_id, 2);
}

TEST(SimulationProtocol, LivingNeedProjectionExposesObservableRestFootprintAndTracksWorldTime) {
    worldsim::protocol::Simulation simulation{42};

    const auto initial = simulation.living_need_projection();
    EXPECT_EQ(initial.entity_id, 2);
    EXPECT_EQ(initial.status, worldsim::protocol::LivingNeedStatus::traveling);
    EXPECT_EQ(initial.target_x_mm, -3'000);
    EXPECT_EQ(initial.target_z_mm, -3'000);
    EXPECT_EQ(initial.axis_arrival_tolerance_mm, 150);
    EXPECT_EQ(
        initial.axis_occupancy_tolerance_mm,
        150 + worldsim::sim::kFirstPlayableBody.radius.value
    );
    EXPECT_EQ(initial.tick, 0);
    EXPECT_EQ(initial.revision, 2);
    EXPECT_EQ(initial.protocol_version, worldsim::protocol::kProtocolVersion);

    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
    const auto advanced = simulation.advance_locomotion_tick();
    ASSERT_TRUE(advanced.has_value());

    const auto after_tick = simulation.living_need_projection();
    EXPECT_EQ(after_tick.entity_id, 2);
    EXPECT_EQ(after_tick.status, worldsim::protocol::LivingNeedStatus::traveling);
    EXPECT_EQ(after_tick.target_x_mm, initial.target_x_mm);
    EXPECT_EQ(after_tick.target_z_mm, initial.target_z_mm);
    EXPECT_EQ(after_tick.axis_arrival_tolerance_mm, initial.axis_arrival_tolerance_mm);
    EXPECT_EQ(after_tick.axis_occupancy_tolerance_mm, initial.axis_occupancy_tolerance_mm);
    EXPECT_EQ(after_tick.tick, advanced->tick);
    EXPECT_EQ(after_tick.revision, advanced->revision);
    EXPECT_EQ(after_tick.protocol_version, worldsim::protocol::kProtocolVersion);
}

TEST(SimulationProtocol, ControlledActorStartsWithAuthoritativeSpatialState) {
    worldsim::protocol::Simulation simulation{42};

    const auto spatial = simulation.controlled_actor_spatial_projection();

    EXPECT_EQ(spatial.entity_id, 1);
    EXPECT_EQ(spatial.x_mm, 0);
    EXPECT_EQ(spatial.y_mm, 0);
    EXPECT_EQ(spatial.z_mm, 0);
    EXPECT_EQ(spatial.velocity_x_mm_per_second, 0);
    EXPECT_EQ(spatial.velocity_y_mm_per_second, 0);
    EXPECT_EQ(spatial.velocity_z_mm_per_second, 0);
    EXPECT_EQ(spatial.spatial_epoch, 1);
    EXPECT_EQ(spatial.tick, 0);
    EXPECT_EQ(spatial.revision, 2);
    EXPECT_EQ(spatial.protocol_version, worldsim::protocol::kProtocolVersion);
}

TEST(SimulationProtocol, BootstrapMoveUpdatesRevisionWithoutChangingSpatialState) {
    worldsim::protocol::Simulation simulation{42};
    const auto spatial_before = simulation.controlled_actor_spatial_projection();

    const auto result = simulation.bootstrap_move({.dx = 1, .dy = 0});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->actor.entity_id, 1);
    EXPECT_EQ(result->actor.x, 1);
    EXPECT_EQ(result->actor.y, 0);
    EXPECT_EQ(result->actor.tick, 0);
    EXPECT_EQ(result->actor.revision, 3);
    EXPECT_EQ(result->actor.seed, 42);
    EXPECT_EQ(result->actor.protocol_version, worldsim::protocol::kProtocolVersion);

    const auto observed = simulation.observed_world_projection();
    EXPECT_EQ(observed.controlled_actor_id, result->actor.entity_id);
    EXPECT_EQ(observed.tick, result->actor.tick);
    EXPECT_EQ(observed.revision, result->actor.revision);
    ASSERT_EQ(observed.entities.size(), 2U);
    EXPECT_EQ(observed.entities[0].entity_id, result->actor.entity_id);
    EXPECT_EQ(observed.entities[1].entity_id, 2);

    const auto spatial_after = simulation.controlled_actor_spatial_projection();
    EXPECT_EQ(spatial_after.entity_id, spatial_before.entity_id);
    EXPECT_EQ(spatial_after.x_mm, spatial_before.x_mm);
    EXPECT_EQ(spatial_after.y_mm, spatial_before.y_mm);
    EXPECT_EQ(spatial_after.z_mm, spatial_before.z_mm);
    EXPECT_EQ(
        spatial_after.velocity_x_mm_per_second,
        spatial_before.velocity_x_mm_per_second
    );
    EXPECT_EQ(
        spatial_after.velocity_y_mm_per_second,
        spatial_before.velocity_y_mm_per_second
    );
    EXPECT_EQ(
        spatial_after.velocity_z_mm_per_second,
        spatial_before.velocity_z_mm_per_second
    );
    EXPECT_EQ(spatial_after.spatial_epoch, spatial_before.spatial_epoch);
    EXPECT_EQ(spatial_after.tick, 0);
    EXPECT_EQ(spatial_after.revision, 3);
    EXPECT_EQ(spatial_after.protocol_version, worldsim::protocol::kProtocolVersion);
}

TEST(SimulationProtocol, RejectsMalformedBootstrapIntentWithoutMutatingObservedWorld) {
    worldsim::protocol::Simulation simulation{7};
    const auto before = simulation.observed_world_projection();

    const auto result = simulation.bootstrap_move({.dx = 1, .dy = 1});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::protocol::BootstrapMoveError::invalid_delta);
    EXPECT_EQ(simulation.observed_world_projection(), before);
}

} // namespace
