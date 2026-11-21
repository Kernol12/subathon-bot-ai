// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Included after the shared Stage 1 writer and scope helpers.
namespace {

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

void intake_origin(Populated &f) {
    f.install();
    advance(f.db);
    stage_scope(f.db);
}

void catchup(CatalogFile &db, int next = 1, int slots = 0) {
    mutate(db,
           "INSERT INTO main.subathon_intake_operations VALUES("
           "X'696e7374616c6c',X'4f',X'50',X'41',X'53',1,1,X'47',0,X'6733',?,?,X'54',?,?,0)",
           {next + 1, QByteArray("catchup:F:Z"), next, slots});
}

PageBinding page_binding(qint64 ordinal = 1) {
    return {"O", 9,   ordinal + 1, "T", {}, ordinal,
            0,   "U", "F",         "E", {}, PageBinding::Coverage::contiguous,
            "Z"};
}

QByteArray be32(quint32 n) {
    const char bytes[]{static_cast<char>(n >> 24), static_cast<char>(n >> 16),
                       static_cast<char>(n >> 8), static_cast<char>(n)};
    return QByteArray(bytes, 4);
}

QByteArray frame_fields(const QList<QByteArray> &fields) {
    QByteArray frame;
    for (const auto &field : fields) {
        frame += be32(static_cast<quint32>(field.size()));
        frame += field;
    }
    return frame;
}

// Test specimens have an explicit tag, item count and length-delimited fields.
// The oracle uses the fixed overhead and compares the literal persisted bytes.
QByteArray normalized_item(int bytes) {
    return QByteArray("I") + frame_fields({"observed", QByteArray(bytes - 17, 'i')});
}

PageAdmission page_admission(qint64 ordinal, QList<QByteArray> items) {
    auto request = page_binding(ordinal);
    request.items = items.size();
    request.page = QByteArray("P") + be32(static_cast<quint32>(items.size())) + frame_fields(items);
    return {std::move(request), std::move(items)};
}

auto intake_cursors(CatalogFile &db) {
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

void anchor_page(CatalogFile &db, const PageBinding &page) {
    mutate(db,
           "INSERT INTO main.subathon_intake_pages VALUES("
           "X'696e7374616c6c',X'4f',?,?,?, ?,0,0,NULL)",
           {page.ordinal, page.page, page.expected_continuation, page.items});
}

void completed_prefix(CatalogFile &db, int pages, int items) {
    const auto frame =
        page_admission(1, QList<QByteArray>(items, normalized_item(18))).binding.page;
    db.write([&](const auto &s) {
        (void)s.execute({"WITH RECURSIVE p(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM p WHERE n<?) "
                         "INSERT INTO main.subathon_intake_pages "
                         "SELECT X'696e7374616c6c',X'4f',n,?,X'54',?,?,1,NULL FROM p",
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

} // namespace
