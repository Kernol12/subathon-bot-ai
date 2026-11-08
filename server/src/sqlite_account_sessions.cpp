// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <span>
#include <subathon/server/sqlite_account_sessions.hpp>
#include <subathon/server/sqlite_reference_capture.hpp>
#include <tuple>
#include <utility>
#include <vector>

namespace subathon::server {
namespace {
using ReadFailure = AccountSessionReadFailure;
using InputError = AccountSessionInputError;
using MutationResult = LifecycleWriteResult;
[[noreturn]] void input(InputError e) { throw AccountSessionInputFailure(e); }
[[noreturn]] void fail(ReadFailure e) { throw AccountSessionStorageFailure(e); }
void data(bool b) {
    if (!b)
        fail(ReadFailure::invalid_data);
}
void space(bool b) {
    if (!b)
        fail(ReadFailure::resource_limit);
}
struct Count {
    std::uint64_t limit, value{0};
    bool add(std::uint64_t n) {
        if (n > limit - value)
            return false;
        value += n;
        return true;
    }
};
void options(const AccountSessionStoreLimits &l) {
    const auto maximum =
        std::min({std::uintmax_t(std::numeric_limits<std::size_t>::max() - 1),
                  std::uintmax_t(std::numeric_limits<std::ptrdiff_t>::max() - 1),
                  std::uintmax_t(std::numeric_limits<qsizetype>::max() - 1),
                  std::uintmax_t(std::string{}.max_size()), std::uintmax_t(QByteArray::maxSize())});
    for (auto n : {l.reference_bytes, l.selected_bytes, l.mutation_bytes, l.page_rows})
        if (!n || n > maximum)
            input(InputError::invalid_limits);
    if (l.page_rows >= std::uint64_t(std::numeric_limits<qint64>::max()) ||
        l.page_rows >= std::vector<SessionRecordFacts>{}.max_size())
        input(InputError::invalid_limits);
    Count peak{std::uint64_t(maximum)};
    if (!peak.add(l.selected_bytes) || !peak.add(l.selected_bytes) || !peak.add(l.selected_bytes) ||
        !peak.add(l.mutation_bytes))
        input(InputError::invalid_limits);
}
// Project every borrowed occurrence before shape checks, ticket access or copies.
class Input {
  public:
    Input(const AccountSessionStoreLimits &l, bool mutation)
        : l_(l), n_{mutation ? l.mutation_bytes : l.selected_bytes} {}
    void fixed(std::uint64_t n) {
        if (!n_.add(n))
            input(InputError::resource_limit);
    }
    void ref(std::string_view s, std::uint64_t cap = UINT64_MAX) {
        if (s.size() > std::min(cap, l_.reference_bytes))
            input(InputError::resource_limit);
        fixed(s.size());
    }
    void verifier(std::string_view s) {
        if (s.size() > 127)
            input(InputError::resource_limit);
        fixed(s.size());
    }
    void add(const StoredInstallationFacts &v) {
        ref(v.current.installation_ref);
        ref(v.current.session_epoch_ref);
        ref(v.revision_ref);
    }
    void add(const StoredAccountFacts &v) {
        ref(v.current.installation_ref);
        ref(v.current.account_ref);
        ref(v.current.session_epoch_ref);
        ref(v.revision_ref);
        fixed(1);
    }
    void add(const StoredLocalCredential &v) {
        ref(v.installation_ref);
        ref(v.credential.account_ref, 256);
        ref(v.credential.revision_ref, 256);
        verifier(v.credential.encoded_verifier);
    }
    void add(const SessionRecordFacts &v) {
        ref(v.installation_ref);
        ref(v.account_ref);
        ref(v.session_ref);
        ref(v.installation_epoch_ref);
        ref(v.account_epoch_ref);
        ref(v.record_revision_ref);
        fixed(57);
    }
    void add(const AcceptedSessionAuthentication &v) {
        ref(v.installation_ref);
        ref(v.account_ref);
        ref(v.attempt_ref);
        ref(v.installation_epoch_ref);
        ref(v.account_epoch_ref);
        ref(v.account_revision_ref);
        ref(v.authenticator_revision_ref);
        ref(v.acceptance_revision_ref);
        fixed(9);
    }
    void add(const CompleteLocalCredentialFacts &v) {
        add(v.local);
        add(v.account);
        add(v.installation);
        ref(v.snapshot_ref);
    }

  private:
    const AccountSessionStoreLimits &l_;
    Count n_;
};
bool filled(std::initializer_list<std::string_view> v) {
    return std::all_of(v.begin(), v.end(), [](auto s) { return !s.empty(); });
}
void shape(const StoredInstallationFacts &v) {
    if (!filled({v.current.installation_ref, v.current.session_epoch_ref, v.revision_ref}))
        input(InputError::invalid_input);
}
void shape(const StoredAccountFacts &v) {
    if (!filled({v.current.installation_ref, v.current.account_ref, v.current.session_epoch_ref,
                 v.revision_ref}))
        input(InputError::invalid_input);
}
void shape(const StoredLocalCredential &v) {
    if (!filled({v.installation_ref, v.credential.account_ref, v.credential.revision_ref}))
        input(InputError::invalid_input);
}
void shape(const SessionRecordFacts &v) {
    if (!filled({v.installation_ref, v.account_ref, v.session_ref, v.installation_epoch_ref,
                 v.account_epoch_ref, v.record_revision_ref}) ||
        (v.state != SessionRecordState::active && v.state != SessionRecordState::revoked) ||
        v.lifetime_started_utc_ms > v.authenticated_utc_ms ||
        v.authenticated_utc_ms >= v.expires_utc_ms)
        input(InputError::invalid_input);
}
void shape(const AcceptedSessionAuthentication &v) {
    if (!filled({v.installation_ref, v.account_ref, v.attempt_ref, v.installation_epoch_ref,
                 v.account_epoch_ref, v.account_revision_ref, v.authenticator_revision_ref,
                 v.acceptance_revision_ref}) ||
        (v.method != AcceptedSignInMethod::local_password &&
         v.method != AcceptedSignInMethod::verified_twitch))
        input(InputError::invalid_input);
}
void shape(SessionRetention r) {
    if (r != SessionRetention::normal && r != SessionRetention::remembered)
        input(InputError::invalid_input);
}
void keys(const AccountSessionStoreLimits &l, std::initializer_list<std::string_view> refs) {
    Input n(l, false);
    for (auto r : refs)
        n.ref(r);
    if (!filled(refs))
        input(InputError::invalid_input);
}
bool same(const StoredInstallationFacts &a, const StoredInstallationFacts &b) {
    return std::tie(a.current.installation_ref, a.current.session_epoch_ref, a.revision_ref) ==
           std::tie(b.current.installation_ref, b.current.session_epoch_ref, b.revision_ref);
}
bool same(const StoredAccountFacts &a, const StoredAccountFacts &b) {
    return std::tie(a.current.installation_ref, a.current.account_ref, a.current.session_epoch_ref,
                    a.current.application_access_enabled, a.revision_ref) ==
           std::tie(b.current.installation_ref, b.current.account_ref, b.current.session_epoch_ref,
                    b.current.application_access_enabled, b.revision_ref);
}
bool same(const StoredLocalCredential &a, const StoredLocalCredential &b) {
    return std::tie(a.installation_ref, a.credential.account_ref, a.credential.revision_ref,
                    a.credential.encoded_verifier) ==
           std::tie(b.installation_ref, b.credential.account_ref, b.credential.revision_ref,
                    b.credential.encoded_verifier);
}
bool same(const SessionRecordFacts &a, const SessionRecordFacts &b) {
    return std::tie(a.installation_ref, a.account_ref, a.session_ref, a.installation_epoch_ref,
                    a.account_epoch_ref, a.record_revision_ref, a.digest, a.state,
                    a.lifetime_started_utc_ms, a.authenticated_utc_ms, a.expires_utc_ms) ==
           std::tie(b.installation_ref, b.account_ref, b.session_ref, b.installation_epoch_ref,
                    b.account_epoch_ref, b.record_revision_ref, b.digest, b.state,
                    b.lifetime_started_utc_ms, b.authenticated_utc_ms, b.expires_utc_ms);
}

bool decimal(std::string_view s) {
    return !s.empty() && (s.size() == 1 || s.front() != '0') &&
           std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
}
bool above(std::string_view s, std::string_view m) {
    return s.size() > m.size() || (s.size() == m.size() && s > m);
}
int digit(unsigned char c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    return c == '+' ? 62 : c == '/' ? 63 : -1;
}
bool b64(std::string_view s, std::size_t n, int mask) {
    return s.size() == n && std::all_of(s.begin(), s.end(), [](auto c) { return digit(c) >= 0; }) &&
           (digit(s.back()) & mask) == 0;
}
// Alphabet/length/pad-bit checks prove canonical fixed-width Base64 without
// owning decoded verifier bytes. This pure write gate never runs on stored reads.
bool verifier(std::string_view s) {
    constexpr std::string_view prefix = "$argon2id$v=19$m=";
    if (s.size() > 127 || !s.starts_with(prefix))
        return false;
    s.remove_prefix(prefix.size());
    const auto t = s.find(",t="), p = s.find(",p=1$");
    if (t == s.npos || p == s.npos || p < t + 3)
        return false;
    const auto m = s.substr(0, t), ops = s.substr(t + 3, p - t - 3);
    if (!decimal(m) || !decimal(ops) || ops == "0" || (m.size() == 1 && m.front() < '8') ||
        above(m, "262144") || above(ops, "10"))
        return false;
    s.remove_prefix(p + 5);
    const auto sep = s.find('$');
    return sep != s.npos && b64(s.substr(0, sep), 22, 15) && b64(s.substr(sep + 1), 43, 3);
}
QByteArray blob(std::string_view s) {
    return s.empty() ? QByteArray("", 0) : QByteArray(s.data(), qsizetype(s.size()));
}
QByteArray blob(const AccountSessionDigest &d) {
    return QByteArray(reinterpret_cast<const char *>(d.data()), 32);
}
bool integer(const QVariant &v) {
    return !v.isNull() &&
           (v.metaType().id() == QMetaType::Int || v.metaType().id() == QMetaType::LongLong);
}

enum class Kind { ref, credential, verifier, digest, flag, format, time, singleton };
struct Column {
    const char *name;
    Kind kind;
};
constexpr std::array ic{Column{"singleton", Kind::singleton}, Column{"installation_ref", Kind::ref},
                        Column{"session_epoch_ref", Kind::ref}, Column{"revision_ref", Kind::ref}};
constexpr std::array ac{Column{"installation_ref", Kind::ref}, Column{"account_ref", Kind::ref},
                        Column{"application_access_enabled", Kind::flag},
                        Column{"session_epoch_ref", Kind::ref}, Column{"revision_ref", Kind::ref}};
constexpr std::array lc{
    Column{"installation_ref", Kind::ref}, Column{"account_ref", Kind::credential},
    Column{"revision_ref", Kind::credential}, Column{"verifier_format", Kind::format},
    Column{"encoded_verifier", Kind::verifier}};
constexpr std::array sc{Column{"installation_ref", Kind::ref},
                        Column{"account_ref", Kind::ref},
                        Column{"session_ref", Kind::ref},
                        Column{"installation_epoch_ref", Kind::ref},
                        Column{"account_epoch_ref", Kind::ref},
                        Column{"record_revision_ref", Kind::ref},
                        Column{"digest_format", Kind::format},
                        Column{"digest", Kind::digest},
                        Column{"state", Kind::flag},
                        Column{"retention", Kind::flag},
                        Column{"lifetime_started_utc_ms", Kind::time},
                        Column{"authenticated_utc_ms", Kind::time},
                        Column{"expires_utc_ms", Kind::time}};
struct Table {
    const char *alias;
    std::span<const Column> columns;
};
const Table ti{"i", ic}, ta{"a", ac}, tl{"l", lc}, ts{"s", sc};
constexpr auto fi = "main.subathon_account_installation AS i";
constexpr auto fa = "main.subathon_accounts AS a LEFT JOIN main.subathon_account_installation AS i "
                    "ON i.installation_ref=a.installation_ref";
constexpr auto fl =
    "main.subathon_local_credentials AS l LEFT JOIN main.subathon_accounts AS a ON "
    "a.installation_ref=l.installation_ref AND a.account_ref=l.account_ref LEFT JOIN "
    "main.subathon_account_installation AS i ON i.installation_ref=l.installation_ref";
constexpr auto fs =
    "main.subathon_account_sessions AS s LEFT JOIN main.subathon_accounts AS a ON "
    "a.installation_ref=s.installation_ref AND a.account_ref=s.account_ref LEFT JOIN "
    "main.subathon_account_installation AS i ON i.installation_ref=s.installation_ref";
constexpr auto fp =
    "main.subathon_account_sessions AS s INDEXED BY subathon_account_sessions_page_idx";
constexpr auto page_payload_from = "main.subathon_account_sessions AS s";
enum class Type { null, blob, integer, other };
struct Field {
    Type type{Type::null};
    std::uint64_t length{0};
    qint64 value{0};
};
struct Meta {
    Table table;
    std::optional<qint64> rowid;
    std::array<Field, 13> fields{};
};
using RowMeta = std::vector<Meta>;
using Metas = std::vector<RowMeta>;
bool scalar(Kind k) {
    return k == Kind::flag || k == Kind::format || k == Kind::time || k == Kind::singleton;
}
// All identifiers are compiled descriptors. The metadata pass returns only
// bounded SQL class names, lengths, fixed scalars and private rowid locators.
Metas metadata(const SqliteWorkSession &s, std::span<const Table> tables, const char *from,
               const QString &tail, QList<QVariant> bindings) {
    QStringList columns;
    for (const auto &t : tables) {
        const auto a = QString::fromLatin1(t.alias);
        columns.append(a + ".rowid");
        for (const auto &c : t.columns) {
            const auto n = a + '.' + c.name;
            columns.append("typeof(" + n + ')');
            columns.append("length(" + n + ')');
            columns.append("CASE WHEN typeof(" + n + ")='integer' THEN " + n + " ELSE NULL END");
        }
    }
    const auto rows =
        s.execute({"SELECT " + columns.join(',') + " FROM " + from + tail, std::move(bindings)});
    Metas result;
    result.reserve(std::size_t(rows.size()));
    for (const auto &r : rows) {
        RowMeta out;
        out.reserve(tables.size());
        qsizetype at = 0;
        for (const auto &t : tables) {
            Meta m{t, std::nullopt, {}};
            data(at < r.size());
            if (!r[at].isNull()) {
                data(integer(r[at]));
                m.rowid = r[at].toLongLong();
            }
            ++at;
            for (std::size_t c = 0; c < t.columns.size(); ++c) {
                data(at + 2 < r.size() && r[at].metaType().id() == QMetaType::QString);
                const auto type = r[at].toString();
                auto &f = m.fields[c];
                f.type = type == "null"      ? Type::null
                         : type == "blob"    ? Type::blob
                         : type == "integer" ? Type::integer
                                             : Type::other;
                if (!r[at + 1].isNull()) {
                    data(integer(r[at + 1]) && r[at + 1].toLongLong() >= 0);
                    f.length = std::uint64_t(r[at + 1].toLongLong());
                }
                if (!r[at + 2].isNull()) {
                    data(integer(r[at + 2]));
                    f.value = r[at + 2].toLongLong();
                }
                at += 3;
            }
            out.push_back(std::move(m));
        }
        data(at == r.size());
        result.push_back(std::move(out));
    }
    return result;
}
// Complete bounds pass before class/shape checks: oversized hidden fields and
// page probes cannot be concealed by an earlier malformed/revoked field.
void bounds(const Metas &rows, const AccountSessionStoreLimits &l, Count &n) {
    for (const auto &r : rows)
        for (const auto &m : r)
            if (m.rowid)
                for (std::size_t c = 0; c < m.table.columns.size(); ++c) {
                    const auto k = m.table.columns[c].kind;
                    const auto &f = m.fields[c];
                    if (scalar(k)) {
                        space(n.add(k == Kind::time ? 8 : 1));
                        continue;
                    }
                    auto cap = l.selected_bytes;
                    if (k == Kind::ref || k == Kind::credential)
                        cap = l.reference_bytes;
                    if (k == Kind::credential)
                        cap = std::min<std::uint64_t>(cap, 256);
                    if (k == Kind::verifier)
                        cap = 127;
                    space(f.length <= cap && n.add(f.length));
                }
}
void valid(const Metas &rows) {
    for (const auto &r : rows)
        for (const auto &m : r) {
            data(m.rowid.has_value());
            for (std::size_t c = 0; c < m.table.columns.size(); ++c) {
                const auto k = m.table.columns[c].kind;
                const auto &f = m.fields[c];
                data(f.type == (scalar(k) ? Type::integer : Type::blob));
                if (k == Kind::ref || k == Kind::credential)
                    data(f.length != 0);
                if (k == Kind::digest)
                    data(f.length == 32);
                if (k == Kind::flag)
                    data(f.value == 0 || f.value == 1);
                if (k == Kind::format || k == Kind::singleton)
                    data(f.value == 1);
            }
            if (std::string_view(m.table.alias) == "s")
                data(m.fields[10].value <= m.fields[11].value &&
                     m.fields[11].value < m.fields[12].value);
        }
}
void admit(const Metas &m, const AccountSessionStoreLimits &l) {
    Count n{l.selected_bytes};
    bounds(m, l, n);
    data(m.size() <= 1);
}
void fields(Count &n, const Meta &m, std::initializer_list<int> indices) {
    for (auto c : indices)
        space(n.add(m.fields[std::size_t(c)].length));
}
// The CTE holds only fixed-width metadata. One fixed guard per column is shared
// across a page; admitted table payload is fetched only by these locators.
SqlRows payload(const SqliteWorkSession &s, const Metas &selected, const char *from,
                bool ordered = false) {
    if (selected.empty())
        return {};
    QStringList names, projection, guards, tuples;
    QList<QVariant> bindings;
    projection.append(QString::fromLatin1(selected[0][0].table.alias) + ".rowid");
    for (std::size_t t = 0; t < selected[0].size(); ++t) {
        const auto &m = selected[0][t];
        const auto a = QString::fromLatin1(m.table.alias);
        const auto r = "r" + QString::number(qulonglong(t));
        names.append(r);
        guards.append(a + ".rowid=b." + r);
        for (std::size_t c = 0; c < m.table.columns.size(); ++c) {
            const auto n = a + '.' + m.table.columns[c].name,
                       v = "v" + QString::number(qulonglong(t)) + '_' +
                           QString::number(qulonglong(c));
            names.append(v);
            projection.append(n);
            guards.append(scalar(m.table.columns[c].kind)
                              ? "typeof(" + n + ")='integer' AND " + n + "=b." + v
                              : "typeof(" + n + ")='blob' AND length(" + n + ")=b." + v);
        }
    }
    for (const auto &row : selected) {
        QStringList slots;
        for (const auto &m : row) {
            bindings.append(*m.rowid);
            slots.append("?");
            for (std::size_t c = 0; c < m.table.columns.size(); ++c) {
                bindings.append(scalar(m.table.columns[c].kind) ? m.fields[c].value
                                                                : qint64(m.fields[c].length));
                slots.append("?");
            }
        }
        tuples.append('(' + slots.join(',') + ')');
    }
    auto rows =
        s.execute({"WITH b(" + names.join(',') + ") AS (VALUES " + tuples.join(',') + ") SELECT " +
                       projection.join(',') + " FROM b CROSS JOIN " + from + " WHERE " +
                       guards.join(" AND ") + (ordered ? " ORDER BY s.session_ref ASC" : ""),
                   std::move(bindings)});
    data(std::size_t(rows.size()) == selected.size());
    for (std::size_t r = 0; r < selected.size(); ++r) {
        const auto &row = rows[qsizetype(r)];
        data(!row.empty() && integer(row[0]) && row[0].toLongLong() == *selected[r][0].rowid);
        qsizetype at = 1;
        for (const auto &m : selected[r])
            for (std::size_t c = 0; c < m.table.columns.size(); ++c, ++at) {
                data(at < row.size());
                const auto &f = m.fields[c];
                if (scalar(m.table.columns[c].kind))
                    data(integer(row[at]) && row[at].toLongLong() == f.value);
                else
                    data(!row[at].isNull() && row[at].metaType().id() == QMetaType::QByteArray &&
                         std::uint64_t(row[at].toByteArray().size()) == f.length);
            }
        data(at == row.size());
    }
    return rows;
}
std::string text(const SqlRow &r, qsizetype i) {
    const auto b = r[i].toByteArray();
    return {b.constData(), std::size_t(b.size())};
}
bool equal(const QVariant &v, std::string_view s) {
    const auto b = v.toByteArray();
    return std::string_view(b.constData(), std::size_t(b.size())) == s;
}
StoredInstallationFacts installation(const SqlRow &r, qsizetype i) {
    return {{text(r, i + 1), text(r, i + 2)}, text(r, i + 3)};
}
StoredAccountFacts account(const SqlRow &r, qsizetype i) {
    return {{text(r, i), text(r, i + 1), text(r, i + 3), r[i + 2].toLongLong() != 0},
            text(r, i + 4)};
}
StoredLocalCredential local(const SqlRow &r, qsizetype i) {
    return {text(r, i), {text(r, i + 1), text(r, i + 2), text(r, i + 4)}};
}
SessionRecordFacts record(const SqlRow &r, qsizetype i) {
    AccountSessionDigest d{};
    const auto b = r[i + 7].toByteArray();
    std::copy_n(reinterpret_cast<const std::uint8_t *>(b.constData()), 32, d.begin());
    return {text(r, i),
            text(r, i + 1),
            text(r, i + 2),
            text(r, i + 3),
            text(r, i + 4),
            text(r, i + 5),
            d,
            static_cast<SessionRecordState>(r[i + 8].toLongLong()),
            r[i + 10].toLongLong(),
            r[i + 11].toLongLong(),
            r[i + 12].toLongLong()};
}
void account_link(const SqlRow &r, qsizetype a, qsizetype i) { data(r[a] == r[i + 1]); }
struct Account {
    StoredAccountFacts account;
    StoredInstallationFacts installation;
};
struct Session {
    SessionRecordFacts record;
    SessionRetention retention;
    StoredAccountFacts account;
    StoredInstallationFacts installation;
};
// Borrows the caller ticket. Private reads never create public snapshots.
class Access {
  public:
    Access(const SqliteWorkSession &s, const AccountSessionStoreLimits &l,
           const AccountSessionReadScope &scope, std::uint64_t &ordinal, bool &exhausted)
        : s_(s), l_(l), scope_(scope), ordinal_(ordinal), exhausted_(exhausted) {}
    std::optional<StoredInstallationFacts> install(std::string_view key, bool any = false) {
        const std::array tables{ti};
        const auto m =
            metadata(s_, tables, fi, any ? " LIMIT 2" : " WHERE i.installation_ref=? LIMIT 2",
                     any ? QList<QVariant>{} : QList<QVariant>{blob(key)});
        admit(m, l_);
        if (m.empty())
            return {};
        Count n{l_.selected_bytes};
        space(n.add(1));
        fields(n, m[0][0], {1, 2, 3});
        valid(m);
        const auto rows = payload(s_, m, fi);
        if (!any)
            data(equal(rows[0][2], key));
        return installation(rows[0], 1);
    }
    std::optional<Account> account_at(std::string_view i, std::string_view a) {
        const std::array tables{ta, ti};
        const auto m =
            metadata(s_, tables, fa, " WHERE a.installation_ref=? AND a.account_ref=? LIMIT 2",
                     {blob(i), blob(a)});
        admit(m, l_);
        if (m.empty())
            return {};
        Count n{l_.selected_bytes};
        space(n.add(2));
        fields(n, m[0][0], {0, 1, 3, 4});
        valid(m);
        const auto rows = payload(s_, m, fa);
        const auto &r = rows[0];
        account_link(r, 1, 6);
        data(equal(r[1], i) && equal(r[2], a));
        return Account{account(r, 1), installation(r, 6)};
    }
    std::optional<CompleteLocalCredentialFacts> local_at(std::string_view i, std::string_view a,
                                                         bool snapshot = false) {
        const std::array tables{tl, ta, ti};
        const auto m =
            metadata(s_, tables, fl, " WHERE l.installation_ref=? AND l.account_ref=? LIMIT 2",
                     {blob(i), blob(a)});
        admit(m, l_);
        if (m.empty())
            return {};
        Count n{l_.selected_bytes};
        space(n.add(2 + (snapshot ? 44 : 0)));
        fields(n, m[0][0], {0, 1, 2, 4});
        fields(n, m[0][1], {0, 1, 3, 4});
        fields(n, m[0][2], {1, 2, 3});
        if (snapshot)
            ready();
        valid(m);
        const auto rows = payload(s_, m, fl);
        const auto &r = rows[0];
        data(r[1] == r[6] && r[2] == r[7]);
        account_link(r, 6, 11);
        data(equal(r[1], i) && equal(r[2], a));
        CompleteLocalCredentialFacts result{local(r, 1), account(r, 6), installation(r, 11), {}};
        if (snapshot) {
            result.snapshot_ref = snapshot_value();
            done();
        }
        return result;
    }
    std::optional<Session> session_at(const QString &where, QList<QVariant> bindings,
                                      bool snapshot = false, bool actor = false,
                                      const SessionLimits *supplier = nullptr) {
        const std::array tables{ts, ta, ti};
        const auto m =
            metadata(s_, tables, fs, " WHERE " + where + " LIMIT 2", std::move(bindings));
        admit(m, l_);
        if (m.empty())
            return {};
        Count n{l_.selected_bytes};
        space(n.add(59 + (snapshot ? 44 : 0) + (actor ? 1 : 0)));
        fields(n, m[0][0], {0, 1, 2, 3, 4, 5});
        fields(n, m[0][1], {0, 1, 3});
        fields(n, m[0][2], {1, 2});
        if (supplier) {
            Count strings{supplier->owned_bytes};
            space(supplier->reference_bytes >= 44 && strings.add(44));
            for (auto c : {0, 1, 2, 3, 4, 5})
                supplier_field(strings, m[0][0].fields[std::size_t(c)].length, *supplier);
            for (auto c : {0, 1, 3})
                supplier_field(strings, m[0][1].fields[std::size_t(c)].length, *supplier);
            for (auto c : {1, 2})
                supplier_field(strings, m[0][2].fields[std::size_t(c)].length, *supplier);
        }
        if (snapshot)
            ready();
        valid(m);
        const auto rows = payload(s_, m, fs);
        const auto &r = rows[0];
        data(r[1] == r[14] && r[2] == r[15]);
        account_link(r, 14, 19);
        return Session{record(r, 1), static_cast<SessionRetention>(r[10].toLongLong()),
                       account(r, 14), installation(r, 19)};
    }
    std::optional<Session> selected(std::string_view i, std::string_view a, std::string_view key) {
        auto v = session_at("s.installation_ref=? AND s.account_ref=? AND s.session_ref=?",
                            {blob(i), blob(a), blob(key)});
        if (v)
            data(v->record.installation_ref == i && v->record.account_ref == a &&
                 v->record.session_ref == key);
        return v;
    }
    std::optional<Session> identity(std::string_view key) {
        return session_at("s.session_ref=?", {blob(key)});
    }
    std::optional<Session> digest(std::string_view i, const AccountSessionDigest &d) {
        return session_at("s.installation_ref=? AND s.digest=?", {blob(i), blob(d)});
    }
    CompleteSessionFacts publish(Session v) {
        CompleteSessionFacts out{std::move(v.record), std::move(v.account.current),
                                 std::move(v.installation.current), snapshot_value()};
        done();
        return out;
    }
    std::optional<LifecyclePageFacts> page(std::string_view i, std::string_view a,
                                           const std::optional<std::string> &after,
                                           std::uint64_t page_size) {
        const std::array parents{ta, ti};
        const std::array children{ts};
        const auto parent =
            metadata(s_, parents, fa, " WHERE a.installation_ref=? AND a.account_ref=? LIMIT 2",
                     {blob(i), blob(a)});
        const auto children_meta =
            metadata(s_, children, fp,
                     after ? " WHERE s.installation_ref=? AND s.account_ref=? AND s.session_ref>? "
                             "ORDER BY s.session_ref ASC LIMIT ?"
                           : " WHERE s.installation_ref=? AND s.account_ref=? ORDER BY "
                             "s.session_ref ASC LIMIT ?",
                     after ? QList<QVariant>{blob(i), blob(a), blob(*after), qint64(page_size + 1)}
                           : QList<QVariant>{blob(i), blob(a), qint64(page_size + 1)});
        Count physical{l_.selected_bytes};
        bounds(parent, l_, physical);
        bounds(children_meta, l_, physical);
        data(parent.size() <= 1 && children_meta.size() <= page_size + 1);
        if (parent.empty()) {
            data(children_meta.empty());
            return {};
        }
        Count n{l_.selected_bytes};
        space(n.add(46));
        fields(n, parent[0][0], {0, 1, 3});
        fields(n, parent[0][1], {1, 2});
        for (const auto &r : children_meta) {
            space(n.add(57));
            fields(n, r[0], {0, 1, 2, 3, 4, 5});
        }
        ready();
        valid(parent);
        valid(children_meta);
        const auto p = payload(s_, parent, fa);
        const auto rows = payload(s_, children_meta, page_payload_from, true);
        account_link(p[0], 1, 6);
        data(equal(p[0][1], i) && equal(p[0][2], a));
        for (qsizetype at = 0; at < rows.size(); ++at) {
            data(rows[at][1] == p[0][1] && rows[at][2] == p[0][2]);
            const auto key = rows[at][3].toByteArray();
            if (at)
                data(rows[at - 1][3].toByteArray().compare(key) < 0);
            else if (after)
                data(blob(*after).compare(key) < 0);
        }
        LifecyclePageFacts result{
            {text(p[0], 1), text(p[0], 2), text(p[0], 4), p[0][3].toLongLong() != 0},
            {text(p[0], 7), text(p[0], 8)},
            {},
            {}};
        result.sessions.reserve(std::size_t(rows.size()));
        for (const auto &r : rows)
            result.sessions.push_back(record(r, 1));
        result.snapshot_ref = snapshot_value();
        done();
        return result;
    }
    void change(const SqlStatement &q) {
        (void)s_.execute(q);
        const auto r = s_.execute({"SELECT changes()", {}});
        data(r.size() == 1 && r[0].size() == 1 && integer(r[0][0]) && r[0][0].toLongLong() == 1);
    }

  private:
    const SqliteWorkSession &s_;
    const AccountSessionStoreLimits &l_;
    const AccountSessionReadScope &scope_;
    std::uint64_t &ordinal_;
    bool &exhausted_;
    static void supplier_field(Count &n, std::uint64_t size, const SessionLimits &l) {
        space(size <= l.reference_bytes && n.add(size));
    }
    void ready() const { space(!exhausted_ && l_.reference_bytes >= 44); }
    std::string snapshot_value() const {
        std::string s("SAS1", 4);
        s.append(reinterpret_cast<const char *>(scope_.read_scope_ref.data()), 32);
        for (int shift = 56; shift >= 0; shift -= 8)
            s.push_back(char((ordinal_ >> shift) & 255));
        return s;
    }
    void done() {
        if (ordinal_ == UINT64_MAX)
            exhausted_ = true;
        else
            ++ordinal_;
    }
};
template <class T, class F> AccountSessionRead<T> reading(F body) {
    try {
        auto value = body();
        if (!value)
            return ReadFailure::missing;
        return std::move(*value);
    } catch (const AccountSessionInputFailure &) {
        throw;
    } catch (const SqliteWorkFailure &) {
        throw;
    } catch (const AccountSessionStorageFailure &e) {
        return e.code();
    } catch (...) {
        return ReadFailure::unavailable;
    }
}
template <class F> MutationResult writing(F body) {
    try {
        return body();
    } catch (const AccountSessionInputFailure &) {
        throw;
    } catch (const SqliteWorkFailure &) {
        throw;
    } catch (const AccountSessionStorageFailure &) {
        throw;
    } catch (...) {
        fail(ReadFailure::unavailable);
    }
}
void supplier_options(const SessionLimits &l) {
    constexpr auto max = std::numeric_limits<std::size_t>::max();
    if (!l.reference_bytes || !l.owned_bytes || l.reference_bytes >= max ||
        l.reference_bytes > std::string{}.max_size() ||
        l.owned_bytes > std::min<std::uint64_t>(UINT64_MAX, max) / 3)
        input(InputError::invalid_limits);
}
bool current(const Session &s) {
    return s.account.current.application_access_enabled &&
           s.record.installation_epoch_ref == s.installation.current.session_epoch_ref &&
           s.record.account_epoch_ref == s.account.current.session_epoch_ref;
}
bool accepted(const SessionRecordFacts &r, const StoredAccountFacts &a,
              const StoredInstallationFacts &i, const AcceptedSessionAuthentication &v) {
    return a.current.application_access_enabled && r.installation_ref == v.installation_ref &&
           r.account_ref == v.account_ref && r.installation_epoch_ref == v.installation_epoch_ref &&
           r.account_epoch_ref == v.account_epoch_ref &&
           i.current.session_epoch_ref == v.installation_epoch_ref &&
           a.current.session_epoch_ref == v.account_epoch_ref &&
           a.revision_ref == v.account_revision_ref && r.authenticated_utc_ms == v.accepted_utc_ms;
}
// Full current parents are rechecked by the statement, using one bounded copy.
const QString fence_cte = "f(installation_ref,account_ref,account_epoch,enabled,account_revision,"
                          "installation_epoch,installation_revision) AS (VALUES(?,?,?,?,?,?,?))";
const QString fence =
    "EXISTS(SELECT 1 FROM f JOIN main.subathon_accounts AS a ON "
    "a.installation_ref=f.installation_ref AND a.account_ref=f.account_ref JOIN "
    "main.subathon_account_installation AS i ON i.installation_ref=f.installation_ref WHERE "
    "a.session_epoch_ref=f.account_epoch AND a.application_access_enabled=f.enabled AND "
    "a.revision_ref=f.account_revision AND i.singleton=1 AND "
    "i.session_epoch_ref=f.installation_epoch AND i.revision_ref=f.installation_revision)";
QList<QVariant> fences(const StoredAccountFacts &a, const StoredInstallationFacts &i) {
    return {blob(a.current.installation_ref),
            blob(a.current.account_ref),
            blob(a.current.session_epoch_ref),
            qint64(a.current.application_access_enabled),
            blob(a.revision_ref),
            blob(i.current.session_epoch_ref),
            blob(i.revision_ref)};
}
const QString record_names =
    "installation_ref,account_ref,session_ref,installation_epoch_ref,account_epoch_ref,record_"
    "revision_ref,digest_format,digest,state,retention,lifetime_started_utc_ms,authenticated_utc_"
    "ms,expires_utc_ms";
const QString record_projection =
    "n.installation_ref,n.account_ref,n.session_ref,n.installation_epoch_ref,n.account_epoch_ref,"
    "n.record_revision_ref,n.digest_format,n.digest,n.state,n.retention,n.lifetime_started_utc_ms,"
    "n.authenticated_utc_ms,n.expires_utc_ms";
const QString record_equal =
    "s.installation_ref=e.installation_ref AND s.account_ref=e.account_ref AND "
    "s.session_ref=e.session_ref AND s.installation_epoch_ref=e.installation_epoch_ref AND "
    "s.account_epoch_ref=e.account_epoch_ref AND s.record_revision_ref=e.record_revision_ref AND "
    "s.digest_format=e.digest_format AND s.digest=e.digest AND s.state=e.state AND "
    "s.retention=e.retention AND s.lifetime_started_utc_ms=e.lifetime_started_utc_ms AND "
    "s.authenticated_utc_ms=e.authenticated_utc_ms AND s.expires_utc_ms=e.expires_utc_ms";
constexpr auto record_slots = "?,?,?,?,?,?,?,?,?,?,?,?,?";
QList<QVariant> bindings(const SessionRecordFacts &v, SessionRetention retention) {
    return {blob(v.installation_ref),
            blob(v.account_ref),
            blob(v.session_ref),
            blob(v.installation_epoch_ref),
            blob(v.account_epoch_ref),
            blob(v.record_revision_ref),
            qint64(1),
            blob(v.digest),
            qint64(v.state),
            qint64(retention),
            qint64(v.lifetime_started_utc_ms),
            qint64(v.authenticated_utc_ms),
            qint64(v.expires_utc_ms)};
}
bool authenticator(Access &access, const AcceptedSessionAuthentication &v) {
    // The actual accepted-attempt membership/full verifier lineage remains the
    // caller's precondition. This is only a current physical local-revision fence.
    if (v.method != AcceptedSignInMethod::local_password)
        return true;
    const auto local = access.local_at(v.installation_ref, v.account_ref);
    return local && local->local.credential.revision_ref == v.authenticator_revision_ref;
}
const QString local_auth_fence =
    " AND EXISTS(SELECT 1 FROM main.subathon_local_credentials AS l JOIN f ON "
    "l.installation_ref=f.installation_ref AND l.account_ref=f.account_ref WHERE l.revision_ref=? "
    "AND l.verifier_format=1)";
} // namespace

AccountSessionInputFailure::AccountSessionInputFailure(AccountSessionInputError c)
    : std::runtime_error("Account session storage input rejected"), code_(c) {}
AccountSessionInputError AccountSessionInputFailure::code() const noexcept { return code_; }
AccountSessionStorageFailure::AccountSessionStorageFailure(AccountSessionReadFailure c)
    : std::runtime_error("Account session storage operation failed"), code_(c) {}
AccountSessionReadFailure AccountSessionStorageFailure::code() const noexcept { return code_; }
AccountSessionStorageNotImplemented::AccountSessionStorageNotImplemented()
    : std::logic_error("STORAGE-020 account-session storage not implemented") {}

MigrationManifest account_session_repository_manifest() {
    auto result = reference_capture_repository_manifest();
    MigrationStep step;
    step.version = 8;
    step.id = "account-sessions-v1";
    step.sql = {
        "CREATE TABLE subathon_account_installation(singleton INTEGER NOT NULL PRIMARY KEY "
        "CHECK(typeof(singleton)='integer' AND singleton=1), installation_ref BLOB NOT NULL UNIQUE "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), session_epoch_ref "
        "BLOB NOT NULL CHECK(typeof(session_epoch_ref)='blob' AND length(session_epoch_ref)>0), "
        "revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref)>0))",
        "CREATE TABLE subathon_accounts(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB "
        "NOT NULL CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), "
        "application_access_enabled INTEGER NOT NULL "
        "CHECK(typeof(application_access_enabled)='integer' AND application_access_enabled IN "
        "(0,1)), session_epoch_ref BLOB NOT NULL CHECK(typeof(session_epoch_ref)='blob' AND "
        "length(session_epoch_ref)>0), revision_ref BLOB NOT NULL "
        "CHECK(typeof(revision_ref)='blob' AND length(revision_ref)>0), PRIMARY "
        "KEY(installation_ref,account_ref), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_account_installation(installation_ref))",
        "CREATE TABLE subathon_local_credentials(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB "
        "NOT NULL CHECK(typeof(account_ref)='blob' AND length(account_ref)>0 AND "
        "length(account_ref)<=256), revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' "
        "AND length(revision_ref)>0 AND length(revision_ref)<=256), verifier_format INTEGER NOT "
        "NULL CHECK(typeof(verifier_format)='integer' AND verifier_format=1), encoded_verifier "
        "BLOB NOT NULL CHECK(typeof(encoded_verifier)='blob' AND length(encoded_verifier)<=127), "
        "PRIMARY KEY(installation_ref,account_ref), FOREIGN KEY(installation_ref,account_ref) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
        "CREATE TABLE subathon_account_sessions(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB "
        "NOT NULL CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), session_ref BLOB "
        "NOT NULL PRIMARY KEY CHECK(typeof(session_ref)='blob' AND length(session_ref)>0), "
        "installation_epoch_ref BLOB NOT NULL CHECK(typeof(installation_epoch_ref)='blob' AND "
        "length(installation_epoch_ref)>0), account_epoch_ref BLOB NOT NULL "
        "CHECK(typeof(account_epoch_ref)='blob' AND length(account_epoch_ref)>0), "
        "record_revision_ref BLOB NOT NULL CHECK(typeof(record_revision_ref)='blob' AND "
        "length(record_revision_ref)>0), digest_format INTEGER NOT NULL "
        "CHECK(typeof(digest_format)='integer' AND digest_format=1), digest BLOB NOT NULL "
        "CHECK(typeof(digest)='blob' AND length(digest)=32), state INTEGER NOT NULL "
        "CHECK(typeof(state)='integer' AND state IN (0,1)), retention INTEGER NOT NULL "
        "CHECK(typeof(retention)='integer' AND retention IN (0,1)), lifetime_started_utc_ms "
        "INTEGER NOT NULL CHECK(typeof(lifetime_started_utc_ms)='integer'), authenticated_utc_ms "
        "INTEGER NOT NULL CHECK(typeof(authenticated_utc_ms)='integer'), expires_utc_ms INTEGER "
        "NOT NULL CHECK(typeof(expires_utc_ms)='integer'), UNIQUE(installation_ref,digest), "
        "CHECK(lifetime_started_utc_ms<=authenticated_utc_ms AND "
        "authenticated_utc_ms<expires_utc_ms), FOREIGN KEY(installation_ref,account_ref) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
        "CREATE INDEX subathon_account_sessions_page_idx ON "
        "subathon_account_sessions(installation_ref,account_ref,session_ref)"};
    step.expected_schema = result.steps.back().expected_schema;
    const std::array<QString, 4> names{"subathon_account_installation", "subathon_accounts",
                                       "subathon_local_credentials", "subathon_account_sessions"};
    for (std::size_t n = 0; n < names.size(); ++n) {
        const auto &name = names[n];
        step.expected_schema.append({"table", name, name, step.sql[qsizetype(n)]});
        step.expected_schema.append(
            {"index", "sqlite_autoindex_" + name + "_1", name, std::nullopt});
    }
    step.expected_schema.append({"index", "sqlite_autoindex_subathon_account_sessions_2",
                                 "subathon_account_sessions", std::nullopt});
    step.expected_schema.append(
        {"index", "subathon_account_sessions_page_idx", "subathon_account_sessions", step.sql[4]});
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return std::tie(a.type, a.name) < std::tie(b.type, b.name);
              });
    result.steps.append(std::move(step));
    result.supported_starting_versions = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    return result;
}
SqliteAccountSessionUnit::SqliteAccountSessionUnit(const SqliteWorkSession &s,
                                                   AccountSessionReadScope scope,
                                                   AccountSessionStoreLimits limits)
    : session_(s), scope_(scope), limits_(limits) {
    options(limits_);
}
SqliteAccountSessionUnit::~SqliteAccountSessionUnit() = default;
AccountSessionRead<StoredInstallationFacts>
SqliteAccountSessionUnit::read_installation(std::string_view i) {
    keys(limits_, {i});
    return reading<StoredInstallationFacts>([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        return a.install(i);
    });
}
AccountSessionRead<StoredAccountFacts>
SqliteAccountSessionUnit::read_account(std::string_view i, std::string_view key) {
    keys(limits_, {i, key});
    return reading<StoredAccountFacts>([&]() -> std::optional<StoredAccountFacts> {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        auto v = a.account_at(i, key);
        if (!v)
            return {};
        return std::move(v->account);
    });
}
AccountSessionRead<CompleteLocalCredentialFacts>
SqliteAccountSessionUnit::read_local_credential(std::string_view i, std::string_view key) {
    Input n(limits_, false);
    n.ref(i);
    n.ref(key, 256);
    if (!filled({i, key}))
        input(InputError::invalid_input);
    return reading<CompleteLocalCredentialFacts>([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        return a.local_at(i, key, true);
    });
}
AccountSessionRead<CompleteSessionFacts>
SqliteAccountSessionUnit::read_selected(std::string_view i, std::string_view key,
                                        std::string_view session) {
    keys(limits_, {i, key, session});
    return reading<CompleteSessionFacts>([&]() -> std::optional<CompleteSessionFacts> {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        auto v = a.session_at("s.installation_ref=? AND s.account_ref=? AND s.session_ref=?",
                              {blob(i), blob(key), blob(session)}, true);
        if (!v)
            return {};
        data(v->record.installation_ref == i && v->record.account_ref == key &&
             v->record.session_ref == session);
        return a.publish(std::move(*v));
    });
}
AccountSessionRead<LifecycleCurrentSession>
SqliteAccountSessionUnit::read_lifecycle_actor(const SessionLookupRequest &q,
                                               const SessionLimits &l) {
    supplier_options(l);
    Input bytes(limits_, false);
    bytes.ref(q.installation_ref);
    bytes.fixed(32);
    if (q.installation_ref.empty())
        input(InputError::invalid_input);
    if (q.installation_ref.size() > l.reference_bytes || q.installation_ref.size() > l.owned_bytes)
        input(InputError::resource_limit);
    return reading<LifecycleCurrentSession>([&]() -> std::optional<LifecycleCurrentSession> {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        auto v = a.session_at("s.installation_ref=? AND s.digest=?",
                              {blob(q.installation_ref), blob(q.digest)}, true, true, &l);
        if (!v)
            return {};
        data(v->record.installation_ref == q.installation_ref && v->record.digest == q.digest);
        const auto r = v->retention;
        return LifecycleCurrentSession{a.publish(std::move(*v)), r};
    });
}
SessionLookup SqliteAccountSessionUnit::load_current(const SessionLookupRequest &q,
                                                     const SessionLimits &l) {
    try {
        supplier_options(l);
        Input bytes(limits_, false);
        bytes.ref(q.installation_ref);
        bytes.fixed(32);
        if (q.installation_ref.empty())
            input(InputError::invalid_input);
        if (q.installation_ref.size() > l.reference_bytes ||
            q.installation_ref.size() > l.owned_bytes)
            return SessionSourceResourceLimit{};
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        auto v = a.session_at("s.installation_ref=? AND s.digest=?",
                              {blob(q.installation_ref), blob(q.digest)}, true, false, &l);
        if (!v)
            return SessionMissing{};
        data(v->record.installation_ref == q.installation_ref && v->record.digest == q.digest);
        return a.publish(std::move(*v));
    } catch (const AccountSessionInputFailure &e) {
        return e.code() == InputError::resource_limit ? SessionLookup{SessionSourceResourceLimit{}}
                                                      : SessionLookup{SessionSourceUnavailable{}};
    } catch (const AccountSessionStorageFailure &e) {
        if (e.code() == ReadFailure::resource_limit)
            return SessionSourceResourceLimit{};
        if (e.code() == ReadFailure::invalid_data)
            return SessionSourceInvalidData{};
        return SessionSourceUnavailable{};
    } catch (const SqliteWorkFailure &e) {
        if (e.error() == WorkError::resource_limit || e.error() == WorkError::result_limit)
            return SessionSourceResourceLimit{};
        return SessionSourceUnavailable{};
    } catch (...) {
        return SessionSourceUnavailable{};
    }
}
AccountSessionRead<LifecyclePageFacts>
SqliteAccountSessionUnit::read_page(std::string_view i, std::string_view key,
                                    const std::optional<std::string> &after, std::uint64_t size) {
    Input n(limits_, false);
    n.ref(i);
    n.ref(key);
    n.fixed(9);
    if (after)
        n.ref(*after);
    if (!filled({i, key}) || (after && after->empty()) || !size || size > limits_.page_rows)
        input(InputError::invalid_input);
    return reading<LifecyclePageFacts>([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        return a.page(i, key, after, size);
    });
}

MutationResult SqliteAccountSessionUnit::insert_installation(const StoredInstallationFacts &v) {
    Input n(limits_, true);
    n.add(v);
    shape(v);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        if (a.install(v.current.installation_ref, true))
            return MutationResult::collision;
        a.change({"INSERT INTO "
                  "main.subathon_account_installation(singleton,installation_ref,session_epoch_ref,"
                  "revision_ref) SELECT 1,?,?,? WHERE NOT EXISTS(SELECT 1 FROM "
                  "main.subathon_account_installation)",
                  {blob(v.current.installation_ref), blob(v.current.session_epoch_ref),
                   blob(v.revision_ref)}});
        return MutationResult::staged;
    });
}
MutationResult SqliteAccountSessionUnit::replace_installation_epoch(
    const StoredInstallationFacts &expected, std::string_view epoch, std::string_view revision) {
    Input n(limits_, true);
    n.add(expected);
    n.ref(epoch);
    n.ref(revision);
    shape(expected);
    if (!filled({epoch, revision}) || revision == expected.revision_ref)
        input(InputError::invalid_input);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        const auto old = a.install(expected.current.installation_ref);
        if (!old || !same(*old, expected))
            return MutationResult::stale;
        a.change(
            {"UPDATE main.subathon_account_installation SET session_epoch_ref=?,revision_ref=? "
             "WHERE singleton=1 AND installation_ref=? AND session_epoch_ref=? AND revision_ref=?",
             {blob(epoch), blob(revision), blob(expected.current.installation_ref),
              blob(expected.current.session_epoch_ref), blob(expected.revision_ref)}});
        return MutationResult::staged;
    });
}
MutationResult SqliteAccountSessionUnit::insert_account(const StoredAccountFacts &v) {
    Input n(limits_, true);
    n.add(v);
    shape(v);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        const auto parent = a.install(v.current.installation_ref);
        // Validate an existing selected account/closure before collision semantics.
        if (a.account_at(v.current.installation_ref, v.current.account_ref))
            return MutationResult::collision;
        if (!parent)
            return MutationResult::stale;
        a.change(
            {"WITH n(installation_ref,account_ref,enabled,epoch,revision) AS (VALUES(?,?,?,?,?)) "
             "INSERT INTO main.subathon_accounts SELECT "
             "n.installation_ref,n.account_ref,n.enabled,n.epoch,n.revision FROM n JOIN "
             "main.subathon_account_installation AS i ON i.installation_ref=n.installation_ref "
             "WHERE i.singleton=1 AND i.session_epoch_ref=? AND i.revision_ref=? AND NOT "
             "EXISTS(SELECT 1 FROM main.subathon_accounts AS a WHERE "
             "a.installation_ref=n.installation_ref AND a.account_ref=n.account_ref)",
             {blob(v.current.installation_ref), blob(v.current.account_ref),
              qint64(v.current.application_access_enabled), blob(v.current.session_epoch_ref),
              blob(v.revision_ref), blob(parent->current.session_epoch_ref),
              blob(parent->revision_ref)}});
        return MutationResult::staged;
    });
}
MutationResult SqliteAccountSessionUnit::replace_account(const StoredAccountFacts &expected,
                                                         const StoredAccountFacts &replacement) {
    Input n(limits_, true);
    n.add(expected);
    n.add(replacement);
    shape(expected);
    shape(replacement);
    if (expected.current.installation_ref != replacement.current.installation_ref ||
        expected.current.account_ref != replacement.current.account_ref ||
        expected.revision_ref == replacement.revision_ref)
        input(InputError::invalid_input);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        const auto old =
            a.account_at(expected.current.installation_ref, expected.current.account_ref);
        if (!old || !same(old->account, expected))
            return MutationResult::stale;
        auto b = fences(old->account, old->installation);
        b.append(qint64(replacement.current.application_access_enabled));
        b.append(blob(replacement.current.session_epoch_ref));
        b.append(blob(replacement.revision_ref));
        a.change(
            {"WITH " + fence_cte +
                 ", n(enabled,epoch,revision) AS (VALUES(?,?,?)) UPDATE main.subathon_accounts AS "
                 "target SET (application_access_enabled,session_epoch_ref,revision_ref)=(SELECT "
                 "enabled,epoch,revision FROM n) WHERE EXISTS(SELECT 1 FROM f WHERE "
                 "target.installation_ref=f.installation_ref AND target.account_ref=f.account_ref "
                 "AND target.application_access_enabled=f.enabled AND "
                 "target.session_epoch_ref=f.account_epoch AND "
                 "target.revision_ref=f.account_revision) AND " +
                 fence,
             std::move(b)});
        return MutationResult::staged;
    });
}
MutationResult SqliteAccountSessionUnit::replace_local_credential(
    const std::optional<StoredLocalCredential> &expected, const StoredLocalCredential &replacement,
    const StoredAccountFacts &account_facts, const StoredInstallationFacts &installation_facts) {
    Input n(limits_, true);
    n.fixed(1);
    if (expected)
        n.add(*expected);
    n.add(replacement);
    n.add(account_facts);
    n.add(installation_facts);
    if (expected)
        shape(*expected);
    shape(replacement);
    shape(account_facts);
    shape(installation_facts);
    if (!verifier(replacement.credential.encoded_verifier) ||
        replacement.installation_ref != account_facts.current.installation_ref ||
        replacement.installation_ref != installation_facts.current.installation_ref ||
        replacement.credential.account_ref != account_facts.current.account_ref ||
        (expected && (expected->installation_ref != replacement.installation_ref ||
                      expected->credential.account_ref != replacement.credential.account_ref ||
                      expected->credential.revision_ref == replacement.credential.revision_ref)))
        input(InputError::invalid_input);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        const auto old =
            a.local_at(replacement.installation_ref, replacement.credential.account_ref);
        const auto parents =
            a.account_at(replacement.installation_ref, replacement.credential.account_ref);
        if (!parents || !same(parents->account, account_facts) ||
            !same(parents->installation, installation_facts))
            return MutationResult::stale;
        if (!expected && old)
            return MutationResult::collision;
        if (expected && (!old || !same(old->local, *expected)))
            return MutationResult::stale;
        auto b = fences(account_facts, installation_facts);
        b.append(blob(replacement.credential.revision_ref));
        b.append(blob(replacement.credential.encoded_verifier));
        if (expected) {
            b.append(blob(expected->credential.revision_ref));
            b.append(blob(expected->credential.encoded_verifier));
            a.change({"WITH " + fence_cte +
                          ", n(revision,verifier) AS (VALUES(?,?)), e(revision,verifier) AS "
                          "(VALUES(?,?)) UPDATE main.subathon_local_credentials AS l SET "
                          "(revision_ref,encoded_verifier)=(SELECT revision,verifier FROM n) WHERE "
                          "l.verifier_format=1 AND EXISTS(SELECT 1 FROM f,e WHERE "
                          "l.installation_ref=f.installation_ref AND l.account_ref=f.account_ref "
                          "AND l.revision_ref=e.revision AND l.encoded_verifier=e.verifier) AND " +
                          fence,
                      std::move(b)});
        } else {
            a.change(
                {"WITH " + fence_cte +
                     ", n(revision,verifier) AS (VALUES(?,?)) INSERT INTO "
                     "main.subathon_local_credentials SELECT "
                     "f.installation_ref,f.account_ref,n.revision,1,n.verifier FROM f,n WHERE " +
                     fence +
                     " AND NOT EXISTS(SELECT 1 FROM main.subathon_local_credentials AS l WHERE "
                     "l.installation_ref=f.installation_ref AND l.account_ref=f.account_ref)",
                 std::move(b)});
        }
        return MutationResult::staged;
    });
}
MutationResult
SqliteAccountSessionUnit::replace_password_upgrade(const PasswordReplacement &proposal,
                                                   std::string_view revision,
                                                   const CompleteLocalCredentialFacts &expected) {
    Input n(limits_, true);
    n.ref(proposal.account_ref, 256);
    n.ref(proposal.login_attempt_ref, 256);
    n.ref(proposal.expected_revision_ref, 256);
    n.verifier(proposal.expected_encoded_verifier);
    n.verifier(proposal.replacement_encoded_verifier);
    n.ref(revision, 256);
    n.add(expected);
    shape(expected.local);
    shape(expected.account);
    shape(expected.installation);
    if (!filled({proposal.account_ref, proposal.login_attempt_ref, proposal.expected_revision_ref,
                 revision, expected.snapshot_ref}) ||
        !verifier(proposal.replacement_encoded_verifier) ||
        revision == expected.local.credential.revision_ref ||
        proposal.account_ref != expected.local.credential.account_ref ||
        expected.local.installation_ref != expected.account.current.installation_ref ||
        expected.local.installation_ref != expected.installation.current.installation_ref ||
        expected.local.credential.account_ref != expected.account.current.account_ref)
        input(InputError::invalid_input);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        const auto old = a.local_at(expected.local.installation_ref, proposal.account_ref);
        if (!old || !same(old->local, expected.local) || !same(old->account, expected.account) ||
            !same(old->installation, expected.installation) ||
            !old->account.current.application_access_enabled ||
            proposal.expected_revision_ref != old->local.credential.revision_ref ||
            proposal.expected_encoded_verifier != old->local.credential.encoded_verifier)
            return MutationResult::stale;
        // Caller has checked still-accepted actual local login in this same scope.
        auto b = fences(expected.account, expected.installation);
        b.append(blob(revision));
        b.append(blob(proposal.replacement_encoded_verifier));
        b.append(blob(proposal.expected_revision_ref));
        b.append(blob(proposal.expected_encoded_verifier));
        a.change({"WITH " + fence_cte +
                      ", n(revision,verifier) AS (VALUES(?,?)), e(revision,verifier) AS "
                      "(VALUES(?,?)) UPDATE main.subathon_local_credentials AS l SET "
                      "(revision_ref,encoded_verifier)=(SELECT revision,verifier FROM n) WHERE "
                      "l.verifier_format=1 AND EXISTS(SELECT 1 FROM f,e WHERE "
                      "l.installation_ref=f.installation_ref AND l.account_ref=f.account_ref AND "
                      "l.revision_ref=e.revision AND l.encoded_verifier=e.verifier) AND " +
                      fence,
                  std::move(b)});
        return MutationResult::staged;
    });
}
MutationResult
SqliteAccountSessionUnit::insert_session(const SessionRecordFacts &v, SessionRetention retention,
                                         const AcceptedSessionAuthentication &authentication) {
    Input n(limits_, true);
    n.add(v);
    n.fixed(1);
    n.add(authentication);
    shape(v);
    shape(retention);
    shape(authentication);
    if (v.state != SessionRecordState::active ||
        v.lifetime_started_utc_ms != v.authenticated_utc_ms)
        input(InputError::invalid_input);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        const auto parents = a.account_at(v.installation_ref, v.account_ref);
        // These probes validate complete selected collisions, not just row counts.
        if (a.identity(v.session_ref))
            return MutationResult::collision;
        if (a.digest(v.installation_ref, v.digest))
            return MutationResult::collision;
        if (!parents || !accepted(v, parents->account, parents->installation, authentication) ||
            !authenticator(a, authentication))
            return MutationResult::stale;
        auto b = bindings(v, retention);
        b.append(fences(parents->account, parents->installation));
        auto auth = QString{};
        if (authentication.method == AcceptedSignInMethod::local_password) {
            auth = local_auth_fence;
            b.append(blob(authentication.authenticator_revision_ref));
        }
        a.change({"WITH n(" + record_names + ") AS (VALUES(" + record_slots + ")), " + fence_cte +
                      " INSERT INTO main.subathon_account_sessions SELECT " + record_projection +
                      " FROM n WHERE " + fence + auth +
                      " AND NOT EXISTS(SELECT 1 FROM main.subathon_account_sessions AS s WHERE "
                      "s.session_ref=n.session_ref) AND NOT EXISTS(SELECT 1 FROM "
                      "main.subathon_account_sessions AS s WHERE "
                      "s.installation_ref=n.installation_ref AND s.digest=n.digest)",
                  std::move(b)});
        return MutationResult::staged;
    });
}
MutationResult SqliteAccountSessionUnit::replace_session(
    const SessionRecordFacts &expected, const SessionRecordFacts &replacement,
    SessionRetention retention, const AcceptedSessionAuthentication *authentication) {
    Input n(limits_, true);
    n.add(expected);
    n.add(replacement);
    n.fixed(2);
    if (authentication)
        n.add(*authentication);
    shape(expected);
    shape(replacement);
    shape(retention);
    if (authentication)
        shape(*authentication);
    if (expected.state != SessionRecordState::active ||
        replacement.state != SessionRecordState::active ||
        expected.installation_ref != replacement.installation_ref ||
        expected.account_ref != replacement.account_ref ||
        expected.session_ref != replacement.session_ref ||
        expected.installation_epoch_ref != replacement.installation_epoch_ref ||
        expected.account_epoch_ref != replacement.account_epoch_ref ||
        expected.record_revision_ref == replacement.record_revision_ref ||
        (!authentication &&
         (expected.lifetime_started_utc_ms != replacement.lifetime_started_utc_ms ||
          expected.authenticated_utc_ms != replacement.authenticated_utc_ms ||
          expected.expires_utc_ms != replacement.expires_utc_ms)) ||
        (authentication &&
         (replacement.lifetime_started_utc_ms != replacement.authenticated_utc_ms ||
          replacement.authenticated_utc_ms != authentication->accepted_utc_ms)))
        input(InputError::invalid_input);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        const auto old =
            a.selected(expected.installation_ref, expected.account_ref, expected.session_ref);
        if (!old || !same(old->record, expected) || !current(*old) ||
            (!authentication && old->retention != retention))
            return MutationResult::stale;
        if (authentication &&
            (!accepted(replacement, old->account, old->installation, *authentication) ||
             !authenticator(a, *authentication)))
            return MutationResult::stale;
        if (a.digest(replacement.installation_ref, replacement.digest))
            return MutationResult::collision;
        auto b = bindings(expected, old->retention);
        b.append(bindings(replacement, retention));
        b.append(fences(old->account, old->installation));
        auto auth = QString{};
        if (authentication && authentication->method == AcceptedSignInMethod::local_password) {
            auth = local_auth_fence;
            b.append(blob(authentication->authenticator_revision_ref));
        }
        a.change({"WITH e(" + record_names + ") AS (VALUES(" + record_slots + ")), n(" +
                      record_names + ") AS (VALUES(" + record_slots + ")), " + fence_cte +
                      " UPDATE main.subathon_account_sessions AS s SET "
                      "(record_revision_ref,digest,retention,lifetime_started_utc_ms,authenticated_"
                      "utc_ms,expires_utc_ms)=(SELECT "
                      "record_revision_ref,digest,retention,lifetime_started_utc_ms,authenticated_"
                      "utc_ms,expires_utc_ms FROM n) WHERE EXISTS(SELECT 1 FROM e WHERE " +
                      record_equal + ") AND " + fence + auth,
                  std::move(b)});
        return MutationResult::staged;
    });
}
MutationResult SqliteAccountSessionUnit::revoke_session(const SessionRecordFacts &expected,
                                                        std::string_view revision) {
    Input n(limits_, true);
    n.add(expected);
    n.ref(revision);
    shape(expected);
    if (revision.empty() || revision == expected.record_revision_ref ||
        expected.state != SessionRecordState::active)
        input(InputError::invalid_input);
    return writing([&] {
        Access a(session_, limits_, scope_, read_ordinal_, read_ordinal_exhausted_);
        const auto old =
            a.selected(expected.installation_ref, expected.account_ref, expected.session_ref);
        if (!old || !same(old->record, expected))
            return MutationResult::stale;
        auto b = bindings(expected, old->retention);
        b.append(blob(revision));
        a.change({"WITH e(" + record_names + ") AS (VALUES(" + record_slots +
                      ")) UPDATE main.subathon_account_sessions AS s SET "
                      "state=1,record_revision_ref=? WHERE EXISTS(SELECT 1 FROM e WHERE " +
                      record_equal + ")",
                  std::move(b)});
        return MutationResult::staged;
    });
}
} // namespace subathon::server
