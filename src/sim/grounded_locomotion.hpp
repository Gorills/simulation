#pragma once

#include "sim/spatial.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace worldsim::sim {

inline constexpr std::int32_t kIntentScale = 1000;
inline constexpr std::uint32_t kSlopeRunScale = 1000;
inline constexpr MillimetersPerSecondSquared kNonMagicalGravityBaseline{9807};
// Direct solver acceptance fixtures historically reached the 5.8 m/s migration
// cap in one 60 Hz step. Keep that neutral response as the aggregate default so
// geometry tests stay geometry tests. Production World always overwrites these
// two values from authoritative actor capability before invoking the solver.
inline constexpr MillimetersPerSecondSquared kDirectSolverInstantPlanarResponse{348'000};

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

// Fully resolved inputs for one actor's grounded step. `move_speed`,
// `acceleration` and `braking` are not universal world constants: the World
// layer resolves them from authoritative actor capability + requested pace before
// invoking this solver. Direct native fixture tests may provide them explicitly.
struct GroundedStepConfig final {
    std::uint32_t ticks_per_second{};
    MillimetersPerSecond move_speed{};
    MillimetersPerSecondSquared acceleration{kDirectSolverInstantPlanarResponse};
    MillimetersPerSecondSquared braking{kDirectSolverInstantPlanarResponse};
    std::uint32_t max_slope_rise_per_1000_run{};
    Millimeters max_step_up{};
    MillimetersPerSecondSquared gravity{kNonMagicalGravityBaseline};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return ticks_per_second > 0
            && move_speed.value >= 0
            && acceleration.value >= 0
            && braking.value >= 0
            && max_slope_rise_per_1000_run > 0
            && max_step_up.value >= 0
            && gravity.value >= 0;
    }

    constexpr bool operator==(const GroundedStepConfig &) const = default;
};

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

struct GroundedIntegrationRemainder final {
    std::int64_t x{};
    std::int64_t y{};
    std::int64_t z{};
    std::int64_t velocity_x{};
    std::int64_t velocity_z{};
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

[[nodiscard]] std::expected<GroundedStepState, GroundedStepError> step_grounded(
    const GroundedEnvironment &environment,
    const UprightCapsule &body,
    const GroundedStepConfig &config,
    const GroundedStepState &state,
    PlanarMoveIntent intent
) noexcept;

} // namespace worldsim::sim
