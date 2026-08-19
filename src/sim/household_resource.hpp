#pragma once

#include "sim/types.hpp"

#include <cstdint>

namespace worldsim::sim {

// Integer physical quantity for the first household staple. Grams are an
// internal deterministic unit; the current acceptance quantities are not claims
// about historical daily consumption, yields, prices or sack sizes.
struct GrainGrams final {
    std::int64_t value{};

    [[nodiscard]] constexpr bool is_nonnegative() const noexcept {
        return value >= 0;
    }

    constexpr bool operator==(const GrainGrams &) const = default;
};

struct HouseholdGrainState final {
    GrainGrams stored{};
    GrainGrams shortage_below{};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return stored.is_nonnegative() && shortage_below.is_nonnegative();
    }

    [[nodiscard]] constexpr bool is_shortage() const noexcept {
        return stored.value < shortage_below.value;
    }

    constexpr bool operator==(const HouseholdGrainState &) const = default;
};

struct HouseholdSpawnState final {
    HouseholdGrainState grain{};
};

struct HouseholdState final {
    HouseholdId id{};
    HouseholdGrainState grain{};

    constexpr bool operator==(const HouseholdState &) const = default;
};

enum class HouseholdResourceError : std::uint8_t {
    invalid_household_id,
    invalid_grain_state,
    duplicate_household,
    unknown_household,
    invalid_amount,
    insufficient_stock,
};

} // namespace worldsim::sim
