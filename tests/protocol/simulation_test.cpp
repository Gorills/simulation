#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

namespace {

TEST(SimulationProtocol, ObservedWorldStartsWithOnlyTheBoundControlledActor) {
    worldsim::protocol::Simulation simulation{42};

    const auto projection = simulation.observed_world_projection();

    EXPECT_EQ(projection.controlled_actor_id, 1U);
    EXPECT_EQ(projection.tick, 0U);
    EXPECT_EQ(projection.revision, 1U);
    EXPECT_EQ(projection.protocol_version, worldsim::protocol::kProtocolVersion);
    ASSERT_EQ(projection.entities.size(), 1U);
    EXPECT_EQ(projection.entities.front().entity_id, 1U);
}

TEST(SimulationProtocol, BootstrapMoveUpdatesRevisionSeenByObservedWorld) {
    worldsim::protocol::Simulation simulation{42};

    const auto result = simulation.bootstrap_move({.dx = 1, .dy = 0});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->actor.entity_id, 1U);
    EXPECT_EQ(result->actor.x, 1);
    EXPECT_EQ(result->actor.y, 0);
    EXPECT_EQ(result->actor.tick, 0U);
    EXPECT_EQ(result->actor.revision, 2U);
    EXPECT_EQ(result->actor.seed, 42U);
    EXPECT_EQ(result->actor.protocol_version, worldsim::protocol::kProtocolVersion);

    const auto observed = simulation.observed_world_projection();
    EXPECT_EQ(observed.controlled_actor_id, result->actor.entity_id);
    EXPECT_EQ(observed.tick, result->actor.tick);
    EXPECT_EQ(observed.revision, result->actor.revision);
    ASSERT_EQ(observed.entities.size(), 1U);
    EXPECT_EQ(observed.entities.front().entity_id, result->actor.entity_id);
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
