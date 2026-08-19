#include "sim/world.hpp"

namespace worldsim::sim {

World::World(const WorldSeed seed) noexcept : seed_(seed) {}

void World::move(const CardinalDirection direction) noexcept {
    switch (direction) {
    case CardinalDirection::north:
        --player_position_.y;
        break;
    case CardinalDirection::east:
        ++player_position_.x;
        break;
    case CardinalDirection::south:
        ++player_position_.y;
        break;
    case CardinalDirection::west:
        --player_position_.x;
        break;
    }

    ++tick_.value;
}

GridPosition World::player_position() const noexcept {
    return player_position_;
}

SimulationTick World::tick() const noexcept {
    return tick_;
}

WorldSeed World::seed() const noexcept {
    return seed_;
}

} // namespace worldsim::sim
