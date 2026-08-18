#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <charconv>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <thread>

namespace {

bool send_key(Display* display, Window window, KeyCode keycode, int type) {
    XKeyEvent key_event{};
    key_event.display = display;
    key_event.window = window;
    key_event.root = DefaultRootWindow(display);
    key_event.subwindow = None;
    key_event.time = CurrentTime;
    key_event.x = 1;
    key_event.y = 1;
    key_event.x_root = 1;
    key_event.y_root = 1;
    key_event.same_screen = True;
    key_event.keycode = keycode;
    key_event.state = 0;
    key_event.type = type;

    XEvent event{};
    event.xkey = key_event;
    const auto mask = type == KeyPress ? KeyPressMask : KeyReleaseMask;
    return XSendEvent(display, window, False, mask, &event) != 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: platform_x11_key_sender <window-id> <key>\n";
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

    const std::string_view key_text{argv[2]};
    if (key_text.size() != 1) {
        std::cerr << "key must be one ASCII character\n";
        return 2;
    }

    Display* const display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "unable to open DISPLAY\n";
        return 3;
    }

    char key_name[2]{static_cast<char>(key_text.front()), '\0'};
    const KeySym keysym = XStringToKeysym(key_name);
    const KeyCode keycode = XKeysymToKeycode(display, keysym);
    const auto window = static_cast<Window>(parsed_window);

    const bool pressed = send_key(display, window, keycode, KeyPress);
    XFlush(display);
    std::this_thread::sleep_for(std::chrono::milliseconds{40});
    const bool released = send_key(display, window, keycode, KeyRelease);
    XFlush(display);
    XCloseDisplay(display);

    if (!pressed || !released) {
        std::cerr << "failed to send X11 key event\n";
        return 4;
    }
    return 0;
}
