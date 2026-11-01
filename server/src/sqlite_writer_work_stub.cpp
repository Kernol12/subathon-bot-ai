// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_writer.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

// Test-author baseline only. No work admission, SQL, transaction or callback
// behavior is implemented here. STORAGE-010 must replace these explicit stubs.
namespace subathon::server {
StorageError SqliteWriter::enqueue_work(Work, WorkCompletion) {
    return StorageError::feature_not_implemented;
}
SqliteWorkSession SqliteWorkContext::begin() const {
    throw SqliteWorkFailure(WorkError::feature_not_implemented);
}
SqlRows SqliteWorkSession::execute(const SqlStatement &) const {
    throw SqliteWorkFailure(WorkError::feature_not_implemented);
}
void SqliteWorkSession::commit() const {
    throw SqliteWorkFailure(WorkError::feature_not_implemented);
}
void SqliteWorkSession::rollback() const {
    throw SqliteWorkFailure(WorkError::feature_not_implemented);
}
WorkError SqliteWorkSession::cleanup() const noexcept { return WorkError::feature_not_implemented; }
StorageError testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(SqliteWriter &,
                                                                           WorkControlFault) {
    return StorageError::feature_not_implemented;
}
} // namespace subathon::server
