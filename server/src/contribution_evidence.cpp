// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/contribution_evidence.hpp>

namespace subathon::server {
ContributionLinkResult ContributionEvidenceService::observe(const ContributionObservation &) {
    throw ContributionEvidenceNotImplemented{};
}
} // namespace subathon::server
