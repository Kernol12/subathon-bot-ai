// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <string_view>
namespace account_capture_test {
using namespace std::literals;
inline constexpr auto z = "0000000000000000000000000000000000000000000000000000000000000000"sv;
inline constexpr auto bearer =
    "Bearer 0000000000000000000000000000000000000000000000000000000000000000"sv;
inline constexpr auto cookie =
    "__Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000"sv;
inline constexpr auto desktop =
    "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
    "Authorization: Bearer 0000000000000000000000000000000000000000000000000000000000000000\r\n\r\n"sv;
inline constexpr auto a =
    "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
    "Authorization: Bearer 0000000000000000000000000000000000000000000000000000000000000000\r\n"
    "Authorization: Bearer 0000000000000000000000000000000000000000000000000000000000000000\r\n\r\n"sv;
inline constexpr auto b =
    "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
    "Cookie: "
    "__Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000\r\n"
    "Origin: https://timer.example\r\noRiGiN: https://timer.example\r\nX-Subathon-CSRF: 1\r\n\r\n"sv;
inline constexpr auto c =
    "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
    "Cookie: "
    "__Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000\r\n"
    "X-Subathon-CSRF: 1\r\nx-subathon-csrf: 1\r\n\r\n"sv;
inline constexpr auto d =
    "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
    "Cookie: "
    "__Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000\r\n"
    "Cookie: "
    "__Host-subathon_session=0000000000000000000000000000000000000000000000000000000000000000\r\n"
    "X-Subathon-CSRF: 1\r\n\r\n"sv;
static_assert(desktop.size() == 149 && a.size() == 237 && b.size() == 241 && c.size() == 199 &&
              d.size() == 277);
struct Field {
    std::string_view name, value;
};
inline constexpr std::array fields_desktop{Field{"Host", "timer.example"},
                                           Field{"Authorization", bearer}};
inline constexpr std::array fields_a{Field{"Host", "timer.example"}, Field{"Authorization", bearer},
                                     Field{"Authorization", bearer}};
inline constexpr std::array fields_b{Field{"Host", "timer.example"}, Field{"Cookie", cookie},
                                     Field{"Origin", "https://timer.example"},
                                     Field{"oRiGiN", "https://timer.example"},
                                     Field{"X-Subathon-CSRF", "1"}};
inline constexpr std::array fields_c{Field{"Host", "timer.example"}, Field{"Cookie", cookie},
                                     Field{"X-Subathon-CSRF", "1"}, Field{"x-subathon-csrf", "1"}};
inline constexpr std::array fields_d{Field{"Host", "timer.example"}, Field{"Cookie", cookie},
                                     Field{"Cookie", cookie}, Field{"X-Subathon-CSRF", "1"}};
} // namespace account_capture_test
