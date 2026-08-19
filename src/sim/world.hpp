#pragma once

#include "sim/grounded_locomotion.hpp"
#include "sim/household_resource.hpp"
#include "sim/types.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace worldsim::sim {

enum class LocomotionPace : std::uint8_t {
    walk,
    run,
    sprint,
};

[[nodiscard]] constexpr bool is_valid_locomotion_pace(const LocomotionPace pace) noexcept {
    switch (pace) {
    case LocomotionPace::walk:
    case LocomotionPace::run:
    case LocomotionPace::sprint:
        return true;
    }
    return false;
}

struct ActorLocomotionCapability final {
    MillimetersPerSecond walk_speed{1'000};
    MillimetersPerSecond run_speed{3'000};
    MillimetersPerSecond sprint_speed{5'800};
    MillimetersPerSecondSquared acceleration{6'000};
    MillimetersPerSecondSquared braking{8'000};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return walk_speed.value >= 0
            && run_speed.value >= walk_speed.value
            && sprint_speed.value >= run_speed.value
            && acceleration.value >= 0
            && braking.value >= 0;
    }

    constexpr bool operator==(const ActorLocomotionCapability &) const = default;
};

enum class WorldError : std::uint8_t {
    invalid_entity_id,
    invalid_spatial_state,
    invalid_locomotion_capability,
    invalid_rest_need_state,
    duplicate_entity,
    unknown_entity,
};

enum class GroundedLocomotionTickError : std::uint8_t {
    invalid_context,
    invalid_entity_id,
    unknown_entity,
    missing_spatial_state,
    duplicate_actor_intent,
    invalid_locomotion_capability,
    invalid_pace,
    invalid_continuation_state,
    incompatible_tick_rate,
    invalid_intent,
    no_ground_support,
    arithmetic_overflow,
};

enum class WorldSnapshotError : std::uint8_t {
    unsupported_schema_version,
    invalid_entity_id,
    invalid_spatial_state,
    invalid_locomotion_capability,
    invalid_rest_need_state,
    invalid_grounded_locomotion_state,
    duplicate_entity,
    invalid_household_id,
    invalid_household_grain_state,
    duplicate_household,
};

struct GroundedLocomotionContext final {
    GroundedEnvironment environment{};
    UprightCapsule body{};
    GroundedStepConfig config{};

    [[nodiscard]] bool is_valid() const noexcept {
        return environment.is_valid() && body.is_valid() && config.is_valid();
    }
};

[[nodiscard]] inline GroundedLocomotionContext make_flat_locomotion_acceptance_context() {
    GroundedLocomotionContext context{
        .body = UprightCapsule{
            .radius = Millimeters{380},
            .height = Millimeters{1800},
        },
        .config = GroundedStepConfig{
            .ticks_per_second = 60,
            .move_speed = MillimetersPerSecond{0},
            .acceleration = MillimetersPerSecondSquared{0},
            .braking = MillimetersPerSecondSquared{0},
            .max_slope_rise_per_1000_run = 1192,
            .max_step_up = Millimeters{300},
            .gravity = kNonMagicalGravityBaseline,
        },
    };
    context.environment.ground.push_back(GroundPatch{
        .x = MillimeterRange{Millimeters{-10'000}, Millimeters{10'000}},
        .z = MillimeterRange{Millimeters{-10'000}, Millimeters{10'000}},
        .gradient_axis = PlanarAxis::x,
        .height_at_min = Millimeters{0},
        .height_at_max = Millimeters{0},
    });
    return context;
}

struct GroundedLocomotionContinuation final {
    GroundedIntegrationRemainder remainder{};
    std::uint32_t ticks_per_second{};

    [[nodiscard]] constexpr bool is_pristine() const noexcept {
        return ticks_per_second == 0 && remainder == GroundedIntegrationRemainder{};
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        if (ticks_per_second == 0) {
            return remainder == GroundedIntegrationRemainder{};
        }
        const auto rate = static_cast<std::int64_t>(ticks_per_second);
        return remainder.x > -rate && remainder.x < rate
            && remainder.y > -rate && remainder.y < rate
            && remainder.z > -rate && remainder.z < rate
            && remainder.velocity_x > -rate && remainder.velocity_x < rate
            && remainder.velocity_z > -rate && remainder.velocity_z < rate
            && remainder.vertical_velocity > -rate
            && remainder.vertical_velocity < rate;
    }

    [[nodiscard]] constexpr bool is_compatible(const std::uint32_t tick_rate) const noexcept {
        return ticks_per_second == 0 || ticks_per_second == tick_rate;
    }

    constexpr bool operator==(const GroundedLocomotionContinuation &) const = default;
};

struct RestNeedState final {
    Millimeters rest_x{};
    Millimeters rest_z{};
    Millimeters axis_arrival_tolerance{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return axis_arrival_tolerance.value >= 0;
    }

    constexpr bool operator==(const RestNeedState &) const = default;
};

struct ActorSpawnState final {
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};
    ActorLocomotionCapability locomotion_capability{};
    std::optional<RestNeedState> rest_need{};
};

struct ActorState final {
    EntityId id{};
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};
    ActorLocomotionCapability locomotion_capability{};
    std::optional<RestNeedState> rest_need{};
    GroundedLocomotionContinuation grounded_locomotion{};

    constexpr bool operator==(const ActorState &) const = default;
};

struct ActorGroundedMoveIntent final {
    EntityId actor{};
    PlanarMoveIntent move{};
    LocomotionPace pace{LocomotionPace::walk};

    constexpr bool operator==(const ActorGroundedMoveIntent &) const = default;
};

struct GroundedLocomotionSample final {
    EntityId actor{};
    SpatialState spatial{};

    constexpr bool operator==(const GroundedLocomotionSample &) const = default;
};

struct GroundedLocomotionTickResult final {
    SimulationTick tick{};
    WorldRevision revision{};
    std::vector<GroundedLocomotionSample> samples{};

    bool operator==(const GroundedLocomotionTickResult &) const = default;
};

inline constexpr std::uint32_t kWorldSnapshotSchemaVersion = 5;

struct WorldSnapshot final {
    std::uint32_t schema_version{kWorldSnapshotSchemaVersion};
    WorldSeed seed{};
    SimulationTick tick{};
    WorldRevision revision{};
    std::vector<ActorState> actors{};
    std::vector<HouseholdState> households{};

    bool operator==(const WorldSnapshot &) const = default;
};

class World final {
public:
    explicit World(WorldSeed seed = WorldSeed{1}) noexcept;

    [[nodiscard]] std::expected<void, WorldError> spawn_actor(
        EntityId id,
        ActorSpawnState initial = {}
    );

    [[nodiscard]] std::expected<void, HouseholdResourceError> spawn_household(
        HouseholdId id,
        HouseholdSpawnState initial = {}
    );

    // Immediate authoritative stock transition. It changes WorldRevision but
    // deliberately does not advance SimulationTick: recurring consumption/time
    // scheduling is not admitted by this slice.
    [[nodiscard]] std::expected<void, HouseholdResourceError> consume_household_grain(
        HouseholdId id,
        GrainGrams amount
    ) noexcept;

    [[nodiscard]] std::expected<void, WorldError> apply_bootstrap_step(
        EntityId id,
        CardinalDirection direction
    ) noexcept;

    [[nodiscard]] std::expected<GroundedLocomotionTickResult, GroundedLocomotionTickError>
    advance_grounded_locomotion_tick(
        const GroundedLocomotionContext &context,
        std::span<const ActorGroundedMoveIntent> intents
    );

    void advance_one_tick() noexcept;

    [[nodiscard]] WorldSnapshot snapshot() const;
    [[nodiscard]] std::expected<void, WorldSnapshotError> restore(const WorldSnapshot &snapshot);

    [[nodiscard]] bool contains_actor(EntityId id) const noexcept;
    [[nodiscard]] std::optional<GridPosition> actor_bootstrap_position(EntityId id) const noexcept;
    [[nodiscard]] std::optional<SpatialState> actor_spatial_state(EntityId id) const noexcept;
    [[nodiscard]] std::optional<ActorLocomotionCapability> actor_locomotion_capability(EntityId id) const noexcept;
    [[nodiscard]] std::optional<RestNeedState> actor_rest_need(EntityId id) const noexcept;

    [[nodiscard]] bool contains_household(HouseholdId id) const noexcept;
    [[nodiscard]] std::optional<HouseholdGrainState> household_grain_state(HouseholdId id) const noexcept;

    [[nodiscard]] bool is_planar_position_occupied_by_other_actor(
        EntityId excluded_actor,
        Millimeters x,
        Millimeters z,
        Millimeters axis_tolerance
    ) const noexcept;

    [[nodiscard]] SimulationTick tick() const noexcept;
    [[nodiscard]] WorldRevision revision() const noexcept;
    [[nodiscard]] WorldSeed seed() const noexcept;

private:
    [[nodiscard]] std::optional<std::size_t> actor_index(EntityId id) const noexcept;
    [[nodiscard]] ActorState *find_actor(EntityId id) noexcept;
    [[nodiscard]] std::optional<std::size_t> household_index(HouseholdId id) const noexcept;
    [[nodiscard]] HouseholdState *find_household(HouseholdId id) noexcept;

    std::vector<ActorState> actors_{};
    std::unordered_map<std::int64_t, std::size_t> actor_index_by_id_{};
    std::vector<HouseholdState> households_{};
    std::unordered_map<std::int64_t, std::size_t> household_index_by_id_{};
    SimulationTick tick_{};
    WorldRevision revision_{};
    WorldSeed seed_{};
};

} // namespace worldsim::sim
