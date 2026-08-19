#pragma once

#include "sim/grounded_locomotion.hpp"
#include "sim/types.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace worldsim::sim {

enum class WorldError : std::uint8_t {
    invalid_entity_id,
    invalid_spatial_state,
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
    invalid_rest_need_state,
    invalid_grounded_locomotion_state,
    duplicate_entity,
};

struct GroundedLocomotionContext final {
    GroundedEnvironment environment{};
    UprightCapsule body{};
    GroundedStepConfig config{};

    [[nodiscard]] bool is_valid() const noexcept {
        return environment.is_valid() && body.is_valid() && config.is_valid();
    }
};

// Temporary Stage C2 integration context. It is neutral Simulation-owned data,
// not a production content location. Real location content can supply another
// GroundedLocomotionContext through the same World operation.
[[nodiscard]] inline GroundedLocomotionContext make_flat_locomotion_acceptance_context() {
    GroundedLocomotionContext context{
        .body = UprightCapsule{
            .radius = Millimeters{380},
            .height = Millimeters{1800},
        },
        .config = GroundedStepConfig{
            .ticks_per_second = 60,
            .move_speed = MillimetersPerSecond{5800},
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
            && remainder.vertical_velocity > -rate
            && remainder.vertical_velocity < rate;
    }

    [[nodiscard]] constexpr bool is_compatible(const std::uint32_t tick_rate) const noexcept {
        return ticks_per_second == 0 || ticks_per_second == tick_rate;
    }

    constexpr bool operator==(const GroundedLocomotionContinuation &) const = default;
};

// First Milestone 1 need state. The actor needs to be within the assigned local
// rest-point tolerance; satisfaction is derived from authoritative SpatialState,
// so no parallel Boolean completion flag can drift from location truth. This is
// deliberately not a generic needs/task framework or a production home model.
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
    // Milestone 0 transport probe only. Production spatial movement must not
    // depend on this grid position.
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};
    std::optional<RestNeedState> rest_need{};
};

struct ActorState final {
    EntityId id{};
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};
    std::optional<RestNeedState> rest_need{};
    // Hidden fixed-step continuation state. It affects future authoritative
    // movement and therefore belongs to snapshot truth, but not to presentation
    // projections.
    GroundedLocomotionContinuation grounded_locomotion{};

    constexpr bool operator==(const ActorState &) const = default;
};

struct ActorGroundedMoveIntent final {
    EntityId actor{};
    PlanarMoveIntent move{};

    constexpr bool operator==(const ActorGroundedMoveIntent &) const = default;
};

// One post-transition authoritative sample produced by a successful locomotion
// batch. Samples are presentation-neutral Core values and never contain protocol
// or Godot types.
struct GroundedLocomotionSample final {
    EntityId actor{};
    SpatialState spatial{};

    constexpr bool operator==(const GroundedLocomotionSample &) const = default;
};

// One successful fixed locomotion transition produces exactly one temporal batch.
// Every sample shares this post-transition tick/revision. Samples are sorted by
// ascending EntityId so presentation order is independent of intent-source order.
struct GroundedLocomotionTickResult final {
    SimulationTick tick{};
    WorldRevision revision{};
    std::vector<GroundedLocomotionSample> samples{};

    bool operator==(const GroundedLocomotionTickResult &) const = default;
};

inline constexpr std::uint32_t kWorldSnapshotSchemaVersion = 3;

// Core-owned in-memory persistence contract. Serialization format, content and
// protocol envelope versions belong to a later persistence layer; this value
// snapshot contains only authoritative World state in deterministic actor order.
struct WorldSnapshot final {
    std::uint32_t schema_version{kWorldSnapshotSchemaVersion};
    WorldSeed seed{};
    SimulationTick tick{};
    WorldRevision revision{};
    std::vector<ActorState> actors{};

    bool operator==(const WorldSnapshot &) const = default;
};

class World final {
public:
    explicit World(WorldSeed seed = WorldSeed{1}) noexcept;

    [[nodiscard]] std::expected<void, WorldError> spawn_actor(
        EntityId id,
        ActorSpawnState initial = {}
    );

    // Milestone 0 transport probe only. Production spatial movement must use a
    // real actor-location contract rather than extending this cardinal grid API.
    [[nodiscard]] std::expected<void, WorldError> apply_bootstrap_step(
        EntityId id,
        CardinalDirection direction
    ) noexcept;

    // Applies one fixed authoritative locomotion tick atomically to the supplied
    // actor intents. One batch advances SimulationTick/WorldRevision once
    // regardless of actor count, so human and NPC intents share the same world
    // transition rather than advancing time through player-only calls. On
    // success the returned samples are post-transition and canonically ordered
    // by EntityId.
    [[nodiscard]] std::expected<GroundedLocomotionTickResult, GroundedLocomotionTickError>
    advance_grounded_locomotion_tick(
        const GroundedLocomotionContext &context,
        std::span<const ActorGroundedMoveIntent> intents
    );

    void advance_one_tick() noexcept;

    // Snapshot/restore is intentionally value-based. Derived runtime indexes are
    // rebuilt on restore and never persisted as authoritative state.
    [[nodiscard]] WorldSnapshot snapshot() const;
    [[nodiscard]] std::expected<void, WorldSnapshotError> restore(const WorldSnapshot &snapshot);

    // EntityId is the durable external reference. Queries return values rather
    // than addresses into World storage so later actor growth cannot invalidate
    // a caller-held pointer/reference.
    [[nodiscard]] bool contains_actor(EntityId id) const noexcept;
    [[nodiscard]] std::optional<GridPosition> actor_bootstrap_position(EntityId id) const noexcept;
    [[nodiscard]] std::optional<SpatialState> actor_spatial_state(EntityId id) const noexcept;
    [[nodiscard]] std::optional<RestNeedState> actor_rest_need(EntityId id) const noexcept;
    [[nodiscard]] SimulationTick tick() const noexcept;
    [[nodiscard]] WorldRevision revision() const noexcept;
    [[nodiscard]] WorldSeed seed() const noexcept;

private:
    [[nodiscard]] std::optional<std::size_t> actor_index(EntityId id) const noexcept;
    [[nodiscard]] ActorState *find_actor(EntityId id) noexcept;

    // actors_ owns deterministic insertion order and compact state. The index is
    // lookup-only; its iteration order must never define simulation behavior.
    std::vector<ActorState> actors_{};
    std::unordered_map<std::int64_t, std::size_t> actor_index_by_id_{};
    SimulationTick tick_{};
    WorldRevision revision_{};
    WorldSeed seed_{};
};

} // namespace worldsim::sim
