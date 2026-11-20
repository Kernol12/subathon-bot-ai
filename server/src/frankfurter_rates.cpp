// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/frankfurter_rates.hpp>

namespace subathon::server {

FrankfurterRequestResult make_frankfurter_request(const FrankfurterRequest &) {
    throw FrankfurterNotImplemented("make_frankfurter_request");
}

FrankfurterFetchResult normalize_frankfurter_response(const FrankfurterHttpRequest &,
                                                      const FrankfurterHttpResult &) {
    throw FrankfurterNotImplemented("normalize_frankfurter_response");
}

FrankfurterFetchResult FrankfurterAdapter::fetch(const FrankfurterRequest &) const {
    throw FrankfurterNotImplemented("FrankfurterAdapter::fetch");
}

ConnectorReport FrankfurterAdapter::report() const {
    throw FrankfurterNotImplemented("FrankfurterAdapter::report");
}

FrankfurterLatestResult propose_frankfurter_latest(const FrankfurterFetch &,
                                                   const FrankfurterRequest &,
                                                   const core::EffectiveRateSnapshot &,
                                                   const RateSourceBasis &, const std::string &) {
    throw FrankfurterNotImplemented("propose_frankfurter_latest");
}

FrankfurterFreshness assess_frankfurter_freshness(const FrankfurterFreshnessInput &) {
    throw FrankfurterNotImplemented("assess_frankfurter_freshness");
}

} // namespace subathon::server
