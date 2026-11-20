// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_original_claim_fixture.hpp"
#include "sqlite_rate_override_catalog_fixture.hpp"

namespace rate_fixture {
class RateFile final {
    QTemporaryDir dir_;

  public:
    RateFile() {
        require(dir_.isValid(), "Disposable rate file path");
        open(finance::limits());
        expect_report(run_catalog(*writer, MigrationMode::advance,
                                  provider_credentials_repository_manifest(), v16_manifest_bytes),
                      CatalogState::current, 16, 16, 16);
        receive_literal_v17(*this);
        // Provisioning uses the existing migration fixture limits. Normal rate
        // operations run on the reopened writer with shipped defaults.
        reopen();
    }

    void reopen(StorageLimits limits = {}) {
        stop();
        open(limits);
    }

    void install_fixture_trigger(const QString &sql) {
        stop();
        {
            // Exclusive offline fixture setup. Trigger bodies contain a
            // semicolon, which the production SQL allowlist deliberately rejects.
            original_claim_fixture::Observer offline(path());
            (void)offline.rows(sql);
        }
        open({});
    }

    SqlRows rows(const QString &sql) const {
        auto result = reader_.read({sql, {}});
        require(result.error == StorageError::none, "Independent committed rate query succeeds");
        return std::move(result.rows);
    }

    template <class Body> void write(Body body) {
        const auto result = qt_test_support::run_work(
            *writer,
            [=](auto context) -> QVariant {
                const auto session = context.begin();
                body(session);
                session.commit();
                return 1;
            },
            10000);
        finance::normal(result);
    }

    std::unique_ptr<SqliteWriter> writer;

  private:
    QString path() const { return dir_.filePath("rates.sqlite"); }
    void stop() {
        const auto stopped = std::make_shared<bool>(false);
        require(writer->shutdown([stopped] { *stopped = true; }) == StorageError::none &&
                    QTest::qWaitFor([stopped] { return *stopped; }, 10000),
                "Writer releases the database before reopen or offline setup");
        require(reader_.close() == StorageError::none, "Close committed observer");
        writer.reset();
    }
    void open(StorageLimits limits) {
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path(), limits) == StorageError::none, "Open single rate writer");
        require(reader_.open(path()) == StorageError::none, "Open real read-only rate observer");
    }
    mutable SqliteReader reader_;
};

constexpr auto pair_eur_usd = "0000000345555200000003555344";
constexpr auto pair_eur_gbp = "0000000345555200000003474250";
constexpr auto pair_gbp_usd = "0000000347425000000003555344";
constexpr auto operation = "rates.override.edit.v1";

inline PermissionGraphTrust literal_rate_trust() {
    auto trust = reference_permission_graph_trust();
    trust.catalog.revision = "permission-rates-v1";
    trust.catalog.operations.push_back(
        {operation, core::PermissionOperationClass::ordinary, {"pair"}, {}, {}});
    return trust;
}
inline core::PermissionPolicyEntry grant(std::vector<std::string> pairs = {pair_eur_usd}) {
    return {core::PermissionOrigin::individual,
            "U",
            {},
            operation,
            core::PermissionEffect::allow,
            "G",
            {{"pair", core::PermissionResourceMode::allowed_set, std::move(pairs)}},
            {}};
}
inline RateAuthorityFacts authorized() {
    const core::PermissionEditableState use{
        core::PermissionFactsState::complete, {}, {{"A", {}, {grant()}}}};
    return {{"I", "A", "N", "R", "P", 0, 100, true},
            {"I",
             1,
             {core::PermissionOwnerState::present, "A"},
             literal_rate_trust().catalog,
             use,
             {core::PermissionFactsState::complete, {}, {{"A", {}, {}}}},
             {{"I", "i"}, "iR"},
             {{{"I", "A", "a", true}, "aR"}}}};
}
class Authority final : public RateAuthoritySource {
  public:
    explicit Authority(RateAuthorityResolution current = authorized())
        : current_(std::move(current)) {}
    RateAuthorityResolution resolve(const SqliteWorkSession &) override {
        ++reads;
        return current_;
    }
    int reads = 0;

  private:
    RateAuthorityResolution current_;
};

inline core::Exact ratio(int n, int d = 1) { return core::Exact(n) / d; }
inline core::RatePairChange change(std::string from, std::string to, int n, int d,
                                   std::optional<core::Exact> before = {}) {
    return {std::move(from), std::move(to), std::move(before), ratio(n, d)};
}
inline const std::vector<core::RatePairChange> first_changes{change("EUR", "USD", 6, 5),
                                                             change("USD", "EUR", 5, 6)};
inline void expect_changes(const std::vector<core::RatePairChange> &actual,
                           const std::vector<core::RatePairChange> &expected) {
    require(actual.size() == expected.size(), "Exact directed change count");
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const auto &a = actual[i];
        const auto &e = expected[i];
        require(a.from_currency == e.from_currency && a.to_currency == e.to_currency &&
                    a.before == e.before && a.after == e.after,
                "Exact ordered directed pair, prior rate and new rate");
    }
}

inline RateOverrideEdit edit(std::string operation_key = "K", std::string base = "E",
                             std::string successor = "S", std::string occurrence = "O",
                             std::string decision = "D") {
    return {"I",
            std::move(operation_key),
            std::move(base),
            std::move(successor),
            {std::move(occurrence),
             core::RateOverrideKind::pinned,
             "EUR",
             "USD",
             "1.20",
             std::move(decision),
             "A",
             "r",
             {}},
            {}};
}
inline const RateSourceBasis empty_basis{"B", {}, {}};

inline RateEditPreview first_preview() {
    auto input = edit();
    return {input,
            empty_basis,
            {{input.successor,
              input.base,
              {},
              {},
              {input.override_input},
              {core::ManualRateConstraint{"O"}},
              {{{{"EUR", "1", "1"}, {"USD", "6", "5"}}}}},
             first_changes,
             {}}};
}
inline RateEditPreview replacement_preview() {
    auto input = edit("K2", "S", "T", "O2", "D2");
    input.replacement = "O";
    const auto first = first_preview();
    auto candidate = first.proposal.candidate;
    candidate.snapshot_ref = "T";
    candidate.parent_snapshot_ref = "S";
    candidate.overrides = {input.override_input};
    candidate.constraints = {core::ManualRateConstraint{"O2"}};
    return {input, empty_basis, {candidate, {}, input.replacement}};
}

inline const QByteArray first_body =
    QByteArray::fromHex("000000014f 01 00000003455552 00000003555344 00000004312e3230 "
                        "0000000144 01 0000000141 0000000172 00");
inline const QByteArray first_intent =
    QByteArray::fromHex("52494e31 0000000149 000000014b 0000000145 0000000153 "
                        "000000014f 01 00000003455552 00000003555344 00000004312e3230 "
                        "0000000144 01 0000000141 0000000172 00 "
                        "00 0000000142 00000000 00000000");
inline const QByteArray first_decision = QByteArray::fromHex(
    "52444331 0000000149 000000014b 0000000144 0000000145 0000000153 000000014f "
    "00 0000000141 000000014e 0000000152 0000000150 "
    "000000137065726d697373696f6e2d72617465732d7631 0000000000000001 "
    "00 0000000155 0000000147 01 0000000000000000");
inline const QByteArray second_body =
    QByteArray::fromHex("000000024f32 01 00000003455552 00000003555344 00000004312e3230 "
                        "000000024432 01 0000000141 0000000172 00");
inline const QByteArray second_intent =
    QByteArray::fromHex("52494e31 0000000149 000000024b32 0000000153 0000000154 "
                        "000000024f32 01 00000003455552 00000003555344 00000004312e3230 "
                        "000000024432 01 0000000141 0000000172 00 "
                        "01 000000014f 0000000142 00000000 00000000");
inline const QByteArray second_decision = QByteArray::fromHex(
    "52444331 0000000149 000000024b32 000000024432 0000000153 0000000154 000000024f32 "
    "01 000000014f 0000000141 000000014e 0000000152 0000000150 "
    "000000137065726d697373696f6e2d72617465732d7631 0000000000000001 "
    "00 0000000155 0000000147 01 0000000000000000");

inline core::FetchedRateObservationInput selected_f(std::string usd = "1.30") {
    return {{"F", "p", "c", "q", "e", {}, 0, {"T", {}}, {"", {}}},
            {{"EUR", "GBP", "0.90"}, {"EUR", "USD", std::move(usd)}}};
}
inline const core::FetchedRateObservationInput unused_g{{"G", "p", "c", "q", "e", {}, {}, {}, {}},
                                                        {{"CAD", "JPY", "2.0"}}};
inline const RateSourceBasis fetched_basis{"B", {selected_f(), unused_g}, {{0, 1}, {0, 2}}};
inline const QByteArray selected_rob =
    QByteArray::fromHex("524f4231 0000000146 0000000170 0000000163 0000000171 0000000165 "
                        "00 01 0000000000000000 01 0000000154 00 01 00000000 00 "
                        "00000002 00000003455552 00000003474250 00000004302e3930 "
                        "00000003455552 00000003555344 00000004312e3330");
inline const QByteArray unused_rob =
    QByteArray::fromHex("524f4231 0000000147 0000000170 0000000163 0000000171 0000000165 "
                        "00 00 00 00 00 00 00000001 00000003434144 000000034a5059 00000003322e30");
inline const QByteArray fetched_intent =
    QByteArray::fromHex("52494e31 0000000149 000000014b 0000000145 0000000153 "
                        "000000014f 01 00000003455552 00000003555344 00000004312e3230 "
                        "0000000144 01 0000000141 0000000172 00 "
                        "00 0000000142 00000002 0000000146 0000000147 "
                        "00000002 00000000 0000000000000001 00000000 0000000000000002");

inline RateEditPreview fetched_preview() {
    auto preview = first_preview();
    preview.basis = fetched_basis;
    auto &candidate = preview.proposal.candidate;
    candidate.sources = {fetched_basis.observations.front()};
    candidate.constraints.push_back(core::FetchedRateConstraint{"F", 1});
    candidate.components = {{{{"EUR", "1", "1"}, {"GBP", "9", "10"}, {"USD", "6", "5"}}}};
    preview.proposal.affected_rates = {change("EUR", "GBP", 9, 10), change("EUR", "USD", 6, 5),
                                       change("GBP", "EUR", 10, 9), change("GBP", "USD", 4, 3),
                                       change("USD", "EUR", 5, 6),  change("USD", "GBP", 3, 4)};
    return preview;
}

inline void seed_revision(const SqliteWorkSession &session,
                          const core::EffectiveRateSnapshotInput &input) {
    (void)session.execute({"INSERT INTO subathon_rate_revisions VALUES(?,?,?,?)",
                           {blob(input.snapshot_ref), blob(*input.parent_snapshot_ref), rsv_format,
                            snapshot_bytes(input)}});
    (void)session.execute({"UPDATE subathon_rate_head SET revision_ref=? WHERE slot=X'48'",
                           {blob(input.snapshot_ref)}});
}
inline void seed_first(RateFile &file, bool fetched = false) {
    const auto setup = fetched ? fetched_preview() : first_preview();
    file.write([=](const auto &session) {
        if (fetched) {
            (void)session.execute(
                {"INSERT INTO subathon_rate_observations VALUES(?,?,?),(?,?,?)",
                 {blob("F"), rob_format, selected_rob, blob("G"), rob_format, unused_rob}});
        }
        seed_revision(session, setup.proposal.candidate);
        (void)session.execute(
            {"INSERT INTO subathon_rate_decisions VALUES(?,?,?,?,?,?,?,?,?)",
             {blob("D"), blob("I"), blob("K"), blob("E"), blob("S"), rin_format,
              fetched ? fetched_intent : first_intent, rdc_format, first_decision}});
        (void)session.execute({"INSERT INTO subathon_rate_occurrences VALUES(?,?,?,?)",
                               {blob("O"), blob("D"), blob("S"), first_body}});
    });
}
inline void seed_second(RateFile &file) {
    file.write([](const auto &session) {
        seed_revision(session, replacement_preview().proposal.candidate);
        (void)session.execute({"INSERT INTO subathon_rate_decisions VALUES(?,?,?,?,?,?,?,?,?)",
                               {blob("D2"), blob("I"), blob("K2"), blob("S"), blob("T"), rin_format,
                                second_intent, rdc_format, second_decision}});
        (void)session.execute({"INSERT INTO subathon_rate_occurrences VALUES(?,?,?,?)",
                               {blob("O2"), blob("D2"), blob("T"), second_body}});
    });
}

template <class T, class Body> T selected(RateFile &file, Body body) {
    const auto result = std::make_shared<std::optional<T>>();
    const auto ni = std::make_shared<bool>(false);
    const auto work = qt_test_support::run_work(
        *file.writer,
        [=](auto context) -> QVariant {
            const auto session = context.begin();
            try {
                result->emplace(body(SqliteRateOverrideUnit(session)));
            } catch (const RateOverridesNotImplemented &) {
                *ni = true;
                throw;
            }
            session.commit();
            return 1;
        },
        10000);
    if (*ni)
        throw RateOverridesNotImplemented();
    finance::normal(work);
    require(result->has_value(), "Observed a typed rate result");
    return std::move(result->value());
}
template <class Result> void expect_refusal(const Result &result, RateStoreRefusal expected) {
    const auto *why = std::get_if<RateStoreRefusal>(&result);
    require(why && *why == expected, "Expected exact typed rate refusal");
}

inline void expect_receipt(const RateReceiptSelection &actual, RateReceiptOrigin origin,
                           const RateEditPreview &expected) {
    const auto &e = expected.edit;
    const auto &r = actual.receipt;
    require(actual.origin == origin && r.decision == e.override_input.decision_ref &&
                r.base == e.base && r.successor == e.successor &&
                r.introduced == e.override_input.override_ref && r.replaced == e.replacement,
            "Exact receipt identity, links, replacement and origin");
    expect_changes(r.changes, expected.proposal.affected_rates);
}

// Probe the real work outcome separately from the provisional typed value.
// Post-commit failure is thrown after the actual writer commit succeeds.
struct SaveCapture {
    std::optional<RateSaveResult> staged;
    std::optional<SqliteWorkResult> completion;
    bool not_implemented = false;
    int authority_reads = 0;
    int samples = 0;
};
inline auto enqueue_save(SqliteWriter &writer, RateOverrideEdit input, RateEditPreview preview,
                         RateSourceBasis current, RateAuthorityResolution authority,
                         bool fail_after_commit = false, bool roll_back = false,
                         std::optional<std::int64_t> accepted_time = 0) {
    auto capture = std::make_shared<SaveCapture>();
    require(writer.enqueue_work(
                [=](const auto &context) -> QVariant {
                    const auto session = context.begin();
                    Authority source(authority);
                    try {
                        capture->staged = SqliteRateOverrideUnit(session).save(
                            source, literal_rate_trust(), input, preview, current, [=] {
                                ++capture->samples;
                                return accepted_time;
                            });
                    } catch (const RateOverridesNotImplemented &) {
                        capture->not_implemented = true;
                        throw;
                    }
                    capture->authority_reads = source.reads;
                    if (roll_back) {
                        session.rollback();
                        return {};
                    }
                    session.commit();
                    if (fail_after_commit)
                        throw std::runtime_error("Synthetic post-COMMIT body failure");
                    return 1;
                },
                [capture](auto result) { capture->completion = std::move(result); }) ==
                StorageError::none,
            "Writer admits an ordinary rate work item");
    return capture;
}
inline void wait_for_save(const std::shared_ptr<SaveCapture> &capture) {
    require(QTest::qWaitFor([capture] { return capture->completion.has_value(); }, 10000),
            "Writer completes the admitted rate work item");
    if (capture->not_implemented)
        throw RateOverridesNotImplemented();
}
inline auto save(RateFile &file, const RateEditPreview &preview,
                 RateAuthorityResolution authority = authorized()) {
    auto capture =
        enqueue_save(*file.writer, preview.edit, preview, preview.basis, std::move(authority));
    wait_for_save(capture);
    finance::normal(*capture->completion);
    require(capture->staged.has_value(), "Typed rate save result");
    if (std::holds_alternative<RateReceiptSelection>(*capture->staged))
        require(capture->authority_reads == 1, "Receipt selection resolves current authority once");
    return capture;
}
} // namespace rate_fixture
