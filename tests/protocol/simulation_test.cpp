#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

namespace {

TEST(SimulationProtocol, ValidatesIntentThenReturnsAuthoritativeProjection) {
    worldsim::protocol::Simulation simulation{42};

    const auto result = simulation.move({.dx = 1, .dy = 0});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->player.x, 1);
    EXPECT_EQ(result->player.y, 0);
    EXPECT_EQ(result->player.tick, 1U);
    EXPECT_EQ(result->player.seed, 42U);
    EXPECT_EQ(result->player.protocol_version, worldsim::protocol::kProtocolVersion);
    EXPECT_EQ(simulation.player_projection(), result->player);
}

TEST(SimulationProtocol, RejectsMalformedIntentWithoutMutatingTheWorld) {
    worldsim::protocol::Simulation simulation{7};
    const auto before = simulation.player_projection();

    const auto result = simulation.move({.dx = 1, .dy = 1});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::protocol::MoveError::invalid_delta);
    EXPECT_EQ(simulation.player_projection(), before);
}

} // namespace
