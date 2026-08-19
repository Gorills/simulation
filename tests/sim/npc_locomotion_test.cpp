#include "sim/npc_locomotion.hpp"

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

TEST(NpcLocomotionDecision, ProducesDeterministicLocalIntentTowardWaypoint) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{31}};
    const worldsim::sim::EntityId npc{2};
    ASSERT_TRUE(world.spawn_actor(
        npc,
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());

    const worldsim::sim::NpcLocalWaypoint diagonal{
        .actor = npc,
        .x = worldsim::sim::Millimeters{1'000},
        .z = worldsim::sim::Millimeters{-1'000},
        .axis_arrival_tolerance = worldsim::sim::Millimeters{50},
    };

    const auto first = worldsim::sim::decide_npc_local_move_toward_waypoint(world, diagonal);
    const auto second = worldsim::sim::decide_npc_local_move_toward_waypoint(world, diagonal);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*first, *second);
    EXPECT_EQ(first->actor, npc);
    EXPECT_EQ(first->move, (worldsim::sim::PlanarMoveIntent{.x = 707, .z = -707}));
    EXPECT_TRUE(first->move.is_valid());
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{0}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{1}));
}

TEST(NpcLocomotionDecision, StopsInsideCallerSuppliedArrivalTolerance) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{32}};
    const worldsim::sim::EntityId npc{2};
    ASSERT_TRUE(world.spawn_actor(
        npc,
        worldsim::sim::ActorSpawnState{.spatial = spatial_at(1'000, 0, -2'000)}
    ).has_value());

    const auto decided = worldsim::sim::decide_npc_local_move_toward_waypoint(
        world,
        worldsim::sim::NpcLocalWaypoint{
            .actor = npc,
            .x = worldsim::sim::Millimeters{1'075},
            .z = worldsim::sim::Millimeters{-1'925},
            .axis_arrival_tolerance = worldsim::sim::Millimeters{75},
        }
    );

    ASSERT_TRUE(decided.has_value());
    EXPECT_EQ(decided->move, (worldsim::sim::PlanarMoveIntent{}));
}

TEST(NpcLocomotionDecision, RejectsInvalidOrUnavailableActorStateWithoutMutation) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{33}};
    ASSERT_TRUE(world.spawn_actor(worldsim::sim::EntityId{2}).has_value());
    const auto before = world.snapshot();

    const auto invalid = worldsim::sim::decide_npc_local_move_toward_waypoint(
        world,
        worldsim::sim::NpcLocalWaypoint{
            .actor = worldsim::sim::EntityId{2},
            .axis_arrival_tolerance = worldsim::sim::Millimeters{-1},
        }
    );
    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error(), worldsim::sim::NpcLocomotionDecisionError::invalid_waypoint);

    const auto missing_spatial = worldsim::sim::decide_npc_local_move_toward_waypoint(
        world,
        worldsim::sim::NpcLocalWaypoint{
            .actor = worldsim::sim::EntityId{2},
        }
    );
    ASSERT_FALSE(missing_spatial.has_value());
    EXPECT_EQ(
        missing_spatial.error(),
        worldsim::sim::NpcLocomotionDecisionError::missing_spatial_state
    );

    const auto unknown = worldsim::sim::decide_npc_local_move_toward_waypoint(
        world,
        worldsim::sim::NpcLocalWaypoint{
            .actor = worldsim::sim::EntityId{99},
        }
    );
    ASSERT_FALSE(unknown.has_value());
    EXPECT_EQ(unknown.error(), worldsim::sim::NpcLocomotionDecisionError::unknown_actor);
    EXPECT_EQ(world.snapshot(), before);
}

TEST(NpcLocomotionDecision, EquivalentHumanAndNpcIntentUseTheSameWorldTransition) {
    auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    worldsim::sim::World world{worldsim::sim::WorldSeed{34}};
    const worldsim::sim::EntityId human{1};
    const worldsim::sim::EntityId npc{2};

    ASSERT_TRUE(world.spawn_actor(
        human,
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());
    ASSERT_TRUE(world.spawn_actor(
        npc,
        worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0, 1'000)}
    ).has_value());

    const auto npc_decision = worldsim::sim::decide_npc_local_move_toward_waypoint(
        world,
        worldsim::sim::NpcLocalWaypoint{
            .actor = npc,
            .x = worldsim::sim::Millimeters{2'000},
            .z = worldsim::sim::Millimeters{1'000},
        }
    );
    ASSERT_TRUE(npc_decision.has_value());
    ASSERT_EQ(npc_decision->move, (worldsim::sim::PlanarMoveIntent{.x = 1000, .z = 0}));

    // Deliberately place the NPC-produced intent first. World still owns one
    // actor-generic transition and canonical EntityId-ordered samples.
    const std::array intents{
        *npc_decision,
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = human,
            .move = {.x = 1000, .z = 0},
        },
    };
    const auto advanced = world.advance_grounded_locomotion_tick(context, intents);

    ASSERT_TRUE(advanced.has_value());
    ASSERT_EQ(advanced->samples.size(), 2U);
    EXPECT_EQ(advanced->tick, (worldsim::sim::SimulationTick{1}));
    EXPECT_EQ(advanced->revision, (worldsim::sim::WorldRevision{3}));
    EXPECT_EQ(advanced->samples[0].actor, human);
    EXPECT_EQ(advanced->samples[1].actor, npc);
    EXPECT_EQ(advanced->samples[0].spatial.position.x.value, 96);
    EXPECT_EQ(advanced->samples[1].spatial.position.x.value, 96);
    EXPECT_EQ(advanced->samples[0].spatial.position.z.value, 0);
    EXPECT_EQ(advanced->samples[1].spatial.position.z.value, 1'000);
    EXPECT_EQ(
        advanced->samples[0].spatial.velocity.x,
        advanced->samples[1].spatial.velocity.x
    );
    EXPECT_EQ(
        advanced->samples[0].spatial.velocity.z,
        advanced->samples[1].spatial.velocity.z
    );
    EXPECT_EQ(advanced->samples[0].spatial.epoch, (worldsim::sim::SpatialEpoch{1}));
    EXPECT_EQ(advanced->samples[1].spatial.epoch, (worldsim::sim::SpatialEpoch{1}));
}

} // namespace
