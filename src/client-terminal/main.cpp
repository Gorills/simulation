#include "protocol/protocol.hpp"
#include "sim/simulation.hpp"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

constexpr std::uint32_t kSmokeSeed = 20260818U;
constexpr int kHalfWidth = 4;
constexpr int kHalfHeight = 2;

[[nodiscard]] std::optional<simulation::protocol::MoveDirection> direction_for(char key) noexcept {
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(key)))) {
        case 'w':
            return simulation::protocol::MoveDirection::north;
        case 's':
            return simulation::protocol::MoveDirection::south;
        case 'a':
            return simulation::protocol::MoveDirection::west;
        case 'd':
            return simulation::protocol::MoveDirection::east;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] std::string_view direction_name(simulation::protocol::MoveDirection direction) noexcept {
    switch (direction) {
        case simulation::protocol::MoveDirection::north:
            return "north";
        case simulation::protocol::MoveDirection::south:
            return "south";
        case simulation::protocol::MoveDirection::west:
            return "west";
        case simulation::protocol::MoveDirection::east:
            return "east";
    }
    return "unknown";
}

void print_frame(const simulation::protocol::PlayerProjection projection) {
    std::cout << "FRAME_BEGIN\n";
    std::cout << "tick=" << projection.tick << " player=(" << projection.x << ',' << projection.y << ")\n";
    for (int y = -kHalfHeight; y <= kHalfHeight; ++y) {
        for (int x = -kHalfWidth; x <= kHalfWidth; ++x) {
            if (x == projection.x && y == projection.y) {
                std::cout << '@';
            } else if (x == 0 && y == 0) {
                std::cout << '+';
            } else {
                std::cout << '.';
            }
        }
        std::cout << '\n';
    }
    std::cout << "FRAME_END\n";
}

void print_debug(
    const simulation::Simulation& simulation,
    const simulation::protocol::CommandResult* last_result,
    std::string_view last_direction) {
    const auto projection = simulation.player_projection();
    std::cout << "DEBUG_JSON {\"ready\":true,\"protocolVersion\":"
              << simulation::protocol::kProtocolVersion
              << ",\"seed\":" << simulation.seed()
              << ",\"player\":{\"x\":" << projection.x
              << ",\"y\":" << projection.y
              << ",\"tick\":" << projection.tick << "},\"lastCommandResult\":";
    if (last_result == nullptr) {
        std::cout << "null";
    } else {
        std::cout << "{\"intent\":\"MoveIntent\",\"accepted\":"
                  << (last_result->accepted ? "true" : "false")
                  << ",\"direction\":\"" << last_direction
                  << "\",\"authoritativeTick\":" << last_result->player.tick << '}';
    }
    std::cout << "}\n";
}

}  // namespace

int main() {
    simulation::Simulation simulation{kSmokeSeed};
    std::cout << "Simulation playable spine\n";
    std::cout << "W/A/S/D move. Q quits.\n";
    print_frame(simulation.player_projection());
    print_debug(simulation, nullptr, "");

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        const char key = static_cast<char>(std::tolower(static_cast<unsigned char>(line.front())));
        if (key == 'q') {
            std::cout << "QUIT\n";
            return 0;
        }

        const auto direction = direction_for(key);
        if (!direction.has_value()) {
            std::cout << "IGNORED key=" << key << "\n";
            continue;
        }

        const auto result = simulation.execute(simulation::protocol::MoveIntent{.direction = *direction});
        const auto name = direction_name(*direction);
        std::cout << "COMMAND MoveIntent direction=" << name
                  << " accepted=" << (result.accepted ? "true" : "false") << "\n";
        print_frame(result.player);
        print_debug(simulation, &result, name);
    }

    return 0;
}
