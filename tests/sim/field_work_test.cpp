#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>

namespace {

using worldsim::sim::ActorSpawnState;
using worldsim::sim::EntityId;
using worldsim::sim::FieldWorkAssignmentState;
using worldsim::sim::FieldWorkError;
using worldsim::sim::HouseholdState;
using worldsim::sim::Millimeters;
using worldsim::sim::PlaceState;
using worldsim::sim::SpatialEpoch;
using worldsim::sim::SpatialState;
using worldsim::sim::World;
using worldsim::sim::WorldError;
using worldsim::sim::WorldSeed;
using worldsim::sim::WorldSnapshotError;

inline constexpr EntityId kActorOne{1};
inline constexpr EntityId kActorTwo{2};
inline constexpr EntityId kStore{10};
inline constexpr EntityId kField{11};
inline constexpr EntityId kDestination{20};
inline constexpr Millimeters kFieldX{1'000};
inline constexpr Millimeters kFieldZ{-2'000};
inline constexpr Millimeters kTolerance{150};

[[nodiscard]] ActorSpawnState exact_actor_at(
    const Millimeters x,
    const Millimeters z
) {
    return ActorSpawnState{
        .spatial = SpatialState{
            .position = {.x = x, .y = Millimeters{0}, .z = z},
            .velocity = {},
            .epoch = SpatialEpoch{1},
        },
    };
}

[[nodiscard]] std::optional<World> make_work_world(
    const std::int64_t stock,
    const std::int64_t yield,
    const std::uint32_t remaining,
    const ActorSpawnState first = exact_actor_at(kFieldX, kFieldZ),
    const ActorSpawnState second = exact_actor_at(kFieldX, kFieldZ)
) {
    World world{WorldSeed{42}};
    if (!world.spawn_actor(kActorOne, first).has_value()) {
        return std::nullopt;
    }
    if (!world.spawn_actor(kActorTwo, second).has_value()) {
        return std::nullopt;
    }
    if (!world.add_place(PlaceState{
            .id = kStore,
            .x = Millimeters{0},
            .z = Millimeters{0},
            .axis_occupancy_tolerance = kTolerance,
        }).has_value()) {
        return std::nullopt;
    }
    if (!world.add_place(PlaceState{
            .id = kField,
            .x = kFieldX,
            .z = kFieldZ,
            .axis_occupancy_tolerance = kTolerance,
        }).has_value()) {
        return std::nullopt;
    }
    if (!world.add_household(HouseholdState{
            .id = kDestination,
            .store_place = kStore,
            .grain_stock_units = stock,
            .shortage_threshold_units = 2,
            .consume_amount_units = 1,
        }).has_value()) {
        return std::nullopt;
    }
    if (!world.add_field_work_assignment(FieldWorkAssignmentState{
            .work_place = kField,
            .destination_household = kDestination,
            .yield_grain_units = yield,
            .remaining_work_completions = remaining,
        }).has_value()) {
        return std::nullopt;
    }
    return world;
}

TEST(FieldWork, ProducesFixtureYieldWithoutAdvancingSimulationTime) {
    auto world_value = make_work_world(5, 3, 2);
    ASSERT_TRUE(world_value.has_value());
    auto &world = *world_value;
    const auto before = world.snapshot();

    const auto worked = world.complete_field_work(kActorOne);

    ASSERT_TRUE(worked.has_value());
    EXPECT_EQ(worked->actor, kActorOne);
    EXPECT_EQ(worked->work_place, kField);
    EXPECT_EQ(worked->destination_household, kDestination);
    EXPECT_EQ(worked->produced_grain_units, 3);
    EXPECT_EQ(worked->destination_grain_stock_units, 8);
    EXPECT_EQ(worked->remaining_work_completions, 1U);
    EXPECT_EQ(worked->tick, before.tick);
    EXPECT_EQ(worked->revision.value, before.revision.value + 1);

    const auto destination = world.household_state(kDestination);
    const auto assignment = world.field_work_assignment();
    ASSERT_TRUE(destination.has_value());
    ASSERT_TRUE(assignment.has_value());
    EXPECT_EQ(destination->grain_stock_units, 8);
    EXPECT_EQ(assignment->remaining_work_completions, 1U);
}

TEST(FieldWork, RefusalsAreAtomicForMissingSpatialOutsideExhaustedAndOverflow) {
    auto missing_value = make_work_world(5, 3, 1, ActorSpawnState{});
    ASSERT_TRUE(missing_value.has_value());
    auto &missing = *missing_value;
    const auto missing_before = missing.snapshot();
    const auto missing_result = missing.complete_field_work(kActorOne);
    ASSERT_FALSE(missing_result.has_value());
    EXPECT_EQ(missing_result.error(), FieldWorkError::missing_spatial_state);
    EXPECT_EQ(missing.snapshot(), missing_before);

    auto outside_value = make_work_world(
        5,
        3,
        1,
        exact_actor_at(Millimeters{0}, Millimeters{0})
    );
    ASSERT_TRUE(outside_value.has_value());
    auto &outside = *outside_value;
    const auto outside_before = outside.snapshot();
    const auto outside_result = outside.complete_field_work(kActorOne);
    ASSERT_FALSE(outside_result.has_value());
    EXPECT_EQ(outside_result.error(), FieldWorkError::outside_field);
    EXPECT_EQ(outside.snapshot(), outside_before);

    auto exhausted_value = make_work_world(5, 3, 0);
    ASSERT_TRUE(exhausted_value.has_value());
    auto &exhausted = *exhausted_value;
    const auto exhausted_before = exhausted.snapshot();
    const auto exhausted_result = exhausted.complete_field_work(kActorOne);
    ASSERT_FALSE(exhausted_result.has_value());
    EXPECT_EQ(exhausted_result.error(), FieldWorkError::work_exhausted);
    EXPECT_EQ(exhausted.snapshot(), exhausted_before);

    auto overflow_value = make_work_world(
        std::numeric_limits<std::int64_t>::max(),
        1,
        1
    );
    ASSERT_TRUE(overflow_value.has_value());
    auto &overflow = *overflow_value;
    const auto overflow_before = overflow.snapshot();
    const auto overflow_result = overflow.complete_field_work(kActorOne);
    ASSERT_FALSE(overflow_result.has_value());
    EXPECT_EQ(overflow_result.error(), FieldWorkError::stock_overflow);
    EXPECT_EQ(overflow.snapshot(), overflow_before);
}

TEST(FieldWork, AssignmentCompositionAndRestoreValidateReferencesAtomically) {
    World world{WorldSeed{7}};
    ASSERT_TRUE(world.spawn_actor(kActorOne, exact_actor_at(kFieldX, kFieldZ)).has_value());
    ASSERT_TRUE(world.add_place(PlaceState{
        .id = kStore,
        .axis_occupancy_tolerance = kTolerance,
    }).has_value());
    ASSERT_TRUE(world.add_place(PlaceState{
        .id = kField,
        .x = kFieldX,
        .z = kFieldZ,
        .axis_occupancy_tolerance = kTolerance,
    }).has_value());
    ASSERT_TRUE(world.add_household(HouseholdState{
        .id = kDestination,
        .store_place = kStore,
        .grain_stock_units = 1,
        .shortage_threshold_units = 1,
        .consume_amount_units = 1,
    }).has_value());

    const auto before_assignment = world.snapshot();
    const auto invalid = world.add_field_work_assignment(FieldWorkAssignmentState{
        .work_place = kField,
        .destination_household = kDestination,
        .yield_grain_units = 0,
        .remaining_work_completions = 1,
    });
    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error(), WorldError::invalid_field_work_assignment_state);
    EXPECT_EQ(world.snapshot(), before_assignment);

    const auto unknown_place = world.add_field_work_assignment(FieldWorkAssignmentState{
        .work_place = EntityId{99},
        .destination_household = kDestination,
        .yield_grain_units = 1,
        .remaining_work_completions = 1,
    });
    ASSERT_FALSE(unknown_place.has_value());
    EXPECT_EQ(unknown_place.error(), WorldError::unknown_work_place);
    EXPECT_EQ(world.snapshot(), before_assignment);

    const auto unknown_destination = world.add_field_work_assignment(FieldWorkAssignmentState{
        .work_place = kField,
        .destination_household = EntityId{99},
        .yield_grain_units = 1,
        .remaining_work_completions = 1,
    });
    ASSERT_FALSE(unknown_destination.has_value());
    EXPECT_EQ(
        unknown_destination.error(),
        WorldError::unknown_work_destination_household
    );
    EXPECT_EQ(world.snapshot(), before_assignment);

    ASSERT_TRUE(world.add_field_work_assignment(FieldWorkAssignmentState{
        .work_place = kField,
        .destination_household = kDestination,
        .yield_grain_units = 1,
        .remaining_work_completions = 1,
    }).has_value());
    const auto valid_snapshot = world.snapshot();

    const auto duplicate = world.add_field_work_assignment(FieldWorkAssignmentState{
        .work_place = kField,
        .destination_household = kDestination,
        .yield_grain_units = 2,
        .remaining_work_completions = 2,
    });
    ASSERT_FALSE(duplicate.has_value());
    EXPECT_EQ(duplicate.error(), WorldError::field_work_assignment_already_exists);
    EXPECT_EQ(world.snapshot(), valid_snapshot);

    World target{WorldSeed{99}};
    ASSERT_TRUE(target.spawn_actor(EntityId{90}).has_value());
    const auto target_before = target.snapshot();

    auto bad_place = valid_snapshot;
    ASSERT_TRUE(bad_place.field_work_assignment.has_value());
    bad_place.field_work_assignment->work_place = EntityId{98};
    const auto bad_place_restore = target.restore(bad_place);
    ASSERT_FALSE(bad_place_restore.has_value());
    EXPECT_EQ(bad_place_restore.error(), WorldSnapshotError::unknown_work_place);
    EXPECT_EQ(target.snapshot(), target_before);

    auto bad_destination = valid_snapshot;
    ASSERT_TRUE(bad_destination.field_work_assignment.has_value());
    bad_destination.field_work_assignment->destination_household = EntityId{98};
    const auto bad_destination_restore = target.restore(bad_destination);
    ASSERT_FALSE(bad_destination_restore.has_value());
    EXPECT_EQ(
        bad_destination_restore.error(),
        WorldSnapshotError::unknown_work_destination_household
    );
    EXPECT_EQ(target.snapshot(), target_before);

    auto bad_yield = valid_snapshot;
    ASSERT_TRUE(bad_yield.field_work_assignment.has_value());
    bad_yield.field_work_assignment->yield_grain_units = 0;
    const auto bad_yield_restore = target.restore(bad_yield);
    ASSERT_FALSE(bad_yield_restore.has_value());
    EXPECT_EQ(
        bad_yield_restore.error(),
        WorldSnapshotError::invalid_field_work_assignment_state
    );
    EXPECT_EQ(target.snapshot(), target_before);
}

TEST(FieldWorkSnapshot, RestorePreservesDeterministicWorkContinuation) {
    auto first_value = make_work_world(4, 2, 2);
    ASSERT_TRUE(first_value.has_value());
    auto &first = *first_value;

    World second{WorldSeed{999}};
    ASSERT_TRUE(second.restore(first.snapshot()).has_value());
    EXPECT_EQ(second.snapshot(), first.snapshot());

    const auto first_work = first.complete_field_work(kActorOne);
    const auto second_work = second.complete_field_work(kActorOne);
    ASSERT_TRUE(first_work.has_value());
    ASSERT_TRUE(second_work.has_value());
    EXPECT_EQ(*first_work, *second_work);
    EXPECT_EQ(first.snapshot(), second.snapshot());

    const auto first_second_work = first.complete_field_work(kActorTwo);
    const auto second_second_work = second.complete_field_work(kActorTwo);
    ASSERT_TRUE(first_second_work.has_value());
    ASSERT_TRUE(second_second_work.has_value());
    EXPECT_EQ(*first_second_work, *second_second_work);
    EXPECT_EQ(first.snapshot(), second.snapshot());

    const auto exhausted_before = first.snapshot();
    const auto exhausted = first.complete_field_work(kActorOne);
    ASSERT_FALSE(exhausted.has_value());
    EXPECT_EQ(exhausted.error(), FieldWorkError::work_exhausted);
    EXPECT_EQ(first.snapshot(), exhausted_before);
}

TEST(FieldWorkParity, EquivalentActorsUseTheSameWorldRule) {
    auto world_value = make_work_world(1, 2, 2);
    ASSERT_TRUE(world_value.has_value());
    auto &world = *world_value;

    const auto first = world.complete_field_work(kActorOne);
    const auto second = world.complete_field_work(kActorTwo);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->produced_grain_units, 2);
    EXPECT_EQ(second->produced_grain_units, 2);
    EXPECT_EQ(first->work_place, second->work_place);
    EXPECT_EQ(first->destination_household, second->destination_household);
    EXPECT_EQ(first->remaining_work_completions, 1U);
    EXPECT_EQ(second->remaining_work_completions, 0U);
    EXPECT_EQ(second->tick, first->tick);
    EXPECT_EQ(second->revision.value, first->revision.value + 1);
}

} // namespace
