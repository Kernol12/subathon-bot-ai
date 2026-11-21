// SPDX-License-Identifier: GPL-3.0-or-later
#include "stage1_boundary.hpp"

#include <subathon/server/testing/diagnostics.hpp>

namespace release_002_stage1 {
namespace {
namespace diag = subathon::server::diagnostics;
using Control = diag::testing::LogControl;

class Current final : public diag::Current {
  public:
    Current(Ports &ports, Gate gate) : ports_(ports), gate_(gate) {}

    std::optional<diag::Session> session() override {
        const auto session = ports_.current_session(gate_, false);
        if (!session)
            return {};
        const auto &s = session->scope;
        return diag::Session{
            {s.installation, s.actor, s.session}, session->guarded_utc_ms, session->expires_utc_ms};
    }
    diag::Use use(const diag::Scope &s, const subathon::core::PermissionRequest &request) override {
        switch (ports_.current_use(gate_, {s.installation, s.actor, s.session}, request)) {
        case Use::allowed:
            return diag::Use::allowed;
        case Use::denied:
            return diag::Use::denied;
        case Use::unavailable:
            return diag::Use::unavailable;
        }
        return diag::Use::unavailable;
    }
    diag::Attempt attempt(const diag::Scope &s, const diag::Selector &selector) override {
        Selector input{selector.receipt, selector.attempt, {}};
        for (auto field : selector.fields)
            input.fields.push_back(field == diag::Field::disposition ? Field::disposition
                                                                     : Field::schema_version);
        auto result = ports_.read_attempt(gate_, {s.installation, s.actor, s.session}, input);
        if (!diag::normally_acknowledged(result.completion))
            throw subathon::server::SqliteWorkFailure(
                subathon::server::WorkError::uncertain_outcome);
        return std::move(result.values);
    }

  private:
    Ports &ports_;
    Gate gate_;
};

struct Settlement {
    const std::function<diag::MutationOutcome()> *immediate;
    std::optional<diag::MutationOutcome> outcome;
};

class Boundary final : public Stage1Boundary {
  public:
    Boundary(QString directory, Ports ports)
        : ports_(std::move(ports)), logs_(control().open(std::move(directory))),
          connections_(*logs_), export_(*logs_, connections_, [&] {
              const auto seen = ports_.versions();
              return diag::Versions{version(seen.server), version(seen.qt_runtime)};
          }) {}

    void startup() override { sweep(); }
    void sweep() override { Control::sweep(*logs_); }
    void host_entered_serving() override { logs_->host_entered_serving(); }
    void register_adapter(AdapterId id, subathon::server::ConnectorProvider provider) override {
        connections_.register_adapter(id, provider);
    }
    void report(AdapterId id, const subathon::server::ConnectorAdapter &adapter,
                const subathon::server::ConnectorSetupContext &) override {
        connections_.report(id, adapter);
    }
    // The negative fixture describes a foreign shape. There is no typed
    // server ingress accepting those members.
    void submit_connector_candidate(AdapterId, ExtraMembers) override {}
    void drain_observations() override { Control::drain(*logs_); }
    std::vector<LogNotice> pending_notices() const override {
        std::vector<LogNotice> result;
        for (auto notice : Control::notices(*logs_))
            result.push_back(notice == diag::Notice::capacity_reached
                                 ? LogNotice::capacity_reached
                                 : LogNotice::retention_unavailable);
        return result;
    }

    ExportId prepare(std::optional<QByteArray> selector) override {
        const auto id =
            export_.begin(selector ? std::optional(QByteArrayView(*selector)) : std::nullopt);
        if (const auto *selection = export_.selection(id)) {
            try {
                if (!export_.stage(id, read(Gate::preparation)(*selection, nullptr)))
                    return {0};
            } catch (...) {
                export_.cancel(id);
                return {0};
            }
        }
        Control::barrier(*logs_);
        return {id};
    }
    void handoff(ExportId id, const Transfer &transfer) override {
        export_.handoff(
            id.value, read(Gate::handoff), [](auto) { return true; },
            [&](diag::Reply reply) {
                transfer({static_cast<ReplyStatus>(reply.status), std::move(reply.artifact)});
            });
    }

  private:
    static std::optional<diag::Version> version(std::optional<VersionTuple> tuple) {
        if (!tuple)
            return {};
        return diag::Version{tuple->major, tuple->minor, tuple->patch};
    }
    diag::ReadGate read(Gate gate) {
        return [this, gate](const auto &selectors, const diag::Scope *scope) {
            Current current(ports_, gate);
            return diag::read_gate(current, selectors, scope);
        };
    }
    Control control() {
        return {ports_.log_utc, [this](diag::Mutation kind, const auto &operation) {
                    auto state = std::make_shared<Settlement>(Settlement{&operation, {}});
                    ports_.mutate(
                        static_cast<Mutation>(kind),
                        [this, state] {
                            const auto outcome = state->immediate ? (*state->immediate)()
                                                                  : Control::run_pending(*logs_);
                            return static_cast<MutationOutcome>(outcome);
                        },
                        [this, state](MutationOutcome outcome) {
                            const auto mapped = static_cast<diag::MutationOutcome>(outcome);
                            if (state->immediate)
                                state->outcome = mapped;
                            else
                                Control::settle(*logs_, mapped);
                        });
                    state->immediate = nullptr;
                    return state->outcome;
                }};
    }

    Ports ports_;
    std::unique_ptr<diag::LogOwner> logs_;
    diag::Connections connections_;
    diag::Export export_;
};
} // namespace

std::unique_ptr<Stage1Boundary> make_stage1_boundary(QString directory, Ports ports) {
    return std::make_unique<Boundary>(std::move(directory), std::move(ports));
}
} // namespace release_002_stage1
