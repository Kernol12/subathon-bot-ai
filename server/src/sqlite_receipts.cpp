// SPDX-License-Identifier: GPL-3.0-or-later
#include <QThread>
#include <subathon/server/sqlite_receipts.hpp>
#include <utility>

namespace subathon::server {
QByteArray encode_receipt_uint64(std::uint64_t) { throw SqliteReceiptsNotImplemented(); }
std::uint64_t decode_receipt_uint64(const QByteArray &) { throw SqliteReceiptsNotImplemented(); }
MigrationManifest receipt_repository_manifest() { throw SqliteReceiptsNotImplemented(); }

SqliteReceiptRepository::SqliteReceiptRepository(SqliteWorkContext context,
                                                 SqliteReceiptLimits limits)
    : capability_(std::move(context)), construction_thread_(QThread::currentThread()),
      limits_(limits) {}
SqliteReceiptRepository::SqliteReceiptRepository(AdoptActiveReceiptSessionTag,
                                                 SqliteWorkSession session,
                                                 SqliteReceiptLimits limits)
    : capability_(std::move(session)), construction_thread_(QThread::currentThread()),
      limits_(limits) {}

std::unique_ptr<ReceiptTransaction> SqliteReceiptRepository::begin() {
    throw SqliteReceiptsNotImplemented();
}
} // namespace subathon::server
