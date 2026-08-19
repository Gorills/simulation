#pragma once

#include "protocol/spatial.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace worldsim::protocol {

inline constexpr std::int32_t kPlanarMoveIntentScale = 1000;

enum class ControlledActorLocomotionPace : std::uint8_t {
    walk = 0,
    run = 1,
    sprint = 2,
};

[[nodiscard]] constexpr bool is_valid_controlled_actor_locomotion_pace(
    const ControlledActorLocomotionPace pace
) noexcept {
    switch (pace) {
    case ControlledActorLocomotionPace::walk:
    case ControlledActorLocomotionPace::run:
    case ControlledActorLocomotionPace::sprint:
        return true;
    }
    return false;
}

// Direction/magnitude + semantic pace only. The application client cannot submit
// a desired velocity or displacement; authoritative actor capability resolves the
// actual movement limits inside Simulation World.
struct ControlledActorMoveIntent final {
    std::int32_t x{};
    std::int32_t z{};
    ControlledActorLocomotionPace pace{ControlledActorLocomotionPace::run};

    constexpr bool operator==(const ControlledActorMoveIntent &) const = default;
};

// Post-transition exact state for one actor in an authoritative locomotion batch.
// Tick/revision live on the batch envelope because every sample in the batch was
// produced by the same atomic World transition.
struct AuthoritativeMovementSample final {
    std::int64_t entity_id{};
    std::int64_t x_mm{};
    std::int64_t y_mm{};
    std::int64_t z_mm{};
    std::int64_t velocity_x_mm_per_second{};
    std::int64_t velocity_y_mm_per_second{};
    std::int64_t velocity_z_mm_per_second{};
    ProtocolInteger spatial_epoch{};

    constexpr bool operator==(const AuthoritativeMovementSample &) const = default;
};

// One fixed authoritative locomotion tick. Samples are strictly ordered by
// ascending EntityId and therefore do not inherit player/NPC intent collection
// order. Across batches, tick orders locomotion time while revision positions the
// batch relative to other authoritative World mutations.
struct AuthoritativeMovementSampleBatch final {
    ProtocolInteger tick{};
    ProtocolInteger revision{};
    std::uint32_t protocol_version{kProtocolVersion};
    std::vector<AuthoritativeMovementSample> samples{};

    bool operator==(const AuthoritativeMovementSampleBatch &) const = default;
};

enum class ControlledActorMovementError : std::uint8_t {
    invalid_intent,
    invalid_pace,
    protocol_integer_exhausted,
    controlled_actor_missing,
    controlled_actor_spatial_state_missing,
    world_rejected,
};

using ControlledActorMoveIntentOutcome =
    std::expected<void, ControlledActorMovementError>;
using ControlledActorLocomotionTickOutcome =
    std::expected<AuthoritativeMovementSampleBatch, ControlledActorMovementError>;

} // namespace worldsim::protocol
