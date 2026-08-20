#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace {

[[nodiscard]] const worldsim::protocol::HouseholdResourceProjection *find_household_for_actor(
    const worldsim::protocol::VillageHouseholdResourceProjection &projection,
    const worldsim::protocol::ProtocolInteger actor
) {
    for (const auto &household : projection.households) {
        for (const auto member : household.member_actor_ids) {
            if (member == actor) {
                return &household;
            }
        }
    }
    return nullptr;
}

[[nodiscard]] const worldsim::protocol::HouseholdResourceProjection *find_shortage_household(
    const worldsim::protocol::VillageHouseholdResourceProjection &projection
) {
    for (const auto &household : projection.households) {
        if (household.status == worldsim::protocol::HouseholdResourceStatus::shortage) {
            return &household;
        }
    }
    return nullptr;
}

TEST(ProtocolInteger, MatchesSignedApplicationBoundaryWithoutUnsignedWrap) {
    using worldsim::protocol::ProtocolInteger;

    const auto zero = worldsim::protocol::to_protocol_integer(0);
    const auto maximum = worldsim::protocol::to_protocol_integer(
        worldsim::protocol::kMaxProtocolInteger
    );
    const auto too_large = worldsim::protocol::to_protocol_integer(
        worldsim::protocol::kMaxProtocolInteger + 1U
    );

    ASSERT_TRUE(zero.has_value());
    EXPECT_EQ(*zero, 0);
    ASSERT_TRUE(maximum.has_value());
    EXPECT_EQ(*maximum, std::numeric_limits<ProtocolInteger>::max());
    EXPECT_FALSE(too_large.has_value());
}

TEST(SimulationProtocol, SeedUsesTheRepresentableProtocolIntegerDomain) {
    using worldsim::protocol::ProtocolInteger;

    worldsim::protocol::Simulation maximum_seed{
        std::numeric_limits<ProtocolInteger>::max()
    };
    EXPECT_EQ(
        maximum_seed.bootstrap_controlled_actor_projection().seed,
        std::numeric_limits<ProtocolInteger>::max()
    );
    EXPECT_THROW((worldsim::protocol::Simulation{-1}), std::invalid_argument);
}

TEST(SimulationProtocol, ObservedWorldComesFromCoreActorComposition) {
    worldsim::protocol::Simulation simulation{42};

    const auto projection = simulation.observed_world_projection();

    EXPECT_EQ(projection.controlled_actor_id, 1);
    EXPECT_EQ(projection.tick, 0);
    EXPECT_EQ(projection.revision, 9);
    EXPECT_EQ(projection.protocol_version, worldsim::protocol::kProtocolVersion);
    ASSERT_EQ(projection.entities.size(), 3U);
    EXPECT_EQ(projection.entities[0].entity_id, 1);
    EXPECT_EQ(projection.entities[1].entity_id, 2);
    EXPECT_EQ(projection.entities[2].entity_id, 3);
}

TEST(SimulationProtocol, HouseholdDiscoverySharesUnchangedStartupRevision) {
    worldsim::protocol::Simulation simulation{42};

    const auto households = simulation.village_household_resource_projection();
    const auto observed = simulation.observed_world_projection();
    const auto controlled = simulation.controlled_actor_spatial_projection();

    EXPECT_EQ(households.tick, 0);
    EXPECT_EQ(households.revision, 9);
    EXPECT_EQ(households.tick, observed.tick);
    EXPECT_EQ(households.revision, observed.revision);
    EXPECT_EQ(households.tick, controlled.tick);
    EXPECT_EQ(households.revision, controlled.revision);
    EXPECT_EQ(households.protocol_version, worldsim::protocol::kProtocolVersion);
    ASSERT_EQ(households.households.size(), 2U);

    const auto *controlled_household = find_household_for_actor(
        households,
        observed.controlled_actor_id
    );
    ASSERT_NE(controlled_household, nullptr);
    EXPECT_EQ(controlled_household->grain_stock_units, 8);
    EXPECT_EQ(controlled_household->shortage_threshold_units, 2);
    EXPECT_EQ(
        controlled_household->status,
        worldsim::protocol::HouseholdResourceStatus::adequate
    );
    ASSERT_EQ(controlled_household->member_actor_ids.size(), 2U);

    const worldsim::protocol::HouseholdResourceProjection *neighbour = nullptr;
    for (const auto &household : households.households) {
        if (household.household_id != controlled_household->household_id) {
            neighbour = &household;
        }
    }
    ASSERT_NE(neighbour, nullptr);
    EXPECT_EQ(neighbour->grain_stock_units, 2);
    EXPECT_EQ(neighbour->shortage_threshold_units, 2);
    EXPECT_EQ(neighbour->status, worldsim::protocol::HouseholdResourceStatus::adequate);
    ASSERT_EQ(neighbour->member_actor_ids.size(), 1U);
    EXPECT_EQ(neighbour->member_actor_ids[0], 2);
    EXPECT_EQ(neighbour->store_x_mm, -3'000);
    EXPECT_EQ(neighbour->store_z_mm, -3'000);
    EXPECT_EQ(neighbour->store_axis_tolerance_mm, 1'000);
}

TEST(SimulationProtocol, LivingNeedProjectionKeepsM1MeaningUnderGenericCollection) {
    worldsim::protocol::Simulation simulation{42};

    const auto initial = simulation.living_need_projection();
    EXPECT_EQ(initial.entity_id, 2);
    EXPECT_EQ(initial.status, worldsim::protocol::LivingNeedStatus::traveling);
    EXPECT_EQ(initial.target_x_mm, -3'000);
    EXPECT_EQ(initial.target_z_mm, -3'000);
    EXPECT_EQ(initial.axis_arrival_tolerance_mm, 1'000);
    EXPECT_EQ(
        initial.axis_occupancy_tolerance_mm,
        1'000 + worldsim::sim::kFirstPlayableBody.radius.value
    );
    EXPECT_EQ(initial.tick, 0);
    EXPECT_EQ(initial.revision, 9);
    EXPECT_EQ(initial.protocol_version, worldsim::protocol::kProtocolVersion);

    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
    const auto advanced = simulation.advance_locomotion_tick();
    ASSERT_TRUE(advanced.has_value());

    const auto after_tick = simulation.living_need_projection();
    EXPECT_EQ(after_tick.entity_id, initial.entity_id);
    EXPECT_EQ(after_tick.status, worldsim::protocol::LivingNeedStatus::traveling);
    EXPECT_EQ(after_tick.target_x_mm, initial.target_x_mm);
    EXPECT_EQ(after_tick.target_z_mm, initial.target_z_mm);
    EXPECT_EQ(after_tick.axis_arrival_tolerance_mm, initial.axis_arrival_tolerance_mm);
    EXPECT_EQ(after_tick.axis_occupancy_tolerance_mm, initial.axis_occupancy_tolerance_mm);
    EXPECT_EQ(after_tick.tick, advanced->tick);
    EXPECT_EQ(after_tick.revision, advanced->revision);
    EXPECT_EQ(after_tick.protocol_version, worldsim::protocol::kProtocolVersion);
}

TEST(SimulationProtocol, AutonomousConsumeCommitsAfterMovementOnTheSameTick) {
    worldsim::protocol::Simulation simulation{42};
    ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());

    bool shortage_seen = false;
    worldsim::protocol::AuthoritativeMovementSampleBatch movement_at_shortage{};
    worldsim::protocol::VillageHouseholdResourceProjection resource_at_shortage{};

    for (int opportunity = 0; opportunity < 480; ++opportunity) {
        const auto movement = simulation.advance_locomotion_tick();
        ASSERT_TRUE(movement.has_value());
        const auto resources = simulation.village_household_resource_projection();

        const auto *shortage = find_shortage_household(resources);
        if (shortage == nullptr) {
            EXPECT_EQ(resources.tick, movement->tick);
            EXPECT_EQ(resources.revision, movement->revision);
            continue;
        }

        shortage_seen = true;
        movement_at_shortage = *movement;
        resource_at_shortage = resources;
        EXPECT_EQ(shortage->grain_stock_units, 1);
        EXPECT_EQ(shortage->shortage_threshold_units, 2);
        EXPECT_EQ(resources.tick, movement->tick);
        EXPECT_EQ(resources.revision, movement->revision + 1);
        break;
    }

    ASSERT_TRUE(shortage_seen);

    const auto next_movement = simulation.advance_locomotion_tick();
    ASSERT_TRUE(next_movement.has_value());
    const auto after_next = simulation.village_household_resource_projection();
    EXPECT_EQ(next_movement->tick, movement_at_shortage.tick + 1);
    EXPECT_GT(next_movement->revision, resource_at_shortage.revision);
    EXPECT_EQ(after_next.tick, next_movement->tick);
    EXPECT_EQ(after_next.revision, next_movement->revision);
    ASSERT_NE(find_shortage_household(after_next), nullptr);
}

TEST(SimulationProtocol, ControlledActorStartsWithAuthoritativeSpatialState) {
    worldsim::protocol::Simulation simulation{42};

    const auto spatial = simulation.controlled_actor_spatial_projection();

    EXPECT_EQ(spatial.entity_id, 1);
    EXPECT_EQ(spatial.x_mm, 0);
    EXPECT_EQ(spatial.y_mm, 0);
    EXPECT_EQ(spatial.z_mm, 0);
    EXPECT_EQ(spatial.velocity_x_mm_per_second, 0);
    EXPECT_EQ(spatial.velocity_y_mm_per_second, 0);
    EXPECT_EQ(spatial.velocity_z_mm_per_second, 0);
    EXPECT_EQ(spatial.spatial_epoch, 1);
    EXPECT_EQ(spatial.tick, 0);
    EXPECT_EQ(spatial.revision, 9);
    EXPECT_EQ(spatial.protocol_version, worldsim::protocol::kProtocolVersion);
}

TEST(SimulationProtocol, BootstrapMoveUpdatesRevisionWithoutChangingSpatialState) {
    worldsim::protocol::Simulation simulation{42};
    const auto spatial_before = simulation.controlled_actor_spatial_projection();

    const auto result = simulation.bootstrap_move({.dx = 1, .dy = 0});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->actor.entity_id, 1);
    EXPECT_EQ(result->actor.x, 1);
    EXPECT_EQ(result->actor.y, 0);
    EXPECT_EQ(result->actor.tick, 0);
    EXPECT_EQ(result->actor.revision, 10);
    EXPECT_EQ(result->actor.seed, 42);
    EXPECT_EQ(result->actor.protocol_version, worldsim::protocol::kProtocolVersion);

    const auto observed = simulation.observed_world_projection();
    EXPECT_EQ(observed.controlled_actor_id, result->actor.entity_id);
    EXPECT_EQ(observed.tick, result->actor.tick);
    EXPECT_EQ(observed.revision, result->actor.revision);
    ASSERT_EQ(observed.entities.size(), 3U);
    EXPECT_EQ(observed.entities[0].entity_id, result->actor.entity_id);
    EXPECT_EQ(observed.entities[1].entity_id, 2);
    EXPECT_EQ(observed.entities[2].entity_id, 3);

    const auto spatial_after = simulation.controlled_actor_spatial_projection();
    EXPECT_EQ(spatial_after.entity_id, spatial_before.entity_id);
    EXPECT_EQ(spatial_after.x_mm, spatial_before.x_mm);
    EXPECT_EQ(spatial_after.y_mm, spatial_before.y_mm);
    EXPECT_EQ(spatial_after.z_mm, spatial_before.z_mm);
    EXPECT_EQ(
        spatial_after.velocity_x_mm_per_second,
        spatial_before.velocity_x_mm_per_second
    );
    EXPECT_EQ(
        spatial_after.velocity_y_mm_per_second,
        spatial_before.velocity_y_mm_per_second
    );
    EXPECT_EQ(
        spatial_after.velocity_z_mm_per_second,
        spatial_before.velocity_z_mm_per_second
    );
    EXPECT_EQ(spatial_after.spatial_epoch, spatial_before.spatial_epoch);
    EXPECT_EQ(spatial_after.tick, 0);
    EXPECT_EQ(spatial_after.revision, 10);
    EXPECT_EQ(spatial_after.protocol_version, worldsim::protocol::kProtocolVersion);
}

TEST(SimulationProtocol, RejectsMalformedBootstrapIntentWithoutMutatingObservedWorld) {
    worldsim::protocol::Simulation simulation{7};
    const auto before = simulation.observed_world_projection();

    const auto result = simulation.bootstrap_move({.dx = 1, .dy = 1});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), worldsim::protocol::BootstrapMoveError::invalid_delta);
    EXPECT_EQ(simulation.observed_world_projection(), before);
}

} // namespace
