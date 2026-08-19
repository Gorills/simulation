#include "sim/grounded_locomotion.hpp"
#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <array>

namespace {

[[nodiscard]] worldsim::sim::GroundedEnvironment flat_arena() {
    worldsim::sim::GroundedEnvironment environment;
    environment.ground.push_back(worldsim::sim::GroundPatch{
        .x = worldsim::sim::MillimeterRange{
            worldsim::sim::Millimeters{-20'000},
            worldsim::sim::Millimeters{20'000},
        },
        .z = worldsim::sim::MillimeterRange{
            worldsim::sim::Millimeters{-20'000},
            worldsim::sim::Millimeters{20'000},
        },
        .gradient_axis = worldsim::sim::PlanarAxis::x,
        .height_at_min = worldsim::sim::Millimeters{0},
        .height_at_max = worldsim::sim::Millimeters{0},
    });
    return environment;
}

[[nodiscard]] worldsim::sim::SpatialState spatial_at(const std::int64_t z = 0) {
    return worldsim::sim::SpatialState{
        .position = {.z = worldsim::sim::Millimeters{z}},
        .velocity = {},
        .epoch = worldsim::sim::SpatialEpoch{1},
    };
}

TEST(GroundedLocomotionDynamics, AcceleratesAndBrakesWithoutInstantVelocityChanges) {
    const worldsim::sim::GroundedStepConfig config{
        .ticks_per_second = 60,
        .move_speed = worldsim::sim::MillimetersPerSecond{1'000},
        .acceleration = worldsim::sim::MillimetersPerSecondSquared{6'000},
        .braking = worldsim::sim::MillimetersPerSecondSquared{8'000},
        .max_slope_rise_per_1000_run = 1192,
        .max_step_up = worldsim::sim::Millimeters{300},
        .gravity = worldsim::sim::kNonMagicalGravityBaseline,
    };
    const worldsim::sim::UprightCapsule body{
        .radius = worldsim::sim::Millimeters{380},
        .height = worldsim::sim::Millimeters{1'800},
    };
    auto state = worldsim::sim::GroundedStepState{
        .spatial = spatial_at(),
    };

    for (int tick = 0; tick < 10; ++tick) {
        const auto next = worldsim::sim::step_grounded(
            flat_arena(), body, config, state, {.x = 1000, .z = 0}
        );
        ASSERT_TRUE(next.has_value());
        state = *next;
    }

    EXPECT_EQ(state.spatial.position.x.value, 91);
    EXPECT_EQ(state.spatial.velocity.x.value, 1'000);

    for (int tick = 0; tick < 8; ++tick) {
        const auto next = worldsim::sim::step_grounded(
            flat_arena(), body, config, state, {}
        );
        ASSERT_TRUE(next.has_value());
        state = *next;
    }

    EXPECT_EQ(state.spatial.position.x.value, 146);
    EXPECT_EQ(state.spatial.velocity.x.value, 0);
    EXPECT_EQ(state.remainder.x, 0);
    EXPECT_EQ(state.remainder.velocity_x, 0);
}

TEST(GroundedLocomotionDynamics, ReversalBrakesBeforeAcceleratingTheOtherWay) {
    auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    worldsim::sim::World world{worldsim::sim::WorldSeed{71}};
    const worldsim::sim::EntityId actor{1};
    ASSERT_TRUE(world.spawn_actor(
        actor,
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());

    const std::array forward{
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = actor,
            .move = {.x = 1000, .z = 0},
            .pace = worldsim::sim::LocomotionPace::walk,
        },
    };
    for (int tick = 0; tick < 10; ++tick) {
        ASSERT_TRUE(world.advance_grounded_locomotion_tick(context, forward).has_value());
    }
    ASSERT_EQ(world.actor_spatial_state(actor)->velocity.x.value, 1'000);

    const std::array reverse{
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = actor,
            .move = {.x = -1000, .z = 0},
            .pace = worldsim::sim::LocomotionPace::walk,
        },
    };
    const auto first_reverse = world.advance_grounded_locomotion_tick(context, reverse);
    ASSERT_TRUE(first_reverse.has_value());
    EXPECT_GT(first_reverse->samples.front().spatial.velocity.x.value, 0);

    for (int tick = 0; tick < 7; ++tick) {
        ASSERT_TRUE(world.advance_grounded_locomotion_tick(context, reverse).has_value());
    }
    ASSERT_EQ(world.actor_spatial_state(actor)->velocity.x.value, 0);

    const auto accelerating_reverse = world.advance_grounded_locomotion_tick(context, reverse);
    ASSERT_TRUE(accelerating_reverse.has_value());
    EXPECT_EQ(accelerating_reverse->samples.front().spatial.velocity.x.value, -100);
}

TEST(WorldLocomotionCapability, SameActorCapabilityResolvesDifferentPaces) {
    auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    worldsim::sim::World world{worldsim::sim::WorldSeed{72}};
    for (std::int64_t id = 1; id <= 3; ++id) {
        ASSERT_TRUE(world.spawn_actor(
            worldsim::sim::EntityId{id},
            worldsim::sim::ActorSpawnState{.spatial = spatial_at((id - 1) * 2'000)}
        ).has_value());
    }

    const std::array intents{
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{1},
            .move = {.x = 1000, .z = 0},
            .pace = worldsim::sim::LocomotionPace::walk,
        },
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{2},
            .move = {.x = 1000, .z = 0},
            .pace = worldsim::sim::LocomotionPace::run,
        },
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{3},
            .move = {.x = 1000, .z = 0},
            .pace = worldsim::sim::LocomotionPace::sprint,
        },
    };

    for (int tick = 0; tick < 60; ++tick) {
        ASSERT_TRUE(world.advance_grounded_locomotion_tick(context, intents).has_value());
    }

    const auto walk = world.actor_spatial_state(worldsim::sim::EntityId{1});
    const auto run = world.actor_spatial_state(worldsim::sim::EntityId{2});
    const auto sprint = world.actor_spatial_state(worldsim::sim::EntityId{3});
    ASSERT_TRUE(walk.has_value());
    ASSERT_TRUE(run.has_value());
    ASSERT_TRUE(sprint.has_value());
    EXPECT_EQ(walk->position.x.value, 925);
    EXPECT_EQ(walk->velocity.x.value, 1'000);
    EXPECT_EQ(run->position.x.value, 2'275);
    EXPECT_EQ(run->velocity.x.value, 3'000);
    EXPECT_EQ(sprint->position.x.value, 3'045);
    EXPECT_EQ(sprint->velocity.x.value, 5'800);
}

TEST(WorldLocomotionCapability, DifferentActorsResolveTheSamePaceFromTheirOwnCapability) {
    auto context = worldsim::sim::make_flat_locomotion_acceptance_context();
    worldsim::sim::World world{worldsim::sim::WorldSeed{73}};
    ASSERT_TRUE(world.spawn_actor(
        worldsim::sim::EntityId{1},
        worldsim::sim::ActorSpawnState{.spatial = spatial_at()}
    ).has_value());
    const worldsim::sim::ActorLocomotionCapability slower{
        .walk_speed = worldsim::sim::MillimetersPerSecond{500},
        .run_speed = worldsim::sim::MillimetersPerSecond{1'500},
        .sprint_speed = worldsim::sim::MillimetersPerSecond{4'000},
        .acceleration = worldsim::sim::MillimetersPerSecondSquared{3'000},
        .braking = worldsim::sim::MillimetersPerSecondSquared{5'000},
    };
    ASSERT_TRUE(world.spawn_actor(
        worldsim::sim::EntityId{2},
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(2'000),
            .locomotion_capability = slower,
        }
    ).has_value());

    const std::array intents{
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{1},
            .move = {.x = 1000, .z = 0},
            .pace = worldsim::sim::LocomotionPace::walk,
        },
        worldsim::sim::ActorGroundedMoveIntent{
            .actor = worldsim::sim::EntityId{2},
            .move = {.x = 1000, .z = 0},
            .pace = worldsim::sim::LocomotionPace::walk,
        },
    };
    for (int tick = 0; tick < 60; ++tick) {
        ASSERT_TRUE(world.advance_grounded_locomotion_tick(context, intents).has_value());
    }

    const auto normal = world.actor_spatial_state(worldsim::sim::EntityId{1});
    const auto slow = world.actor_spatial_state(worldsim::sim::EntityId{2});
    ASSERT_TRUE(normal.has_value());
    ASSERT_TRUE(slow.has_value());
    EXPECT_EQ(normal->position.x.value, 925);
    EXPECT_EQ(normal->velocity.x.value, 1'000);
    EXPECT_EQ(slow->position.x.value, 462);
    EXPECT_EQ(slow->velocity.x.value, 500);
}

TEST(WorldLocomotionCapability, CapabilityIsSnapshotTruthAndMalformedStateIsRejectedAtomically) {
    const worldsim::sim::ActorLocomotionCapability capability{
        .walk_speed = worldsim::sim::MillimetersPerSecond{850},
        .run_speed = worldsim::sim::MillimetersPerSecond{2'700},
        .sprint_speed = worldsim::sim::MillimetersPerSecond{5'200},
        .acceleration = worldsim::sim::MillimetersPerSecondSquared{5'500},
        .braking = worldsim::sim::MillimetersPerSecondSquared{7'500},
    };
    worldsim::sim::World source{worldsim::sim::WorldSeed{74}};
    ASSERT_TRUE(source.spawn_actor(
        worldsim::sim::EntityId{1},
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(),
            .locomotion_capability = capability,
        }
    ).has_value());

    const auto saved = source.snapshot();
    ASSERT_EQ(saved.schema_version, worldsim::sim::kWorldSnapshotSchemaVersion);
    ASSERT_EQ(saved.actors.size(), 1U);
    EXPECT_EQ(saved.actors.front().locomotion_capability, capability);

    worldsim::sim::World restored{worldsim::sim::WorldSeed{999}};
    ASSERT_TRUE(restored.restore(saved).has_value());
    EXPECT_EQ(restored.actor_locomotion_capability(worldsim::sim::EntityId{1}), capability);

    auto malformed = saved;
    malformed.actors.front().locomotion_capability.run_speed =
        worldsim::sim::MillimetersPerSecond{800};
    worldsim::sim::World target{worldsim::sim::WorldSeed{75}};
    ASSERT_TRUE(target.spawn_actor(worldsim::sim::EntityId{9}).has_value());
    const auto before = target.snapshot();

    const auto rejected = target.restore(malformed);
    ASSERT_FALSE(rejected.has_value());
    EXPECT_EQ(
        rejected.error(),
        worldsim::sim::WorldSnapshotError::invalid_locomotion_capability
    );
    EXPECT_EQ(target.snapshot(), before);
}

} // namespace
