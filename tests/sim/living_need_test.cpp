#include "sim/living_need.hpp"

#include <gtest/gtest.h>

#include <array>
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
    EXPECT_FALSE(decision->blocked_by_other_actor);
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
    EXPECT_FALSE(decision->blocked_by_other_actor);
    EXPECT_EQ(decision->movement.actor, npc);
    EXPECT_EQ(decision->movement.move, (worldsim::sim::PlanarMoveIntent{}));
    EXPECT_EQ(decision->movement.pace, worldsim::sim::LocomotionPace::walk);
}

TEST(LivingNeed, OtherActorOccupancyBlocksRestUntilSharedLocomotionClearsThePlace) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{46}};
    const worldsim::sim::EntityId controlled_actor{1};
    const worldsim::sim::EntityId npc{2};

    ASSERT_TRUE(world.spawn_actor(
        controlled_actor,
        worldsim::sim::ActorSpawnState{.spatial = spatial_at(-3'000, -3'000)}
    ).has_value());
    ASSERT_TRUE(world.spawn_actor(
        npc,
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(-2'896, -3'000),
            .rest_need = rest_need(),
        }
    ).has_value());

    const auto before_decision = world.snapshot();
    const auto blocked = worldsim::sim::decide_npc_rest_need(world, npc);
    ASSERT_TRUE(blocked.has_value());
    EXPECT_FALSE(blocked->satisfied);
    EXPECT_TRUE(blocked->blocked_by_other_actor);
    EXPECT_EQ(blocked->movement.move, (worldsim::sim::PlanarMoveIntent{}));
    EXPECT_EQ(world.snapshot(), before_decision);

    const auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    bool became_satisfied = false;
    for (int tick = 0; tick < 30; ++tick) {
        const std::array intents{
            worldsim::sim::ActorGroundedMoveIntent{
                .actor = controlled_actor,
                .move = worldsim::sim::PlanarMoveIntent{.x = 1000, .z = 0},
                .pace = worldsim::sim::LocomotionPace::run,
            },
            blocked->movement,
        };
        ASSERT_TRUE(world.advance_grounded_locomotion_tick(context, intents).has_value());

        const auto next = worldsim::sim::decide_npc_rest_need(world, npc);
        ASSERT_TRUE(next.has_value());
        if (next->satisfied) {
            EXPECT_FALSE(next->blocked_by_other_actor);
            EXPECT_EQ(next->movement.move, (worldsim::sim::PlanarMoveIntent{}));
            became_satisfied = true;
            break;
        }
        EXPECT_TRUE(next->blocked_by_other_actor);
    }

    EXPECT_TRUE(became_satisfied);
    const auto controlled_spatial = world.actor_spatial_state(controlled_actor);
    ASSERT_TRUE(controlled_spatial.has_value());
    EXPECT_GT(controlled_spatial->position.x.value, -2'850);
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
