// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "missing_port.hpp"
#include "populated_fixture.hpp"

#include <array>

namespace storage_026_fixture {

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &failure) {
        QFAIL(failure.what());
    }
}

inline void mutate(CatalogFile &file, const QString &sql, QList<QVariant> bindings = {}) {
    file.write([&](const auto &s) { (void)s.execute({sql, bindings}); });
}

inline bool matches_sql_rows(SqlRows actual, const SqlRows &expected) {
    // Qt SQL returns typed NULLs. Match literal absent cells without converting
    // any initialized value, including an empty BLOB.
    for (auto &row : actual)
        for (auto &cell : row)
            if (cell.isNull())
                cell = QVariant{};
    return actual == expected;
}

inline auto protected_data(CatalogFile &file, bool wide_attempt_history = false) {
    auto receipts = file.rows("SELECT * FROM main.subathon_receipts ORDER BY receipt_id");
    const QString ordered_attempts =
        QStringLiteral("SELECT * FROM main.subathon_receipt_attempts ORDER BY receipt_id,ordinal");
    auto attempts = file.rows(wide_attempt_history ? ordered_attempts + " LIMIT 1024"
                                                   : ordered_attempts);
    if (wide_attempt_history) {
        // The second read has no SQL limit, so the observer detects overflow
        // instead of silently dropping the end of the retained history.
        attempts.append(file.rows(ordered_attempts + " LIMIT -1 OFFSET 1024"));
    }
    return std::array{
        std::move(receipts),
        std::move(attempts),
        file.rows("SELECT * FROM main.subathon_contributions ORDER BY contribution_id"),
        file.rows("SELECT * FROM main.subathon_contribution_observations ORDER BY receipt_id"),
        file.rows("SELECT * FROM main.subathon_retained_configurations ORDER BY reference"),
        file.rows("SELECT * FROM main.subathon_retained_contexts ORDER BY reference"),
        file.rows("SELECT * FROM main.subathon_plan_revisions ORDER BY plan_ref,revision"),
        file.rows("SELECT * FROM main.subathon_app_results ORDER BY operation_scope,operation_ref"),
        file.rows("SELECT * FROM main.subathon_review_work ORDER BY work_scope,work_ref"),
        file.rows(
            "SELECT * FROM main.subathon_review_entries ORDER BY work_scope,work_ref,ordinal"),
        file.rows("SELECT * FROM main.subathon_review_terminal_requests "
                  "ORDER BY operation_scope,operation_ref,request_ordinal")};
}

template <class Body> auto read_missing(CatalogFile &file, Body body) {
    using Result = std::invoke_result_t<Body, const SqliteWorkSession &>;
    std::optional<Result> result;
    const auto work = qt_test_support::run_work(
        file.writer(),
        [&](auto ctx) -> QVariant {
            const auto s = ctx.begin();
            result = body(s);
            s.rollback();
            return {};
        },
        10000);
    require(result && work.transaction == TransactionOutcome::rolled_back && !work.value &&
                work.error == WorkError::not_committed,
            "Selected seam was entered inside a real writer unit with an acknowledged rollback");
    return *result;
}

inline void stage_scope(CatalogFile &file, const QByteArray &effect_scope = "score") {
    file.write([&](const auto &s) {
        (void)s.execute({"INSERT INTO main.subathon_intake_policies VALUES(?,1,?)",
                         {QByteArray("install"), QByteArray("review-policy")}});
        (void)s.execute({"INSERT INTO main.subathon_intake_members VALUES(?,1,?,?,?,?,?, ?,1,?)",
                         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
                          QByteArray("support"), effect_scope, QByteArray("overlap"),
                          QByteArray("review-lane")}});
        (void)s.execute({"INSERT INTO main.subathon_intake_sources "
                         "VALUES(?,?,?,?,1,?,9,1,1,?,?,?)",
                         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
                          QByteArray("g3"), QByteArray("cut"), QByteArray("F"),
                          QByteArray("cursor-contract")}});
        (void)s.execute({"INSERT INTO main.subathon_intake_gaps VALUES(?,?,?,?,1,?,1,1,0,?,?,?)",
                         {QByteArray("install"), QByteArray("P"), QByteArray("A"), QByteArray("S"),
                          QByteArray("G"), QByteArray("F"), QVariant{}, QByteArray("gap-basis")}});
    });
}

} // namespace storage_026_fixture
