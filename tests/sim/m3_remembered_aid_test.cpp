#include "sim/world.hpp"

#include <gtest/gtest.h>

namespace {

[[nodiscard]] worldsim::sim::SpatialState spatial_at(
    const std::int64_t x,
    const std::int64_t z
) {
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

[[nodiscard]] bool add_store(
    worldsim::sim::World &world,
    const worldsim::sim::EntityId store
) {
    return world.add_place(worldsim::sim::PlaceState{
        .id = store,
        .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
    }).has_value();
}

[[nodiscard]] bool spawn_giver(
    worldsim::sim::World &world,
    const worldsim::sim::EntityId actor,
    const std::int64_t carried = 1,
    const std::int64_t x = 0
) {
    return world.spawn_actor(
        actor,
        worldsim::sim::ActorSpawnState{
            .spatial = spatial_at(x, 0),
            .grain_carry = worldsim::sim::ActorGrainCarryState{
                .carried_grain_units = carried,
                .grain_carry_capacity_units = carried,
            },
        }
    ).has_value();
}

} // namespace

TEST(M3RememberedAid, ShortHouseholdGiftCommitsMaterialAndSocialStateTogether) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{81}};
    const worldsim::sim::EntityId giver{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(spawn_giver(world, giver, 2));
    ASSERT_TRUE(add_store(world, store));
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .store_place = store,
            .grain_stock_units = 1,
            .shortage_threshold_units = 4,
        }).has_value()
    );

    const auto tick_before = world.tick();
    const auto revision_before = world.revision();
    const auto result = world.gift_household_grain(giver, household);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->gifted_grain_units, 2);
    EXPECT_EQ(result->receiving_grain_stock_units, 3);
    EXPECT_EQ(result->tick, tick_before);
    EXPECT_EQ(
        result->revision,
        (worldsim::sim::WorldRevision{revision_before.value + 1U})
    );
    EXPECT_EQ(world.tick(), tick_before);
    EXPECT_EQ(world.revision(), result->revision);

    const auto state = world.household_state(household);
    const auto carry = world.actor_grain_carry_state(giver);
    ASSERT_TRUE(state.has_value());
    ASSERT_TRUE(carry.has_value());
    EXPECT_EQ(state->grain_stock_units, 3);
    EXPECT_EQ(state->remembered_material_aid_actor, giver);
    EXPECT_EQ(carry->carried_grain_units, 0);

    const auto snapshot = world.snapshot();
    ASSERT_EQ(snapshot.households.size(), 1U);
    EXPECT_EQ(snapshot.households.front().remembered_material_aid_actor, giver);
}

TEST(M3RememberedAid, GiftToNonShortHouseholdDoesNotCreatePersonalAidMemory) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{82}};
    const worldsim::sim::EntityId giver{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(spawn_giver(world, giver, 1));
    ASSERT_TRUE(add_store(world, store));
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .store_place = store,
            .grain_stock_units = 4,
            .shortage_threshold_units = 4,
        }).has_value()
    );

    const auto result = world.gift_household_grain(giver, household);
    ASSERT_TRUE(result.has_value());

    const auto state = world.household_state(household);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->grain_stock_units, 5);
    EXPECT_EQ(
        state->remembered_material_aid_actor,
        worldsim::sim::EntityId{}
    );
}

TEST(M3RememberedAid, OutstandingAidSlotIsActorGenericAndDoesNotGetOverwritten) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{83}};
    const worldsim::sim::EntityId first_actor{1};
    const worldsim::sim::EntityId second_actor{2};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(spawn_giver(world, first_actor, 1));
    ASSERT_TRUE(spawn_giver(world, second_actor, 1));
    ASSERT_TRUE(add_store(world, store));
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .store_place = store,
            .grain_stock_units = 0,
            .shortage_threshold_units = 10,
        }).has_value()
    );

    const auto second_gift = world.gift_household_grain(second_actor, household);
    ASSERT_TRUE(second_gift.has_value());
    auto state = world.household_state(household);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->remembered_material_aid_actor, second_actor);
    const auto still_short = world.household_is_short(household);
    ASSERT_TRUE(still_short.has_value());
    EXPECT_TRUE(*still_short);

    const auto first_gift = world.gift_household_grain(first_actor, household);
    ASSERT_TRUE(first_gift.has_value());
    state = world.household_state(household);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->grain_stock_units, 2);
    EXPECT_EQ(state->remembered_material_aid_actor, second_actor);
}

TEST(M3RememberedAid, RefusedGiftCannotCreateRememberedAid) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{84}};
    const worldsim::sim::EntityId giver{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(spawn_giver(world, giver, 1, 101));
    ASSERT_TRUE(add_store(world, store));
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .store_place = store,
            .grain_stock_units = 0,
            .shortage_threshold_units = 2,
        }).has_value()
    );

    const auto before = world.snapshot();
    const auto result = world.gift_household_grain(giver, household);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::sim::HouseholdGiftError::outside_store);
    EXPECT_EQ(world.snapshot(), before);
}

TEST(M3RememberedAid, SnapshotRestorePreservesMemoryAndRejectsInvalidReferences) {
    worldsim::sim::World source{worldsim::sim::WorldSeed{85}};
    const worldsim::sim::EntityId giver{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(spawn_giver(source, giver, 1));
    ASSERT_TRUE(add_store(source, store));
    ASSERT_TRUE(
        source.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .store_place = store,
            .grain_stock_units = 0,
            .shortage_threshold_units = 2,
        }).has_value()
    );
    ASSERT_TRUE(source.gift_household_grain(giver, household).has_value());

    const auto saved = source.snapshot();
    EXPECT_EQ(saved.schema_version, worldsim::sim::kWorldSnapshotSchemaVersion);
    EXPECT_EQ(saved.schema_version, 10U);

    worldsim::sim::World restored{worldsim::sim::WorldSeed{999}};
    ASSERT_TRUE(restored.restore(saved).has_value());
    EXPECT_EQ(restored.snapshot(), saved);

    auto invalid_id = saved;
    invalid_id.households.front().remembered_material_aid_actor =
        worldsim::sim::EntityId{-1};
    const auto restored_before_invalid = restored.snapshot();
    const auto invalid_id_result = restored.restore(invalid_id);
    ASSERT_FALSE(invalid_id_result.has_value());
    EXPECT_EQ(
        invalid_id_result.error(),
        worldsim::sim::WorldSnapshotError::invalid_household_social_state
    );
    EXPECT_EQ(restored.snapshot(), restored_before_invalid);

    auto dangling = saved;
    dangling.households.front().remembered_material_aid_actor =
        worldsim::sim::EntityId{999};
    const auto dangling_result = restored.restore(dangling);
    ASSERT_FALSE(dangling_result.has_value());
    EXPECT_EQ(
        dangling_result.error(),
        worldsim::sim::WorldSnapshotError::unknown_remembered_aid_actor
    );
    EXPECT_EQ(restored.snapshot(), restored_before_invalid);
}

TEST(M3RememberedAid, CompositionRejectsInvalidOrUnknownRememberedActor) {
    const worldsim::sim::EntityId store{10};

    worldsim::sim::World invalid{worldsim::sim::WorldSeed{86}};
    ASSERT_TRUE(add_store(invalid, store));
    const auto invalid_before = invalid.snapshot();
    const auto invalid_result = invalid.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{20},
        .store_place = store,
        .remembered_material_aid_actor = worldsim::sim::EntityId{-1},
    });
    ASSERT_FALSE(invalid_result.has_value());
    EXPECT_EQ(
        invalid_result.error(),
        worldsim::sim::WorldError::invalid_household_social_state
    );
    EXPECT_EQ(invalid.snapshot(), invalid_before);

    worldsim::sim::World unknown{worldsim::sim::WorldSeed{87}};
    ASSERT_TRUE(add_store(unknown, store));
    const auto unknown_before = unknown.snapshot();
    const auto unknown_result = unknown.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{20},
        .store_place = store,
        .remembered_material_aid_actor = worldsim::sim::EntityId{999},
    });
    ASSERT_FALSE(unknown_result.has_value());
    EXPECT_EQ(
        unknown_result.error(),
        worldsim::sim::WorldError::unknown_remembered_aid_actor
    );
    EXPECT_EQ(unknown.snapshot(), unknown_before);
}
