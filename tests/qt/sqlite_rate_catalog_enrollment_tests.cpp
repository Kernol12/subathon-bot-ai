// SPDX-License-Identifier: GPL-3.0-or-later

#include "support/session_installation_epoch_fixture.hpp"
#include "support/sqlite_permission_graph_fixture.hpp"
#include "support/sqlite_rate_override_writer_fixture.hpp"

#include <QCryptographicHash>

namespace {
using namespace installation_epoch_fixture;
namespace rate = rate_fixture;
namespace graph = permission_graph_fixture;
namespace core = rate::core;

constexpr auto original_revision = "permission-rates-v1";
constexpr auto fixture_revision = "permission-rates-fixture-843";

// Independently framed full trust values, including the three management bindings.
constexpr auto original_digest = "5a3cca3bc37dd2d9db60065352145ae4561112441e9bd45f0be01cccb6352ce0";

PermissionGraphTrust original_trust() {
    auto trust = graph::trust();
    trust.catalog.revision = original_revision;
    trust.catalog.operations.push_back(
        {rate::operation, core::PermissionOperationClass::ordinary, {"pair"}, {}, {}});
    return trust;
}

#ifdef SUBATHON_RATE_CATALOG_ENROLLMENT_FIXTURE
constexpr auto fixture_digest = "3cf08fb4299e4f4ee6f6882f016d60b709eccf3828dbcc2d8e6fc529a6c420f8";

PermissionGraphTrust fixture_trust() {
    auto trust = original_trust();
    trust.catalog.revision = fixture_revision;
    trust.catalog.operations.push_back(
        {"fixture.audit.read.v1", core::PermissionOperationClass::ordinary, {}, {}, {}});
    return trust;
}
#endif

void expect_persisted_trust(const rate::RateFile &file, const PermissionGraphTrust &trust,
                            unsigned revision, const char *digest = nullptr) {
    const auto bytes = graph::encoded(trust);
    require(file.rows("SELECT revision,catalog FROM subathon_permission_head") ==
                SqlRows{{graph::encoded(std::uint64_t(revision)), bytes}},
            "Exact committed CAS revision and independently encoded full trust");
    if (digest)
        require(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex() == digest,
                "Full trust matches independently derived SHA-256");
}

void bootstrap(rate::RateFile &file, const PermissionGraphTrust &trust, bool permit = true,
               const char *slot = "pair") {
    auto entry = rate::grant();
    entry.resources.front().slot = slot;
    const core::PermissionEditableState use{
        core::PermissionFactsState::complete,
        {},
        {{"A", {}, permit ? std::vector{entry} : std::vector<core::PermissionPolicyEntry>{}}}};
    const core::PermissionEditableState grant{
        core::PermissionFactsState::complete, {}, {{"A", {}, {}}}};
    file.write([&](const auto &session) {
        seed_identity(session, "A");
        require(SqlitePermissionGraphUnit(session, "I", trust).bootstrap("A", use, grant) ==
                    PermissionGraphWrite::staged,
                "Bootstrap a valid stored graph and session");
    });
}

class Authority final : public RateAuthoritySource {
  public:
    Authority(const PermissionGraphTrust &trust, unsigned revision)
        : trust_(trust), revision_(revision) {}

    RateAuthorityResolution resolve(const SqliteWorkSession &session) override {
        ++calls;
        const auto row = record();
        SqliteAccountSessionUnit sessions(session, {{}});
        Clock clock;
        const auto resolved = resolve_current_account_session(lookup(row), false, sessions, clock);
        exact_session(resolved, row);
        auto loaded = SqlitePermissionGraphUnit(session, "I", trust_).load();
        const auto &graph = selected<StoredPermissionGraph>(loaded);
        require(graph.revision == revision_ && graph.installation == "I" &&
                    permission_graph_fixture::encoded(graph.catalog) ==
                        permission_graph_fixture::encoded(trust_.catalog),
                "Resolved session and trusted persisted graph reach the rate unit");
        return RateAuthorityFacts{selected<ResolvedAccountSession>(resolved), graph};
    }

    unsigned calls = 0;

  private:
    const PermissionGraphTrust &trust_;
    unsigned revision_;
};

QByteArray framed_revision(const char *revision) {
    const QByteArray value(revision);
    // These fixture revisions fit in one byte of the RDC1 u32be length.
    return QByteArray(3, '\0') + char(value.size()) + value;
}

QByteArray replace_revision(QByteArray audit, const char *before, const char *after) {
    const auto field = framed_revision(before);
    const auto offset = audit.indexOf(field);
    require(audit.startsWith("RDC1") && offset >= 0 && audit.lastIndexOf(field) == offset,
            "Exactly one historical revision field in the authentic audit");
    audit.replace(offset, field.size(), framed_revision(after));
    return audit;
}

void recover(rate::RateFile &file, const PermissionGraphTrust &trust,
             std::optional<RateStoreRefusal> refusal = {}) {
    const auto preview = rate::first_preview();
    const auto decisions = file.rows(rate::decision_rows);
    std::optional<RateRecoveryResult> result;
    file.write([&](const auto &session) {
        Authority authority(trust, 1);
        result = SqliteRateOverrideUnit(session).recover(authority, trust, preview.edit, preview);
        require(authority.calls == 1, "Recovery resolves live authority exactly once");
    });
    if (refusal) {
        rate::expect_refusal(*result, *refusal);
    } else {
        require(std::holds_alternative<RateReceiptSelection>(*result),
                "Recover retained receipt after live v1 authorization");
        rate::expect_receipt(selected<RateReceiptSelection>(*result), RateReceiptOrigin::retained,
                             preview);
    }
    require(file.rows(rate::decision_rows) == decisions, "Recovery preserves immutable audit");
}

void replay(const char *historical_revision, bool permit,
            std::optional<RateStoreRefusal> refusal = {}) {
    rate::RateFile file;
    const auto trust = original_trust();
    bootstrap(file, trust);
    const auto preview = rate::first_preview();
    file.write([&](const auto &session) {
        Authority authority(trust, 1);
        const auto result = SqliteRateOverrideUnit(session).save(
            authority, trust, preview.edit, preview, preview.basis, [] { return 2000; });
        rate::expect_receipt(selected<RateReceiptSelection>(result),
                             RateReceiptOrigin::newly_staged, preview);
    });
    recover(file, trust);
    if (!permit) {
        file.write([&](const auto &session) {
            const core::PermissionEditableState use{
                core::PermissionFactsState::complete, {}, {{"A", {}, {}}}};
            require(SqlitePermissionGraphUnit(session, "I", trust).replace_use(1, use) ==
                        PermissionGraphWrite::staged,
                    "Remove permission through the graph CAS before recovering a valid audit");
        });
        expect_persisted_trust(file, trust, 2, original_digest);
        file.write([&](const auto &session) {
            Authority authority(trust, 2);
            rate::expect_refusal(
                SqliteRateOverrideUnit(session).recover(authority, trust, preview.edit, preview),
                RateStoreRefusal::denied);
            require(authority.calls == 1,
                    "Valid historical provenance supplies no live permission");
        });
        return;
    }

    const auto original = file.rows(rate::decision_rows);
    auto modified = original;
    auto &audit = modified.front().back();
    audit = replace_revision(audit.toByteArray(), original_revision, historical_revision);
    file.write([&](const auto &session) {
        (void)session.execute(
            {"UPDATE subathon_rate_decisions SET decision=? WHERE decision_ref=X'44'", {audit}});
    });
    require(file.rows(rate::decision_rows) == modified, "Only the framed audit revision changes");
    recover(file, trust, refusal);
}

class SqliteRateCatalogEnrollmentTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

#ifdef SUBATHON_RATE_CATALOG_ENROLLMENT_FIXTURE
    void compatibleCatalogStagesExactDecision() {
        scenario([] {
            rate::RateFile file;
            const auto original = original_trust();
            const auto extension = fixture_trust();
            const auto preview = rate::first_preview();
            bootstrap(file, original);
            expect_persisted_trust(file, original, 1, original_digest);
            file.write([&](const auto &session) {
                require(SqlitePermissionGraphUnit(session, "I", extension).synchronize_catalog(1) ==
                            PermissionGraphWrite::staged,
                        "Explicit trusted CAS synchronizes the compatible extension");
            });
            expect_persisted_trust(file, extension, 2, fixture_digest);
            require(file.rows(rate::decision_rows).empty(), "Decision starts with no prior audit");
            std::optional<RateSaveResult> result;
            file.write([&](const auto &session) {
                Authority authority(extension, 2);
                result = SqliteRateOverrideUnit(session).save(authority, extension, preview.edit,
                                                              preview, preview.basis,
                                                              [] { return 2000; });
                require(authority.calls == 1, "Decision resolves live authority exactly once");
            });
            require(std::holds_alternative<RateReceiptSelection>(*result),
                    "Compatible, enrolled, synchronized trust stages the exact rate decision");
            rate::expect_receipt(selected<RateReceiptSelection>(*result),
                                 RateReceiptOrigin::newly_staged, preview);

            // RDC1 actor/session sA/v and SAS1 first read scope, CAS revision 2,
            // individual source U/grant G, and acceptance time 2000.
            const auto audit =
                QByteArray::fromHex("52444331 0000000149 000000014b 0000000144 0000000145 "
                                    "0000000153 000000014f 00 0000000141 000000027341 "
                                    "0000000176 0000002c 53415331") +
                QByteArray(40, '\0') + framed_revision(fixture_revision) +
                QByteArray::fromHex(
                    "0000000000000002 00 0000000155 0000000147 01 00000000000007d0");
            require(file.rows(rate::decision_rows) ==
                        SqlRows{
                            { blob("D"),
                              blob("I"),
                              blob("K"),
                              blob("E"),
                              blob("S"),
                              rate::rin_format,
                              rate::first_intent,
                              rate::rdc_format,
                              audit }},
                    "Committed decision preserves the exact intent, witness and extended revision");
        });
    }

    void enrolledHistoricalAuditReplaysUnderLiveV1() {
        scenario([] { replay(fixture_revision, true); });
    }
#else
    void changedRateShapeRefuses() {
        scenario([] {
            rate::RateFile file;
            auto trust = original_trust();
            trust.catalog.operations.back().resource_slots = {"other"};
            bootstrap(file, trust, true, "other");
            expect_persisted_trust(file, trust, 1);
            const auto preview = rate::first_preview();
            file.write([&](const auto &session) {
                Authority authority(trust, 1);
                rate::expect_refusal(SqliteRateOverrideUnit(session).preview(
                                         authority, trust, preview.edit, preview.basis),
                                     RateStoreRefusal::unavailable);
                require(authority.calls == 1, "Changed shape reached rate authorization");
            });
            require(file.rows(rate::decision_rows).empty(), "Changed shape produces no audit");
        });
    }

    void unlistedHistoricalAuditRefusesWithLivePermission() {
        scenario([] { replay(fixture_revision, true, RateStoreRefusal::invalid_retained_data); });
    }

    void validHistoricalAuditRefusesWithoutLivePermission() {
        scenario([] { replay(original_revision, false, RateStoreRefusal::denied); });
    }
#endif
};
} // namespace

QTEST_GUILESS_MAIN(SqliteRateCatalogEnrollmentTests)
#include "sqlite_rate_catalog_enrollment_tests.moc"
