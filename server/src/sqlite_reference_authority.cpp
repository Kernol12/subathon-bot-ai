// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_reference_read_state.hpp"
#include <subathon/server/sqlite_reference_authority.hpp>
namespace subathon::server {
class SqliteReferenceAuthorityUnit::Impl {};
MigrationManifest reference_authority_repository_manifest() {
    throw ReferenceAuthorityNotImplemented();
}
SqliteReferenceAuthorityUnit::SqliteReferenceAuthorityUnit(const SqliteWorkSession &,
                                                           ReferenceAuthorityLimits) {
    throw ReferenceAuthorityNotImplemented();
}
SqliteReferenceAuthorityUnit::~SqliteReferenceAuthorityUnit() = default;
ReferenceAuthorityWrite SqliteReferenceAuthorityUnit::bootstrap(const SessionLookupRequest &,
                                                                CurrentSessionClock &) {
    throw ReferenceAuthorityNotImplemented();
}
ReferenceAuthorityWrite SqliteReferenceAuthorityUnit::enroll(const SessionLookupRequest &,
                                                             CurrentSessionClock &, std::uint64_t,
                                                             const ReferenceAuthorityBinding &,
                                                             std::string_view) {
    throw ReferenceAuthorityNotImplemented();
}
ReferenceAuthorityWrite SqliteReferenceAuthorityUnit::set_state(const SessionLookupRequest &,
                                                                CurrentSessionClock &,
                                                                std::uint64_t, std::string_view,
                                                                ReferenceAuthorityState) {
    throw ReferenceAuthorityNotImplemented();
}
ReferenceAuthorityRead SqliteReferenceAuthorityUnit::read(std::string_view, std::string_view) {
    throw ReferenceAuthorityNotImplemented();
}
ReferenceAuthorityResolution
SqliteReferenceAuthorityUnit::resolve(const ReferenceAuthorityRequest &) {
    throw ReferenceAuthorityNotImplemented();
}
SqliteReferenceAuthoritySource::SqliteReferenceAuthoritySource(ReferenceAuthorityLimits) {
    throw ReferenceAuthorityNotImplemented();
}
ReferenceAuthorityResolution
SqliteReferenceAuthoritySource::resolve(const ApplicationReferenceReadView &,
                                        const ReferenceAuthorityRequest &) {
    throw ReferenceAuthorityNotImplemented();
}
} // namespace subathon::server

namespace subathon::server {
ReferenceAuthorityResolution
ApplicationReferenceReadView::resolve_reference_authority(const ReferenceAuthorityRequest &,
                                                          const ReferenceAuthorityLimits &) const {
    throw ReferenceAuthorityNotImplemented();
}
} // namespace subathon::server
