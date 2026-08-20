#include "sim/acceptance_village.hpp"

namespace worldsim::sim {
namespace {

inline constexpr EntityId kControlledActor{1};
inline constexpr EntityId kShortHouseholdActor{2};
inline constexpr EntityId kSurplusHouseholdNpc{3};
inline constexpr EntityId kSurplusStore{10};
inline constexpr EntityId kShortStore{11};
inline constexpr EntityId kSurplusHousehold{20};
inline constexpr EntityId kShortHousehold{21};

inline constexpr Millimeters kShortStoreX{-3'000};
inline constexpr Millimeters kShortStoreZ{-3'000};
inline constexpr Millimeters kStoreTolerance{150};
inline constexpr std::int64_t kAcceptanceCarryCapacity{2};

[[nodiscard]] std::expected<void, WorldError> add_acceptance_actors(World &world) {
    auto result = world.spawn_actor(
        kControlledActor,
        ActorSpawnState{
            .spatial = SpatialState{
                .position = {},
                .velocity = {},
                .epoch = SpatialEpoch{1},
            },
            .grain_carry = ActorGrainCarryState{
                .grain_carry_capacity_units = kAcceptanceCarryCapacity,
            },
        }
    );
    if (!result.has_value()) {
        return result;
    }

    result = world.spawn_actor(
        kShortHouseholdActor,
        ActorSpawnState{
            .spatial = SpatialState{
                .position = {
                    .x = Millimeters{3'000},
                    .y = Millimeters{0},
                    .z = kShortStoreZ,
                },
                .velocity = {},
                .epoch = SpatialEpoch{1},
            },
            .rest_need = RestNeedState{
                .rest_x = kShortStoreX,
                .rest_z = kShortStoreZ,
                .axis_arrival_tolerance = kStoreTolerance,
            },
        }
    );
    if (!result.has_value()) {
        return result;
    }

    return world.spawn_actor(
        kSurplusHouseholdNpc,
        ActorSpawnState{
            .spatial = SpatialState{
                .position = {
                    .x = Millimeters{2'000},
                    .y = Millimeters{0},
                    .z = Millimeters{2'000},
                },
                .velocity = {},
                .epoch = SpatialEpoch{1},
            },
            .grain_carry = ActorGrainCarryState{
                .grain_carry_capacity_units = kAcceptanceCarryCapacity,
            },
        }
    );
}

[[nodiscard]] std::expected<void, WorldError> add_acceptance_stores(World &world) {
    auto result = world.add_place(PlaceState{
        .id = kSurplusStore,
        .x = Millimeters{0},
        .z = Millimeters{0},
        .axis_occupancy_tolerance = kStoreTolerance,
    });
    if (!result.has_value()) {
        return result;
    }

    return world.add_place(PlaceState{
        .id = kShortStore,
        .x = kShortStoreX,
        .z = kShortStoreZ,
        .axis_occupancy_tolerance = kStoreTolerance,
    });
}

[[nodiscard]] std::expected<void, WorldError> add_acceptance_households(World &world) {
    auto result = world.add_household(HouseholdState{
        .id = kSurplusHousehold,
        .members = {kControlledActor, kSurplusHouseholdNpc},
        .store_place = kSurplusStore,
        .grain_stock_units = 8,
        .shortage_threshold_units = 2,
        .consume_amount_units = 1,
        .remaining_consume_budget = 0,
    });
    if (!result.has_value()) {
        return result;
    }

    return world.add_household(HouseholdState{
        .id = kShortHousehold,
        .members = {kShortHouseholdActor},
        .store_place = kShortStore,
        .grain_stock_units = 2,
        .shortage_threshold_units = 2,
        .consume_amount_units = 1,
        .remaining_consume_budget = 1,
    });
}

} // namespace

std::expected<HouseholdResourceAcceptanceVillageBindings, WorldError>
populate_household_resource_acceptance_village(World &world) {
    auto result = add_acceptance_actors(world);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    result = add_acceptance_stores(world);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    result = add_acceptance_households(world);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    return HouseholdResourceAcceptanceVillageBindings{
        .controlled_actor = kControlledActor,
    };
}

} // namespace worldsim::sim
