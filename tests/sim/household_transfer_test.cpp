#include "sim/world.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>

namespace {

using worldsim::sim::ActorGrainCarryState;
using worldsim::sim::ActorSpawnState;
using worldsim::sim::EntityId;
using worldsim::sim::HouseholdState;
using worldsim::sim::HouseholdTransferError;
using worldsim::sim::Millimeters;
using worldsim::sim::PlaceState;
using worldsim::sim::SpatialEpoch;
using worldsim::sim::SpatialState;
using worldsim::sim::StandingTransferPledge;
using worldsim::sim::World;
using worldsim::sim::WorldError;
using worldsim::sim::WorldSeed;
using worldsim::sim::WorldSnapshotError;

inline constexpr EntityId kActorOne{1};
inline constexpr EntityId kActorTwo{2};
inline constexpr EntityId kOutsider{3};
inline constexpr EntityId kSourceStore{10};
inline constexpr EntityId kDestinationStore{11};
inline constexpr EntityId kSourceHousehold{20};
inline constexpr EntityId kDestinationHousehold{21};
inline constexpr Millimeters kTolerance{150};

[[nodiscard]] ActorSpawnState exact_actor_at(
    const Millimeters x,
    const Millimeters z,
    const std::int64_t carry_capacity = 0
) {
    return ActorSpawnState{
        .spatial = SpatialState{
            .position = {.x = x, .y = Millimeters{0}, .z = z},
            .velocity = {},
            .epoch = SpatialEpoch{1},
        },
        .grain_carry = ActorGrainCarryState{
            .grain_carry_capacity_units = carry_capacity,
        },
    };
}

[[nodiscard]] std::optional<World> make_transfer_world(
    const std::int64_t source_stock,
    const std::int64_t remaining_pledge,
    const std::int64_t destination_stock = 1,
    const ActorSpawnState first = exact_actor_at(Millimeters{0}, Millimeters{0}),
    const ActorSpawnState second = exact_actor_at(Millimeters{0}, Millimeters{0})
) {
    World world{WorldSeed{42}};
    if (!world.spawn_actor(kActorOne, first).has_value()) {
        return std::nullopt;
    }
    if (!world.spawn_actor(kActorTwo, second).has_value()) {
        return std::nullopt;
    }
    if (!world.add_place(PlaceState{
            .id = kSourceStore,
            .axis_occupancy_tolerance = kTolerance,
        }).has_value()) {
        return std::nullopt;
    }
    if (!world.add_place(PlaceState{
            .id = kDestinationStore,
            .x = Millimeters{1'000},
            .z = Millimeters{1'000},
            .axis_occupancy_tolerance = kTolerance,
        }).has_value()) {
        return std::nullopt;
    }
    if (!world.add_household(HouseholdState{
            .id = kDestinationHousehold,
            .store_place = kDestinationStore,
            .grain_stock_units = destination_stock,
            .shortage_threshold_units = 2,
            .consume_amount_units = 1,
        }).has_value()) {
        return std::nullopt;
    }
    if (!world.add_household(HouseholdState{
            .id = kSourceHousehold,
            .members = {kActorOne, kActorTwo},
            .store_place = kSourceStore,
            .grain_stock_units = source_stock,
            .shortage_threshold_units = 2,
            .consume_amount_units = 1,
            .standing_transfer_pledge = StandingTransferPledge{
                .destination_household = kDestinationHousehold,
                .remaining_grain_units = remaining_pledge,
            },
        }).has_value()) {
        return std::nullopt;
    }
    return world;
}

TEST(HouseholdTransfer, MovesEntireRemainingPledgeWithoutAdvancingSimulationTime) {
    auto world_value = make_transfer_world(8, 4, 1);
    ASSERT_TRUE(world_value.has_value());
    auto &world = *world_value;
    const auto before = world.snapshot();

    const auto transferred = world.execute_household_transfer_pledge(kActorOne);

    ASSERT_TRUE(transferred.has_value());
    EXPECT_EQ(transferred->actor, kActorOne);
    EXPECT_EQ(transferred->source_household, kSourceHousehold);
    EXPECT_EQ(transferred->destination_household, kDestinationHousehold);
    EXPECT_EQ(transferred->transferred_grain_units, 4);
    EXPECT_EQ(transferred->source_grain_stock_units, 4);
    EXPECT_EQ(transferred->destination_grain_stock_units, 5);
    EXPECT_EQ(transferred->remaining_pledge_grain_units, 0);
    EXPECT_EQ(transferred->tick, before.tick);
    EXPECT_EQ(transferred->revision.value, before.revision.value + 1);

    const auto source = world.household_state(kSourceHousehold);
    const auto destination = world.household_state(kDestinationHousehold);
    ASSERT_TRUE(source.has_value());
    ASSERT_TRUE(destination.has_value());
    EXPECT_EQ(source->grain_stock_units, 4);
    EXPECT_EQ(source->standing_transfer_pledge.remaining_grain_units, 0);
    EXPECT_EQ(source->standing_transfer_pledge.destination_household, kDestinationHousehold);
    EXPECT_EQ(destination->grain_stock_units, 5);
    EXPECT_TRUE(*world.household_is_short(kDestinationHousehold) == false);
}

TEST(HouseholdTransfer, RefusalsAreAtomicForZeroPledgeOccupancyStockAndOverflow) {
    auto zero_value = make_transfer_world(8, 0);
    ASSERT_TRUE(zero_value.has_value());
    auto &zero = *zero_value;
    const auto zero_before = zero.snapshot();
    const auto zero_result = zero.execute_household_transfer_pledge(kActorOne);
    ASSERT_FALSE(zero_result.has_value());
    EXPECT_EQ(zero_result.error(), HouseholdTransferError::pledge_zero);
    EXPECT_EQ(zero.snapshot(), zero_before);

    auto missing_value = make_transfer_world(8, 4, 1, ActorSpawnState{});
    ASSERT_TRUE(missing_value.has_value());
    auto &missing = *missing_value;
    const auto missing_before = missing.snapshot();
    const auto missing_result = missing.execute_household_transfer_pledge(kActorOne);
    ASSERT_FALSE(missing_result.has_value());
    EXPECT_EQ(missing_result.error(), HouseholdTransferError::missing_spatial_state);
    EXPECT_EQ(missing.snapshot(), missing_before);

    auto outside_value = make_transfer_world(
        8,
        4,
        1,
        exact_actor_at(Millimeters{1'000}, Millimeters{1'000})
    );
    ASSERT_TRUE(outside_value.has_value());
    auto &outside = *outside_value;
    const auto outside_before = outside.snapshot();
    const auto outside_result = outside.execute_household_transfer_pledge(kActorOne);
    ASSERT_FALSE(outside_result.has_value());
    EXPECT_EQ(outside_result.error(), HouseholdTransferError::outside_store);
    EXPECT_EQ(outside.snapshot(), outside_before);

    auto short_stock = make_transfer_world(3, 4);
    ASSERT_TRUE(short_stock.has_value());
    auto &insufficient = *short_stock;
    const auto insufficient_before = insufficient.snapshot();
    const auto insufficient_result = insufficient.execute_household_transfer_pledge(kActorOne);
    ASSERT_FALSE(insufficient_result.has_value());
    EXPECT_EQ(insufficient_result.error(), HouseholdTransferError::insufficient_stock);
    EXPECT_EQ(insufficient.snapshot(), insufficient_before);

    auto overflow_value = make_transfer_world(
        4,
        4,
        std::numeric_limits<std::int64_t>::max()
    );
    ASSERT_TRUE(overflow_value.has_value());
    auto &overflow = *overflow_value;
    const auto overflow_before = overflow.snapshot();
    const auto overflow_result = overflow.execute_household_transfer_pledge(kActorOne);
    ASSERT_FALSE(overflow_result.has_value());
    EXPECT_EQ(overflow_result.error(), HouseholdTransferError::stock_overflow);
    EXPECT_EQ(overflow.snapshot(), overflow_before);
}

TEST(HouseholdTransfer, OccupancyAloneDoesNotAuthorizeANonMember) {
    auto world_value = make_transfer_world(8, 4);
    ASSERT_TRUE(world_value.has_value());
    auto &world = *world_value;
    ASSERT_TRUE(world.spawn_actor(
        kOutsider,
        exact_actor_at(Millimeters{0}, Millimeters{0})
    ).has_value());
    const auto before = world.snapshot();

    const auto outsider = world.execute_household_transfer_pledge(kOutsider);
    ASSERT_FALSE(outsider.has_value());
    EXPECT_EQ(outsider.error(), HouseholdTransferError::actor_without_household);
    EXPECT_EQ(world.snapshot(), before);
}

TEST(HouseholdTransfer, CompositionRejectsSelfAndUnknownPledgeDestinations) {
    World world{WorldSeed{7}};
    ASSERT_TRUE(world.spawn_actor(kActorOne, exact_actor_at(Millimeters{0}, Millimeters{0})).has_value());
    ASSERT_TRUE(world.add_place(PlaceState{
        .id = kSourceStore,
        .axis_occupancy_tolerance = kTolerance,
    }).has_value());
    ASSERT_TRUE(world.add_place(PlaceState{
        .id = kDestinationStore,
        .axis_occupancy_tolerance = kTolerance,
    }).has_value());
    ASSERT_TRUE(world.add_household(HouseholdState{
        .id = kDestinationHousehold,
        .store_place = kDestinationStore,
        .grain_stock_units = 1,
        .shortage_threshold_units = 1,
        .consume_amount_units = 1,
    }).has_value());
    const auto before = world.snapshot();

    const auto unknown = world.add_household(HouseholdState{
        .id = kSourceHousehold,
        .members = {kActorOne},
        .store_place = kSourceStore,
        .grain_stock_units = 8,
        .standing_transfer_pledge = StandingTransferPledge{
            .destination_household = EntityId{99},
            .remaining_grain_units = 4,
        },
    });
    ASSERT_FALSE(unknown.has_value());
    EXPECT_EQ(unknown.error(), WorldError::unknown_pledge_destination_household);
    EXPECT_EQ(world.snapshot(), before);

    const auto self = world.add_household(HouseholdState{
        .id = kSourceHousehold,
        .members = {kActorOne},
        .store_place = kSourceStore,
        .grain_stock_units = 8,
        .standing_transfer_pledge = StandingTransferPledge{
            .destination_household = kSourceHousehold,
            .remaining_grain_units = 4,
        },
    });
    ASSERT_FALSE(self.has_value());
    EXPECT_EQ(self.error(), WorldError::pledge_destination_is_self);
    EXPECT_EQ(world.snapshot(), before);

    const auto remaining_without_destination = world.add_household(HouseholdState{
        .id = kSourceHousehold,
        .members = {kActorOne},
        .store_place = kSourceStore,
        .grain_stock_units = 8,
        .standing_transfer_pledge = StandingTransferPledge{
            .remaining_grain_units = 4,
        },
    });
    ASSERT_FALSE(remaining_without_destination.has_value());
    EXPECT_EQ(
        remaining_without_destination.error(),
        WorldError::invalid_standing_transfer_pledge
    );
    EXPECT_EQ(world.snapshot(), before);
}

TEST(HouseholdTransferSnapshot, RestorePreservesDeterministicTransferContinuation) {
    auto first_value = make_transfer_world(8, 4, 1);
    ASSERT_TRUE(first_value.has_value());
    auto &first = *first_value;

    World second{WorldSeed{999}};
    ASSERT_TRUE(second.restore(first.snapshot()).has_value());
    EXPECT_EQ(second.snapshot(), first.snapshot());
    EXPECT_EQ(first.snapshot().schema_version, worldsim::sim::kWorldSnapshotSchemaVersion);

    const auto first_transfer = first.execute_household_transfer_pledge(kActorOne);
    const auto second_transfer = second.execute_household_transfer_pledge(kActorOne);
    ASSERT_TRUE(first_transfer.has_value());
    ASSERT_TRUE(second_transfer.has_value());
    EXPECT_EQ(*first_transfer, *second_transfer);
    EXPECT_EQ(first.snapshot(), second.snapshot());

    const auto exhausted_before = first.snapshot();
    const auto exhausted = first.execute_household_transfer_pledge(kActorTwo);
    ASSERT_FALSE(exhausted.has_value());
    EXPECT_EQ(exhausted.error(), HouseholdTransferError::pledge_zero);
    EXPECT_EQ(first.snapshot(), exhausted_before);

    World target{WorldSeed{1}};
    ASSERT_TRUE(target.spawn_actor(EntityId{90}).has_value());
    const auto target_before = target.snapshot();
    auto bad_destination = exhausted_before;
    ASSERT_FALSE(bad_destination.households.empty());
    bad_destination.households.back().standing_transfer_pledge.destination_household =
        EntityId{98};
    const auto bad_restore = target.restore(bad_destination);
    ASSERT_FALSE(bad_restore.has_value());
    EXPECT_EQ(bad_restore.error(), WorldSnapshotError::unknown_pledge_destination_household);
    EXPECT_EQ(target.snapshot(), target_before);
}

TEST(HouseholdTransferParity, EquivalentActorsUseTheSameWorldRule) {
    auto first_world = make_transfer_world(8, 4, 1);
    auto second_world = make_transfer_world(8, 4, 1);
    ASSERT_TRUE(first_world.has_value());
    ASSERT_TRUE(second_world.has_value());

    const auto first = first_world->execute_household_transfer_pledge(kActorOne);
    const auto second = second_world->execute_household_transfer_pledge(kActorTwo);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->transferred_grain_units, second->transferred_grain_units);
    EXPECT_EQ(first->source_household, second->source_household);
    EXPECT_EQ(first->destination_household, second->destination_household);
    EXPECT_EQ(first->source_grain_stock_units, second->source_grain_stock_units);
    EXPECT_EQ(first->destination_grain_stock_units, second->destination_grain_stock_units);
    EXPECT_EQ(first->remaining_pledge_grain_units, 0);
    EXPECT_EQ(second->remaining_pledge_grain_units, 0);
    EXPECT_EQ(first->tick, second->tick);
}

} // namespace
