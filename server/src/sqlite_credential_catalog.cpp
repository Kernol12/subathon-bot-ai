// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_credential_catalog.hpp>
#include <subathon/server/sqlite_financial_refunds.hpp>

namespace subathon::server {

MigrationManifest provider_credentials_repository_manifest() {
    // Stage 1 stand-in. Keep the protected v15 catalog linkable for the producer witness.
    return financial_refunds_repository_manifest();
}

} // namespace subathon::server
