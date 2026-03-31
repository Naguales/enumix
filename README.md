# enumix

`enumix` is a small header-only C++20 library for explicit enum/string mappings.

It is designed around a simple tradeoff:

- no compiler-specific reflection tricks
- explicit mappings you control
- predictable compile-time behavior
- simple API for enum parsing and bit-flag formatting

It is useful when you want a lightweight enum utility that stays obvious in the debugger and in code review.

## Features

- `enumix::enumToString(value)` for O(1) enum to string lookup
- `enumix::enumFromString<T>(text)` for string to enum lookup
- optional `enumix::CaseInsensitive` parsing
- `enumix::flagsToString(flags)` and `flagsFromString<T>(text)` for bit flags
- compile-time validation for duplicate enum values and duplicate names
- header-only integration

## Requirements

- C++20
- CMake 3.20+ if you use the provided build files

## Quick Start

```cpp
#include <array>
#include <cstdint>
#include <optional>
#include <utility>

#include <enumix/enumix.hpp>

enum class Color {
    Red = 10,
    Green = 20,
    Blue = 30
};

enum class Permission : std::uint32_t {
    None = 0,
    Read = 1u << 0,
    Write = 1u << 1,
    Exec = 1u << 2
};

ENUM_MAPPING(Color,
    std::pair{E::Red, "Red"sv},
    std::pair{E::Green, "Green"sv},
    std::pair{E::Blue, "Blue"sv}
);

ENUM_MAPPING(Permission,
    std::pair{E::Read, "Read"sv},
    std::pair{E::Write, "Write"sv},
    std::pair{E::Exec, "Exec"sv}
);

ENUM_FLAGS(Permission);

int main()
{
    auto s = enumix::enumToString(Color::Red); // O(1)

    auto e = enumix::enumFromString<Color>("Blue");

    auto e2 =
        enumix::enumFromString<Color, enumix::CaseInsensitive>("green");

    auto p = static_cast<Permission>(
        static_cast<std::uint32_t>(Permission::Read) |
        static_cast<std::uint32_t>(Permission::Write));

    auto fs = enumix::flagsToString(p); // "Read|Write"

    auto parsed = enumix::flagsFromString<Permission>("Read|Exec");
}
```

## Mapping Model

The primary registration API is the macro form, because it keeps enum declarations compact while preserving explicit mappings.

```cpp
ENUM_MAPPING(Color,
    std::pair{E::Red, "Red"sv},
    std::pair{E::Green, "Green"sv},
    std::pair{E::Blue, "Blue"sv}
);
```

You can also write `enumix::traits<T>` manually if you need custom logic, but the macro should cover the common case.

This explicit mapping model is the core design choice. `enumix` does not inspect compiler-generated enum names. That keeps the implementation simple and portable, and it lets you expose only the names you actually want.

### Macro API

```cpp
#define ENUM_MAPPING(EnumType, ...)
#define ENUM_FLAGS(EnumType)
```

Example:

```cpp
ENUM_MAPPING(Color,
    std::pair{E::Red, "Red"sv},
    std::pair{E::Green, "Green"sv},
    std::pair{E::Blue, "Blue"sv}
);
```

Inside `ENUM_MAPPING`, `E` is an alias for the enum type and `std::literals` is enabled, so `"Name"sv` works directly.

## Flags

Mark a flag enum with:

```cpp
ENUM_FLAGS(Permission);
```

Then use:

```cpp
auto text = enumix::flagsToString(value);
auto parsed = enumix::flagsFromString<Permission>("Read|Write");
```

`flagsFromString` also accepts whitespace around tokens. `"0"` maps to a zero-value flag set.

## CMake

`enumix` ships with a modern CMake setup for a header-only library:

- exports an interface target: `enumix::enumix`
- installs headers
- installs CMake package config files
- has opt-in examples and tests
- keeps consumer integration minimal

### Use as a subdirectory

```cmake
add_subdirectory(path/to/enumix)
target_link_libraries(my_app PRIVATE enumix::enumix)
```

### Install and consume with `find_package`

Yes, this works, but only after `enumix` has been installed to a location that CMake searches, or after you point `CMAKE_PREFIX_PATH` at the install prefix.

```cmake
find_package(enumix CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE enumix::enumix)
```

Typical install flow:

```powershell
cmake -S . -B build
cmake --build build
cmake --install build --prefix .install
```

Then consume it from another project with:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/path/to/enumix/.install"
```

### Recommended VS Code workflow

For Visual Studio Code, the usual setup is:

1. Install the `CMake Tools` extension.
2. Configure with a preset or choose a kit from the status bar.
3. Build the `enumix_basic_example` or `enumix_tests` targets directly from VS Code.

A practical configure command is:

```powershell
cmake -S . -B build -DENUMIX_BUILD_EXAMPLES=ON -DENUMIX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

If Catch2 is not already installed, the build can fetch it automatically with `ENUMIX_FETCH_CATCH2=ON`.

## Tests

Tests are written with Catch2 and cover:

- exact enum parsing
- case-insensitive parsing
- out-of-range enum to string lookup
- negative enum values
- flag round-tripping
- whitespace handling in flag parsing

## Example

See `examples/basic.cpp`.

## Notes and Optional Improvements

The implementation is already in a good place for a small explicit-mapping utility, but these are reasonable next steps if you want to expand it:

- add optional custom separators for flags instead of hard-coding `|`
- add `std::expected` or richer error reporting for parse failures
- provide generated `operator|` helpers for flag enums behind an opt-in macro
- add a sparse-mode fallback for very large enum value gaps instead of rejecting them at compile time
- add package presets via `CMakePresets.json` for even smoother VS Code usage

## When It Fits

- complete control over exported names
- stable behavior across compilers
- a tiny, easy-to-read implementation
- no dependence on compiler enum name extraction
