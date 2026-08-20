#include "sim/acceptance_village.hpp"

#include <gtest/gtest.h>

#include <optional>

namespace {

[[nodiscard]] bool contains_member(
    const worldsim::sim::HouseholdState &household,
    const worldsim::sim::EntityId actor
) {
    for (const auto member : household.members) {
        if (member == actor) {
            return true;
        }
    }
    return false;
}

TEST(AcceptanceVillage, CoreContentOwnsActorsHouseholdsStoresAndControlBinding) {
    worldsim::sim::World world{worldsim::sim::WorldSeed{42}};
    const auto bindings = worldsim::sim::populate_household_resource_acceptance_village(world);
    ASSERT_TRUE(bindings.has_value());

    ASSERT_EQ(world.actor_ids().size(), 3U);
    ASSERT_EQ(world.household_ids().size(), 2U);
    EXPECT_TRUE(world.contains_actor(bindings->controlled_actor));
    EXPECT_EQ(world.tick(), (worldsim::sim::SimulationTick{0}));
    EXPECT_EQ(world.revision(), (worldsim::sim::WorldRevision{7}));

    std::optional<worldsim::sim::EntityId> rest_actor;
    for (const auto actor : world.actor_ids()) {
        if (world.actor_rest_need(actor).has_value()) {
            ASSERT_FALSE(rest_actor.has_value());
            rest_actor = actor;
        }
    }
    ASSERT_TRUE(rest_actor.has_value());
    EXPECT_NE(*rest_actor, bindings->controlled_actor);

    std::optional<worldsim::sim::HouseholdState> controlled_household;
    std::optional<worldsim::sim::HouseholdState> short_household;
    for (const auto household_id : world.household_ids()) {
        const auto household = world.household_state(household_id);
        ASSERT_TRUE(household.has_value());
        if (contains_member(*household, bindings->controlled_actor)) {
            controlled_household = household;
        }
        if (contains_member(*household, *rest_actor)) {
            short_household = household;
        }
    }

    ASSERT_TRUE(controlled_household.has_value());
    ASSERT_TRUE(short_household.has_value());
    EXPECT_NE(controlled_household->id, short_household->id);
    ASSERT_EQ(controlled_household->members.size(), 2U);
    EXPECT_EQ(controlled_household->remaining_consume_budget, 0U);
    EXPECT_FALSE(*world.household_is_short(controlled_household->id));

    EXPECT_EQ(short_household->members.size(), 1U);
    EXPECT_EQ(short_household->grain_stock_units, short_household->shortage_threshold_units);
    EXPECT_EQ(short_household->consume_amount_units, 1);
    EXPECT_EQ(short_household->remaining_consume_budget, 1U);
    EXPECT_FALSE(*world.household_is_short(short_household->id));

    const auto need = world.actor_rest_need(*rest_actor);
    const auto store = world.place_state(short_household->store_place);
    ASSERT_TRUE(need.has_value());
    ASSERT_TRUE(store.has_value());
    EXPECT_EQ(store->x, need->rest_x);
    EXPECT_EQ(store->z, need->rest_z);
    EXPECT_EQ(store->axis_occupancy_tolerance, need->axis_arrival_tolerance);
}

} // namespace
