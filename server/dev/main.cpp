// SPDX-License-Identifier: GPL-3.0-or-later
// Development walking skeleton only; this is not the production server host.
#include "development_setup.hpp"
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <limits>

namespace {
using namespace subathon;
using namespace subathon::server;
struct Options {
    QString database;
    std::int64_t start_ms = 0, cap_ms = 0, add_ms = 0;
    std::vector<std::string> events;
    bool controlled = false;
};

// Complete CLI preflight precedes opening the writer, including all event IDs.
std::optional<Options> parse(int argc, char **argv) {
    Options o;
    bool database = false, start = false, cap = false, add = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view key(argv[i]);
        if (key == "--controlled-clock") {
            if (o.controlled)
                return {};
            o.controlled = true;
            continue;
        }
        if (++i == argc)
            return {};
        const std::string_view value(argv[i]);
        if (key == "--event") {
            if (value.empty() || value.size() > ReferenceAdapterLimits{}.reference_bytes)
                return {};
            o.events.emplace_back(value);
            continue;
        }
        if (key == "--database") {
            if (database || value.empty())
                return {};
            database = true;
            o.database = QString::fromLocal8Bit(argv[i]);
            continue;
        }
        auto number = [&](std::int64_t &out, bool &seen) {
            if (seen || value.empty() || value.front() < '0' || value.front() > '9')
                return false;
            const auto r = std::from_chars(value.data(), value.data() + value.size(), out);
            seen = true;
            return r.ec == std::errc{} && r.ptr == value.data() + value.size();
        };
        if (key == "--start-ms" ? !number(o.start_ms, start)
            : key == "--cap-ms" ? !number(o.cap_ms, cap)
            : key == "--add-ms" ? !number(o.add_ms, add)
                                : true)
            return {};
    }
    if (!database || !start || !cap || !add || o.cap_ms < o.start_ms)
        return {};
    return o;
}

bool normal(const SqliteWorkResult &r) {
    if (r.transaction == TransactionOutcome::committed && r.error == WorkError::none && r.value)
        return true;
    std::fprintf(stderr, "Writer completion: transaction=%d, error=%d\n", int(r.transaction),
                 int(r.error));
    return false;
}
struct Cleanup {
    SqliteWorkSession session;
    ~Cleanup() { (void)session.cleanup(); }
};

// All stages and completions run on QCoreApplication's one authoritative queue.
// No event pumping, second writer, raw SQL, provider or alternate application path.
class Run final {
    template <class F> auto guarded(F f) {
        return [this, f = std::move(f)](auto r) mutable {
            try {
                f(std::move(r));
            } catch (const std::exception &e) {
                std::fprintf(stderr, "%s\n", e.what());
                finish(1);
            } catch (...) {
                finish(1);
            }
        };
    }

  public:
    explicit Run(Options o)
        : options_(std::move(o)), sessions_(options_.controlled),
          clock_(options_.controlled ? static_cast<ReferenceClockSampler &>(controlled_clock_)
                                     : static_cast<ReferenceClockSampler &>(system_clock_)),
          facts_(dev::configuration(options_.add_ms, options_.cap_ms)),
          archive_({{facts_.configuration_ref, facts_}}) {}

    void start() {
        const bool existing = QFileInfo::exists(options_.database);
        // Active/Main-positive is the reference runtime's supported startup profile.
        // On reopen start_ms is ignored, including zero.
        if (!existing && options_.start_ms == 0) {
            finish(3);
            return;
        }
        if (existing && !options_.events.empty()) {
            finish(3);
            return;
        }
        // The complete compiled catalog and authority closure exceed the writer's
        // small batch defaults; retain finite cumulative budgets for each unit.
        StorageLimits budget;
        budget.statements_per_batch = 16384;
        budget.statement_utf8_bytes = 4 * 1024 * 1024;
        budget.bound_value_bytes = 16 * 1024 * 1024;
        budget.result_rows = 65536;
        budget.result_value_bytes = 16 * 1024 * 1024;
        if (writer_.open(options_.database, budget) != StorageError::none) {
            finish(1);
            return;
        }
        opened_ = true;
        if (existing) {
            intake_ = make_intake();
            snapshot([this](ReferenceSnapshot s) { print(s); });
            return;
        }
        const auto admission = SqliteMigrationRunner::enqueue(
            writer_, {MigrationMode::advance, reference_authority_repository_manifest(), {}, {}},
            guarded([this](MigrationResult r) {
                if (!normal(r.work) || r.reason != MigrationReason::none || !r.report) {
                    std::fprintf(stderr, "Migration reason=%d\n", int(r.reason));
                    finish(1);
                    return;
                }
                provision();
            }));
        if (admission.error != StorageError::none || admission.reason != MigrationReason::none)
            finish(1);
    }

    void finish(int code) {
        if (finishing_)
            return;
        finishing_ = true;
        if (code)
            std::fprintf(stderr, "Development server failed (exit %d)\n", code);
        auto done = [this, code] {
            if (code == 0 && std::fwrite(output_.constData(), 1, std::size_t(output_.size()),
                                         stdout) != std::size_t(output_.size())) {
                QCoreApplication::exit(1);
                return;
            }
            QCoreApplication::exit(code);
        };
        if (!opened_)
            done();
        else if (writer_.shutdown(std::move(done)) != StorageError::none)
            QCoreApplication::exit(1);
    }

  private:
    void enqueue(ReferencePreparedWork p) {
        if (writer_.enqueue_work(std::move(p.body), std::move(p.completion)) != StorageError::none)
            finish(1);
    }
    std::unique_ptr<ReferenceIntake> make_intake() {
        return std::make_unique<ReferenceIntake>(
            archive_, clock_, entropy_, sessions_,
            ReferenceConnectorRegistration{"development-connection",
                                           facts_.projection.synthetic_origin,
                                           facts_.projection.receipt_namespace},
            runtime_.get());
    }
    void provision() {
        if (writer_.enqueue_work(
                [this](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    try {
                        dev::provision(s, sessions_, facts_.projection);
                    } catch (const std::exception &e) {
                        std::fprintf(stderr, "Development provisioning: %s\n", e.what());
                        throw;
                    }
                    s.commit();
                    return true;
                },
                guarded([this](SqliteWorkResult r) {
                    if (!normal(r)) {
                        finish(1);
                        return;
                    }
                    initialize();
                })) != StorageError::none)
            finish(1);
    }
    void initialize() {
        startup_intake_ = make_intake();
        ReferenceInitialRequest request{dev::owner_session(),
                                        facts_.configuration_ref,
                                        "development-initial-state",
                                        "development-clock-lineage",
                                        "development-initial-observation",
                                        {core::TimerLifecycle::Active, core::CountdownPhase::Main,
                                         options_.start_ms,
                                         facts_.projection.timer_config.grace_duration_ms, false}};
        enqueue(startup_intake_->make_initial_work(
            request, guarded([this](ReferenceInitialCompletion r) {
                if (!normal(r.work) || !r.anchor) {
                    finish(r.refusal ? 3 : 1);
                    return;
                }
                current_ = r.anchor->committed;
                runtime_ = std::make_unique<ReferenceApplicationAdapters>(
                    archive_, authority_, clock_, entropy_, std::move(r.anchor));
                ports_ = std::make_unique<ApplicationServicePorts>(
                    ApplicationServicePorts{*runtime_, *runtime_, *runtime_, *runtime_, *runtime_});
                service_ = std::make_unique<OriginalApplicationService>(writer_, *ports_);
                intake_ = make_intake();
                next_event();
            })));
    }
    void next_event() {
        if (event_ == options_.events.size()) {
            snapshot([this](ReferenceSnapshot s) { print(s); });
            return;
        }
        enqueue(intake_->make_receipt_work(
            facts_.configuration_ref,
            {"development-connection", options_.events[event_], {}, QByteArray("", 0)},
            guarded([this](ReferenceReceiptCompletion r) {
                if (!normal(r.work) || !r.receipt) {
                    finish(r.refusal ? 3 : 1);
                    return;
                }
                evidence(r.receipt->receipt_id);
            })));
    }
    void evidence(core::ReceiptId receipt) {
        enqueue(intake_->make_evidence_work(
            receipt, guarded([this, receipt](ReferenceEvidenceCompletion r) {
                if (!normal(r.work) || !r.evidence || !r.evidence->contribution_id) {
                    finish(r.refusal ? 3 : 1);
                    return;
                }
                enqueue(intake_->make_capture_work(receipt, *r.evidence->contribution_id,
                                                   guarded([this](ReferenceCaptureCompletion c) {
                                                       if (!normal(c.work) || !c.capture) {
                                                           finish(c.refusal ? 3 : 1);
                                                           return;
                                                       }
                                                       apply(*c.capture);
                                                   })));
            })));
    }
    void apply(const ReferenceInitialCapture &capture) {
        OriginalApplicationCommand command;
        command.operation = {"development-run", std::to_string(event_)};
        command.receipt_id = capture.receipt_id;
        command.contribution_id = capture.contribution_id;
        command.plan = capture.plan;
        command.principal = facts_.projection.rule.automation;
        command.choices = {encode_reference_execution_ref(capture.execution_context_ref)};
        auto p = service_->make_work(
            command, guarded([this](ApplicationServiceCompletion r) {
                if (reservation_refused_) {
                    finish(3);
                    return;
                }
                if (r.transaction != TransactionOutcome::committed ||
                    r.work_error != WorkError::none || r.failure || !r.response) {
                    finish(1);
                    return;
                }
                const auto disposition = r.response->disposition;
                if (disposition != ApplicationServiceDisposition::applied &&
                    disposition != ApplicationServiceDisposition::existing_operation &&
                    disposition != ApplicationServiceDisposition::subject_claimed &&
                    disposition != ApplicationServiceDisposition::source_claimed) {
                    runtime_->settle(r, {});
                    finish(3);
                    return;
                }
                record(r);
                snapshot([this, r = std::move(r)](ReferenceSnapshot s) {
                    runtime_->settle(r, s.committed);
                    current_ = std::move(s.committed);
                    ++event_;
                    next_event();
                });
            }));
        auto body = std::move(p.body);
        p.body = [this, command = std::move(command), body = std::move(body)](auto context) {
            // FIFO wait has ended; reserve immediately before the real service body.
            reservation_refused_ = runtime_->reserve(current_, command);
            if (reservation_refused_)
                throw std::runtime_error("Reference clock reservation refused");
            return body(context);
        };
        if (writer_.enqueue_work(std::move(p.body), std::move(p.completion)) != StorageError::none)
            finish(1);
    }
    void record(const ApplicationServiceCompletion &r) {
        const auto &bundle = std::get<RecordedApplicationBundle>(r.response->payload);
        if (bundle.effects.size() != 1)
            throw std::runtime_error("Expected one captured time effect");
        const auto &effect = bundle.effects.front();
        const auto actual =
            std::find_if(effect.components.begin(), effect.components.end(), [](const auto &c) {
                return c.kind == ApplicationComponentKind::main_delta;
            });
        if (actual == effect.components.end())
            throw std::runtime_error("Missing committed time effect");
        const bool applied = r.response->disposition == ApplicationServiceDisposition::applied;
        if (applied) {
            ++applications_;
            const auto elapsed = bundle.persisted_head.timer.main_remaining_ms -
                                 bundle.advanced_before.timer.main_remaining_ms;
            if (elapsed < 0 || elapsed_ms_ > std::numeric_limits<std::int64_t>::max() - elapsed)
                throw std::runtime_error("Elapsed milliseconds out of range");
            elapsed_ms_ += elapsed;
        }
        events_.append(QJsonObject{{"event_id", QString::fromStdString(options_.events[event_])},
                                   {"receipt_id", QString::number(bundle.intent.receipt_id.value)},
                                   {"duplicate", !applied},
                                   {"requested_ms", qint64(effect.requested_units)},
                                   {"actual_ms", qint64(actual->units)}});
    }
    template <class F> void snapshot(F f) {
        enqueue(intake_->make_snapshot_work(
            dev::owner_session(), guarded([this, f = std::move(f)](ReferenceSnapshotCompletion r) {
                if (!normal(r.work) || !r.snapshot) {
                    finish(r.refusal ? 3 : 1);
                    return;
                }
                f(std::move(*r.snapshot));
            })));
    }
    void print(const ReferenceSnapshot &s) {
        output_ =
            QJsonDocument(
                QJsonObject{{"clock", options_.controlled ? "controlled" : "system"},
                            {"timer_ms", qint64(s.committed.snapshot.timer.main_remaining_ms)},
                            {"elapsed_ms", qint64(elapsed_ms_)},
                            {"applications", applications_},
                            {"events", events_}})
                .toJson(QJsonDocument::Compact) +
            '\n';
        finish(0);
    }

    Options options_;
    SqliteWriter writer_;
    dev::DevelopmentSessionClock sessions_;
    dev::ControlledReferenceClock controlled_clock_;
    SystemReferenceClockSampler system_clock_;
    ReferenceClockSampler &clock_;
    OsReferenceEntropy entropy_;
    ReferenceConfigurationFacts facts_;
    ReferenceConfigurationArchive archive_;
    SqliteReferenceAuthoritySource authority_;
    std::unique_ptr<ReferenceApplicationAdapters> runtime_;
    std::unique_ptr<ApplicationServicePorts> ports_;
    std::unique_ptr<OriginalApplicationService> service_;
    std::unique_ptr<ReferenceIntake> startup_intake_, intake_;
    ReferenceCommittedState current_;
    std::optional<ReferenceUnavailable> reservation_refused_;
    std::size_t event_ = 0;
    qint64 applications_ = 0;
    std::int64_t elapsed_ms_ = 0;
    QJsonArray events_;
    QByteArray output_;
    bool opened_ = false, finishing_ = false;
};
} // namespace

int main(int argc, char **argv) {
    try {
        const auto options = parse(argc, argv);
        if (!options) {
            std::fputs("Invalid development server arguments\n", stderr);
            return 2;
        }
        QCoreApplication app(argc, argv);
        Run run(*options);
        QTimer::singleShot(0, &app, [&] {
            try {
                run.start();
            } catch (const std::exception &e) {
                std::fprintf(stderr, "%s\n", e.what());
                run.finish(1);
            }
        });
        return app.exec();
    } catch (const std::exception &e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
