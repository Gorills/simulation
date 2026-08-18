#include "sim/simulation.hpp"

#include <utility>

namespace simulation {
namespace {

[[nodiscard]] constexpr std::pair<std::int32_t, std::int32_t> movement_delta(
    protocol::MoveDirection direction) noexcept {
    switch (direction) {
        case protocol::MoveDirection::north:
            return {0, -1};
        case protocol::MoveDirection::south:
            return {0, 1};
        case protocol::MoveDirection::west:
            return {-1, 0};
        case protocol::MoveDirection::east:
            return {1, 0};
    }
    return {0, 0};
}

}  // namespace

Simulation::Simulation(std::uint32_t seed) noexcept : state_{.seed = seed} {}

protocol::CommandResult Simulation::execute(protocol::MoveIntent intent) noexcept {
    const auto [delta_x, delta_y] = movement_delta(intent.direction);
    state_.player_x += delta_x;
    state_.player_y += delta_y;
    ++state_.tick;

    return protocol::CommandResult{
        .accepted = true,
        .player = player_projection(),
    };
}

protocol::PlayerProjection Simulation::player_projection() const noexcept {
    return protocol::PlayerProjection{
        .x = state_.player_x,
        .y = state_.player_y,
        .tick = state_.tick,
    };
}

std::uint32_t Simulation::seed() const noexcept {
    return state_.seed;
}

}  // namespace simulation
