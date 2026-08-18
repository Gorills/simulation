#include <fenster.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

constexpr int kWidth = 320;
constexpr int kHeight = 200;
constexpr std::uint32_t kBackground = 0x20242AU;
constexpr std::uint32_t kFrame = 0x6F7B86U;
constexpr std::uint32_t kIdle = 0xA33C3CU;
constexpr std::uint32_t kActivated = 0x35A853U;
constexpr std::string_view kWindowTitle = "Simulation Graphics Stack Smoke";

void fill_rect(
    std::span<std::uint32_t> pixels,
    int x0,
    int y0,
    int x1,
    int y1,
    std::uint32_t color) {
    const auto clamped_x0 = std::clamp(x0, 0, kWidth);
    const auto clamped_y0 = std::clamp(y0, 0, kHeight);
    const auto clamped_x1 = std::clamp(x1, 0, kWidth);
    const auto clamped_y1 = std::clamp(y1, 0, kHeight);
    for (int y = clamped_y0; y < clamped_y1; ++y) {
        for (int x = clamped_x0; x < clamped_x1; ++x) {
            pixels[static_cast<std::size_t>(y * kWidth + x)] = color;
        }
    }
}

void draw_frame(std::span<std::uint32_t> pixels, bool activated) {
    std::fill(pixels.begin(), pixels.end(), kBackground);
    fill_rect(pixels, 18, 18, kWidth - 18, 22, kFrame);
    fill_rect(pixels, 18, kHeight - 22, kWidth - 18, kHeight - 18, kFrame);
    fill_rect(pixels, 18, 22, 22, kHeight - 22, kFrame);
    fill_rect(pixels, kWidth - 22, 22, kWidth - 18, kHeight - 22, kFrame);
    fill_rect(pixels, 52, 62, 132, 138, 0x3E6B8FU);
    fill_rect(pixels, 188, 62, 268, 138, activated ? kActivated : kIdle);
}

int self_test() {
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(kWidth * kHeight));
    draw_frame(pixels, false);
    const auto probe = static_cast<std::size_t>(100 * kWidth + 228);
    if (pixels[probe] != kIdle) {
        std::cerr << "idle framebuffer probe mismatch\n";
        return 1;
    }
    draw_frame(pixels, true);
    if (pixels[probe] != kActivated) {
        std::cerr << "activated framebuffer probe mismatch\n";
        return 1;
    }
    std::cout << "graphics framebuffer self-test passed\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view{argv[1]} == "--self-test") {
        return self_test();
    }

    if (argc != 3 || std::string_view{argv[1]} != "--state-file") {
        std::cerr << "usage: platform_graphics_smoke --state-file <path>\n";
        return 2;
    }

    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(kWidth * kHeight));
    struct fenster window {
        .title = kWindowTitle.data(),
        .width = kWidth,
        .height = kHeight,
        .buf = pixels.data(),
        .keys = {},
        .mod = 0,
        .x = 0,
        .y = 0,
        .mouse = 0,
#if defined(__APPLE__)
        .wnd = nullptr,
#elif defined(_WIN32)
        .hwnd = nullptr,
#else
        .dpy = nullptr,
        .w = 0,
        .gc = nullptr,
        .img = nullptr,
#endif
    };

    if (fenster_open(&window) != 0) {
        std::cerr << "failed to open graphics window\n";
        return 3;
    }

    bool activated = false;
    bool previous_d = false;
    bool previous_q = false;
    draw_frame(pixels, activated);

    while (fenster_loop(&window) == 0) {
        const bool d_down = window.keys['D'] != 0;
        const bool q_down = window.keys['Q'] != 0 || window.keys[27] != 0;

        if (d_down && !previous_d) {
            activated = true;
            draw_frame(pixels, activated);
            std::ofstream state{argv[2], std::ios::trunc};
            state << "d_received=1\n";
            state.flush();
            std::cout << "GRAPHICS_SMOKE D_RECEIVED\n" << std::flush;
        }
        if (q_down && !previous_q) {
            break;
        }

        previous_d = d_down;
        previous_q = q_down;
        fenster_sleep(5);
    }

    fenster_close(&window);
    return activated ? 0 : 4;
}
