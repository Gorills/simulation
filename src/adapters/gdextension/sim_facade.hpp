#pragma once

#include "protocol/simulation.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>

namespace worldsim::gdextension {

class SimFacade final : public godot::RefCounted {
    GDCLASS(SimFacade, godot::RefCounted)

public:
    SimFacade();

    [[nodiscard]] godot::Dictionary bootstrap_submit_move(std::int32_t dx, std::int32_t dy);
    [[nodiscard]] godot::Dictionary bootstrap_debug_projection() const;
    [[nodiscard]] godot::Dictionary observed_world_projection() const;
    [[nodiscard]] godot::Dictionary controlled_actor_spatial_projection() const;

protected:
    static void _bind_methods();

private:
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::BootstrapActorProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::ObservedWorldProjection &projection
    );
    [[nodiscard]] static godot::Dictionary to_dictionary(
        const protocol::ControlledActorSpatialProjection &projection
    );

    protocol::Simulation simulation_;
};

} // namespace worldsim::gdextension
