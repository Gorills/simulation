#pragma once

#include "protocol/integer.hpp"
#include "protocol/version.hpp"

#include <cstdint>
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

} // namespace worldsim::protocol
