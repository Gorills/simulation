#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <optional>

namespace {

TEST(WorldActors, DifferentActorsUseTheSameAuthoritativeOperation) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{42}};
    const worldsim::sim::EntityId first{100};
    const worldsim::sim::EntityId second{200};

    ASSERT_TRUE(world.spawn_actor(first).has_value());
    ASSERT_TRUE(
        world.spawn_actor(
            second,
            worldsim::sim::ActorSpawnState{
                .bootstrap_position = {.x = 4, .y = 3},
            }
        ).has_value()
    );
    ASSERT_TRUE(world.apply_bootstrap_step(first, worldsim::sim::CardinalDirection::east).has_value());
    ASSERT_TRUE(world.apply_bootstrap_step(second, worldsim::sim::CardinalDirection::north).has_value());

    const auto first_position = world.actor_bootstrap_position(first);
    const auto second_position = world.actor_bootstrap_position(second);
    ASSERT_TRUE(first_position.has_value());
    ASSERT_TRUE(second_position.has_value());
    EXPECT_EQ(*first_position, (worldsim::sim::GridPosition{.x = 1, .y = 0}));
    EXPECT_EQ(*second_position, (worldsim::sim::GridPosition{.x = 4, .y = 2}));
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{0}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{4}));
}

TEST(WorldActors, IndexedLookupRemainsCorrectAcrossStorageGrowth) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{43}};
    const worldsim::sim::EntityId first{1};
    constexpr std::int64_t actor_count = 4096;

    ASSERT_TRUE(
        world.spawn_actor(
            first,
            worldsim::sim::ActorSpawnState{
                .bootstrap_position = {.x = 7, .y = -3},
            }
        ).has_value()
    );

    for (std::int64_t value = 2; value <= actor_count; ++value) {
        ASSERT_TRUE(world.spawn_actor(worldsim::sim::EntityId{value}).has_value());
    }

    EXPECT_TRUE(world.contains_actor(first));
    EXPECT_TRUE(world.contains_actor(worldsim::sim::EntityId{actor_count}));
    EXPECT_FALSE(world.contains_actor(worldsim::sim::EntityId{actor_count + 1}));
    EXPECT_FALSE(world.contains_actor(worldsim::sim::EntityId{0}));

    const auto first_before_move = world.actor_bootstrap_position(first);
    const auto last_position = world.actor_bootstrap_position(worldsim::sim::EntityId{actor_count});
    ASSERT_TRUE(first_before_move.has_value());
    ASSERT_TRUE(last_position.has_value());
    EXPECT_EQ(*first_before_move, (worldsim::sim::GridPosition{.x = 7, .y = -3}));
    EXPECT_EQ(*last_position, (worldsim::sim::GridPosition{}));

    ASSERT_TRUE(world.apply_bootstrap_step(first, worldsim::sim::CardinalDirection::east).has_value());
    const auto first_after_move = world.actor_bootstrap_position(first);
    ASSERT_TRUE(first_after_move.has_value());
    EXPECT_EQ(*first_after_move, (worldsim::sim::GridPosition{.x = 8, .y = -3}));
    EXPECT_EQ(
        world.revision(),
        (worldsim::sim::WorldRevision{static_cast<std::uint64_t>(actor_count) + 1U})
    );
}

TEST(WorldSpatial, ExactSpatialStateIsSelectiveAndValidated) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{17}};
    const worldsim::sim::EntityId resolved{10};
    const worldsim::sim::EntityId unresolved{11};
    const worldsim::sim::SpatialState spatial{
        .position = {
            .x = worldsim::sim::Millimeters{1250},
            .y = worldsim::sim::Millimeters{0},
            .z = worldsim::sim::Millimeters{-3750},
        },
        .velocity = {
            .x = worldsim::sim::MillimetersPerSecond{120},
            .y = worldsim::sim::MillimetersPerSecond{0},
            .z = worldsim::sim::MillimetersPerSecond{-240},
        },
        .epoch = worldsim::sim::SpatialEpoch{2},
    };

    ASSERT_TRUE(
        world.spawn_actor(
            resolved,
            worldsim::sim::ActorSpawnState{
                .spatial = std::optional<worldsim::sim::SpatialState>{spatial},
            }
        ).has_value()
    );
    ASSERT_TRUE(world.spawn_actor(unresolved).has_value());

    const auto resolved_spatial = world.actor_spatial_state(resolved);
    ASSERT_TRUE(resolved_spatial.has_value());
    EXPECT_EQ(*resolved_spatial, spatial);
    EXPECT_FALSE(world.actor_spatial_state(unresolved).has_value());
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{2}));

    const auto invalid = world.spawn_actor(
        worldsim::sim::EntityId{12},
        worldsim::sim::ActorSpawnState{
            .spatial = std::optional<worldsim::sim::SpatialState>{worldsim::sim::SpatialState{
                .epoch = worldsim::sim::SpatialEpoch{0},
            }},
        }
    );

    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error(), worldsim::sim::WorldError::invalid_spatial_state);
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{2}));
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

TEST(WorldActors, RejectsInvalidDuplicateAndUnknownEntityWithoutMutation) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{9}};
    const worldsim::sim::EntityId actor{3};

    const auto invalid = world.spawn_actor(worldsim::sim::EntityId{0});
    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error(), worldsim::sim::WorldError::invalid_entity_id);
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{0}));

    ASSERT_TRUE(world.spawn_actor(actor).has_value());
    const auto before = world.revision();

    const auto duplicate = world.spawn_actor(actor);
    const auto invalid_move = world.apply_bootstrap_step(
        worldsim::sim::EntityId{-1},
        worldsim::sim::CardinalDirection::south
    );
    const auto unknown_move = world.apply_bootstrap_step(
        worldsim::sim::EntityId{999},
        worldsim::sim::CardinalDirection::south
    );

    ASSERT_FALSE(duplicate.has_value());
    EXPECT_EQ(duplicate.error(), worldsim::sim::WorldError::duplicate_entity);
    ASSERT_FALSE(invalid_move.has_value());
    EXPECT_EQ(invalid_move.error(), worldsim::sim::WorldError::invalid_entity_id);
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

    EXPECT_EQ(first.actor_bootstrap_position(actor), second.actor_bootstrap_position(actor));
    EXPECT_EQ(first.actor_spatial_state(actor), second.actor_spatial_state(actor));
    EXPECT_EQ(first.tick(), second.tick());
    EXPECT_EQ(first.revision(), second.revision());
    EXPECT_EQ(first.seed(), second.seed());
}

} // namespace
