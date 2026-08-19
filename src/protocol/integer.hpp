#pragma once

#include <cstdint>
#include <limits>
#include <optional>

namespace worldsim::protocol {

// Integer values crossing the application protocol into Godot use the same
// signed 64-bit domain as Godot Variant/GDScript integers. Simulation Core may
// keep wider unsigned counters internally, but protocol projections must never
// rely on an implementation-defined unsigned-to-signed narrowing conversion.
using ProtocolInteger = std::int64_t;

inline constexpr std::uint64_t kMaxProtocolInteger =
    static_cast<std::uint64_t>(std::numeric_limits<ProtocolInteger>::max());

[[nodiscard]] constexpr std::optional<ProtocolInteger> to_protocol_integer(
    const std::uint64_t value
) noexcept {
    if (value > kMaxProtocolInteger) {
        return std::nullopt;
    }
    return static_cast<ProtocolInteger>(value);
}

} // namespace worldsim::protocol
