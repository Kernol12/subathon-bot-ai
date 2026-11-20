// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/contribution_review.hpp>

namespace subathon::server {

MigrationManifest contribution_review_repository_manifest() {
    throw ContributionReviewNotImplemented("contribution_review_repository_manifest");
}

void ContributionReview::preview(ReviewPreviewRequest, std::string,
                                 std::function<void(PreviewResult)>) {
    throw ContributionReviewNotImplemented("preview");
}

void ContributionReview::execute(ReviewCommand, std::string, std::function<void(Result)>) {
    throw ContributionReviewNotImplemented("execute");
}

void ContributionReview::lookup(ApplicationOperationKey, ApplicationPrincipalBinding, std::string,
                                std::function<void(Result)>) {
    throw ContributionReviewNotImplemented("lookup");
}

} // namespace subathon::server
