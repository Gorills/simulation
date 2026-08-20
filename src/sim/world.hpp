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
    invalid_actor_grain_carry_state,
    invalid_place_state,
    invalid_household_state,
    invalid_household_social_state,
    invalid_field_work_assignment_state,
    duplicate_entity,
    unknown_entity,
    unknown_household_member,
    unknown_remembered_aid_actor,
    unknown_store_place,
    unknown_work_place,
    unknown_work_destination_household,
    unknown_pledge_destination_household,
    pledge_destination_is_self,
    invalid_standing_transfer_pledge,
    duplicate_household_member,
    actor_already_in_household,
    field_work_assignment_already_exists,
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
    invalid_actor_grain_carry_state,
    invalid_grounded_locomotion_state,
    invalid_place_state,
    invalid_household_state,
    invalid_household_social_state,
    invalid_field_work_assignment_state,
    duplicate_entity,
    unknown_household_member,
    unknown_remembered_aid_actor,
    unknown_store_place,
    unknown_work_place,
    unknown_work_destination_household,
    unknown_pledge_destination_household,
    pledge_destination_is_self,
    invalid_standing_transfer_pledge,
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

enum class HouseholdDrawError : std::uint8_t {
    invalid_entity_id,
    unknown_actor,
    actor_without_household,
    invalid_actor_grain_carry_state,
    invalid_household_state,
    missing_spatial_state,
    outside_store,
    carry_full,
    store_empty,
};

enum class HouseholdDepositError : std::uint8_t {
    invalid_entity_id,
    unknown_actor,
    actor_without_household,
    invalid_actor_grain_carry_state,
    invalid_household_state,
    missing_spatial_state,
    outside_store,
    carry_empty,
    stock_overflow,
};

enum class HouseholdGiftError : std::uint8_t {
    invalid_entity_id,
    unknown_actor,
    unknown_household,
    invalid_actor_grain_carry_state,
    invalid_household_state,
    missing_spatial_state,
    outside_store,
    carry_empty,
    own_household,
    stock_overflow,
};

enum class FieldWorkError : std::uint8_t {
    invalid_entity_id,
    unknown_actor,
    field_work_assignment_missing,
    invalid_field_work_assignment_state,
    unknown_work_place,
    unknown_work_destination_household,
    invalid_household_state,
    missing_spatial_state,
    outside_field,
    work_exhausted,
    stock_overflow,
};

enum class HouseholdTransferError : std::uint8_t {
    invalid_entity_id,
    unknown_actor,
    actor_without_household,
    invalid_household_state,
    invalid_pledge_state,
    unknown_destination_household,
    self_destination,
    missing_spatial_state,
    outside_store,
    pledge_zero,
    insufficient_stock,
    stock_overflow,
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

// One authoritative carried staple slot. This is intentionally not a generic
// inventory/item abstraction: M2 has one grain quantity and one actor-owned bound.
struct ActorGrainCarryState final {
    std::int64_t carried_grain_units{};
    std::int64_t grain_carry_capacity_units{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return carried_grain_units >= 0
            && grain_carry_capacity_units >= 0
            && carried_grain_units <= grain_carry_capacity_units;
    }

    constexpr bool operator==(const ActorGrainCarryState &) const = default;
};

struct ActorSpawnState final {
    // Milestone 0 transport probe only. Production spatial movement must not
    // depend on this grid position.
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};
    ActorLocomotionCapability locomotion_capability{};
    std::optional<RestNeedState> rest_need{};
    ActorGrainCarryState grain_carry{};
};

struct ActorState final {
    EntityId id{};
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};
    ActorLocomotionCapability locomotion_capability{};
    std::optional<RestNeedState> rest_need{};
    ActorGrainCarryState grain_carry{};
    // Hidden fixed-step continuation state. It affects future authoritative
    // movement and therefore belongs to snapshot truth, but not to presentation
    // projections.
    GroundedLocomotionContinuation grounded_locomotion{};

    constexpr bool operator==(const ActorState &) const = default;
};

// First Milestone 2 authoritative place record. It is intentionally only the
// exact local interaction footprint required by household stores and the bounded
// field work point. Production navigation/physics and a generic place ontology
// remain out of scope.
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

// One-way standing household transfer pledge. Remaining quantity plus a durable
// destination household are source-household content, not a live shortage query
// and not a priced exchange.
struct StandingTransferPledge final {
    EntityId destination_household{};
    std::int64_t remaining_grain_units{};

    constexpr bool operator==(const StandingTransferPledge &) const = default;
};

// Household state remains one bounded aggregate rather than an actor class or
// inventory/relationship framework. M3 adds one optional remembered-aid actor:
// EntityId{0} means no outstanding personal favour; a positive actor id means
// this household remembers one qualifying material Gift from that actor. The
// slot is deliberately singular until a playable capability needs concurrent
// household obligations toward multiple actors.
struct HouseholdState final {
    EntityId id{};
    std::vector<EntityId> members{};
    EntityId store_place{};
    std::int64_t grain_stock_units{};
    std::int64_t shortage_threshold_units{};
    std::int64_t consume_amount_units{1};
    std::uint32_t remaining_consume_budget{};
    StandingTransferPledge standing_transfer_pledge{};
    EntityId remembered_material_aid_actor{};

    [[nodiscard]] constexpr bool has_valid_resource_state() const noexcept {
        return grain_stock_units >= 0
            && shortage_threshold_units >= 0
            && consume_amount_units > 0
            && standing_transfer_pledge.remaining_grain_units >= 0;
    }

    [[nodiscard]] constexpr bool has_valid_social_state() const noexcept {
        return remembered_material_aid_actor.value >= 0;
    }

    bool operator==(const HouseholdState &) const = default;
};

// One bounded field/work assignment for M2.7. It is content state, not an entity
// kind or scheduler: the assignment references an existing work place and a
// durable destination household, owns one positive fixture yield, and bounds how
// many revision-only Work transitions may still succeed.
struct FieldWorkAssignmentState final {
    EntityId work_place{};
    EntityId destination_household{};
    std::int64_t yield_grain_units{};
    std::uint32_t remaining_work_completions{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return work_place.is_valid()
            && destination_household.is_valid()
            && yield_grain_units > 0;
    }

    constexpr bool operator==(const FieldWorkAssignmentState &) const = default;
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

struct HouseholdDrawResult final {
    EntityId actor{};
    EntityId household{};
    std::int64_t moved_grain_units{};
    std::int64_t carried_grain_units{};
    std::int64_t remaining_grain_stock_units{};
    SimulationTick tick{};
    WorldRevision revision{};

    constexpr bool operator==(const HouseholdDrawResult &) const = default;
};

struct HouseholdDepositResult final {
    EntityId actor{};
    EntityId household{};
    std::int64_t deposited_grain_units{};
    std::int64_t carried_grain_units{};
    std::int64_t remaining_grain_stock_units{};
    SimulationTick tick{};
    WorldRevision revision{};

    constexpr bool operator==(const HouseholdDepositResult &) const = default;
};

struct HouseholdGiftResult final {
    EntityId actor{};
    EntityId receiving_household{};
    std::int64_t gifted_grain_units{};
    std::int64_t carried_grain_units{};
    std::int64_t receiving_grain_stock_units{};
    SimulationTick tick{};
    WorldRevision revision{};

    constexpr bool operator==(const HouseholdGiftResult &) const = default;
};

struct FieldWorkResult final {
    EntityId actor{};
    EntityId work_place{};
    EntityId destination_household{};
    std::int64_t produced_grain_units{};
    std::int64_t destination_grain_stock_units{};
    std::uint32_t remaining_work_completions{};
    SimulationTick tick{};
    WorldRevision revision{};

    constexpr bool operator==(const FieldWorkResult &) const = default;
};

struct HouseholdTransferResult final {
    EntityId actor{};
    EntityId source_household{};
    EntityId destination_household{};
    std::int64_t transferred_grain_units{};
    std::int64_t source_grain_stock_units{};
    std::int64_t destination_grain_stock_units{};
    std::int64_t remaining_pledge_grain_units{};
    SimulationTick tick{};
    WorldRevision revision{};

    constexpr bool operator==(const HouseholdTransferResult &) const = default;
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

inline constexpr std::uint32_t kWorldSnapshotSchemaVersion = 10;

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
    std::optional<FieldWorkAssignmentState> field_work_assignment{};

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
    [[nodiscard]] std::expected<void, WorldError> add_field_work_assignment(
        FieldWorkAssignmentState assignment
    );

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

    // Shared actor-generic grain carry laws. No operation accepts a quantity from
    // the caller: Draw fills toward the actor's bound, Deposit moves all carried
    // grain into the actor's own household, and Gift moves all carried grain into
    // the explicitly selected receiving household. A Gift to a household that is
    // short immediately before the transition records the first outstanding
    // remembered-aid actor in that same authoritative transition. Accepted
    // operations advance WorldRevision exactly once and never advance
    // SimulationTick.
    [[nodiscard]] std::expected<HouseholdDrawResult, HouseholdDrawError>
    draw_household_grain(EntityId actor) noexcept;
    [[nodiscard]] std::expected<HouseholdDepositResult, HouseholdDepositError>
    deposit_household_grain(EntityId actor) noexcept;
    [[nodiscard]] std::expected<HouseholdGiftResult, HouseholdGiftError>
    gift_household_grain(EntityId actor, EntityId receiving_household) noexcept;

    // Shared actor-generic bounded Work law. The actor supplies no quantity and
    // chooses no destination: authoritative field content owns place, destination,
    // yield and remaining completions. Work is revision-only and fails closed.
    [[nodiscard]] std::expected<FieldWorkResult, FieldWorkError>
    complete_field_work(EntityId actor) noexcept;

    // Shared actor-generic standing household transfer. The actor supplies no
    // quantity and chooses no destination: membership selects the source household
    // pledge, occupancy of that household's store authorizes execution, and the
    // entire remaining pledged amount moves all-or-nothing. Revision-only.
    [[nodiscard]] std::expected<HouseholdTransferResult, HouseholdTransferError>
    execute_household_transfer_pledge(EntityId actor) noexcept;

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
    [[nodiscard]] std::optional<ActorGrainCarryState> actor_grain_carry_state(EntityId id) const noexcept;
    [[nodiscard]] std::optional<PlaceState> place_state(EntityId id) const noexcept;
    [[nodiscard]] std::optional<HouseholdState> household_state(EntityId id) const;
    [[nodiscard]] std::optional<FieldWorkAssignmentState> field_work_assignment() const noexcept;

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
    [[nodiscard]] std::optional<std::size_t> actor_household_index(EntityId id) const noexcept;
    [[nodiscard]] std::optional<std::size_t> actor_index(EntityId id) const noexcept;
    [[nodiscard]] std::optional<std::size_t> place_index(EntityId id) const noexcept;
    [[nodiscard]] std::optional<std::size_t> household_index(EntityId id) const noexcept;
    [[nodiscard]] ActorState *find_actor(EntityId id) noexcept;

    // Authoritative record vectors own deterministic insertion order. Maps and
    // id-only vectors are derived lookup/read structures and are rebuilt on
    // restore rather than persisted in WorldSnapshot. The bounded field work
    // assignment is direct authoritative content, not an index or entity registry.
    std::vector<ActorState> actors_{};
    std::vector<PlaceState> places_{};
    std::vector<HouseholdState> households_{};
    std::optional<FieldWorkAssignmentState> field_work_assignment_{};
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
