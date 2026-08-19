#pragma once

#include "sim/spatial.hpp"
#include "sim/types.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <unordered_map>
#include <vector>

namespace worldsim::sim {

enum class WorldError : std::uint8_t {
    invalid_entity_id,
    invalid_spatial_state,
    duplicate_entity,
    unknown_entity,
};

enum class WorldSnapshotError : std::uint8_t {
    unsupported_schema_version,
    invalid_entity_id,
    invalid_spatial_state,
    duplicate_entity,
};

struct ActorSpawnState final {
    // Milestone 0 transport probe only. Production spatial movement must not
    // depend on this grid position.
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};
};

struct ActorState final {
    EntityId id{};
    GridPosition bootstrap_position{};
    std::optional<SpatialState> spatial{};

    constexpr bool operator==(const ActorState &) const = default;
};

inline constexpr std::uint32_t kWorldSnapshotSchemaVersion = 1;

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
