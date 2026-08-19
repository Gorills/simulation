#pragma once

#include "protocol/integer.hpp"
#include "protocol/version.hpp"

#include <cstdint>
#include <vector>

namespace worldsim::protocol {

struct ObservedEntityProjection final {
    std::int64_t entity_id{};

    constexpr bool operator==(const ObservedEntityProjection &) const = default;
};

struct ObservedWorldProjection final {
    std::int64_t controlled_actor_id{};
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};
    std::vector<ObservedEntityProjection> entities{};

    bool operator==(const ObservedWorldProjection &) const = default;
};

} // namespace worldsim::protocol
