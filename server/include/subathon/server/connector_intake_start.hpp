// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/connector_intake.hpp>
#include <subathon/server/connector_lifecycle.hpp>
#include <subathon/server/session_resolution.hpp>

namespace subathon::server::connector_intake {

enum class StartupAction { register_first, activate_first, checkpoint_first, inspect };
struct StartupExecutionAccount {
    QByteArray installation, account;
};
using StartupBinding = std::variant<SessionLookupRequest, StartupExecutionAccount>;

struct StartupAuthorityRequest {
    StartupAction action;
    StartupBinding binding;
    Scope scope;
    bool require_fresh;
};
struct StartupPermit {
    StartupAction action;
    Scope scope;
    QByteArray principal, account_snapshot, permission_snapshot, actor, decision;
};
struct StartupDenied {};
struct StartupUnavailable {};
using StartupDecision = std::variant<StartupDenied, StartupUnavailable, StartupPermit>;

class StartupAuthority {
  public:
    virtual ~StartupAuthority() = default;
    virtual StartupDecision resolve(const SqliteWorkSession &, const StartupAuthorityRequest &) = 0;
};

struct StartupLane {
    PolicyMember member;
    QByteArray lane_ref, enrollment_generation, planned_generation, profile_revision,
        cursor_contract;
    bool fresh_lane;
};
struct StartupConfiguration {
    QByteArray installation, revision, execution_principal;
    QList<StartupLane> lanes;
    CatchUpMode mode{CatchUpMode::review};
    bool require_fresh{true};
};

struct StartupOperation {
    QByteArray key;
    Scope scope;
    int purpose;
    qint64 epoch, policy_revision;
    QByteArray generation;
    qint64 revision;
    int outcome;
    QByteArray intent;
    std::optional<QByteArray> prior_cut, cut;
    bool operator==(const StartupOperation &) const = default;
};

struct LiveProgressRequest {
    Scope scope;
    Fence fence;
    QByteArray registry_revision, profile_revision, cursor_contract;
    StartupOperation activation;
    QList<StartupOperation> reconnects;
    bool operator==(const LiveProgressRequest &) const = default;
};
struct LiveEndpoint {
    Endpoint boundary;
    QByteArray format, position_namespace;
    std::optional<QByteArray> position;
    bool operator==(const LiveEndpoint &) const = default;
};
struct LiveDispositionClaim {
    Scope scope;
    QByteArray generation, intake_ref, normalized, provenance;
    qint64 arrival_ms;
    std::optional<PageSlot> history;
    int phase, disposition;
    std::optional<qint64> quarantine_bytes;
    std::optional<AttemptLocator> attempt;
};
struct LiveProgressObservation {
    LiveProgressRequest request;
    Coverage coverage;
    LiveEndpoint lower, upper;
    QList<QByteArray> ordered_inputs;
    QList<LiveDispositionClaim> dispositions;
    QByteArray proof;
};
using LiveProgressResult = std::variant<LiveProgressObservation, HistoryFailure>;

class ConnectorLiveProgressPort {
  public:
    virtual ~ConnectorLiveProgressPort() = default;
    virtual LiveProgressResult observe_progress(const LiveProgressRequest &) = 0;
};

struct LiveItemEnvelope {
    Selection selection;
    QByteArray position;
    ConnectorEvent event;
    QByteArray normalized;
};

enum class StartupPreCutRelation { BeforeCut, NewSide, Unknown };

// Registry consumers borrow an immutable binding. Witness validators use the
// installed instance's invocation evidence and bind the full owner request.
struct StartupProfile {
    Scope scope;
    QByteArray registry_revision, profile_revision, cursor_contract, generation;
    ConnectorHistoryAdapter &history;
    ConnectorLiveProgressPort &live;
    std::function<bool(const Fence &, const CutRequest &, const CutObservation &)> validate_cut;
    std::function<bool(const LiveProgressRequest &, const LiveProgressObservation &)> validate_live;
    // Replay validates decoded retained evidence without any invocation handle.
    std::function<bool(const Fence &, const CutRequest &, const CutObservation &)> attest_cut;
    std::function<bool(const LiveProgressRequest &, const LiveProgressObservation &)> attest_live;
    std::function<StartupPreCutRelation(const QByteArray &retained_cut,
                                       const LiveItemEnvelope &observed_item)>
        qualify_live_item{};
};

class StartupProfileRegistry {
  public:
    virtual ~StartupProfileRegistry() = default;
    virtual std::shared_ptr<const StartupProfile> select(const Scope &, const QByteArray &profile,
                                                         const QByteArray &cursor) = 0;
};

// Owner-issued opaque values. Request echoes cannot stand in for these.
class PreparedStartupIntent;
class QualifiedFirstCut;
class QualifiedLivePrefix;

class StartupOwner final {
  public:
    StartupOwner(SqliteWriter &owner_writer, const StartupConfiguration &trusted_configuration,
                 StartupProfileRegistry &registry, StartupAuthority &current_authority,
                 StartupBinding owner_binding)
        : writer(owner_writer), configuration(trusted_configuration), profiles(registry),
          authority(current_authority), binding(std::move(owner_binding)) {}
    StartupOwner(const StartupOwner &) = delete;
    StartupOwner &operator=(const StartupOwner &) = delete;

    SqliteWriter &writer;
    const StartupConfiguration &configuration;
    StartupProfileRegistry &profiles;
    StartupAuthority &authority;
    const StartupBinding binding;
};

struct EnrollmentEcho {
    Scope scope;
    QByteArray generation;
};
struct RegisterRequest {
    QByteArray installation, configuration_revision;
    QList<Scope> cohort;
    std::optional<QByteArray> policy_document;
    QList<PolicyMember> members;
    QList<EnrollmentEcho> enrollment;
};
struct ActivationSource {
    Scope scope;
    Fence expected;
    QByteArray successor_generation, history_ref;
    std::optional<CutObservation> candidate;
};
struct ActivateRequest {
    qint64 expected_policy_head;
    QByteArray configuration_revision;
    std::optional<CatchUpMode> mode;
    QList<ActivationSource> sources;
};
struct CheckpointRequest {
    Scope scope;
    Fence expected;
    QByteArray configuration_revision, activation_ref;
    QList<QByteArray> reconnect_refs;
    QByteArray checkpoint_ref;
    std::optional<LiveProgressObservation> candidate;
    std::shared_ptr<const QualifiedLivePrefix> qualified{};
};
struct StartupSelector {
    std::variant<RegisterRequest, ActivateRequest, CheckpointRequest> transition;
    std::shared_ptr<const PreparedStartupIntent> prepared{};
};

struct StartupSource {
    Scope scope;
    Fence fence;
    int state;
    std::optional<QByteArray> cut, frontier;
    QByteArray cursor_contract;
    bool operator==(const StartupSource &) const = default;
};
struct StartupProjection {
    qint64 policy_head;
    QByteArray policy;
    QList<PolicyMember> members;
    QList<StartupSource> sources;
    QList<StartupOperation> operations;
    bool operator==(const StartupProjection &) const = default;
};
enum class StartupReadback { applied, absent_at_before, superseded };
struct StartupResult {
    IntakeStatus status{IntakeStatus::not_implemented};
    std::optional<StartupProjection> projection{};
    std::optional<StartupReadback> readback{};
};
struct StartupOutcome {
    StorageError admission;
    std::optional<SqliteWorkResult> work;
    std::optional<StartupResult> result;
};
using StartupCompletion = std::function<void(StartupOutcome)>;
struct StartupFaults {
    using Resume = std::function<void()>;
    std::function<bool()> before_commit{};
    std::function<void()> after_commit{};
    std::function<void(Resume)> on_prepared{};
    // Owner-thread observer of the sealed intent before the preparation barrier.
    std::function<void(std::shared_ptr<const PreparedStartupIntent>)> capture_prepared_intent{};
    bool lose_commit_ack{};
};

[[nodiscard]] StorageError register_first(StartupOwner &, RegisterRequest, StartupCompletion,
                                          StartupFaults = {});
[[nodiscard]] StorageError activate_first(StartupOwner &, ActivateRequest, StartupCompletion,
                                          StartupFaults = {});
[[nodiscard]] StorageError checkpoint_first(StartupOwner &, CheckpointRequest, StartupCompletion,
                                            StartupFaults = {});
[[nodiscard]] StorageError inspect_startup(StartupOwner &, StartupSelector, StartupCompletion);

} // namespace subathon::server::connector_intake
