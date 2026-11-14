// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sqlite_account_sessions_fixture.hpp"
#include <algorithm>
#include <subathon/server/sqlite_permission_graph.hpp>
#include <tuple>

namespace permission_graph_fixture {
using namespace subathon::server;
using namespace subathon::core;
using account_sessions_fixture::blob;
using account_sessions_fixture::Cleanup;
using account_sessions_fixture::corrupt;
using account_sessions_fixture::Observer;
using account_sessions_fixture::run;
using account_sessions_fixture::scenario;
using qt_test_support::require;
// Independent proposed step9 SQL, never copied prefix1-8. Keep byte spelling in
// sync with the schema contract before review; freeze it when the migration ships.
inline MigrationStep step9(const MigrationManifest &prefix) {
    MigrationStep step;
    step.version = 9;
    step.id = "permission-graph-v1";
    step.sql = {
        "CREATE TABLE subathon_permission_head(installation_ref BLOB NOT NULL PRIMARY KEY "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), revision BLOB "
        "NOT NULL CHECK(typeof(revision)='blob' AND length(revision)=8 AND "
        "revision<>X'0000000000000000'), owner_ref BLOB NOT NULL "
        "CHECK(typeof(owner_ref)='blob' AND length(owner_ref)>0), catalog BLOB NOT NULL "
        "CHECK(typeof(catalog)='blob' AND length(catalog)>0), "
        "FOREIGN KEY(installation_ref) REFERENCES subathon_account_installation(installation_ref), "
        "FOREIGN KEY(installation_ref,owner_ref) REFERENCES "
        "subathon_accounts(installation_ref,account_ref))",
        "CREATE TABLE subathon_permission_roles(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), channel INTEGER "
        "NOT NULL CHECK(typeof(channel)='integer' AND channel IN (0,1)), role_ref BLOB NOT NULL "
        "CHECK(typeof(role_ref)='blob' AND length(role_ref)>0), entries BLOB NOT NULL "
        "CHECK(typeof(entries)='blob' AND length(entries)>=8), "
        "PRIMARY KEY(installation_ref,channel,role_ref), FOREIGN KEY(installation_ref) "
        "REFERENCES subathon_permission_head(installation_ref))",
        "CREATE TABLE subathon_permission_accounts(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), channel INTEGER "
        "NOT NULL CHECK(typeof(channel)='integer' AND channel IN (0,1)), account_ref BLOB NOT NULL "
        "CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), memberships BLOB NOT NULL "
        "CHECK(typeof(memberships)='blob' AND length(memberships)>=8), entries BLOB NOT NULL "
        "CHECK(typeof(entries)='blob' AND length(entries)>=8), "
        "PRIMARY KEY(installation_ref,channel,account_ref), FOREIGN KEY(installation_ref) "
        "REFERENCES subathon_permission_head(installation_ref), "
        "FOREIGN KEY(installation_ref,account_ref) REFERENCES "
        "subathon_accounts(installation_ref,account_ref))"};
    step.expected_schema = prefix.steps.back().expected_schema;
    const QStringList names{"subathon_permission_head", "subathon_permission_roles",
                            "subathon_permission_accounts"};
    for (qsizetype i = 0; i < names.size(); ++i) {
        step.expected_schema.append({"table", names[i], names[i], step.sql[i]});
        step.expected_schema.append(
            {"index", "sqlite_autoindex_" + names[i] + "_1", names[i], std::nullopt});
    }
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return std::tie(a.type, a.name) < std::tie(b.type, b.name);
              });
    return step;
}
inline MigrationManifest proposed_manifest() {
    auto m = account_session_repository_manifest();
    m.steps.append(step9(m));
    m.supported_starting_versions.append(9);
    return m;
}
inline MigrationResult migrate(SqliteWriter &writer, MigrationManifest manifest,
                               MigrationMode mode = MigrationMode::advance) {
    struct State {
        bool done = false;
        MigrationResult result;
    };
    const auto state = std::make_shared<State>();
    const auto admission = SqliteMigrationRunner::enqueue(
        writer, {mode, std::move(manifest), {}, {}}, [state](auto result) {
            state->result = std::move(result);
            state->done = true;
        });
    require(admission.error == StorageError::none && admission.reason == MigrationReason::none,
            "Real compiled-prefix migration admitted");
    require(QTest::qWaitFor([state] { return state->done; }, 10000),
            "Migration actually completes");
    return state->result;
}
inline PermissionGraphTrust trust() {
    PermissionGraphTrust t;
    t.catalog = {
        PermissionFactsState::complete,
        "permission-reference-v1",
        {{"reference.time-add.v1",
          PermissionOperationClass::ordinary,
          {"installation", "timer", "source"},
          {{"adjustment", "ms", PermissionNumberKind::integer, PermissionRational{"0", "1"},
            PermissionRational{"9223372036854775807", "1"}, 15}},
          std::nullopt},
         {"reference.application-read.v1",
          PermissionOperationClass::ordinary,
          {"installation", "timer", "source"},
          {},
          std::nullopt},
         {"reference.hold-read.v1",
          PermissionOperationClass::ordinary,
          {"installation", "timer", "source"},
          {},
          std::nullopt},
         {"roles.edit", PermissionOperationClass::ordinary, {"role"}, {}, std::nullopt},
         {"members.edit", PermissionOperationClass::ordinary, {"account"}, {}, std::nullopt},
         {"overrides.edit", PermissionOperationClass::ordinary, {"account"}, {}, std::nullopt},
         {"access.manage",
          PermissionOperationClass::protected_access_management,
          {},
          {},
          std::nullopt},
         {"owner.recover", PermissionOperationClass::protected_recovery, {}, {}, std::nullopt},
         {"helper.reset",
          PermissionOperationClass::helper_password_reset,
          {"target"},
          {},
          "target"},
         {"timer.snapshot.read", PermissionOperationClass::ordinary, {"timer"}, {}, std::nullopt}}};
    t.role_edit = {"roles.edit", "role"};
    t.membership_edit = {"members.edit", "account"};
    t.override_edit = {"overrides.edit", "account"};
    return t;
}
inline PermissionPolicyEntry allowance(std::string source = "s",
                                       std::string operation = "reference.time-add.v1") {
    PermissionPolicyEntry e;
    e.source = std::move(source);
    e.grant = "g";
    e.operation = std::move(operation);
    e.resources = {{"installation", PermissionResourceMode::allowed_set, {"I"}},
                   {"timer", PermissionResourceMode::allowed_set, {"T"}},
                   {"source", PermissionResourceMode::allowed_set, {"S"}}};
    if (e.operation == "reference.time-add.v1")
        e.quantities = {{"adjustment", "ms", PermissionQuantityMode::at_most, std::nullopt,
                         PermissionRational{"60000", "1"}}};
    return e;
}
inline PermissionEditableState empty_graph() {
    return {PermissionFactsState::complete, {}, {{"A", {}, {}}, {"O", {}, {}}}};
}
inline PermissionEditableState use_graph() {
    auto g = empty_graph();
    auto row = allowance();
    row.origin = PermissionOrigin::role;
    row.role = "R";
    g.roles = {{"R", {row}}};
    g.accounts[0].assigned_roles = {"R"};
    g.accounts[1].individual_entries = {allowance("owner-use", "reference.application-read.v1")};
    return g;
}
inline PermissionEditableState grant_graph() {
    auto g = empty_graph();
    g.accounts[0].individual_entries = {allowance("delegation")};
    return g;
}
// Test-side full field encoder also serves as deep equality; no production codec
// or expected-output generation is used. Framing is the physical contract, not ABI.
inline void encode(QByteArray &b, std::uint64_t n) {
    for (int shift = 56; shift >= 0; shift -= 8)
        b.append(char((n >> shift) & 255));
}
inline void encode(QByteArray &b, const std::string &v) {
    encode(b, std::uint64_t(v.size()));
    b.append(blob(v));
}
template <class T> void encode(QByteArray &, const std::optional<T> &);
template <class T> void encode(QByteArray &, const std::vector<T> &);
inline void encode(QByteArray &b, PermissionFactsState v) { b.append(char(v)); }
inline void encode(QByteArray &b, const PermissionRational &v) {
    encode(b, v.numerator);
    encode(b, v.denominator);
}
inline void encode(QByteArray &b, const PermissionResourceConstraint &v) {
    encode(b, v.slot);
    b.append(char(v.mode));
    encode(b, v.allowed);
}
inline void encode(QByteArray &b, const PermissionQuantityConstraint &v) {
    encode(b, v.slot);
    encode(b, v.unit);
    b.append(char(v.mode));
    encode(b, v.lower);
    encode(b, v.upper);
}
inline void encode(QByteArray &b, const PermissionPolicyEntry &v) {
    b.append(char(v.origin));
    encode(b, v.source);
    encode(b, v.role);
    encode(b, v.operation);
    b.append(char(v.effect));
    encode(b, v.grant);
    encode(b, v.resources);
    encode(b, v.quantities);
}
inline void encode(QByteArray &b, const PermissionQuantityDefinition &v) {
    encode(b, v.slot);
    encode(b, v.unit);
    b.append(char(v.kind));
    encode(b, v.minimum);
    encode(b, v.maximum);
    b.append(char(v.allowed_modes));
}
inline void encode(QByteArray &b, const PermissionOperationDefinition &v) {
    encode(b, v.operation);
    b.append(char(v.classification));
    encode(b, v.resource_slots);
    encode(b, v.quantity_slots);
    encode(b, v.helper_target_slot);
}
inline void encode(QByteArray &b, const PermissionCatalog &v) {
    encode(b, v.state);
    encode(b, v.revision);
    encode(b, v.operations);
}
inline void encode(QByteArray &b, const PermissionManagementBinding &v) {
    encode(b, v.operation);
    encode(b, v.target_slot);
}
inline void encode(QByteArray &b, const PermissionGraphTrust &v) {
    encode(b, v.catalog);
    encode(b, v.role_edit);
    encode(b, v.membership_edit);
    encode(b, v.override_edit);
}
inline void encode(QByteArray &b, const PermissionEditableRole &v) {
    encode(b, v.role);
    encode(b, v.entries);
}
inline void encode(QByteArray &b, const PermissionEditableAccount &v) {
    encode(b, v.account);
    encode(b, v.assigned_roles);
    encode(b, v.individual_entries);
}
inline void encode(QByteArray &b, const PermissionEditableState &v) {
    encode(b, v.state);
    encode(b, v.roles);
    encode(b, v.accounts);
}
template <class T> void encode(QByteArray &b, const std::optional<T> &v) {
    b.append(v ? '\1' : '\0');
    if (v)
        encode(b, *v);
}
template <class T> void encode(QByteArray &b, const std::vector<T> &v) {
    encode(b, std::uint64_t(v.size()));
    for (const auto &x : v)
        encode(b, x);
}
template <class T> QByteArray encoded(const T &v) {
    QByteArray b;
    encode(b, v);
    return b;
}
struct Database {
    QTemporaryDir directory;
    QString path{directory.filePath("permission.sqlite")};
    SqliteWriter writer;
    PermissionGraphTrust trusted{trust()};
    explicit Database(bool with_head = true) {
        require(directory.isValid(), "Disposable permission database");
        require(writer.open(path, account_sessions_fixture::limits()) == StorageError::none,
                "Real writer opens");
        const auto migration = migrate(writer, proposed_manifest());
        require(migration.reason == MigrationReason::none && migration.report &&
                    migration.report->version == 9 &&
                    migration.work.transaction == TransactionOutcome::committed &&
                    migration.work.error == WorkError::none,
                "Real migration runner installs compiled prefix plus independent proposed step9");
        (void)run(writer, [&](auto context) -> QVariant {
            const auto s = context.begin();
            Cleanup cleanup{s};
            SqliteAccountSessionUnit accounts(s, {{}});
            require(accounts.insert_installation({{"I", "E"}, "J"}) == LifecycleWriteResult::staged,
                    "Existing installation API");
            for (const auto &id : {"A", "O"})
                require(accounts.insert_account({{"I", id, "F", true}, "K"}) ==
                            LifecycleWriteResult::staged,
                        "Existing account API, synthetic identities only");
            if (with_head) {
                SqlitePermissionGraphUnit unit(s, "I", trusted);
                require(unit.bootstrap("O", use_graph(), grant_graph()) ==
                            PermissionGraphWrite::staged,
                        "Initial graph staged");
            }
            s.commit();
            return 1;
        });
    }
    template <class Body> void work(Body body) {
        (void)run(writer, [&](auto context) -> QVariant {
            const auto s = context.begin();
            Cleanup cleanup{s};
            body(s);
            s.commit();
            return 1;
        });
    }
    StoredPermissionGraph load(PermissionGraphLimits limits = {}) {
        StoredPermissionGraph result;
        work([&](const auto &s) {
            SqlitePermissionGraphUnit unit(s, "I", trusted, limits);
            const auto value = unit.load();
            require(std::holds_alternative<StoredPermissionGraph>(value),
                    "Complete graph, never an empty fallback");
            result = std::get<StoredPermissionGraph>(value);
        });
        return result;
    }
};
template <class F> void input_error(F f, PermissionGraphInputError expected) {
    bool reached = false;
    try {
        f();
    } catch (const PermissionGraphInputFailure &e) {
        reached = true;
        require(e.code() == expected, "Typed input refusal");
    }
    require(reached, "Input refusal observed, NI never accepted");
}
template <class F> void work_error(F f, WorkError expected) {
    bool reached = false;
    try {
        f();
    } catch (const SqliteWorkFailure &e) {
        reached = true;
        require(e.error() == expected, "Actual writer refusal");
    }
    require(reached, "Writer refusal observed, NI never accepted");
}
inline void same(const StoredPermissionGraph &v, std::uint64_t revision,
                 const PermissionEditableState &use = use_graph(),
                 const PermissionEditableState &grant = grant_graph()) {
    require(v.installation == "I" && v.revision == revision &&
                v.owner.state == PermissionOwnerState::present && v.owner.account == "O",
            "Exact current head and protected owner");
    require(encoded(v.catalog) == encoded(trust().catalog) && encoded(v.use) == encoded(use) &&
                encoded(v.grant) == encoded(grant),
            "Every graph/catalog field losslessly retained, separate channels");
    require(v.current_installation.current.installation_ref == "I" &&
                v.current_installation.current.session_epoch_ref == "E" &&
                v.current_installation.revision_ref == "J" && v.current_accounts.size() == 2,
            "Existing source closure");
    for (std::size_t i = 0; i < 2; ++i) {
        const auto &a = v.current_accounts[i];
        require(a.current.installation_ref == "I" &&
                    a.current.account_ref == (i == 0 ? "A" : "O") &&
                    a.current.session_epoch_ref == "F" && a.current.application_access_enabled &&
                    a.revision_ref == "K",
                "All current account fields retained");
    }
}
} // namespace permission_graph_fixture
