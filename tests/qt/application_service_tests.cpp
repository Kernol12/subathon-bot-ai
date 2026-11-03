// SPDX-License-Identifier: GPL-3.0-or-later
// PUBLIC orchestration acceptance: independent frozen DATA, real writer/completion.
// No fixture/setup calls the new codec, service or reference view to make an oracle.
#include "support/application_service_fixture.hpp"
#include <QRegularExpression>
#include <QTemporaryDir>
#include <algorithm>
#include <limits>
#include <subathon/server/sqlite_application_plans.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>

using namespace application_service_fixture;
namespace {
using D = ApplicationServiceDisposition;
using IE = ApplicationServiceInputError;
using LR = ApplicationServiceLocalReason;

void rethrow_marker(const std::exception_ptr &error) {
    if (!error)
        return;
    try {
        std::rethrow_exception(error);
    } catch (const OriginalApplicationServiceNotImplemented &) {
        throw;
    } catch (const OriginalApplicationCommandEncodeNotImplemented &) {
        throw;
    } catch (const OriginalApplicationCommandDecodeNotImplemented &) {
        throw;
    } catch (const ApplicationReferenceReadNotImplemented &) {
        throw;
    } catch (...) {
    }
}
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const OriginalApplicationServiceNotImplemented &) {
        QTest::qFail("MISSING_SERVICE: OriginalApplicationServiceNotImplemented", __FILE__,
                     __LINE__);
    } catch (const OriginalApplicationCommandEncodeNotImplemented &) {
        QTest::qFail("MISSING_ENCODE: OriginalApplicationCommandEncodeNotImplemented", __FILE__,
                     __LINE__);
    } catch (const OriginalApplicationCommandDecodeNotImplemented &) {
        QTest::qFail("MISSING_DECODE: OriginalApplicationCommandDecodeNotImplemented", __FILE__,
                     __LINE__);
    } catch (const ApplicationReferenceReadNotImplemented &) {
        QTest::qFail("MISSING_VIEW: ApplicationReferenceReadNotImplemented", __FILE__, __LINE__);
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
template <class Body> void input_failure(Body body, IE expected) {
    bool caught = false;
    try {
        body();
    } catch (const ApplicationServiceInputFailure &e) {
        require(e.error() == expected, "Exact public pure-input error priority");
        caught = true;
    }
    require(caught, "Required pure service/codec failure was missing");
}
void normal(const ApplicationServiceCompletion &c) {
    require(c.transaction == TransactionOutcome::committed && c.work_error == WorkError::none &&
                !c.failure && c.response,
            "Only acknowledged COMMIT plus normal typed work completion is a normal response");
}
void disposition(const ApplicationServiceCompletion &c, D expected) {
    normal(c);
    require(c.response->disposition == expected, "Exact public service disposition");
}
void no_payload(const ApplicationServiceCompletion &c, D expected) {
    disposition(c, expected);
    require(std::holds_alternative<std::monostate>(c.response->payload),
            "Refusal exposes no protected payload");
}
void known(const ApplicationServiceCompletion &c) {
    require(c.transaction == TransactionOutcome::rolled_back && !c.response && c.failure,
            "Actual known whole-unit rollback, no normal service payload");
}
void local_failure(const ApplicationServiceCompletion &c, ApplicationServiceFailureSource source,
                   LR reason) {
    known(c);
    require(c.failure->source == source && std::holds_alternative<LR>(c.failure->reason) &&
                std::get<LR>(c.failure->reason) == reason,
            "Exact bounded local service failure without invented writer truth");
}
void not_ready(const ApplicationServiceCompletion &c) {
    disposition(c, D::not_ready);
    require(std::holds_alternative<ApplicationNotReadyPayload>(c.response->payload) &&
                !std::get<ApplicationNotReadyPayload>(c.response->payload).reason_ref.empty(),
            "Explicit bounded not-ready reason, no imaginary success or hold");
}
void check_proposal(const ApplicationCalculatedProposal &a,
                    const ApplicationCalculatedProposal &b) {
    require(same_timer(a.clock_advanced_timer, b.clock_advanced_timer) &&
                same_timer(a.timer_after, b.timer_after) && a.debt_after == b.debt_after &&
                a.balances_after.size() == b.balances_after.size() &&
                a.effects.size() == b.effects.size() && a.transitions == b.transitions,
            "Complete independent token-free ordered numeric proposal");
    for (size_t n = 0; n < b.effects.size(); ++n)
        check_effect(a.effects[n], b.effects[n]);
    for (size_t n = 0; n < b.balances_after.size(); ++n)
        require(a.balances_after[n].kind == b.balances_after[n].kind &&
                    a.balances_after[n].target_ref == b.balances_after[n].target_ref &&
                    a.balances_after[n].units == b.balances_after[n].units,
                "Complete proposed balance vector");
    const auto &x = a.observation, &y = b.observation;
    require(x.clock_lineage_ref == y.clock_lineage_ref && x.observation_ref == y.observation_ref &&
                x.configuration_ref == y.configuration_ref &&
                x.policy_revision_ref == y.policy_revision_ref &&
                x.checked_head_token == y.checked_head_token &&
                x.observed_utc_ms == y.observed_utc_ms && x.segments.size() == y.segments.size(),
            "Actual bound observation, not a token/UTC guess");
    for (size_t n = 0; n < y.segments.size(); ++n)
        require(x.segments[n].elapsed_ms == y.segments[n].elapsed_ms &&
                    x.segments[n].utc_now_ms == y.segments[n].utc_now_ms &&
                    x.segments[n].paused == y.segments[n].paused,
                "Every constant-pause segment");
}

void check_observation(const ApplicationHoldObservation &a, const ApplicationHoldObservation &b) {
    require(a.persisted_state_token == b.persisted_state_token &&
                a.policy_revision_ref == b.policy_revision_ref &&
                a.permission_revision_ref == b.permission_revision_ref &&
                a.actor_ref == b.actor_ref && a.decision_ref == b.decision_ref &&
                a.observed_utc_ms == b.observed_utc_ms && a.elapsed_ms == b.elapsed_ms,
            "Every typed known/unknown hold observation member");
    original_claim_fixture::same_envelope(a.annotation, b.annotation);
}
void check_creation(const ApplicationHoldView &a, const ApplicationHoldDetection &b) {
    const auto &x = a.cause, &y = b.cause;
    require(x.hold_ref == y.hold_ref && x.source.subject.id == y.source.subject.id &&
                x.source.receipt_id == y.source.receipt_id &&
                x.source.event_ref == y.source.event_ref &&
                x.source.parent_event_ref == y.source.parent_event_ref &&
                x.source.root_event_ref == y.source.root_event_ref &&
                x.source.configuration_ref == y.source.configuration_ref &&
                x.source.arrival_context_ref == y.source.arrival_context_ref &&
                x.command.operation.operation_scope == y.command.operation.operation_scope &&
                x.command.operation.operation_ref == y.command.operation.operation_ref &&
                x.kind == y.kind && x.plan_evidence.kind == y.plan_evidence.kind &&
                x.plan_evidence.extent == y.plan_evidence.extent &&
                x.plan_evidence.plan.has_value() == y.plan_evidence.plan.has_value() &&
                !x.plan_evidence.stopped_for && !y.plan_evidence.stopped_for &&
                !x.plan_evidence.failed_capture && !y.plan_evidence.failed_capture &&
                x.plan_evidence.occurrences.size() == y.plan_evidence.occurrences.size() &&
                x.failure.has_value() == y.failure.has_value() && !x.timer && !y.timer &&
                x.missing_policy_reason == y.missing_policy_reason &&
                x.supersedes_hold_ref == y.supersedes_hold_ref,
            "Complete literal captured hold cause/source/identity/alternative groups");
    if (y.plan_evidence.plan)
        require(x.plan_evidence.plan->plan_ref == y.plan_evidence.plan->plan_ref &&
                    x.plan_evidence.plan->revision == y.plan_evidence.plan->revision,
                "Exact selected captured plan identity");
    original_claim_fixture::same_envelope(x.command.intent, y.command.intent);
    check_observation(x.first_observation, y.first_observation);
    for (size_t n = 0; n < y.plan_evidence.occurrences.size(); ++n) {
        const auto &u = x.plan_evidence.occurrences[n], &v = y.plan_evidence.occurrences[n];
        require(u.ordinal == v.ordinal && u.action_ref == v.action_ref &&
                    u.action_version_ref == v.action_version_ref && u.rule_ref == v.rule_ref &&
                    u.target_ref == v.target_ref && u.unit == v.unit &&
                    u.attempted_units == v.attempted_units &&
                    u.finalized_units == v.finalized_units && u.explanation == v.explanation,
                "Every captured occurrence field and optional-empty distinction");
    }
    if (y.failure) {
        require(x.failure->stage == y.failure->stage &&
                    x.failure->failing_ordinal == y.failure->failing_ordinal &&
                    x.failure->failing_target_ref == y.failure->failing_target_ref &&
                    x.failure->attempted_units == y.failure->attempted_units,
                "Complete exact known numeric failure leaf");
        original_claim_fixture::same_envelope(x.failure->annotation, y.failure->annotation);
    }
    require(a.first_detection.mutation_ref == b.request.mutation_ref &&
                a.first_detection.expected_version_ref == b.request.expected_version_ref &&
                a.first_detection.proposed_version_ref == b.request.proposed_version_ref &&
                a.first_version.locator.hold_ref == y.hold_ref &&
                a.first_version.locator.version_ref == b.request.proposed_version_ref &&
                a.first_version.position == 1 && !a.first_version.parent_version_ref &&
                a.first_version.state == ApplicationHoldState::active &&
                a.first_version.producer_mutation_ref == b.request.mutation_ref &&
                !a.first_version.replacement &&
                a.current_version.locator.hold_ref == a.first_version.locator.hold_ref &&
                a.current_version.locator.version_ref == a.first_version.locator.version_ref &&
                a.current_version.position == 1 && !a.current_version.parent_version_ref &&
                a.current_version.state == ApplicationHoldState::active &&
                a.current_version.producer_mutation_ref == b.request.mutation_ref &&
                !a.current_version.replacement && !a.current_event,
            "Exact newly created first/current version and its producer");
    check_observation(a.first_detection.observation, b.request.observation);
    original_claim_fixture::same_envelope(a.first_detection.audit, b.request.audit);
}

// Optional hooks inspect/read finite inputs only; NONE writes SQL, owns a session
// or mutates authoritative state. All default bindings come from frozen DATA.
class Ports final : public ExecutionAccess,
                    public CurrentApplicationPolicy,
                    public SegmentedApplicationClock,
                    public RequiredApplicationActions,
                    public FreshApplicationReferences {
  public:
    ScriptedPorts script;
    std::function<AccessDecision(const ApplicationReferenceReadView &)> access_hook;
    std::function<PolicyResolution(const ApplicationReferenceReadView &,
                                   const ApplicationBundleInputs &)>
        policy_hook;
    std::function<ClockResolution(const ApplicationReferenceReadView &)> clock_hook;
    std::function<ActionResolution(const ApplicationCalculatedProposal &)> actions_hook;
    std::function<ReferenceResolution(ApplicationReferencePurpose,
                                      const ApplicationReferenceBinding &)>
        reference_hook;
    Ports(const Data &data, const QString &name) : script(data, name) {}
    AccessDecision check(const ApplicationReferenceReadView &v,
                         const OriginalApplicationCommand &c) override {
        auto result = script.check(v, c);
        return access_hook ? access_hook(v) : result;
    }
    bool disclose_application(const ApplicationReferenceReadView &v,
                              const OriginalApplicationCommand &c,
                              const RecordedApplicationBundle &r) override {
        return script.disclose_application(v, c, r);
    }
    bool disclose_holds(const ApplicationReferenceReadView &v, const OriginalApplicationCommand &c,
                        const ApplicationHoldActiveSet &s) override {
        return script.disclose_holds(v, c, s);
    }
    PolicyResolution resolve(const ApplicationReferenceReadView &v,
                             const OriginalApplicationCommand &c, const ApplicationBundleInputs &i,
                             const AccessDecision &a) override {
        auto result = script.resolve(v, c, i, a);
        return policy_hook ? policy_hook(v, i) : result;
    }
    ClockResolution observe(const ApplicationReferenceReadView &v,
                            const OriginalApplicationCommand &c, const ApplicationBundleInputs &i,
                            const BoundApplicationPolicy &p) override {
        auto result = script.observe(v, c, i, p);
        return clock_hook ? clock_hook(v) : result;
    }
    ActionResolution prepare(const ApplicationReferenceReadView &v,
                             const OriginalApplicationCommand &c, const ApplicationBundleInputs &i,
                             const BoundApplicationPolicy &p,
                             const ApplicationCalculatedProposal &a) override {
        auto result = script.prepare(v, c, i, p, a);
        return actions_hook ? actions_hook(a) : result;
    }
    ReferenceResolution issue(ApplicationReferencePurpose p,
                              const ApplicationReferenceBinding &b) override {
        auto result = script.issue(p, b);
        return reference_hook ? reference_hook(p, b) : result;
    }
    ApplicationServicePorts ports() { return {*this, *this, *this, *this, *this}; }
};

struct File {
    QTemporaryDir directory;
    QString path{directory.filePath("service.sqlite")};
    Data data;
    SqliteWriter writer;
    std::unique_ptr<Observer> observer;
    explicit File(const QString &name = {}) {
        require(directory.isValid(), "Disposable independent service database");
        open_sources(writer, path, data);
        if (!name.isEmpty())
            retain_case(writer, data, name);
        observer = std::make_unique<Observer>(path);
    }
    template <class Body> void setup(Body body) {
        (void)run(writer, [&](auto context) -> QVariant {
            const auto s = context.begin();
            body(s);
            s.commit();
            return 1;
        });
    }
    void arrange(const QString &name) {
        setup([&](const auto &s) { data.populate_arrangement(s, name); });
    }
    void modify(const QString &sql, const QList<QVariant> &values = {}) {
        setup([&](const auto &s) { (void)s.execute({sql, values}); });
    }
    auto snapshot() { return data.snapshot(*observer); }
    RecordedApplicationBundle lookup(const ApplicationOperationKey &op) {
        std::optional<RecordedApplicationBundle> value;
        setup([&](const auto &s) { value = SqliteApplicationBundleStore(s).lookup(op); });
        require(value.has_value(), "Old typed store sees whole committed application");
        return *value;
    }
    ApplicationHoldView hold(const std::string &ref) {
        std::optional<ApplicationHoldView> value;
        setup([&](const auto &s) { value = SqliteApplicationHoldStore(s).find(ref); });
        require(value.has_value(), "Old hold reader sees complete durable blocker");
        return *value;
    }
};
struct Attempt {
    SqliteWorkResult raw;
    ApplicationServiceCompletion service;
    std::exception_ptr body_error;
    bool done{false};
    int callbacks{0};
};
using Tail = std::function<QVariant(QVariant)>;
std::shared_ptr<Attempt> prepared_run(OriginalApplicationService &service, SqliteWriter &writer,
                                      const OriginalApplicationCommand &command, Tail tail = {},
                                      bool throw_completion = false) {
    const auto result = std::make_shared<Attempt>();
    auto p = service.make_work(command, [result, throw_completion](auto c) {
        ++result->callbacks;
        result->service = std::move(c);
        if (throw_completion)
            throw std::runtime_error("synthetic response failure AFTER actual normal completion");
    });
    require(bool(p.body) && bool(p.completion), "Matching publicly wrappable real work pair");
    require(writer.enqueue_work(
                [result, body = std::move(p.body), tail](auto context) -> QVariant {
                    try {
                        auto value = body(context);
                        return tail ? tail(std::move(value)) : value;
                    } catch (...) {
                        result->body_error = std::current_exception();
                        throw;
                    }
                },
                [result, completion = std::move(p.completion)](auto raw) {
                    result->raw = raw;
                    result->done = true;
                    completion(std::move(raw));
                }) == StorageError::none,
            "Real writer admits matching prepared body/completion");
    require(QTest::qWaitFor([result] { return result->done; }, 10000),
            "Real writer completed; timeout is never an intended marker red");
    rethrow_marker(result->body_error);
    require(result->callbacks == 1, "Exactly one actual service completion");
    require(result->raw.transaction == result->service.transaction &&
                result->raw.error == result->service.work_error,
            "Actual writer outcome/error survive service bridge");
    return result;
}
QJsonObject codec_row(const Data &data, const QString &name) {
    return name.startsWith("anchor:")
               ? data.frozen["codec_anchors"].toObject()[name.mid(7)].toObject()
               : data.case_data(name);
}
void codec_rows() {
    QTest::addColumn<QString>("name");
    const Data data;
    for (const auto &n : data.frozen["codec_anchors"].toObject().keys())
        QTest::newRow(("anchor:" + n).toUtf8().constData()) << ("anchor:" + n);
    for (const auto &n : data.frozen["cases"].toObject().keys())
        QTest::newRow(n.toUtf8().constData()) << n;
}
void case_rows(const QStringList &names) {
    QTest::addColumn<QString>("name");
    for (const auto &n : names)
        QTest::newRow(n.toUtf8().constData()) << n;
}
QJsonObject historical(const Data &data, const QString &tag) {
    return data.frozen["arrangements"]
        .toObject()["history"]
        .toObject()["records"]
        .toObject()[tag]
        .toObject();
}
void unchanged(File &f, const std::vector<SqlRows> &before) {
    require(f.snapshot() == before, "Complete application/contribution/hold snapshot unchanged");
}
} // namespace

class ApplicationServiceTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    // Complete SAC1 bytes include every command field; no UUT encoding is an oracle.
    void commandEncodingMatchesFrozenFrames_data() { codec_rows(); }
    void commandEncodingMatchesFrozenFrames() {
        QFETCH(QString, name);
        scenario([&] {
            const Data data;
            const auto c = codec_row(data, name);
            const auto expected =
                envelope(c.contains("command_envelope") ? c["command_envelope"] : c["envelope"]);
            const auto actual = encode_original_application_command(command(c["command"]));
            original_claim_fixture::same_envelope(actual, expected);
        });
    }
    void commandDecodingMatchesFrozenMembers_data() { codec_rows(); }
    void commandDecodingMatchesFrozenMembers() {
        QFETCH(QString, name);
        scenario([&] {
            const Data data;
            const auto c = codec_row(data, name);
            const auto bytes =
                envelope(c.contains("command_envelope") ? c["command_envelope"] : c["envelope"]);
            const auto actual = decode_original_application_command(bytes);
            require(same_command(actual, command(c["command"])),
                    "Every lossless decoded field/optional/raw byte");
        });
    }
    // CS17: wire framing is distinct from actual input/output ownership.
    void codecBoundsUseActualOwnedDTOs_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 8; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void codecBoundsUseActualOwnedDTOs() {
        QFETCH(int, row);
        scenario([&] {
            const Data data;
            const auto a = data.frozen["codec_anchors"].toObject()["C0"].toObject();
            const auto c = command(a["command"]);
            const auto e = envelope(a["envelope"]);
            ApplicationServiceLimits l;
            if (row <= 2) {
                l.input_owned_bytes = 25;
                l.result_owned_bytes = row == 0 ? 98 : 97;
                if (row == 0)
                    original_claim_fixture::same_envelope(encode_original_application_command(c, l),
                                                          e);
                else if (row == 1)
                    input_failure([&] { (void)encode_original_application_command(c, l); },
                                  IE::result_limit);
                else
                    input_failure([&] { (void)decode_original_application_command(e, l); },
                                  IE::owned_byte_limit);
            } else if (row == 3 || row == 4) {
                l.command_bytes = 58;
                if (row == 4)
                    l.input_owned_bytes = 24;
                if (row == 3)
                    require(same_command(decode_original_application_command(e, l), c),
                            "Exact framed58 fits");
                else
                    input_failure([&] { (void)encode_original_application_command(c, l); },
                                  IE::owned_byte_limit);
            } else {
                const auto b = data.frozen["boundary_anchor"].toObject();
                const auto bc = command(b["command"]);
                const auto be = envelope(b["envelope"]);
                l.choices_per_command = 2;
                l.result_items = row == 5 ? 2 : 1;
                if (row == 5)
                    require(same_command(decode_original_application_command(be, l), bc),
                            "Two decoded items fit2");
                else if (row == 6)
                    input_failure([&] { (void)decode_original_application_command(be, l); },
                                  IE::result_limit);
                else
                    original_claim_fixture::same_envelope(
                        encode_original_application_command(bc, l), be);
            }
        });
    }
    void codecMalformedAndPurePriority_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 12; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void codecMalformedAndPurePriority() {
        QFETCH(int, row);
        scenario([&] {
            const Data data;
            const auto a = data.frozen["codec_anchors"].toObject()["C0"].toObject();
            auto c = command(a["command"]);
            auto e = envelope(a["envelope"]);
            ApplicationServiceLimits l;
            if (row == 0) {
                l.holds.references.result_items = 0;
                c.receipt_id = {0};
                input_failure([&] { (void)encode_original_application_command(c, l); },
                              IE::invalid_options);
            } else if (row == 1) {
                c.receipt_id = {0};
                input_failure([&] { (void)encode_original_application_command(c); },
                              IE::invalid_source);
            } else if (row == 2) {
                c.operation.operation_ref.clear();
                input_failure([&] { (void)encode_original_application_command(c); },
                              IE::invalid_key);
            } else if (row == 3) {
                c.preview_mode = ApplicationPreviewMode::require_exact;
                input_failure([&] { (void)encode_original_application_command(c); },
                              IE::invalid_conditional_group);
            } else if (row == 4) {
                c.choices.push_back({"x", QByteArray()});
                input_failure([&] { (void)encode_original_application_command(c); },
                              IE::invalid_body);
            } else if (row == 5) {
                c.choices = {{"x", QByteArray()}, {"y", QByteArray("", 0)}};
                l.choices_per_command = 1;
                input_failure([&] { (void)encode_original_application_command(c, l); },
                              IE::collection_limit);
            } else if (row == 6) {
                e.format_ref = "future.command.v9";
                bool caught = false;
                try {
                    (void)decode_original_application_command(e);
                } catch (const ApplicationServiceUnsupportedFormat &x) {
                    caught = x.format_ref() == e.format_ref;
                }
                require(caught, "Healthy unknown format is explicitly unsupported");
            } else {
                if (row == 7)
                    e.document[0] = 'X';
                if (row == 8)
                    e.document.append('\0');
                if (row == 9)
                    e.document.chop(1);
                if (row == 10)
                    e.document[50] = char(2); // C0 optional preview tag, fixed independent offset.
                if (row == 11) {
                    e.document[14] = char(9); // C0 subject tag, not guessed by a UUT parser.
                    input_failure([&] { (void)decode_original_application_command(e); },
                                  IE::invalid_source);
                    return;
                }
                input_failure([&] { (void)decode_original_application_command(e); },
                              IE::invalid_body);
            }
        });
    }
    void codecOwnsBinaryBuffers() {
        scenario([&] {
            const Data data;
            const auto a = data.frozen["codec_anchors"].toObject()["choice"].toObject();
            auto c = command(a["command"]);
            QByteArray backing = QByteArray::fromHex("efbbbf00ff41");
            c.choices[0].document = QByteArray::fromRawData(backing.constData(), backing.size());
            const auto encoded = encode_original_application_command(c);
            backing.fill('Z');
            original_claim_fixture::same_envelope(encoded, envelope(a["envelope"]));
            auto wire_backing = encoded.document;
            auto input = encoded;
            input.document = QByteArray::fromRawData(wire_backing.constData(), wire_backing.size());
            const auto decoded = decode_original_application_command(input);
            wire_backing.fill('Q');
            require(same_command(decoded, command(a["command"])),
                    "Decoded buffers own BOM/NUL/FF beyond caller storage");
        });
    }
    void admissionPreflightBeforeQueue_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 7; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void admissionPreflightBeforeQueue() {
        QFETCH(int, row);
        scenario([&] {
            File f("add10");
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            auto ports = p.ports();
            ApplicationServiceLimits l;
            auto c = f.data.command("add10");
            IE expected = IE::invalid_options;
            if (row == 0)
                l.holds.references.component_bytes = 0;
            if (row == 1) {
                c.operation.operation_ref.clear();
                expected = IE::invalid_key;
            }
            if (row == 2) {
                c.receipt_id = {0};
                expected = IE::invalid_source;
            }
            if (row == 3) {
                c.contribution_id.reset();
                expected = IE::invalid_source;
            }
            if (row == 4) {
                c.choices[0].document = {};
                expected = IE::invalid_body;
            }
            if (row == 5) {
                l.input_owned_bytes = 95;
                expected = IE::owned_byte_limit;
            }
            if (row == 6) {
                l.choices_per_command = 0;
            }
            OriginalApplicationService service(f.writer, ports, l);
            bool callback = false;
            input_failure([&] { (void)service.enqueue(c, [&](auto) { callback = true; }); },
                          expected);
            require(!callback && p.script.calls.empty(), "Pure admission never queues/calls ports");
            unchanged(f, before);
        });
    }
    void foreignAdmissionAndMissingCallbacks_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 4; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void foreignAdmissionAndMissingCallbacks() {
        QFETCH(int, row);
        scenario([&] {
            File f("add10");
            Ports p(f.data, "add10");
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto c = f.data.command("add10");
            const auto before = f.snapshot();
            std::exception_ptr error;
            StorageError admission = StorageError::none;
            bool caught = false, callback = false;
            auto call = [&] {
                try {
                    if (row % 2 == 0)
                        admission =
                            service
                                .enqueue(c, row < 2 ? OriginalApplicationService::Completion{
                                                          [&](auto) { callback = true; }}
                                                    : OriginalApplicationService::Completion{})
                                .error;
                    else
                        (void)service.make_work(c, row < 2
                                                       ? OriginalApplicationService::Completion{
                                                             [&](auto) { callback = true; }}
                                                       : OriginalApplicationService::Completion{});
                } catch (...) {
                    error = std::current_exception();
                }
            };
            if (row < 2) {
                std::thread worker(call);
                worker.join();
            } else
                call();
            rethrow_marker(error); // Named missing stub MUST cross the thread; never terminate it.
            const auto expected = row < 2 ? StorageError::wrong_thread : StorageError::invalid_work;
            if (row % 2 == 0)
                require(!error && admission == expected, "Exact synchronous admission rejection");
            else {
                require(bool(error), "Preparation rejection throws its typed category");
                try {
                    std::rethrow_exception(error);
                } catch (const ApplicationServicePreparationFailure &e) {
                    caught = e.error() == expected;
                }
                require(caught, "Exact make_work rejection");
            }
            require(!callback && p.script.calls.empty(), "No rejected callback or port work");
            unchanged(f, before);
        });
    }
    // AS01–08/17–20, CS01–03/09/13/18. Every successful child and envelope is literal.
    void arithmeticCommitsCompleteFrozenRecord_data() {
        const Data d;
        QStringList names;
        const auto cases = d.frozen["cases"].toObject();
        for (auto it = cases.begin(); it != cases.end(); ++it)
            if (it.value().toObject().contains("expected_record"))
                names.append(it.key());
        case_rows(names);
    }
    void arithmeticCommitsCompleteFrozenRecord() {
        QFETCH(QString, name);
        scenario([&] {
            File f(name);
            const auto c = f.data.case_data(name);
            const auto expected = record(c["expected_record"]);
            const auto prior_count =
                f.observer->rows("SELECT count(*) FROM subathon_app_operations")[0][0].toLongLong();
            Ports p(f.data, name);
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command(name));
            disposition(r->service, D::applied);
            require(std::holds_alternative<RecordedApplicationBundle>(r->service.response->payload),
                    "Full owned success record");
            check_record(std::get<RecordedApplicationBundle>(r->service.response->payload),
                         expected);
            check_record(f.lookup(expected.intent.operation), expected);
            require(p.script.received_proposal.has_value(),
                    "Actions see whole token-free proposal");
            check_proposal(*p.script.received_proposal, proposal(c));
            require(
                p.script.calls == std::vector<std::string>{"access", "policy", "clock", "actions",
                                                           "reference:0", "reference:1"},
                "Access/current policy/clock/whole actions precede exactly two fresh state refs");
            f.setup([&](const auto &s) {
                const auto actual = SqliteApplicationBundleStore(s).read_state();
                require(actual.has_value(), "Current committed after state");
                check_state(*actual, expected.after);
            });
            require(f.observer->rows("SELECT count(*) FROM subathon_app_operations")[0][0]
                            .toLongLong() == prior_count + 1,
                    "Exactly one new operation");
            if (expected.intent.contribution_id) {
                const auto rows = f.observer->rows(
                    "SELECT application FROM subathon_contributions WHERE contribution_id=X'" +
                    QString::fromLatin1(expected.intent.contribution_id->value == 0
                                            ? "0000000000000000"
                                            : "ffffffffffffffff") +
                    "'");
                require(rows.size() == 1 && rows[0][0].toInt() == 1,
                        "Only actual selected contribution becomes applied");
            }
        });
    }
    void exactPreviewAndCurrentPlanRefuse_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 3; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void exactPreviewAndCurrentPlanRefuse() {
        QFETCH(int, row);
        scenario([&] {
            File f("add10");
            auto c = f.data.command("add10");
            D expected = D::state_conflict;
            if (row == 0) {
                c.preview_mode = ApplicationPreviewMode::require_exact;
                c.preview_state_token = "old";
            }
            if (row == 1) {
                f.setup([&](const auto &s) {
                    SqliteApplicationPlanStore store(s);
                    auto previous = store.find_revision("PA", 1);
                    require(previous.has_value(), "Prior complete real plan revision");
                    auto next = previous->revision;
                    next.parent_revision = 1;
                    next.revision = 2;
                    next.override_decision =
                        core::PlanOverrideDecision{core::OverrideAuthority::authorized, "actor",
                                                   "synthetic override", "decision"};
                    require(store.append("PA", 1, next) == SqlitePlanAppendDisposition::appended,
                            "Prior separately committed plan-head2");
                });
                expected = D::plan_conflict;
            }
            if (row == 2) {
                c.plan.plan_ref = "missing";
                expected = D::missing_plan;
            }
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            p.script.expected_command = c;
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, c);
            no_payload(r->service, expected);
            require(p.script.calls == std::vector<std::string>{"access"},
                    "Freshness refusal before policy/clock/actions/ref");
            unchanged(f, before);
        });
    }
    void currentEligibilityKeepsIndependentConditions() {
        scenario([&] {
            File f("money_unresolved");
            const auto before = f.snapshot();
            Ports p(f.data, "money_unresolved");
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command("money_unresolved"));
            disposition(r->service, D::ineligible);
            const auto &actual =
                std::get<std::vector<core::ContributionBlocker>>(r->service.response->payload);
            std::vector<core::ContributionBlocker> expected;
            for (const auto &x :
                 f.data.case_data("money_unresolved")["expected_blockers"].toArray())
                expected.push_back(static_cast<core::ContributionBlocker>(x.toInt()));
            require(actual == expected,
                    "Independent missing evidence/valuation/review/settlement blockers");
            require(p.script.calls == std::vector<std::string>{"access", "policy", "clock"},
                    "Known clock observation precedes final eligibility refusal");
            unchanged(f, before);
        });
    }
    // Public pre-read reassessment only: NOT an invented read→stage concurrent race.
    void currentDimensionsAreCopiedAtExecution_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 15; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void currentDimensionsAreCopiedAtExecution() {
        QFETCH(int, row);
        scenario([&] {
            File f("money_unresolved");
            f.modify("UPDATE subathon_contributions SET "
                     "decision_choice=0,decision_ref=X'64',decision_actor_ref=X'6163746f72' WHERE "
                     "contribution_id=X'000000000000000a'");
            core::ContributionState expected;
            expected.payment_evidence = core::PaymentEvidence::unknown;
            auto &s = expected.conditions;
            s.valuation = {true, core::ReceiptMoney{"1.234500", "USD"}, std::nullopt, true};
            s.review = core::ContributionReview::pending;
            s.evidence_admission = {
                core::EvidencePolicy::require_evidence_review, "p",
                core::EvidenceReviewDecision{core::EvidenceReviewChoice::await_confirmation, "d",
                                             "actor"}};
            s.settlement = core::SettlementGate::unresolved;
            QString column;
            QVariant value;
            switch (row) {
            case 0:
                column = "payment_evidence";
                value = 2;
                expected.payment_evidence = core::PaymentEvidence::confirmed;
                break;
            case 1:
                column = "monetary";
                value = 0;
                s.valuation.monetary = false;
                break;
            case 2:
                column = "original_decimal";
                value = blob("9.0000");
                s.valuation.original_money->original_decimal = "9.0000";
                break;
            case 3:
                column = "currency";
                value = blob("EUR");
                s.valuation.original_money->currency = "EUR";
                break;
            case 4:
                column = "resolution_ref";
                value = blob("resolved");
                s.valuation.resolution_ref = "resolved";
                break;
            case 5:
                column = "missing_rate";
                value = 0;
                s.valuation.missing_rate = false;
                break;
            case 6:
                column = "review";
                value = 2;
                s.review = core::ContributionReview::accepted;
                break;
            case 7:
                column = "evidence_policy";
                value = 1;
                s.evidence_admission.policy = core::EvidencePolicy::accept_provider_report;
                break;
            case 8:
                column = "policy_ref";
                value = blob("other");
                s.evidence_admission.policy_ref = "other";
                break;
            case 9:
                column = "decision_choice";
                value = 1;
                s.evidence_admission.review_decision->choice =
                    core::EvidenceReviewChoice::accept_labelled_report;
                break;
            case 10:
                column = "decision_ref";
                value = blob("new");
                s.evidence_admission.review_decision->decision_ref = "new";
                break;
            case 11:
                column = "decision_actor_ref";
                value = blob("other");
                s.evidence_admission.review_decision->actor_ref = "other";
                break;
            case 12:
                column = "application";
                value = 1;
                s.application = core::ContributionApplication::applied;
                break;
            case 13:
                column = "reversal";
                value = 1;
                s.reversal = core::ContributionReversal::partial;
                break;
            case 14:
                column = "settlement";
                value = 1;
                s.settlement = core::SettlementGate::clear;
                break;
            }
            f.modify("UPDATE subathon_contributions SET " + column +
                         "=? WHERE contribution_id=X'000000000000000a'",
                     {value});
            const auto before = f.snapshot();
            Ports p(f.data, "money_unresolved");
            bool seen = false;
            p.policy_hook = [&](const auto &, const auto &inputs) -> PolicyResolution {
                require(inputs.current_contribution &&
                            inputs.current_contribution->contribution.state == expected,
                        "EVERY current dimension is supplied, not captured/partially copied");
                require(inputs.current_contribution->support_anchor.observation.support_conditions
                            .has_value(),
                        "Immutable original support remains separately present");
                seen = true;
                return p.script.bound_policy;
            };
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command("money_unresolved"));
            disposition(r->service, D::ineligible);
            require(seen, "Current-state read reached trusted bound policy");
            unchanged(f, before);
        });
    }
    void accessBeforeProtectedLookup_data() {
        QTest::addColumn<bool>("unknown");
        QTest::newRow("denied") << false;
        QTest::newRow("unknown") << true;
    }
    void accessBeforeProtectedLookup() {
        QFETCH(bool, unknown);
        scenario([&] {
            File f;
            f.arrange("history");
            // A selected known malformed historical result would fail if looked up before access.
            f.setup([&](const auto &s) {
                f.data.apply_fault(s, "known_historical_result_trailing_byte");
            });
            const auto c = command(historical(f.data, "A")["command"]);
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            p.script.expected_command = c;
            p.script.access = unknown ? AccessDecision{ApplicationAccessUnknown{}}
                                      : AccessDecision{ApplicationAccessDenied{}};
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, c);
            no_payload(r->service, D::denied);
            require(p.script.calls == std::vector<std::string>{"access"},
                    "No protected lookup/disclosure/current planning on denied authority");
            unchanged(f, before);
        });
    }
    void historicalCompleteCommandAndDisclosure_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 14; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void historicalCompleteCommandAndDisclosure() {
        QFETCH(int, row);
        scenario([&] {
            File f;
            f.arrange("history");
            const auto literal = historical(f.data, "A");
            auto c = command(literal["command"]);
            if (row == 2)
                c.receipt_id = {9};
            if (row == 3)
                c.plan.plan_ref = "different";
            if (row == 4)
                c.plan.revision = 2;
            if (row == 5)
                c.preview_state_token = "different";
            if (row == 6)
                c.preview_mode = ApplicationPreviewMode::require_exact;
            if (row == 7)
                c.principal.principal_ref = "different-user";
            if (row == 8)
                c.actor_ref = "";
            if (row == 9)
                c.decision_ref = "";
            if (row == 10)
                c.reason = "";
            if (row == 11)
                c.choices[0].document.append('\0');
            if (row == 12)
                c.choices[0].format_ref = "different";
            if (row == 13)
                c.principal.kind = ApplicationPrincipalKind::automation;
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            p.script.expected_command = c;
            p.script.application_visible = row != 1;
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, c);
            if (row == 0) {
                disposition(r->service, D::existing_operation);
                check_record(std::get<RecordedApplicationBundle>(r->service.response->payload),
                             record(literal));
            } else
                no_payload(r->service, row == 1 ? D::disclose_denied : D::operation_conflict);
            require(p.script.calls == std::vector<std::string>{"access", "disclose_application"},
                    "Complete retained intent/disclosure BEFORE current revaluation");
            unchanged(f, before);
        });
    }
    void globalClaimsBeforeCurrentWork_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 4; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void globalClaimsBeforeCurrentWork() {
        QFETCH(int, row);
        scenario([&] {
            File f;
            f.arrange("history");
            auto c = f.data.command("add10");
            QString tag = "A";
            D expected = D::subject_claimed;
            if (row == 0)
                c.receipt_id = {9}; // Actual fixture confirmation of CID0.
            if (row == 1) {
                c.receipt_id = {std::numeric_limits<std::uint64_t>::max()};
                c.contribution_id = core::ContributionId{0};
            }
            if (row == 2) {
                c.receipt_id = {std::numeric_limits<std::uint64_t>::max()};
                c.contribution_id = core::ContributionId{42};
                tag = "B";
                expected = D::source_claimed;
            }
            if (row == 3) {
                c.subject_kind = ApplicationSubjectKind::receipt;
                c.contribution_id.reset();
                c.receipt_id = {8};
                c.plan = {"PC", 1};
                tag = "C";
            }
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            p.script.expected_command = c;
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, c);
            disposition(r->service, expected);
            check_record(std::get<RecordedApplicationBundle>(r->service.response->payload),
                         record(historical(f.data, tag)));
            require(p.script.calls == std::vector<std::string>{"access", "disclose_application"},
                    "Global subject priority then original source, no candidate revaluation");
            unchanged(f, before);
        });
    }
    void allActiveHoldsGateNewKeys_data() {
        QTest::addColumn<bool>("visible");
        QTest::newRow("disclosed") << true;
        QTest::newRow("protected") << false;
    }
    void allActiveHoldsGateNewKeys() {
        QFETCH(bool, visible);
        scenario([&] {
            File f;
            f.arrange("active_holds");
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            p.script.holds_visible = visible;
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command("add10"));
            disposition(r->service, D::active_hold);
            const auto &a = std::get<ApplicationActiveHoldPayload>(r->service.response->payload);
            require(a.holds.has_value() == visible,
                    "Current disclosure controls whole hold details");
            if (visible) {
                require(a.holds->subject.id == ApplicationHoldSubject{core::ContributionId{0}}.id &&
                            a.holds->holds.size() == 2 &&
                            a.holds->holds[0].cause.hold_ref == "h1" &&
                            a.holds->holds[1].cause.hold_ref == "h2",
                        "ALL independent active holds globally block a new key");
                const auto h = f.data.holds();
                for (size_t n = 0; n < 2; ++n) {
                    const auto e = h.view(n == 0 ? "h1" : "h2");
                    check_creation(a.holds->holds[n], h.detection(n == 0 ? "d1" : "d2"));
                    original_claim_fixture::same_envelope(a.holds->holds[n].cause.command.intent,
                                                          e.cause.command.intent);
                    original_claim_fixture::same_envelope(
                        a.holds->holds[n].cause.first_observation.annotation,
                        e.cause.first_observation.annotation);
                    require(a.holds->holds[n].current_version.locator.version_ref ==
                                    e.current_version.locator.version_ref &&
                                a.holds->holds[n].current_version.state ==
                                    ApplicationHoldState::active,
                            "Actual current active versions, not historical retry outputs");
                }
            }
            require(p.script.calls == std::vector<std::string>{"access", "disclose_holds"},
                    "No new policy/clock/actions/token can bypass another-key hold");
            unchanged(f, before);
        });
    }
    void numericOverflowCommitsOnlyDurableHold_data() {
        case_rows({"score_overflow", "count_overflow", "min_overflow", "clock_overflow"});
    }
    void numericOverflowCommitsOnlyDurableHold() {
        QFETCH(QString, name);
        scenario([&] {
            File f(name);
            const auto c = f.data.case_data(name);
            const auto a = c["arithmetic"].toObject();
            const auto prior_head = f.data.head(name);
            Ports p(f.data, name);
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command(name));
            disposition(r->service, D::held_numeric);
            const auto &out = std::get<ApplicationHeldPayload>(r->service.response->payload);
            const auto expected_ref = name == "clock_overflow" ? "hClock" : "hNew";
            require(out.hold_ref == expected_ref &&
                        out.outcome.kind == ApplicationHoldMutationKind::detection &&
                        out.outcome.produced_versions.size() == 1,
                    "Fresh recorded hold, not a historical active-version certificate");
            const auto h = f.hold(expected_ref);
            require(h.cause.kind == ApplicationHoldCauseKind::numeric_overflow && h.cause.failure &&
                        h.cause.first_observation.persisted_state_token == prior_head.token &&
                        h.current_version.state == ApplicationHoldState::active &&
                        h.current_version.position == 1 && !h.current_version.parent_version_ref &&
                        h.current_version.locator.version_ref ==
                            out.outcome.produced_versions[0].locator.version_ref &&
                        h.cause.failure->failing_ordinal == integer(a["ordinal"]) &&
                        h.cause.failure->attempted_units == core::Exact(text(a["attempt"])),
                    "Whole-input known overflow evidence and exact newly current blocker");
            original_claim_fixture::same_envelope(h.cause.command.intent,
                                                  envelope(c["command_envelope"]));
            const auto source_requests = requests(c["requests"]);
            require(h.cause.plan_evidence.occurrences.size() == source_requests.size(),
                    "No fitting-prefix truncation");
            for (size_t n = 0; n < source_requests.size(); ++n) {
                const auto &o = h.cause.plan_evidence.occurrences[n];
                const auto &q = source_requests[n];
                require(
                    o.ordinal == std::int64_t(n + 1) && o.unit == q.unit &&
                        o.attempted_units == q.exact_requested_units &&
                        o.finalized_units == q.requested_units && o.action_ref == q.action_ref &&
                        o.action_version_ref == q.action_version_ref && o.rule_ref == q.rule_ref &&
                        o.target_ref == q.target_ref && o.explanation == q.calculation_explanation,
                    "Every original complete request/provenance/empty-presence retained");
            }
            if (c.contains("expected_detection")) {
                const auto expected =
                    f.data.holds().detection(text(c["expected_detection"]).c_str());
                check_creation(h, expected);
                original_claim_fixture::same_envelope(h.cause.first_observation.annotation,
                                                      expected.cause.first_observation.annotation);
                original_claim_fixture::same_envelope(h.first_detection.observation.annotation,
                                                      expected.request.observation.annotation);
                original_claim_fixture::same_envelope(h.first_detection.audit,
                                                      expected.request.audit);
                original_claim_fixture::same_envelope(h.cause.failure->annotation,
                                                      expected.cause.failure->annotation);
                require(h.cause.source.subject.id == expected.cause.source.subject.id &&
                            h.cause.source.receipt_id == expected.cause.source.receipt_id &&
                            h.cause.source.event_ref == expected.cause.source.event_ref &&
                            h.cause.source.parent_event_ref ==
                                expected.cause.source.parent_event_ref &&
                            h.cause.source.root_event_ref == expected.cause.source.root_event_ref &&
                            h.cause.source.configuration_ref ==
                                expected.cause.source.configuration_ref &&
                            h.cause.source.arrival_context_ref ==
                                expected.cause.source.arrival_context_ref &&
                            h.first_detection.mutation_ref == expected.request.mutation_ref &&
                            h.first_detection.proposed_version_ref ==
                                expected.request.proposed_version_ref,
                        "Frozen complete hold source/first detection identity");
            }
            require(
                f.observer->rows("SELECT count(*) FROM subathon_app_operations")[0][0].toInt() ==
                        0 &&
                    f.observer->rows("SELECT count(*) FROM subathon_app_effects")[0][0].toInt() ==
                        0 &&
                    f.observer->rows("SELECT count(*) FROM subathon_app_jobs")[0][0].toInt() == 0 &&
                    f.observer->rows("SELECT count(*) FROM subathon_app_claims")[0][0].toInt() == 0,
                "No award, effect, job, claim or success record survives overflow");
            f.setup([&](const auto &s) {
                check_state(*SqliteApplicationBundleStore(s).read_state(), prior_head);
            });
            require(p.script.calls == std::vector<std::string>{"access", "policy", "clock",
                                                               "reference:2", "reference:3",
                                                               "reference:4"},
                    "Whole overflow never prepares actions or success-state tokens");
        });
    }
    void closedDetectionRetryCannotClaimHeld() {
        scenario([&] {
            File f;
            f.arrange("resolved_retry");
            const auto before = f.snapshot();
            Ports p(f.data, "score_overflow");
            p.script.reference_script(f.data.frozen["arrangements"]
                                          .toObject()["resolved_retry"]
                                          .toObject()["retry_reference_script"]
                                          .toArray(),
                                      "a");
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command("score_overflow"));
            disposition(r->service, D::stage_conflict);
            const auto &tag = std::get<ApplicationStageConflict>(r->service.response->payload);
            require(std::holds_alternative<SqliteHoldMutationDisposition>(tag) &&
                        std::get<SqliteHoldMutationDisposition>(tag) ==
                            SqliteHoldMutationDisposition::already_recorded,
                    "Historical active v1 from resolved hRetry is a typed conflict, NEVER held");
            require(f.hold("hRetry").current_version.state == ApplicationHoldState::resolved,
                    "No implicit reactivation/ref regeneration");
            unchanged(f, before);
        });
    }
    void lifecyclePauseAndDeadlineDoNotCredit_data() {
        case_rows({"ready", "finished", "paused", "hard_end_paused", "hard_end_unpaused",
                   "missing_counter", "negative_review"});
    }
    void lifecyclePauseAndDeadlineDoNotCredit() {
        QFETCH(QString, name);
        scenario([&] {
            File f(name);
            const auto before = f.snapshot();
            Ports p(f.data, name);
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command(name));
            not_ready(r->service);
            require(p.script.calls == std::vector<std::string>{"access", "policy", "clock"},
                    "Clock callback precedes final refusal, no actions/issuer");
            if (name == "missing_counter" || name == "negative_review") {
                const auto &reason =
                    std::get<ApplicationNotReadyPayload>(r->service.response->payload).reason_ref;
                require(reason == (name == "missing_counter" ? "required_counter_definition"
                                                             : "required_mixed_timer_policy"),
                        "Required owning policy/definition gate, not silent initialization/global "
                        "deferral");
            }
            unchanged(f, before);
            // Discarded paused transitions/Ready exclusion additionally require the
            // separately recorded actual source audit, not a fake mutation/trace.
        });
    }
    void portBindingsAndIntermediateBounds_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 5; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void portBindingsAndIntermediateBounds() {
        QFETCH(int, row);
        scenario([&] {
            File f("add10");
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            auto &policy = std::get<BoundApplicationPolicy>(p.script.bound_policy);
            auto &clock = std::get<ApplicationClockObservation>(p.script.observed_clock);
            ApplicationServiceLimits l;
            if (row == 0)
                policy.checked_state_token = "not-current";
            if (row == 1)
                policy.configuration_ref = "wrong";
            if (row == 2)
                clock.policy_revision_ref = "wrong";
            if (row == 3)
                clock.segments[0].elapsed_ms = -1;
            if (row == 4) {
                l.intermediate_owned_bytes = 4096;
                policy.explanation.assign(8192, 'x');
            }
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports, l);
            const auto r = prepared_run(service, f.writer, f.data.command("add10"));
            known(r->service);
            require(r->service.failure->source == ApplicationServiceFailureSource::port_contract &&
                        std::holds_alternative<LR>(r->service.failure->reason),
                    "Bad trusted output has local bounded category, never authority from envelope");
            if (row == 4)
                require(std::get<LR>(r->service.failure->reason) == LR::result_limit &&
                            p.script.calls == std::vector<std::string>{"access", "policy"},
                        "Actual oversized policy value fails before clock/allocating outcome");
            unchanged(f, before);
        });
    }
    void issuerCollisionsNeverRegenerate_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 3; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void issuerCollisionsNeverRegenerate() {
        QFETCH(int, row);
        scenario([&] {
            File f("add10");
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            if (row == 0) {
                for (auto &r : p.script.issued)
                    if (r.binding.purpose == ApplicationReferencePurpose::after_state)
                        r.reference = "a"; // Actual existing head.
            }
            if (row == 1) {
                p.reference_hook = [&](auto purpose, const auto &) -> ReferenceResolution {
                    for (auto r : p.script.issued)
                        if (r.binding.purpose == purpose) {
                            r.binding.plan.revision = 2;
                            return r;
                        }
                    return ApplicationUnavailable{"missing"};
                };
            }
            if (row == 2)
                p.reference_hook = [](auto, const auto &) -> ReferenceResolution {
                    return ApplicationUnavailable{"issuer-unavailable"};
                };
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command("add10"));
            if (row == 0)
                no_payload(r->service, D::state_token_conflict);
            if (row == 1)
                local_failure(r->service, ApplicationServiceFailureSource::port_contract,
                              LR::invalid_binding);
            if (row == 2)
                not_ready(r->service);
            for (int purpose = 0; purpose != 5; ++purpose)
                require(std::count(p.script.calls.begin(), p.script.calls.end(),
                                   "reference:" + std::to_string(purpose)) <= 1,
                        "No automatic regenerate/retry loop");
            unchanged(f, before);
        });
    }
    void referenceViewPureAndCallbackLifetime_data() {
        QTest::addColumn<int>("row");
        QTest::newRow("default-expired") << 0;
        QTest::newRow("invalid-key-before-expired") << 1;
        QTest::newRow("previous-callback-expired-in-active-unit") << 2;
        QTest::newRow("valid-callback-read") << 3;
    }
    void referenceViewPureAndCallbackLifetime() {
        QFETCH(int, row);
        scenario([&] {
            if (row < 2) {
                ApplicationReferenceReadView view;
                bool caught = false;
                try {
                    (void)view.find_retained({RetainedKind::configuration, row == 1 ? "" : "cfg"});
                } catch (const ApplicationReferenceViewFailure &e) {
                    caught =
                        row == 0 && e.error() == ApplicationReferenceViewError::expired_callback;
                } catch (const RetentionFailure &e) {
                    caught = row == 1 && e.error() == RetentionError::invalid_reference;
                }
                require(caught,
                        "Narrow view expiry differs from real stale_session; pure key wins");
                return;
            }
            File f("add10");
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            std::optional<ApplicationReferenceReadView> old_view;
            bool seen = false;
            p.access_hook = [&](const auto &v) -> AccessDecision {
                if (row == 2)
                    old_view = v;
                else {
                    const auto value = v.find_retained({RetainedKind::configuration, "cfg"});
                    require(value && value->key.reference == "cfg" && !value->document.isNull(),
                            "Actual same-session finite retained read");
                    seen = true;
                }
                return p.script.access;
            };
            if (row == 2)
                p.policy_hook = [&](const auto &, const auto &) -> PolicyResolution {
                    try {
                        (void)old_view->find_retained({RetainedKind::configuration, "cfg"});
                    } catch (const ApplicationReferenceViewFailure &e) {
                        require(e.error() == ApplicationReferenceViewError::expired_callback,
                                "Callback expired while real session remains active");
                        seen = true;
                        throw;
                    }
                    throw std::runtime_error("Required callback expiry missing");
                };
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command("add10"));
            require(seen, "The public reference view path was reached");
            if (row == 2) {
                local_failure(r->service, ApplicationServiceFailureSource::port_contract,
                              LR::invalid_output);
                require(r->service.work_error != WorkError::stale_session,
                        "No synthetic stale writer truth");
                unchanged(f, before);
            } else {
                disposition(r->service, D::applied);
                check_record(std::get<RecordedApplicationBundle>(r->service.response->payload),
                             record(f.data.case_data("add10")["expected_record"]));
            }
        });
    }
    void referenceViewForeignReadAndUnderlyingBudget_data() {
        QTest::addColumn<bool>("budget");
        QTest::newRow("live-foreign-view") << false;
        QTest::newRow("real-retained-query-result-limit") << true;
    }
    void referenceViewForeignReadAndUnderlyingBudget() {
        QFETCH(bool, budget);
        scenario([&] {
            File f("add10");
            const auto before = f.snapshot();
            bool closed = false;
            require(f.writer.shutdown([&] { closed = true; }) == StorageError::none &&
                        QTest::qWaitFor([&] { return closed; }, 10000),
                    "Close original OLD setup writer");
            SqliteWriter writer;
            auto storage_limits = limits();
            if (budget)
                storage_limits.result_value_bytes = 5; // Real cfg bytes exceed5.
            require(writer.open(f.path, storage_limits) == StorageError::none,
                    "Open bounded actual writer after setup");
            Ports p(f.data, "add10");
            bool seen = false;
            p.access_hook = [&](const auto &v) -> AccessDecision {
                if (budget) {
                    (void)v.find_retained({RetainedKind::configuration, "cfg"});
                    throw std::runtime_error("Required real retained-query result limit missing");
                }
                std::exception_ptr error;
                std::thread worker([&] {
                    try {
                        (void)v.find_retained({RetainedKind::configuration, "cfg"});
                    } catch (...) {
                        error = std::current_exception();
                    }
                });
                worker.join();
                rethrow_marker(error);
                require(bool(error), "Foreign finite view rejected before Qt SQL");
                try {
                    std::rethrow_exception(error);
                } catch (const ApplicationReferenceViewFailure &e) {
                    require(e.error() == ApplicationReferenceViewError::wrong_thread,
                            "Actual local view wrong_thread tag");
                    seen = true;
                    throw;
                }
            };
            auto ports = p.ports();
            OriginalApplicationService service(writer, ports);
            const auto r = prepared_run(service, writer, f.data.command("add10"));
            known(r->service);
            if (budget) {
                require(
                    r->service.work_error == WorkError::result_limit &&
                        r->service.failure->source == ApplicationServiceFailureSource::retained &&
                        std::holds_alternative<WorkError>(r->service.failure->reason) &&
                        std::get<WorkError>(r->service.failure->reason) == WorkError::result_limit,
                    "Actual already-rolled-back query reason maps without second cleanup");
            } else {
                require(seen, "Foreign exception transferred safely to owner");
                local_failure(r->service, ApplicationServiceFailureSource::port_contract,
                              LR::invalid_output);
            }
            unchanged(f, before);
        });
    }
    void recordSemanticFormatsAreChecked_data() {
        case_rows({"unknown_state_format", "known_state_bad_magic", "known_state_trailing_byte",
                   "known_state_typed_contradiction", "unknown_historical_result_format",
                   "known_historical_result_trailing_byte"});
    }
    void recordSemanticFormatsAreChecked() {
        QFETCH(QString, name);
        scenario([&] {
            const bool historical_case = name.contains("historical");
            File f(historical_case ? QString{} : QString{"add10"});
            if (historical_case)
                f.arrange("history");
            f.setup([&](const auto &s) { f.data.apply_fault(s, name); });
            auto c = historical_case ? command(historical(f.data, "A")["command"])
                                     : f.data.command("add10");
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            p.script.expected_command = c;
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, c);
            if (name.startsWith("unknown"))
                not_ready(r->service);
            else {
                known(r->service);
                require(r->service.failure->source ==
                            ApplicationServiceFailureSource::retained_semantics,
                        "Known malformed/contradictory document is semantic failure, no fallback");
            }
            require(std::find(p.script.calls.begin(), p.script.calls.end(), "actions") ==
                        p.script.calls.end(),
                    "Unsupported/malformed retained meaning does not prepare actions");
            unchanged(f, before);
        });
    }
    void resultBoundsCountOwnedPayloadOnly_data() {
        QTest::addColumn<int>("row");
        for (int n = 0; n != 4; ++n)
            QTest::newRow(QByteArray::number(n).constData()) << n;
    }
    void resultBoundsCountOwnedPayloadOnly() {
        QFETCH(int, row);
        scenario([&] {
            File f;
            f.arrange("history");
            const auto h = historical(f.data, "A");
            const auto c = command(h["command"]);
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            p.script.expected_command = c;
            ApplicationServiceLimits l;
            l.result_owned_bytes =
                row < 2 ? (row == 1 ? 2131 : 2132) : 4194304; // frozen record2131 + disposition1.
            l.result_items =
                row < 2 ? 65536 : (row == 3 ? 10 : 11); // No extra decoded-command/segment items.
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports, l);
            const auto r = prepared_run(service, f.writer, c);
            if (row == 1 || row == 3)
                known(r->service);
            else {
                disposition(r->service, D::existing_operation);
                check_record(std::get<RecordedApplicationBundle>(r->service.response->payload),
                             record(h));
            }
            unchanged(f, before);
        });
    }
    // Real engine prerequisite makes this B08 fault reachable only AFTER actual
    // operation/effect/new-state/application patch and BEFORE head publication.
    void actualKnownRollbackAtLateJobWrite() {
        scenario([&] {
            File f("add10");
            const auto before = f.snapshot();
            (void)f.observer->rows(
                "CREATE TRIGGER service_job_fault BEFORE INSERT ON subathon_app_jobs "
                "WHEN NEW.operation_scope=X'737663' AND NEW.operation_ref=X'6164643130' "
                "AND EXISTS(SELECT 1 FROM subathon_app_operations WHERE "
                "operation_scope=NEW.operation_scope AND operation_ref=NEW.operation_ref) "
                "AND EXISTS(SELECT 1 FROM subathon_app_effects WHERE "
                "operation_scope=NEW.operation_scope AND operation_ref=NEW.operation_ref AND "
                "request_ordinal=1) "
                "AND EXISTS(SELECT 1 FROM subathon_app_states WHERE token=X'63' AND "
                "main_remaining_ms=60) "
                "AND EXISTS(SELECT 1 FROM subathon_contributions WHERE "
                "contribution_id=X'0000000000000000' AND application=1) "
                "AND EXISTS(SELECT 1 FROM subathon_app_head WHERE singleton=1 AND token=X'61') "
                "BEGIN SELECT RAISE(ABORT,'synthetic service B08 after partial stage'); END");
            require(
                f.observer
                        ->rows(
                            "SELECT count(*) FROM sqlite_schema WHERE name='service_job_fault'")[0]
                                                                                                [0]
                        .toInt() == 1,
                "Fault installed on idle independent observer, never fake port SQL");
            Ports p(f.data, "add10");
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command("add10"));
            known(r->service);
            require(
                r->raw.error == WorkError::sql_failure &&
                    r->service.failure->source == ApplicationServiceFailureSource::bundle &&
                    std::holds_alternative<SqliteBundleStorageReason>(r->service.failure->reason) &&
                    std::get<SqliteBundleStorageReason>(r->service.failure->reason) ==
                        SqliteBundleStorageReason::sql_failure &&
                    !r->raw.value,
                "Genuine midstage known SQL rollback, not later caller exception");
            unchanged(f, before);
            (void)f.observer->rows("DROP TRIGGER service_job_fault");
            p.script.calls.clear();
            const auto retry = prepared_run(service, f.writer, f.data.command("add10"));
            disposition(retry->service, D::applied);
            check_record(f.lookup(f.data.command("add10").operation),
                         record(f.data.case_data("add10")["expected_record"]));
        });
    }
    void commitAckLossHasNoNormalResponse_data() {
        QTest::addColumn<bool>("duplicate");
        QTest::newRow("new-bundle") << false;
        QTest::newRow("old-duplicate-still-unknown") << true;
    }
    void commitAckLossHasNoNormalResponse() {
        QFETCH(bool, duplicate);
        scenario([&] {
            File f(duplicate ? QString{} : QString{"add10"});
            if (duplicate)
                f.arrange("history");
            const auto c =
                duplicate ? command(historical(f.data, "A")["command"]) : f.data.command("add10");
            Ports p(f.data, "add10");
            p.script.expected_command = c;
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        f.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Actual next physical COMMIT acknowledgement loss seam");
            const auto r = prepared_run(service, f.writer, c);
            require(r->raw.transaction == TransactionOutcome::outcome_unknown &&
                        r->raw.error == WorkError::uncertain_outcome && !r->raw.value &&
                        !r->service.response && r->callbacks == 1,
                    "Physical COMMIT/old durable duplicate cannot produce a normal payload after "
                    "unknown");
            require(std::count(p.script.calls.begin(), p.script.calls.end(), "access") == 1,
                    "No blind retry/replay");
            if (!duplicate) {
                require(
                    f.observer->rows("SELECT count(*) FROM subathon_app_operations")[0][0]
                                .toInt() == 1 &&
                        f.observer->rows("SELECT count(*) FROM subathon_app_jobs")[0][0].toInt() ==
                            1 &&
                        f.observer->rows("SELECT count(*) FROM subathon_app_claims")[0][0]
                                .toInt() == 1 &&
                        f.observer->rows("SELECT hex(token) FROM subathon_app_head")[0][0]
                                .toString() == "63",
                    "Independent observer proves physical whole commit, never an applied response");
            }
        });
    }
    void rollbackAckLossOutranksLocalFailure() {
        scenario([&] {
            File f("add10");
            const auto before = f.snapshot();
            Ports p(f.data, "add10");
            std::get<BoundApplicationPolicy>(p.script.bound_policy).checked_state_token = "wrong";
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        f.writer, testing::WorkControlFault::lose_next_rollback_ack) ==
                        StorageError::none,
                    "Actual physical rollback acknowledgement fault");
            const auto r = prepared_run(service, f.writer, f.data.command("add10"));
            require(r->service.transaction == TransactionOutcome::outcome_unknown &&
                        r->service.work_error == WorkError::uncertain_outcome &&
                        !r->service.response && !r->raw.value,
                    "Unknown cleanup outranks remembered local invalid binding, never forged known "
                    "rollback");
            unchanged(f, before);
        });
    }
    void postCommitBodyTailCannotPublish() {
        scenario([&] {
            File f("add10");
            Ports p(f.data, "add10");
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            bool tail = false;
            const auto r = prepared_run(
                service, f.writer, f.data.command("add10"), [&](QVariant value) -> QVariant {
                    require(
                        value.metaType() == QMetaType::fromType<ApplicationServiceResponse>() &&
                            f.observer->rows("SELECT count(*) FROM subathon_app_operations")[0][0]
                                    .toInt() == 1,
                        "Real acknowledged COMMIT before body tail, value still provisional");
                    tail = true;
                    throw std::runtime_error(
                        "synthetic exception AFTER actual COMMIT, BEFORE normal body return");
                });
            require(tail && r->raw.transaction == TransactionOutcome::committed &&
                        r->raw.error == WorkError::work_exception && !r->raw.value &&
                        !r->service.response,
                    "Durable accounting with NO normal payload/publication");
            check_record(f.lookup(f.data.command("add10").operation),
                         record(f.data.case_data("add10")["expected_record"]));
        });
    }
    void wrongWorkValueCannotPublish_data() {
        QTest::addColumn<bool>("invalid");
        QTest::newRow("wrong-metatype") << false;
        QTest::newRow("invalid-QVariant") << true;
    }
    void wrongWorkValueCannotPublish() {
        QFETCH(bool, invalid);
        scenario([&] {
            File f("add10");
            Ports p(f.data, "add10");
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r =
                prepared_run(service, f.writer, f.data.command("add10"), [&](QVariant) -> QVariant {
                    return invalid ? QVariant{}
                                   : QVariant{QString("not an owned service response")};
                });
            require(r->raw.transaction == TransactionOutcome::committed &&
                        r->raw.error == WorkError::none && r->raw.value && !r->service.response &&
                        r->service.failure &&
                        r->service.failure->source == ApplicationServiceFailureSource::bridge &&
                        std::get<LR>(r->service.failure->reason) == LR::invalid_completion_payload,
                    "Exact typed bridge extraction, no coercion/captured success substitute");
            check_record(f.lookup(f.data.command("add10").operation),
                         record(f.data.case_data("add10")["expected_record"]));
        });
    }
    void completionFailureDoesNotUndoDurability() {
        scenario([&] {
            File f("add10");
            Ports p(f.data, "add10");
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto r = prepared_run(service, f.writer, f.data.command("add10"), {}, true);
            disposition(r->service, D::applied);
            require(r->raw.transaction == TransactionOutcome::committed && r->raw.value,
                    "Normal typed result existed before later response callback threw");
            check_record(f.lookup(f.data.command("add10").operation),
                         record(f.data.case_data("add10")["expected_record"]));
        });
    }
    void fifoAcceptanceAwardsOnce() {
        scenario([&] {
            File f("add10");
            Ports p(f.data, "add10");
            auto ports = p.ports();
            OriginalApplicationService service(f.writer, ports);
            const auto c = f.data.command("add10");
            std::vector<ApplicationServiceCompletion> results;
            require(
                service.enqueue(c, [&](auto r) { results.push_back(std::move(r)); }).error ==
                        StorageError::none &&
                    service.enqueue(c, [&](auto r) { results.push_back(std::move(r)); }).error ==
                        StorageError::none,
                "Two separately queued actual requests");
            require(QTest::qWaitFor([&] { return results.size() == 2; }, 10000),
                    "Both actual FIFO completions");
            disposition(results[0], D::applied);
            disposition(results[1], D::existing_operation);
            const auto expected = record(f.data.case_data("add10")["expected_record"]);
            check_record(std::get<RecordedApplicationBundle>(results[0].response->payload),
                         expected);
            check_record(std::get<RecordedApplicationBundle>(results[1].response->payload),
                         expected);
            require(p.script.calls == std::vector<std::string>{"access", "policy", "clock",
                                                               "actions", "reference:0",
                                                               "reference:1", "access",
                                                               "disclose_application"} &&
                        f.observer->rows("SELECT count(*) FROM subathon_app_operations")[0][0]
                                .toInt() == 1,
                    "Exactly one award; second attempt never revalues or allocates");
        });
    }
    void queuedAndReturnedBytesStayOwned() {
        scenario([&] {
            std::optional<RecordedApplicationBundle> retained;
            Data expected;
            {
                File f("add10");
                Ports p(f.data, "add10");
                auto ports = p.ports();
                OriginalApplicationService service(f.writer, ports);
                auto c = f.data.command("add10");
                QByteArray backing = QByteArray::fromHex("efbbbf00ff41");
                c.choices[0].document =
                    QByteArray::fromRawData(backing.constData(), backing.size());
                bool done = false;
                require(service.enqueue(c,
                                        [&](auto r) {
                                            disposition(r, D::applied);
                                            retained = std::get<RecordedApplicationBundle>(
                                                r.response->payload);
                                            done = true;
                                        })
                                .error == StorageError::none,
                        "Owned queued admission");
                backing.fill('X');
                c.operation.operation_ref = "mutated";
                c.principal.principal_ref = "mutated";
                c.preview_state_token = "mutated";
                require(QTest::qWaitFor([&] { return done; }, 10000),
                        "Snapshot input survives caller mutation");
            }
            require(retained.has_value(),
                    "Owned retained value survives service/ports/writer/database scope");
            check_record(*retained, record(expected.case_data("add10")["expected_record"]));
            require(
                retained->jobs[0].envelope.document.contains(QByteArray::fromHex("efbbbf00ff41")),
                "Nonempty BOM/NUL/FF ownership oracle, not an empty-buffer assertion");
        });
    }
    void freshServiceReopenReturnsOriginal() {
        scenario([&] {
            File f("add10");
            {
                Ports p(f.data, "add10");
                auto ports = p.ports();
                OriginalApplicationService service(f.writer, ports);
                disposition(prepared_run(service, f.writer, f.data.command("add10"))->service,
                            D::applied);
            }
            bool closed = false;
            require(f.writer.shutdown([&] { closed = true; }) == StorageError::none &&
                        QTest::qWaitFor([&] { return closed; }, 10000),
                    "Real writer closes before fresh instance");
            SqliteWriter reopened;
            require(reopened.open(f.path, limits()) == StorageError::none,
                    "Same committed file reopened by new owner");
            Ports p(f.data, "add10");
            auto ports = p.ports();
            OriginalApplicationService service(reopened, ports);
            const auto r = prepared_run(service, reopened, f.data.command("add10"));
            disposition(r->service, D::existing_operation);
            check_record(std::get<RecordedApplicationBundle>(r->service.response->payload),
                         record(f.data.case_data("add10")["expected_record"]));
            require(p.script.calls == std::vector<std::string>{"access", "disclose_application"} &&
                        f.observer->rows("SELECT count(*) FROM subathon_app_operations")[0][0]
                                .toInt() == 1,
                    "Fresh instance returns original, without effect/recovery/delivery replay");
        });
    }
};
QTEST_GUILESS_MAIN(ApplicationServiceTests)
#include "application_service_tests.moc"
