// SPDX-License-Identifier: GPL-3.0-or-later
#include "permission_graph_sql.hpp"
#include <subathon/core/permission_graph_validation.hpp>
#include <tuple>

namespace subathon::server {
namespace {
using namespace permission_graph;
namespace validation = core::internal;
[[noreturn]] void input(PermissionGraphInputError e) { throw PermissionGraphInputFailure(e); }
void limits_valid(const PermissionGraphLimits &l) {
    const auto cap =
        std::min({std::uint64_t(SIZE_MAX), std::uint64_t(PTRDIFF_MAX),
                  std::uint64_t(std::numeric_limits<qsizetype>::max()), std::uint64_t(INT64_MAX)});
    for (auto n : {l.reference_bytes, l.component_bytes, l.rows, l.items, l.selected_bytes,
                   l.mutation_bytes})
        if (!n || n > cap)
            input(PermissionGraphInputError::invalid_limits);
    for (auto n : {l.reference_bytes, l.component_bytes, l.rows, l.items})
        if (n == cap)
            input(PermissionGraphInputError::invalid_limits);
    if (l.selected_bytes > (cap - l.mutation_bytes) / 3)
        input(PermissionGraphInputError::invalid_limits);
}
template <class F> auto input_guard(F f) {
    try {
        return f();
    } catch (const PermissionGraphStorageFailure &e) {
        input(e.code() == Failure::resource_limit ? PermissionGraphInputError::resource_limit
                                                  : PermissionGraphInputError::invalid_input);
    } catch (const core::EffectivePermissionFailure &e) {
        input(e.code() == core::EffectivePermissionError::allocation_failure
                  ? PermissionGraphInputError::resource_limit
                  : PermissionGraphInputError::invalid_input);
    } catch (const std::bad_alloc &) {
        input(PermissionGraphInputError::resource_limit);
    } catch (const std::length_error &) {
        input(PermissionGraphInputError::resource_limit);
    }
}
template <class F> auto storage_guard(F f) {
    try {
        return f();
    } catch (const core::EffectivePermissionFailure &e) {
        fail(e.code() == core::EffectivePermissionError::allocation_failure
                 ? Failure::resource_limit
                 : Failure::invalid_data);
    } catch (const std::bad_alloc &) {
        fail(Failure::resource_limit);
    } catch (const std::length_error &) {
        fail(Failure::resource_limit);
    }
}
void trust_shape(const PermissionGraphTrust &t) {
    validation::validate_permission_graph_catalog(t.catalog);
    for (const auto *b : {&t.role_edit, &t.membership_edit, &t.override_edit}) {
        const auto op = std::find_if(t.catalog.operations.begin(), t.catalog.operations.end(),
                                     [&](const auto &v) { return v.operation == b->operation; });
        data(op != t.catalog.operations.end() && op->resource_slots.size() == 1 &&
             op->resource_slots[0] == b->target_slot && op->quantity_slots.empty());
    }
}
bool entries_equal(const std::vector<PermissionPolicyEntry> &a,
                   const std::vector<PermissionPolicyEntry> &b) {
    return a.size() == b.size() && std::all_of(a.begin(), a.end(), [&](const auto &x) {
               return std::any_of(b.begin(), b.end(), [&](const auto &y) {
                   return validation::permission_graph_entry_equal(x, y);
               });
           });
}
bool memberships_equal(const std::vector<std::string> &a, const std::vector<std::string> &b) {
    return a.size() == b.size() && std::all_of(a.begin(), a.end(), [&](const auto &x) {
               return std::find(b.begin(), b.end(), x) != b.end();
           });
}
void roster(const PermissionEditableState &s, const std::vector<StoredAccountFacts> &accounts) {
    if (s.accounts.size() != accounts.size() ||
        !std::all_of(s.accounts.begin(), s.accounts.end(), [&](const auto &a) {
            return std::any_of(accounts.begin(), accounts.end(),
                               [&](const auto &b) { return a.account == b.current.account_ref; });
        }))
        input(PermissionGraphInputError::invalid_input);
}
} // namespace

PermissionGraphTrust reference_permission_graph_trust() {
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
MigrationManifest permission_graph_repository_manifest() {
    auto result = account_session_repository_manifest();
    MigrationStep step;
    step.version = 9;
    step.id = "permission-graph-v1";
    step.sql = {
        "CREATE TABLE subathon_permission_head(installation_ref BLOB NOT NULL PRIMARY KEY "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), revision BLOB NOT "
        "NULL CHECK(typeof(revision)='blob' AND length(revision)=8 AND "
        "revision<>X'0000000000000000'), owner_ref BLOB NOT NULL CHECK(typeof(owner_ref)='blob' "
        "AND length(owner_ref)>0), catalog BLOB NOT NULL CHECK(typeof(catalog)='blob' AND "
        "length(catalog)>0), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_account_installation(installation_ref), FOREIGN KEY(installation_ref,owner_ref) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
        "CREATE TABLE subathon_permission_roles(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), channel INTEGER "
        "NOT NULL CHECK(typeof(channel)='integer' AND channel IN (0,1)), role_ref BLOB NOT NULL "
        "CHECK(typeof(role_ref)='blob' AND length(role_ref)>0), entries BLOB NOT NULL "
        "CHECK(typeof(entries)='blob' AND length(entries)>=8), PRIMARY "
        "KEY(installation_ref,channel,role_ref), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_permission_head(installation_ref))",
        "CREATE TABLE subathon_permission_accounts(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), channel INTEGER "
        "NOT NULL CHECK(typeof(channel)='integer' AND channel IN (0,1)), account_ref BLOB NOT NULL "
        "CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), memberships BLOB NOT NULL "
        "CHECK(typeof(memberships)='blob' AND length(memberships)>=8), entries BLOB NOT NULL "
        "CHECK(typeof(entries)='blob' AND length(entries)>=8), PRIMARY "
        "KEY(installation_ref,channel,account_ref), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_permission_head(installation_ref), FOREIGN KEY(installation_ref,account_ref) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
    };
    step.expected_schema = result.steps.back().expected_schema;
    const std::array<QString, 3> names{"subathon_permission_head", "subathon_permission_roles",
                                       "subathon_permission_accounts"};
    for (std::size_t i = 0; i < names.size(); ++i) {
        step.expected_schema.append({"table", names[i], names[i], step.sql[qsizetype(i)]});
        step.expected_schema.append(
            {"index", "sqlite_autoindex_" + names[i] + "_1", names[i], std::nullopt});
    }
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return std::tie(a.type, a.name) < std::tie(b.type, b.name);
              });
    result.steps.append(std::move(step));
    result.supported_starting_versions.append(9);
    return result;
}

class SqlitePermissionGraphUnit::Impl {
  public:
    // The revocable weak session handle is copied, including from a temporary.
    SqliteWorkSession session;
    std::string installation;
    const PermissionGraphTrust &trust;
    PermissionGraphLimits limits;
    Impl(const SqliteWorkSession &s, std::string_view i, const PermissionGraphTrust &t,
         PermissionGraphLimits l)
        : session(s), installation(i), trust(t), limits(l) {}
    template <class... T> void preflight(const T &...v) const {
        input_guard([&] {
            Projection p{limits, limits.mutation_bytes};
            p(v...);
            data(!p.empty);
        });
    }
    void graph_shape(const PermissionEditableState &s) const {
        input_guard([&] { validation::validate_permission_graph(trust.catalog, s); });
    }
    // revision==0 is PRIVATE proven absence, retaining the validated account
    // closure for bootstrap. Public loads return missing instead of this value.
    StoredPermissionGraph source(bool synchronize = false) const {
        const auto meta = metadata(session, installation, limits);
        const auto rows = payload(session, meta);
        std::array<std::size_t, 5> counts{};
        const SqlRow *head = nullptr, *install = nullptr;
        for (std::size_t i = 0; i < meta.size(); ++i) {
            ++counts[meta[i].table];
            if (meta[i].table == 0)
                head = &rows[i];
            if (meta[i].table == 3)
                install = &rows[i];
            // Every selected key remains byte-exact within this scope.
            data(view(rows[i], meta[i].table == 3 ? 1 : 0) == installation);
        }
        data(counts[0] <= 1 && counts[3] == 1);
        data(head || (!counts[1] && !counts[2]));
        space(counts[4] <= std::vector<StoredAccountFacts>{}.max_size());
        space(counts[4] <= std::vector<PermissionEditableAccount>{}.max_size());
        space(counts[1] <= std::vector<PermissionEditableRole>{}.max_size());
        Projection out{limits, limits.selected_bytes};
        out(std::string_view(installation));
        out.fixed(8 + 2 + 2); // revision, owner tags, channel states
        if (head)
            out(view(*head, 2));
        // Current installation and roster, and both synthesized account identities.
        out(view(*install, 1), view(*install, 2), view(*install, 3));
        out.elements(counts[4]);
        out.elements(counts[4]);
        out.elements(counts[4]);
        for (std::size_t i = 0; i < meta.size(); ++i) {
            const auto &r = rows[i];
            if (meta[i].table == 4) {
                out(view(r, 0), view(r, 1), view(r, 3), view(r, 4));
                out.fixed(1);
                out(view(r, 1), view(r, 1));
            }
            if (meta[i].table == 1) {
                out.elements(1);
                out(view(r, 2));
                Decoder scan{view(r, 3), out, false};
                (void)scan.read<std::vector<PermissionPolicyEntry>>();
            }
            if (meta[i].table == 2) {
                Decoder memberships{view(r, 3), out, false};
                (void)memberships.read<std::vector<std::string>>();
                Decoder entries{view(r, 4), out, false};
                (void)entries.read<std::vector<PermissionPolicyEntry>>();
            }
        }
        // Validate/project the ENTIRE encoded trust independently, but only the
        // catalog is part of StoredPermissionGraph's complete result projection.
        if (head) {
            Projection p{limits, limits.selected_bytes};
            Decoder scan{view(*head, 3), p, false};
            PermissionGraphTrust value;
            scan.one(value.catalog);
            out.fixed(p.bytes);
            out.elements(p.items);
            scan(value.role_edit, value.membership_edit, value.override_edit);
            scan.finish();
            data(!p.empty);
        }
        data(!out.empty);
        // All output strings/vectors, including empty-account synthesis, are now
        // admitted. Only this point begins owning the decoded result.
        StoredPermissionGraph result;
        result.installation = installation;
        result.current_installation = {{text(*install, 1), text(*install, 2)}, text(*install, 3)};
        result.use.state = result.grant.state = PermissionFactsState::complete;
        for (std::size_t i = 0; i < meta.size(); ++i)
            if (meta[i].table == 4) {
                const auto &r = rows[i];
                result.current_accounts.push_back(
                    {{text(r, 0), text(r, 1), text(r, 3), r[2].toLongLong() != 0}, text(r, 4)});
            }
        std::sort(result.current_accounts.begin(), result.current_accounts.end(),
                  [](const auto &a, const auto &b) {
                      return byte_less(a.current.account_ref, b.current.account_ref);
                  });
        for (const auto &a : result.current_accounts) {
            if (result.use.accounts.size())
                data(result.use.accounts.back().account != a.current.account_ref);
            result.use.accounts.push_back({a.current.account_ref, {}, {}});
            result.grant.accounts.push_back({a.current.account_ref, {}, {}});
        }
        std::array<std::vector<std::string_view>, 2> seen;
        for (std::size_t i = 0; i < meta.size(); ++i) {
            const auto &r = rows[i];
            if (meta[i].table != 1 && meta[i].table != 2)
                continue;
            auto &graph = r[1].toLongLong() == 0 ? result.use : result.grant;
            if (meta[i].table == 1) {
                graph.roles.push_back({text(r, 2), decode<std::vector<PermissionPolicyEntry>>(
                                                       r[3].toByteArray(), limits)});
            } else {
                const auto account = view(r, 2);
                auto &ids = seen[r[1].toLongLong() == 0 ? 0 : 1];
                data(std::find(ids.begin(), ids.end(), account) == ids.end());
                space(ids.size() < ids.max_size());
                ids.push_back(account);
                const auto a = std::find_if(graph.accounts.begin(), graph.accounts.end(),
                                            [&](const auto &v) { return v.account == account; });
                data(a != graph.accounts.end());
                a->assigned_roles = decode<std::vector<std::string>>(r[3].toByteArray(), limits);
                a->individual_entries =
                    decode<std::vector<PermissionPolicyEntry>>(r[4].toByteArray(), limits);
            }
        }
        for (auto *g : {&result.use, &result.grant})
            std::sort(g->roles.begin(), g->roles.end(),
                      [](const auto &a, const auto &b) { return byte_less(a.role, b.role); });
        if (!head)
            return result;
        const auto rev = (*head)[1].toByteArray();
        for (auto c : rev)
            result.revision = (result.revision << 8) | static_cast<unsigned char>(c);
        data(result.revision != 0);
        result.owner = {PermissionOwnerState::present, text(*head, 2)};
        data(std::any_of(
            result.current_accounts.begin(), result.current_accounts.end(),
            [&](const auto &a) { return a.current.account_ref == result.owner.account; }));
        auto stored_trust = decode<PermissionGraphTrust>((*head)[3].toByteArray(), limits);
        trust_shape(stored_trust);
        validation::validate_permission_graph(stored_trust.catalog, result.use);
        validation::validate_permission_graph(stored_trust.catalog, result.grant);
        if (stored_trust.catalog.revision == trust.catalog.revision) {
            // Compare the exact trusted encoding without allocating a fourth
            // variable-payload projection alongside driver rows and decoded DTOs.
            Encoder comparison{view(*head, 3)};
            comparison(trust);
            comparison.finish();
        } else if (!synchronize)
            fail(Failure::unavailable);
        if (synchronize) {
            validation::validate_permission_graph(trust.catalog, result.use);
            validation::validate_permission_graph(trust.catalog, result.grant);
        }
        result.catalog = std::move(stored_trust.catalog);
        return result;
    }
    using Plan = std::vector<SqlStatement>;
    struct WritePlan {
        const Impl &unit;
        Projection projection;
        Plan statements;
        explicit WritePlan(const Impl &u)
            : unit(u), projection{u.limits, u.limits.selected_bytes} {}
        template <class... T> void raw(const T &...v) { projection(v...); }
        template <class... T> void encoded(const T &...v) {
            projection.framed = true;
            projection(v...);
            projection.framed = false;
        }
        void append(SqlStatement s) {
            space(statements.size() < statements.max_size());
            statements.push_back(std::move(s));
        }
        // Two traversals: the first projects ALL binding bytes/items before any
        // statement/binding/payload ownership, the second constructs exactly them.
        void channel(const PermissionEditableState &g, int ch, bool own) {
            for (const auto &r : g.roles) {
                if (!own) {
                    raw(unit.installation, std::uint64_t(ch), r.role);
                    encoded(r.entries);
                    continue;
                }
                append({"INSERT INTO main.subathon_permission_roles VALUES(?,?,?,?)",
                        {blob(unit.installation), qint64(ch), blob(r.role),
                         encode(r.entries, unit.limits)}});
            }
            for (const auto &a : g.accounts)
                if (!a.assigned_roles.empty() || !a.individual_entries.empty()) {
                    if (!own) {
                        raw(unit.installation, std::uint64_t(ch), a.account);
                        encoded(a.assigned_roles, a.individual_entries);
                        continue;
                    }
                    append({"INSERT INTO main.subathon_permission_accounts VALUES(?,?,?,?,?)",
                            {blob(unit.installation), qint64(ch), blob(a.account),
                             encode(a.assigned_roles, unit.limits),
                             encode(a.individual_entries, unit.limits)}});
                }
        }
    };
    void changed(const SqlStatement &probe) const {
        const auto r = session.execute(probe);
        data(r.size() == 1 && r[0].size() == 1 && integer(r[0][0]) && r[0][0].toLongLong() == 1);
    }
    PermissionGraphWrite replace(std::uint64_t expected, const PermissionEditableState &after,
                                 int channel) {
        preflight(installation, expected, after);
        if (!expected)
            input(PermissionGraphInputError::invalid_input);
        graph_shape(after);
        return storage_guard([&] {
            const auto before = source();
            roster(after, before.current_accounts);
            if (!before.revision || expected != before.revision)
                return PermissionGraphWrite::stale;
            space(before.revision != UINT64_MAX);
            WritePlan p(*this);
            p.raw(installation, installation, installation, std::uint64_t(0), std::uint64_t(0),
                  expected, expected + 1);
            p.channel(after, channel, false);
            p.append(
                {"UPDATE main.subathon_permission_head SET revision=? WHERE installation_ref=? AND "
                 "revision=?",
                 {encode(expected + 1, limits), blob(installation), encode(expected, limits)}});
            p.append({"DELETE FROM main.subathon_permission_roles WHERE installation_ref=? AND "
                      "channel=?",
                      {blob(installation), qint64(channel)}});
            p.append({"DELETE FROM main.subathon_permission_accounts WHERE installation_ref=? AND "
                      "channel=?",
                      {blob(installation), qint64(channel)}});
            p.channel(after, channel, true);
            const SqlStatement probe{"SELECT changes()", {}};
            (void)session.execute(p.statements[0]);
            changed(probe);
            for (std::size_t i = 1; i < p.statements.size(); ++i)
                (void)session.execute(p.statements[i]);
            return PermissionGraphWrite::staged;
        });
    }
};
SqlitePermissionGraphUnit::SqlitePermissionGraphUnit(const SqliteWorkSession &s, std::string_view i,
                                                     const PermissionGraphTrust &t,
                                                     PermissionGraphLimits l) {
    limits_valid(l);
    input_guard([&] {
        Projection p{l, l.mutation_bytes};
        p(i, t);
        data(!p.empty);
        trust_shape(t);
        impl_ = std::make_unique<Impl>(s, i, t, l);
    });
}
SqlitePermissionGraphUnit::~SqlitePermissionGraphUnit() = default;
PermissionGraphRead<StoredPermissionGraph> SqlitePermissionGraphUnit::load() {
    try {
        return storage_guard([&] {
            auto v = impl_->source();
            if (!v.revision)
                fail(Failure::missing);
            return v;
        });
    } catch (const PermissionGraphStorageFailure &e) {
        return e.code();
    }
}
PermissionGraphRead<PermissionGraphEditFacts>
SqlitePermissionGraphUnit::describe_use_edit(const PermissionEditableState &after) {
    impl_->preflight(impl_->installation, after);
    impl_->graph_shape(after);
    try {
        return storage_guard([&] {
            auto before = impl_->source();
            if (!before.revision)
                fail(Failure::missing);
            roster(after, before.current_accounts);
            // Borrowed intents first: determine the complete result before copying
            // the proposal or any target/operation strings.
            struct Intent {
                const PermissionManagementBinding *binding;
                const std::string *target;
            };
            auto each = [&](auto visit) {
                for (const auto *g :
                     std::array<const PermissionEditableState *, 2>{&before.use, &after}) {
                    const auto &other = g == &after ? before.use : after;
                    for (const auto &r : g->roles) {
                        auto old = std::find_if(other.roles.begin(), other.roles.end(),
                                                [&](const auto &v) { return v.role == r.role; });
                        if (old == other.roles.end() || !entries_equal(r.entries, old->entries))
                            visit(Intent{&impl_->trust.role_edit, &r.role});
                    }
                }
                for (const auto &a : after.accounts) {
                    const auto old =
                        std::find_if(before.use.accounts.begin(), before.use.accounts.end(),
                                     [&](const auto &v) { return v.account == a.account; });
                    if (!memberships_equal(a.assigned_roles, old->assigned_roles))
                        visit(Intent{&impl_->trust.membership_edit, &a.account});
                    if (!entries_equal(a.individual_entries, old->individual_entries))
                        visit(Intent{&impl_->trust.override_edit, &a.account});
                }
            };
            auto less = [](const Intent &a, const Intent &b) {
                if (a.binding->operation != b.binding->operation)
                    return byte_less(a.binding->operation, b.binding->operation);
                if (a.binding->target_slot != b.binding->target_slot)
                    return byte_less(a.binding->target_slot, b.binding->target_slot);
                return byte_less(*a.target, *b.target);
            };
            // Bound the temporary borrowed index before allocation. Duplicate role
            // changes are removed before charging/materializing returned requests.
            std::size_t n = 0;
            each([&](Intent) {
                space(n < std::vector<Intent>{}.max_size());
                ++n;
            });
            std::vector<Intent> intents;
            intents.reserve(n);
            each([&](Intent i) { intents.push_back(i); });
            std::sort(intents.begin(), intents.end(), less);
            intents.erase(std::unique(intents.begin(), intents.end(),
                                      [&](auto a, auto b) { return !less(a, b) && !less(b, a); }),
                          intents.end());
            Projection p{impl_->limits, impl_->limits.selected_bytes};
            p(before, after);
            p.elements(intents.size());
            space(intents.size() <= std::vector<PermissionRequest>{}.max_size());
            for (const auto &i : intents) {
                p(i.binding->operation, i.binding->target_slot, *i.target);
                p.elements(1);
            }
            PermissionGraphEditFacts result{std::move(before), after, {}};
            result.management.reserve(intents.size());
            for (const auto &i : intents)
                result.management.push_back(
                    {i.binding->operation, {{i.binding->target_slot, *i.target}}, {}});
            return result;
        });
    } catch (const PermissionGraphStorageFailure &e) {
        return e.code();
    }
}
PermissionGraphWrite SqlitePermissionGraphUnit::bootstrap(std::string_view owner,
                                                          const PermissionEditableState &use,
                                                          const PermissionEditableState &grant) {
    impl_->preflight(impl_->installation, owner, use, grant);
    impl_->graph_shape(use);
    impl_->graph_shape(grant);
    return storage_guard([&] {
        const auto before = impl_->source();
        if (!std::any_of(before.current_accounts.begin(), before.current_accounts.end(),
                         [&](const auto &a) { return a.current.account_ref == owner; }))
            fail(Failure::invalid_data);
        roster(use, before.current_accounts);
        roster(grant, before.current_accounts);
        if (before.revision)
            return PermissionGraphWrite::collision;
        Impl::WritePlan p(*impl_);
        p.raw(impl_->installation, std::uint64_t(1), owner);
        p.encoded(impl_->trust);
        p.channel(use, 0, false);
        p.channel(grant, 1, false);
        p.append({"INSERT INTO main.subathon_permission_head VALUES(?,?,?,?)",
                  {blob(impl_->installation), encode(std::uint64_t(1), impl_->limits), blob(owner),
                   encode(impl_->trust, impl_->limits)}});
        p.channel(use, 0, true);
        p.channel(grant, 1, true);
        for (const auto &s : p.statements)
            (void)impl_->session.execute(s);
        return PermissionGraphWrite::staged;
    });
}
PermissionGraphWrite SqlitePermissionGraphUnit::replace_use(std::uint64_t expected,
                                                            const PermissionEditableState &after) {
    return impl_->replace(expected, after, 0);
}
PermissionGraphWrite
SqlitePermissionGraphUnit::replace_grant_from_owner(std::uint64_t expected,
                                                    const PermissionEditableState &after) {
    return impl_->replace(expected, after, 1);
}
PermissionGraphWrite SqlitePermissionGraphUnit::synchronize_catalog(std::uint64_t expected) {
    impl_->preflight(impl_->installation, expected, impl_->trust);
    if (!expected)
        input(PermissionGraphInputError::invalid_input);
    return storage_guard([&] {
        const auto before = impl_->source(true);
        if (!before.revision || before.revision != expected)
            return PermissionGraphWrite::stale;
        space(before.revision != UINT64_MAX);
        Projection p{impl_->limits, impl_->limits.selected_bytes};
        p(impl_->installation, expected, expected + 1);
        p.framed = true;
        p(impl_->trust);
        const SqlStatement s{"UPDATE main.subathon_permission_head SET revision=?,catalog=? WHERE "
                             "installation_ref=? AND revision=?",
                             {encode(expected + 1, impl_->limits),
                              encode(impl_->trust, impl_->limits), blob(impl_->installation),
                              encode(expected, impl_->limits)}};
        const SqlStatement probe{"SELECT changes()", {}};
        (void)impl_->session.execute(s);
        impl_->changed(probe);
        return PermissionGraphWrite::staged;
    });
}
} // namespace subathon::server
