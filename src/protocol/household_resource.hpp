#pragma once

#include "protocol/integer.hpp"
#include "protocol/version.hpp"

#include <cstdint>

namespace worldsim::protocol {

// Purpose-built read model for the first M2 household resource slice. It exposes
// current authoritative stock and derived shortage only; it is not a generic
// inventory, market, price or resource-definition protocol.
struct HouseholdGrainProjection final {
    ProtocolInteger household_id{};
    ProtocolInteger stored_grams{};
    ProtocolInteger shortage_below_grams{};
    bool shortage{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    constexpr bool operator==(const HouseholdGrainProjection &) const = default;
};

} // namespace worldsim::protocol
