// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../server/src/sqlite_clock_checkpoint.hpp"
#include "../support/sqlite_test_support.hpp"

#include <QByteArray>
#include <QMetaType>
#include <array>

namespace storage_003_fixture {
using namespace subathon::server;
using qt_test_support::require;

using Charge = detail::ClockStatementLedger;

// The K-orphan returns the first violation: (27 + 8 + 40 + 8) bytes.
// The healthy first pass returns 330,180 bytes and no FK violation row.
inline constexpr Charge missing_k{6, 187, 661, 0, 0, 301, 3472, 330263};
inline constexpr qsizetype missing_k_fk_row_bytes = 83;
static_assert(missing_k.result_bytes == 330180 + missing_k_fk_row_bytes);
inline constexpr Charge ready_probe{7, 1586, 2247, 0, 0, 301, 3471, 330206};
inline constexpr Charge blank_probe{4, 187, 312, 0, 0, 2, 2, 23};
// Both marker refusals stop after reading the entire history result. The
// 16-byte wrong ID replaces the 38-byte ID in the TEXT and BLOB projections.
inline constexpr Charge wrong_marker_probe{4, 187, 567, 0, 0, 299, 3467, 330126};
inline constexpr Charge wrong_digest_probe{4, 187, 567, 0, 0, 299, 3467, 330170};
inline constexpr Charge ready_advance{31, 2684, 9865, 3, 76, 911, 10643, 1020621};
inline constexpr Charge blank_advance{198, 8804, 142915, 69, 1800, 3239, 38698, 4223254};
inline constexpr Charge ready_cancel{22, 2684, 8772, 0, 0, 301, 3471, 330206};
inline constexpr Charge old_inspect{12, 187, 1322, 0, 0, 600, 6936, 660360};
inline constexpr Charge successor_inspect{12, 187, 1322, 0, 0, 634, 7334, 693858};
inline constexpr Charge successor_empty_witness{13, 187, 1472, 0, 0, 635, 7335, 693866};
// v22 history alone refuses old-inspector and digest drift. Index drift adds
// one 12-cell schema row charging 352 bytes before the projection comparison.
inline constexpr Charge successor_history_stop{4, 187, 567, 0, 0, 316, 3666, 346919};
inline constexpr Charge successor_index_stop{4, 187, 567, 0, 0, 317, 3678, 347271};
inline constexpr qsizetype joined_bytes = 298;
inline constexpr qsizetype changes_result_bytes = 8;

inline StorageLimits profile(const Charge &c) {
    return {64,     c.attempts,    c.peak_sql, qMax(qsizetype{1}, c.binding_bytes),
            c.rows, c.result_bytes};
}

inline constexpr StorageLimits setup_profile{64, 4096, 65536, 1048576, 8192, 16777216};
inline constexpr auto empty_additions =
    "SELECT CASE WHEN NOT EXISTS(SELECT 1 FROM main.subathon_clock_checkpoint) AND "
    "NOT EXISTS(SELECT 1 FROM main.subathon_clock_evidence) THEN 1 ELSE 0 END";

inline constexpr std::array metered_limits{
    &StorageLimits::statements_per_batch, &StorageLimits::statement_utf8_bytes,
    &StorageLimits::result_rows, &StorageLimits::result_value_bytes,
    &StorageLimits::bound_value_bytes};

// These are storage grammar bytes. The qualified 250 ms observation for W2–W4
// belongs to the genuine application producer and is not derived from this slot.
inline const QByteArray slot_document =
    QByteArray::fromHex("00"
                        "00000001490000000154000000014c0000000150000000014f00000001430000000156"
                        "010000000000000000010000000000000000"
                        "0100000000000000000000");

inline const QByteArray seed_root =
    QByteArray::fromHex("0000000035010000000153000000014a00000001410000000141"
                        "0000000000000000000000000000000000000000000000010000000000000001");
inline const QByteArray applied_root =
    QByteArray::fromHex("0000000035010000000153000000015100000001410000000142"
                        "0000000000000001000000000000000100000000000000020000000000000002");
inline const QByteArray semantic_root =
    QByteArray::fromHex("0000000035010000000153000000014d00000001420000000142"
                        "0000000000000002000000000000000200000000000000030000000000000003");

inline QVariant integer(qint64 n) { return QVariant::fromValue(n); }
inline QVariant blob(const QByteArray &b) { return QVariant::fromValue(b); }
inline QVariant sql_null() { return QVariant(QMetaType::fromType<qint64>()); }

inline const SqlRow ready_state{blob("A"),  blob("E"),  blob("ready"), blob("K"),
                                integer(0), integer(0), integer(0),    integer(0),
                                integer(0), integer(0), integer(0)};
inline const SqlRow active_state{blob("A"),  blob("E"),  blob("active"), blob("K"),
                                 integer(1), integer(0), integer(10000), integer(0),
                                 integer(0), integer(0), integer(0)};
inline const SqlRow seed_slot{integer(1), blob("A"),          blob("K"),  integer(1),
                              integer(1), integer(1),         integer(0), sql_null(),
                              sql_null(), integer(10000),     integer(0), integer(1000),
                              integer(0), blob(slot_document)};
inline const SqlRow successor_slot{integer(1), blob("B"),          blob("K"),  integer(1),
                                   integer(2), integer(2),         integer(0), sql_null(),
                                   sql_null(), integer(9887),      integer(0), integer(1000),
                                   integer(0), blob(slot_document)};
inline const SqlRow same_base_slot{integer(1), blob("B"),          blob("K"),  integer(1),
                                   integer(3), integer(2),         integer(0), sql_null(),
                                   sql_null(), integer(9887),      integer(0), integer(1000),
                                   integer(0), blob(slot_document)};
inline const SqlRow semantic_slot{integer(1), blob("B"),          blob("K"),  integer(1),
                                  integer(3), integer(3),         integer(0), sql_null(),
                                  sql_null(), integer(9887),      integer(0), integer(1000),
                                  integer(0), blob(slot_document)};

// Positional projection includes the seven nullable evidence columns: episode,
// schedule, nominal time, fold, link revision, link ordinal and page.
inline SqlRow root_row(qint64 revision, const QByteArray &base, const QByteArray &operation,
                       const QByteArray &document) {
    return {integer(revision), integer(0), integer(0),    blob("I"),  blob("T"),       blob("L"),
            blob(base),        blob("K"),  integer(0),    blob("S"),  blob(operation), sql_null(),
            sql_null(),        sql_null(), sql_null(),    integer(1), sql_null(),      sql_null(),
            sql_null(),        integer(1), blob(document)};
}

inline const SqlRow seed_evidence = root_row(1, "A", "J", seed_root);
inline const SqlRow applied_evidence = root_row(2, "B", "Q", applied_root);
inline const SqlRow semantic_evidence = root_row(3, "B", "M", semantic_root);

inline SqlRow joined_row(const QByteArray &head, const SqlRow &slot, const SqlRow &root) {
    SqlRow result{blob(head)};
    result.append(slot.sliced(1));
    result.append({root[2], root[15], root[9], root[10], root[6], root[7], root[20]});
    return result;
}

inline const SqlRow seed_join = joined_row("A", seed_slot, seed_evidence);
inline const SqlRow mismatch_join = joined_row("B", seed_slot, seed_evidence);
inline const SqlRow applied_join = joined_row("B", successor_slot, applied_evidence);
inline const SqlRow same_base_join = joined_row("B", same_base_slot, applied_evidence);
inline const SqlRow semantic_join = joined_row("B", semantic_slot, semantic_evidence);

inline constexpr qint64 original_main = 10000;
inline constexpr qint64 qualified_elapsed = 250;
inline constexpr qint64 advanced_main = 9750;
inline constexpr qint64 requested_main = 137;
inline constexpr qint64 applied_main = 9887;

inline void expect_row(const SqlRow &actual, const SqlRow &expected, const QString &subject) {
    require(actual.size() == expected.size(), subject + ": column count");
    for (qsizetype i = 0; i != expected.size(); ++i) {
        const auto &got = actual[i];
        const auto &want = expected[i];
        const auto column = subject + ": column " + QString::number(i);
        if (want.isNull()) {
            require(got.isNull(), column + " SQL NULL");
        } else if (want.metaType().id() == QMetaType::LongLong) {
            require(!got.isNull() &&
                        (got.metaType().id() == QMetaType::LongLong ||
                         got.metaType().id() == QMetaType::Int) &&
                        got.toLongLong() == want.toLongLong(),
                    column + " INTEGER");
        } else {
            require(!got.isNull() && got.metaType() == want.metaType() && got == want,
                    column + " exact typed value");
        }
    }
}

inline void expect_rows(const SqlRows &actual, const SqlRows &expected, const QString &subject) {
    require(actual.size() == expected.size(), subject + ": row count");
    for (qsizetype i = 0; i != expected.size(); ++i)
        expect_row(actual[i], expected[i], subject);
}

inline void expect_commit(const SqliteWorkResult &result) {
    require(result.transaction == TransactionOutcome::committed &&
                result.error == WorkError::none && result.value.has_value(),
            "Actual acknowledged COMMIT and normal work value");
}

template <typename Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

} // namespace storage_003_fixture
