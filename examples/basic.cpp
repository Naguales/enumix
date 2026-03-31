#include <array>
#include <cstdint>
#include <iostream>
#include <utility>

#include <enumix/enumix.hpp>

enum class Color {
    Red = 10,
    Blue = 20,
    Green = 30
};

enum class Permission : std::uint32_t {
    None = 0,
    Read = 1u << 0,
    Write = 1u << 1,
    Exec = 1u << 2
};

ENUM_MAPPING(Color,
    std::pair{E::Red, "Red"sv},
    std::pair{E::Blue, "Blue"sv},
    std::pair{E::Green, "Green"sv}
);

ENUM_MAPPING(Permission,
    std::pair{E::Read, "Read"sv},
    std::pair{E::Write, "Write"sv},
    std::pair{E::Exec, "Exec"sv}
);

ENUM_FLAGS(Permission);

int main()
{
    const auto colorName = enumix::enumToString(Color::Red);
    const auto exact = enumix::enumFromString<Color>("Blue");
    const auto insensitive =
        enumix::enumFromString<Color, enumix::CaseInsensitive>("green");

    const auto permissions = static_cast<Permission>(
        static_cast<std::uint32_t>(Permission::Read) |
        static_cast<std::uint32_t>(Permission::Write));

    const auto flags = enumix::flagsToString(permissions);
    const auto parsed = enumix::flagsFromString<Permission>("Read|Exec");

    std::cout << "Color::Red -> " << colorName << '\n';
    std::cout << "\"Blue\" parsed? " << (exact.has_value() ? "yes" : "no") << '\n';
    std::cout << "\"green\" parsed case-insensitively? "
              << (insensitive.has_value() ? "yes" : "no") << '\n';
    std::cout << "Read|Write -> " << flags << '\n';
    std::cout << "\"Read|Exec\" parsed? " << (parsed.has_value() ? "yes" : "no") << '\n';
}
