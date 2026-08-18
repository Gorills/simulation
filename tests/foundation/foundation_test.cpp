#include <expected>
#include <iostream>

int main() {
    static_assert(__cplusplus >= 202100L, "project requires C++23 mode");
    const std::expected<int, int> value{42};
    if (!value.has_value() || *value != 42) {
        std::cerr << "std::expected contract failed\n";
        return 1;
    }
    return 0;
}
