// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace subathon::server::detail {

struct V23SchemaObject {
    std::string_view type;
    std::string_view name;
    std::string_view table_name;
    std::optional<std::string_view> sql;
};

struct V23StepData {
    int version;
    std::string_view id;
    std::span<const std::string_view> statements;
    std::span<const V23SchemaObject> projection;
    std::string_view step_sha256_hex;
    std::string_view prefix_sha256_hex;
};

[[nodiscard]] const V23StepData &sqlite_v23_step_data() noexcept;

} // namespace subathon::server::detail
