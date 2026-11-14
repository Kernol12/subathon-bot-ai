// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "permission_graph_sql.hpp"
#include "reference_authority_values.hpp"
namespace subathon::server::reference_authority {
using permission_graph::blob;
using permission_graph::text;
using permission_graph::view;
struct Column {
    const char *name;
    int kind = 0;
}; // reference0, u64 blob1, i64 2, state3
inline constexpr std::array head_columns{Column{"installation_ref"}, Column{"revision", 1}};
inline constexpr std::array binding_columns{Column{"installation_ref"},
                                            Column{"capability_ref"},
                                            Column{"principal_ref"},
                                            Column{"rule_ref"},
                                            Column{"rule_version_ref"},
                                            Column{"action_ref"},
                                            Column{"action_version_ref"},
                                            Column{"timer_ref"},
                                            Column{"provider"},
                                            Column{"source_account"},
                                            Column{"identifier_namespace"},
                                            Column{"requested_ms", 2},
                                            Column{"original_author"},
                                            Column{"execution_account"},
                                            Column{"created_revision", 1}};
inline constexpr std::array event_columns{Column{"installation_ref"},
                                          Column{"capability_ref"},
                                          Column{"revision", 1},
                                          Column{"state", 3},
                                          Column{"actor_ref"},
                                          Column{"session_ref"},
                                          Column{"session_revision_ref"},
                                          Column{"authenticated_utc_ms", 2}};
inline QByteArray u64(std::uint64_t v) {
    QByteArray b(8, 0);
    for (int i = 7; i >= 0; --i) {
        b[i] = char(v & 255);
        v >>= 8;
    }
    return b;
}
inline std::uint64_t unsigned_value(const SqlRow &r, qsizetype at) {
    const auto b = view(r, at);
    data(b.size() == 8);
    std::uint64_t n = 0;
    for (unsigned char c : b)
        n = (n << 8) | c;
    return n;
}
// A finite indexed selection: obtain only rowid and fixed metadata, admit ALL
// encoded byte lengths (even wrong-class TEXT), then materialize one guarded row.
inline std::optional<SqlRow> select(const SqliteWorkSession &s, const char *table,
                                    std::span<const Column> columns, const QString &where,
                                    const QList<QVariant> &bindings,
                                    const ReferenceAuthorityLimits &l, std::uint64_t &selected,
                                    const QString &order = {}) {
    QStringList meta{"rowid"}, names;
    for (auto c : columns) {
        const auto n = QString::fromLatin1(c.name);
        names << n;
        meta << permission_graph::type_code(n) << permission_graph::byte_length(n)
             << ("CASE WHEN typeof(" + n + ")='integer' THEN " + n + " ELSE 0 END");
    }
    auto rows = s.execute({"SELECT " + meta.join(',') + " FROM main." + table + " WHERE " + where +
                               order + " LIMIT 1",
                           bindings});
    if (rows.empty())
        return {};
    const auto &r = rows[0];
    data(r.size() == qsizetype(1 + 3 * columns.size()));
    bool valid = true;
    for (std::size_t i = 0; i < columns.size(); ++i) {
        auto at = qsizetype(1 + 3 * i);
        data(permission_graph::integer(r[at]) && permission_graph::integer(r[at + 1]));
        auto len = r[at + 1].toLongLong();
        data(len >= 0);
        const auto kind = columns[i].kind;
        space(std::uint64_t(len) <= std::uint64_t(QByteArray::maxSize()));
        if (kind == 0)
            space(std::uint64_t(len) <= l.reference_bytes);
        add(selected, kind >= 2 ? (kind == 3 ? 1 : 8) : std::uint64_t(len),
            l.registry_selected_bytes);
        valid &= r[at].toLongLong() == (kind >= 2 ? 2 : 1);
        valid &= kind == 0 ? len > 0 : kind == 1 ? len == 8 : true;
        if (kind == 3)
            valid &= r[at + 2].toLongLong() >= 0 && r[at + 2].toLongLong() <= 2;
    }
    data(valid);
    auto result = s.execute(
        {"SELECT " + names.join(',') + " FROM main." + table + " WHERE rowid=? LIMIT 1", {r[0]}});
    data(result.size() == 1 && result[0].size() == qsizetype(columns.size()));
    return std::move(result[0]);
}
} // namespace subathon::server::reference_authority
