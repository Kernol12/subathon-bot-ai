// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_026/transition_fixture.hpp"
#include "history_oracle.hpp"
#include <subathon/server/connector_history_profile.hpp>

namespace {
using namespace api_029_fixture;
using Fetch = std::variant<Page, HistoryFailure>;
const QString sources_table = "subathon_intake_sources", gaps_table = "subathon_intake_gaps",
              pages_table = "subathon_intake_pages", segments_table = "subathon_intake_segments";

bool same_request(const HistoryRequest &got, const HistoryRequest &expected) {
    return got.source.scope == expected.source.scope && got.source.fence == expected.source.fence &&
           got.source.profile == expected.source.profile && got.operation == expected.operation &&
           got.ordinal == expected.ordinal && got.continuation == expected.continuation &&
           got.frontier == expected.frontier && got.interval.start == expected.interval.start &&
           got.interval.end == expected.interval.end &&
           got.interval.membership_basis == expected.interval.membership_basis &&
           got.interval.order_basis == expected.interval.order_basis &&
           got.upper.format == expected.upper.format &&
           got.upper.position_namespace == expected.upper.position_namespace &&
           got.upper.position == expected.upper.position &&
           got.upper.qualified_generation == expected.upper.qualified_generation &&
           got.upper.proof == expected.upper.proof &&
           got.resolved_start_ms == expected.resolved_start_ms;
}

class Adapter final : public ConnectorHistoryAdapter {
  public:
    HistoryRequest expected;
    Fetch response;
    CutResult cut_response{HistoryFailure::unproved};
    std::optional<CutRequest> expected_cut;
    quint64 calls{}, cut_calls{};
    explicit Adapter(Page transcript)
        : expected(transcript.request), response(std::move(transcript)) {}
    CutResult observe_cut(const CutRequest &request) override {
        require(expected_cut && same_cut_request(request, *expected_cut), "Exact cut invocation");
        ++cut_calls;
        return cut_response;
    }
    Fetch fetch_page(const HistoryRequest &request) override {
        require(same_request(request, expected), "Adapter receives the exact selected request");
        ++calls;
        return response;
    }
};

Item audit_item(const QByteArray &id, const QByteArray &position, const QByteArray &document) {
    return {"H" + id,
            "delivery",
            "S/history",
            id,
            {"support", 1, {}, {}, {}, {}, {}, {}},
            position,
            "in-range",
            {},
            {},
            SourceTimeQuality::absent,
            core::PaymentEvidence::confirmed,
            ContributionObservationKind::support,
            "individual",
            {},
            document};
}

bool same_item(const Item &got, const Item &expected) {
    return got.key == expected.key && got.role == expected.role &&
           got.identifier_namespace == expected.identifier_namespace &&
           got.identifier == expected.identifier && got.facts == expected.facts &&
           got.position == expected.position && got.membership_proof == expected.membership_proof &&
           got.disjointness_proof == expected.disjointness_proof &&
           got.activity == expected.activity && got.source_time == expected.source_time &&
           got.evidence == expected.evidence && got.kind == expected.kind &&
           got.award_class == expected.award_class &&
           got.aggregate_metric == expected.aggregate_metric &&
           got.normalized == expected.normalized;
}

bool same_range(const std::optional<RangeProof> &got, const std::optional<RangeProof> &expected) {
    if (!got || !expected)
        return got.has_value() == expected.has_value();
    return got->meaning == expected->meaning && got->lower == expected->lower &&
           got->upper == expected->upper && got->proof == expected->proof &&
           got->loss_reason == expected->loss_reason;
}

HistoryRequest catch_up(qint64 ordinal = 1, QByteArray token = "T") {
    return {{a, {2, 4, 2, "g3"}, profile},
            "O",
            ordinal,
            std::move(token),
            QByteArray("11"),
            {Endpoint::exclusive, Endpoint::inclusive, "ordered-membership", "ordered-key"},
            {"key-v1", "S/history", QByteArray("14"), "g3", "cut14"},
            {}};
}

QByteArray intent_frame() {
    return QByteArray("IOP1\x00", 5) + f("install") + f("P") + f("A") + f("S") + integer(2) +
           integer(2) + QByteArray("\x02\x01", 2) + f("ordered-membership") + f("ordered-key") +
           f(profile) + f("actor/decision") + f("G") + '\x01' + f("T") +
           f(cut("g3", "cut14", QByteArray("14")));
}

std::optional<core::ReceiptIdentity> receipt_identity(const Item &item) {
    if (!item.identifier)
        return {};
    return core::ReceiptIdentity{
        {"P", "A"},
        (f("S") + f(item.role) + f(item.identifier_namespace)).toStdString(),
        item.identifier->toStdString()};
}

struct HistoryRegistry final : StartupProfileRegistry {
    api_031_fixture::Transcript live{false};
    std::shared_ptr<const StartupProfile> current;

    void install(ConnectorHistoryAdapter &adapter, const Selection &selection,
                 QByteArray registry_revision = "registry-history") {
        current = std::make_shared<const StartupProfile>(StartupProfile{selection.scope,
                                                                        registry_revision,
                                                                        selection.profile,
                                                                        cursor,
                                                                        selection.fence.generation,
                                                                        adapter,
                                                                        live,
                                                                        {},
                                                                        {},
                                                                        {},
                                                                        {}});
    }

    std::shared_ptr<const StartupProfile> select(const Scope &scope, const QByteArray &revision,
                                                 const QByteArray &contract) override {
        return current->scope == scope && current->profile_revision == revision &&
                       current->cursor_contract == contract
                   ? current
                   : nullptr;
    }
};

struct Attempt {
    Store &db;
    const Page transcript;
    Adapter adapter;
    AdmittedHistoryRequest binding;
    bool witness{true}, order_proved{true};
    bool deny_page{}, deny_inspect{};
    QList<SlotEvidence> slots_evidence;
    std::optional<RangeProof> witnessed_range;
    QList<QuarantineEntry> quarantine;
    int witnessed{};
    HistoryRegistry registry;
    HistoryProfile registered;
    OwnerInputs owner;

    Attempt(Store &store, Page observation, qint64 revision = 1)
        : db(store), transcript(std::move(observation)), adapter(transcript),
          binding{transcript.request,
                  HistoryPurpose::catch_up,
                  cursor,
                  intent_frame(),
                  QByteArray("G"),
                  revision,
                  1,
                  "registry-history"},
          slots_evidence(transcript.items.size(), {Membership::member, Overlap::unresolved}),
          witnessed_range(transcript.coverage),
          registered{registry,
                     {},
                     [this](const PageInvocation &invocation) -> std::optional<HistoryEvidence> {
                         if (!witness || &invocation.adapter != &adapter ||
                             invocation.identity != adapter.calls || adapter.calls != 1 ||
                             !same_request(invocation.binding.request, transcript.request) ||
                             invocation.binding.purpose != binding.purpose ||
                             invocation.binding.intent != binding.intent ||
                             invocation.binding.cursor_contract != cursor ||
                             invocation.binding.gap != binding.gap ||
                             invocation.binding.operation_revision != binding.operation_revision ||
                             invocation.binding.gap_revision != binding.gap_revision ||
                             invocation.binding.registry_revision != binding.registry_revision)
                             return {};
                         const auto &page = invocation.observed;
                         if (!same_request(page.request, transcript.request) ||
                             page.next != transcript.next ||
                             !same_range(page.coverage, transcript.coverage) ||
                             !std::ranges::equal(page.items, transcript.items, same_item))
                             return {};
                         ++witnessed;
                         return evidence();
                     }},
          owner{[this](Action action, const Scope &scope) {
                    if (scope != a || (action == Action::page && deny_page) ||
                        (action == Action::inspect && deny_inspect))
                        return Authorization::denied;
                    return Authorization::allowed;
                },
                adapter,
                {},
                {}} {
        registry.install(adapter, transcript.request.source);
        registered.snapshot = registry.current;
        for (qsizetype i = 0; i < transcript.items.size(); ++i)
            slots_evidence[i].receipt_identity = receipt_identity(transcript.items[i]);
    }
    HistoryEvidence evidence() const {
        return {slots_evidence, order_proved, witnessed_range, quarantine};
    }
    void resume_registration(const Selection &selection) {
        registry.install(adapter, selection, "registry-reconnect");
        registered.snapshot = registry.current;
    }
    void retain(const QByteArray &frame) {
        require(witnessed == 1, "Retention follows the verified invocation");
        registered.reconstruct =
            [this, frame, frozen = binding, retained_evidence = evidence()](
                const RetainedPageRequest &selected) -> std::optional<RetainedHistoryPage> {
            const auto &original = selected.original;
            if (!same_request(original.request, transcript.request) ||
                original.intent != frozen.intent || original.purpose != frozen.purpose ||
                original.operation_revision != frozen.operation_revision ||
                original.gap_revision != frozen.gap_revision ||
                original.cursor_contract != frozen.cursor_contract ||
                original.registry_revision != frozen.registry_revision ||
                original.gap != frozen.gap || selected.page != frame)
                return {};
            return RetainedHistoryPage{transcript, retained_evidence};
        };
    }
    PageQualification qualify(const Page &candidate) {
        return qualify_history_page(db.writer(), owner, registered, binding, candidate);
    }
    PageQualification qualify() { return qualify(transcript); }
    std::shared_ptr<const QualifiedPage> qualified() {
        const auto result = qualify();
        require(result.status == IntakeStatus::committed && result.qualified &&
                    adapter.calls == 1 && witnessed == 1 && result.slot_evidence == slots_evidence,
                "Only the witnessed actual retrieval issues a typed page");
        return result.qualified;
    }
    PageOutcome bind(const QualifiedPage &page) {
        return bind_history_page(db.writer(), owner, page);
    }
    PageOutcome finish(const QualifiedPage &page) {
        return finalize_history_page(db.writer(), owner, page);
    }
};

void page_row(Store &db, qint64 ordinal, const QByteArray &page, const QByteArray &expected_token,
              qint64 count, qint64 prefix, qint64 outcome, const QByteArray &proof = {}) {
    auto rows =
        db.rows("SELECT page,expected_continuation,item_count,committed_prefix,outcome,proof "
                "FROM main.subathon_intake_pages WHERE ordinal=" +
                QString::number(ordinal));
    canonical_cells(rows);
    require(rows == SqlRows{{page, expected_token, count, prefix, outcome,
                             proof.isNull() ? QVariant{} : QVariant{proof}}},
            "Read back the exact P, original token, slot count, prefix, outcome and proof");
}

void operation_row(Store &db, qint64 revision, qint64 ordinal, const QByteArray &token,
                   qint64 reserved, qint64 outcome) {
    auto rows = db.rows("SELECT intent,revision,next_ordinal,continuation,slots,outcome "
                        "FROM main.subathon_intake_operations WHERE operation_ref=X'4F'");
    canonical_cells(rows);
    require(rows == SqlRows{{intent_frame(), revision, ordinal,
                             token.isNull() ? QVariant{} : QVariant{token}, reserved, outcome}},
            "Operation retains exact IOP1 and advances only the expected fields");
}

void coverage_rows(Store &db, const SqlRows &gaps, const SqlRows &segments) {
    require(db.rows("SELECT revision,status FROM main.subathon_intake_gaps") == gaps &&
                db.rows("SELECT revision,ordinal,coverage,document "
                        "FROM main.subathon_intake_segments ORDER BY revision") == segments,
            "Exact gap and ordered segment rows");
}

Page audit_page_echo() {
    return {catch_up(),
            {audit_item("12", "12", audit_items[0]), audit_item("13", "13", audit_items[1])},
            QByteArray("U"),
            RangeProof{Coverage::contiguous, "11", "13", "cover-11-13", {}}};
}

ManagedInput audit_input(const Item &item, qint64 slot) {
    const RetainedDocument configuration{{RetainedKind::configuration, "history-cfg"}, "cfg", {}};
    const RetainedDocument context{{RetainedKind::arrival_context, "history-ctx"},
                                   "ctx",
                                   ConfigurationReference{"history-cfg"}};
    return {a,
            item.key,
            item.normalized,
            "audit",
            {*receipt_identity(item), item.facts, {8000, "history", "history-cfg", "history-ctx"}},
            {},
            IntakeAdmission::audit_only,
            PageSlot{"O", 1, slot},
            configuration,
            context,
            {2, 4, 2, "g3"}};
}

auto admit_receipt(Store &db, const ManagedInput &input, std::function<bool()> veto = {}) {
    std::optional<ManagedResult> result;
    const auto work = qt_test_support::run_work(
        db.writer(),
        [&](auto ctx) -> QVariant {
            result = admit_managed(ctx.begin(), input, veto);
            return true;
        },
        10000);
    require(result.has_value(), "Managed receipt owner responds");
    return std::pair{work, *result};
}

void refused_receipt(Store &db, const ManagedInput &input, ManagedStatus expected,
                     std::function<bool()> veto = {}) {
    const auto [work, result] = admit_receipt(db, input, std::move(veto));
    require(work.transaction == TransactionOutcome::rolled_back &&
                work.error == WorkError::not_committed && result.status == expected &&
                !result.attempt,
            "Protected refusal rolls back without issuing an attempt");
}

AttemptLocator admit_audit(Store &db, const ManagedInput &input) {
    const auto [completion, receipt] = admit_receipt(db, input);
    catalog::finance::normal(completion);
    require(receipt.status == ManagedStatus::committed && receipt.attempt,
            "Protected receipt owner commits the distinct audit attempt");
    db.work([&](const auto &work) -> QVariant {
        require(stage_intake_disposition(work, input, *receipt.attempt,
                                         IntakeDisposition::observed_only) == HistoryStatus::staged,
                "Protected disposition acknowledges the exact audit slot");
        return true;
    });
    return *receipt.attempt;
}

QByteArray cut_frame(Endpoint endpoint, const QualifiedCut &cut) {
    return QByteArray("CUT1") + char(endpoint) + f(cut.format) + f(cut.position_namespace) +
           (cut.position ? QByteArray(1, '\1') + f(*cut.position) : QByteArray(1, '\0')) +
           f(cut.qualified_generation) + f(cut.proof);
}

OperationIntent import_intent(const Fence &head, QByteArray position, QByteArray proof) {
    return {a,
            "I",
            head.epoch,
            head.policy_revision,
            head.generation,
            QByteArray("T"),
            "2026-11-21",
            "UTC",
            "tzdb",
            1795219200000LL,
            {Endpoint::inclusive, Endpoint::exclusive, "date-membership", "position-order"},
            {"key-v1", "S/history", position, head.generation, proof},
            profile,
            "actor/decision",
            "actor/decision",
            HistoryPurpose::import,
            {},
            head.revision};
}
QByteArray import_bytes(const OperationIntent &intent) {
    auto specimen = frozen::import_frame();
    specimen.replace(31, 16, integer(2) + integer(2));
    const auto original_cut =
        cut_frame(Endpoint::exclusive, {"key-v1", "S/history", QByteArray("Z"), "g3", "proof"});
    specimen.chop(f(original_cut).size());
    return specimen + f(cut_frame(Endpoint::exclusive, intent.upper_cut));
}
bool exact_cut(const CutObservation &got, const CutObservation &expected) {
    return same_cut_request(got.request, expected.request) &&
           cut_frame(got.boundary, got.cut) == cut_frame(expected.boundary, expected.cut);
}

IntakeStatus open_import(Attempt &attempt, const OperationIntent &intent,
                         const HistoryRequest &request, bool malformed = false,
                         std::optional<qint64> date_echo = {}) {
    const CutRequest selected{request.source, CutPurpose::import_upper, request.frontier};
    const CutObservation observation{selected, intent.upper_cut, Endpoint::exclusive};
    auto echo = observation;
    if (malformed)
        echo.cut.position = QByteArray("contradictory");
    attempt.adapter.expected_cut = selected;
    attempt.adapter.cut_response = observation;
    if (!attempt.owner.zone)
        attempt.owner.zone = [](const QByteArray &zone, const QByteArray &rules,
                                QDate date) -> std::optional<ZoneBasis> {
            if (zone != "UTC" || rules != "tzdb" || date != QDate(2026, 11, 21))
                return {};
            return ZoneBasis{0, {}};
        };
    auto request_echo = request;
    if (date_echo)
        request_echo.resolved_start_ms = date_echo;
    return open_history_import(attempt.db.writer(), attempt.owner, attempt.registered,
                               {selected, intent, cursor, request.source.fence}, request_echo, echo,
                               [&](quint64 identity, const CutObservation &actual) {
                                   return identity == attempt.adapter.cut_calls && identity == 1 &&
                                          exact_cut(actual, observation);
                               });
}
struct PageEcho {
    const char *label;
    void (*mutate)(Page &);
};
const PageEcho page_echoes[]{
    {"scope", [](Page &page) { page.request.source.scope = b; }},
    {"epoch", [](Page &page) { ++page.request.source.fence.epoch; }},
    {"source-revision", [](Page &page) { ++page.request.source.fence.revision; }},
    {"policy-revision", [](Page &page) { ++page.request.source.fence.policy_revision; }},
    {"generation", [](Page &page) { page.request.source.fence.generation = "g4"; }},
    {"profile", [](Page &page) { page.request.source.profile = "history-v2"; }},
    {"operation", [](Page &page) { page.request.operation = "O2"; }},
    {"ordinal", [](Page &page) { ++page.request.ordinal; }},
    {"token-presence", [](Page &page) { page.request.continuation.reset(); }},
    {"frontier", [](Page &page) { page.request.frontier = "12"; }},
    {"endpoint", [](Page &page) { page.request.interval.start = Endpoint::inclusive; }},
    {"cut", [](Page &page) { page.request.upper.position = "15"; }},
    {"import-start", [](Page &page) { page.request.resolved_start_ms = 1795219200000; }},
    {"normalized", [](Page &page) { page.items.front().normalized = "normalized-echo"; }},
    {"key", [](Page &page) { page.items.front().key = "H00"; }},
    {"role", [](Page &page) { page.items.front().role = "echo"; }},
    {"namespace", [](Page &page) { page.items.front().identifier_namespace = "another"; }},
    {"identifier", [](Page &page) { page.items.front().identifier = "0"; }},
    {"receipt-facts", [](Page &page) { page.items.front().facts.actor_id = "another"; }},
    {"position", [](Page &page) { page.items.front().position = "9"; }},
    {"membership", [](Page &page) { page.items.front().membership_proof = "outside"; }},
    {"disjointness", [](Page &page) { page.items.front().disjointness_proof = "disjoint"; }},
    {"source-time",
     [](Page &page) { page.items.front().source_time = SourceTimeQuality::unreliable; }},
    {"evidence", [](Page &page) { page.items.front().evidence = core::PaymentEvidence::unknown; }},
    {"award-class", [](Page &page) { page.items.front().award_class = "other"; }},
    {"aggregate", [](Page &page) { page.items.front().aggregate_metric = "other"; }},
    {"order", [](Page &page) { page.items.swapItemsAt(0, 1); }},
    {"next", [](Page &page) { page.next = "V"; }},
    {"coverage-upper", [](Page &page) { page.coverage->upper = "14"; }},
    {"coverage-meaning", [](Page &page) { page.coverage->meaning = Coverage::lost; }},
    {"activity",
     [](Page &page) {
         page.items.front().activity =
             VerifiedActivityLink{{{"P", "A"}, "activity", "K12"}, "echo"};
     }},
    {"coverage-lower", [](Page &page) { page.coverage->lower = "10"; }},
    {"cut-evidence", [](Page &page) { page.request.upper.proof = "another-cut"; }},
    {"history-token", [](Page &page) { page.request.continuation = QByteArray("V"); }},
};
QByteArray reconnect_document(const Fence &before, const Fence &after, const QByteArray &frontier) {
    return QByteArray("RC1") + source_head(a, before, frontier) + source_head(a, after, frontier) +
           f(cursor) + f("actor/decision") + f("connected");
}
QByteArray prefix_witness(const AttemptLocator &first) {
    const auto blob = [](const QByteArray &value) { return integer(value.size()) + value; };
    const auto page_fields = blob("install") + blob("P") + blob("A") + blob("S") + blob("O") +
                             integer(2) + integer(2) + blob("g4") + blob("G") +
                             blob(intent_frame()) + integer(0) + integer(1) + integer(1) +
                             integer(2) + blob("T") + blob(audit_page);
    const auto prefix_fields = integer(1) + integer(1) + integer(2) + blob("g3") + integer(2) +
                               blob("H12") + blob(integer(first.receipt.value)) +
                               integer(first.ordinal) + integer(8000);
    const auto seal = [](const QByteArray &value) {
        return QCryptographicHash::hash(value, QCryptographicHash::Sha256);
    };
    return QByteArray("RP1\x01\x01", 5) + integer(2) + seal(page_fields) + seal(prefix_fields) +
           reconnect_document({2, 4, 2, "g3"}, {2, 5, 2, "g4"}, "11");
}
void reconnect_source(Store &db, const Fence &from, const QByteArray &generation,
                      const QByteArray &key) {
    db.work([&](const auto &work) -> QVariant {
        require(stage_reconnect(work, {a, from, generation, cursor, key, "actor/decision",
                                       "connected"}) == HistoryStatus::staged,
                "Stage reconnect at the exact source fence");
        return true;
    });
}

void history_row(Store &db, const QByteArray &key, const QByteArray &document) {
    const SqlRows expected{{4, document, k10, k10}};
    QVERIFY(
        db.rows("SELECT purpose,intent,prior_cut,selected_cut FROM main.subathon_intake_operations "
                "WHERE operation_ref=X'" +
                QString::fromLatin1(key.toHex()) + "'") == expected);
}

class HistoryTests : public QObject {
    Q_OBJECT
  private slots:
    void normalPageBindsBeforeItems_data() {
        QTest::addColumn<bool>("reconnect");
        QTest::newRow("normal") << false;
        QTest::newRow("r6-protected-readback") << true;
    }

    void normalPageBindsBeforeItems() {
        QFETCH(bool, reconnect);
        check_page_oracles();
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        const auto before = db.image();
        Attempt attempt(db, audit_page_echo());
        const auto qualified = attempt.qualified();
        QVERIFY(db.image() == before);
        QCOMPARE(attempt.bind(*qualified).status, IntakeStatus::committed);
        page_row(db, 1, audit_page, "T", 2, 0, 0);
        operation_row(db, 1, 1, "T", 2, 0);
        const auto bound = db.image();
        const auto premature = attempt.finish(*qualified);
        QCOMPARE(premature.status, IntakeStatus::unproved);
        QCOMPARE(premature.storage_status, std::optional{HistoryStatus::incomplete_prefix});
        QVERIFY(db.image() == bound);
        QVERIFY(std::ranges::all_of(attempt.slots_evidence, [](const auto &slot) {
            return slot.overlap == Overlap::unresolved && !slot.activity &&
                   !slot.canonical_identity;
        }));
        const auto first = admit_audit(db, audit_input(attempt.transcript.items[0], 1));
        page_row(db, 1, audit_page, "T", 2, 1, 0);
        auto second_input = audit_input(attempt.transcript.items[1], 2);
        std::shared_ptr<const QualifiedPage> resumed;
        std::optional<RetainedPageRequest> retained_request;
        if (reconnect) {
            const auto prefix = db.image();
            refused_receipt(db, second_input, ManagedStatus::known_rollback, [] { return false; });
            QVERIFY(db.image() == prefix);
            const auto replay = attempt.bind(*qualified);
            QCOMPARE(replay.storage_status, std::optional{HistoryStatus::exact_replay});
            QCOMPARE(replay.prefix, std::optional<qint64>{1});
            QVERIFY(db.image() == prefix);
            attempt.deny_inspect = true;
            const auto denied = attempt.bind(*qualified);
            QCOMPARE(denied.status, IntakeStatus::denied);
            QVERIFY(!denied.prefix && !denied.storage_status);
            attempt.deny_inspect = false;
            Page replacement = attempt.transcript;
            replacement.next = QByteArray("V");
            Attempt contradiction(db, attempt.transcript);
            QCOMPARE(contradiction.qualify(replacement).status, IntakeStatus::invalid_input);
            Attempt different(db, replacement);
            QCOMPARE(different.bind(*different.qualified()).status, IntakeStatus::conflict);
            QVERIFY(db.image() == prefix);
            db.reopen();
            RetainedPageRequest retained{attempt.binding, attempt.binding, audit_page, {}};
            const auto reconstruct = [&] {
                return reconstruct_history_page(db.writer(), attempt.owner, attempt.registered,
                                                retained);
            };
            QCOMPARE(reconstruct().status, IntakeStatus::unproved);
            attempt.retain(audit_page);
            const auto reopened = reconstruct();
            QCOMPARE(reopened.status, IntakeStatus::committed);
            QVERIFY(reopened.qualified);
            QCOMPARE(attempt.bind(*reopened.qualified).prefix, std::optional<qint64>{1});
            QCOMPARE(attempt.adapter.calls, quint64{1});
            QVERIFY(db.image() == prefix);
            reconnect_source(db, {2, 4, 2, "g3"}, "g4", "R6");
            page_row(db, 1, audit_page, "T", 2, 1, 0);
            second_input.expected_source = {2, 5, 2, "g4"};
            operation_row(db, 2, 1, "T", 2, 0);
            history_row(db, "R6", prefix_witness(first));
            const auto inherited = db.image();
            QCOMPARE(attempt.bind(*qualified).status, IntakeStatus::stale);
            retained.current.request.source.fence = second_input.expected_source;
            retained.current.operation_revision = 2;
            attempt.resume_registration(retained.current.request.source);
            retained.current.registry_revision = "registry-reconnect";
            retained.reconnect = "wrong-R6";
            const auto wrong = reconstruct();
            QCOMPARE(wrong.status, IntakeStatus::unproved);
            QCOMPARE(wrong.storage_status, std::optional{HistoryStatus::incomplete_prefix});
            refused_receipt(db, second_input, ManagedStatus::stale);
            QVERIFY(db.image() == inherited);
            second_input.page_reconnect = "R6";
            retained.reconnect = "R6";
            const auto current = reconstruct();
            QCOMPARE(current.status, IntakeStatus::committed);
            QVERIFY(current.qualified);
            resumed = current.qualified;
            retained_request = retained;
        }
        const auto second = admit_audit(db, second_input);
        QVERIFY(first.receipt != second.receipt);
        QCOMPARE(first.ordinal, 1);
        QCOMPARE(second.ordinal, 1);
        page_row(db, 1, audit_page, "T", 2, 2, 0);
        if (reconnect) {
            QCOMPARE(attempt.finish(*resumed).status, IntakeStatus::committed);
            db.reopen();
            const auto terminal = db.image();
            const auto prior_writes = db.changes();
            auto wrong = *retained_request;
            wrong.current.operation_revision = 1;
            const auto replay = [&](const RetainedPageRequest &request) {
                const auto selection = reconstruct_history_page(db.writer(), attempt.owner,
                                                                attempt.registered, request);
                if (!selection.qualified)
                    return PageOutcome{selection.status, selection.storage_status, {}};
                return attempt.finish(*selection.qualified);
            };
            const auto rejected = replay(wrong);
            QCOMPARE(rejected.status, IntakeStatus::unproved);
            QCOMPARE(rejected.storage_status, std::optional{HistoryStatus::incomplete_prefix});
            QVERIFY(db.image() == terminal);
            const auto exact = replay(*retained_request);
            QCOMPARE(exact.status, IntakeStatus::exact_replay);
            QCOMPARE(exact.storage_status, std::optional{HistoryStatus::exact_replay});
            QVERIFY(db.image() == terminal);
            QCOMPARE(db.changes(), prior_writes);
        } else {
            QCOMPARE(attempt.finish(*qualified).status, IntakeStatus::committed);
        }
        page_row(db, 1, audit_page, "T", 2, 2, 1, covered_11_13);
        operation_row(db, reconnect ? 3 : 2, 2, "U", 2, 0);
        const SqlRows sources{
            {2, reconnect ? 6 : 5, 2, QByteArray(reconnect ? "g4" : "g3"), QByteArray("13")}};

        const SqlRows entries{{1, 2, 3, integer(first.receipt.value), 1},
                              {2, 2, 3, integer(second.receipt.value), 1}};
        QVERIFY(db.rows("SELECT epoch,revision,policy_revision,generation,frontier "
                        "FROM main.subathon_intake_sources") == sources);
        coverage_rows(db, {{2, 0}}, {{2, 1, 0, covered_11_13}});
        QVERIFY(db.rows("SELECT item_ordinal,phase,disposition,receipt_id,attempt_ordinal "
                        "FROM main.subathon_intake_entries WHERE contribution_id IS NULL "
                        "ORDER BY item_ordinal") == entries);
        if (!reconnect)
            lossAfterReconnect(db, attempt, *qualified);
    }

  private:
    void lossAfterReconnect(Store &db, Attempt &initial, const QualifiedPage &old) {
        const auto before = db.image();
        reconnect_source(db, {2, 5, 2, "g3"}, "g4", "R3");
        const auto connected = db.image();
        history_row(db, "R3", reconnect_document({2, 5, 2, "g3"}, {2, 6, 2, "g4"}, "13"));
        QVERIFY(connected.value(gaps_table) == before.value(gaps_table));
        QVERIFY(connected.value(pages_table) == before.value(pages_table));
        QVERIFY(connected.value(segments_table) == before.value(segments_table));
        QCOMPARE(initial.finish(old).status, IntakeStatus::stale);
        operation_row(db, 3, 2, "U", 2, 0);
        auto request = catch_up(2, "U");
        request.source.fence = {2, 6, 2, "g4"};
        request.frontier = QByteArray("13");
        const Page loss{
            request, {}, {}, RangeProof{Coverage::lost, "13", "14", "loss-13-14", "retention"}};
        for (const auto &[failure, status] :
             {std::pair{HistoryFailure::unsupported, IntakeStatus::unsupported},
              std::pair{HistoryFailure::temporarily_unavailable, IntakeStatus::unavailable}}) {
            Attempt rejected(db, loss, 3);
            rejected.binding.gap_revision = 2;
            rejected.adapter.response = failure;
            QCOMPARE(rejected.qualify().status, status);
            QVERIFY(db.image() == connected);
        }
        Attempt next(db, loss, 3);
        next.binding.gap_revision = 2;
        const auto qualified = next.qualified();
        const auto cv = QByteArray("CV1\x01\x02\x01", 6) + f("key-v1") + f("S/history") + f("13") +
                        f("14") + f("retention") + f("loss-13-14");
        const auto p = page_frame({}, {}, cv);
        require_frame(p, 81, "998b4fac45546bf452adff85f162890d9c9b09051159c996d770715ed75aeaac");
        QCOMPARE(next.bind(*qualified).status, IntakeStatus::committed);
        page_row(db, 2, p, "U", 0, 0, 0);
        QCOMPARE(next.finish(*qualified).status, IntakeStatus::committed);
        page_row(db, 2, p, "U", 0, 0, 1, cv);
        operation_row(db, 4, 3, {}, 2, 1);
        coverage_rows(db, {{3, 1}}, {{2, 1, 0, covered_11_13}, {3, 1, 1, cv}});
        QVERIFY(db.image().value(sources_table) == connected.value(sources_table));
        page_row(db, 1, audit_page, "T", 2, 2, 1, covered_11_13);
    }

  private slots:
    void gapTimeAndUpperWitness() {
        for (const auto &[trusted, echo, expected] :
             {std::tuple<std::optional<qint64>, std::optional<qint64>, IntakeStatus>{
                  8000, 8000, IntakeStatus::committed},
              {8000, {}, IntakeStatus::committed},
              {{}, {}, IntakeStatus::committed},
              {8000, 8001, IntakeStatus::invalid_input},
              {{}, 8000, IntakeStatus::invalid_input}}) {
            HistoryFixture fixture;
            auto &db = fixture.startup.store;
            const auto before = db.image();
            Attempt attempt(db, audit_page_echo());
            attempt.registry.install(attempt.adapter, {a, progressed, profile},
                                     "registry-gap-open");
            attempt.registered.snapshot = attempt.registry.current;
            const auto gap = gap_opening();
            const QByteArray document =
                trusted ? f("gap-basis") + '\1' + integer(*trusted) : QByteArray("gap-basis");
            if (trusted)
                require_frame(document, 22,
                              "09faf8b24c97b26026c85116cf5c5e9b6d81b11695aab14631718fc1c8302cd2");
            QCOMPARE(
                open_history_gap(db.writer(), attempt.owner, attempt.registered, gap, {"G", echo},
                                 [&](const GapInvocation &invoked) -> std::optional<GapEvidence> {
                                     const auto &got = invoked.binding;
                                     if (&invoked.profile != attempt.registered.snapshot.get() ||
                                         invoked.identity != 1 || got.scope != a ||
                                         got.expected != progressed || got.gap_ref != "G" ||
                                         got.new_generation != "g3" || got.frontier != "11" ||
                                         got.upper_cut != "14" || got.cursor_contract != cursor ||
                                         got.cut_request != "gap-upper:14")
                                         return {};
                                     return GapEvidence{document, trusted};
                                 }),
                expected);
            if (expected != IntakeStatus::committed) {
                QVERIFY(db.image() == before);
                continue;
            }
            QCOMPARE(db.rows("SELECT revision,status,start_frontier,upper_cut,document "
                             "FROM main.subathon_intake_gaps"),
                     (SqlRows{{1, 0, QByteArray("11"), QByteArray("14"), document}}));
            const auto request = catch_up();
            attempt.resume_registration(request.source);
            const CutRequest selected{request.source, CutPurpose::gap_upper, request.frontier};
            const CutObservation observed{selected, request.upper, Endpoint::inclusive};
            attempt.adapter.expected_cut = selected;
            attempt.adapter.cut_response = observed;
            QCOMPARE(begin_history_catch_up(db.writer(), attempt.owner, attempt.registered,
                                            {selected, catch_up_intent(), cursor, progressed},
                                            observed,
                                            [&](quint64 id, const CutObservation &actual) {
                                                return id == 1 && attempt.adapter.cut_calls == 1 &&
                                                       exact_cut(actual, observed);
                                            }),
                     IntakeStatus::committed);
            operation_row(db, 1, 1, "T", 0, 0);
            QCOMPARE(attempt.adapter.cut_calls, 1u);
            QVERIFY(db.image().value(pages_table).isEmpty());
            QVERIFY(db.image().value(segments_table).isEmpty());
        }
    }

    void importUsesExactStartupPredecessor_data() {
        QTest::addColumn<bool>("checkpoint");
        QTest::newRow("B0-null-frontier") << false;
        QTest::newRow("K0-frontier-11") << true;
    }

    void importUsesExactStartupPredecessor() {
        QFETCH(bool, checkpoint);
        HistoryFixture fixture(checkpoint);
        auto &db = fixture.startup.store;
        const auto before = db.image();
        const auto head = checkpoint ? progressed : active;
        const auto intent = import_intent(head, "11", "import11");
        const HistoryRequest request{{a, head, profile},
                                     "I",
                                     1,
                                     QByteArray("T"),
                                     checkpoint ? std::optional{QByteArray("11")} : std::nullopt,
                                     intent.interval,
                                     intent.upper_cut,
                                     intent.resolved_utc_start};
        auto member = audit_item("9", "9", "norm-A-9");
        member.source_time = SourceTimeQuality::reliable;
        member.disjointness_proof = "outside-K10-live-lane";
        auto outside = audit_item("11", "11", "norm-A-11");
        outside.source_time = SourceTimeQuality::reliable;
        const auto untimed = audit_item("unknown", {}, "norm-A-untimed");
        Attempt members(db, {request, {member, outside, untimed}, {}, {}});
        members.binding = {request, HistoryPurpose::import, cursor, import_bytes(intent), {}, 1,
                           0,       "registry-history"};
        members.slots_evidence[0].overlap = Overlap::disjoint;
        members.slots_evidence[0].disjointness = "outside-K10-live-lane";
        members.slots_evidence[1].membership = Membership::outside;
        members.slots_evidence[2].membership = Membership::unknown;
        Attempt wrong_date(db, {request, {}, {}, {}});
        QCOMPARE(open_import(wrong_date, intent, request, false, 1795215600000LL),
                 IntakeStatus::invalid_input);
        QVERIFY(db.image() == before);
        QCOMPARE(open_import(members, intent, request), IntakeStatus::committed);
        const auto opened = db.image();
        QVERIFY(members.qualified());
        QVERIFY(db.image() == opened);
        auto next_cut = import_intent(head, "12", "cut12");
        auto next_request = request;
        next_request.upper = next_cut.upper_cut;
        Attempt malformed(db, {next_request, {}, {}, {}});
        QCOMPARE(open_import(malformed, next_cut, next_request, true), IntakeStatus::invalid_input);
        Attempt retained_date(db, {request, {}, {}, {}});
        retained_date.owner.zone = [](const QByteArray &, const QByteArray &, QDate) {
            return std::optional{ZoneBasis{3600, {}}};
        };
        QCOMPARE(open_import(retained_date, intent, request, false, 1795215600000LL),
                 IntakeStatus::invalid_input);
        QVERIFY(db.image() == opened);
        HistoryFixture independent(checkpoint);
        Attempt valid_cut12(independent.startup.store, {next_request, {}, {}, {}});
        QCOMPARE(open_import(valid_cut12, next_cut, next_request), IntakeStatus::committed);
        Attempt conflict(db, {next_request, {}, {}, {}});
        QCOMPARE(open_import(conflict, next_cut, next_request), IntakeStatus::conflict);
        QVERIFY(db.image() == opened);
        const auto evidence = QByteArray("import-empty-through-11");
        const RangeProof range{Coverage::contiguous, "start", "11", evidence, {}};
        Attempt empty(db, {request, {}, {}, range});
        empty.binding = members.binding;
        const auto qualified = empty.qualified();
        const auto p = page_frame({}, {}, evidence);
        require_frame(p, 36, "8c20a8f5ce2a31c85723a2e671e799498bc11bdedb89dc4a99aa54e82735e165");
        QCOMPARE(empty.bind(*qualified).status, IntakeStatus::committed);
        page_row(db, 1, p, "T", 0, 0, 0);
        QCOMPARE(empty.finish(*qualified).status, IntakeStatus::committed);
        page_row(db, 1, p, "T", 0, 0, 1);
        auto expected = before;
        const auto after = db.image();
        expected["subathon_intake_operations"] = after.value("subathon_intake_operations");
        expected[pages_table] = after.value(pages_table);
        QVERIFY(after == expected);
        auto startup_rows = db.rows("SELECT * FROM main.subathon_intake_operations "
                                    "WHERE operation_ref<>X'49' ORDER BY operation_ref");
        canonical_cells(startup_rows);
        QCOMPARE(startup_rows, before.value("subathon_intake_operations"));
        auto rows = db.rows("SELECT intent,revision,next_ordinal,continuation,slots,outcome "
                            "FROM main.subathon_intake_operations WHERE operation_ref=X'49'");
        canonical_cells(rows);
        const SqlRows settled{{import_bytes(intent), 2, 2, QVariant{}, 0, 1}};
        QVERIFY(rows == settled);
    }

    void importIntentLimits() {
        for (int proof_size : {16175, 16176, 16340}) {
            HistoryFixture fixture;
            auto &db = fixture.startup.store;
            reconnect_source(db, progressed, "g3", "R7");
            const auto before = db.image();
            auto intent = import_intent({2, 4, 2, "g3"}, "Z", QByteArray(proof_size, 'e'));
            intent.operation_ref = "O";
            const HistoryRequest request{{a, {2, 4, 2, "g3"}, profile},
                                         "O",
                                         1,
                                         QByteArray("T"),
                                         QByteArray("11"),
                                         intent.interval,
                                         intent.upper_cut,
                                         intent.resolved_utc_start};
            Attempt attempt(db, {request, {}, {}, {}});
            const auto bytes = import_bytes(intent);
            QCOMPARE(bytes.size(), 209 + proof_size);
            QCOMPARE(cut_frame(Endpoint::exclusive, intent.upper_cut).size(), 44 + proof_size);
            QCOMPARE(open_import(attempt, intent, request),
                     proof_size == 16175 ? IntakeStatus::committed : IntakeStatus::limit_exceeded);
            if (proof_size == 16175) {
                require_frame(bytes, 16384,
                              "702e9a94cd6e60bbb00482bd25694bc8d081bf964e3fb61404132c0e2406f476");
                const SqlRows wanted{{bytes, 1, 1, QByteArray("T"), 0, 0}};
                QVERIFY(db.rows("SELECT intent,revision,next_ordinal,continuation,slots,outcome "
                                "FROM main.subathon_intake_operations WHERE operation_ref=X'4F'") ==
                        wanted);
                auto startup_rows = db.rows("SELECT * FROM main.subathon_intake_operations "
                                            "WHERE operation_ref<>X'4F' ORDER BY operation_ref");
                canonical_cells(startup_rows);
                QCOMPARE(startup_rows, before.value("subathon_intake_operations"));
                QVERIFY(db.image().value("subathon_intake_sources") ==
                        before.value("subathon_intake_sources"));
            } else {
                QVERIFY(db.image() == before);
            }
        }
    }

    void verifiedActivityQualification_data() {
        QTest::addColumn<bool>("admit_distinct");
        QTest::newRow("E1-qualification-only") << false;
        QTest::newRow("E6-shared-activity-distinct-receipts") << true;
    }

    void verifiedActivityQualification() {
        QFETCH(bool, admit_distinct);
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        auto observation = audit_page_echo();
        for (auto &item : observation.items)
            item.activity =
                VerifiedActivityLink{{{"P", "A"}, "activity", "K" + item.identifier->toStdString()},
                                     "verified-live/history"};
        auto live_item = observation.items[0];
        live_item.key = "L12";
        live_item.identifier = "live-receipt-12";
        live_item.normalized = "norm-A-12-live-v1";
        auto live = audit_input(live_item, 1);
        live.history.reset();
        live.receipt.arrival.connection_ref = "live";
        live.activity = live_item.activity;
        std::optional<AttemptLocator> live_attempt;
        if (admit_distinct)
            live_attempt = admit_audit(db, live);
        const auto before = db.image();
        Attempt attempt(db, observation);
        for (qsizetype slot = 0; slot < observation.items.size(); ++slot) {
            attempt.slots_evidence[slot].overlap = Overlap::verified_activity;
            attempt.slots_evidence[slot].activity = observation.items[slot].activity;
        }
        const auto qualified = attempt.qualified();
        QVERIFY(db.image() == before);
        if (live_attempt) {
            QCOMPARE(attempt.bind(*qualified).status, IntakeStatus::committed);
            auto history = audit_input(observation.items[0], 1);
            history.activity = attempt.slots_evidence[0].activity;
            QCOMPARE(history.activity, live.activity);
            QVERIFY(history.receipt.identity != live.receipt.identity);
            QVERIFY(admit_audit(db, history).receipt != live_attempt->receipt);
        }
    }

    void handoffAndLegacyEnvelope_data() {
        QTest::addColumn<QByteArray>("variant");
        for (const auto *name : {"commit", "one-cut", "through-11", "before-fence", "swapped",
                                 "contradictory", "different-attempt", "stale", "denied-omitted",
                                 "veto", "legacy-missing", "legacy-commit"})
            QTest::newRow(name) << QByteArray(name);
    }

    void handoffAndLegacyEnvelope() {
        QFETCH(QByteArray, variant);
        const bool legacy = variant.startsWith("legacy-");
        Fixture fixture(!legacy);
        require_startup(fixture.submit(fixture.register_request()), "Real pending registration");
        if (!legacy) {
            require_startup(fixture.submit(fixture.activate_request()),
                            "Real two-source activation");
            require_startup(fixture.submit(fixture.checkpoint_request()), "Checkpoint only A");
        }
        auto &db = fixture.store;
        const auto before = db.image();
        const auto purpose = legacy ? CutPurpose::legacy_adoption : CutPurpose::new_only_handoff;
        const auto frontier = legacy ? std::nullopt : std::optional{QByteArray("11")};
        const Fence prior = legacy ? pending : progressed;
        const Fence next = legacy ? Fence{2, 2, 2, "gLegacy"} : Fence{3, 4, 3, "gA2"};
        const Fence next_b{3, 3, 3, "gB2"};
        const QByteArray proof = legacy ? "legacy-envelope-14" : "handoff-A14-B15";
        const QByteArray selected_a = cut(next.generation, legacy ? "legacy14" : "fenceA14", "14");
        const QByteArray selected_b = cut("gB2", "fenceB14", "14");
        const QByteArray decision("actor/decision");
        const auto policy_doc =
            legacy ? activation().policy : f("review") + f("cfg-2") + f(decision);
        auto new_a = member(a);
        auto new_b = member(b);
        if (!legacy) {
            new_a.primary = false;
            new_a.document = member(b).document;
            new_b.primary = true;
            new_b.document = member(a).document;
        }
        SourceChange change{a,
                            prior,
                            next.generation,
                            policy_doc,
                            {new_a},
                            selected_a,
                            legacy ? "L9" : "HA",
                            decision,
                            cursor,
                            proof,
                            legacy ? SourcePurpose::adoption : SourcePurpose::handoff,
                            legacy ? 1 : 2};
        if (!legacy) {
            change.members.append(new_b);
            change.affected_sources.append({b, active_b, "gB2", selected_b, "HB", decision, cursor,
                                            proof, SourcePurpose::handoff});
        }
        if (variant == "stale")
            --change.expected_installation_policy_revision;
        const CutObservation side_a{{{a, next, profile}, purpose, frontier},
                                    {"key-v1", "S/history", QByteArray("14"), next.generation,
                                     legacy ? "legacy14" : "fenceA14"},
                                    Endpoint::inclusive};
        const CutObservation side_b{{{b, next_b, profile}, purpose, {}},
                                    {"key-v1", "S/history", QByteArray("14"), "gB2", "fenceB14"},
                                    Endpoint::inclusive};
        Adapter adapter_a(audit_page_echo()), adapter_b(audit_page_echo());
        adapter_a.expected_cut = side_a.request;
        adapter_a.cut_response = variant == "swapped" ? side_b : side_a;
        adapter_b.expected_cut = side_b.request;
        adapter_b.cut_response = variant == "one-cut"
                                     ? CutResult{HistoryFailure::unproved}
                                     : CutResult{variant == "swapped" ? side_a : side_b};
        HistoryRegistry registry_a, registry_b;
        registry_a.install(adapter_a, side_a.request.source);
        registry_b.install(adapter_b, side_b.request.source);
        HistoryProfile profile_a{registry_a, registry_a.current, {}};
        HistoryProfile profile_b{registry_b, registry_b.current, {}};
        QList<const HistoryProfile *> profiles{&profile_a};
        AdmittedHandoff admitted{change, {side_a.request}, {legacy ? QByteArray{} : k10}};
        QList<CutObservation> echoes{side_a};
        if (!legacy) {
            profiles.append(&profile_b);
            admitted.selected.append(side_b.request);
            admitted.prior_cuts.append(kb10);
            if (variant != "denied-omitted")
                echoes.append(side_b);
        }
        if (variant == "contradictory")
            echoes[0].cut.position = QByteArray("11");
        QSet<QByteArray> authorized;
        OwnerInputs owner{[&](Action, const Scope &scope) {
                              authorized.insert(scope.account);
                              return variant == "denied-omitted" && scope == b
                                         ? Authorization::denied
                                         : Authorization::allowed;
                          },
                          adapter_a,
                          {},
                          {}};
        const QByteArray eligible_upper = variant == "through-11" ? "11" : "14";
        auto through = prior;
        if (variant == "before-fence")
            --through.revision;
        const std::optional<QByteArray> old_upper =
            variant == "legacy-missing" ? std::nullopt : std::optional{eligible_upper};
        const quint64 new_attempt = variant == "different-attempt" ? 2 : 1;
        const auto matches = [](const ObservedCut &got, const Adapter &adapter,
                                const CutObservation &want) {
            return got.adapter == &adapter && got.invocation == 1 && adapter.cut_calls == 1 &&
                   exact_cut(got.observation, want);
        };
        const HandoffValidators validators{
            [&](const ObservedCut &got) -> std::optional<OldEnvelope> {
                if (!old_upper || !matches(got, adapter_a, side_a))
                    return {};
                return OldEnvelope{
                    got,  *old_upper, through,
                    true, 1,          legacy ? std::optional{QByteArray("15")} : std::nullopt};
            },
            [&](const ObservedCut &got) -> std::optional<NewSide> {
                if (!matches(got, adapter_b, side_b))
                    return {};
                return NewSide{got, "14", "15", new_attempt};
            },
            [&](const OldEnvelope &old,
                const std::optional<NewSide> &incoming) -> std::optional<QByteArray> {
                if (!matches(old.source, adapter_a, side_a) || old.eligible_upper != "14" ||
                    old.through != prior || !old.includes_pending_and_outstanding)
                    return {};
                if (legacy)
                    return !incoming && old.legacy_eligible_first == QByteArray("15")
                               ? std::optional{proof}
                               : std::nullopt;
                if (!incoming || !matches(incoming->source, adapter_b, side_b) ||
                    incoming->attempt != old.attempt ||
                    incoming->exclusive_lower != old.eligible_upper || incoming->first != "15")
                    return {};
                return proof;
            }};
        bool vetoed = false;
        const auto outcome =
            commit_history_handoff(db.writer(), owner, profiles, admitted, echoes, validators, [&] {
                vetoed = variant == "veto";
                return !vetoed;
            });
        const auto status = outcome.status;
        const bool committed = variant == "commit" || variant == "legacy-commit";
        if (!committed) {
            if (variant == "veto") {
                QVERIFY(vetoed);
                QVERIFY(outcome.work);
                QCOMPARE(outcome.work->transaction, TransactionOutcome::rolled_back);
                QCOMPARE(outcome.work->error, WorkError::not_committed);
                QVERIFY(status != IntakeStatus::committed);
            } else {
                QCOMPARE(status, variant == "denied-omitted" ? IntakeStatus::denied
                                 : variant == "stale"        ? IntakeStatus::stale
                                 : variant == "swapped" || variant == "contradictory"
                                     ? IntakeStatus::invalid_input
                                     : IntakeStatus::unproved);
            }
            if (variant == "denied-omitted") {
                QVERIFY(authorized.contains("B"));
                QVERIFY(!adapter_a.cut_calls && !adapter_b.cut_calls);
            }
            QVERIFY(db.image() == before);
            return;
        }
        QCOMPARE(status, IntakeStatus::committed);
        QVERIFY(outcome.work);
        catalog::finance::normal(*outcome.work);
        QCOMPARE(adapter_a.cut_calls, quint64{1});
        QCOMPARE(adapter_b.cut_calls, quint64(!legacy));
        QVERIFY(authorized.contains("A") && (legacy || authorized.contains("B")));
        const auto sh = [&](const Scope &scope, const Fence &from, const Fence &to,
                            const std::optional<QByteArray> &front) {
            return QByteArray("SH1") + source_head(scope, from, front) + source_head(scope, to) +
                   f(cursor) + f(decision) + f(proof);
        };
        SqlRows history{{legacy ? 3 : 4, sh(a, prior, next, frontier),
                         legacy ? QVariant{} : QVariant{k10}, selected_a}};
        if (!legacy)
            history.append({4, sh(b, active_b, next_b, {}), kb10, selected_b});
        auto actual = db.rows("SELECT purpose,intent,prior_cut,selected_cut FROM main."
                              "subathon_intake_operations WHERE operation_ref IN "
                              "(X'4841',X'4842',X'4C39') ORDER BY account");
        canonical_cells(actual);
        QVERIFY(actual == history);
        SqlRows heads{{next.epoch, next.revision, next.policy_revision, next.generation, QVariant{},
                       cursor, selected_a, 1}};
        if (!legacy)
            heads.append({3, 3, 3, QByteArray("gB2"), QVariant{}, cursor, selected_b, 1});
        auto selected =
            db.rows("SELECT epoch,revision,policy_revision,generation,frontier,document,"
                    "cut,baseline_state FROM main.subathon_intake_sources ORDER BY account");
        canonical_cells(selected);
        QVERIFY(selected == heads);
        const SqlRows policy_rows{{next.policy_revision, policy_doc}};
        QVERIFY(db.rows("SELECT revision,document FROM main.subathon_intake_policies "
                        "ORDER BY revision DESC LIMIT 1") == policy_rows);
        auto startup_rows = db.rows("SELECT * FROM main.subathon_intake_operations "
                                    "WHERE operation_ref IN (X'4230',X'4B30',X'424230') "
                                    "ORDER BY operation_ref");
        canonical_cells(startup_rows);
        QCOMPARE(startup_rows, before.value("subathon_intake_operations"));
        QVERIFY(db.rows("SELECT * FROM main.subathon_intake_entries").isEmpty());
    }

    void quarantineCapacityBeforeBinding() {
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        const auto entry = [](int n) {
            const auto key = "Q" + QByteArray::number(n).rightJustified(3, '0');
            return QuarantineEntry{a, key, key, "unidentified", 8000, {2, 4, 2, "g3"}};
        };
        for (int n = 1; n <= 256; ++n)
            db.work([&](const auto &work) -> QVariant {
                require(stage_quarantine(work, entry(n)) == QuarantineStatus::staged,
                        "Live unidentified item uses the actual quota owner");
                return true;
            });
        const auto full = db.image();
        auto item = audit_item("257", "12", "Q257");
        item.key = "Q257";
        item.identifier.reset();
        const auto p = page_frame({"Q257"}, QByteArray("U"), {});
        auto slot = entry(257);
        slot.history = QuarantinePageSlot{{"O", 1, 1}, 1, 1, "T", p};
        const Page observation{catch_up(), {item}, QByteArray("U"), {}};
        Attempt blocked(db, observation);
        blocked.quarantine = {slot};
        QCOMPARE(blocked.bind(*blocked.qualified()).status, IntakeStatus::quarantine_full);
        QVERIFY(db.image() == full);
        db.work([&](const auto &work) -> QVariant {
            require(stage_quarantine_resolution(work, entry(1), "actor/decision") ==
                        QuarantineStatus::staged,
                    "Resolving exact Q001 releases one active slot");
            return true;
        });
        const SqlRows resolved{{3, 2, QVariant{}}};
        QVERIFY(
            db.rows("SELECT phase,disposition,quarantine_bytes FROM main.subathon_intake_entries "
                    "WHERE intake_ref=X'51303031'") == resolved);
        QCOMPARE(db.rows("SELECT count(*) FROM main.subathon_intake_entries "
                         "WHERE quarantine_bytes IS NOT NULL"),
                 (SqlRows{{255}}));
        Attempt retry(db, observation);
        retry.quarantine = {slot};
        const auto qualified = retry.qualified();
        QCOMPARE(retry.bind(*qualified).status, IntakeStatus::committed);
        page_row(db, 1, p, "T", 1, 0, 0);
        operation_row(db, 1, 1, "T", 1, 0);
        db.work([&](const auto &work) -> QVariant {
            require(stage_quarantine(work, slot) == QuarantineStatus::staged,
                    "The identical slot can now use the freed capacity");
            return true;
        });
        page_row(db, 1, p, "T", 1, 1, 0);
        const SqlRows charges{{256, 6400, 0}};
        QVERIFY(db.rows("SELECT count(*),sum(quarantine_bytes),count(receipt_id) "
                        "FROM main.subathon_intake_entries WHERE quarantine_bytes IS NOT NULL") ==
                charges);
        QCOMPARE(retry.finish(*qualified).status, IntakeStatus::committed);
        page_row(db, 1, p, "T", 1, 1, 1);
        operation_row(db, 2, 2, "U", 1, 0);
        QVERIFY(db.image().value(sources_table) == full.value(sources_table));
        QVERIFY(db.image().value(gaps_table) == full.value(gaps_table));
    }

    void contradictoryEchoes_data() {
        QTest::addColumn<int>("field");
        for (int field = 0; field < int(std::size(page_echoes)); ++field)
            QTest::newRow(page_echoes[field].label) << field;
    }

    void contradictoryEchoes() {
        QFETCH(int, field);
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        const auto before = db.image();
        Attempt attempt(db, audit_page_echo());
        auto candidate = attempt.transcript;
        page_echoes[field].mutate(candidate);
        const auto result = attempt.qualify(candidate);
        QCOMPARE(result.status, IntakeStatus::invalid_input);
        QVERIFY(!result.qualified);
        QVERIFY(db.image() == before);
    }

    void authenticUnboundPageStalesOnReconnect_data() {
        QTest::addColumn<bool>("replace_registry");
        QTest::newRow("reconnect") << false;
        QTest::newRow("registry-replacement") << true;
    }

    void authenticUnboundPageStalesOnReconnect() {
        QFETCH(bool, replace_registry);
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        Attempt attempt(db, audit_page_echo());
        const auto handle = attempt.qualified();
        if (replace_registry)
            attempt.registry.install(attempt.adapter, attempt.binding.request.source,
                                     "registry-new");
        else
            reconnect_source(db, {2, 4, 2, "g3"}, "g4", "R3");
        const auto after = db.image();
        QCOMPARE(attempt.bind(*handle).status, IntakeStatus::stale);
        QVERIFY(after.value(pages_table).isEmpty());
        QCOMPARE(attempt.qualify().status, IntakeStatus::stale);
        QCOMPARE(attempt.adapter.calls, 1u);
        operation_row(db, replace_registry ? 1 : 2, 1, "T", 0, 0);
        QVERIFY(db.image() == after);
    }

    void authorityWinsBeforeObservationAndAtBind() {
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        const auto before = db.image();
        Attempt denied(db, audit_page_echo());
        denied.deny_page = true;
        auto candidate = denied.transcript;
        candidate.request.ordinal = 0;
        QCOMPARE(denied.qualify(candidate).status, IntakeStatus::denied);
        QCOMPARE(denied.adapter.calls, 0u);
        Attempt revoked(db, audit_page_echo());
        const auto qualified = revoked.qualified();
        revoked.deny_page = true;
        QCOMPARE(revoked.bind(*qualified).status, IntakeStatus::denied);
        QVERIFY(db.image() == before);
    }

    void bindRespectsPageAndProofLimits_data() {
        QTest::addColumn<int>("shape");
        QTest::addColumn<bool>("over");
        QTest::newRow("page-equality") << 0 << false;
        QTest::newRow("page-one-over") << 0 << true;
        QTest::newRow("proof-equality") << 1 << false;
        QTest::newRow("proof-one-over") << 1 << true;
        QTest::newRow("sixteen-slots") << 2 << false;
        QTest::newRow("seventeen-slots") << 2 << true;
    }

    void bindRespectsPageAndProofLimits() {
        QFETCH(int, shape);
        QFETCH(bool, over);
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        const auto before = db.image();
        const auto count = shape == 0 ? 8 : shape == 1 ? 0 : 16 + over;
        QList<Item> items;
        QList<QByteArray> documents;
        for (int slot = 1; slot <= count; ++slot) {
            const auto id = QByteArray::number(slot);
            const auto size = shape == 0 ? (slot <= 7 ? 32768 : 16338 + over) : 32;
            const auto doc = id + QByteArray(size - id.size(), 'x');
            documents.append(doc);
            items.append(audit_item(id, "12", doc));
        }
        std::optional<RangeProof> range;
        QByteArray proof;
        if (shape != 2) {
            const QByteArray evidence(16384 - 49 + (shape == 1 && over), 'e');
            range = RangeProof{Coverage::contiguous, "11", "14", evidence, {}};
            proof = range_frame("14", evidence);
            QCOMPARE(proof.size(), 16384 + (shape == 1 && over));
        }
        const auto frame = page_frame(documents, QByteArray("U"), proof);
        if (shape == 0)
            QCOMPARE(frame.size(), 262144 + over);
        Attempt attempt(db, {catch_up(), items, QByteArray("U"), range});
        const auto result = attempt.qualify();
        if (over && !result.qualified) {
            QCOMPARE(result.status, IntakeStatus::limit_exceeded);
            QVERIFY(db.image() == before);
            return;
        }
        QCOMPARE(result.status, IntakeStatus::committed);
        QVERIFY(result.qualified);
        const auto bound = attempt.bind(*result.qualified);
        if (over) {
            QCOMPARE(bound.status, IntakeStatus::limit_exceeded);
            QCOMPARE(bound.storage_status, std::optional{HistoryStatus::intake_limit});
            QVERIFY(db.image() == before);
            return;
        }
        QCOMPARE(bound.status, IntakeStatus::committed);
        page_row(db, 1, frame, "T", count, 0, 0);
        operation_row(db, 1, 1, "T", count, 0);
    }

    void pageCapAndRollover() {
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        QByteArray token("T");
        const auto first_source = db.rows("SELECT * FROM main.subathon_intake_sources");
        const auto first_gap = db.rows("SELECT * FROM main.subathon_intake_gaps");
        for (qint64 ordinal = 1; ordinal <= 256; ++ordinal) {
            const auto next = "U" + QByteArray::number(ordinal);
            if (ordinal == 256)
                operation_row(db, 256, 256, "U255", 0, 0);
            Attempt attempt(db, {catch_up(ordinal, token), {}, next, {}}, ordinal);
            const auto qualified = attempt.qualified();
            QCOMPARE(attempt.bind(*qualified).status, IntakeStatus::committed);
            QCOMPARE(attempt.finish(*qualified).status, IntakeStatus::committed);
            page_row(db, ordinal, page_frame({}, next, {}), token, 0, 0, 1);
            token = next;
        }
        operation_row(db, 257, 257, "U256", 0, 0);
        const auto before = db.image();
        Attempt over(db, {catch_up(257, token), {}, QByteArray("U257"), {}}, 257);
        const auto result = over.qualify();
        const auto status = result.qualified ? over.bind(*result.qualified).status : result.status;
        QCOMPARE(status, IntakeStatus::limit_exceeded);
        QVERIFY(db.image() == before);
        const auto pages = db.rows("SELECT * FROM main.subathon_intake_pages ORDER BY ordinal");
        db.work([](const auto &work) -> QVariant {
            const Rollover rollover{a,   "O", "O2", "U256", "11", intent_frame(), "actor/decision",
                                    257, 4};
            require(stage_rollover(work, rollover) == HistoryStatus::staged,
                    "Authorized rollover preserves the full completed prefix");
            return true;
        });
        QVERIFY(db.rows("SELECT * FROM main.subathon_intake_pages ORDER BY ordinal") == pages);
        QVERIFY(db.rows("SELECT * FROM main.subathon_intake_sources") == first_source);
        QVERIFY(db.rows("SELECT * FROM main.subathon_intake_gaps") == first_gap);
        const SqlRows old{{3, 0, QByteArray("U256")}};
        QVERIFY(db.rows("SELECT outcome,slots,continuation FROM main.subathon_intake_operations "
                        "WHERE operation_ref=X'4F'") == old);
        const SqlRows replacement{
            {1, 1, QByteArray("g3"), intent_frame(), QByteArray("U256"), 0, 0}};
        QVERIFY(db.rows("SELECT revision,next_ordinal,generation,intent,continuation,slots,outcome "
                        "FROM main.subathon_intake_operations WHERE operation_ref=X'4F32'") ==
                replacement);
    }

    void unprovedOrderAndInventedUnknownAdvancement() {
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        const auto before = db.image();
        auto tied = audit_page_echo();
        tied.items[1].position = tied.items[0].position;
        Attempt unordered(db, tied);
        unordered.order_proved = false;
        QCOMPARE(unordered.qualify().status, IntakeStatus::unproved);
        Attempt unknown(db, {catch_up(), {}, {}, {}});
        auto candidate = unknown.transcript;
        candidate.coverage = tied.coverage;
        QCOMPARE(unknown.qualify(candidate).status, IntakeStatus::invalid_input);
        QVERIFY(db.image() == before);
    }

    void emptyPageRequiresWitness_data() {
        QTest::addColumn<int>("failure");
        QTest::addColumn<int>("status");
        for (const auto &[name, failure, status] :
             {std::tuple{"missing-witness", -1, IntakeStatus::unproved},
              {"unknown-capability", int(HistoryFailure::unknown_capability),
               IntakeStatus::unproved},
              {"unproved", int(HistoryFailure::unproved), IntakeStatus::unproved},
              {"unsupported", int(HistoryFailure::unsupported), IntakeStatus::unsupported},
              {"temporary", int(HistoryFailure::temporarily_unavailable),
               IntakeStatus::unavailable}})
            QTest::newRow(name) << failure << int(status);
    }

    void emptyPageRequiresWitness() {
        QFETCH(int, failure);
        QFETCH(int, status);
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        const auto before = db.image();
        Attempt attempt(
            db,
            {catch_up(), {}, {}, RangeProof{Coverage::recovered, "11", "14", "recover-11-14", {}}});
        attempt.witness = false;
        if (failure >= 0)
            attempt.adapter.response = HistoryFailure(failure);
        const auto result = attempt.qualify();
        QCOMPARE(result.status, IntakeStatus(status));
        QVERIFY(!result.qualified);
        QCOMPARE(attempt.adapter.calls, 1u);
        QCOMPARE(result.adapter_failure,
                 failure < 0 ? std::optional<HistoryFailure>{} : HistoryFailure(failure));
        QVERIFY(db.image() == before);
    }

    void emptyPageCoverageAndTokenPresence_data() {
        QTest::addColumn<bool>("recovered");
        QTest::addColumn<bool>("present");
        QTest::newRow("unknown-exhausted") << false << false;
        QTest::newRow("recovered-exhausted") << true << false;
        QTest::newRow("unknown-present-empty") << false << true;
    }

    void emptyPageCoverageAndTokenPresence() {
        QFETCH(bool, recovered);
        QFETCH(bool, present);
        HistoryFixture fixture;
        fixture.open_catch_up();
        auto &db = fixture.startup.store;
        const auto token = present ? std::optional<QByteArray>{QByteArray("", 0)} : std::nullopt;
        const auto range =
            recovered
                ? std::optional<RangeProof>{{Coverage::recovered, "11", "14", "recover-11-14", {}}}
                : std::nullopt;
        const auto proof = recovered ? recovered_11_14 : QByteArray{};
        const auto frame = page_frame({}, token, proof);
        Attempt attempt(db, {catch_up(), {}, token, range});
        const auto qualified = attempt.qualified();
        QCOMPARE(attempt.bind(*qualified).status, IntakeStatus::committed);
        page_row(db, 1, frame, "T", 0, 0, 0);
        if (present) {
            QCOMPARE(frame, page_frame({}, std::nullopt, proof));
            db.reopen();
            const auto before = db.image();
            const RetainedPageRequest retained{attempt.binding, attempt.binding, frame, {}};
            const auto reconstruct = [&] {
                return reconstruct_history_page(db.writer(), attempt.owner, attempt.registered,
                                                retained);
            };
            QCOMPARE(reconstruct().status, IntakeStatus::unproved);
            QVERIFY(db.image() == before);
            attempt.retain(frame);
            const auto original = attempt.registered.reconstruct;
            attempt.registered.reconstruct = [original](const RetainedPageRequest &request) {
                const auto real = original(request);
                if (!real)
                    return real;
                auto alternative = *real;
                alternative.page.next.reset();
                const QList matches{*real, alternative};
                const auto same_bytes = [&](const RetainedHistoryPage &candidate) {
                    return page_frame({}, candidate.page.next, {}) == request.page;
                };
                return std::ranges::count_if(matches, same_bytes) == 1 ? real : std::nullopt;
            };
            QCOMPARE(reconstruct().status, IntakeStatus::unproved);
            QVERIFY(db.image() == before);
            attempt.registered.reconstruct = original;
            const auto exact = reconstruct();
            QCOMPARE(exact.status, IntakeStatus::committed);
            QVERIFY(exact.qualified);
            QCOMPARE(attempt.bind(*exact.qualified).storage_status,
                     std::optional{HistoryStatus::exact_replay});
            QVERIFY(db.image() == before);
        }
        QCOMPARE(attempt.finish(*qualified).status, IntakeStatus::committed);
        page_row(db, 1, frame, "T", 0, 0, 1, proof);
        operation_row(db, 2, 2, token.value_or(QByteArray{}), 0, present ? 0 : 1);
        const SqlRows token_fact =
            present ? SqlRows{{QString("blob"), 0}} : SqlRows{{QString("null"), QVariant{}}};
        auto sql_token = db.rows("SELECT typeof(continuation),length(continuation) "
                                 "FROM main.subathon_intake_operations WHERE operation_ref=X'4F'");
        canonical_cells(sql_token);
        QVERIFY(sql_token == token_fact);
        const SqlRows source{{recovered ? 5 : 4, QByteArray(recovered ? "14" : "11")}};
        const SqlRows gap{{recovered ? 2 : 1, recovered ? 2 : 0}};
        QVERIFY(db.rows("SELECT revision,frontier FROM main.subathon_intake_sources") == source);
        const SqlRows segments = recovered ? SqlRows{{2, 1, 0, recovered_11_14}} : SqlRows{};
        coverage_rows(db, gap, segments);
    }
};
} // namespace
QTEST_GUILESS_MAIN(HistoryTests)
#include "history_tests.moc"
