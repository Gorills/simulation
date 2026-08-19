#include "protocol/simulation.hpp"

#include <expected>
#include <optional>

namespace worldsim::protocol {
namespace {

[[nodiscard]] std::optional<sim::CardinalDirection> cardinal_direction(const MoveIntent &intent) noexcept {
    if (intent.dx == 0 && intent.dy == -1) {
        return sim::CardinalDirection::north;
    }
    if (intent.dx == 1 && intent.dy == 0) {
        return sim::CardinalDirection::east;
    }
    if (intent.dx == 0 && intent.dy == 1) {
        return sim::CardinalDirection::south;
    }
    if (intent.dx == -1 && intent.dy == 0) {
        return sim::CardinalDirection::west;
    }
    return std::nullopt;
}

} // namespace

Simulation::Simulation(const std::uint64_t seed) noexcept : world_(sim::WorldSeed{seed}) {}

MoveOutcome Simulation::move(const MoveIntent &intent) noexcept {
    const auto direction = cardinal_direction(intent);
    if (!direction.has_value()) {
        return std::unexpected(MoveError::invalid_delta);
    }

    world_.move(*direction);
    return MoveResult{.player = player_projection()};
}

PlayerProjection Simulation::player_projection() const noexcept {
    const auto position = world_.player_position();
    return PlayerProjection{
        .x = position.x,
        .y = position.y,
        .tick = world_.tick().value,
        .seed = world_.seed().value,
        .protocol_version = kProtocolVersion,
    };
}

} // namespace worldsim::protocol
