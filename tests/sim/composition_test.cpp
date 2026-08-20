#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <array>

namespace {

TEST(WorldComposition, StoresActorsPlacesAndHouseholdsInDeterministicOrder) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{41}};
    const worldsim::sim::EntityId first_actor{30};
    const worldsim::sim::EntityId second_actor{10};
    const worldsim::sim::EntityId first_store{100};
    const worldsim::sim::EntityId second_store{200};
    const worldsim::sim::EntityId first_household{300};
    const worldsim::sim::EntityId second_household{400};

    ASSERT_TRUE(world.spawn_actor(first_actor).has_value());
    ASSERT_TRUE(world.spawn_actor(second_actor).has_value());
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = first_store,
            .x = worldsim::sim::Millimeters{-3'000},
            .z = worldsim::sim::Millimeters{-3'000},
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{150},
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = second_store,
            .x = worldsim::sim::Millimeters{3'000},
            .z = worldsim::sim::Millimeters{3'000},
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{250},
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = first_household,
            .members = {first_actor},
            .store_place = first_store,
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = second_household,
            .members = {second_actor},
            .store_place = second_store,
        }).has_value()
    );

    const std::array expected_actor_ids{first_actor, second_actor};
    const std::array expected_household_ids{first_household, second_household};
    ASSERT_EQ(world.actor_ids().size(), expected_actor_ids.size());
    ASSERT_EQ(world.household_ids().size(), expected_household_ids.size());
    for (std::size_t index = 0; index < expected_actor_ids.size(); ++index) {
        EXPECT_EQ(world.actor_ids()[index], expected_actor_ids[index]);
    }
    for (std::size_t index = 0; index < expected_household_ids.size(); ++index) {
        EXPECT_EQ(world.household_ids()[index], expected_household_ids[index]);
    }

    ASSERT_TRUE(world.contains_place(first_store));
    ASSERT_TRUE(world.contains_household(first_household));
    const worldsim::sim::PlaceState expected_first_store{
        .id = first_store,
        .x = worldsim::sim::Millimeters{-3'000},
        .z = worldsim::sim::Millimeters{-3'000},
        .axis_occupancy_tolerance = worldsim::sim::Millimeters{150},
    };
    const worldsim::sim::HouseholdState expected_first_household{
        .id = first_household,
        .members = {first_actor},
        .store_place = first_store,
    };
    EXPECT_EQ(world.place_state(first_store), expected_first_store);
    EXPECT_EQ(world.household_state(first_household), expected_first_household);
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{0}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{6}));
}

TEST(WorldComposition, RejectsCrossKindDuplicateIdsWithoutMutation) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{42}};
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(world.spawn_actor(actor).has_value());
    const auto actor_snapshot = world.snapshot();
    const auto place_over_actor = world.add_place(worldsim::sim::PlaceState{
        .id = actor,
        .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
    });
    ASSERT_FALSE(place_over_actor.has_value());
    EXPECT_EQ(place_over_actor.error(), worldsim::sim::WorldError::duplicate_entity);
    EXPECT_EQ(world.snapshot(), actor_snapshot);

    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    const auto place_snapshot = world.snapshot();
    const auto actor_over_place = world.spawn_actor(store);
    ASSERT_FALSE(actor_over_place.has_value());
    EXPECT_EQ(actor_over_place.error(), worldsim::sim::WorldError::duplicate_entity);
    EXPECT_EQ(world.snapshot(), place_snapshot);

    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
        }).has_value()
    );
    const auto household_snapshot = world.snapshot();
    const auto place_over_household = world.add_place(worldsim::sim::PlaceState{
        .id = household,
        .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
    });
    ASSERT_FALSE(place_over_household.has_value());
    EXPECT_EQ(place_over_household.error(), worldsim::sim::WorldError::duplicate_entity);
    EXPECT_EQ(world.snapshot(), household_snapshot);
}

TEST(WorldComposition, ValidatesHouseholdReferencesAndSingleMembershipAtomically) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{43}};
    const worldsim::sim::EntityId first_actor{1};
    const worldsim::sim::EntityId second_actor{2};
    const worldsim::sim::EntityId store{10};

    ASSERT_TRUE(world.spawn_actor(first_actor).has_value());
    ASSERT_TRUE(world.spawn_actor(second_actor).has_value());
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{150},
        }).has_value()
    );
    const auto baseline = world.snapshot();

    const auto unknown_member = world.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{20},
        .members = {worldsim::sim::EntityId{999}},
        .store_place = store,
    });
    ASSERT_FALSE(unknown_member.has_value());
    EXPECT_EQ(unknown_member.error(), worldsim::sim::WorldError::unknown_household_member);
    EXPECT_EQ(world.snapshot(), baseline);

    const auto duplicate_member = world.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{20},
        .members = {first_actor, first_actor},
        .store_place = store,
    });
    ASSERT_FALSE(duplicate_member.has_value());
    EXPECT_EQ(duplicate_member.error(), worldsim::sim::WorldError::duplicate_household_member);
    EXPECT_EQ(world.snapshot(), baseline);

    const auto unknown_store = world.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{20},
        .members = {first_actor},
        .store_place = worldsim::sim::EntityId{999},
    });
    ASSERT_FALSE(unknown_store.has_value());
    EXPECT_EQ(unknown_store.error(), worldsim::sim::WorldError::unknown_store_place);
    EXPECT_EQ(world.snapshot(), baseline);

    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = worldsim::sim::EntityId{20},
            .members = {first_actor},
            .store_place = store,
        }).has_value()
    );
    const auto before_reused_member = world.snapshot();
    const auto reused_member = world.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{21},
        .members = {first_actor, second_actor},
        .store_place = store,
    });
    ASSERT_FALSE(reused_member.has_value());
    EXPECT_EQ(reused_member.error(), worldsim::sim::WorldError::actor_already_in_household);
    EXPECT_EQ(world.snapshot(), before_reused_member);

    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = worldsim::sim::EntityId{22},
            .members = {second_actor},
            .store_place = store,
        }).has_value()
    );
    EXPECT_TRUE(world.contains_household(worldsim::sim::EntityId{20}));
    EXPECT_TRUE(world.contains_household(worldsim::sim::EntityId{22}));
}

TEST(WorldCompositionSnapshot, RestoresCompositionAndDerivedIdViews) {
    worldsim::sim::World source{worldsim::sim::WorldSeed{44}};
    const worldsim::sim::EntityId first_actor{7};
    const worldsim::sim::EntityId second_actor{9};
    const worldsim::sim::EntityId store{70};
    const worldsim::sim::EntityId household{90};

    ASSERT_TRUE(source.spawn_actor(first_actor).has_value());
    ASSERT_TRUE(source.spawn_actor(second_actor).has_value());
    ASSERT_TRUE(
        source.add_place(worldsim::sim::PlaceState{
            .id = store,
            .x = worldsim::sim::Millimeters{-1'500},
            .z = worldsim::sim::Millimeters{2'500},
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{175},
        }).has_value()
    );
    ASSERT_TRUE(
        source.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {first_actor, second_actor},
            .store_place = store,
        }).has_value()
    );
    source.advance_one_tick();
    const auto saved = source.snapshot();

    worldsim::sim::World restored{worldsim::sim::WorldSeed{999}};
    ASSERT_TRUE(restored.spawn_actor(worldsim::sim::EntityId{999}).has_value());
    ASSERT_TRUE(restored.restore(saved).has_value());

    EXPECT_EQ(restored.snapshot(), saved);
    ASSERT_EQ(restored.actor_ids().size(), 2U);
    EXPECT_EQ(restored.actor_ids()[0], first_actor);
    EXPECT_EQ(restored.actor_ids()[1], second_actor);
    ASSERT_EQ(restored.household_ids().size(), 1U);
    EXPECT_EQ(restored.household_ids()[0], household);
    EXPECT_EQ(restored.place_state(store), source.place_state(store));
    EXPECT_EQ(restored.household_state(household), source.household_state(household));
}

TEST(WorldCompositionSnapshot, RejectsBrokenReferencesAndCrossKindIdentityAtomically) {
    worldsim::sim::World source{worldsim::sim::WorldSeed{45}};
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(source.spawn_actor(actor).has_value());
    ASSERT_TRUE(
        source.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{150},
        }).has_value()
    );
    ASSERT_TRUE(
        source.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
        }).has_value()
    );
    const auto valid = source.snapshot();

    worldsim::sim::World target{worldsim::sim::WorldSeed{46}};
    ASSERT_TRUE(target.spawn_actor(worldsim::sim::EntityId{99}).has_value());
    const auto before = target.snapshot();

    auto cross_kind_duplicate = valid;
    cross_kind_duplicate.places.front().id = actor;
    auto result = target.restore(cross_kind_duplicate);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::sim::WorldSnapshotError::duplicate_entity);
    EXPECT_EQ(target.snapshot(), before);

    auto unknown_member = valid;
    unknown_member.households.front().members = {worldsim::sim::EntityId{999}};
    result = target.restore(unknown_member);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::sim::WorldSnapshotError::unknown_household_member);
    EXPECT_EQ(target.snapshot(), before);

    auto unknown_store = valid;
    unknown_store.households.front().store_place = worldsim::sim::EntityId{999};
    result = target.restore(unknown_store);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::sim::WorldSnapshotError::unknown_store_place);
    EXPECT_EQ(target.snapshot(), before);

    auto duplicate_member = valid;
    duplicate_member.households.front().members = {actor, actor};
    result = target.restore(duplicate_member);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::sim::WorldSnapshotError::duplicate_household_member);
    EXPECT_EQ(target.snapshot(), before);

    auto repeated_membership = valid;
    repeated_membership.households.push_back(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{21},
        .members = {actor},
        .store_place = store,
    });
    result = target.restore(repeated_membership);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::sim::WorldSnapshotError::actor_already_in_household);
    EXPECT_EQ(target.snapshot(), before);

    auto invalid_place = valid;
    invalid_place.places.front().axis_occupancy_tolerance = worldsim::sim::Millimeters{-1};
    result = target.restore(invalid_place);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::sim::WorldSnapshotError::invalid_place_state);
    EXPECT_EQ(target.snapshot(), before);
}

} // namespace
