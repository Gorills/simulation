#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <limits>

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
    EXPECT_EQ(saved.schema_version, 7U);

    worldsim::sim::World restored{worldsim::sim::WorldSeed{999}};
    ASSERT_TRUE(restored.restore(saved).has_value());

    const auto source_result = source.consume_household_grain(actor);
    const auto restored_result = restored.consume_household_grain(actor);
    ASSERT_TRUE(source_result.has_value());
    ASSERT_TRUE(restored_result.has_value());
    EXPECT_EQ(*restored_result, *source_result);
    EXPECT_EQ(restored.snapshot(), source.snapshot());
}

TEST(HouseholdCarry, CarryInvariantIsValidatedAtSpawnAndRestore) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{61}};
    const auto baseline = world.snapshot();

    const auto negative = world.spawn_actor(
        worldsim::sim::EntityId{1},
        worldsim::sim::ActorSpawnState{
            .grain_carry = worldsim::sim::ActorGrainCarryState{
                .carried_grain_units = -1,
                .grain_carry_capacity_units = 2,
            },
        }
    );
    ASSERT_FALSE(negative.has_value());
    EXPECT_EQ(negative.error(), worldsim::sim::WorldError::invalid_actor_grain_carry_state);
    EXPECT_EQ(world.snapshot(), baseline);

    const auto over_capacity = world.spawn_actor(
        worldsim::sim::EntityId{2},
        worldsim::sim::ActorSpawnState{
            .grain_carry = worldsim::sim::ActorGrainCarryState{
                .carried_grain_units = 3,
                .grain_carry_capacity_units = 2,
            },
        }
    );
    ASSERT_FALSE(over_capacity.has_value());
    EXPECT_EQ(
        over_capacity.error(),
        worldsim::sim::WorldError::invalid_actor_grain_carry_state
    );
    EXPECT_EQ(world.snapshot(), baseline);

    worldsim::sim::World source{worldsim::sim::WorldSeed{62}};
    const worldsim::sim::EntityId actor{5};
    ASSERT_TRUE(
        source.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{
                .grain_carry = worldsim::sim::ActorGrainCarryState{
                    .carried_grain_units = 1,
                    .grain_carry_capacity_units = 3,
                },
            }
        ).has_value()
    );
    const auto saved = source.snapshot();
    EXPECT_EQ(saved.schema_version, 7U);

    worldsim::sim::World restored{worldsim::sim::WorldSeed{999}};
    ASSERT_TRUE(restored.restore(saved).has_value());
    EXPECT_EQ(
        restored.actor_grain_carry_state(actor),
        source.actor_grain_carry_state(actor)
    );

    auto malformed = saved;
    malformed.actors.front().grain_carry.carried_grain_units = 4;
    const auto before_invalid_restore = restored.snapshot();
    const auto invalid_restore = restored.restore(malformed);
    ASSERT_FALSE(invalid_restore.has_value());
    EXPECT_EQ(
        invalid_restore.error(),
        worldsim::sim::WorldSnapshotError::invalid_actor_grain_carry_state
    );
    EXPECT_EQ(restored.snapshot(), before_invalid_restore);
}

TEST(HouseholdCarry, DrawMovesAvailableStockIntoBoundedCarryWithoutAdvancingTick) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{63}};
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};
    ASSERT_TRUE(
        world.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{
                .spatial = spatial_at(0, 0),
                .grain_carry = worldsim::sim::ActorGrainCarryState{
                    .carried_grain_units = 1,
                    .grain_carry_capacity_units = 4,
                },
            }
        ).has_value()
    );
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 2,
        }).has_value()
    );

    const auto tick_before = world.tick();
    const auto revision_before = world.revision();
    const auto before_household = world.household_state(household);
    const auto before_carry = world.actor_grain_carry_state(actor);
    ASSERT_TRUE(before_household.has_value());
    ASSERT_TRUE(before_carry.has_value());
    const auto total_before = before_household->grain_stock_units + before_carry->carried_grain_units;

    const auto result = world.draw_household_grain(actor);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->moved_grain_units, 2);
    EXPECT_EQ(result->carried_grain_units, 3);
    EXPECT_EQ(result->remaining_grain_stock_units, 0);
    EXPECT_EQ(result->tick, tick_before);
    EXPECT_EQ(result->revision, (worldsim::sim::WorldRevision{revision_before.value + 1U}));
    EXPECT_EQ(world.tick(), tick_before);

    const auto after_household = world.household_state(household);
    const auto after_carry = world.actor_grain_carry_state(actor);
    ASSERT_TRUE(after_household.has_value());
    ASSERT_TRUE(after_carry.has_value());
    EXPECT_EQ(
        after_household->grain_stock_units + after_carry->carried_grain_units,
        total_before
    );
}

TEST(HouseholdCarry, DepositMovesEntireCarryWithCheckedAddition) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{64}};
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};
    ASSERT_TRUE(
        world.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{
                .spatial = spatial_at(0, 0),
                .grain_carry = worldsim::sim::ActorGrainCarryState{
                    .carried_grain_units = 3,
                    .grain_carry_capacity_units = 3,
                },
            }
        ).has_value()
    );
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 5,
        }).has_value()
    );

    const auto tick_before = world.tick();
    const auto revision_before = world.revision();
    const auto result = world.deposit_household_grain(actor);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->deposited_grain_units, 3);
    EXPECT_EQ(result->carried_grain_units, 0);
    EXPECT_EQ(result->remaining_grain_stock_units, 8);
    EXPECT_EQ(result->tick, tick_before);
    EXPECT_EQ(result->revision, (worldsim::sim::WorldRevision{revision_before.value + 1U}));
    EXPECT_EQ(world.tick(), tick_before);

    const auto carry = world.actor_grain_carry_state(actor);
    const auto state = world.household_state(household);
    ASSERT_TRUE(carry.has_value());
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(carry->carried_grain_units, 0);
    EXPECT_EQ(state->grain_stock_units, 8);
}

TEST(HouseholdCarry, GiftMovesEntireCarryOnlyToAnotherHouseholdStore) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{65}};
    const worldsim::sim::EntityId giver{1};
    const worldsim::sim::EntityId source_store{10};
    const worldsim::sim::EntityId receiving_store{11};
    const worldsim::sim::EntityId source_household{20};
    const worldsim::sim::EntityId receiving_household{21};
    ASSERT_TRUE(
        world.spawn_actor(
            giver,
            worldsim::sim::ActorSpawnState{
                .spatial = spatial_at(0, 0),
                .grain_carry = worldsim::sim::ActorGrainCarryState{
                    .carried_grain_units = 2,
                    .grain_carry_capacity_units = 2,
                },
            }
        ).has_value()
    );
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = source_store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_place(worldsim::sim::PlaceState{
            .id = receiving_store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = source_household,
            .members = {giver},
            .store_place = source_store,
            .grain_stock_units = 5,
        }).has_value()
    );
    ASSERT_TRUE(
        world.add_household(worldsim::sim::HouseholdState{
            .id = receiving_household,
            .store_place = receiving_store,
            .grain_stock_units = 1,
            .shortage_threshold_units = 2,
        }).has_value()
    );

    const auto tick_before = world.tick();
    const auto revision_before = world.revision();
    const auto result = world.gift_household_grain(giver, receiving_household);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->gifted_grain_units, 2);
    EXPECT_EQ(result->carried_grain_units, 0);
    EXPECT_EQ(result->receiving_grain_stock_units, 3);
    EXPECT_EQ(result->tick, tick_before);
    EXPECT_EQ(result->revision, (worldsim::sim::WorldRevision{revision_before.value + 1U}));
    EXPECT_EQ(world.tick(), tick_before);

    const auto source = world.household_state(source_household);
    const auto receiving = world.household_state(receiving_household);
    const auto carry = world.actor_grain_carry_state(giver);
    ASSERT_TRUE(source.has_value());
    ASSERT_TRUE(receiving.has_value());
    ASSERT_TRUE(carry.has_value());
    EXPECT_EQ(source->grain_stock_units, 5);
    EXPECT_EQ(receiving->grain_stock_units, 3);
    EXPECT_EQ(carry->carried_grain_units, 0);
}

TEST(HouseholdCarry, FullEmptyOwnHouseholdAndOverflowRefusalsAreAtomic) {
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId store{10};
    const worldsim::sim::EntityId household{20};

    worldsim::sim::World full{worldsim::sim::WorldSeed{66}};
    ASSERT_TRUE(
        full.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{
                .spatial = spatial_at(0, 0),
                .grain_carry = worldsim::sim::ActorGrainCarryState{
                    .carried_grain_units = 2,
                    .grain_carry_capacity_units = 2,
                },
            }
        ).has_value()
    );
    ASSERT_TRUE(
        full.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        full.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 5,
        }).has_value()
    );
    const auto full_before = full.snapshot();
    const auto full_draw = full.draw_household_grain(actor);
    ASSERT_FALSE(full_draw.has_value());
    EXPECT_EQ(full_draw.error(), worldsim::sim::HouseholdDrawError::carry_full);
    EXPECT_EQ(full.snapshot(), full_before);
    const auto own_gift = full.gift_household_grain(actor, household);
    ASSERT_FALSE(own_gift.has_value());
    EXPECT_EQ(own_gift.error(), worldsim::sim::HouseholdGiftError::own_household);
    EXPECT_EQ(full.snapshot(), full_before);

    worldsim::sim::World empty{worldsim::sim::WorldSeed{67}};
    ASSERT_TRUE(
        empty.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{
                .spatial = spatial_at(0, 0),
                .grain_carry = worldsim::sim::ActorGrainCarryState{
                    .grain_carry_capacity_units = 2,
                },
            }
        ).has_value()
    );
    ASSERT_TRUE(
        empty.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        empty.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = 0,
        }).has_value()
    );
    const auto empty_before = empty.snapshot();
    const auto empty_draw = empty.draw_household_grain(actor);
    ASSERT_FALSE(empty_draw.has_value());
    EXPECT_EQ(empty_draw.error(), worldsim::sim::HouseholdDrawError::store_empty);
    EXPECT_EQ(empty.snapshot(), empty_before);
    const auto empty_deposit = empty.deposit_household_grain(actor);
    ASSERT_FALSE(empty_deposit.has_value());
    EXPECT_EQ(empty_deposit.error(), worldsim::sim::HouseholdDepositError::carry_empty);
    EXPECT_EQ(empty.snapshot(), empty_before);
    const auto empty_gift = empty.gift_household_grain(actor, household);
    ASSERT_FALSE(empty_gift.has_value());
    EXPECT_EQ(empty_gift.error(), worldsim::sim::HouseholdGiftError::carry_empty);
    EXPECT_EQ(empty.snapshot(), empty_before);

    worldsim::sim::World overflow{worldsim::sim::WorldSeed{68}};
    const worldsim::sim::EntityId target_store{11};
    const worldsim::sim::EntityId target_household{21};
    ASSERT_TRUE(
        overflow.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{
                .spatial = spatial_at(0, 0),
                .grain_carry = worldsim::sim::ActorGrainCarryState{
                    .carried_grain_units = 1,
                    .grain_carry_capacity_units = 1,
                },
            }
        ).has_value()
    );
    ASSERT_TRUE(
        overflow.add_place(worldsim::sim::PlaceState{
            .id = store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        overflow.add_place(worldsim::sim::PlaceState{
            .id = target_store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        overflow.add_household(worldsim::sim::HouseholdState{
            .id = household,
            .members = {actor},
            .store_place = store,
            .grain_stock_units = std::numeric_limits<std::int64_t>::max(),
        }).has_value()
    );
    ASSERT_TRUE(
        overflow.add_household(worldsim::sim::HouseholdState{
            .id = target_household,
            .store_place = target_store,
            .grain_stock_units = std::numeric_limits<std::int64_t>::max(),
        }).has_value()
    );
    const auto overflow_before = overflow.snapshot();
    const auto deposit_overflow = overflow.deposit_household_grain(actor);
    ASSERT_FALSE(deposit_overflow.has_value());
    EXPECT_EQ(
        deposit_overflow.error(),
        worldsim::sim::HouseholdDepositError::stock_overflow
    );
    EXPECT_EQ(overflow.snapshot(), overflow_before);
    const auto gift_overflow = overflow.gift_household_grain(actor, target_household);
    ASSERT_FALSE(gift_overflow.has_value());
    EXPECT_EQ(gift_overflow.error(), worldsim::sim::HouseholdGiftError::stock_overflow);
    EXPECT_EQ(overflow.snapshot(), overflow_before);
}

TEST(HouseholdCarrySnapshot, RestorePreservesDeterministicDrawGiftContinuation) {
    worldsim::sim::World source{worldsim::sim::WorldSeed{69}};
    const worldsim::sim::EntityId actor{1};
    const worldsim::sim::EntityId source_store{10};
    const worldsim::sim::EntityId receiving_store{11};
    const worldsim::sim::EntityId source_household{20};
    const worldsim::sim::EntityId receiving_household{21};
    ASSERT_TRUE(
        source.spawn_actor(
            actor,
            worldsim::sim::ActorSpawnState{
                .spatial = spatial_at(0, 0),
                .grain_carry = worldsim::sim::ActorGrainCarryState{
                    .grain_carry_capacity_units = 2,
                },
            }
        ).has_value()
    );
    ASSERT_TRUE(
        source.add_place(worldsim::sim::PlaceState{
            .id = source_store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        source.add_place(worldsim::sim::PlaceState{
            .id = receiving_store,
            .axis_occupancy_tolerance = worldsim::sim::Millimeters{100},
        }).has_value()
    );
    ASSERT_TRUE(
        source.add_household(worldsim::sim::HouseholdState{
            .id = source_household,
            .members = {actor},
            .store_place = source_store,
            .grain_stock_units = 5,
        }).has_value()
    );
    ASSERT_TRUE(
        source.add_household(worldsim::sim::HouseholdState{
            .id = receiving_household,
            .store_place = receiving_store,
            .grain_stock_units = 1,
            .shortage_threshold_units = 2,
        }).has_value()
    );

    const auto saved = source.snapshot();
    worldsim::sim::World restored{worldsim::sim::WorldSeed{999}};
    ASSERT_TRUE(restored.restore(saved).has_value());

    const auto source_draw = source.draw_household_grain(actor);
    const auto restored_draw = restored.draw_household_grain(actor);
    ASSERT_TRUE(source_draw.has_value());
    ASSERT_TRUE(restored_draw.has_value());
    EXPECT_EQ(*restored_draw, *source_draw);
    EXPECT_EQ(restored.snapshot(), source.snapshot());

    const auto source_gift = source.gift_household_grain(actor, receiving_household);
    const auto restored_gift = restored.gift_household_grain(actor, receiving_household);
    ASSERT_TRUE(source_gift.has_value());
    ASSERT_TRUE(restored_gift.has_value());
    EXPECT_EQ(*restored_gift, *source_gift);
    EXPECT_EQ(restored.snapshot(), source.snapshot());
}

} // namespace
