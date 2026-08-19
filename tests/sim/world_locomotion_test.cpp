#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

namespace {

[[nodiscard]] worldsim::sim::SpatialState spatial_at(
    const std::int64_t x = 0,
    const std::int64_t y = 0,
    const std::int64_t z = 0
) {
    return worldsim::sim::SpatialState{
        .position = {
            .x = worldsim::sim::Millimeters{x},
            .y = worldsim::sim::Millimeters{y},
            .z = worldsim::sim::Millimeters{z},
        },
        .velocity = {},
        .epoch = worldsim::sim::SpatialEpoch{1},
    };
}

TEST(WorldLocomotion, DifferentActorsAdvanceThroughOneSharedWorldTick) {
    auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    worldsim::sim::World world{worldsim::sim::WorldSeed{21}};

    ASSERT_TRUE(world.spawn_actor(
        worldsim::sim::EntityId{1},
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());
    ASSERT_TRUE(world.spawn_actor(
        worldsim::sim::EntityId{2},
        worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0, 1'000)}
    ).has_value());

    // Deliberately reverse source order. Samples must be canonical by EntityId,
    // not inherit collection order from future player/NPC intent producers.
    const std::array intents{
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{2},
            .move = {.x = 0, .z = 1000},
        },
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{1},
            .move = {.x = 1000, .z = 0},
        },
    };
    const auto advanced = world.advance_grounded_locomotion_tick(context, intents);

    ASSERT_TRUE(advanced.has_value());
    ASSERT_EQ(advanced->samples.size(), 2U);
    EXPECT_EQ(advanced->tick, (worldsim::sim::SimulationTick{1}));
    EXPECT_EQ(advanced->revision, (worldsim::sim::WorldRevision{3}));
    EXPECT_EQ(advanced->samples[0].actor, (worldsim::sim::EntityId{1}));
    EXPECT_EQ(advanced->samples[1].actor, (worldsim::sim::EntityId{2}));
    EXPECT_EQ(advanced->samples[0].spatial.position.x.value, 96);
    EXPECT_EQ(advanced->samples[0].spatial.position.z.value, 0);
    EXPECT_EQ(advanced->samples[1].spatial.position.x.value, 0);
    EXPECT_EQ(advanced->samples[1].spatial.position.z.value, 1'096);

    const auto first = world.actor_spatial_state(worldsim::sim::EntityId{1});
    const auto second = world.actor_spatial_state(worldsim::sim::EntityId{2});
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*first, advanced->samples[0].spatial);
    EXPECT_EQ(*second, advanced->samples[1].spatial);
    EXPECT_EQ(world.tick(), advanced->tick);
    EXPECT_EQ(world.revision(), advanced->revision);
    EXPECT_EQ(first->epoch.value, 1U);
    EXPECT_EQ(second->epoch.value, 1U);
}

TEST(WorldLocomotion, InvalidBatchIsRejectedWithoutPartialMutationOrTimeAdvance) {
    auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    worldsim::sim::World world{worldsim::sim::WorldSeed{22}};

    ASSERT_TRUE(world.spawn_actor(
        worldsim::sim::EntityId{1},
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());
    ASSERT_TRUE(world.spawn_actor(
        worldsim::sim::EntityId{2},
        worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0, 1'000)}
    ).has_value());
    const auto before = world.snapshot();

    const std::array intents{
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{1},
            .move = {.x = 1000, .z = 0},
        },
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{2},
            .move = {.x = 1000, .z = 1000},
        },
    };
    const auto advanced = world.advance_grounded_locomotion_tick(context, intents);

    ASSERT_FALSE(advanced.has_value());
    EXPECT_EQ(advanced.error(), worldsim::sim::GroundedLocomotionTickError::invalid_intent);
    EXPECT_EQ(world.snapshot(), before);
}

TEST(WorldLocomotion, DuplicateActorIntentIsRejectedAtomically) {
    auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    worldsim::sim::World world{worldsim::sim::WorldSeed{23}};

    ASSERT_TRUE(world.spawn_actor(
        worldsim::sim::EntityId{1},
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());
    const auto before = world.snapshot();

    const std::array intents{
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{1},
            .move = {.x = 1000, .z = 0},
        },
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{1},
            .move = {.x = 0, .z = 1000},
        },
    };
    const auto advanced = world.advance_grounded_locomotion_tick(context, intents);

    ASSERT_FALSE(advanced.has_value());
    EXPECT_EQ(
        advanced.error(),
        worldsim::sim::GroundedLocomotionTickError::duplicate_actor_intent
    );
    EXPECT_EQ(world.snapshot(), before);
}

TEST(WorldLocomotion, FixedStepContinuationSurvivesSnapshotRestore) {
    auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    worldsim::sim::World uninterrupted{worldsim::sim::WorldSeed{24}};
    const worldsim::sim::EntityId actor{1};

    ASSERT_TRUE(uninterrupted.spawn_actor(
        actor,
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());

    const std::array intent{
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = actor,
            .move = {.x = 1000, .z = 0},
        },
    };
    ASSERT_TRUE(uninterrupted.advance_grounded_locomotion_tick(context, intent).has_value());
    const auto saved = uninterrupted.snapshot();

    ASSERT_EQ(saved.schema_version, worldsim::sim::kWorldSnapshotSchemaVersion);
    ASSERT_EQ(saved.actors.size(), 1U);
    EXPECT_EQ(saved.actors.front().grounded_locomotion.ticks_per_second, 60U);
    EXPECT_EQ(saved.actors.front().grounded_locomotion.remainder.x, 40);

    worldsim::sim::World restored{worldsim::sim::WorldSeed{999}};
    ASSERT_TRUE(restored.restore(saved).has_value());

    const auto uninterrupted_next = uninterrupted.advance_grounded_locomotion_tick(context, intent);
    const auto restored_next = restored.advance_grounded_locomotion_tick(context, intent);
    ASSERT_TRUE(uninterrupted_next.has_value());
    ASSERT_TRUE(restored_next.has_value());

    EXPECT_EQ(*restored_next, *uninterrupted_next);
    EXPECT_EQ(restored.snapshot(), uninterrupted.snapshot());
    const auto spatial = restored.actor_spatial_state(actor);
    ASSERT_TRUE(spatial.has_value());
    EXPECT_EQ(spatial->position.x.value, 193);
}

TEST(WorldLocomotion, RestoreRejectsMalformedContinuationWithoutMutatingTarget) {
    worldsim::sim::World source{worldsim::sim::WorldSeed{25}};
    ASSERT_TRUE(source.spawn_actor(
        worldsim::sim::EntityId{1},
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());

    auto malformed = source.snapshot();
    ASSERT_EQ(malformed.actors.size(), 1U);
    malformed.actors.front().grounded_locomotion.ticks_per_second = 60;
    malformed.actors.front().grounded_locomotion.remainder.x = 60;

    worldsim::sim::World target{worldsim::sim::WorldSeed{26}};
    ASSERT_TRUE(target.spawn_actor(worldsim::sim::EntityId{9}).has_value());
    const auto before = target.snapshot();

    const auto restored = target.restore(malformed);

    ASSERT_FALSE(restored.has_value());
    EXPECT_EQ(
        restored.error(),
        worldsim::sim::WorldSnapshotError::invalid_grounded_locomotion_state
    );
    EXPECT_EQ(target.snapshot(), before);
}

} // namespace
