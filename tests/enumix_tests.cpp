#include <array>
#include <cstdint>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <enumix/enumix.hpp>

enum class Color {
    Red = 10,
    Blue = 20,
    Green = 30
};

enum class Status {
    Draft = -1,
    Published = 0,
    Archived = 1
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

ENUM_MAPPING(Status,
    std::pair{E::Draft, "Draft"sv},
    std::pair{E::Published, "Published"sv},
    std::pair{E::Archived, "Archived"sv}
);

ENUM_MAPPING(Permission,
    std::pair{E::Read, "Read"sv},
    std::pair{E::Write, "Write"sv},
    std::pair{E::Exec, "Exec"sv}
);

ENUM_FLAGS(Permission);

TEST_CASE("enumToString returns the registered name")
{
    STATIC_REQUIRE(enumix::enumToString(Color::Red) == "Red");
    STATIC_REQUIRE(enumix::enumToString(Status::Draft) == "Draft");
    STATIC_REQUIRE(enumix::enumToString(static_cast<Color>(999)).empty());
}

TEST_CASE("enumFromString resolves exact and case-insensitive names")
{
    STATIC_REQUIRE(enumix::enumFromString<Color>("Blue") == Color::Blue);
    STATIC_REQUIRE_FALSE(enumix::enumFromString<Color>("blue").has_value());
    STATIC_REQUIRE(
        enumix::enumFromString<Color, enumix::CaseInsensitive>("green") == Color::Green);
    STATIC_REQUIRE(enumix::enumFromString<Color>(" Green ") == Color::Green);
}

TEST_CASE("enumFromStringOrDefault falls back when parsing fails")
{
    STATIC_REQUIRE(
        enumix::enumFromStringOrDefault<Color>("missing", Color::Red) == Color::Red);
}

TEST_CASE("flagsToString joins the enabled flag names")
{
    const auto value = static_cast<Permission>(
        static_cast<std::uint32_t>(Permission::Read) |
        static_cast<std::uint32_t>(Permission::Write));

    REQUIRE(enumix::flagsToString(value) == "Read|Write");
    REQUIRE(enumix::flagsToString(Permission::None) == "0");
}

TEST_CASE("flagsFromString parses composite flag names")
{
    const auto parsed = enumix::flagsFromString<Permission>("Read|Exec");
    REQUIRE(parsed.has_value());

    const auto expected = static_cast<Permission>(
        static_cast<std::uint32_t>(Permission::Read) |
        static_cast<std::uint32_t>(Permission::Exec));

    REQUIRE(*parsed == expected);
    REQUIRE_FALSE(enumix::flagsFromString<Permission>(" read | exec ").has_value());
    REQUIRE(
        enumix::flagsFromString<Permission, enumix::CaseInsensitive>(" read | exec ")
            == expected);
    REQUIRE(enumix::flagsFromString<Permission>("0") == Permission::None);
}
