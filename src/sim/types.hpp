#pragma once

#include <cstdint>

namespace worldsim::sim {

struct EntityId final {
    std::int64_t value{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value > 0;
    }

    constexpr bool operator==(const EntityId &) const = default;
};

// Stable identity for an authoritative household aggregate. A household is not
// an actor and must not be represented by a fake actor EntityId merely to reuse
// storage or presentation machinery.
struct HouseholdId final {
    std::int64_t value{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value > 0;
    }

    constexpr bool operator==(const HouseholdId &) const = default;
};

struct SimulationTick final {
    std::uint64_t value{};

    constexpr bool operator==(const SimulationTick &) const = default;
};

struct WorldRevision final {
    std::uint64_t value{};

    constexpr bool operator==(const WorldRevision &) const = default;
};

struct WorldSeed final {
    std::uint64_t value{};

    constexpr bool operator==(const WorldSeed &) const = default;
};

// Bootstrap-only spatial probe. The production third-person location model will
// replace this grid representation; EntityId/WorldRevision are durable concepts.
struct GridPosition final {
    std::int32_t x{};
    std::int32_t y{};

    constexpr bool operator==(const GridPosition &) const = default;
};

enum class CardinalDirection : std::uint8_t {
    north,
    east,
    south,
    west,
};

} // namespace worldsim::sim
