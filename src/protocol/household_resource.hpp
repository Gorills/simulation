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

// Purpose-built Work discovery/read. The field assignment is one bounded Core
// content record, so protocol exposes exactly that record rather than a generic
// jobs/place registry. Yield/destination remain read-only authoritative facts.
struct FieldWorkProjection final {
    ProtocolInteger work_place_id{};
    ProtocolInteger work_x_mm{};
    ProtocolInteger work_z_mm{};
    ProtocolInteger work_axis_tolerance_mm{};
    ProtocolInteger destination_household_id{};
    ProtocolInteger yield_grain_units{};
    ProtocolInteger remaining_work_completions{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    bool operator==(const FieldWorkProjection &) const = default;
};

enum class ControlledActorWorkError : std::uint8_t {
    protocol_integer_exhausted,
    controlled_actor_missing,
    field_work_unavailable,
    invalid_field_work_state,
    controlled_actor_spatial_state_missing,
    outside_field,
    work_exhausted,
    stock_overflow,
};

// Semantic Work result. No command payload supplies yield, destination or amount;
// the result reports the authoritative content/output selected by Core.
struct ControlledActorWorkResult final {
    ProtocolInteger entity_id{};
    ProtocolInteger work_place_id{};
    ProtocolInteger destination_household_id{};
    ProtocolInteger produced_grain_units{};
    ProtocolInteger destination_household_grain_stock_units{};
    ProtocolInteger remaining_work_completions{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    bool operator==(const ControlledActorWorkResult &) const = default;
};

using ControlledActorWorkOutcome =
    std::expected<ControlledActorWorkResult, ControlledActorWorkError>;

// Purpose-built standing-transfer read for the controlled actor's member household.
// Remaining quantity and destination are Core content, not shop inventory.
struct StandingTransferPledgeProjection final {
    ProtocolInteger source_household_id{};
    ProtocolInteger destination_household_id{};
    ProtocolInteger remaining_grain_units{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    bool operator==(const StandingTransferPledgeProjection &) const = default;
};

enum class ControlledActorTransferError : std::uint8_t {
    protocol_integer_exhausted,
    controlled_actor_missing,
    actor_without_household,
    invalid_household_state,
    invalid_pledge_state,
    destination_household_missing,
    self_destination,
    controlled_actor_spatial_state_missing,
    outside_store,
    pledge_zero,
    insufficient_stock,
    stock_overflow,
};

struct ControlledActorTransferResult final {
    ProtocolInteger entity_id{};
    ProtocolInteger source_household_id{};
    ProtocolInteger destination_household_id{};
    ProtocolInteger transferred_grain_units{};
    ProtocolInteger source_household_grain_stock_units{};
    ProtocolInteger destination_household_grain_stock_units{};
    ProtocolInteger remaining_pledge_grain_units{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    bool operator==(const ControlledActorTransferResult &) const = default;
};

using ControlledActorTransferOutcome =
    std::expected<ControlledActorTransferResult, ControlledActorTransferError>;

} // namespace worldsim::protocol
