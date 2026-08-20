#pragma once

#include "protocol/integer.hpp"
#include "protocol/version.hpp"

#include <cstdint>
#include <expected>

namespace worldsim::protocol {

// Purpose-built M3 read for one household selected from the existing village
// projection. It exposes only whether that household currently remembers the
// controlled actor's qualifying material aid; the identity of any other creditor
// remains hidden from this presentation contract.
struct ControlledActorReciprocalAidProjection final {
    ProtocolInteger household_id{};
    bool remembered_for_controlled_actor{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    constexpr bool operator==(const ControlledActorReciprocalAidProjection &) const = default;
};

enum class ControlledActorReciprocalAidError : std::uint8_t {
    protocol_integer_exhausted,
    controlled_actor_missing,
    target_household_missing,
    invalid_actor_carry_state,
    invalid_household_state,
    controlled_actor_spatial_state_missing,
    outside_store,
    no_remembered_aid,
    remembered_for_other_actor,
    carry_full,
    insufficient_surplus,
};

// Semantic reciprocal-aid result. The caller supplies only the selected household
// identity; Core chooses the quantity from current free carry capacity and safe
// household surplus and consumes the remembered favour atomically on success.
struct ControlledActorReciprocalAidResult final {
    ProtocolInteger entity_id{};
    ProtocolInteger household_id{};
    ProtocolInteger received_grain_units{};
    ProtocolInteger carried_grain_units{};
    ProtocolInteger remaining_household_grain_stock_units{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    constexpr bool operator==(const ControlledActorReciprocalAidResult &) const = default;
};

using ControlledActorReciprocalAidProjectionOutcome = std::expected<
    ControlledActorReciprocalAidProjection,
    ControlledActorReciprocalAidError
>;
using ControlledActorReciprocalAidOutcome = std::expected<
    ControlledActorReciprocalAidResult,
    ControlledActorReciprocalAidError
>;

} // namespace worldsim::protocol
