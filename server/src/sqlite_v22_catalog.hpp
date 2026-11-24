// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <string_view>

namespace subathon::server::detail {

struct V22CatalogStepIdentity {
    std::string_view id;
    std::string_view content_sha256_hex;
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

} // namespace subathon::server::detail
