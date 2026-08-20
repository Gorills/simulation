#pragma once

#include "sim/world.hpp"

#include <expected>

namespace worldsim::sim {

// Session binding returned by the concrete first M2 content builder. World/content
// owns entity existence and ids; protocol only binds control to one existing actor.
struct HouseholdResourceAcceptanceVillageBindings final {
    EntityId controlled_actor{};

    constexpr bool operator==(const HouseholdResourceAcceptanceVillageBindings &) const = default;
};

// Code-defined bounded content for the first Household Resource Loop village.
// This is deliberately not a scenario DSL, registry or generic content system.
[[nodiscard]] std::expected<HouseholdResourceAcceptanceVillageBindings, WorldError>
populate_household_resource_acceptance_village(World &world);

} // namespace worldsim::sim
