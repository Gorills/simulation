#pragma once

#include "sim/world.hpp"

#include <vector>

namespace worldsim::protocol {

// Minimum real application-composition owner admitted by ADR 0009 when M2
// expands beyond the original two named actors. This is concrete first-village
// content, not a generic scenario DSL, ECS registry or dynamic population system.
struct ScenarioActor final {
    sim::EntityId id{};
    sim::ActorSpawnState initial{};
    bool observed{};
};

struct ScenarioHousehold final {
    sim::HouseholdId id{};
    sim::HouseholdSpawnState initial{};
};

struct ApplicationScenario final {
    sim::EntityId controlled_actor{};
    std::vector<ScenarioActor> actors{};
    std::vector<ScenarioHousehold> households{};
};

[[nodiscard]] inline ApplicationScenario make_first_village_scenario() {
    return ApplicationScenario{
        .controlled_actor = sim::EntityId{1},
        .actors = {
            ScenarioActor{
                .id = sim::EntityId{1},
                .initial = sim::ActorSpawnState{
                    .spatial = sim::SpatialState{
                        .position = {},
                        .velocity = {},
                        .epoch = sim::SpatialEpoch{1},
                    },
                },
                .observed = true,
            },
            ScenarioActor{
                .id = sim::EntityId{2},
                .initial = sim::ActorSpawnState{
                    .spatial = sim::SpatialState{
                        .position = {
                            .x = sim::Millimeters{3'000},
                            .y = sim::Millimeters{0},
                            .z = sim::Millimeters{-3'000},
                        },
                        .velocity = {},
                        .epoch = sim::SpatialEpoch{1},
                    },
                    .rest_need = sim::RestNeedState{
                        .rest_x = sim::Millimeters{-3'000},
                        .rest_z = sim::Millimeters{-3'000},
                        .axis_arrival_tolerance = sim::Millimeters{150},
                    },
                },
                .observed = true,
            },
        },
        .households = {
            ScenarioHousehold{
                .id = sim::HouseholdId{1},
                .initial = sim::HouseholdSpawnState{
                    .grain = sim::HouseholdGrainState{
                        .stored = sim::GrainGrams{900},
                        .shortage_below = sim::GrainGrams{1'000},
                    },
                },
            },
        },
    };
}

} // namespace worldsim::protocol
