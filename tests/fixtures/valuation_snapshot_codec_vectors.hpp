// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace subathon::test::valuation_snapshot {
inline constexpr std::string_view format = "subathon.internal.valuation-snapshot.v1";

// RSV1-01 and RSV1-02. Line breaks in the populated vector follow the wire sections.
inline constexpr char empty_bytes[] =
    "\x52\x53\x56\x31\x00\x00\x00\x01\x53\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
inline constexpr char populated_bytes[] =
    "\x52\x53\x56\x31\x00\x00\x00\x03\x53\x00\xff\x01\x00\x00\x00\x01\x50\x01\xff\xff\xff\xff\xff"
    "\xff\xff\xff"
    "\x00\x00\x00\x01"
    "\x00\x00\x00\x01\x6f\x00\x00\x00\x03\x70\x00\x80\x00\x00\x00\x01\x63"
    "\x00\x00\x00\x01\x72\x00\x00\x00\x01\x65\x01\x00\x00\x00\x01\x76"
    "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x74\x00\x00"
    "\x01\x00\x00\x00\x00\x01\xff\xff\xff\xff\xff\xff\xff\xfe"
    "\x00\x00\x00\x02"
    "\x00\x00\x00\x03\x45\x55\x52\x00\x00\x00\x03\x55\x53\x44\x00\x00\x00\x04\x39\x2e\x39\x39"
    "\x00\x00\x00\x03\x45\x55\x52\x00\x00\x00\x03\x47\x42\x50\x00\x00\x00\x06\x39\x2e\x30\x65\x2d"
    "\x31"
    "\x00\x00\x00\x01"
    "\x00\x00\x00\x01\x6d\x01\x00\x00\x00\x03\x45\x55\x52\x00\x00\x00\x03\x55\x53\x44"
    "\x00\x00\x00\x06\x31\x2e\x32\x30\x30\x30\x00\x00\x00\x01\x64\x01\x00\x00\x00\x01\x61"
    "\x00\x00\x00\x03\x52\x00\xff\x01\xff\xff\xff\xff\xff\xff\xff\xfd"
    "\x00\x00\x00\x02"
    "\x00\x00\x00\x00\x01\x6f\x00\x00\x00\x00\x00\x00\x00\x02\x01\x00\x00\x00\x01\x6d"
    "\x00\x00\x00\x01\x00\x00\x00\x03"
    "\x00\x00\x00\x03\x45\x55\x52\x00\x00\x00\x01\x31\x00\x00\x00\x01\x31"
    "\x00\x00\x00\x03\x47\x42\x50\x00\x00\x00\x01\x39\x00\x00\x00\x02\x31\x30"
    "\x00\x00\x00\x03\x55\x53\x44\x00\x00\x00\x01\x36\x00\x00\x00\x01\x35";

inline constexpr std::string_view empty_document{empty_bytes, sizeof(empty_bytes) - 1};
inline constexpr std::string_view populated_document{populated_bytes, sizeof(populated_bytes) - 1};
static_assert(format.size() == 39);
static_assert(empty_document.size() == 27);
static_assert(populated_document.size() == 289);

// §5 primitives for synthetic boundary rows. The small literal vectors above
// do not depend on these helpers.
inline std::string big_endian(std::uint64_t value, unsigned width) {
    std::string bytes(width, '\0');
    while (width != 0) {
        bytes[--width] = static_cast<char>(value & 0xff);
        value >>= 8;
    }
    return bytes;
}
inline std::string u32(std::uint32_t value) { return big_endian(value, 4); }
inline std::string u64(std::uint64_t value) { return big_endian(value, 8); }
inline std::string byte(unsigned char value) { return std::string(1, static_cast<char>(value)); }
inline std::string field(std::string_view value) {
    return u32(static_cast<std::uint32_t>(value.size())) + std::string(value);
}
inline std::string base_prefix() { return "RSV1" + field("S") + byte(0) + byte(0); }
inline std::string context(std::string_view observation, std::string_view request = "r") {
    return field(observation) + field("p") + field("c") + field(request) + field("e") +
           std::string(6, '\0');
}
inline std::string quote(std::string_view from, std::string_view to, std::string_view spelling) {
    return field(from) + field(to) + field(spelling);
}
inline std::string fetched(std::string_view observation, std::uint64_t ordinal) {
    return byte(0) + field(observation) + u64(ordinal);
}
inline std::string manual(std::string_view override_ref) { return byte(1) + field(override_ref); }
inline std::string temporary_override(std::string_view ref, std::string_view spelling,
                                      std::string_view reason) {
    return field(ref) + byte(0) + quote("A", "B", spelling) + field("d") + byte(0) + field(reason) +
           byte(0);
}
inline std::string member(std::string_view currency, std::string_view n = "1",
                          std::string_view d = "1") {
    return field(currency) + field(n) + field(d);
}
inline std::string unit_component() { return u32(2) + member("A") + member("B"); }
} // namespace subathon::test::valuation_snapshot
