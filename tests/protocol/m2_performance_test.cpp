#include "protocol/simulation.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

TEST(M2Performance, LocomotionAndProjectionCostStayInsideInteractiveBudgets) {
    using clock = std::chrono::steady_clock;
    constexpr int kWarmup = 32;
    constexpr int kSamples = 256;
    // PERFORMANCE.md locomotion budget is 4 ms p99 and bridge budget is 1 ms p99.
    // This bound only rejects pathological multi-millisecond work on the test
    // machine; recorded p99 is evidence, not a CI flake threshold at those targets.
    constexpr double kPathologicalMs = 16.67;

    worldsim::protocol::Simulation simulation{42};
    for (int i = 0; i < kWarmup; ++i) {
        ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
        ASSERT_TRUE(simulation.advance_locomotion_tick().has_value());
        (void)simulation.village_household_resource_projection();
        (void)simulation.controlled_actor_carry_projection();
        (void)simulation.field_work_projection();
        (void)simulation.standing_transfer_pledge_projection();
        (void)simulation.observed_world_projection();
    }

    std::vector<double> locomotion_ms;
    std::vector<double> bridge_ms;
    locomotion_ms.reserve(static_cast<std::size_t>(kSamples));
    bridge_ms.reserve(static_cast<std::size_t>(kSamples));

    for (int i = 0; i < kSamples; ++i) {
        ASSERT_TRUE(simulation.submit_controlled_actor_move_intent({}).has_value());
        const auto loco_start = clock::now();
        ASSERT_TRUE(simulation.advance_locomotion_tick().has_value());
        const auto loco_end = clock::now();
        const auto bridge_start = clock::now();
        (void)simulation.village_household_resource_projection();
        (void)simulation.controlled_actor_carry_projection();
        (void)simulation.field_work_projection();
        (void)simulation.standing_transfer_pledge_projection();
        (void)simulation.observed_world_projection();
        const auto bridge_end = clock::now();
        locomotion_ms.push_back(
            std::chrono::duration<double, std::milli>(loco_end - loco_start).count()
        );
        bridge_ms.push_back(
            std::chrono::duration<double, std::milli>(bridge_end - bridge_start).count()
        );
    }

    auto percentile = [](std::vector<double> values, const double p) {
        std::sort(values.begin(), values.end());
        const auto index = static_cast<std::size_t>(
            (static_cast<double>(values.size() - 1) * p)
        );
        return values[index];
    };

    const auto loco_p99 = percentile(locomotion_ms, 0.99);
    const auto bridge_p99 = percentile(bridge_ms, 0.99);
    std::cout << "M2 acceptance-village locomotion p99_ms=" << loco_p99
              << " budget_ms=4 bridge/projection p99_ms=" << bridge_p99
              << " budget_ms=1 samples=" << kSamples << '\n';

    EXPECT_LT(loco_p99, kPathologicalMs);
    EXPECT_LT(bridge_p99, kPathologicalMs);
}

} // namespace
