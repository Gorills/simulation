#pragma once

#include "protocol/version.hpp"

#include <cstdint>

namespace worldsim::protocol {

// Purpose-built presentation sample for the human-controlled actor. Units stay
// integer and explicit across the application boundary; GDExtension translates
// millimeters to Godot meters.
struct ControlledActorSpatialProjection final {
    std::int64_t entity_id{};
    std::int64_t x_mm{};
    std::int64_t y_mm{};
    std::int64_t z_mm{};
    std::int64_t velocity_x_mm_per_second{};
    std::int64_t velocity_y_mm_per_second{};
    std::int64_t velocity_z_mm_per_second{};
    std::uint64_t spatial_epoch{};
    std::uint64_t tick{};
    std::uint64_t revision{};
    std::uint32_t protocol_version{kProtocolVersion};

    constexpr bool operator==(const ControlledActorSpatialProjection &) const = default;
};

} // namespace worldsim::protocol
