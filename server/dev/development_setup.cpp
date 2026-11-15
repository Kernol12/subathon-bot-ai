// SPDX-License-Identifier: GPL-3.0-or-later
#include "development_setup.hpp"
#include <QDateTime>
#include <limits>
#include <span>
#include <stdexcept>

namespace subathon::dev {
using namespace server;
namespace {
constexpr std::int64_t controlled_utc_ms = 1'800'000'000'000;
constexpr std::int64_t session_lifetime_ms = 86'400'000;

void staged(LifecycleWriteResult r) {
    if (r != LifecycleWriteResult::staged)
        throw std::runtime_error("Development account/session provisioning refused");
}
std::uint64_t revision(const ReferenceAuthorityWrite &r) {
    if (auto v = std::get_if<ReferenceAuthorityStaged>(&r))
        return v->revision;
    throw std::runtime_error("Development rule authorization refused");
}

// Synthetic owner USE/GRANT for this one timer; source scope is explicitly Any.
// No RSS1 codec is needed. This graph is never installed by a server library.
core::PermissionEditableState permissions(const PermissionGraphTrust &trust) {
    core::PermissionEditableAccount owner{"development-owner", {}, {}};
    for (const auto &op : trust.catalog.operations) {
        if (!op.operation.starts_with("reference.") && op.operation != "overrides.edit" &&
            op.operation != "timer.snapshot.read")
            continue;
        core::PermissionPolicyEntry e;
        e.source = "development-provisioning." + op.operation;
        e.grant = op.operation;
        e.operation = op.operation;
        for (const auto &slot : op.resource_slots) {
            core::PermissionResourceConstraint r{slot, core::PermissionResourceMode::any, {}};
            if (slot != "source") {
                r.mode = core::PermissionResourceMode::allowed_set;
                r.allowed = {slot == "installation" ? "development-installation"
                             : slot == "timer"      ? "development-timer"
                                                    : "development-owner"};
            }
            e.resources.push_back(std::move(r));
        }
        for (const auto &q : op.quantity_slots)
            e.quantities.push_back({q.slot, q.unit, core::PermissionQuantityMode::any, {}, {}});
        owner.individual_entries.push_back(std::move(e));
    }
    return {core::PermissionFactsState::complete, {}, {std::move(owner)}};
}
} // namespace

SessionClockResolution DevelopmentSessionClock::sample_current() {
    return SessionClockSample{controlled_ ? controlled_utc_ms
                                          : QDateTime::currentMSecsSinceEpoch()};
}
ReferenceClockSampleResult ControlledReferenceClock::sample() {
    return ReferenceClockSample{0, controlled_utc_ms};
}
SessionLookupRequest owner_session() {
    // Public synthetic digest, not a credential or a real login token.
    AccountSessionDigest digest{};
    digest.fill(0x44);
    return {"development-installation", digest};
}
ReferenceConfigurationFacts configuration(std::int64_t add_ms, std::int64_t cap_ms) {
    ReferenceConfigurationFacts f;
    f.configuration_ref = "development-configuration";
    f.configuration_owner_revision_ref = "development-configuration-owner-v1";
    auto &c = f.projection;
    c.configuration_ref = f.configuration_ref;
    c.policy_revision_ref = "development-policy-v1";
    c.synthetic_origin = {"synthetic.reference.v1", "development-source"};
    c.receipt_namespace = "development-events";
    c.rule = {"development-rule",
              "development-rule-v1",
              "development-action",
              "development-action-v1",
              "development-timer",
              add_ms,
              {ApplicationPrincipalKind::automation, "development-installation",
               "development-automation"},
              "development-capability"};
    c.maximum_main_ms = cap_ms;
    c.other_rules = c.modifiers = c.milestones = c.outbound_actions = c.generated_events =
        ReferenceFactsState::complete;
    f.rules_state = ReferenceFactsState::complete;
    f.enabled_rules = {
        {c.rule, {"synthetic.reference.support.v1"}, {"time_add"}, {"ms"}, {}, {}, {}}};
    f.modifiers.state = f.milestones.state = f.outbound_actions.state = f.generated_events.state =
        ReferenceFactsState::complete;
    return f;
}

// One-time DEVELOPMENT ONLY: synthetic accepted sign-in facts, no provider call/login.
// All mutations use public stores on the caller's single writer transaction.
void provision(const SqliteWorkSession &s, CurrentSessionClock &clock,
               const ReferenceConfiguration &c) {
    const auto lookup = owner_session();
    const auto now = std::get<SessionClockSample>(clock.sample_current()).now_utc_ms;
    if (now > std::numeric_limits<std::int64_t>::max() - session_lifetime_ms)
        throw std::runtime_error("Development session lifetime out of range");
    AccountSessionReadScope scope{};
    scope.read_scope_ref.fill(0x53);
    SqliteAccountSessionUnit accounts(s, scope);
    const StoredInstallationFacts installation{
        {lookup.installation_ref, "development-installation-epoch"}, "development-installation-v1"};
    const StoredAccountFacts account{
        {lookup.installation_ref, "development-owner", "development-account-epoch", true},
        "development-account-v1"};
    staged(accounts.insert_installation(installation));
    staged(accounts.insert_account(account));
    // Public synthetic password: only a development verifier, never a real credential.
    constexpr std::string_view password = "subathon-development-only";
    const auto verifier = create_password_hash(std::as_bytes(std::span(password)));
    staged(accounts.replace_local_credential(
        {},
        {lookup.installation_ref,
         {"development-owner", "development-synthetic-authenticator", verifier}},
        account, installation));
    const AcceptedSessionAuthentication accepted{lookup.installation_ref,
                                                 "development-owner",
                                                 "development-attempt",
                                                 "development-installation-epoch",
                                                 "development-account-epoch",
                                                 "development-account-v1",
                                                 "development-synthetic-authenticator",
                                                 "development-synthetic-acceptance",
                                                 AcceptedSignInMethod::local_password,
                                                 now};
    // Synthetic local-password acceptance facts; this executable performs no login.
    staged(accounts.insert_session(
        {lookup.installation_ref, "development-owner", "development-session",
         "development-installation-epoch", "development-account-epoch", "development-session-v1",
         lookup.digest, SessionRecordState::active, now, now, now + session_lifetime_ms},
        SessionRetention::normal, accepted));
    const auto trust = reference_permission_graph_trust();
    const auto graph = permissions(trust);
    if (SqlitePermissionGraphUnit(s, lookup.installation_ref, trust)
            .bootstrap("development-owner", graph, graph) != PermissionGraphWrite::staged)
        throw std::runtime_error("Development permission provisioning refused");
    SqliteReferenceAuthorityUnit authority(s);
    auto head = revision(authority.bootstrap(lookup, clock));
    const auto &r = c.rule;
    head = revision(authority.enroll(lookup, clock, head,
                                     {lookup.installation_ref, r.automation_capability_ref,
                                      r.automation.principal_ref, r.rule_ref, r.rule_version_ref,
                                      r.action_ref, r.action_version_ref, r.timer_ref,
                                      c.synthetic_origin, c.receipt_namespace, r.add_ms},
                                     "development-owner"));
    (void)revision(authority.set_state(lookup, clock, head, r.automation_capability_ref,
                                       ReferenceAuthorityState::authorized));
}
} // namespace subathon::dev
