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

TEST(HouseholdResource, ConsumeMakesDerivedShortageWithoutAdvancingSimulationTime) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{51}};
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(
        world.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(100, 100)}
        ).has_value()
    );
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = store,
            .x = worldsim::sim::Millimeters{0},
            .z = worldsim::sim::Millimeters{0},
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 12,
            .shortage_threshold_units = 10,
            .consume_amount_units = 3,
            .remaining_consume_budget = 2,
        }).has_value()
    );

    const auto initial_shortage = world.household_is_short(household);
    ASSERT_TRUE(initial_shortage.has_value());
    EXPECT_FALSE(*initial_shortage);
    EXPECT_TRUE(world.is_actor_inside_place(actor, store));

    const auto tick_before = world.tick();
    const auto revision_before = world.revision();
    const auto consumed = world.consume_household_grain(actor);

    ASSERT_TRUE(consumed.has_value());
    EXPECT_EQ(consumed->actor, actor);
    EXPECT_EQ(consumed->household, household);
    EXPECT_EQ(consumed->consumed_grain_units, 3);
    EXPECT_EQ(consumed->remaining_grain_stock_units, 9);
    EXPECT_EQ(consumed->remaining_consume_budget, 1U);
    EXPECT_TRUE(consumed->shortage);
    EXPECT_EQ(consumed->tick, tick_before);
    EXPECT_EQ(
        consumed->revision,
        (worldsim::sim::WorldRevision{revision_before.value + 1U})
    );
    EXPECT_EQ(world.tick(), tick_before);
    EXPECT_EQ(world.revision(), consumed->revision);

    const auto state = world.household_state(household);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->grain_stock_units, 9);
    EXPECT_EQ(state->remaining_consume_budget, 1U);
    const auto shortage = world.household_is_short(household);
    ASSERT_TRUE(shortage.has_value());
    EXPECT_TRUE(*shortage);
}

TEST(HouseholdResource, PlaceGateUsesExactArrivalToleranceWithoutBodyRadius) {
    const worldsim::sim::EntityId store{10};

    worldsim::sim::World on_boundary{worldsim::sim::WorldSeed{52}};
    ASSERT_TRUE(
        on_boundary.spawn_actor(
            worldsim::sim::EntityId{1},
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(100, -100)}
        ).has_value()
    );
    ASSERT_TRUE(
        on_boundary.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    EXPECT_TRUE(on_boundary.is_actor_inside_place(worldsim::sim::EntityId{1}, store));

    worldsim::sim::World outside{worldsim::sim::WorldSeed{53}};
    ASSERT_TRUE(
        outside.spawn_actor(
            worldsim::sim::EntityId{1},
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(101, 0)}
        ).has_value()
    );
    ASSERT_TRUE(
        outside.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    EXPECT_FALSE(outside.is_actor_inside_place(worldsim::sim::EntityId{1}, store));
}

TEST(HouseholdResource, ConsumeRefusalsLeaveWorldUnchanged) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{54}};
    const worldsim::sim::EntityId member{1};
    const worldsim::sim::EntityId outsider{2};
    const worldsim::sim::EntityId no_spatial{3};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(
        world.spawn_actor(
            member,
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(101, 0)}
        ).has_value()
    );
    ASSERT_TRUE(
        world.spawn_actor(
            outsider,
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0)}
        ).has_value()
    );
    ASSERT_TRUE(world.spawn_actor(no_spatial).has_value());
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {member, no_spatial},
            .store_place = store,
            .grain_stock_units = 5,
            .shortage_threshold_units = 4,
            .consume_amount_units = 2,
            .remaining_consume_budget = 1,
        }).has_value()
    );
    const auto baseline = world.snapshot();

    const auto invalid = world.consume_household_grain(worldsim::sim::EntityId{0});
    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error(), worldsim::sim::HouseholdConsumeError::invalid_entity_id);
    EXPECT_EQ(world.snapshot(), baseline);

    const auto unknown = world.consume_household_grain(worldsim::sim::EntityId{999});
    ASSERT_FALSE(unknown.has_value());
    EXPECT_EQ(unknown.error(), worldsim::sim::HouseholdConsumeError::unknown_actor);
    EXPECT_EQ(world.snapshot(), baseline);

    const auto no_household = world.consume_household_grain(outsider);
    ASSERT_FALSE(no_household.has_value());
    EXPECT_EQ(
        no_household.error(),
        worldsim::sim::HouseholdConsumeError::actor_without_household
    );
    EXPECT_EQ(world.snapshot(), baseline);

    const auto missing_spatial = world.consume_household_grain(no_spatial);
    ASSERT_FALSE(missing_spatial.has_value());
    EXPECT_EQ(
        missing_spatial.error(),
        worldsim::sim::HouseholdConsumeError::missing_spatial_state
    );
    EXPECT_EQ(world.snapshot(), baseline);

    const auto outside_store = world.consume_household_grain(member);
    ASSERT_FALSE(outside_store.has_value());
    EXPECT_EQ(outside_store.error(), worldsim::sim::HouseholdConsumeError::outside_store);
    EXPECT_EQ(world.snapshot(), baseline);
}

TEST(HouseholdResource, BudgetAndStockRefusalsLeaveWorldUnchanged) {
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    worldsim::sim::World exhausted{worldsim::sim::WorldSeed{55}};
    ASSERT_TRUE(
        exhausted.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0)}
        ).has_value()
    );
    ASSERT_TRUE(
        exhausted.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        exhausted.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 5,
            .shortage_threshold_units = 3,
            .consume_amount_units = 1,
            .remaining_consume_budget = 0,
        }).has_value()
    );
    const auto exhausted_before = exhausted.snapshot();
    const auto exhausted_result = exhausted.consume_household_grain(actor);
    ASSERT_FALSE(exhausted_result.has_value());
    EXPECT_EQ(
        exhausted_result.error(),
        worldsim::sim::HouseholdConsumeError::consume_budget_exhausted
    );
    EXPECT_EQ(exhausted.snapshot(), exhausted_before);

    worldsim::sim::World insufficient{worldsim::sim::WorldSeed{56}};
    ASSERT_TRUE(
        insufficient.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0)}
        ).has_value()
    );
    ASSERT_TRUE(
        insufficient.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        insufficient.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 1,
            .shortage_threshold_units = 1,
            .consume_amount_units = 2,
            .remaining_consume_budget = 1,
        }).has_value()
    );
    const auto insufficient_before = insufficient.snapshot();
    const auto insufficient_result = insufficient.consume_household_grain(actor);
    ASSERT_FALSE(insufficient_result.has_value());
    EXPECT_EQ(
        insufficient_result.error(),
        worldsim::sim::HouseholdConsumeError::insufficient_stock
    );
    EXPECT_EQ(insufficient.snapshot(), insufficient_before);
}

TEST(HouseholdResource, InvalidResourceContentIsRejectedAtCompositionAndRestore) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{57}};
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};

    ASSERT_TRUE(
        world.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0)}
        ).has_value()
    );
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    const auto baseline = world.snapshot();

    const auto negative_stock = world.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{20},
        .members = {actor},
        .store_place = store,
        .grain_stock_units = -1,
        .shortage_threshold_units = 0,
        .consume_amount_units = 1,
        .remaining_consume_budget = 1,
    });
    ASSERT_FALSE(negative_stock.has_value());
    EXPECT_EQ(negative_stock.error(), worldsim::sim::WorldError::invalid_household_state);
    EXPECT_EQ(world.snapshot(), baseline);

    const auto negative_threshold = world.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{20},
        .members = {actor},
        .store_place = store,
        .grain_stock_units = 0,
        .shortage_threshold_units = -1,
        .consume_amount_units = 1,
        .remaining_consume_budget = 1,
    });
    ASSERT_FALSE(negative_threshold.has_value());
    EXPECT_EQ(
        negative_threshold.error(),
        worldsim::sim::WorldError::invalid_household_state
    );
    EXPECT_EQ(world.snapshot(), baseline);

    const auto zero_amount = world.add_household(worldsim::sim::HouseholdState{
        .id = worldsim::sim::EntityId{20},
        .members = {actor},
        .store_place = store,
        .grain_stock_units = 0,
        .shortage_threshold_units = 0,
        .consume_amount_units = 0,
        .remaining_consume_budget = 1,
    });
    ASSERT_FALSE(zero_amount.has_value());
    EXPECT_EQ(zero_amount.error(), worldsim::sim::WorldError::invalid_household_state);
    EXPECT_EQ(world.snapshot(), baseline);

    worldsim::sim::World source{worldsim::sim::WorldSeed{58}};
    ASSERT_TRUE(
        source.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0)}
        ).has_value()
    );
    ASSERT_TRUE(
        source.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        source.add_household(worldsim::sim::HouseholdState{
            .id = worldsim::sim::EntityId{20},
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 5,
            .shortage_threshold_units = 4,
            .consume_amount_units = 1,
            .remaining_consume_budget = 1,
        }).has_value()
    );
    auto malformed = source.snapshot();
    malformed.households.front().consume_amount_units = 0;

    worldsim::sim::World target{worldsim::sim::WorldSeed{59}};
    ASSERT_TRUE(target.spawn_actor(worldsim::sim::EntityId{99}).has_value());
    const auto target_before = target.snapshot();
    const auto restored = target.restore(malformed);
    ASSERT_FALSE(restored.has_value());
    EXPECT_EQ(restored.error(), worldsim::sim::WorldSnapshotError::invalid_household_state);
    EXPECT_EQ(target.snapshot(), target_before);
}

TEST(HouseholdResourceSnapshot, RestorePreservesDeterministicConsumeContinuation) {
    worldsim::sim::World source{worldsim::sim::WorldSeed{60}};
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    ASSERT_TRUE(
        source.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{.spatial = spatial_at(0, 0)}
        ).has_value()
    );
    ASSERT_TRUE(
        source.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        source.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 7,
            .shortage_threshold_units = 6,
            .consume_amount_units = 2,
            .remaining_consume_budget = 2,
        }).has_value()
    );
    const auto saved = source.snapshot();
    EXPECT_EQ(saved.schema_version, 6U);

    worldsim::sim::World restored{worldsim::sim::WorldSeed{999}};
    ASSERT_TRUE(restored.restore(saved).has_value());

    const auto source_result = source.consume_household_grain(actor);
    const auto restored_result = restored.consume_household_grain(actor);
    ASSERT_TRUE(source_result.has_value());
    ASSERT_TRUE(restored_result.has_value());
    EXPECT_EQ(*restored_result, *source_result);
    EXPECT_EQ(restored.snapshot(), source.snapshot());
}

} // namespace
