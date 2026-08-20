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

// Authoritative base capability for ordinary grounded locomotion. These values
// are deliberately actor state rather than a global controller profile. Future
// causal state such as wounds, carried load, progression or concrete magical
// effects can alter the limits resolved by World without changing intent or
// creating a player/NPC-specific movement law.
//
// The current numbers are first project feel baselines, not biological claims:
// ordinary walk 1.0 m/s, run 3.0 m/s, sprint 5.8 m/s, with deterministic
// acceleration/braking. They remain playtest-tunable through actor/content state.
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
    invalid_place_state,
    invalid_household_state,
    duplicate_entity,
    unknown_entity,
    unknown_household_member,
    unknown_store_place,
    duplicate_household_member,
    actor_already_in_household,
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
    invalid_place_state,
    invalid_household_state,
    duplicate_entity,
    unknown_household_member,
    unknown_store_place,
    duplicate_household_member,
    actor_already_in_household,
};

enum class HouseholdConsumeError : std::uint8_t {
    invalid_entity_id,
    unknown_actor,
    actor_without_household,
    invalid_household_state,
    missing_spatial_state,
    outside_store,
    consume_budget_exhausted,
    insufficient_stock,
};

struct GroundedLocomotionContext final {
    GroundedEnvironment environment{};
    UprightCapsule body{};
    GroundedStepConfig config{};

    [[nodiscard]] bool is_valid() const noexcept {
        return environment.is_valid() && body.is_valid() && config.is_valid();
    }
};

// Temporary Stage C2/Milestone 1 integration context. It owns shared world-law
// fixture data only. Per-actor speed/acceleration/braking are resolved by World
// from ActorState + requested pace before each step.
[[nodiscard]] inline GroundedLocomotionContext make_flat_locomotion_acceptance_context() {
    GroundedLocomotionContext context{
        .body = kFirstPlayableBody,
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
    ActorLocomotionCapability locomotion_capability{};
    std::optional<RestNeedState> rest_need{};
};

struct ActorState final {
    EntityId id{};
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};
    ActorLocomotionCapability locomotion_capability{};
    std::optional<RestNeedState> rest_need{};
    // Hidden fixed-step continuation state. It affects future authoritative
    // movement and therefore belongs to snapshot truth, but not to presentation
    // projections.
    GroundedLocomotionContinuation grounded_locomotion{};

    constexpr bool operator==(const ActorState &) const = default;
};

// First Milestone 2 authoritative place record. It is intentionally only the
// exact local interaction footprint required by household stores. Fields and
// production assignments are admitted later by their own bounded tasks rather
// than through a generic place ontology.
struct PlaceState final {
    EntityId id{};
    Millimeters x{};
    Millimeters z{};
    Millimeters axis_occupancy_tolerance{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return id.is_valid() && axis_occupancy_tolerance.value >= 0;
    }

    constexpr bool operator==(const PlaceState &) const = default;
};

// Household state remains one bounded aggregate rather than an actor class or
// inventory framework. M2.2 adds only the first staple stock, derived-shortage
// threshold, positive per-consume amount and finite acceptance consume budget.
// Carry, work and standing transfer state are admitted by later bounded tasks.
struct HouseholdState final {
    EntityId id{};
    std::vector<EntityId> members{};
    EntityId store_place{};
    std::int64_t grain_stock_units{};
    std::int64_t shortage_threshold_units{};
    std::int64_t consume_amount_units{1};
    std::uint32_t remaining_consume_budget{};

    [[nodiscard]] constexpr bool has_valid_resource_state() const noexcept {
        return grain_stock_units >= 0
            && shortage_threshold_units >= 0
            && consume_amount_units > 0;
    }

    bool operator==(const HouseholdState &) const = default;
};

// One accepted immediate household consumption transition. The operation is
// revision-only: it never advances SimulationTick merely to change stock.
struct HouseholdConsumeResult final {
    EntityId actor{};
    EntityId household{};
    std::int64_t consumed_grain_units{};
    std::int64_t remaining_grain_stock_units{};
    std::uint32_t remaining_consume_budget{};
    bool shortage{};
    SimulationTick tick{};
    WorldRevision revision{};

    constexpr bool operator==(const HouseholdConsumeResult &) const = default;
};

// Intent carries direction/magnitude plus a semantic pace choice. It never
// carries a requested meters-per-second value: World resolves that from the
// authoritative actor capability so a client/NPC decision source cannot author
// the movement outcome directly.
struct ActorGroundedMoveIntent final {
    EntityId actor{};
    PlanarMoveIntent move{};
    LocomotionPace pace{LocomotionPace::walk};

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

inline constexpr std::uint32_t kWorldSnapshotSchemaVersion = 6;

// Core-owned in-memory persistence contract. Serialization format, content and
// protocol envelope versions belong to a later persistence layer. Authoritative
// records stay in deterministic insertion order; derived lookup/id-view state is
// rebuilt on restore and is not persisted.
struct WorldSnapshot final {
    std::uint32_t schema_version{kWorldSnapshotSchemaVersion};
    WorldSeed seed{};
    SimulationTick tick{};
    WorldRevision revision{};
    std::vector<ActorState> actors{};
    std::vector<PlaceState> places{};
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
    [[nodiscard]] std::expected<void, WorldError> add_place(PlaceState place);
    [[nodiscard]] std::expected<void, WorldError> add_household(HouseholdState household);

    // Read-only application eligibility filter. It intentionally mirrors the
    // actor-generic Consume prerequisites so bounded autonomy can suppress
    // impossible proposals; consume_household_grain still revalidates authority.
    [[nodiscard]] bool can_consume_household_grain(EntityId actor) const noexcept;

    // Shared actor-generic M2 consumption rule. Membership, exact-spatial store
    // occupancy, positive content, remaining budget and stock are revalidated
    // against current World state. Accepted Consume changes revision exactly once;
    // refusal changes neither tick nor revision.
    [[nodiscard]] std::expected<HouseholdConsumeResult, HouseholdConsumeError>
    consume_household_grain(EntityId actor) noexcept;

    // Milestone 0 transport probe only. Production spatial movement must use a
    // real actor-location contract rather than extending this cardinal grid API.
    [[nodiscard]] std::expected<void, WorldError> apply_bootstrap_step(
        EntityId id,
        CardinalDirection direction
    ) noexcept;

    // Applies one fixed authoritative locomotion tick atomically to the supplied
    // actor intents. One batch advances SimulationTick/WorldRevision once
    // regardless of actor count, so human and NPC intents share the same world
    // transition rather than advancing time through player-only calls. World
    // resolves each actor's capability + requested pace into solver limits before
    // computing the step. On success samples are post-transition and canonically
    // ordered by EntityId.
    [[nodiscard]] std::expected<GroundedLocomotionTickResult, GroundedLocomotionTickError>
    advance_grounded_locomotion_tick(
        const GroundedLocomotionContext &context,
        std::span<const ActorGroundedMoveIntent> intents
    );

    void advance_one_tick() noexcept;

    // Snapshot/restore is intentionally value-based. Derived runtime indexes and
    // deterministic id views are rebuilt on restore and never persisted as
    // authoritative state.
    [[nodiscard]] WorldSnapshot snapshot() const;
    [[nodiscard]] std::expected<void, WorldSnapshotError> restore(const WorldSnapshot &snapshot);

    // EntityId is the durable external reference. Queries return values rather
    // than addresses into World storage so later storage growth cannot invalidate
    // a caller-held pointer/reference.
    [[nodiscard]] bool contains_actor(EntityId id) const noexcept;
    [[nodiscard]] bool contains_place(EntityId id) const noexcept;
    [[nodiscard]] bool contains_household(EntityId id) const noexcept;
    [[nodiscard]] std::optional<GridPosition> actor_bootstrap_position(EntityId id) const noexcept;
    [[nodiscard]] std::optional<SpatialState> actor_spatial_state(EntityId id) const noexcept;
    [[nodiscard]] std::optional<ActorLocomotionCapability> actor_locomotion_capability(EntityId id) const noexcept;
    [[nodiscard]] std::optional<RestNeedState> actor_rest_need(EntityId id) const noexcept;
    [[nodiscard]] std::optional<PlaceState> place_state(EntityId id) const noexcept;
    [[nodiscard]] std::optional<HouseholdState> household_state(EntityId id) const;

    // Shortage remains derived state: unknown/invalid household state has no
    // readable shortage, and valid state is short exactly when stock < threshold.
    [[nodiscard]] std::optional<bool> household_is_short(EntityId id) const noexcept;

    // Bounded deterministic composition views for protocol observation/decision
    // collection and household discovery. These are derived runtime views, not
    // snapshot truth and not a generic entity registry.
    [[nodiscard]] std::span<const EntityId> actor_ids() const noexcept;
    [[nodiscard]] std::span<const EntityId> household_ids() const noexcept;

    // Resource place occupancy uses exact arrival semantics only: per-axis X/Z
    // distance must fit the place tolerance. Unlike RestNeed body occupancy this
    // does not add the actor capsule radius.
    [[nodiscard]] bool is_actor_inside_place(EntityId actor, EntityId place) const noexcept;

    // Bounded exact-spatial presence query for a concrete local causal condition.
    // Another exact-spatial actor occupies the place when its first-playable
    // planar body overlaps the caller's per-axis X/Z box. This is not
    // actor-body collision, navigation, observation policy or a spatial index.
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
    [[nodiscard]] bool entity_id_in_use(EntityId id) const noexcept;
    [[nodiscard]] bool actor_belongs_to_household(EntityId id) const noexcept;
    [[nodiscard]] std::optional<std::size_t> actor_index(EntityId id) const noexcept;
    [[nodiscard]] std::optional<std::size_t> place_index(EntityId id) const noexcept;
    [[nodiscard]] std::optional<std::size_t> household_index(EntityId id) const noexcept;
    [[nodiscard]] ActorState *find_actor(EntityId id) noexcept;

    // Authoritative record vectors own deterministic insertion order. Maps and
    // id-only vectors are derived lookup/read structures and are rebuilt on
    // restore rather than persisted in WorldSnapshot.
    std::vector<ActorState> actors_{};
    std::vector<PlaceState> places_{};
    std::vector<HouseholdState> households_{};
    std::unordered_map<std::int64_t, std::size_t> actor_index_by_id_{};
    std::unordered_map<std::int64_t, std::size_t> place_index_by_id_{};
    std::unordered_map<std::int64_t, std::size_t> household_index_by_id_{};
    std::vector<EntityId> actor_ids_{};
    std::vector<EntityId> household_ids_{};
    SimulationTick tick_{};
    WorldRevision revision_{};
    WorldSeed seed_{};
};

} // namespace worldsim::sim
