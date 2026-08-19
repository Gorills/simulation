#pragma once

#include <cstdint>

namespace worldsim::sim {

inline constexpr std::int64_t kMillimetersPerMeter = 1000;

struct Millimeters final {
    std::int64_t value{};

    constexpr bool operator==(const Millimeters &) const = default;
};

struct MillimetersPerSecond final {
    std::int64_t value{};

    constexpr bool operator==(const MillimetersPerSecond &) const = default;
};

struct MillimetersPerSecondSquared final {
    std::int64_t value{};

    constexpr bool operator==(const MillimetersPerSecondSquared &) const = default;
};

struct SpatialPosition final {
    Millimeters x{};
    Millimeters y{};
    Millimeters z{};

    constexpr bool operator==(const SpatialPosition &) const = default;
};

struct SpatialVelocity final {
    MillimetersPerSecond x{};
    MillimetersPerSecond y{};
    MillimetersPerSecond z{};

    constexpr bool operator==(const SpatialVelocity &) const = default;
};

// Increment this value whenever an authoritative relocation is discontinuous
// (teleport, respawn, world-space transfer, etc.). Presentation may interpolate
// samples only while the epoch is unchanged.
struct SpatialEpoch final {
    std::uint64_t value{1};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value > 0;
    }

    constexpr bool operator==(const SpatialEpoch &) const = default;
};

// Exact spatial state is optional world state. An entity can continue to exist
// authoritatively without carrying a centimeter-by-centimeter pose when current
// gameplay does not require identity-resolved spatial causality.
struct SpatialState final {
    SpatialPosition position{};
    SpatialVelocity velocity{};
    SpatialEpoch epoch{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return epoch.is_valid();
    }

    constexpr bool operator==(const SpatialState &) const = default;
};

} // namespace worldsim::sim
