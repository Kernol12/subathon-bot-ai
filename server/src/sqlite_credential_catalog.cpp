// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <optional>
#include <subathon/server/sqlite_credential_catalog.hpp>
#include <subathon/server/sqlite_financial_refunds.hpp>
#include <tuple>
#include <utility>

namespace subathon::server {

MigrationManifest provider_credentials_repository_manifest() {
    auto manifest = financial_refunds_repository_manifest();
    const QString table = "subathon_provider_credentials";
    const QString ddl =
        "CREATE TABLE subathon_provider_credentials(scope BLOB PRIMARY KEY NOT NULL "
        "CHECK(typeof(scope)='blob'), record BLOB NOT NULL CHECK(typeof(record)='blob'))";
    MigrationStep step{16, "provider-credentials-v1", {ddl}, manifest.steps.back().expected_schema};
    step.expected_schema.append({"table", table, table, ddl});
    step.expected_schema.append(
        {"index", "sqlite_autoindex_subathon_provider_credentials_1", table, std::nullopt});
    std::ranges::sort(step.expected_schema, [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(16);
    return manifest;
}

} // namespace subathon::server
