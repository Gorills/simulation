#pragma once

#include <cstdint>

namespace worldsim::sim {

struct SimulationTick final {
    std::uint64_t value{};

    constexpr bool operator==(const SimulationTick &) const = default;
};

struct WorldSeed final {
    std::uint64_t value{};

    constexpr bool operator==(const WorldSeed &) const = default;
};

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
