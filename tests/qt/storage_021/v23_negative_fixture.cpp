// SPDX-License-Identifier: GPL-3.0-or-later
#include "v23_negative_fixture.hpp"

namespace subathon::server::detail::testing {
namespace {

// SqliteWriter drains on its owning Qt thread. Each run arms one disposable path.
thread_local V23NegativeSpecimen *active_specimen{};
constexpr auto history_insert = "INSERT INTO main.subathon_migration_history(";

} // namespace

V23NegativeSpecimen::V23NegativeSpecimen(QString file, Phase injection)
    : path(std::move(file)), phase(injection) {
    Q_ASSERT(!active_specimen);
    active_specimen = this;
}

V23NegativeSpecimen::~V23NegativeSpecimen() { active_specimen = nullptr; }

V23NegativeOwner::V23NegativeOwner(V23OwnerConnection &owner)
    : owner_(owner),
      specimen_(active_specimen && owner.path() == active_specimen->path ? active_specimen
                                                                         : nullptr) {}

ReadResult V23NegativeOwner::read(const QString &sql) {
    if (specimen_)
        specimen_->attempts.append(sql);
    auto result = owner_.read(sql);
    if (!specimen_ || result.error != StorageError::none)
        return result;

    if (specimen_->phase == V23NegativeSpecimen::Phase::after_history &&
        sql.startsWith(QLatin1StringView(history_insert)))
        inject_orphan();

    const auto &attempts = specimen_->attempts;
    if (sql == "PRAGMA foreign_keys" && attempts.size() >= 2 &&
        attempts[attempts.size() - 2] == "PRAGMA foreign_keys=ON" && result.rows.size() == 1 &&
        result.rows.front().size() == 1) {
        const auto &value = result.rows.front().front();
        specimen_->foreign_keys_restored = (value.metaType().id() == QMetaType::LongLong ||
                                            value.metaType().id() == QMetaType::Int) &&
                                           value.toLongLong() == 1;
    }
    return result;
}

V23OwnerConnection::Control
V23NegativeOwner::transaction(V23OwnerConnection::Transaction operation) {
    using Transaction = V23OwnerConnection::Transaction;
    if (specimen_)
        specimen_->attempts.append(operation == Transaction::begin    ? "BEGIN IMMEDIATE"
                                   : operation == Transaction::commit ? "COMMIT"
                                                                      : "ROLLBACK");
    return owner_.transaction(operation);
}

void V23NegativeOwner::before_history() {
    if (specimen_ && specimen_->phase == V23NegativeSpecimen::Phase::before_history)
        inject_orphan();
}

void V23NegativeOwner::inject_orphan() {
    if (specimen_->injection_after != 0)
        return;
    specimen_->injection_after = specimen_->attempts.size();

    // A well-formed receipt ID absent from the donor. UPDATE preserves the
    // rebuilt projection and all non-FK constraints. Prove the orphan on this
    // transaction with a separate, table-scoped SQLite query.
    const auto update =
        owner_.read("UPDATE main.subathon_plan_headers SET receipt_id=X'FFFFFFFFFFFFFFFE' "
                    "WHERE rowid=(SELECT min(rowid) FROM main.subathon_plan_headers) "
                    "AND NOT EXISTS (SELECT 1 FROM main.subathon_receipts "
                    "WHERE receipt_id=X'FFFFFFFFFFFFFFFE') RETURNING rowid");
    if (update.error != StorageError::none || update.rows.size() != 1 ||
        update.rows.front().size() != 1)
        return;
    const auto orphan = owner_.read("PRAGMA main.foreign_key_check('subathon_plan_headers')");
    specimen_->orphan_verified =
        orphan.error == StorageError::none && orphan.rows.size() == 1 &&
        orphan.rows.front().size() == 4 &&
        orphan.rows.front()[0].toString() == "subathon_plan_headers" &&
        orphan.rows.front()[1].toLongLong() == update.rows.front().front().toLongLong() &&
        orphan.rows.front()[2].toString() == "subathon_receipts";
}

} // namespace subathon::server::detail::testing
