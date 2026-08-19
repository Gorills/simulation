#pragma once

#include "sim/world.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>

namespace worldsim::gdextension {

class SimFacade final : public godot::RefCounted {
    GDCLASS(SimFacade, godot::RefCounted)

public:
    SimFacade();

    [[nodiscard]] godot::Dictionary submit_move(std::int32_t dx, std::int32_t dy);
    [[nodiscard]] godot::Dictionary debug_projection() const;

protected:
    static void _bind_methods();

private:
    [[nodiscard]] static godot::Dictionary to_dictionary(const protocol::PlayerProjection &projection);

    sim::World world_;
};

} // namespace worldsim::gdextension
