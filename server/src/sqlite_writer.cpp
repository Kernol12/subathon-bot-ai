// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server {
// Explicit baseline stubs: no SQL, queue or lifecycle behavior is implemented.
class SqliteWriter::Impl {};
SqliteWriter::SqliteWriter(QObject *parent) : QObject(parent), impl_(std::make_unique<Impl>()) {}
SqliteWriter::~SqliteWriter() = default;
StorageError SqliteWriter::open(const QString &, StorageLimits) {
    return StorageError::feature_not_implemented;
}
StorageError SqliteWriter::enqueue(const WriteBatch &, Completion) {
    return StorageError::feature_not_implemented;
}
StorageError SqliteWriter::shutdown(std::function<void()>) {
    return StorageError::feature_not_implemented;
}

class SqliteReader::Impl {};
SqliteReader::SqliteReader() : impl_(std::make_unique<Impl>()) {}
SqliteReader::~SqliteReader() = default;
StorageError SqliteReader::open(const QString &) { return StorageError::feature_not_implemented; }
ReadResult SqliteReader::read(const SqlStatement &, qsizetype) {
    return {StorageError::feature_not_implemented, {}};
}
StorageError SqliteReader::close() { return StorageError::feature_not_implemented; }
} // namespace subathon::server
