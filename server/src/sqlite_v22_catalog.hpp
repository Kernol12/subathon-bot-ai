// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <optional>
#include <span>
#include <string_view>

namespace subathon::server::detail {

struct V22CatalogStepIdentity {
    std::string_view id;
    std::string_view content_sha256_hex;
};

struct V22CatalogObject {
    std::string_view type;
    std::string_view name;
    std::string_view table_name;
    std::optional<std::string_view> sql;
};

struct V22CatalogIdentity {
    int format;
    std::string_view store;
    std::array<V22CatalogStepIdentity, 22> steps;
    int projection_objects;
    std::string_view prefix_sha256_hex;
};

// The entry index plus one is the version. Its content digest binds the
// ordered SQL and typed projection, including SQL NULL. The prefix hashes
// the concatenated raw step digests in version order.
[[nodiscard]] const V22CatalogIdentity &sqlite_v22_catalog() noexcept;
// Canonical tuple order. A disengaged sql field represents SQLite NULL.
[[nodiscard]] std::span<const V22CatalogObject> sqlite_v22_projection() noexcept;

} // namespace subathon::server::detail
