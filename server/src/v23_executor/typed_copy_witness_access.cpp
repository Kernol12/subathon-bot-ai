// SPDX-License-Identifier: GPL-3.0-or-later
#include "typed_copy_witness_access.hpp"

#include "copy_comparison.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <stdexcept>

namespace subathon::server::detail::testing {

void require_v23_typed_fixture_copy_match(const QSqlDatabase &database) {
    if (!database.isOpen() || database.driverName() != "QSQLITE")
        throw std::runtime_error(
            "Typed copy witness requires an open disposable QSQLITE connection");
    require_v23_copy_match([&database](std::string_view sql) {
        QSqlQuery query(database);
        if (!query.exec(QString::fromUtf8(sql.data(), static_cast<qsizetype>(sql.size()))))
            throw std::runtime_error(query.lastError().text().toStdString());
        if (query.record().count() != 1)
            throw std::runtime_error("Typed copy probe did not return one column");
        SqlRows rows;
        while (query.next())
            rows.append(SqlRow{query.value(0)});
        if (query.lastError().isValid())
            throw std::runtime_error(query.lastError().text().toStdString());
        return rows;
    });
}

} // namespace subathon::server::detail::testing
