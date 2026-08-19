#include "sim/grounded_locomotion.hpp"

#include <gtest/gtest.h>

namespace worldsim::sim {
namespace {

constexpr UprightCapsule kPlayableBody{
    .radius = Millimeters{380},
    .height = Millimeters{1800},
};

constexpr GroundedStepConfig kPlayableStep{
    .ticks_per_second = 60,
    .move_speed = MillimetersPerSecond{5800},
    .max_slope_rise_per_1000_run = 1192,
};

[[nodiscard]] GroundedEnvironment flat_arena(const bool with_wall) {
    GroundedEnvironment environment;
    environment.ground.push_back(GroundPatch{
        .x = MillimeterRange{Millimeters{-10'000}, Millimeters{10'000}},
        .z = MillimeterRange{Millimeters{-10'000}, Millimeters{10'000}},
        .gradient_axis = PlanarAxis::x,
        .height_at_min = Millimeters{0},
        .height_at_max = Millimeters{0},
    });
    if (with_wall) {
        environment.barriers.push_back(VerticalBarrier{
            .normal_axis = PlanarAxis::x,
            .coordinate = Millimeters{2'000},
            .vertical = MillimeterRange{Millimeters{0}, Millimeters{3'000}},
        });
    }
    return environment;
}

[[nodiscard]] GroundedEnvironment walkable_slope_arena() {
    GroundedEnvironment environment;
    environment.ground.push_back(GroundPatch{
        .x = MillimeterRange{Millimeters{0}, Millimeters{10'000}},
        .z = MillimeterRange{Millimeters{-10'000}, Millimeters{10'000}},
        .gradient_axis = PlanarAxis::x,
        .height_at_min = Millimeters{0},
        .height_at_max = Millimeters{10'000},
    });
    return environment;
}

[[nodiscard]] GroundedEnvironment steep_slope_gate() {
    GroundedEnvironment environment;
    environment.ground.push_back(GroundPatch{
        .x = MillimeterRange{Millimeters{-10'000}, Millimeters{0}},
        .z = MillimeterRange{Millimeters{-10'000}, Millimeters{10'000}},
        .gradient_axis = PlanarAxis::x,
        .height_at_min = Millimeters{0},
        .height_at_max = Millimeters{0},
    });
    environment.ground.push_back(GroundPatch{
        .x = MillimeterRange{Millimeters{0}, Millimeters{10'000}},
        .z = MillimeterRange{Millimeters{-10'000}, Millimeters{10'000}},
        .gradient_axis = PlanarAxis::x,
        .height_at_min = Millimeters{0},
        .height_at_max = Millimeters{15'000},
    });
    return environment;
}

[[nodiscard]] GroundedStepState initial_state() {
    return GroundedStepState{
        .spatial = SpatialState{
            .position = SpatialPosition{},
            .velocity = SpatialVelocity{},
            .epoch = SpatialEpoch{1},
        },
    };
}

[[nodiscard]] GroundedStepState replay(
    const GroundedEnvironment &environment,
    const PlanarMoveIntent intent,
    const int ticks,
    GroundedStepState state = initial_state()
) {
    for (int tick = 0; tick < ticks; ++tick) {
        auto next = step_grounded(environment, kPlayableBody, kPlayableStep, state, intent);
        EXPECT_TRUE(next.has_value());
        if (!next.has_value()) {
            return state;
        }
        state = *next;
    }
    return state;
}

TEST(GroundedLocomotion, IntegratesCurrentPlayableSpeedExactlyOverOneSecond) {
    const auto state = replay(flat_arena(false), PlanarMoveIntent{.x = 1000, .z = 0}, 60);

    EXPECT_EQ(state.spatial.position.x.value, 5800);
    EXPECT_EQ(state.spatial.position.y.value, 0);
    EXPECT_EQ(state.spatial.position.z.value, 0);
    EXPECT_EQ(state.spatial.velocity.x.value, 5800);
    EXPECT_EQ(state.remainder.x, 0);
    EXPECT_EQ(state.spatial.epoch.value, 1U);
}

TEST(GroundedLocomotion, HeadOnWallStopsAtCapsuleRadiusWithoutAccumulatingPenetration) {
    auto state = replay(flat_arena(true), PlanarMoveIntent{.x = 1000, .z = 0}, 60);

    EXPECT_EQ(state.spatial.position.x.value, 1620);
    EXPECT_EQ(state.spatial.velocity.x.value, 0);
    EXPECT_EQ(state.remainder.x, 0);

    const auto environment = flat_arena(true);
    for (int tick = 0; tick < 120; ++tick) {
        auto next = step_grounded(
            environment,
            kPlayableBody,
            kPlayableStep,
            state,
            PlanarMoveIntent{.x = 1000, .z = 0}
        );
        ASSERT_TRUE(next.has_value());
        state = *next;
    }

    EXPECT_EQ(state.spatial.position.x.value, 1620);
    EXPECT_EQ(state.spatial.velocity.x.value, 0);
}

TEST(GroundedLocomotion, ObliqueIntentKeepsTangentialMotionWhileWallBlocksNormalMotion) {
    const auto state = replay(flat_arena(true), PlanarMoveIntent{.x = 707, .z = 707}, 60);

    EXPECT_EQ(state.spatial.position.x.value, 1620);
    EXPECT_GT(state.spatial.position.z.value, 0);
    EXPECT_EQ(state.spatial.velocity.x.value, 0);
    EXPECT_GT(state.spatial.velocity.z.value, 0);
}

TEST(GroundedLocomotion, ZeroIntentStopsWithoutPositionJitter) {
    auto state = replay(flat_arena(false), PlanarMoveIntent{.x = 1000, .z = 0}, 7);
    const auto before = state.spatial.position;
    const auto environment = flat_arena(false);

    ASSERT_NE(state.remainder.x, 0);

    for (int tick = 0; tick < 120; ++tick) {
        auto next = step_grounded(environment, kPlayableBody, kPlayableStep, state, PlanarMoveIntent{});
        ASSERT_TRUE(next.has_value());
        state = *next;
    }

    EXPECT_EQ(state.spatial.position, before);
    EXPECT_EQ(state.spatial.velocity, SpatialVelocity{});
    EXPECT_EQ(state.remainder, GroundedIntegrationRemainder{});
}

TEST(GroundedLocomotion, IdenticalInputReplayProducesIdenticalState) {
    const auto environment = flat_arena(true);
    const auto first = replay(environment, PlanarMoveIntent{.x = 707, .z = 707}, 90);
    const auto second = replay(environment, PlanarMoveIntent{.x = 707, .z = 707}, 90);

    EXPECT_EQ(first, second);
}

TEST(GroundedLocomotion, RejectsIntentOutsideUnitCircle) {
    const auto result = step_grounded(
        flat_arena(false),
        kPlayableBody,
        kPlayableStep,
        initial_state(),
        PlanarMoveIntent{.x = 1000, .z = 1000}
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), GroundedStepError::invalid_intent);
}

TEST(GroundedLocomotion, TraversesWalkableSlopeAndDerivesAuthoritativeHeight) {
    const auto environment = walkable_slope_arena();
    const auto state = replay(environment, PlanarMoveIntent{.x = 1000, .z = 0}, 60);

    EXPECT_EQ(state.spatial.position.x.value, 5800);
    EXPECT_EQ(state.spatial.position.y.value, 5800);
    EXPECT_EQ(state.spatial.position.z.value, 0);
    EXPECT_EQ(state.spatial.velocity.x.value, 5800);
    EXPECT_GT(state.spatial.velocity.y.value, 0);
    EXPECT_EQ(state.spatial.epoch.value, 1U);
}

TEST(GroundedLocomotion, WalkableSlopeDoesNotCreepWithZeroIntent) {
    const auto environment = walkable_slope_arena();
    auto state = replay(environment, PlanarMoveIntent{.x = 1000, .z = 0}, 30);
    const auto before = state.spatial.position;

    state = replay(environment, PlanarMoveIntent{}, 120, state);

    EXPECT_EQ(state.spatial.position, before);
    EXPECT_EQ(state.spatial.velocity, SpatialVelocity{});
    EXPECT_EQ(state.remainder, GroundedIntegrationRemainder{});
}

TEST(GroundedLocomotion, TooSteepSlopeBlocksGradientMotion) {
    const auto state = replay(steep_slope_gate(), PlanarMoveIntent{.x = 1000, .z = 0}, 120);

    EXPECT_EQ(state.spatial.position.x.value, 0);
    EXPECT_EQ(state.spatial.position.y.value, 0);
    EXPECT_EQ(state.spatial.velocity.x.value, 0);
    EXPECT_EQ(state.remainder.x, 0);
}

TEST(GroundedLocomotion, TooSteepSlopePreservesTangentialMotion) {
    const auto state = replay(steep_slope_gate(), PlanarMoveIntent{.x = 707, .z = 707}, 60);

    EXPECT_EQ(state.spatial.position.x.value, 0);
    EXPECT_GT(state.spatial.position.z.value, 0);
    EXPECT_EQ(state.spatial.velocity.x.value, 0);
    EXPECT_GT(state.spatial.velocity.z.value, 0);
}

TEST(GroundedLocomotion, IdenticalSlopeReplayProducesIdenticalState) {
    const auto environment = walkable_slope_arena();
    const auto first = replay(environment, PlanarMoveIntent{.x = 1000, .z = 0}, 60);
    const auto second = replay(environment, PlanarMoveIntent{.x = 1000, .z = 0}, 60);

    EXPECT_EQ(first, second);
}

} // namespace
} // namespace worldsim::sim
