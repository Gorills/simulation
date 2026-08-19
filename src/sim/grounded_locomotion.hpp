#pragma once

#include "sim/spatial.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace worldsim::sim {

inline constexpr std::int32_t kIntentScale = 1000;
inline constexpr std::uint32_t kSlopeRunScale = 1000;
inline constexpr MillimetersPerSecondSquared kNonMagicalGravityBaseline{9807};

enum class PlanarAxis : std::uint8_t {
    x,
    z,
};

struct MillimeterRange final {
    Millimeters min{};
    Millimeters max{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return min.value <= max.value;
    }

    [[nodiscard]] constexpr bool contains(const Millimeters value) const noexcept {
        return value.value >= min.value && value.value <= max.value;
    }

    constexpr bool operator==(const MillimeterRange &) const = default;
};

// A bounded support patch whose height changes linearly along one planar axis.
// Flat ground is represented by equal endpoint heights. Non-flat patches are
// classified against GroundedStepConfig's project-owned slope threshold.
struct GroundPatch final {
    MillimeterRange x{};
    MillimeterRange z{};
    PlanarAxis gradient_axis{PlanarAxis::x};
    Millimeters height_at_min{};
    Millimeters height_at_max{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        if (!x.is_valid() || !z.is_valid()) {
            return false;
        }
        if (height_at_min == height_at_max) {
            return true;
        }
        return gradient_axis == PlanarAxis::x
            ? x.min.value < x.max.value
            : z.min.value < z.max.value;
    }

    [[nodiscard]] constexpr bool is_flat() const noexcept {
        return height_at_min == height_at_max;
    }

    constexpr bool operator==(const GroundPatch &) const = default;
};

// Two-sided, axis-aligned vertical blocker. For the current acceptance arena a
// blocker spans its full lane, so finite wall-end/corner behavior is deliberately
// deferred until a real location requires it.
struct VerticalBarrier final {
    PlanarAxis normal_axis{PlanarAxis::x};
    Millimeters coordinate{};
    MillimeterRange vertical{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return vertical.is_valid();
    }

    constexpr bool operator==(const VerticalBarrier &) const = default;
};

struct GroundedEnvironment final {
    std::vector<GroundPatch> ground{};
    std::vector<VerticalBarrier> barriers{};

    [[nodiscard]] bool is_valid() const noexcept;
};

struct UprightCapsule final {
    Millimeters radius{};
    Millimeters height{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return radius.value > 0 && height.value >= radius.value && (height.value - radius.value) >= radius.value;
    }

    constexpr bool operator==(const UprightCapsule &) const = default;
};

struct GroundedStepConfig final {
    std::uint32_t ticks_per_second{};
    MillimetersPerSecond move_speed{};
    // Maximum absolute rise allowed for each 1000 mm of horizontal run.
    // 1192 approximates the existing project-owned 50 degree presentation
    // baseline without floating-point/trigonometry in authoritative code.
    std::uint32_t max_slope_rise_per_1000_run{};
    // Maximum positive support discontinuity that ordinary grounded movement
    // may step up in one transition. This is independent from grounding snap.
    Millimeters max_step_up{};
    // Positive downward acceleration magnitude. The current playable baseline
    // rounds standard gravity to integer millimeters per second squared.
    MillimetersPerSecondSquared gravity{kNonMagicalGravityBaseline};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return ticks_per_second > 0
            && move_speed.value >= 0
            && max_slope_rise_per_1000_run > 0
            && max_step_up.value >= 0
            && gravity.value >= 0;
    }

    constexpr bool operator==(const GroundedStepConfig &) const = default;
};

// Signed analog intent in [-1000, 1000] per axis. The vector magnitude may not
// exceed 1000, so protocol/input code can preserve analog strength without
// handing the solver a client-authored displacement.
struct PlanarMoveIntent final {
    std::int32_t x{};
    std::int32_t z{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        if (x < -kIntentScale || x > kIntentScale || z < -kIntentScale || z > kIntentScale) {
            return false;
        }
        const auto xx = static_cast<std::int64_t>(x) * static_cast<std::int64_t>(x);
        const auto zz = static_cast<std::int64_t>(z) * static_cast<std::int64_t>(z);
        return xx + zz <= static_cast<std::int64_t>(kIntentScale) * kIntentScale;
    }

    constexpr bool operator==(const PlanarMoveIntent &) const = default;
};

// Position remainders retain fractional millimeters for each axis. The vertical
// velocity remainder separately retains fractional mm/s while gravity is
// integrated at the fixed authoritative tick rate.
struct GroundedIntegrationRemainder final {
    std::int64_t x{};
    std::int64_t y{};
    std::int64_t z{};
    std::int64_t vertical_velocity{};

    constexpr bool operator==(const GroundedIntegrationRemainder &) const = default;
};

struct GroundedStepState final {
    SpatialState spatial{};
    GroundedIntegrationRemainder remainder{};

    constexpr bool operator==(const GroundedStepState &) const = default;
};

enum class GroundedStepError : std::uint8_t {
    invalid_environment,
    invalid_body,
    invalid_config,
    invalid_state,
    invalid_intent,
    no_ground_support,
    arithmetic_overflow,
};

// The historical name reflects the grounded-locomotion subsystem. The transition
// now also owns unsupported airborne state caused by walking off support, applying
// gravity until it lands on walkable support again.
[[nodiscard]] std::expected<GroundedStepState, GroundedStepError> step_grounded(
    const GroundedEnvironment &environment,
    const UprightCapsule &body,
    const GroundedStepConfig &config,
    const GroundedStepState &state,
    PlanarMoveIntent intent
) noexcept;

} // namespace worldsim::sim
