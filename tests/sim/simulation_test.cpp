#include "protocol/protocol.hpp"
#include "sim/simulation.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

template <typename Actual, typename Expected>
void expect_equal(const Actual& actual, const Expected& expected, std::string_view message) {
    if (actual == expected) {
        return;
    }
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

void expect_true(bool value, std::string_view message) {
    if (value) {
        return;
    }
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

void movement_changes_authoritative_state() {
    simulation::Simulation simulation{42U};
    const auto result = simulation.execute(
        simulation::protocol::MoveIntent{.direction = simulation::protocol::MoveDirection::east});

    expect_true(result.accepted, "east move is accepted");
    expect_equal(result.player.x, std::int32_t{1}, "east move increments authoritative x");
    expect_equal(result.player.y, std::int32_t{0}, "east move preserves authoritative y");
    expect_equal(result.player.tick, std::uint64_t{1}, "accepted move advances authoritative tick");
    expect_equal(simulation.seed(), std::uint32_t{42}, "seed remains explicit state");
}

void same_inputs_are_deterministic() {
    constexpr std::array commands{
        simulation::protocol::MoveDirection::east,
        simulation::protocol::MoveDirection::north,
        simulation::protocol::MoveDirection::east,
        simulation::protocol::MoveDirection::south,
        simulation::protocol::MoveDirection::west,
    };

    simulation::Simulation first{2026U};
    simulation::Simulation second{2026U};

    for (const auto direction : commands) {
        const auto first_result = first.execute(simulation::protocol::MoveIntent{.direction = direction});
        const auto second_result = second.execute(simulation::protocol::MoveIntent{.direction = direction});
        expect_true(first_result.accepted, "first deterministic move accepted");
        expect_true(second_result.accepted, "second deterministic move accepted");
        expect_equal(first_result.player.x, second_result.player.x, "deterministic x");
        expect_equal(first_result.player.y, second_result.player.y, "deterministic y");
        expect_equal(first_result.player.tick, second_result.player.tick, "deterministic tick");
    }

    expect_equal(first.seed(), second.seed(), "deterministic seed");
}

}  // namespace

int main() {
    movement_changes_authoritative_state();
    same_inputs_are_deterministic();
    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed\n";
        return 1;
    }
    std::cout << "PASS: sim_core movement and determinism\n";
    return 0;
}
