#include "sim/grounded_locomotion.hpp"

#include <limits>
#include <optional>

namespace worldsim::sim {
namespace {

[[nodiscard]] bool checked_add(
    const std::int64_t a,
    const std::int64_t b,
    std::int64_t &out
) noexcept {
    if (b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) {
        return false;
    }
    if (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b) {
        return false;
    }
    out = a + b;
    return true;
}

[[nodiscard]] bool checked_sub(
    const std::int64_t a,
    const std::int64_t b,
    std::int64_t &out
) noexcept {
    if (b > 0 && a < std::numeric_limits<std::int64_t>::min() + b) {
        return false;
    }
    if (b < 0 && a > std::numeric_limits<std::int64_t>::max() + b) {
        return false;
    }
    out = a - b;
    return true;
}

[[nodiscard]] bool checked_scale_intent(
    const std::int64_t speed,
    const std::int32_t intent,
    std::int64_t &out
) noexcept {
    const auto component = static_cast<std::int64_t>(intent);
    if (component == 0 || speed == 0) {
        out = 0;
        return true;
    }

    const auto magnitude = component < 0 ? -component : component;
    if (speed > std::numeric_limits<std::int64_t>::max() / magnitude) {
        return false;
    }
    out = (speed * component) / static_cast<std::int64_t>(kIntentScale);
    return true;
}

[[nodiscard]] bool integrate_axis(
    const std::int64_t velocity,
    const std::int64_t old_remainder,
    const std::uint32_t ticks_per_second,
    std::int64_t &delta,
    std::int64_t &new_remainder
) noexcept {
    std::int64_t numerator{};
    if (!checked_add(velocity, old_remainder, numerator)) {
        return false;
    }
    const auto divisor = static_cast<std::int64_t>(ticks_per_second);
    delta = numerator / divisor;
    new_remainder = numerator % divisor;
    return true;
}

[[nodiscard]] std::optional<Millimeters> flat_support_height(
    const GroundedEnvironment &environment,
    const Millimeters x,
    const Millimeters z,
    const Millimeters expected_height
) noexcept {
    for (const auto &patch : environment.ground) {
        if (!patch.is_flat()) {
            continue;
        }
        if (patch.x.contains(x) && patch.z.contains(z) && patch.height_at_min == expected_height) {
            return patch.height_at_min;
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool overlaps_vertical(
    const MillimeterRange &barrier,
    const Millimeters body_bottom,
    const Millimeters body_top
) noexcept {
    return body_top.value >= barrier.min.value && body_bottom.value <= barrier.max.value;
}

} // namespace

bool GroundedEnvironment::is_valid() const noexcept {
    for (const auto &patch : ground) {
        if (!patch.is_valid()) {
            return false;
        }
    }
    for (const auto &barrier : barriers) {
        if (!barrier.is_valid()) {
            return false;
        }
    }
    return true;
}

std::expected<GroundedStepState, GroundedStepError> step_grounded(
    const GroundedEnvironment &environment,
    const UprightCapsule &body,
    const GroundedStepConfig &config,
    const GroundedStepState &state,
    const PlanarMoveIntent intent
) noexcept {
    if (!environment.is_valid()) {
        return std::unexpected(GroundedStepError::invalid_environment);
    }
    if (!body.is_valid()) {
        return std::unexpected(GroundedStepError::invalid_body);
    }
    if (!config.is_valid()) {
        return std::unexpected(GroundedStepError::invalid_config);
    }
    if (!state.spatial.is_valid()) {
        return std::unexpected(GroundedStepError::invalid_state);
    }
    const auto tick_rate = static_cast<std::int64_t>(config.ticks_per_second);
    if (
        state.remainder.x <= -tick_rate || state.remainder.x >= tick_rate ||
        state.remainder.z <= -tick_rate || state.remainder.z >= tick_rate
    ) {
        return std::unexpected(GroundedStepError::invalid_state);
    }
    if (!intent.is_valid()) {
        return std::unexpected(GroundedStepError::invalid_intent);
    }

    const auto &position = state.spatial.position;
    if (!flat_support_height(environment, position.x, position.z, position.y).has_value()) {
        return std::unexpected(GroundedStepError::no_flat_support);
    }

    std::int64_t velocity_x{};
    std::int64_t velocity_z{};
    if (
        !checked_scale_intent(config.move_speed.value, intent.x, velocity_x) ||
        !checked_scale_intent(config.move_speed.value, intent.z, velocity_z)
    ) {
        return std::unexpected(GroundedStepError::arithmetic_overflow);
    }

    std::int64_t delta_x{};
    std::int64_t delta_z{};
    std::int64_t remainder_x{};
    std::int64_t remainder_z{};
    if (
        !integrate_axis(velocity_x, state.remainder.x, config.ticks_per_second, delta_x, remainder_x) ||
        !integrate_axis(velocity_z, state.remainder.z, config.ticks_per_second, delta_z, remainder_z)
    ) {
        return std::unexpected(GroundedStepError::arithmetic_overflow);
    }

    GroundedStepState next = state;
    if (
        !checked_add(position.x.value, delta_x, next.spatial.position.x.value) ||
        !checked_add(position.z.value, delta_z, next.spatial.position.z.value)
    ) {
        return std::unexpected(GroundedStepError::arithmetic_overflow);
    }
    next.remainder = GroundedIntegrationRemainder{.x = remainder_x, .z = remainder_z};
    next.spatial.velocity = SpatialVelocity{
        .x = MillimetersPerSecond{velocity_x},
        .y = MillimetersPerSecond{0},
        .z = MillimetersPerSecond{velocity_z},
    };

    std::int64_t body_top_value{};
    if (!checked_add(position.y.value, body.height.value, body_top_value)) {
        return std::unexpected(GroundedStepError::arithmetic_overflow);
    }
    const Millimeters body_top{body_top_value};

    for (const auto &barrier : environment.barriers) {
        if (!overlaps_vertical(barrier.vertical, position.y, body_top)) {
            continue;
        }

        const auto start_coordinate = barrier.normal_axis == PlanarAxis::x
            ? position.x.value
            : position.z.value;
        auto &candidate_coordinate = barrier.normal_axis == PlanarAxis::x
            ? next.spatial.position.x.value
            : next.spatial.position.z.value;

        if (start_coordinate == barrier.coordinate.value) {
            return std::unexpected(GroundedStepError::invalid_state);
        }

        if (start_coordinate > barrier.coordinate.value) {
            std::int64_t minimum_center{};
            if (!checked_add(barrier.coordinate.value, body.radius.value, minimum_center)) {
                return std::unexpected(GroundedStepError::arithmetic_overflow);
            }
            if (candidate_coordinate < minimum_center) {
                candidate_coordinate = minimum_center;
                if (barrier.normal_axis == PlanarAxis::x) {
                    next.spatial.velocity.x.value = 0;
                    next.remainder.x = 0;
                } else {
                    next.spatial.velocity.z.value = 0;
                    next.remainder.z = 0;
                }
            }
        } else {
            std::int64_t maximum_center{};
            if (!checked_sub(barrier.coordinate.value, body.radius.value, maximum_center)) {
                return std::unexpected(GroundedStepError::arithmetic_overflow);
            }
            if (candidate_coordinate > maximum_center) {
                candidate_coordinate = maximum_center;
                if (barrier.normal_axis == PlanarAxis::x) {
                    next.spatial.velocity.x.value = 0;
                    next.remainder.x = 0;
                } else {
                    next.spatial.velocity.z.value = 0;
                    next.remainder.z = 0;
                }
            }
        }
    }

    if (!flat_support_height(
            environment,
            next.spatial.position.x,
            next.spatial.position.z,
            position.y
        ).has_value()) {
        return std::unexpected(GroundedStepError::no_flat_support);
    }

    next.spatial.position.y = position.y;
    return next;
}

} // namespace worldsim::sim
