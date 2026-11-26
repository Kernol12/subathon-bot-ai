// SPDX-License-Identifier: GPL-3.0-or-later
#include "astro_fixture_api.hpp"

astro_fixture::Status astro_fixture::ingest(std::string_view, std::uint64_t,
    const AcknowledgedRoute &, const subathon::core::ReceiptArrival &,
    subathon::core::EvidencePolicy, HoldOwner &, CoverageJournal &) {
    return Status::not_implemented;
}


astro_fixture::TransportResult astro_fixture::advance(TransportState &, TransportInput,
                                                      CoverageJournal &) {
    return {TransportStatus::not_implemented, std::nullopt};
}
