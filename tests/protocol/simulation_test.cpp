#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

namespace {

TEST(SimulationProtocol, BootstrapMoveReturnsTheBoundActorProjection) {
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
    EXPECT_EQ(simulation.bootstrap_controlled_actor_projection(), result->actor);
}

TEST(SimulationProtocol, RejectsMalformedBootstrapIntentWithoutMutatingTheWorld) {
    worldsim::protocol::Simulation simulation{7};
    const auto before = simulation.bootstrap_controlled_actor_projection();

    const auto result = simulation.bootstrap_move({.dx = 1, .dy = 1});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::protocol::BootstrapMoveError::invalid_delta);
    EXPECT_EQ(simulation.bootstrap_controlled_actor_projection(), before);
}

} // namespace
