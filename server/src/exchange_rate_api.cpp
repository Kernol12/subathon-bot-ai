// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/exchange_rate_api.hpp>

namespace subathon::server::exchange_rate_api {

// Stage 1 stop: these entry points deliberately admit no selection or dispatch.
SelectionStatus Adapter::select() { return SelectionStatus::not_implemented; }
SelectionStatus Adapter::edit_reference_allowance(std::optional<std::uint64_t>) {
    return SelectionStatus::not_implemented;
}
std::optional<SelectedProfile> Adapter::current() const { return std::nullopt; }
StartResult Adapter::start(const Request &, const Clocks &) {
    return Failure{FailureKind::not_implemented};
}
Completion Adapter::finish(const Started &started, Reply) {
    return {started.admission, started.dispatch, Failure{FailureKind::not_implemented}};
}
QuotaView Adapter::quota_view(const Scope &, const Clocks &) const { return {}; }

} // namespace subathon::server::exchange_rate_api
