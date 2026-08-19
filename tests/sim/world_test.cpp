#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <array>

namespace {

TEST(WorldActors, DifferentActorsUseTheSameAuthoritativeOperation) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{42}};
    const worldsim::sim::EntityId first{100};
    const worldsim::sim::EntityId second{200};

    ASSERT_TRUE(world.spawn_actor(first).has_value());
    ASSERT_TRUE(world.spawn_actor(second, {.x = 4, .y = 3}).has_value());
    ASSERT_TRUE(world.apply_bootstrap_step(first, worldsim::sim::CardinalDirection::east).has_value());
    ASSERT_TRUE(world.apply_bootstrap_step(second, worldsim::sim::CardinalDirection::north).has_value());

    ASSERT_NE(world.actor(first), nullptr);
    ASSERT_NE(world.actor(second), nullptr);
    EXPECT_EQ(world.actor(first)->bootstrap_position, (worldsim::sim::GridPosition{.x = 1, .y = 0}));
    EXPECT_EQ(world.actor(second)->bootstrap_position, (worldsim::sim::GridPosition{.x = 4, .y = 2}));
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{0}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{4}));
}

TEST(WorldTime, ActorActionsDoNotPretendThatWorldTimeAdvanced) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{7}};
    const worldsim::sim::EntityId actor{1};

    ASSERT_TRUE(world.spawn_actor(actor).has_value());
    ASSERT_TRUE(world.apply_bootstrap_step(actor, worldsim::sim::CardinalDirection::east).has_value());
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{0}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{2}));

    world.advance_one_tick();

    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{1}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{3}));
}

TEST(WorldActors, RejectsDuplicateAndUnknownEntityWithoutMutation) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{9}};
    const worldsim::sim::EntityId actor{3};

    ASSERT_TRUE(world.spawn_actor(actor).has_value());
    const auto before = world.revision();

    const auto duplicate = world.spawn_actor(actor);
    const auto unknown_move = world.apply_bootstrap_step(
        worldsim::sim::EntityId{999},
        worldsim::sim::CardinalDirection::south
    );

    ASSERT_FALSE(duplicate.has_value());
    EXPECT_EQ(duplicate.error(), worldsim::sim::WorldError::duplicate_entity);
    ASSERT_FALSE(unknown_move.has_value());
    EXPECT_EQ(unknown_move.error(), worldsim::sim::WorldError::unknown_entity);
    EXPECT_EQ(world.revision(), before);
}

TEST(WorldDeterminism, SameInitialActorsAndActionsProduceTheSameState) {
    worldsim::sim::World first{worldsim::sim::WorldSeed{11}};
    worldsim::sim::World second{worldsim::sim::WorldSeed{11}};
    const worldsim::sim::EntityId actor{5};
    constexpr std::array directions{
        worldsim::sim::CardinalDirection::north,
        worldsim::sim::CardinalDirection::east,
        worldsim::sim::CardinalDirection::south,
        worldsim::sim::CardinalDirection::west,
        worldsim::sim::CardinalDirection::east,
    };

    ASSERT_TRUE(first.spawn_actor(actor).has_value());
    ASSERT_TRUE(second.spawn_actor(actor).has_value());
    for (const auto direction : directions) {
        ASSERT_TRUE(first.apply_bootstrap_step(actor, direction).has_value());
        ASSERT_TRUE(second.apply_bootstrap_step(actor, direction).has_value());
    }

    ASSERT_NE(first.actor(actor), nullptr);
    ASSERT_NE(second.actor(actor), nullptr);
    EXPECT_EQ(*first.actor(actor), *second.actor(actor));
    EXPECT_EQ(first.tick(), second.tick());
    EXPECT_EQ(first.revision(), second.revision());
    EXPECT_EQ(first.seed(), second.seed());
}

} // namespace
