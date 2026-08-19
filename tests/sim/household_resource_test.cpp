#include "sim/world.hpp"

#include <gtest/gtest.h>

namespace {

[[nodiscard]] worldsim::sim::HouseholdSpawnState household_with_grain(
    const std::int64_t stored,
    const std::int64_t shortage_below
) {
    return worldsim::sim::HouseholdSpawnState{
        .grain = worldsim::sim::HouseholdGrainState{
            .stored = worldsim::sim::GrainGrams{stored},
            .shortage_below = worldsim::sim::GrainGrams{shortage_below},
        },
    };
}

TEST(HouseholdResource, GrainConsumptionChangesRevisionWithoutAdvancingTimeAndCanCreateShortage) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{71}};
    const worldsim::sim::HouseholdId household{1};
    ASSERT_TRUE(world.spawn_household(household, household_with_grain(1'500, 1'000)).has_value());

    const auto before = world.household_grain_state(household);
    ASSERT_TRUE(before.has_value());
    EXPECT_EQ(before->stored, (worldsim::sim::GrainGrams{1'500}));
    EXPECT_FALSE(before->is_shortage());
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{0}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{1}));

    ASSERT_TRUE(world.consume_household_grain(household, worldsim::sim::GrainGrams{600}).has_value());

    const auto after = world.household_grain_state(household);
    ASSERT_TRUE(after.has_value());
    EXPECT_EQ(after->stored, (worldsim::sim::GrainGrams{900}));
    EXPECT_TRUE(after->is_shortage());
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{0}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{2}));
}

TEST(HouseholdResource, InvalidOrInsufficientConsumptionDoesNotPartiallyMutateStock) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{72}};
    const worldsim::sim::HouseholdId household{1};
    ASSERT_TRUE(world.spawn_household(household, household_with_grain(500, 1'000)).has_value());
    const auto before = world.snapshot();

    const auto zero = world.consume_household_grain(household, worldsim::sim::GrainGrams{0});
    ASSERT_FALSE(zero.has_value());
    EXPECT_EQ(zero.error(), worldsim::sim::HouseholdResourceError::invalid_amount);
    EXPECT_EQ(world.snapshot(), before);

    const auto too_much = world.consume_household_grain(household, worldsim::sim::GrainGrams{501});
    ASSERT_FALSE(too_much.has_value());
    EXPECT_EQ(too_much.error(), worldsim::sim::HouseholdResourceError::insufficient_stock);
    EXPECT_EQ(world.snapshot(), before);

    const auto unknown = world.consume_household_grain(
        worldsim::sim::HouseholdId{2},
        worldsim::sim::GrainGrams{1}
    );
    ASSERT_FALSE(unknown.has_value());
    EXPECT_EQ(unknown.error(), worldsim::sim::HouseholdResourceError::unknown_household);
    EXPECT_EQ(world.snapshot(), before);
}

TEST(HouseholdResource, GrainStateSurvivesSnapshotRestoreAndRejectsInvalidSnapshot) {
    worldsim::sim::World source{worldsim::sim::WorldSeed{73}};
    const worldsim::sim::HouseholdId household{1};
    ASSERT_TRUE(source.spawn_household(household, household_with_grain(900, 1'000)).has_value());
    const auto snapshot = source.snapshot();
    EXPECT_EQ(snapshot.schema_version, worldsim::sim::kWorldSnapshotSchemaVersion);
    ASSERT_EQ(snapshot.households.size(), 1U);

    worldsim::sim::World restored{worldsim::sim::WorldSeed{99}};
    ASSERT_TRUE(restored.restore(snapshot).has_value());
    EXPECT_EQ(restored.snapshot(), snapshot);
    ASSERT_TRUE(restored.household_grain_state(household).has_value());
    EXPECT_TRUE(restored.household_grain_state(household)->is_shortage());

    auto invalid_snapshot = snapshot;
    invalid_snapshot.households.front().grain.stored = worldsim::sim::GrainGrams{-1};
    const auto before_invalid_restore = restored.snapshot();
    const auto invalid_restore = restored.restore(invalid_snapshot);
    ASSERT_FALSE(invalid_restore.has_value());
    EXPECT_EQ(
        invalid_restore.error(),
        worldsim::sim::WorldSnapshotError::invalid_household_grain_state
    );
    EXPECT_EQ(restored.snapshot(), before_invalid_restore);
}

TEST(HouseholdResource, HouseholdIdentityAndStateValidationAreAuthoritative) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{74}};

    const auto invalid_id = world.spawn_household(
        worldsim::sim::HouseholdId{0},
        household_with_grain(100, 100)
    );
    ASSERT_FALSE(invalid_id.has_value());
    EXPECT_EQ(invalid_id.error(), worldsim::sim::HouseholdResourceError::invalid_household_id);

    const auto invalid_state = world.spawn_household(
        worldsim::sim::HouseholdId{1},
        household_with_grain(-1, 100)
    );
    ASSERT_FALSE(invalid_state.has_value());
    EXPECT_EQ(invalid_state.error(), worldsim::sim::HouseholdResourceError::invalid_grain_state);
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{0}));

    ASSERT_TRUE(world.spawn_household(
        worldsim::sim::HouseholdId{1},
        household_with_grain(100, 100)
    ).has_value());
    const auto duplicate = world.spawn_household(
        worldsim::sim::HouseholdId{1},
        household_with_grain(200, 100)
    );
    ASSERT_FALSE(duplicate.has_value());
    EXPECT_EQ(duplicate.error(), worldsim::sim::HouseholdResourceError::duplicate_household);
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{1}));
}

} // namespace
