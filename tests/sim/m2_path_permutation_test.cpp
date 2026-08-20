#include "sim/acceptance_village.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

namespace {

using worldsim::sim::EntityId;
using worldsim::sim::Millimeters;
using worldsim::sim::World;
using worldsim::sim::WorldSeed;

enum class ReliefPath : std::uint8_t {
    gift,
    work,
    transfer,
};

[[nodiscard]] bool place_actor(World &world, const EntityId actor, const Millimeters x, const Millimeters z) {
    auto snapshot = world.snapshot();
    bool found = false;
    for (auto &state : snapshot.actors) {
        if (state.id != actor) {
            continue;
        }
        if (!state.spatial.has_value()) {
            return false;
        }
        state.spatial->position.x = x;
        state.spatial->position.z = z;
        found = true;
        break;
    }
    return found && world.restore(snapshot).has_value();
}

[[nodiscard]] bool consume_short_household(World &world) {
    std::optional<EntityId> rest_actor;
    for (const auto actor : world.actor_ids()) {
        if (world.actor_rest_need(actor).has_value()) {
            rest_actor = actor;
            break;
        }
    }
    if (!rest_actor.has_value()) {
        return false;
    }
    std::optional<worldsim::sim::HouseholdState> household;
    for (const auto household_id : world.household_ids()) {
        const auto state = world.household_state(household_id);
        if (!state.has_value()) {
            continue;
        }
        for (const auto member : state->members) {
            if (member == *rest_actor) {
                household = state;
                break;
            }
        }
    }
    if (!household.has_value()) {
        return false;
    }
    const auto store = world.place_state(household->store_place);
    if (!store.has_value()) {
        return false;
    }
    if (!place_actor(world, *rest_actor, store->x, store->z)) {
        return false;
    }
    return world.consume_household_grain(*rest_actor).has_value();
}

[[nodiscard]] bool run_gift(World &world, const EntityId actor, const EntityId receiving) {
    const auto carry = world.actor_grain_carry_state(actor);
    const auto household = [&]() -> std::optional<worldsim::sim::HouseholdState> {
        for (const auto household_id : world.household_ids()) {
            const auto state = world.household_state(household_id);
            if (!state.has_value()) {
                continue;
            }
            for (const auto member : state->members) {
                if (member == actor) {
                    return state;
                }
            }
        }
        return std::nullopt;
    }();
    if (!carry.has_value() || !household.has_value()) {
        return false;
    }
    const auto own_store = world.place_state(household->store_place);
    const auto receiving_state = world.household_state(receiving);
    if (!own_store.has_value() || !receiving_state.has_value()) {
        return false;
    }
    const auto receiving_store = world.place_state(receiving_state->store_place);
    if (!receiving_store.has_value()) {
        return false;
    }
    if (!place_actor(world, actor, own_store->x, own_store->z)) {
        return false;
    }
    if (!world.draw_household_grain(actor).has_value()) {
        return false;
    }
    if (!place_actor(world, actor, receiving_store->x, receiving_store->z)) {
        return false;
    }
    return world.gift_household_grain(actor, receiving).has_value();
}

[[nodiscard]] bool run_work(World &world, const EntityId actor) {
    const auto assignment = world.field_work_assignment();
    if (!assignment.has_value()) {
        return false;
    }
    const auto field = world.place_state(assignment->work_place);
    if (!field.has_value()) {
        return false;
    }
    if (!place_actor(world, actor, field->x, field->z)) {
        return false;
    }
    return world.complete_field_work(actor).has_value();
}

[[nodiscard]] bool run_transfer(World &world, const EntityId actor) {
    std::optional<worldsim::sim::HouseholdState> household;
    for (const auto household_id : world.household_ids()) {
        const auto state = world.household_state(household_id);
        if (!state.has_value()) {
            continue;
        }
        for (const auto member : state->members) {
            if (member == actor) {
                household = state;
                break;
            }
        }
    }
    if (!household.has_value()) {
        return false;
    }
    const auto store = world.place_state(household->store_place);
    if (!store.has_value()) {
        return false;
    }
    if (!place_actor(world, actor, store->x, store->z)) {
        return false;
    }
    return world.execute_household_transfer_pledge(actor).has_value();
}

TEST(AcceptanceVillagePermutation, GiftWorkAndTransferEachSucceedOnceInAnyOrder) {
    constexpr std::array<std::array<ReliefPath, 3>, 6> orders{{
        {ReliefPath::gift, ReliefPath::work, ReliefPath::transfer},
        {ReliefPath::gift, ReliefPath::transfer, ReliefPath::work},
        {ReliefPath::work, ReliefPath::gift, ReliefPath::transfer},
        {ReliefPath::work, ReliefPath::transfer, ReliefPath::gift},
        {ReliefPath::transfer, ReliefPath::gift, ReliefPath::work},
        {ReliefPath::transfer, ReliefPath::work, ReliefPath::gift},
    }};

    for (const auto &order : orders) {
        World world{WorldSeed{42}};
        const auto bindings = worldsim::sim::populate_household_resource_acceptance_village(world);
        ASSERT_TRUE(bindings.has_value());
        ASSERT_TRUE(consume_short_household(world));

        std::optional<EntityId> short_household;
        for (const auto household_id : world.household_ids()) {
            const auto state = world.household_state(household_id);
            ASSERT_TRUE(state.has_value());
            if (state->remaining_consume_budget == 0 && state->grain_stock_units == 1) {
                short_household = household_id;
            }
        }
        ASSERT_TRUE(short_household.has_value());

        for (const auto path : order) {
            bool ok = false;
            switch (path) {
            case ReliefPath::gift:
                ok = run_gift(world, bindings->controlled_actor, *short_household);
                break;
            case ReliefPath::work:
                ok = run_work(world, bindings->controlled_actor);
                break;
            case ReliefPath::transfer:
                ok = run_transfer(world, bindings->controlled_actor);
                break;
            }
            ASSERT_TRUE(ok);
        }

        const auto short_state = world.household_state(*short_household);
        ASSERT_TRUE(short_state.has_value());
        EXPECT_GE(short_state->grain_stock_units, short_state->shortage_threshold_units);
        EXPECT_FALSE(*world.household_is_short(*short_household));

        const auto assignment = world.field_work_assignment();
        ASSERT_TRUE(assignment.has_value());
        EXPECT_EQ(assignment->remaining_work_completions, 0U);

        bool pledge_cleared = false;
        for (const auto household_id : world.household_ids()) {
            const auto state = world.household_state(household_id);
            ASSERT_TRUE(state.has_value());
            if (state->id == *short_household) {
                continue;
            }
            EXPECT_EQ(state->standing_transfer_pledge.remaining_grain_units, 0);
            pledge_cleared = true;
        }
        EXPECT_TRUE(pledge_cleared);

        const auto carry = world.actor_grain_carry_state(bindings->controlled_actor);
        ASSERT_TRUE(carry.has_value());
        EXPECT_EQ(carry->carried_grain_units, 0);
    }
}

} // namespace
