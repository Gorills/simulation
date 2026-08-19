#include "sim/living_need.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

[[nodiscard]] worldsim::sim::SpatialState spatial_at(const std::int64_t x, const std::int64_t z) {
    return worldsim::sim::SpatialState{
        .position = {
            .x = worldsim::sim::Millimeters{x},
            .y = worldsim::sim::Millimeters{0},
            .z = worldsim::sim::Millimeters{z},
        },
        .velocity = {},
        .epoch = worldsim::sim::SpatialEpoch{1},
    };
}

[[nodiscard]] worldsim::sim::RestNeedState rest_need() {
    return worldsim::sim::RestNeedState{
        .rest_x = worldsim::sim::Millimeters{-3'000},
        .rest_z = worldsim::sim::Millimeters{-3'000},
        .axis_arrival_tolerance = worldsim::sim::Millimeters{150},
    };
}

TEST(LivingNeed, RestNeedProducesNpcTravelIntentFromAuthoritativeState) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{41}};
    const worldsim::sim::EntityId npc{2};
    ASSERT_TRUE(world.spawn_actor(
        npc,
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(3'000, -3'000),
            .rest_need = rest_need(),
        }
    ).has_value());

    const auto before = world.snapshot();
    const auto decision = worldsim::sim::decide_npc_rest_need(world, npc);

    ASSERT_TRUE(decision.has_value());
    EXPECT_FALSE(decision->satisfied);
    EXPECT_EQ(decision->movement.actor, npc);
    EXPECT_EQ(decision->movement.move, (worldsim::sim::PlanarMoveIntent{.x = -1000, .z = 0}));
    EXPECT_EQ(decision->movement.pace, worldsim::sim::LocomotionPace::walk);
    EXPECT_EQ(world.snapshot(), before);
}

TEST(LivingNeed, RestNeedIsSatisfiedByAuthoritativePositionInsideTolerance) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{42}};
    const worldsim::sim::EntityId npc{2};
    ASSERT_TRUE(world.spawn_actor(
        npc,
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(-2'896, -3'000),
            .rest_need = rest_need(),
        }
    ).has_value());

    const auto decision = worldsim::sim::decide_npc_rest_need(world, npc);

    ASSERT_TRUE(decision.has_value());
    EXPECT_TRUE(decision->satisfied);
    EXPECT_EQ(decision->movement.actor, npc);
    EXPECT_EQ(decision->movement.move, (worldsim::sim::PlanarMoveIntent{}));
    EXPECT_EQ(decision->movement.pace, worldsim::sim::LocomotionPace::walk);
}

TEST(LivingNeed, RestNeedSurvivesSnapshotRestoreAsCausalActorState) {
    worldsim::sim::World source{worldsim::sim::WorldSeed{43}};
    const worldsim::sim::EntityId npc{2};
    ASSERT_TRUE(source.spawn_actor(
        npc,
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(3'000, -3'000),
            .rest_need = rest_need(),
        }
    ).has_value());

    const auto snapshot = source.snapshot();
    EXPECT_EQ(snapshot.schema_version, worldsim::sim::kWorldSnapshotSchemaVersion);

    worldsim::sim::World restored{worldsim::sim::WorldSeed{99}};
    ASSERT_TRUE(restored.restore(snapshot).has_value());
    EXPECT_EQ(restored.snapshot(), snapshot);
    ASSERT_TRUE(restored.actor_rest_need(npc).has_value());
    EXPECT_EQ(*restored.actor_rest_need(npc), rest_need());
}

TEST(LivingNeed, InvalidRestNeedIsRejectedWithoutMutatingWorldOrRestoreTarget) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{44}};
    const auto invalid_spawn = world.spawn_actor(
        worldsim::sim::EntityId{2},
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(0, 0),
            .rest_need = worldsim::sim::RestNeedState{
                .axis_arrival_tolerance = worldsim::sim::Millimeters{-1},
            },
        }
    );
    ASSERT_FALSE(invalid_spawn.has_value());
    EXPECT_EQ(invalid_spawn.error(), worldsim::sim::WorldError::invalid_rest_need_state);
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{0}));

    ASSERT_TRUE(world.spawn_actor(
        worldsim::sim::EntityId{7},
        worldsim::sim::ActorSpawnState{.spatial = spatial_at(100, 100)}
    ).has_value());
    const auto before = world.snapshot();

    worldsim::sim::World source{worldsim::sim::WorldSeed{45}};
    ASSERT_TRUE(source.spawn_actor(
        worldsim::sim::EntityId{2},
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(3'000, -3'000),
            .rest_need = rest_need(),
        }
    ).has_value());
    auto invalid_snapshot = source.snapshot();
    invalid_snapshot.actors.front().rest_need->axis_arrival_tolerance = worldsim::sim::Millimeters{-1};

    const auto restored = world.restore(invalid_snapshot);
    ASSERT_FALSE(restored.has_value());
    EXPECT_EQ(restored.error(), worldsim::sim::WorldSnapshotError::invalid_rest_need_state);
    EXPECT_EQ(world.snapshot(), before);
}

} // namespace
