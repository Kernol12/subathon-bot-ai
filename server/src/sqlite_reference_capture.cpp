// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_reference_capture.hpp>

namespace subathon::server {
MigrationManifest reference_capture_repository_manifest() {
    throw SqliteReferenceCaptureNotImplemented{};
}
std::optional<ReferenceInitialCapture>
SqliteReferenceCaptureStore::find(const ReferenceCaptureKey &) const {
    throw SqliteReferenceCaptureNotImplemented{};
}
ReferenceCaptureRetainDisposition
SqliteReferenceCaptureStore::retain_initial(const ReferenceInitialCapture &) const {
    throw SqliteReferenceCaptureNotImplemented{};
}
} // namespace subathon::server
