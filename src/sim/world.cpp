#include "sim/world.hpp"

#include <algorithm>
#include <cassert>
#include <expected>
#include <utility>
#include <vector>

namespace worldsim::sim {
namespace {

struct PendingGroundedMove final {
    std::size_t actor_index{};
    GroundedStepState next{};
};

[[nodiscard]] constexpr std::uint64_t unsigned_distance(
    const std::int64_t first,
    const std::int64_t second
) noexcept {
    if (first >= second) {
        return static_cast<std::uint64_t>(first) - static_cast<std::uint64_t>(second);
    }
    return static_cast<std::uint64_t>(second) - static_cast<std::uint64_t>(first);
}

[[nodiscard]] bool has_duplicate_members(const std::vector<EntityId> &members) noexcept {
    for (std::size_t index = 0; index < members.size(); ++index) {
        for (std::size_t previous = 0; previous < index; ++previous) {
            if (members[index] == members[previous]) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] GroundedLocomotionTickError map_step_error(const GroundedStepError error) noexcept {
    switch (error) {
    case GroundedStepError::invalid_environment:
    case GroundedStepError::invalid_body:
    case GroundedStepError::invalid_config:
        return GroundedLocomotionTickError::invalid_context;
    case GroundedStepError::invalid_state:
        return GroundedLocomotionTickError::invalid_continuation_state;
    case GroundedStepError::invalid_intent:
        return GroundedLocomotionTickError::invalid_intent;
    case GroundedStepError::no_ground_support:
        return GroundedLocomotionTickError::no_ground_support;
    case GroundedStepError::arithmetic_overflow:
        return GroundedLocomotionTickError::arithmetic_overflow;
    }
    return GroundedLocomotionTickError::arithmetic_overflow;
}

// This is the single actor-state -> solver-limits seam. Today it resolves only
// the actor's stored base locomotion capability plus semantic pace. When a real
// mechanic later introduces wounds, carried load, progression or a concrete
// magical effect, its authoritative state belongs here rather than in input,
// Godot, an NPC-only multiplier, or a duplicate movement solver.
[[nodiscard]] std::expected<GroundedStepConfig, GroundedLocomotionTickError>
resolve_grounded_step_config(
    const GroundedStepConfig &world_config,
    const ActorState &actor,
    const LocomotionPace pace
) noexcept {
    if (!actor.locomotion_capability.is_valid()) {
        return std::unexpected(GroundedLocomotionTickError::invalid_locomotion_capability);
    }
    if (!is_valid_locomotion_pace(pace)) {
        return std::unexpected(GroundedLocomotionTickError::invalid_pace);
    }

    GroundedStepConfig resolved = world_config;
    switch (pace) {
    case LocomotionPace::walk:
        resolved.move_speed = actor.locomotion_capability.walk_speed;
        break;
    case LocomotionPace::run:
        resolved.move_speed = actor.locomotion_capability.run_speed;
        break;
    case LocomotionPace::sprint:
        resolved.move_speed = actor.locomotion_capability.sprint_speed;
        break;
    }
    resolved.acceleration = actor.locomotion_capability.acceleration;
    resolved.braking = actor.locomotion_capability.braking;
    return resolved;
}

} // namespace

World::World(const WorldSeed seed) noexcept : seed_(seed) {}

std::expected<void, WorldError> World::spawn_actor(
    const EntityId id,
    const ActorSpawnState initial
) {
    if (!id.is_valid()) {
        return std::unexpected(WorldError::invalid_entity_id);
    }
    if (initial.spatial.has_value() && !initial.spatial->is_valid()) {
        return std::unexpected(WorldError::invalid_spatial_state);
    }
    if (!initial.locomotion_capability.is_valid()) {
        return std::unexpected(WorldError::invalid_locomotion_capability);
    }
    if (initial.rest_need.has_value() && !initial.rest_need->is_valid()) {
        return std::unexpected(WorldError::invalid_rest_need_state);
    }
    if (entity_id_in_use(id)) {
        return std::unexpected(WorldError::duplicate_entity);
    }

    const auto [index_entry, inserted] = actor_index_by_id_.emplace(id.value, actors_.size());
    assert(inserted);
    (void)inserted;

    try {
        actors_.push_back(ActorState{
            .id = id,
            .bootstrap_position = initial.bootstrap_position,
            .spatial = initial.spatial,
            .locomotion_capability = initial.locomotion_capability,
            .rest_need = initial.rest_need,
        });
        try {
            actor_ids_.push_back(id);
        } catch (...) {
            actors_.pop_back();
            throw;
        }
    } catch (...) {
        actor_index_by_id_.erase(index_entry);
        throw;
    }

    ++revision_.value;
    return {};
}

std::expected<void, WorldError> World::add_place(const PlaceState place) {
    if (!place.id.is_valid()) {
        return std::unexpected(WorldError::invalid_entity_id);
    }
    if (!place.is_valid()) {
        return std::unexpected(WorldError::invalid_place_state);
    }
    if (entity_id_in_use(place.id)) {
        return std::unexpected(WorldError::duplicate_entity);
    }

    const auto [index_entry, inserted] = place_index_by_id_.emplace(place.id.value, places_.size());
    assert(inserted);
    (void)inserted;
    try {
        places_.push_back(place);
    } catch (...) {
        place_index_by_id_.erase(index_entry);
        throw;
    }

    ++revision_.value;
    return {};
}

std::expected<void, WorldError> World::add_household(HouseholdState household) {
    if (!household.id.is_valid()) {
        return std::unexpected(WorldError::invalid_entity_id);
    }
    if (!household.store_place.is_valid() || !household.has_valid_resource_state()) {
        return std::unexpected(WorldError::invalid_household_state);
    }
    for (const auto member : household.members) {
        if (!member.is_valid()) {
            return std::unexpected(WorldError::invalid_household_state);
        }
    }
    if (entity_id_in_use(household.id)) {
        return std::unexpected(WorldError::duplicate_entity);
    }
    if (!contains_place(household.store_place)) {
        return std::unexpected(WorldError::unknown_store_place);
    }
    if (has_duplicate_members(household.members)) {
        return std::unexpected(WorldError::duplicate_household_member);
    }
    for (const auto member : household.members) {
        if (!contains_actor(member)) {
            return std::unexpected(WorldError::unknown_household_member);
        }
        if (actor_belongs_to_household(member)) {
            return std::unexpected(WorldError::actor_already_in_household);
        }
    }

    const auto [index_entry, inserted] = household_index_by_id_.emplace(
        household.id.value,
        households_.size()
    );
    assert(inserted);
    (void)inserted;
    try {
        households_.push_back(std::move(household));
        try {
            household_ids_.push_back(households_.back().id);
        } catch (...) {
            households_.pop_back();
            throw;
        }
    } catch (...) {
        household_index_by_id_.erase(index_entry);
        throw;
    }

    ++revision_.value;
    return {};
}

std::expected<void, WorldError> World::apply_bootstrap_step(
    const EntityId id,
    const CardinalDirection direction
) noexcept {
    if (!id.is_valid()) {
        return std::unexpected(WorldError::invalid_entity_id);
    }

    auto *state = find_actor(id);
    if (state == nullptr) {
        return std::unexpected(WorldError::unknown_entity);
    }

    switch (direction) {
    case CardinalDirection::north:
        --state->bootstrap_position.y;
        break;
    case CardinalDirection::east:
        ++state->bootstrap_position.x;
        break;
    case CardinalDirection::south:
        ++state->bootstrap_position.y;
        break;
    case CardinalDirection::west:
        --state->bootstrap_position.x;
        break;
    }

    ++revision_.value;
    return {};
}

std::expected<GroundedLocomotionTickResult, GroundedLocomotionTickError>
World::advance_grounded_locomotion_tick(
    const GroundedLocomotionContext &context,
    const std::span<const ActorGroundedMoveIntent> intents
) {
    if (!context.is_valid()) {
        return std::unexpected(GroundedLocomotionTickError::invalid_context);
    }

    std::vector<PendingGroundedMove> pending;
    pending.reserve(intents.size());

    for (const auto &intent : intents) {
        if (!intent.actor.is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_entity_id);
        }
        if (!intent.move.is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_intent);
        }
        if (!is_valid_locomotion_pace(intent.pace)) {
            return std::unexpected(GroundedLocomotionTickError::invalid_pace);
        }

        const auto index = actor_index(intent.actor);
        if (!index.has_value()) {
            return std::unexpected(GroundedLocomotionTickError::unknown_entity);
        }
        for (const auto &already_pending : pending) {
            if (already_pending.actor_index == *index) {
                return std::unexpected(GroundedLocomotionTickError::duplicate_actor_intent);
            }
        }

        const auto &actor = actors_[*index];
        if (!actor.spatial.has_value()) {
            return std::unexpected(GroundedLocomotionTickError::missing_spatial_state);
        }
        if (!actor.locomotion_capability.is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_locomotion_capability);
        }
        if (!actor.grounded_locomotion.is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_continuation_state);
        }
        if (!actor.grounded_locomotion.is_compatible(context.config.ticks_per_second)) {
            return std::unexpected(GroundedLocomotionTickError::incompatible_tick_rate);
        }

        const auto resolved_config = resolve_grounded_step_config(context.config, actor, intent.pace);
        if (!resolved_config.has_value()) {
            return std::unexpected(resolved_config.error());
        }
        if (!resolved_config->is_valid()) {
            return std::unexpected(GroundedLocomotionTickError::invalid_context);
        }

        const auto step = step_grounded(
            context.environment,
            context.body,
            *resolved_config,
            GroundedStepState{
                .spatial = *actor.spatial,
                .remainder = actor.grounded_locomotion.remainder,
            },
            intent.move
        );
        if (!step.has_value()) {
            return std::unexpected(map_step_error(step.error()));
        }

        pending.push_back(PendingGroundedMove{
            .actor_index = *index,
            .next = *step,
        });
    }

    // Build and order the complete public result before mutating World state.
    // Allocation failure therefore cannot leave a successful subset committed.
    std::vector<GroundedLocomotionSample> samples;
    samples.reserve(pending.size());
    for (const auto &update : pending) {
        samples.push_back(GroundedLocomotionSample{
            .actor = actors_[update.actor_index].id,
            .spatial = update.next.spatial,
        });
    }
    std::sort(
        samples.begin(),
        samples.end(),
        [](const GroundedLocomotionSample &left, const GroundedLocomotionSample &right) {
            return left.actor.value < right.actor.value;
        }
    );

    for (const auto &update : pending) {
        auto &actor = actors_[update.actor_index];
        actor.spatial = update.next.spatial;
        actor.grounded_locomotion = GroundedLocomotionContinuation{
            .remainder = update.next.remainder,
            .ticks_per_second = context.config.ticks_per_second,
        };
    }

    ++tick_.value;
    ++revision_.value;
    return GroundedLocomotionTickResult{
        .tick = tick_,
        .revision = revision_,
        .samples = std::move(samples),
    };
}

void World::advance_one_tick() noexcept {
    ++tick_.value;
    ++revision_.value;
}

WorldSnapshot World::snapshot() const {
    return WorldSnapshot{
        .schema_version = kWorldSnapshotSchemaVersion,
        .seed = seed_,
        .tick = tick_,
        .revision = revision_,
        .actors = actors_,
        .places = places_,
        .households = households_,
    };
}

std::expected<void, WorldSnapshotError> World::restore(const WorldSnapshot &snapshot_state) {
    if (snapshot_state.schema_version != kWorldSnapshotSchemaVersion) {
        return std::unexpected(WorldSnapshotError::unsupported_schema_version);
    }

    // Build into a temporary World so malformed snapshots or allocation failure
    // never partially mutate the current authoritative state.
    World restored{snapshot_state.seed};
    restored.actors_.reserve(snapshot_state.actors.size());
    restored.places_.reserve(snapshot_state.places.size());
    restored.households_.reserve(snapshot_state.households.size());
    restored.actor_index_by_id_.reserve(snapshot_state.actors.size());
    restored.place_index_by_id_.reserve(snapshot_state.places.size());
    restored.household_index_by_id_.reserve(snapshot_state.households.size());
    restored.actor_ids_.reserve(snapshot_state.actors.size());
    restored.household_ids_.reserve(snapshot_state.households.size());

    for (const auto &actor : snapshot_state.actors) {
        if (!actor.id.is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_entity_id);
        }
        if (actor.spatial.has_value() && !actor.spatial->is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_spatial_state);
        }
        if (!actor.locomotion_capability.is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_locomotion_capability);
        }
        if (actor.rest_need.has_value() && !actor.rest_need->is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_rest_need_state);
        }
        if (
            !actor.grounded_locomotion.is_valid() ||
            (!actor.spatial.has_value() && !actor.grounded_locomotion.is_pristine())
        ) {
            return std::unexpected(WorldSnapshotError::invalid_grounded_locomotion_state);
        }
        if (restored.entity_id_in_use(actor.id)) {
            return std::unexpected(WorldSnapshotError::duplicate_entity);
        }

        restored.actor_index_by_id_.emplace(actor.id.value, restored.actors_.size());
        restored.actors_.push_back(actor);
        restored.actor_ids_.push_back(actor.id);
    }

    for (const auto &place : snapshot_state.places) {
        if (!place.id.is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_entity_id);
        }
        if (!place.is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_place_state);
        }
        if (restored.entity_id_in_use(place.id)) {
            return std::unexpected(WorldSnapshotError::duplicate_entity);
        }

        restored.place_index_by_id_.emplace(place.id.value, restored.places_.size());
        restored.places_.push_back(place);
    }

    for (const auto &household : snapshot_state.households) {
        if (!household.id.is_valid()) {
            return std::unexpected(WorldSnapshotError::invalid_entity_id);
        }
        if (!household.store_place.is_valid() || !household.has_valid_resource_state()) {
            return std::unexpected(WorldSnapshotError::invalid_household_state);
        }
        for (const auto member : household.members) {
            if (!member.is_valid()) {
                return std::unexpected(WorldSnapshotError::invalid_household_state);
            }
        }
        if (restored.entity_id_in_use(household.id)) {
            return std::unexpected(WorldSnapshotError::duplicate_entity);
        }
        if (!restored.contains_place(household.store_place)) {
            return std::unexpected(WorldSnapshotError::unknown_store_place);
        }
        if (has_duplicate_members(household.members)) {
            return std::unexpected(WorldSnapshotError::duplicate_household_member);
        }
        for (const auto member : household.members) {
            if (!restored.contains_actor(member)) {
                return std::unexpected(WorldSnapshotError::unknown_household_member);
            }
            if (restored.actor_belongs_to_household(member)) {
                return std::unexpected(WorldSnapshotError::actor_already_in_household);
            }
        }

        restored.household_index_by_id_.emplace(
            household.id.value,
            restored.households_.size()
        );
        restored.households_.push_back(household);
        restored.household_ids_.push_back(household.id);
    }

    restored.tick_ = snapshot_state.tick;
    restored.revision_ = snapshot_state.revision;
    *this = std::move(restored);
    return {};
}

bool World::contains_actor(const EntityId id) const noexcept {
    return id.is_valid() && actor_index_by_id_.contains(id.value);
}

bool World::contains_place(const EntityId id) const noexcept {
    return id.is_valid() && place_index_by_id_.contains(id.value);
}

bool World::contains_household(const EntityId id) const noexcept {
    return id.is_valid() && household_index_by_id_.contains(id.value);
}

std::optional<GridPosition> World::actor_bootstrap_position(const EntityId id) const noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return actors_[*index].bootstrap_position;
}

std::optional<SpatialState> World::actor_spatial_state(const EntityId id) const noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return actors_[*index].spatial;
}

std::optional<ActorLocomotionCapability> World::actor_locomotion_capability(const EntityId id) const noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return actors_[*index].locomotion_capability;
}

std::optional<RestNeedState> World::actor_rest_need(const EntityId id) const noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return actors_[*index].rest_need;
}

std::optional<PlaceState> World::place_state(const EntityId id) const noexcept {
    const auto index = place_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return places_[*index];
}

std::optional<HouseholdState> World::household_state(const EntityId id) const {
    const auto index = household_index(id);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return households_[*index];
}

std::span<const EntityId> World::actor_ids() const noexcept {
    return {actor_ids_.data(), actor_ids_.size()};
}

std::span<const EntityId> World::household_ids() const noexcept {
    return {household_ids_.data(), household_ids_.size()};
}

bool World::is_planar_position_occupied_by_other_actor(
    const EntityId excluded_actor,
    const Millimeters x,
    const Millimeters z,
    const Millimeters axis_tolerance
) const noexcept {
    if (!excluded_actor.is_valid() || axis_tolerance.value < 0) {
        return false;
    }

    // Occupancy is body-overlap of the rest box, not a point-in-box test. The
    // first playable capsule is 380 mm; the rest arrival box is smaller, so a
    // standing actor covering the place would otherwise fail to occupy it.
    const auto occupancy_tolerance =
        static_cast<std::uint64_t>(axis_tolerance.value)
        + static_cast<std::uint64_t>(kFirstPlayableBody.radius.value);
    for (const auto &actor : actors_) {
        if (actor.id == excluded_actor || !actor.spatial.has_value()) {
            continue;
        }
        if (
            unsigned_distance(actor.spatial->position.x.value, x.value) <= occupancy_tolerance &&
            unsigned_distance(actor.spatial->position.z.value, z.value) <= occupancy_tolerance
        ) {
            return true;
        }
    }
    return false;
}

SimulationTick World::tick() const noexcept {
    return tick_;
}

WorldRevision World::revision() const noexcept {
    return revision_;
}

WorldSeed World::seed() const noexcept {
    return seed_;
}

bool World::entity_id_in_use(const EntityId id) const noexcept {
    return actor_index_by_id_.contains(id.value)
        || place_index_by_id_.contains(id.value)
        || household_index_by_id_.contains(id.value);
}

bool World::actor_belongs_to_household(const EntityId id) const noexcept {
    for (const auto &household : households_) {
        for (const auto member : household.members) {
            if (member == id) {
                return true;
            }
        }
    }
    return false;
}

std::optional<std::size_t> World::actor_index(const EntityId id) const noexcept {
    if (!id.is_valid()) {
        return std::nullopt;
    }
    const auto entry = actor_index_by_id_.find(id.value);
    if (entry == actor_index_by_id_.end()) {
        return std::nullopt;
    }

    assert(entry->second < actors_.size());
    assert(actors_[entry->second].id == id);
    return entry->second;
}

std::optional<std::size_t> World::place_index(const EntityId id) const noexcept {
    if (!id.is_valid()) {
        return std::nullopt;
    }
    const auto entry = place_index_by_id_.find(id.value);
    if (entry == place_index_by_id_.end()) {
        return std::nullopt;
    }

    assert(entry->second < places_.size());
    assert(places_[entry->second].id == id);
    return entry->second;
}

std::optional<std::size_t> World::household_index(const EntityId id) const noexcept {
    if (!id.is_valid()) {
        return std::nullopt;
    }
    const auto entry = household_index_by_id_.find(id.value);
    if (entry == household_index_by_id_.end()) {
        return std::nullopt;
    }

    assert(entry->second < households_.size());
    assert(households_[entry->second].id == id);
    return entry->second;
}

ActorState *World::find_actor(const EntityId id) noexcept {
    const auto index = actor_index(id);
    if (!index.has_value()) {
        return nullptr;
    }
    return &actors_[*index];
}

} // namespace worldsim::sim
