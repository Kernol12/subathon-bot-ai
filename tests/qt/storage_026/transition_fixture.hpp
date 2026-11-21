// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "common_fixture.hpp"

namespace storage_026_fixture {

template <class Body> Stop transition(CatalogFile &db, Body body) {
    std::optional<Stop> selected;
    const auto work = qt_test_support::run_work(
        db.writer(),
        [&](auto ctx) -> QVariant {
            const auto session = ctx.begin();
            selected = body(session);
            if (*selected == Stop::committed)
                session.commit();
            else
                session.rollback();
            return 1;
        },
        10000);
    require(selected.has_value(), "Selected transition entered after real BEGIN");
    if (*selected == Stop::committed)
        finance::normal(work);
    else
        require(work.transaction == TransactionOutcome::rolled_back &&
                    work.error == WorkError::not_committed && !work.value,
                "Refusal, readback or missing behavior leaves acknowledged cleanup");
    return *selected;
}

inline void intake_origin(Populated &f, const QByteArray &effect_scope = "score") {
    f.install();
    advance(f.db);
    stage_scope(f.db, effect_scope);
}

inline void catchup(CatalogFile &db, int next = 1, int slot_count = 0) {
    mutate(
        db,
        "INSERT INTO main.subathon_intake_operations VALUES("
        "X'696e7374616c6c',X'4f',X'50',X'41',X'53',1,1,X'47',0,X'6733',?,?,X'54',?,?,0,NULL,NULL)",
        {next + 1, QByteArray("catchup:F:Z"), next, slot_count});
}

inline PageBinding page_binding(qint64 ordinal = 1) {
    return {"O", 9,   ordinal + 1, "T", {}, ordinal,
            0,   "U", "F",         "E", {}, PageBinding::Coverage::contiguous,
            "Z"};
}

inline QByteArray be32(quint32 n) {
    const char bytes[]{static_cast<char>(n >> 24), static_cast<char>(n >> 16),
                       static_cast<char>(n >> 8), static_cast<char>(n)};
    return QByteArray(bytes, 4);
}

inline QByteArray frame_fields(const QList<QByteArray> &fields) {
    QByteArray frame;
    for (const auto &field : fields) {
        frame += be32(static_cast<quint32>(field.size()));
        frame += field;
    }
    return frame;
}

inline QByteArray be64(qint64 n) {
    const auto bits = static_cast<quint64>(n);
    return be32(quint32(bits >> 32)) + be32(quint32(bits));
}

inline QByteArray present_bytes(const std::optional<QByteArray> &bytes) {
    return bytes ? QByteArray(1, '\1') + frame_fields({*bytes}) : QByteArray(1, '\0');
}

inline OperationIntent import_intent(QByteArray cut_proof = "proof") {
    return {"install",
            "P",
            "A",
            "S",
            "O",
            1,
            1,
            "g3",
            QByteArray("T"),
            "2026-11-21",
            "UTC",
            "tzdb",
            1795219200000LL,
            {OperationIntent::Endpoint::inclusive, OperationIntent::Endpoint::exclusive,
             "date-membership", "position-order"},
            {"key-v1", "S/history", QByteArray("Z"), "g3", std::move(cut_proof)},
            "history-v1",
            "actor/decision",
            "actor/decision"};
}

// Independent import specimen. All strings use BE32 byte lengths. IOP1 carries
// import purpose, exact S, epoch and retained policy revision, inclusive/exclusive
// tags, membership and ordering bases, capability and retained decision provenance,
// local date/zone/rules and signed BE64 UTC ms, then the tagged initial token and
// the BE32-length-prefixed qualified CUT1 result. CUT1 carries endpoint meaning,
// format, namespace, position presence and bytes, the original qualifying
// generation and bounded proof. The row's operative generation and the caller's
// current decision can change independently of this retained evidence.
inline QByteArray import_frame(QByteArray proof = "proof") {
    const auto prefix = QByteArray::fromHex(
        "494f503101 00000007696e7374616c6c 0000000150 0000000141 0000000153 "
        "0000000000000001 0000000000000001 0102 "
        "0000000f646174652d6d656d62657273686970 0000000e706f736974696f6e2d6f72646572 "
        "0000000a686973746f72792d7631 0000000e6163746f722f6465636973696f6e "
        "0000000a323032362d31312d3231 00000003555443 00000004747a6462 000001a1fb671800 "
        "010000000154");
    const auto cut = QByteArray::fromHex(
                         "4355543102 000000066b65792d7631 00000009532f686973746f7279 01000000015a "
                         "000000026733") +
                     frame_fields({proof});
    return prefix + frame_fields({cut});
}

struct HistoricalHead {
    qint64 epoch, revision, policy;
    QByteArray generation;
    std::optional<QByteArray> cut, frontier;
};

inline QByteArray source_history(const HistoricalHead &prior, const HistoricalHead &selected,
                          const QByteArray &capability, const QByteArray &authority,
                          const QByteArray &proof) {
    // The row has the selected epoch/policy/generation and stable scope. The
    // fixture uses a same-scope switch, so the bound prior scope is identical.
    // prior_cut and selected_cut carry the exact qualified cut bytes and presence
    // separately, preserving both cut limits without shrinking either field.
    const auto head = [](const auto &h) {
        return frame_fields({"install", "P", "A", "S"}) + be64(h.epoch) + be64(h.revision) +
               be64(h.policy) + frame_fields({h.generation}) + present_bytes(h.frontier);
    };
    return QByteArray("SH1") + head(prior) + head(selected) +
           frame_fields({capability, authority, proof});
}

inline void anchor_source_history(CatalogFile &db, const QByteArray &ref, Purpose purpose,
                           const HistoricalHead &prior, const HistoricalHead &head,
                           const QByteArray &document) {
    mutate(db,
           "INSERT INTO main.subathon_intake_operations VALUES("
           "X'696e7374616c6c',?,X'50',X'41',X'53',?,?,NULL,?,?,1,?,NULL,1,0,1,?,?)",
           {ref, head.epoch, head.policy, int(purpose), head.generation, document,
            prior.cut ? QVariant{*prior.cut} : QVariant{}, *head.cut});
}

inline SqlRows selected_source_history(CatalogFile &db) {
    return db.rows(
        "SELECT operation_ref,purpose,epoch,policy_revision,generation,intent,"
        "gap_ref,continuation,next_ordinal,slots,outcome,prior_cut,selected_cut "
        "FROM main.subathon_intake_operations WHERE purpose IN (2,3,4) ORDER BY purpose");
}

inline QByteArray coverage_document(SegmentCoverage meaning, const QByteArray &lower,
                             const QByteArray &upper, const QByteArray &reason,
                             const QByteArray &evidence) {
    // Segment rows store the typed meaning separately. CV1 repeats it and binds
    // bounds, their meaning (exclusive lower, inclusive upper), reason and evidence.
    // The owning row supplies the exact scoped gap and operation locator.
    return QByteArray("CV1") + char(meaning) + QByteArray::fromHex("0201") +
           frame_fields({"key-v1", "S/history", lower, upper, reason, evidence});
}

// Test specimens have an explicit tag, item count and length-delimited fields.
// The oracle uses the fixed overhead and compares the literal persisted bytes.
inline QByteArray normalized_item(int bytes) {
    return QByteArray("I") + frame_fields({"observed", QByteArray(bytes - 17, 'i')});
}

inline PageAdmission page_admission(qint64 ordinal, QList<QByteArray> items) {
    auto request = page_binding(ordinal);
    request.items = items.size();
    request.page = QByteArray("P") + be32(static_cast<quint32>(items.size())) + frame_fields(items);
    return {std::move(request), std::move(items)};
}

inline auto intake_cursors(CatalogFile &db) {
    return std::array{
        db.rows("SELECT revision,generation,frontier FROM main.subathon_intake_sources"),
        db.rows("SELECT operation_ref,revision,next_ordinal,continuation,slots,outcome "
                "FROM main.subathon_intake_operations ORDER BY operation_ref"),
        db.rows("SELECT ordinal,item_count,committed_prefix,outcome "
                "FROM main.subathon_intake_pages ORDER BY ordinal"),
        db.rows("SELECT revision,status,start_frontier,upper_cut "
                "FROM main.subathon_intake_gaps ORDER BY gap_ref"),
        db.rows("SELECT revision,ordinal,coverage,document "
                "FROM main.subathon_intake_segments ORDER BY revision,ordinal"),
        db.rows("SELECT count(*) FROM main.subathon_intake_entries")};
}

inline auto fenced_history(CatalogFile &db) {
    return std::array{
        db.rows("SELECT * FROM main.subathon_intake_sources ORDER BY "
                "installation,provider,account,stream"),
        db.rows(
            "SELECT * FROM main.subathon_intake_operations ORDER BY installation,operation_ref"),
        db.rows(
            "SELECT * FROM main.subathon_intake_pages ORDER BY installation,operation_ref,ordinal"),
        db.rows("SELECT * FROM main.subathon_intake_gaps ORDER BY "
                "installation,provider,account,stream,epoch,gap_ref"),
        db.rows("SELECT * FROM main.subathon_intake_segments ORDER BY "
                "installation,provider,account,stream,epoch,gap_ref,revision,ordinal"),
        db.rows("SELECT * FROM main.subathon_intake_entries ORDER BY "
                "installation,provider,account,stream,intake_ref")};
}

inline void anchor_page(CatalogFile &db, const PageBinding &page) {
    mutate(db,
           "INSERT INTO main.subathon_intake_pages VALUES("
           "X'696e7374616c6c',X'4f',?,?,?, ?,0,0,NULL,0)",
           {page.ordinal, page.page, page.expected_continuation, page.items});
}

inline void completed_prefix(CatalogFile &db, int pages, int items) {
    const auto frame =
        page_admission(1, QList<QByteArray>(items, normalized_item(18))).binding.page;
    db.write([&](const auto &s) {
        (void)s.execute({"WITH RECURSIVE p(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM p WHERE n<?) "
                         "INSERT INTO main.subathon_intake_pages "
                         "SELECT X'696e7374616c6c',X'4f',n,?,X'54',?,?,1,NULL,0 FROM p",
                         {pages, frame, items, items}});
        if (items != 0)
            (void)s.execute(
                {"WITH RECURSIVE p(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM p WHERE n<?), "
                 "i(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM i WHERE n<?) "
                 "INSERT INTO main.subathon_intake_entries("
                 "installation,provider,account,stream,intake_ref,epoch,generation,policy_revision,"
                 "operation_ref,page_ordinal,item_ordinal,input,provenance,arrival_ms,phase,"
                 "disposition) "
                 "SELECT X'696e7374616c6c',X'50',X'41',X'53',CAST(printf('%d/%d',p.n,i.n) AS BLOB),"
                 "1,X'6733',1,X'4f',p.n,i.n,?,X'70',8000,2,1 FROM p CROSS JOIN i",
                 {pages, items, normalized_item(18)}});
    });
    require(db.rows("SELECT count(*),sum(item_count),sum(committed_prefix) "
                    "FROM main.subathon_intake_pages") ==
                    SqlRows{{pages, pages * items, pages * items}} &&
                db.rows("SELECT count(*) FROM main.subathon_intake_entries") ==
                    SqlRows{{pages * items}},
            "Only owned new rows anchor the independently counted complete page prefix");
}

} // namespace storage_026_fixture
