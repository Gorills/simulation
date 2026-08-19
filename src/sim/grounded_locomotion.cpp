#include "sim/grounded_locomotion.hpp"

#include <limits>
#include <optional>

namespace worldsim::sim {
namespace {

struct SurfaceSample final {
    Millimeters height{};
    bool walkable{};
    PlanarAxis gradient_axis{PlanarAxis::x};
};

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

[[nodiscard]] bool checked_abs(const std::int64_t value, std::int64_t &out) noexcept {
    if (value == std::numeric_limits<std::int64_t>::min()) {
        return false;
    }
    out = value < 0 ? -value : value;
    return true;
}

[[nodiscard]] bool checked_mul_nonnegative_signed(
    const std::int64_t nonnegative,
    const std::int64_t signed_value,
    std::int64_t &out
) noexcept {
    if (nonnegative < 0) {
        return false;
    }
    if (nonnegative == 0 || signed_value == 0) {
        out = 0;
        return true;
    }
    if (signed_value > 0) {
        if (nonnegative > std::numeric_limits<std::int64_t>::max() / signed_value) {
            return false;
        }
    } else if (signed_value < std::numeric_limits<std::int64_t>::min() / nonnegative) {
        return false;
    }
    out = nonnegative * signed_value;
    return true;
}

[[nodiscard]] bool checked_mul_nonnegative_u32(
    const std::int64_t nonnegative,
    const std::uint32_t multiplier,
    std::int64_t &out
) noexcept {
    if (nonnegative < 0) {
        return false;
    }
    if (multiplier == 0 || nonnegative == 0) {
        out = 0;
        return true;
    }
    if (nonnegative > std::numeric_limits<std::int64_t>::max() / static_cast<std::int64_t>(multiplier)) {
        return false;
    }
    out = nonnegative * static_cast<std::int64_t>(multiplier);
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
    if (velocity == 0) {
        delta = 0;
        new_remainder = 0;
        return true;
    }

    std::int64_t numerator{};
    if (!checked_add(velocity, old_remainder, numerator)) {
        return false;
    }
    const auto divisor = static_cast<std::int64_t>(ticks_per_second);
    delta = numerator / divisor;
    new_remainder = numerator % divisor;
    return true;
}

[[nodiscard]] bool patch_height_at(
    const GroundPatch &patch,
    const Millimeters x,
    const Millimeters z,
    Millimeters &out
) noexcept {
    if (!patch.x.contains(x) || !patch.z.contains(z)) {
        return false;
    }
    if (patch.is_flat()) {
        out = patch.height_at_min;
        return true;
    }

    const auto coordinate = patch.gradient_axis == PlanarAxis::x ? x.value : z.value;
    const auto minimum = patch.gradient_axis == PlanarAxis::x ? patch.x.min.value : patch.z.min.value;
    const auto maximum = patch.gradient_axis == PlanarAxis::x ? patch.x.max.value : patch.z.max.value;

    std::int64_t run{};
    std::int64_t offset{};
    std::int64_t rise{};
    if (
        !checked_sub(maximum, minimum, run) || run <= 0 ||
        !checked_sub(coordinate, minimum, offset) || offset < 0 || offset > run ||
        !checked_sub(patch.height_at_max.value, patch.height_at_min.value, rise)
    ) {
        return false;
    }
    if (offset == run) {
        out = patch.height_at_max;
        return true;
    }

    std::int64_t product{};
    std::int64_t delta_height{};
    std::int64_t height{};
    if (
        !checked_mul_nonnegative_signed(offset, rise, product) ||
        !checked_add(patch.height_at_min.value, product / run, height)
    ) {
        return false;
    }
    delta_height = product / run;
    (void)delta_height;
    out = Millimeters{height};
    return true;
}

[[nodiscard]] bool patch_is_walkable(
    const GroundPatch &patch,
    const GroundedStepConfig &config,
    bool &out
) noexcept {
    if (patch.is_flat()) {
        out = true;
        return true;
    }

    const auto minimum = patch.gradient_axis == PlanarAxis::x ? patch.x.min.value : patch.z.min.value;
    const auto maximum = patch.gradient_axis == PlanarAxis::x ? patch.x.max.value : patch.z.max.value;
    std::int64_t run{};
    std::int64_t signed_rise{};
    std::int64_t rise{};
    if (
        !checked_sub(maximum, minimum, run) || run <= 0 ||
        !checked_sub(patch.height_at_max.value, patch.height_at_min.value, signed_rise) ||
        !checked_abs(signed_rise, rise)
    ) {
        return false;
    }

    const auto scale = static_cast<std::int64_t>(kSlopeRunScale);
    const auto whole_runs = run / scale;
    const auto partial_run = run % scale;
    std::int64_t whole_allowed{};
    std::int64_t partial_allowed_scaled{};
    std::int64_t allowed_rise{};
    if (
        !checked_mul_nonnegative_u32(whole_runs, config.max_slope_rise_per_1000_run, whole_allowed) ||
        !checked_mul_nonnegative_u32(partial_run, config.max_slope_rise_per_1000_run, partial_allowed_scaled) ||
        !checked_add(whole_allowed, partial_allowed_scaled / scale, allowed_rise)
    ) {
        return false;
    }

    out = rise <= allowed_rise;
    return true;
}

[[nodiscard]] std::expected<std::optional<SurfaceSample>, GroundedStepError> surface_at(
    const GroundedEnvironment &environment,
    const GroundedStepConfig &config,
    const Millimeters x,
    const Millimeters z
) noexcept {
    std::optional<SurfaceSample> steep_surface;
    for (const auto &patch : environment.ground) {
        if (!patch.x.contains(x) || !patch.z.contains(z)) {
            continue;
        }

        Millimeters height{};
        bool walkable{};
        if (!patch_height_at(patch, x, z, height) || !patch_is_walkable(patch, config, walkable)) {
            return std::unexpected(GroundedStepError::arithmetic_overflow);
        }
        const SurfaceSample sample{
            .height = height,
            .walkable = walkable,
            .gradient_axis = patch.gradient_axis,
        };
        if (walkable) {
            return sample;
        }
        if (!steep_surface.has_value()) {
            steep_surface = sample;
        }
    }
    return steep_surface;
}

[[nodiscard]] bool overlaps_vertical(
    const MillimeterRange &barrier,
    const Millimeters body_bottom,
    const Millimeters body_top
) noexcept {
    return body_top.value >= barrier.min.value && body_bottom.value <= barrier.max.value;
}

void block_axis(
    GroundedStepState &next,
    const GroundedStepState &state,
    const PlanarAxis axis
) noexcept {
    if (axis == PlanarAxis::x) {
        next.spatial.position.x = state.spatial.position.x;
        next.spatial.velocity.x = MillimetersPerSecond{0};
        next.remainder.x = 0;
    } else {
        next.spatial.position.z = state.spatial.position.z;
        next.spatial.velocity.z = MillimetersPerSecond{0};
        next.remainder.z = 0;
    }
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
    auto current_surface_result = surface_at(environment, config, position.x, position.z);
    if (!current_surface_result.has_value()) {
        return std::unexpected(current_surface_result.error());
    }
    const auto current_surface = *current_surface_result;
    if (
        !current_surface.has_value() ||
        !current_surface->walkable ||
        current_surface->height != position.y
    ) {
        return std::unexpected(GroundedStepError::no_ground_support);
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

    auto target_surface_result = surface_at(
        environment,
        config,
        next.spatial.position.x,
        next.spatial.position.z
    );
    if (!target_surface_result.has_value()) {
        return std::unexpected(target_surface_result.error());
    }
    auto target_surface = *target_surface_result;
    if (!target_surface.has_value()) {
        return std::unexpected(GroundedStepError::no_ground_support);
    }

    if (!target_surface->walkable) {
        block_axis(next, state, target_surface->gradient_axis);
        target_surface_result = surface_at(
            environment,
            config,
            next.spatial.position.x,
            next.spatial.position.z
        );
        if (!target_surface_result.has_value()) {
            return std::unexpected(target_surface_result.error());
        }
        target_surface = *target_surface_result;
        if (!target_surface.has_value() || !target_surface->walkable) {
            return std::unexpected(GroundedStepError::no_ground_support);
        }
    }

    next.spatial.position.y = target_surface->height;
    std::int64_t vertical_delta{};
    std::int64_t vertical_velocity{};
    if (
        !checked_sub(next.spatial.position.y.value, position.y.value, vertical_delta) ||
        !checked_mul_nonnegative_signed(
            static_cast<std::int64_t>(config.ticks_per_second),
            vertical_delta,
            vertical_velocity
        )
    ) {
        return std::unexpected(GroundedStepError::arithmetic_overflow);
    }
    next.spatial.velocity.y = MillimetersPerSecond{vertical_velocity};
    return next;
}

} // namespace worldsim::sim
