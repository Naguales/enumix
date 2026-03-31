#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace enumix {

// Specialize traits for each enum that should participate in enumix.
// The mapping must return a constexpr std::array of { EnumValue, "Name" } pairs.
template<typename Enum>
struct traits;

// Opt-in marker for enums that should be treated as bit flags.
template<typename Enum>
struct flags : std::false_type {};

template<typename Enum>
inline constexpr bool is_flags_enum = flags<Enum>::value;

struct CaseSensitive {};
struct CaseInsensitive {};

template<typename Enum>
constexpr auto mapping()
{
    static_assert(std::is_enum_v<Enum>, "enumix requires an enum type");
    constexpr auto value = traits<Enum>::mapping();
    static_assert(value.size() > 0, "enumix::traits<Enum>::mapping() must not be empty");
    return value;
}

namespace detail {

template<typename Enum>
using underlying_t = std::underlying_type_t<Enum>;

constexpr char asciiLower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

constexpr int compare(std::string_view lhs, std::string_view rhs, CaseSensitive)
{
    if (lhs < rhs) {
        return -1;
    }

    if (lhs > rhs) {
        return 1;
    }

    return 0;
}

constexpr int compare(std::string_view lhs, std::string_view rhs, CaseInsensitive)
{
    const std::size_t limit = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
    for (std::size_t i = 0; i < limit; ++i) {
        const char left = asciiLower(lhs[i]);
        const char right = asciiLower(rhs[i]);

        if (left < right) {
            return -1;
        }

        if (left > right) {
            return 1;
        }
    }

    if (lhs.size() < rhs.size()) {
        return -1;
    }

    if (lhs.size() > rhs.size()) {
        return 1;
    }

    return 0;
}

constexpr std::string_view trim(std::string_view text)
{
    std::size_t first = 0;
    while (first < text.size()) {
        const char c = text[first];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
        ++first;
    }

    std::size_t last = text.size();
    while (last > first) {
        const char c = text[last - 1];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
        --last;
    }

    return text.substr(first, last - first);
}

template<typename Enum>
consteval bool hasDuplicateStrings()
{
    constexpr auto entries = mapping<Enum>();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        for (std::size_t j = i + 1; j < entries.size(); ++j) {
            if (entries[i].second == entries[j].second) {
                return true;
            }
        }
    }

    return false;
}

template<typename Enum>
consteval bool hasDuplicateValues()
{
    constexpr auto entries = mapping<Enum>();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        for (std::size_t j = i + 1; j < entries.size(); ++j) {
            if (entries[i].first == entries[j].first) {
                return true;
            }
        }
    }

    return false;
}

template<typename Enum>
consteval auto enumRange()
{
    constexpr auto entries = mapping<Enum>();
    using U = underlying_t<Enum>;

    U minValue = static_cast<U>(entries[0].first);
    U maxValue = minValue;

    for (const auto& [value, name] : entries) {
        (void)name;
        const U raw = static_cast<U>(value);
        if (raw < minValue) {
            minValue = raw;
        }
        if (raw > maxValue) {
            maxValue = raw;
        }
    }

    return std::pair{minValue, maxValue};
}

template<typename Enum, typename Policy>
inline constexpr auto sortedMappingStorage = [] {
    constexpr auto original = mapping<Enum>();

    static_assert(!hasDuplicateStrings<Enum>(),
        "Duplicate enum string mapping detected");
    static_assert(!hasDuplicateValues<Enum>(),
        "Duplicate enum value mapping detected");

    auto sorted = original;
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        for (std::size_t j = i + 1; j < sorted.size(); ++j) {
            if (compare(sorted[j].second, sorted[i].second, Policy{}) < 0) {
                const auto tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }
        }
    }

    return sorted;
}();

template<typename Enum>
inline constexpr auto enumToStringTableStorage = [] {
    constexpr auto entries = mapping<Enum>();
    using U = underlying_t<Enum>;
    constexpr auto range = enumRange<Enum>();
    constexpr U minValue = range.first;
    constexpr U maxValue = range.second;

    constexpr std::size_t span = static_cast<std::size_t>(maxValue - minValue) + 1;

    // enumToString is intentionally O(1). Reject mappings that would force
    // a disproportionately large sparse lookup table.
    static_assert(span <= entries.size() * 64 || span <= 4096,
        "Enum values are too sparse for enumToString O(1) storage");

    std::array<std::string_view, span> table{};
    for (auto& slot : table) {
        slot = {};
    }

    for (const auto& [value, name] : entries) {
        const auto index = static_cast<std::size_t>(static_cast<U>(value) - minValue);
        table[index] = name;
    }

    return std::pair<std::array<std::string_view, span>, U>{table, minValue};
}();

} // namespace detail

template<typename Enum>
constexpr std::string_view enumToString(Enum value)
{
    static_assert(std::is_enum_v<Enum>, "enumix requires an enum type");

    using U = detail::underlying_t<Enum>;
    const auto& data = detail::enumToStringTableStorage<Enum>;
    const auto& table = data.first;
    const U minValue = data.second;

    const U raw = static_cast<U>(value);
    if (raw < minValue) {
        return {};
    }

    const auto index = static_cast<std::size_t>(raw - minValue);
    if (index >= table.size()) {
        return {};
    }

    return table[index];
}

template<typename Enum, typename CasePolicy = CaseSensitive>
constexpr std::optional<Enum> enumFromString(std::string_view text)
{
    static_assert(std::is_enum_v<Enum>, "enumix requires an enum type");

    constexpr auto& sorted = detail::sortedMappingStorage<Enum, CasePolicy>;
    const std::string_view query = detail::trim(text);

    std::size_t left = 0;
    std::size_t right = sorted.size();

    while (left < right) {
        const std::size_t mid = left + (right - left) / 2;
        const int relation = detail::compare(query, sorted[mid].second, CasePolicy{});

        if (relation == 0) {
            return sorted[mid].first;
        }

        if (relation < 0) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }

    return std::nullopt;
}

template<typename Enum>
constexpr std::optional<Enum> enumFromStringInsensitive(std::string_view text)
{
    return enumFromString<Enum, CaseInsensitive>(text);
}

template<typename Enum>
constexpr Enum enumFromStringOrDefault(std::string_view text, Enum fallback)
{
    if (const auto value = enumFromString<Enum>(text)) {
        return *value;
    }

    return fallback;
}

template<typename Enum>
std::string flagsToString(Enum value)
{
    static_assert(is_flags_enum<Enum>, "flagsToString requires a flag enum");

    using U = detail::underlying_t<Enum>;
    constexpr auto entries = mapping<Enum>();

    std::string result;
    U remaining = static_cast<U>(value);
    bool first = true;

    for (const auto& [flag, name] : entries) {
        const U bit = static_cast<U>(flag);
        if (bit == 0) {
            continue;
        }

        if ((remaining & bit) == bit) {
            if (!first) {
                result.push_back('|');
            }

            result += name;
            remaining = static_cast<U>(remaining & ~bit);
            first = false;
        }
    }

    if (result.empty() && static_cast<U>(value) == 0) {
        return "0";
    }

    return result;
}

template<typename Enum, typename CasePolicy = CaseSensitive>
constexpr std::optional<Enum> flagsFromString(std::string_view text)
{
    static_assert(is_flags_enum<Enum>, "flagsFromString requires a flag enum");

    using U = detail::underlying_t<Enum>;
    const std::string_view query = detail::trim(text);

    if (query.empty() || query == "0") {
        return static_cast<Enum>(0);
    }

    U result = 0;
    std::size_t start = 0;

    while (start <= query.size()) {
        const std::size_t separator = query.find('|', start);
        const std::size_t end = (separator == std::string_view::npos) ? query.size() : separator;
        const std::string_view token = detail::trim(query.substr(start, end - start));

        if (token.empty()) {
            return std::nullopt;
        }

        const auto value = enumFromString<Enum, CasePolicy>(token);
        if (!value) {
            return std::nullopt;
        }

        result = static_cast<U>(result | static_cast<U>(*value));

        if (separator == std::string_view::npos) {
            break;
        }

        start = separator + 1;
    }

    return static_cast<Enum>(result);
}

template<typename Enum>
using EnumTraits = traits<Enum>;

template<typename Enum>
using EnumFlags = flags<Enum>;

template<typename Enum>
inline constexpr bool isFlagsEnum = is_flags_enum<Enum>;

} // namespace enumix

namespace enum_utils = enumix;

#define ENUMIX_ENUM_MAPPING(EnumType, ...) \
template<> \
struct enumix::traits<EnumType> { \
    static constexpr auto mapping() \
    { \
        using E = EnumType; \
        using namespace std::literals; \
        return std::array{ __VA_ARGS__ }; \
    } \
}

#define ENUM_MAPPING(EnumType, ...) ENUMIX_ENUM_MAPPING(EnumType, __VA_ARGS__)

#define ENUMIX_FLAGS(EnumType) \
template<> \
struct enumix::flags<EnumType> : std::true_type {}

#define ENUM_FLAGS(EnumType) ENUMIX_FLAGS(EnumType)
