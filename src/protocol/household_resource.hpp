#pragma once

#include "protocol/integer.hpp"
#include "protocol/version.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace worldsim::protocol {

enum class HouseholdResourceStatus : std::uint8_t {
    adequate,
    shortage,
};

struct HouseholdResourceProjection final {
    ProtocolInteger household_id{};
    std::vector<ProtocolInteger> member_actor_ids{};
    ProtocolInteger store_place_id{};
    ProtocolInteger store_x_mm{};
    ProtocolInteger store_z_mm{};
    ProtocolInteger store_axis_tolerance_mm{};
    ProtocolInteger grain_stock_units{};
    ProtocolInteger shortage_threshold_units{};
    HouseholdResourceStatus status{HouseholdResourceStatus::adequate};

    bool operator==(const HouseholdResourceProjection &) const = default;
};

// Village-scoped discovery/read for the bounded M2 acceptance composition.
// Households/places do not enter ObservedWorldProjection because that projection
// is actor-scoped and drives actor presentation materialization.
struct VillageHouseholdResourceProjection final {
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};
    std::vector<HouseholdResourceProjection> households{};

    bool operator==(const VillageHouseholdResourceProjection &) const = default;
};

// Purpose-built controlled-actor inventory read. Household membership remains
// optional because actor existence does not imply household membership. No client
// balance is persisted: every field is derived from current Core state.
struct ControlledActorCarryProjection final {
    ProtocolInteger entity_id{};
    ProtocolInteger carried_grain_units{};
    ProtocolInteger grain_carry_capacity_units{};
    std::optional<ProtocolInteger> member_household_id{};
    std::optional<ProtocolInteger> member_household_grain_stock_units{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    bool operator==(const ControlledActorCarryProjection &) const = default;
};

enum class ControlledActorResourceError : std::uint8_t {
    protocol_integer_exhausted,
    controlled_actor_missing,
    actor_without_household,
    invalid_actor_carry_state,
    invalid_household_state,
    controlled_actor_spatial_state_missing,
    outside_store,
    carry_full,
    store_empty,
    carry_empty,
    target_household_missing,
    own_household,
    stock_overflow,
};

// One semantic resource command result. The caller never supplies moved quantity;
// this result reports the quantity and destination state chosen by authoritative
// Core law after acceptance.
struct ControlledActorResourceResult final {
    ProtocolInteger entity_id{};
    ProtocolInteger affected_household_id{};
    ProtocolInteger moved_grain_units{};
    ProtocolInteger carried_grain_units{};
    ProtocolInteger affected_household_grain_stock_units{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    bool operator==(const ControlledActorResourceResult &) const = default;
};

using ControlledActorResourceOutcome =
    std::expected<ControlledActorResourceResult, ControlledActorResourceError>;

} // namespace worldsim::protocol
