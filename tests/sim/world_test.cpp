#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <array>

namespace {

TEST(WorldMovement, AppliesCardinalDomainMovementAndAdvancesSimulationTime) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{42}};

    world.move(worldsim::sim::CardinalDirection::east);
    world.move(worldsim::sim::CardinalDirection::north);

    EXPECT_EQ(world.player_position(), (worldsim::sim::GridPosition{.x = 1, .y = -1}));
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{2}));
    EXPECT_EQ(world.seed(), (worldsim::sim::WorldSeed{42}));
}

TEST(WorldMovement, SameInitialStateAndDirectionsProduceTheSameState) {
    worldsim::sim::World first{worldsim::sim::WorldSeed{7}};
    worldsim::sim::World second{worldsim::sim::WorldSeed{7}};
    constexpr std::array directions{
        worldsim::sim::CardinalDirection::north,
        worldsim::sim::CardinalDirection::east,
        worldsim::sim::CardinalDirection::south,
        worldsim::sim::CardinalDirection::west,
        worldsim::sim::CardinalDirection::east,
    };

    for (const auto direction : directions) {
        first.move(direction);
        second.move(direction);
    }

    EXPECT_EQ(first.player_position(), second.player_position());
    EXPECT_EQ(first.tick(), second.tick());
    EXPECT_EQ(first.seed(), second.seed());
}

} // namespace
