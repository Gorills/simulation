#pragma once

#include <cstdint>

namespace worldsim::sim {

struct EntityId final {
    std::uint64_t value{};

    constexpr bool operator==(const EntityId &) const = default;
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
