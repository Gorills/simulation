#include "protocol/move.hpp"
#include "sim/world.hpp"

#include <gtest/gtest.h>

namespace {

TEST(WorldMovement, AppliesTheSameAuthoritativeStepForTheSameInput) {
    worldsim::sim::World first{42};
    worldsim::sim::World second{42};
    const worldsim::protocol::MoveIntent move_right{.dx = 1, .dy = 0};

    const auto first_result = first.move(move_right);
    const auto second_result = second.move(move_right);

    ASSERT_TRUE(first_result.has_value());
    ASSERT_TRUE(second_result.has_value());
    EXPECT_EQ(first_result.value(), second_result.value());
    EXPECT_EQ(first.player_projection(), second.player_projection());
    EXPECT_EQ(first.player_projection().x, 1);
    EXPECT_EQ(first.player_projection().y, 0);
    EXPECT_EQ(first.player_projection().tick, 1U);
    EXPECT_EQ(first.player_projection().seed, 42U);
    EXPECT_EQ(first.player_projection().protocol_version, worldsim::protocol::kProtocolVersion);
}

TEST(WorldMovement, RejectsInvalidMovementWithoutChangingWorldState) {
    worldsim::sim::World world{7};
    const auto before = world.player_projection();

    const auto result = world.move({.dx = 1, .dy = 1});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::protocol::MoveError::invalid_delta);
    EXPECT_EQ(world.player_projection(), before);
}

} // namespace
