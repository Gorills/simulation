#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <bit>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

std::uint8_t component(unsigned long pixel, unsigned long mask) {
    if (mask == 0UL) {
        return 0;
    }
    const auto shift = std::countr_zero(mask);
    const unsigned long maximum = mask >> shift;
    const unsigned long value = (pixel & mask) >> shift;
    return static_cast<std::uint8_t>((value * 255UL + maximum / 2UL) / maximum);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: platform_x11_capture <window-id> <output.ppm>\n";
        return 2;
    }

    std::uintptr_t parsed_window = 0;
    const std::string_view id_text{argv[1]};
    const auto base = id_text.starts_with("0x") ? 16 : 10;
    const auto digits = id_text.starts_with("0x") ? id_text.substr(2) : id_text;
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), parsed_window, base);
    if (error != std::errc{} || end != digits.data() + digits.size()) {
        std::cerr << "invalid window id\n";
        return 2;
    }

    Display* const display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "unable to open DISPLAY\n";
        return 3;
    }

    const auto window = static_cast<Window>(parsed_window);
    XWindowAttributes attributes{};
    if (XGetWindowAttributes(display, window, &attributes) == 0) {
        std::cerr << "unable to query window attributes\n";
        XCloseDisplay(display);
        return 4;
    }

    XImage* const image = XGetImage(
        display,
        window,
        0,
        0,
        static_cast<unsigned int>(attributes.width),
        static_cast<unsigned int>(attributes.height),
        AllPlanes,
        ZPixmap);
    if (image == nullptr) {
        std::cerr << "unable to capture window image\n";
        XCloseDisplay(display);
        return 5;
    }

    std::ofstream output{argv[2], std::ios::binary | std::ios::trunc};
    output << "P6\n" << attributes.width << ' ' << attributes.height << "\n255\n";
    for (int y = 0; y < attributes.height; ++y) {
        for (int x = 0; x < attributes.width; ++x) {
            const unsigned long pixel = XGetPixel(image, x, y);
            const char rgb[3]{
                static_cast<char>(component(pixel, image->red_mask)),
                static_cast<char>(component(pixel, image->green_mask)),
                static_cast<char>(component(pixel, image->blue_mask)),
            };
            output.write(rgb, 3);
        }
    }

    const bool okay = output.good();
    output.close();
    XDestroyImage(image);
    XCloseDisplay(display);
    if (!okay) {
        std::cerr << "failed while writing capture\n";
        return 6;
    }
    return 0;
}
