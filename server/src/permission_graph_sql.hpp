// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "permission_graph_codec.hpp"
#include <array>
#include <span>

namespace subathon::server::permission_graph {
enum class ColumnKind { reference, revision, payload, channel, flag, singleton };
struct Column {
    const char *name;
    ColumnKind kind;
};
using K = ColumnKind;
inline constexpr std::array head_columns{
    Column{"installation_ref", K::reference}, Column{"revision", K::revision},
    Column{"owner_ref", K::reference}, Column{"catalog", K::payload}};
inline constexpr std::array role_columns{
    Column{"installation_ref", K::reference}, Column{"channel", K::channel},
    Column{"role_ref", K::reference}, Column{"entries", K::payload}};
inline constexpr std::array policy_columns{
    Column{"installation_ref", K::reference}, Column{"channel", K::channel},
    Column{"account_ref", K::reference}, Column{"memberships", K::payload},
    Column{"entries", K::payload}};
inline constexpr std::array installation_columns{
    Column{"singleton", K::singleton}, Column{"installation_ref", K::reference},
    Column{"session_epoch_ref", K::reference}, Column{"revision_ref", K::reference}};
inline constexpr std::array account_columns{
    Column{"installation_ref", K::reference}, Column{"account_ref", K::reference},
    Column{"application_access_enabled", K::flag}, Column{"session_epoch_ref", K::reference},
    Column{"revision_ref", K::reference}};
struct Table {
    const char *name;
    std::span<const Column> columns;
};
inline const std::array tables{Table{"subathon_permission_head", head_columns},
                               Table{"subathon_permission_roles", role_columns},
                               Table{"subathon_permission_accounts", policy_columns},
                               Table{"subathon_account_installation", installation_columns},
                               Table{"subathon_accounts", account_columns}};
inline bool integer(const QVariant &v) {
    return !v.isNull() &&
           (v.metaType().id() == QMetaType::LongLong || v.metaType().id() == QMetaType::Int);
}
struct Field {
    qint64 type, length, scalar;
};
struct Metadata {
    std::size_t table;
    qint64 rowid;
    std::array<Field, 5> fields{};
};
inline QString type_code(const QString &n) {
    return "CASE typeof(" + n +
           ") WHEN 'blob' THEN 1 WHEN 'integer' THEN 2 WHEN 'null' THEN 3 WHEN 'text' THEN 4 ELSE "
           "5 END";
}
// CAST counts encoded bytes even for malformed TEXT (including embedded NUL),
// unlike length(TEXT), which counts characters. No newer SQLite function needed.
inline QString byte_length(const QString &name) {
    return "coalesce(length(CAST(" + name + " AS BLOB)),0)";
}
inline bool scalar(K k) { return k == K::channel || k == K::flag || k == K::singleton; }
// All tables/channels share one physical row and byte budget. No policy payload
// is selected until the COMPLETE metadata bounds pass has succeeded.
inline std::vector<Metadata> metadata(const SqliteWorkSession &s, std::string_view installation,
                                      const PermissionGraphLimits &l) {
    std::vector<Metadata> result;
    std::uint64_t physical = 0;
    bool valid = true;
    for (std::size_t t = 0; t < tables.size(); ++t) {
        const auto &table = tables[t];
        QStringList columns{"rowid"};
        for (const auto &c : table.columns) {
            const auto n = QString::fromLatin1(c.name);
            columns << type_code(n) << byte_length(n)
                    << ("CASE WHEN typeof(" + n + ")='integer' THEN " + n + " ELSE 0 END");
        }
        const auto rows = s.execute({"SELECT " + columns.join(',') + " FROM main." + table.name +
                                         " WHERE installation_ref=? LIMIT ?",
                                     {blob(installation), qint64(l.rows - result.size() + 1)}});
        space(std::uint64_t(rows.size()) <= l.rows - result.size());
        space(std::uint64_t(rows.size()) <= result.max_size() - result.size());
        for (const auto &r : rows) {
            data(r.size() == qsizetype(1 + 3 * table.columns.size()) && integer(r[0]));
            Metadata m{t, r[0].toLongLong(), {}};
            for (std::size_t i = 0; i < table.columns.size(); ++i) {
                const auto at = static_cast<qsizetype>(1 + i * 3);
                data(integer(r[at]) && integer(r[at + 1]) && integer(r[at + 2]));
                auto &f = m.fields[i];
                f = {r[at].toLongLong(), r[at + 1].toLongLong(), r[at + 2].toLongLong()};
                data(f.length >= 0);
                const auto k = table.columns[i].kind;
                // Even a wrong SQLite class cannot hide an oversized field.
                if (!scalar(k)) {
                    space(std::uint64_t(f.length) <=
                          (k == K::reference ? l.reference_bytes : l.selected_bytes));
                    space(std::uint64_t(f.length) <=
                          static_cast<std::uint64_t>(QByteArray::maxSize()));
                    add(physical, std::uint64_t(f.length), l.selected_bytes);
                } else
                    add(physical, 8, l.selected_bytes);
                valid &= scalar(k) ? f.type == 2 : f.type == 1;
                if (k == K::reference || k == K::payload)
                    valid &= f.length > 0;
                if (k == K::revision)
                    valid &= f.length == 8;
                if (k == K::channel || k == K::flag)
                    valid &= f.scalar == 0 || f.scalar == 1;
                if (k == K::singleton)
                    valid &= f.scalar == 1;
            }
            result.push_back(m);
        }
    }
    data(valid);
    return result;
}
inline std::vector<SqlRow> payload(const SqliteWorkSession &s, const std::vector<Metadata> &meta) {
    std::vector<SqlRow> result;
    space(meta.size() <= result.max_size());
    result.reserve(meta.size());
    for (const auto &m : meta) {
        const auto &table = tables[m.table];
        QStringList names, guards{"rowid=?"};
        QList<QVariant> bindings{m.rowid};
        for (std::size_t i = 0; i < table.columns.size(); ++i) {
            const auto n = QString::fromLatin1(table.columns[i].name);
            names << n;
            guards << (type_code(n) + "=?") << (byte_length(n) + "=?");
            bindings << m.fields[i].type << m.fields[i].length;
            if (scalar(table.columns[i].kind)) {
                guards << (n + "=?");
                bindings << m.fields[i].scalar;
            }
        }
        auto rows = s.execute({"SELECT " + names.join(',') + " FROM main." + table.name +
                                   " WHERE " + guards.join(" AND ") + " LIMIT 2",
                               std::move(bindings)});
        data(rows.size() == 1 && rows[0].size() == qsizetype(table.columns.size()));
        for (std::size_t i = 0; i < table.columns.size(); ++i) {
            const auto &v = rows[0][static_cast<qsizetype>(i)];
            const auto &f = m.fields[i];
            if (scalar(table.columns[i].kind))
                data(integer(v) && v.toLongLong() == f.scalar);
            else
                data(!v.isNull() && v.metaType().id() == QMetaType::QByteArray &&
                     v.toByteArray().size() == f.length);
        }
        result.push_back(std::move(rows[0]));
    }
    return result;
}
inline std::string text(const SqlRow &r, qsizetype at) {
    const auto b = r[at].toByteArray();
    return std::string(b.constData(), static_cast<std::size_t>(b.size()));
}
inline std::string_view view(const SqlRow &r, qsizetype at) {
    // QVariant owns the QByteArray throughout the full projection/decoding pass.
    const auto &b = *static_cast<const QByteArray *>(r[at].constData());
    return {b.constData(), static_cast<std::size_t>(b.size())};
}
} // namespace subathon::server::permission_graph
