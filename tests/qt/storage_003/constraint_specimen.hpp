// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "protected_predecessor_fixture.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

namespace storage_003_fixture {

// Closed negative-copy and physical constraint specimens. There is no public
// arbitrary SQL entry, and this connection is never used by the product writer.
class ConstraintSpecimen final {
  public:
    enum class Open { preparation, read_only };
    enum class Drift { marker21, digest21, index22, digest22 };
    enum class Initial { configuration, state, root, slot, unreferenced_root };
    enum class Mutation {
        insert_head,
        replace_head,
        upsert_head,
        delete_head,
        update_slotless_head,
        insert_slot,
        replace_slot,
        upsert_slot,
        delete_slot,
        update_evidence,
        delete_evidence,
        replace_evidence
    };

    explicit ConstraintSpecimen(const DisposableFile &file, Open mode = Open::preparation)
        : name_(QUuid::createUuid().toString(QUuid::WithoutBraces)),
          db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        try {
            db_.setDatabaseName(file.path());
            db_.setConnectOptions(mode == Open::read_only
                                      ? "QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=0"
                                      : "QSQLITE_BUSY_TIMEOUT=0");
            require(db_.open(), "Open the disposable specimen on its own QSQLITE connection");
            expect_single("PRAGMA busy_timeout", integer(0));
            if (mode == Open::preparation) {
                expect_single("PRAGMA journal_mode=WAL", QStringLiteral("wal"));
                exec_control("PRAGMA synchronous=FULL");
                expect_single("PRAGMA synchronous", integer(2));
            } else {
                exec_control("PRAGMA query_only=ON");
                expect_single("PRAGMA query_only", integer(1));
            }
            exec_control("PRAGMA foreign_keys=ON");
            expect_single("PRAGMA foreign_keys", integer(1));
        } catch (...) {
            close();
            throw;
        }
    }

    ~ConstraintSpecimen() { close(); }
    ConstraintSpecimen(const ConstraintSpecimen &) = delete;
    ConstraintSpecimen &operator=(const ConstraintSpecimen &) = delete;

    void make_k_orphan() {
        exec_control("PRAGMA foreign_keys=OFF");
        expect_single("PRAGMA foreign_keys", integer(0));
        begin();
        QSqlQuery remove(db_);
        require(remove.exec("DELETE FROM subathon_retained_configurations WHERE reference=X'4b'") &&
                    remove.numRowsAffected() == 1,
                "Delete only K with one affected row on this offline copy");
        commit();
        exec_control("PRAGMA foreign_keys=ON");
        expect_single("PRAGMA foreign_keys", integer(1));
    }

    void verify_k_orphan() {
        exec_control("BEGIN");
        expect_rows(collect(select_state, 1), {ready_state}, "Ready A still points at K");
        expect_rows(collect(select_head, 1), initial_head, "A remains the only head");
        expect_rows(collect(select_configuration, 1), {}, "K alone is absent");
        expect_single("SELECT * FROM pragma_integrity_check() LIMIT 2", QStringLiteral("ok"));
        const auto violations = collect("SELECT * FROM pragma_foreign_key_check() LIMIT 2", 2);
        expect_rows(violations,
                    {{QStringLiteral("subathon_app_states"), integer(1),
                      QStringLiteral("subathon_retained_configurations"), integer(0)}},
                    "Exactly one real FK violation before the probe");
        const qsizetype fk_bytes = 4 * 8 + violations[0][0].toString().toUtf8().size() +
                                   violations[0][2].toString().toUtf8().size();
        require(fk_bytes == missing_k_fk_row_bytes, "FK result row charges the fixed 83 bytes");
        commit();
    }

    void drift(Drift kind) {
        begin();
        QSqlQuery query(db_);
        if (kind == Drift::index22) {
            require(query.exec("CREATE INDEX subathon_test_v22_drift "
                               "ON subathon_clock_evidence(kind)"),
                    "Add only the fixed projection drift index");
        } else if (kind == Drift::marker21) {
            require(query.exec("UPDATE subathon_migration_history "
                               "SET migration_id='wrong-v21-marker' WHERE version=21") &&
                        query.numRowsAffected() == 1,
                    "Replace only the v21 marker");
        } else {
            const int version = kind == Drift::digest21 ? 21 : 22;
            auto bad = QByteArray::fromHex(step_digests[version - 1]);
            bad[0] = char(static_cast<unsigned char>(bad.at(0)) ^ 1);
            require(query.prepare("UPDATE subathon_migration_history SET content_sha256=? "
                                  "WHERE version=? AND content_sha256=?"),
                    "Prepare the fixed digest-copy control");
            query.addBindValue(bad);
            query.addBindValue(version);
            query.addBindValue(QByteArray::fromHex(step_digests[version - 1]));
            require(query.exec() && query.numRowsAffected() == 1,
                    "Flip only the first digest octet");
        }
        commit();
    }

    void begin() { exec_control("BEGIN IMMEDIATE"); }
    void commit() { exec_control("COMMIT"); }
    void rollback() { exec_control("ROLLBACK"); }

    void require_missing_head_at_commit() {
        QSqlQuery query(db_);
        require(!query.exec("COMMIT"), "Real COMMIT refuses the missing singleton parent");
        expect_constraint(query.lastError(), 787, "FOREIGN KEY constraint failed");
        rollback();
    }

    void insert(Initial kind) { stage(kind); }

    void reject_head_first() {
        QSqlQuery query(db_);
        require(!query.exec(head_insert), "Head-first INSERT is physically refused");
        expect_constraint(query.lastError(), 1811, "checkpoint head insert");
    }

    void reject(Mutation mutation, const QString &trigger_message) {
        QSqlQuery query(db_);
        prepare_mutation(query, mutation);
        require(!query.exec(), "Fixed constraint counterpart must fail at its actual statement");
        expect_constraint(query.lastError(), 1811, trigger_message);
    }

    void permit_unreferenced_change(Mutation mutation) {
        require(mutation == Mutation::update_evidence || mutation == Mutation::delete_evidence ||
                    mutation == Mutation::replace_evidence,
                "Only unreferenced-evidence controls may use this entry");
        QSqlQuery query(db_);
        prepare_mutation(query, mutation);
        require(query.exec() && query.numRowsAffected() == 1,
                "DDL permits this exact edit on the otherwise lawful unreferenced root");
    }

  private:
    static constexpr auto head_insert = "INSERT INTO subathon_app_head VALUES(1,X'41')";
    static constexpr auto evidence_args = "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    static constexpr auto slot_args = "(?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

    void close() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }

    void exec_control(const char *sql) {
        QSqlQuery query(db_);
        require(query.exec(QString::fromLatin1(sql)), query.lastError().text());
    }

    SqlRows collect(const char *sql, qsizetype limit) {
        QSqlQuery q(db_);
        require(q.exec(QString::fromLatin1(sql)), q.lastError().text());
        SqlRows rows;
        while (q.next()) {
            require(rows.size() < limit, "Fixed specimen projection stays within its row bound");
            SqlRow row;
            for (int col = 0; col < q.record().count(); ++col)
                row.append(q.value(col));
            rows.append(row);
        }
        require(!q.lastError().isValid(), q.lastError().text());
        return rows;
    }

    void expect_single(const char *sql, const QVariant &value) {
        expect_rows(collect(sql, 1), {{value}}, QString::fromLatin1(sql));
    }

    static void expect_constraint(const QSqlError &error, int extended, const QString &message) {
        bool valid = false;
        const auto code = error.nativeErrorCode().toInt(&valid);
        require(valid && (code == 19 || code == extended) && error.databaseText().contains(message),
                QString("Expected exact SQLite constraint witness, got %1 %2")
                    .arg(error.nativeErrorCode(), error.text()));
    }

    static void prepare(QSqlQuery &query, const QString &sql, const SqlRow &row) {
        require(query.prepare(sql), query.lastError().text());
        for (const auto &cell : row)
            query.addBindValue(cell);
    }

    void stage(Initial kind) {
        QSqlQuery query(db_);
        switch (kind) {
        case Initial::configuration:
            prepare(query, "INSERT INTO subathon_retained_configurations VALUES(?,?)",
                    initial_configuration[0]);
            break;
        case Initial::state:
            prepare(query, "INSERT INTO subathon_app_states VALUES(?,?,?,?,?,?,?,?,?,?,?)",
                    active_state);
            break;
        case Initial::root:
            prepare(query, QString("INSERT INTO subathon_clock_evidence VALUES") + evidence_args,
                    seed_evidence);
            break;
        case Initial::slot:
            prepare(query, QString("INSERT INTO subathon_clock_checkpoint VALUES") + slot_args,
                    seed_slot);
            break;
        case Initial::unreferenced_root:
            // From real A/1/1. An otherwise lawful semantic M root at A/2/2,
            // kept unreferenced so that immutability is not proved by an FK.
            prepare(query, QString("INSERT INTO subathon_clock_evidence VALUES") + evidence_args,
                    unreferenced_root());
            break;
        }
        require(query.exec() && query.numRowsAffected() == 1,
                "Fixed physical seed inserts one row");
    }

    static SqlRow unreferenced_root() {
        return root_row(2, "A", "M",
                        QByteArray::fromHex(
                            "0000000035010000000153000000014d00000001410000000141"
                            "0000000000000001000000000000000100000000000000020000000000000002"));
    }

    static void prepare_mutation(QSqlQuery &q, Mutation mutation) {
        switch (mutation) {
        case Mutation::insert_head:
            return prepare(q, head_insert, {});
        case Mutation::replace_head:
            return prepare(q, "REPLACE INTO subathon_app_head VALUES(1,X'41')", {});
        case Mutation::upsert_head:
            return prepare(q,
                           QString(head_insert) +
                               " ON CONFLICT(singleton) DO UPDATE SET token=excluded.token",
                           {});
        case Mutation::delete_head:
            return prepare(q, "DELETE FROM subathon_app_head WHERE singleton=1", {});
        case Mutation::update_slotless_head:
            return prepare(q, "UPDATE subathon_app_head SET token=token WHERE singleton=1", {});
        case Mutation::insert_slot:
            return prepare(q, QString("INSERT INTO subathon_clock_checkpoint VALUES") + slot_args,
                           seed_slot);
        case Mutation::replace_slot:
            return prepare(q, QString("REPLACE INTO subathon_clock_checkpoint VALUES") + slot_args,
                           seed_slot);
        case Mutation::upsert_slot:
            return prepare(
                q,
                QString("INSERT INTO subathon_clock_checkpoint VALUES") + slot_args +
                    " ON CONFLICT(singleton) DO UPDATE SET base_token=excluded.base_token",
                seed_slot);
        case Mutation::delete_slot:
            return prepare(q, "DELETE FROM subathon_clock_checkpoint WHERE singleton=1", {});
        case Mutation::update_evidence:
            return prepare(q,
                           "UPDATE subathon_clock_evidence SET ledger_revision=1 "
                           "WHERE revision=2 AND ordinal=0",
                           {});
        case Mutation::delete_evidence:
            return prepare(q, "DELETE FROM subathon_clock_evidence WHERE revision=2 AND ordinal=0",
                           {});
        case Mutation::replace_evidence: {
            auto changed = unreferenced_root();
            changed[8] = integer(1);
            return prepare(
                q, QString("REPLACE INTO subathon_clock_evidence VALUES") + evidence_args, changed);
        }
        }
    }

    const QString name_;
    QSqlDatabase db_;
};

} // namespace storage_003_fixture
