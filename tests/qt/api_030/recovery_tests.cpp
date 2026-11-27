// SPDX-License-Identifier: GPL-3.0-or-later
#include "candidate.hpp"
#include "../storage_026/receipt_fixture.hpp"
#include "../storage_026/transition_fixture.hpp"
#include <subathon/server/modifier_capture.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
namespace {
namespace fixture = storage_026_fixture;
namespace candidate = subathon::server::connector_intake::recovery_candidate;
namespace intake = subathon::server::connector_intake;
namespace core = subathon::core;
using namespace subathon::server;
using fixture::frame_fields;
using qt_test_support::require;
constexpr candidate::Reservation allowance{1, 11, 1, 8};
// clang-format off
constexpr StorageLimits minimum{
    64, 26 + 2 * allowance.statements, 65536, 172528 + 2 * allowance.bound_bytes,
    26 + 2 * allowance.result_rows, 6803914 + 2 * allowance.result_bytes};
// clang-format on
const intake::StartupBinding binding = intake::StartupExecutionAccount{"install", "owner"};
struct Input {
    intake::ManagedInput managed;
    RetainedDocument detail;
};
enum class Origin { live, catch_up, import };
Input envelope(
    QByteArray stream, QByteArray key, qint64 arrival, QByteArray valuation,
    Origin origin = Origin::live,
    intake::IntakeAdmission admission = intake::IntakeAdmission::creation_eligible_verified_support,
    QByteArray detail_key = {}) {
    const intake::Scope scope{"install", "P", "A", stream};
    const QByteArray context_key = "modifier-" + key;
    if (detail_key.isEmpty())
        detail_key = "detail-" + key;
    const auto name = valuation.toStdString();
    const auto config_key = "configured-" + name;
    const auto identity = core::ReceiptIdentity{
        {"P", "A"}, frame_fields({stream, "R", "support"}).toStdString(), key.toStdString()};
    const core::ReceiptArrival original{arrival, "connection", config_key,
                                        context_key.toStdString()};
    const auto event = "event-" + key.toStdString();
    const auto observed = prepare_modifier_arrival({identity, original, {event, event, {}}, {}},
                                                   {262144, 262144, 65536});
    require(std::holds_alternative<PreparedModifierArrival>(observed),
            "Protected modifier owner prepares its arrival document");
    const auto complete = ReferenceFactsState::complete;
    // clang-format off
    const ReferenceRule rule{"rule-" + name, "rev-" + name, "action", "action-r1", "timer",
                             valuation == "VL" ? 60000 : 120000,
                             {ApplicationPrincipalKind::automation, "install", "owner"}, "cap"};
    // clang-format on
    ReferenceConfiguration configuration;
    configuration.configuration_ref = config_key;
    configuration.policy_revision_ref = "policy";
    configuration.synthetic_origin = identity.origin;
    configuration.receipt_namespace = identity.identifier_namespace;
    configuration.rule = rule;
    configuration.timer_config = {core::TimerEndMode::Immediate, 0, core::GraceReuse::Fresh, {}};
    configuration.other_rules = configuration.modifiers = configuration.milestones =
        configuration.outbound_actions = configuration.generated_events = complete;
    // Wire oracle from the frozen field table, without a candidate encoder.
    QByteArray detail("ARD1\1", 5);
    detail += frame_fields(
        {"install", "P", "A", stream, key, QByteArray::fromStdString(config_key), context_key});
    detail += fixture::be64(arrival);
    detail += char(1) + frame_fields({"shared-link", "activity", "verification"});
    detail += char(1) + frame_fields({"connection"});
    detail += char(1) + fixture::be64(1000);
    const bool support = admission == intake::IntakeAdmission::creation_eligible_verified_support;
    detail.append(static_cast<char>(admission));
    detail.append(char(1)); // accept provider report
    if (support) {
        detail.append(static_cast<char>(origin));
        if (origin == Origin::catch_up)
            detail.append(char(1)); // automatic catch-up
    }
    detail += frame_fields({"decision", "reported support"});
    detail.append(static_cast<char>(support));
    if (support)
        detail += frame_fields({QByteArray::fromStdString(rule.rule_ref),
                                QByteArray::fromStdString(rule.rule_version_ref)});
    detail.append(QByteArray::fromHex("0201")); // resolved with revision present
    detail += frame_fields({valuation}) + char(1) + frame_fields({"quoted-rate"});
    // clang-format off
    const core::ReceiptFacts facts{"support", 1, 1000, {}, {},
                                   core::ReceiptMoney{"20.00", "EUR"}, {}, {}};
    const RetainedDocument config{{RetainedKind::configuration, config_key},
                                  encode_reference_configuration(configuration).document, {}};
    const auto history = origin == Origin::live ? std::nullopt
                                                : std::optional<intake::PageSlot>{{"O", 1, 1}};
    const auto normalized = history ? fixture::normalized_item(18) : "normalized-" + key;
    return {{scope, key, normalized, QByteArray("ARC1\1", 5) + frame_fields({detail_key}),
             {identity, facts, original},
             VerifiedActivityLink{{{"P", "A"}, "shared-link", "activity"}, "verification"},
             admission, history, config, std::get<PreparedModifierArrival>(observed).retained,
             {1, 9, 1, "g3"}},
            {{RetainedKind::arrival_context, detail_key.toStdString()}, std::move(detail),
             ConfigurationReference{config_key}}};
    // clang-format on
}
struct File {
    fixture::Populated fixture_data;
    fixture::CatalogFile &db = fixture_data.db;
    File() {
        fixture::intake_origin(fixture_data);
        fixture::mutate(db,
                        "INSERT INTO main.subathon_intake_sources "
                        "VALUES(?,?,?,?,1,?,9,1,1,?,?,?)",
                        {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("H"),
                         QByteArray("g3"), QByteArray("cut"), QByteArray("F"),
                         QByteArray("cursor-contract")});
    }
    void seed_page(const Input &input, Origin origin) {
        if (origin == Origin::live)
            return;
        fixture::mutate(db, "UPDATE main.subathon_intake_policies SET document=? "
                            "WHERE installation=X'696e7374616c6c' AND revision=1",
                        {frame_fields({"automatic", "policy", "decision"})});
        fixture::catchup(db, 1, 1);
        const auto page = fixture::page_admission(1, {input.managed.normalized}).binding;
        if (origin == Origin::catch_up) {
            fixture::anchor_page(db, page);
            return;
        }
        fixture::mutate(db,
                        "UPDATE main.subathon_intake_operations "
                        "SET purpose=1,gap_ref=NULL,intent=? WHERE operation_ref=?",
                        {fixture::import_frame(), QByteArray("O")});
        fixture::mutate(db,
                        "INSERT INTO main.subathon_intake_pages VALUES("
                        "X'696e7374616c6c',X'4f',1,?,?,1,0,0,NULL,1)",
                        {page.page, page.expected_continuation});
    }
    intake::AttemptLocator seed(const Input &input, bool retain_detail = true) {
        std::optional<intake::ManagedResult> result;
        const auto work = qt_test_support::run_work(
            db.writer(),
            [&](auto context) -> QVariant {
                const auto session = context.begin();
                if (retain_detail) {
                    SqliteRetainedContextStore store(session);
                    const auto config = store.retain(*input.managed.configuration);
                    require(config == RetainDisposition::inserted ||
                                config == RetainDisposition::already_present,
                            "Independent seed retains the linked original configuration");
                    require(store.retain(input.detail) == RetainDisposition::inserted,
                            "Independent writer seed retains exact intake detail");
                }
                result = intake::admit_managed(session, input.managed);
                return 1;
            },
            10000);
        fixture::finance::normal(work);
        require(result && result->status == intake::ManagedStatus::committed && result->attempt,
                "Protected receipt and entry commit the seed");
        return *result->attempt;
    }
};
class Authority final : public intake::StartupAuthority {
  public:
    QByteArray denied_stream, overrun_stream;
    bool unavailable = false;
    enum class Fault { none, action = 4, scope, empty, oversized };
    Fault fault = Fault::none;
    QList<QByteArray> seen;
    intake::StartupDecision resolve(const SqliteWorkSession &session,
                                    const intake::StartupAuthorityRequest &request) override {
        require(request.action == intake::StartupAction::inspect && request.require_fresh &&
                    std::holds_alternative<intake::StartupExecutionAccount>(request.binding),
                "Recovery uses the existing fresh Inspect gate and binding");
        const auto &account = std::get<intake::StartupExecutionAccount>(request.binding);
        require(account.installation == "install" && account.account == "owner",
                "Recovery preserves the exact supplied authority binding");
        seen.append(request.scope.stream);
        (void)session.execute({"SELECT length(?)", {QByteArray("abc")}});
        if (request.scope.stream == overrun_stream)
            (void)session.execute({"SELECT 2", {}});
        if (request.scope.stream == denied_stream) {
            if (unavailable)
                return intake::StartupUnavailable{};
            return intake::StartupDenied{};
        }
        intake::StartupPermit permit{request.action, request.scope, "principal", "account",
                                     "permission",   "actor",       "decision"};
        if (fault == Fault::action)
            permit.action = intake::StartupAction::register_first;
        else if (fault == Fault::scope)
            permit.scope.stream = "other";
        else if (fault == Fault::empty)
            permit.actor.clear();
        else if (fault == Fault::oversized)
            permit.decision = QByteArray(4097, 'x');
        return permit;
    }
};
candidate::Recovery read(File &file, const Input &input, Authority &gate,
                         std::optional<intake::AttemptLocator> locator = {}) {
    return fixture::read_missing(file.db, [&](const auto &session) {
        return candidate::recover_intake(session, gate, binding, true, allowance,
                                         input.managed.scope, input.managed.intake_ref, locator);
    });
}
void expect_refusal(const candidate::Recovery &result, candidate::Status expected) {
    require(result.status == expected, "Recovery returns the required typed outcome");
    require(!result.selected && !result.original, "Non-success publishes no partial closure");
}
void expect_closure(const candidate::Closure &selected, const Input &input,
                    intake::AttemptLocator locator) {
    const auto &managed = input.managed;
    require(selected.scope == managed.scope && selected.intake_ref == managed.intake_ref &&
                selected.locator == locator && selected.identity == managed.receipt.identity &&
                selected.history == managed.history && selected.activity == managed.activity &&
                selected.admission == managed.admission,
            "Exact stable key resolves the requested receipt and attempt locator");
    require(selected.attempt.arrival == managed.receipt.arrival &&
                selected.attempt.observed_facts == managed.receipt.facts &&
                selected.attempt.disposition == core::ReceiptDisposition::created,
            "Retain source facts and original arrival, without the retry body");
    require(selected.normalized == managed.normalized && selected.capture == managed.capture &&
                selected.modifier_arrival == *managed.context && selected.detail == input.detail &&
                selected.configuration == *managed.configuration,
            "Exact immutable context and wire bytes survive independent readback");
}
} // namespace
class Api030RecoveryTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void originalSurvivesRestart_data() {
        QTest::addColumn<bool>("high_first");
        QTest::addColumn<int>("origin");
        QTest::addColumn<bool>("committed_evidence");
        QTest::newRow("low-first-cross-stream") << false << int(Origin::live) << false;
        QTest::newRow("high-first-cross-stream") << true << int(Origin::live) << false;
        QTest::newRow("live-first") << false << int(Origin::catch_up) << false;
        QTest::newRow("catch-up-first") << true << int(Origin::catch_up) << false;
        QTest::newRow("import-first") << true << int(Origin::import) << false;
        QTest::newRow("peer-links-existing-original") << false << int(Origin::catch_up) << true;
    }
    void originalSurvivesRestart() {
        QFETCH(bool, high_first);
        QFETCH(int, origin);
        QFETCH(bool, committed_evidence);
        fixture::check([&] {
            File file;
            const bool history = origin != int(Origin::live);
            const auto low = envelope("S", "low", 8000, "VL");
            const auto high = envelope(history ? "S" : "H", "high", 9000, "VH", Origin(origin));
            file.seed_page(high, Origin(origin));
            const auto &first = high_first ? high : low;
            const auto &peer = high_first ? low : high;
            const auto original_id = file.seed(first);
            file.db.reopen();
            const auto peer_id = file.seed(peer);
            std::optional<core::ContributionId> cid;
            if (committed_evidence) {
                // Arrange only: use the owning ACCT-002 unit to prepare the durable predecessor.
                const auto &input = first.managed;
                cid =
                    fixture::evidence(
                        file.db, {input.intake_ref, input.receipt, *input.activity, input.capture},
                        original_id.receipt)
                        .contribution_id;
                require(
                    cid && file.db.rows(
                               "SELECT c.contribution_id,c.original_receipt_id,o.kind,r.receipt_id "
                               "FROM subathon_contributions c "
                               "JOIN subathon_contribution_observations o ON "
                               "o.receipt_id=c.original_receipt_id "
                               "JOIN subathon_receipts r ON r.receipt_id=o.receipt_id") ==
                               SqlRows{{fixture::blob8(cid->value),
                                        fixture::blob8(original_id.receipt.value), 0,
                                        fixture::blob8(original_id.receipt.value)}},
                    "Arrange has one exact committed original, receipt and kind-zero observation");
            }
            const auto before = fixture::intake_cursors(file.db);
            const auto snapshot = [&] {
                return std::pair{fixture::protected_data(file.db),
                                 file.db.rows("SELECT * FROM main.subathon_app_effects")};
            };
            const auto evidence_before = snapshot();
            file.db.reopen(minimum);
            Authority authority;
            const auto result = read(file, peer, authority, peer_id);
            require(result.status == candidate::Status::recovered && result.selected &&
                        result.original,
                    "Key-only peer recovery publishes both exact committed closures");
            expect_closure(*result.selected, peer, peer_id);
            expect_closure(*result.original, first, original_id);
            const auto retained_rule = decode_reference_configuration(
                {"subathon.reference.configuration.v1", result.original->configuration.document});
            require(retained_rule.rule.add_ms == (high_first ? 120000 : 60000),
                    "The selected original retains its own rule, independent of the later peer");
            require(authority.seen ==
                        QList<QByteArray>{peer.managed.scope.stream, first.managed.scope.stream},
                    "Authorize requested and resolved scopes independently in order");
            require(file.db.rows("SELECT count(*) FROM main.subathon_intake_basis") ==
                            SqlRows{{1}} &&
                        file.db.rows("SELECT count(*) FROM main.subathon_receipt_attempts") ==
                            SqlRows{{2}} &&
                        fixture::intake_cursors(file.db) == before,
                    "Readback does not consume an attempt, reselect the basis or move history");
            require(snapshot() == evidence_before,
                    "Key-only recovery publishes no new evidence or effects");
            if (history) {
                const auto evidence = [&](const candidate::Closure &closure) {
                    return fixture::read_missing(file.db, [&](const auto &session) {
                        return intake::select_evidence(
                            session, {closure.scope,
                                      closure.intake_ref,
                                      closure.normalized,
                                      closure.capture,
                                      {closure.identity, closure.attempt.observed_facts,
                                       closure.attempt.arrival},
                                      closure.activity,
                                      closure.admission,
                                      closure.history});
                    });
                };
                const auto blocked = evidence(*result.selected);
                require(snapshot() == evidence_before,
                        "Recovery and its handoff publish no new evidence or effects");
                if (cid) {
                    require(blocked.status == intake::EvidenceStatus::may_link &&
                                blocked.original == cid && blocked.basis == original_id &&
                                blocked.capture == first.managed.capture,
                            "A peer may only link to the exact already-committed original");
                    return;
                }
                const auto eligible = evidence(*result.original);
                require(blocked.status == intake::EvidenceStatus::pending_original &&
                            !blocked.original &&
                            eligible.status == intake::EvidenceStatus::may_create &&
                            eligible.basis == original_id &&
                            eligible.capture == first.managed.capture,
                        "The protected evidence gate cannot promote the recovered peer");
                fixture::mutate(file.db, "UPDATE main.subathon_intake_basis SET capture=?",
                                {QByteArray("foreign capture")});
                const auto inconsistent = evidence(*result.original);
                require(inconsistent.status == intake::EvidenceStatus::inconsistent_original &&
                            !inconsistent.basis && !inconsistent.original &&
                            !inconsistent.capture &&
                            file.db.rows("SELECT count(*) FROM main.subathon_contributions") ==
                                SqlRows{{0}},
                        "An inconsistent original has no handoff and executes no effects");
            }
        });
    }
    void authorityFence_data() {
        QTest::addColumn<int>("fault");
        // clang-format off
        const char *names[]{"requested-denied", "requested-unavailable", "original-denied",
                            "original-unavailable", "action", "scope", "empty", "oversized",
                            "requested-overrun", "original-overrun"};
        // clang-format on
        for (int i = 0; i < std::ssize(names); ++i)
            QTest::newRow(names[i]) << i;
    }
    void authorityFence() {
        QFETCH(int, fault);
        fixture::check([&] {
            using Status = candidate::Status;
            File file;
            const bool original = fault == 2 || fault == 3 || fault == 9;
            auto first = envelope("S", "first", 8000, "VL");
            if (fault < 8)
                first.detail.document.append(char(0));
            file.seed(first);
            const auto peer = envelope("H", "peer", 9000, "VH");
            if (original)
                file.seed(peer);
            const auto &selected = original ? peer : first;
            Authority authority;
            if (fault < 4) {
                authority.denied_stream = "S";
                authority.unavailable = fault % 2 != 0;
            } else if (fault < 8) {
                authority.fault = Authority::Fault(fault);
            } else {
                authority.overrun_stream = "S";
            }
            expect_refusal(read(file, selected, authority),
                           fault == 0 || fault == 2 ? Status::denied : Status::unavailable);
            require(authority.seen ==
                        (original ? QList<QByteArray>{"H", "S"} : QList<QByteArray>{"S"}),
                    "The targeted gate stops recovery before malformed or unauthorized payload");
        });
    }
    void entryAndBasisStatus_data() {
        QTest::addColumn<int>("fault");
        const char *names[]{"locator-mismatch", "no-basis", "missing", "retained-unidentified"};
        for (int i = 0; i < std::ssize(names); ++i)
            QTest::newRow(names[i]) << i;
    }
    void entryAndBasisStatus() {
        QFETCH(int, fault);
        fixture::check([&] {
            using Status = candidate::Status;
            File file;
            const auto item = envelope("S", "selected", 8000, "VL");
            std::optional<intake::AttemptLocator> locator;
            if (fault < 2) {
                locator = file.seed(item);
                if (fault == 0)
                    ++locator->ordinal;
                else
                    fixture::mutate(file.db, "DELETE FROM main.subathon_intake_basis");
            } else if (fault == 3) {
                file.db.write([&](const auto &session) {
                    require(intake::stage_quarantine(session,
                                                     {item.managed.scope, item.managed.intake_ref,
                                                      "opaque-observation", "local-reason", 8000,
                                                      item.managed.expected_source}) ==
                                intake::QuarantineStatus::staged,
                            "Protected quarantine owns the unidentified row");
                });
            }
            Authority authority;
            const auto result = read(file, item, authority, locator);
            if (fault == 1)
                require(result.status == Status::no_basis && !result.original,
                        "No retained basis supplies no substitute original");
            else {
                const Status statuses[]{Status::locator_mismatch, Status::no_basis, Status::missing,
                                        Status::not_identified};
                expect_refusal(result, statuses[fault]);
            }
            require(file.db.rows("SELECT count(*) FROM main.subathon_receipt_attempts") ==
                        SqlRows{{fault < 2 ? 1 : 0}},
                    "Recovery never manufactures a receipt attempt");
        });
    }
    void detailPreflight_data() {
        QTest::addColumn<int>("fault");
        // clang-format off
        const char *names[]{"oversized-detail", "trailing-byte", "arrival", "scope",
                            "config", "mca1-key", "connection", "source-utc", "activity",
                            "foreign-entry", "missing-detail", "aliases-mca1", "wrong-role",
                            "oversized-basis", "legacy", "oversized-legacy", "oversized-current",
                            "malformed-current", "changed-basis", "invalid-presence",
                            "unknown-class", "unknown-policy", "unknown-origin", "unknown-mode",
                            "unknown-valuation", "valuation-presence", "audit-mode",
                            "confirmation-rule", "live-mode", "import-mode", "empty-required",
                            "unsupported-arc1-version", "audit-rule", "confirmation-mode",
                            "unknown-source-quality", "invalid-rule-presence", "missing-rate-with-resolution",
                            "oversized-field", "length-past-eof", "arc1-trailing", "arc1-empty",
                            "wrong-classification", "wrong-route-origin", "wrong-rule-revision",
                            "resolved-without-resolution", "invalid-connection-presence"};
        // clang-format on
        for (int i = 0; i < std::ssize(names); ++i)
            QTest::newRow(names[i]) << i;
    }
    void detailPreflight() {
        QFETCH(int, fault);
        fixture::check([&] {
            File file;
            using Admission = intake::IntakeAdmission;
            const auto origin = fault == 23                  ? Origin::catch_up
                                : fault == 29 || fault == 42 ? Origin::import
                                                             : Origin::live;
            const auto lane = fault == 26 || fault == 32 ? Admission::audit_only
                              : fault == 27 || fault == 33
                                  ? Admission::confirmation_only
                                  : Admission::creation_eligible_verified_support;
            auto item = envelope("S", "bad-detail", 8000, "VL", origin, lane);
            file.seed_page(item, origin);
            auto &bytes = item.detail.document;
            const std::pair<QByteArray, QByteArray> mismatches[]{
                {fixture::be64(8000), fixture::be64(8001)},
                {frame_fields({"P", "A", "S"}), frame_fields({"P", "A", "other"})},
                {frame_fields({"configured-VL"}), frame_fields({"other-config"})},
                {frame_fields({"modifier-bad-detail"}), frame_fields({"other-context"})},
                {frame_fields({"connection"}), frame_fields({"other-connection"})},
                {fixture::be64(1000), fixture::be64(1001)},
                {frame_fields({"activity"}), frame_fields({"other-activity"})}};
            auto &capture = item.managed.capture;
            if (fault == 0)
                bytes.resize(69733, 'x');
            else if (fault == 1)
                bytes.append(char(0));
            else if (fault >= 2 && fault <= 8) {
                const auto &[from, to] = mismatches[fault - 2];
                bytes.replace(from, to);
            } else if (fault == 9)
                bytes = envelope("S", "foreign", 8000, "VL").detail.document;
            else if (fault == 11)
                capture = QByteArray("ARC1\1", 5) + frame_fields({"modifier-bad-detail"});
            else if (fault == 12)
                bytes = item.managed.context->document;
            else if (fault >= 14 && fault <= 17) {
                const QByteArray specimens[]{"old opaque bytes", QByteArray(4106, 'x'),
                                             QByteArray("ARC1\1", 5) + QByteArray(4101, 'x'),
                                             QByteArray("ARC1\1", 5)};
                capture = specimens[fault - 14];
            }
            if (fault >= 19) {
                // Locate fields by adjacent independent fixture values, not a product decoder.
                const auto decision = bytes.indexOf(frame_fields({"decision", "reported support"}));
                const auto value = bytes.indexOf(frame_fields({"VL"}), decision);
                require(decision > 4 && value > decision, "Wire mutation anchors exist");
                // clang-format off
                switch (fault) {
                case 19: bytes[bytes.indexOf(frame_fields({"shared-link", "activity", "verification"})) - 1] = 2; break;
                case 20: bytes[decision - 3] = 127; break; // classification
                case 21: bytes[decision - 2] = 127; break; // evidence policy
                case 22: case 23: bytes[decision - 1] = 127; break; // live origin or catch-up mode
                case 24: case 25: bytes[value - 2] = fault == 24 ? 127 : 0; break;
                case 26: case 33: bytes.insert(decision, QByteArray::fromHex("0101")); break;
                case 27: case 32:
                    bytes[value - 3] = 1;
                    bytes.insert(value - 2, frame_fields({"rule-VL", "rev-VL"}));
                    break;
                case 28: case 29: bytes.insert(decision, char(1)); break;
                case 30: bytes.replace(frame_fields({"decision"}), frame_fields({QByteArray{}})); break;
                case 31: capture[4] = 2; break;
                case 34: bytes[bytes.indexOf(fixture::be64(1000)) - 1] = 127; break;
                case 35: bytes[bytes.indexOf(frame_fields({"rule-VL", "rev-VL"})) - 1] = 2; break;
                case 36: bytes[value - 2] = 1; break; // missing rate cannot carry a resolution
                case 37: bytes.replace(frame_fields({"decision"}), frame_fields({QByteArray(4097, 'd')})); break;
                case 38: bytes.chop(1); break;
                case 39: capture.append(char(0)); break;
                case 40: capture = QByteArray("ARC1\1", 5) + frame_fields({QByteArray{}}); break;
                case 41: bytes[decision - 3] = 1; bytes.remove(decision - 1, 1);
                    bytes.replace(QByteArray(1, char(1)) + frame_fields({"rule-VL", "rev-VL"}), QByteArray(1, char(0)));
                    break;
                case 42: bytes[decision - 1] = 0; break; // valid live tag on a bound import
                case 43: bytes.replace(frame_fields({"rev-VL"}), frame_fields({"rev-VH"})); break;
                case 44: bytes.chop(frame_fields({"quoted-rate"}).size()); bytes.back() = 0; break;
                case 45: bytes[bytes.indexOf(frame_fields({"connection"})) - 1] = 2; break;
                }
                // clang-format on
            }
            file.seed(item, fault != 10 && fault != 11 && (fault < 14 || fault >= 18));
            if (fault == 18)
                file.db.write([&](const auto &session) {
                    auto alternate = item.detail;
                    alternate.key.reference = "another-detail";
                    require(SqliteRetainedContextStore(session).retain(alternate) ==
                                RetainDisposition::inserted,
                            "The changed basis points at a real retained document");
                });
            if (fault == 13 || fault == 18)
                fixture::mutate(file.db, "UPDATE main.subathon_intake_basis SET capture=?",
                                {fault == 13
                                     ? QByteArray(4106, 'x')
                                     : QByteArray("ARC1\1", 5) + frame_fields({"another-detail"})});
            const auto retained = fixture::protected_data(file.db);
            const auto basis = file.db.rows("SELECT capture FROM main.subathon_intake_basis");
            Authority authority;
            using Status = candidate::Status;
            const bool oversized = fault == 0 || fault == 13 || fault == 15 || fault == 16;
            expect_refusal(read(file, item, authority), fault == 14 || fault == 31
                                                            ? Status::unsupported_legacy
                                                        : oversized ? Status::resource_limit
                                                                    : Status::inconsistent);
            require(fixture::protected_data(file.db) == retained &&
                        file.db.rows("SELECT capture FROM main.subathon_intake_basis") == basis &&
                        file.db.rows("SELECT provenance FROM main.subathon_intake_entries") ==
                            SqlRows{{capture}},
                    "Refusal preserves exact committed evidence");
        });
    }
    void writerProfile_data() {
        QTest::addColumn<int>("dimension");
        const char *names[]{"statements", "binds", "rows", "result", "width", "equality"};
        for (int i = 0; i < std::ssize(names); ++i)
            QTest::newRow(names[i]) << i;
    }
    void writerProfile() {
        QFETCH(int, dimension);
        fixture::check([&] {
            File file;
            const auto item = envelope("S", "profile", 8000, "VL");
            file.seed(item);
            auto profile = minimum;
            qsizetype StorageLimits::*dimensions[]{
                &StorageLimits::statements_per_batch, &StorageLimits::bound_value_bytes,
                &StorageLimits::result_rows, &StorageLimits::result_value_bytes,
                &StorageLimits::statement_utf8_bytes};
            if (dimension < std::ssize(dimensions))
                --(profile.*dimensions[dimension]);
            file.db.reopen(profile);
            const bool qualified = candidate::qualified(file.db.writer(), allowance);
            require(qualified == (dimension == std::ssize(dimensions)),
                    "All four cumulative minima and the width threshold qualify independently");
            Authority authority;
            const auto result = read(file, item, authority);
            if (!qualified) {
                expect_refusal(result, candidate::Status::unavailable);
                require(authority.seen.isEmpty(), "Unqualified recovery never enters authority");
            } else
                require(result.status == candidate::Status::recovered, "Exact minimum recovers");
        });
    }
    void admissionClosure_data() {
        QTest::addColumn<int>("scenario");
        const char *names[]{"acknowledged",   "veto",
                            "lost-ack",       "exact-retry",
                            "changed-detail", "binding-mismatch",
                            "capture-limit",  "arc1-exact-cutoff",
                            "audit-no-rule",  "confirmation-no-rule"};
        for (int i = 0; i < std::ssize(names); ++i)
            QTest::newRow(names[i]) << i;
    }
    void admissionClosure() {
        QFETCH(int, scenario);
        fixture::check([&] {
            using Status = intake::ManagedStatus;
            using Admission = intake::IntakeAdmission;
            File file;
            const auto lane = scenario == 8   ? Admission::audit_only
                              : scenario == 9 ? Admission::confirmation_only
                                              : Admission::creation_eligible_verified_support;
            auto item = envelope("S", "commit", 8000, "VL", Origin::live, lane,
                                 scenario == 7 ? QByteArray(4096, 'd') : QByteArray{});
            const auto prior =
                scenario == 3 || scenario == 4 ? std::optional{file.seed(item)} : std::nullopt;
            const auto before = fixture::protected_data(file.db);
            const auto basis_before = file.db.rows("SELECT * FROM main.subathon_intake_basis");
            const auto entry_before = file.db.rows("SELECT * FROM main.subathon_intake_entries");
            const auto exact_capture = item.managed.capture;
            if (scenario == 4)
                item.detail.document.replace(frame_fields({"VL"}), frame_fields({"V2"}));
            if (scenario == 5 || scenario == 6)
                item.managed.capture =
                    QByteArray("ARC1\1", 5) +
                    frame_fields({scenario == 5 ? QByteArray("other") : QByteArray(4097, 'd')});
            if (scenario == 2)
                require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            file.db.writer(), testing::WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Arm only the actual next managed COMMIT acknowledgement");
            const bool stages = scenario <= 2 || scenario >= 7;
            std::optional<intake::ManagedResult> result;
            bool witnessed = false;
            const auto work = qt_test_support::run_work(
                file.db.writer(),
                [&](auto context) -> QVariant {
                    const auto session = context.begin();
                    result = candidate::admit(session, item.managed, item.detail, [&] {
                        witnessed = true;
                        if (stages) {
                            const QVariant basis =
                                scenario < 8 ? QVariant{item.managed.capture} : QVariant{};
                            const auto ref = QByteArray::fromStdString(item.detail.key.reference);
                            const auto rows = session.execute({R"SQL(
SELECT e.receipt_id,a.ordinal,e.provenance,b.capture,d.document
FROM main.subathon_intake_entries e
JOIN main.subathon_receipt_attempts a ON (a.receipt_id,a.ordinal)=(e.receipt_id,e.attempt_ordinal)
LEFT JOIN main.subathon_intake_basis b ON (b.receipt_id,b.attempt_ordinal)=(e.receipt_id,e.attempt_ordinal)
JOIN main.subathon_retained_contexts d ON d.reference=?
)SQL",
                                                               {ref}});
                            require(fixture::matches_sql_rows(
                                        rows, SqlRows{{fixture::blob8(1), 1, item.managed.capture,
                                                       basis, item.detail.document}}),
                                    "Pre-COMMIT callback observes the actual complete staged unit");
                        }
                        return scenario != 1;
                    });
                    return 1;
                },
                10000);
            require(witnessed == stages,
                    "Only a freshly staged admission reaches its veto boundary");
            if (scenario == 2) {
                require(!result && !work.value && work.error == WorkError::uncertain_outcome &&
                            work.transaction == TransactionOutcome::outcome_unknown,
                        "An actual lost COMMIT acknowledgement publishes no success");
                file.db.reopen(minimum);
            } else if (scenario == 1 || !stages) {
                const auto status = scenario == 1   ? Status::known_rollback
                                    : scenario == 3 ? Status::exact_replay
                                    : scenario == 6 ? Status::intake_limit
                                                    : Status::conflict;
                require(result && result->status == status &&
                            result->attempt == (scenario == 3 ? prior : std::nullopt) &&
                            work.transaction == TransactionOutcome::rolled_back,
                        "Known rollback, replay and refusals publish only their prescribed result");
                require(fixture::protected_data(file.db) == before &&
                            file.db.rows("SELECT * FROM main.subathon_intake_basis") ==
                                basis_before &&
                            file.db.rows("SELECT * FROM main.subathon_intake_entries") ==
                                entry_before &&
                            entry_before.size() == (prior ? 1 : 0),
                        "Veto, exact retry and conflicts do not change the committed image");
                if (scenario != 1 && scenario != 5)
                    return;
                item.managed.capture = exact_capture;
                const auto retry_work = qt_test_support::run_work(
                    file.db.writer(),
                    [&](auto ctx) {
                        result = candidate::admit(ctx.begin(), item.managed, item.detail);
                        return QVariant{1};
                    },
                    10000);
                fixture::finance::normal(retry_work);
                require(result && result->status == Status::committed &&
                            result->attempt == intake::AttemptLocator{{1}, 1},
                        "Exact retry after rejection acknowledges the first locator");
            } else {
                require(result && result->status == Status::committed &&
                            result->attempt == intake::AttemptLocator{{1}, 1},
                        "API-030 publishes the exact acknowledged locator");
                fixture::finance::normal(work);
            }
            require(file.db.rows("SELECT document FROM main.subathon_retained_contexts "
                                 "WHERE reference=?",
                                 {QByteArray::fromStdString(item.detail.key.reference)}) ==
                        SqlRows{{item.detail.document}},
                    "Committed detail matches the independent ARC1/ARD1 fixture");
            Authority authority;
            const auto recovered = read(file, item, authority, intake::AttemptLocator{{1}, 1});
            if (scenario >= 8) {
                require(recovered.status == candidate::Status::no_basis && !recovered.original,
                        "Admitted non-support has no selected rule pair or original basis");
                return;
            }
            require(recovered.status == candidate::Status::recovered && recovered.selected,
                    "Fresh key-only read closes the one committed admission");
            expect_closure(*recovered.selected, item, {{1}, 1});
            require(file.db.rows("SELECT count(*) FROM main.subathon_receipt_attempts") ==
                        SqlRows{{1}},
                    "Recovery consumes no replacement attempt");
        });
    }
};
QTEST_GUILESS_MAIN(Api030RecoveryTests)
#include "recovery_tests.moc"
