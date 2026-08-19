#include "sim/world.hpp"

#include <expected>

namespace worldsim::sim {

World::World(const std::uint64_t seed) noexcept : seed_(seed) {}

protocol::MoveOutcome World::move(const protocol::MoveIntent &intent) noexcept {
    const bool horizontal = (intent.dx == -1 || intent.dx == 1) && intent.dy == 0;
    const bool vertical = (intent.dy == -1 || intent.dy == 1) && intent.dx == 0;
    if (!horizontal && !vertical) {
        return std::unexpected(protocol::MoveError::invalid_delta);
    }

    player_x_ += intent.dx;
    player_y_ += intent.dy;
    ++tick_;

    return protocol::MoveResult{.player = player_projection()};
}

protocol::PlayerProjection World::player_projection() const noexcept {
    return protocol::PlayerProjection{
        .x = player_x_,
        .y = player_y_,
        .tick = tick_,
        .seed = seed_,
        .protocol_version = protocol::kProtocolVersion,
    };
}

} // namespace worldsim::sim
