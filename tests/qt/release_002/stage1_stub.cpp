// SPDX-License-Identifier: GPL-3.0-or-later
#include "stage1_boundary.hpp"

namespace release_002_stage1 {
namespace {
class NotImplemented final : public Stage1Boundary {
  public:
    void startup() override {}
    void sweep() override {}
    void host_entered_serving() override {}
    void register_adapter(AdapterId, subathon::server::ConnectorProvider) override {}
    void report(AdapterId, const subathon::server::ConnectorAdapter &,
                const subathon::server::ConnectorSetupContext &) override {}
    void submit_connector_candidate(AdapterId, ExtraMembers) override {}
    void drain_observations() override {}
    std::vector<LogNotice> pending_notices() const override { return {}; }
    ExportId prepare(std::optional<QByteArray>) override { return {0}; }
    void handoff(ExportId, const Transfer &) override {}
};
} // namespace

std::unique_ptr<Stage1Boundary> make_stage1_boundary(QString, Ports) {
    return std::make_unique<NotImplemented>();
}
} // namespace release_002_stage1
