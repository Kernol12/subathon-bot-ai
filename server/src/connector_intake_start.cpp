// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/connector_intake_start.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

#include "sqlite_connector_intake_private.hpp"
#include "sqlite_receipt_read.hpp"

#include <QSet>
#include <QThread>
#include <QtEndian>
#include <algorithm>
#include <limits>

namespace subathon::server::connector_intake {
namespace {

using Status = IntakeStatus;
using detail::bounded;
using detail::document_limit;
using detail::item_limit;
using detail::page_limit;
using detail::reference_limit;
using detail::scope_bindings;
using Transition = decltype(StartupSelector::transition);
using Profile = std::shared_ptr<const StartupProfile>;

constexpr qsizetype cohort_limit = 256;
constexpr qsizetype selection_limit = 16;

QByteArray frame(const QByteArray &value) {
    const auto length = qToBigEndian(static_cast<quint32>(value.size()));
    return QByteArray(reinterpret_cast<const char *>(&length), sizeof(length)) + value;
}

QByteArray frame_optional(const std::optional<QByteArray> &value) {
    return value ? QByteArray(1, '\1') + frame(*value) : QByteArray(1, '\0');
}

QByteArray integer(qint64 value) {
    const auto bits = qToBigEndian(static_cast<quint64>(value));
    return QByteArray(reinterpret_cast<const char *>(&bits), sizeof(bits));
}

QByteArray scope_key(const Scope &scope) {
    return frame(scope.installation) + frame(scope.provider) + frame(scope.account) +
           frame(scope.stream);
}

struct Head {
    Scope scope;
    Fence fence;
    std::optional<QByteArray> frontier;
    bool operator==(const Head &) const = default;
};

Head head(const StartupSource &source) { return {source.scope, source.fence, source.frontier}; }

QByteArray encode_head(const Head &value) {
    return scope_key(value.scope) + integer(value.fence.epoch) + integer(value.fence.revision) +
           integer(value.fence.policy_revision) + frame(value.fence.generation) +
           frame_optional(value.frontier);
}

// These are the existing v20 framed documents. Keep presence distinct from an
// empty BLOB and reject trailing bytes, invalid tags, and oversized fields.
class Decoder {
  public:
    explicit Decoder(QByteArrayView document) : remaining(document) {}

    void magic(QByteArrayView value) {
        require(remaining.startsWith(value));
        remaining = remaining.sliced(value.size());
    }

    void require(bool condition) const {
        if (!condition)
            throw Invalid{};
    }

    struct Invalid {};

    quint8 tag() {
        require(!remaining.isEmpty());
        const auto value = static_cast<quint8>(remaining.front());
        remaining = remaining.sliced(1);
        return value;
    }

    Endpoint endpoint() {
        const auto value = tag();
        require(value == 1 || value == 2);
        return static_cast<Endpoint>(value);
    }

    qint64 positive_integer() {
        require(remaining.size() >= 8);
        const auto value = qFromBigEndian<quint64>(remaining.data());
        require(value > 0 && value <= static_cast<quint64>(std::numeric_limits<qint64>::max()));
        remaining = remaining.sliced(8);
        return static_cast<qint64>(value);
    }

    QByteArray field(qsizetype limit = reference_limit, bool allow_empty = false) {
        require(remaining.size() >= 4);
        const auto length = qFromBigEndian<quint32>(remaining.data());
        remaining = remaining.sliced(4);
        require(length <= limit && length <= remaining.size() && (length || allow_empty));
        const auto value = remaining.first(length).toByteArray();
        remaining = remaining.sliced(length);
        return value;
    }

    std::optional<QByteArray> optional_position() {
        const auto present = tag();
        require(present <= 1);
        return present ? std::optional{field(reference_limit, true)} : std::nullopt;
    }

    Head source_head() {
        return {{field(), field(), field(), field()},
                {positive_integer(), positive_integer(), positive_integer(), field()},
                optional_position()};
    }

    void end() const { require(remaining.isEmpty()); }

  private:
    QByteArrayView remaining;
};

template <class Decode> auto decoded(const QByteArray &bytes, Decode decode) {
    using Result = decltype(decode(std::declval<Decoder &>()));
    if (!bounded(bytes, document_limit))
        return std::optional<Result>{};
    try {
        Decoder reader(bytes);
        auto value = decode(reader);
        reader.end();
        return std::optional<Result>{std::move(value)};
    } catch (const Decoder::Invalid &) {
        return std::optional<Result>{};
    }
}

struct Boundary {
    Endpoint endpoint;
    QualifiedCut cut;
};

QByteArray cut_document(const CutObservation &observed) {
    const auto &cut = observed.cut;
    return QByteArray("CUT1") + char(observed.boundary) + frame(cut.format) +
           frame(cut.position_namespace) + frame_optional(cut.position) +
           frame(cut.qualified_generation) + frame(cut.proof);
}

auto decode_cut(const QByteArray &bytes) {
    return decoded(bytes, [](Decoder &reader) {
        reader.magic("CUT1");
        return Boundary{reader.endpoint(),
                        {reader.field(), reader.field(), reader.optional_position(), reader.field(),
                         reader.field(document_limit)}};
    });
}

struct History {
    Head before, after;
    QByteArray cursor, decision, proof;
};

auto decode_history(const StartupOperation &operation, QByteArrayView magic) {
    return decoded(operation.intent, [&](Decoder &reader) {
        reader.magic(magic);
        auto before = reader.source_head();
        auto after = reader.source_head();
        reader.require(before.scope == operation.scope && after.scope == operation.scope &&
                       after.fence.epoch == operation.epoch &&
                       after.fence.policy_revision == operation.policy_revision &&
                       after.fence.generation == operation.generation && operation.revision == 1 &&
                       operation.outcome == 1);
        return History{std::move(before), std::move(after), reader.field(document_limit),
                       reader.field(), reader.field(document_limit)};
    });
}

QByteArray coverage_document(const LiveProgressObservation &observation) {
    const auto &lower = observation.lower;
    const auto &upper = observation.upper;
    return QByteArray("CV1", 4) + char(lower.boundary) + char(upper.boundary) +
           frame(lower.format) + frame(lower.position_namespace) + frame(*lower.position) +
           frame(*upper.position) + frame(QByteArray{}) + frame(observation.proof);
}

struct CoverageProof {
    LiveEndpoint lower, upper;
    QByteArray proof;
};

auto decode_coverage(const QByteArray &bytes) {
    return decoded(bytes, [](Decoder &reader) {
        reader.magic(QByteArrayView("CV1", 4));
        const auto lower = reader.endpoint();
        reader.require(reader.endpoint() == Endpoint::inclusive);
        const auto format = reader.field();
        const auto name_space = reader.field();
        const auto start = reader.field();
        const auto finish = reader.field();
        reader.require(reader.field(0, true).isEmpty());
        return CoverageProof{{lower, format, name_space, start},
                             {Endpoint::inclusive, format, name_space, finish},
                             reader.field(document_limit)};
    });
}

QByteArray mode_token(CatchUpMode mode) {
    switch (mode) {
    case CatchUpMode::review:
        return "review";
    case CatchUpMode::automatic:
        return "automatic";
    case CatchUpMode::record_without_credit:
        return "record_without_credit";
    }
    return {};
}

struct Policy {
    QByteArray mode, configuration, decision;
};

auto decode_policy(const QByteArray &bytes) {
    return decoded(bytes, [](Decoder &reader) {
        auto mode = reader.field();
        reader.require(mode == "review" || mode == "automatic" || mode == "record_without_credit");
        return Policy{std::move(mode), reader.field(), reader.field()};
    });
}

QByteArray policy_document(const StartupConfiguration &config, const StartupPermit &permit) {
    return frame(mode_token(config.mode)) + frame(config.revision) + frame(permit.decision);
}

bool exceeds_scope(const Scope &scope) {
    return scope.installation.size() > reference_limit || scope.provider.size() > reference_limit ||
           scope.account.size() > reference_limit || scope.stream.size() > reference_limit;
}

bool exceeds_position(const std::optional<QByteArray> &position) {
    return position && position->size() > reference_limit;
}

bool exceeds_cut(const CutObservation &value) {
    const auto &request = value.request;
    const auto &source = request.source;
    const auto &cut = value.cut;
    if (exceeds_scope(source.scope) || source.fence.generation.size() > reference_limit ||
        source.profile.size() > reference_limit || exceeds_position(request.prior_frontier) ||
        cut.format.size() > reference_limit || cut.position_namespace.size() > reference_limit ||
        cut.qualified_generation.size() > reference_limit || exceeds_position(cut.position) ||
        cut.proof.size() > document_limit)
        return true;
    const auto request_bytes =
        encode_head({source.scope, source.fence, request.prior_frontier}).size() +
        frame(source.profile).size() + 1;
    return request_bytes + cut_document(value).size() > document_limit;
}

bool exceeds_operation(const StartupOperation &value) {
    return value.key.size() > reference_limit || exceeds_scope(value.scope) ||
           value.generation.size() > reference_limit || value.intent.size() > document_limit ||
           (value.cut && value.cut->size() > document_limit) ||
           (value.prior_cut && value.prior_cut->size() > document_limit);
}

bool exceeds_live(const LiveProgressObservation &value) {
    const auto &request = value.request;
    if (exceeds_scope(request.scope) || request.fence.generation.size() > reference_limit ||
        request.registry_revision.size() > reference_limit ||
        request.profile_revision.size() > reference_limit ||
        request.cursor_contract.size() > document_limit || exceeds_operation(request.activation) ||
        request.reconnects.size() > selection_limit ||
        std::ranges::any_of(request.reconnects, exceeds_operation) ||
        value.ordered_inputs.size() > selection_limit ||
        value.dispositions.size() > selection_limit || value.proof.size() > document_limit)
        return true;
    for (const auto *endpoint : {&value.lower, &value.upper})
        if (endpoint->format.size() > reference_limit ||
            endpoint->position_namespace.size() > reference_limit ||
            exceeds_position(endpoint->position))
            return true;

    qsizetype bytes = 0;
    for (const auto &input : value.ordered_inputs) {
        if (input.size() > item_limit)
            return true;
        bytes += input.size();
    }
    if (bytes > page_limit)
        return true;
    bytes = 0;
    for (const auto &claim : value.dispositions) {
        if (exceeds_scope(claim.scope) || claim.generation.size() > reference_limit ||
            claim.intake_ref.size() > reference_limit || claim.normalized.size() > item_limit ||
            claim.provenance.size() > document_limit ||
            (claim.history && claim.history->operation.size() > reference_limit))
            return true;
        bytes += claim.normalized.size() + claim.provenance.size();
    }
    return bytes > page_limit;
}

bool same_cut_request(const CutRequest &a, const CutRequest &b) {
    return a.source.scope == b.source.scope && a.source.fence == b.source.fence &&
           a.source.profile == b.source.profile && a.purpose == b.purpose &&
           a.prior_frontier == b.prior_frontier;
}

bool same_cut(const CutObservation &a, const CutObservation &b) {
    return same_cut_request(a.request, b.request) && cut_document(a) == cut_document(b);
}

bool same_claim(const LiveDispositionClaim &a, const LiveDispositionClaim &b) {
    return a.scope == b.scope && a.generation == b.generation && a.intake_ref == b.intake_ref &&
           a.normalized == b.normalized && a.provenance == b.provenance &&
           a.arrival_ms == b.arrival_ms && a.history == b.history && a.phase == b.phase &&
           a.disposition == b.disposition && a.quarantine_bytes == b.quarantine_bytes &&
           a.attempt == b.attempt;
}

bool same_live(const LiveProgressObservation &a, const LiveProgressObservation &b) {
    return a.request == b.request && a.coverage == b.coverage && a.lower == b.lower &&
           a.upper == b.upper && a.ordered_inputs == b.ordered_inputs && a.proof == b.proof &&
           std::ranges::equal(a.dispositions, b.dispositions, same_claim);
}

Status history_failure(HistoryFailure failure) {
    switch (failure) {
    case HistoryFailure::unsupported:
        return Status::unsupported;
    case HistoryFailure::temporarily_unavailable:
        return Status::unavailable;
    case HistoryFailure::unknown_capability:
    case HistoryFailure::unproved:
        return Status::unproved;
    }
    return Status::unproved;
}

QVariant sql_blob(const std::optional<QByteArray> &value) {
    return value ? QVariant{*value} : QVariant{};
}

std::optional<QByteArray> optional_blob(const QVariant &value) {
    return value.isNull() ? std::nullopt : std::optional{value.toByteArray()};
}

const StartupLane *lane_for(const StartupConfiguration &config, const Scope &scope) {
    const auto found = std::ranges::find_if(
        config.lanes, [&](const auto &lane) { return lane.member.scope == scope; });
    return found == config.lanes.end() ? nullptr : &*found;
}

Profile select_profile(StartupProfileRegistry &registry, const StartupLane &lane,
                       const QByteArray &generation) {
    auto profile = registry.select(lane.member.scope, lane.profile_revision, lane.cursor_contract);
    if (!profile || profile->scope != lane.member.scope ||
        profile->profile_revision != lane.profile_revision ||
        profile->cursor_contract != lane.cursor_contract || profile->generation != generation ||
        !bounded(profile->registry_revision, reference_limit))
        return {};
    return profile;
}

StartupResult stale(bool inspecting, StartupReadback reason = StartupReadback::superseded) {
    return {Status::stale, {}, inspecting ? std::optional{reason} : std::nullopt};
}

StartupResult applied(bool inspecting, StartupProjection projection) {
    return {Status::exact_replay, std::move(projection),
            inspecting ? std::optional{StartupReadback::applied} : std::nullopt};
}

struct Owner {
    SqliteWriter &writer;
    const StartupConfiguration &configuration;
    StartupProfileRegistry &profiles;
    StartupAuthority &authority;
    StartupBinding binding;
};

using Permits = QList<StartupPermit>;

std::variant<Status, Permits> authorize(const SqliteWorkSession &session, const Owner &owner,
                                        StartupAction action, const QList<Scope> &scopes) {
    Permits permits;
    bool unavailable = false;
    for (const auto &scope : scopes) {
        auto decision = owner.authority.resolve(
            session, {action, owner.binding, scope, owner.configuration.require_fresh});
        if (std::holds_alternative<StartupDenied>(decision))
            return Status::denied;
        auto *permit = std::get_if<StartupPermit>(&decision);
        if (!permit || permit->action != action || permit->scope != scope ||
            !bounded(permit->principal, reference_limit) ||
            !bounded(permit->account_snapshot, reference_limit) ||
            !bounded(permit->permission_snapshot, reference_limit) ||
            !bounded(permit->actor, reference_limit) || !bounded(permit->decision, reference_limit))
            unavailable = true;
        else
            permits.append(std::move(*permit));
    }
    if (unavailable)
        return Status::unavailable;
    return permits;
}

StartupAction action_for(const Transition &request) {
    if (std::holds_alternative<RegisterRequest>(request))
        return StartupAction::register_first;
    if (std::holds_alternative<ActivateRequest>(request))
        return StartupAction::activate_first;
    return StartupAction::checkpoint_first;
}

QList<Scope> authorized_scopes(const StartupConfiguration &config, const Transition &request) {
    if (const auto *checkpoint = std::get_if<CheckpointRequest>(&request)) {
        if (const auto *lane = lane_for(config, checkpoint->scope))
            return {lane->member.scope};
    }
    QList<Scope> scopes;
    for (const auto &lane : config.lanes)
        if (!scopes.contains(lane.member.scope))
            scopes.append(lane.member.scope);
    return scopes;
}

std::optional<Status> configuration_shape(const StartupConfiguration &config) {
    if (config.lanes.size() > cohort_limit || config.installation.size() > reference_limit ||
        config.revision.size() > reference_limit ||
        config.execution_principal.size() > reference_limit)
        return Status::limit_exceeded;
    if (config.lanes.isEmpty() || config.installation.isEmpty() || config.revision.isEmpty() ||
        config.execution_principal.isEmpty() || mode_token(config.mode).isEmpty())
        return Status::invalid_input;

    QSet<QByteArray> scopes, lanes, groups, primaries;
    for (const auto &lane : config.lanes) {
        const auto &member = lane.member;
        const QList<QByteArray> refs{lane.lane_ref,           lane.enrollment_generation,
                                     lane.planned_generation, lane.profile_revision,
                                     member.award_class,      member.effect_scope,
                                     member.overlap_set};
        if (exceeds_scope(member.scope) || lane.cursor_contract.size() > document_limit ||
            member.document.size() > document_limit ||
            std::ranges::any_of(refs, [](const auto &ref) { return ref.size() > reference_limit; }))
            return Status::limit_exceeded;
        const auto scope = scope_key(member.scope);
        const auto group = frame(member.overlap_set) + frame(member.effect_scope);
        if (!detail::valid_scope(member.scope) ||
            member.scope.installation != config.installation || lane.cursor_contract.isEmpty() ||
            scopes.contains(scope) || lanes.contains(lane.lane_ref) ||
            std::ranges::any_of(refs, [](const auto &ref) { return ref.isEmpty(); }) ||
            (member.primary && primaries.contains(group)) || member.document.isEmpty())
            return Status::invalid_input;
        scopes.insert(scope);
        lanes.insert(lane.lane_ref);
        groups.insert(group);
        if (member.primary)
            primaries.insert(group);
    }
    if (groups != primaries)
        return Status::invalid_input;
    return {};
}

bool valid_fence(const Fence &fence) {
    return fence.epoch > 0 && fence.revision > 0 && fence.policy_revision > 0 &&
           bounded(fence.generation, reference_limit);
}

bool over_ref(const QByteArray &ref) { return ref.size() > reference_limit; }

std::optional<Status> request_shape(const StartupConfiguration &config,
                                    const RegisterRequest &request) {
    if (request.cohort.size() > cohort_limit || request.members.size() > cohort_limit ||
        request.enrollment.size() > cohort_limit || over_ref(request.installation) ||
        over_ref(request.configuration_revision) ||
        (request.policy_document && request.policy_document->size() > document_limit))
        return Status::limit_exceeded;
    if (std::ranges::any_of(request.cohort, exceeds_scope))
        return Status::limit_exceeded;
    for (const auto &member : request.members)
        if (exceeds_scope(member.scope) || over_ref(member.award_class) ||
            over_ref(member.effect_scope) || over_ref(member.overlap_set) ||
            member.document.size() > document_limit)
            return Status::limit_exceeded;
    for (const auto &entry : request.enrollment)
        if (exceeds_scope(entry.scope) || over_ref(entry.generation))
            return Status::limit_exceeded;

    const auto size = config.lanes.size();
    if (request.installation != config.installation ||
        request.configuration_revision != config.revision || request.cohort.size() != size ||
        request.members.size() != size || request.enrollment.size() != size)
        return Status::invalid_input;
    for (const auto &lane : config.lanes) {
        const auto &scope = lane.member.scope;
        if (!request.cohort.contains(scope) || !request.members.contains(lane.member) ||
            !std::ranges::any_of(request.enrollment, [&](const auto &echo) {
                return echo.scope == scope && echo.generation == lane.enrollment_generation;
            }))
            return Status::invalid_input;
    }
    return {};
}

std::optional<Status> request_shape(const StartupConfiguration &config,
                                    const ActivateRequest &request) {
    if (request.sources.size() > cohort_limit || over_ref(request.configuration_revision))
        return Status::limit_exceeded;
    for (const auto &entry : request.sources)
        if (exceeds_scope(entry.scope) || over_ref(entry.expected.generation) ||
            over_ref(entry.successor_generation) || over_ref(entry.history_ref) ||
            (entry.candidate && exceeds_cut(*entry.candidate)))
            return Status::limit_exceeded;

    if (request.sources.size() != config.lanes.size() || request.expected_policy_head <= 0 ||
        request.configuration_revision.isEmpty())
        return Status::invalid_input;
    QSet<QByteArray> keys, scopes;
    for (const auto &entry : request.sources) {
        const auto *lane = lane_for(config, entry.scope);
        const auto scope = scope_key(entry.scope);
        if (!lane || scopes.contains(scope) || keys.contains(entry.history_ref) ||
            entry.history_ref.isEmpty() || entry.successor_generation.isEmpty() ||
            !valid_fence(entry.expected))
            return Status::invalid_input;
        scopes.insert(scope);
        keys.insert(entry.history_ref);
    }
    return {};
}

std::optional<Status> request_shape(const StartupConfiguration &config,
                                    const CheckpointRequest &request) {
    if (exceeds_scope(request.scope) || over_ref(request.expected.generation) ||
        over_ref(request.configuration_revision) || over_ref(request.activation_ref) ||
        over_ref(request.checkpoint_ref) || request.reconnect_refs.size() > selection_limit ||
        std::ranges::any_of(request.reconnect_refs, over_ref) ||
        (request.candidate && exceeds_live(*request.candidate)))
        return Status::limit_exceeded;
    if (!lane_for(config, request.scope) || !valid_fence(request.expected) ||
        request.configuration_revision.isEmpty() || request.activation_ref.isEmpty() ||
        request.checkpoint_ref.isEmpty() ||
        std::ranges::any_of(request.reconnect_refs, [](const auto &ref) { return ref.isEmpty(); }))
        return Status::invalid_input;
    return {};
}

qint64 policy_head(const SqliteWorkSession &session, const QByteArray &installation) {
    const auto rows =
        session.execute({"SELECT revision FROM main.subathon_intake_policies WHERE installation=? "
                         "ORDER BY revision DESC LIMIT 1",
                         {installation}});
    return rows.isEmpty() ? 0 : rows.front().front().toLongLong();
}

QByteArray read_policy(const SqliteWorkSession &session, const QByteArray &installation,
                       qint64 revision) {
    const auto rows = session.execute(
        {"SELECT document FROM main.subathon_intake_policies WHERE installation=? AND revision=?",
         {installation, revision}});
    return rows.isEmpty() ? QByteArray{} : rows.front().front().toByteArray();
}

std::optional<QList<PolicyMember>> read_members(const SqliteWorkSession &session,
                                                const QByteArray &installation, qint64 revision,
                                                const Scope *scope = nullptr) {
    QString query =
        "SELECT CASE WHEN typeof(provider)='blob' AND length(provider) BETWEEN 1 AND 4096 "
        "THEN provider END,CASE WHEN typeof(account)='blob' AND length(account) BETWEEN 1 AND 4096 "
        "THEN account END,CASE WHEN typeof(stream)='blob' AND length(stream) BETWEEN 1 AND 4096 "
        "THEN stream END,CASE WHEN typeof(award_class)='blob' AND length(award_class) BETWEEN 1 "
        "AND 4096 THEN award_class END,CASE WHEN typeof(effect_scope)='blob' AND "
        "length(effect_scope) BETWEEN 1 AND 4096 THEN effect_scope END,CASE WHEN "
        "typeof(overlap_set)='blob' AND length(overlap_set) BETWEEN 1 AND 4096 THEN overlap_set "
        "END,"
        "primary_slot,document FROM main.subathon_intake_members WHERE installation=? "
        "AND policy_revision=?";
    QList<QVariant> args{installation, revision};
    if (scope) {
        query += " AND provider=? AND account=? AND stream=? LIMIT 2";
        args.append({scope->provider, scope->account, scope->stream});
    } else {
        query += " LIMIT 257";
    }
    const auto rows = session.execute({query, args});
    if (rows.size() > (scope ? 1 : cohort_limit))
        return {};

    QList<PolicyMember> members;
    for (const auto &row : rows) {
        if (std::any_of(row.cbegin(), row.cbegin() + 6,
                        [](const auto &field) { return field.isNull(); }))
            return {};
        members.append(
            {{installation, row[0].toByteArray(), row[1].toByteArray(), row[2].toByteArray()},
             row[3].toByteArray(),
             row[4].toByteArray(),
             row[5].toByteArray(),
             !row[6].isNull(),
             row[7].toByteArray()});
    }
    return members;
}

QByteArray configured_member_document(const StartupConfiguration &config, const StartupLane &lane) {
    return frame(lane.lane_ref) + frame(config.execution_principal) + frame(lane.profile_revision) +
           frame(lane.cursor_contract);
}

QList<PolicyMember> configured_members(const StartupConfiguration &config) {
    QList<PolicyMember> members;
    for (const auto &lane : config.lanes)
        members.append(lane.member);
    return members;
}

bool same_members(const QList<PolicyMember> &expected, const QList<PolicyMember> &retained) {
    return expected.size() == retained.size() &&
           std::ranges::all_of(expected,
                               [&](const auto &member) { return retained.contains(member); });
}

std::optional<StartupSource> read_source(const SqliteWorkSession &session, const Scope &scope) {
    const auto rows = session.execute(
        {"SELECT epoch,revision,policy_revision,generation,baseline_state,cut,frontier,document "
         "FROM main.subathon_intake_sources "
         "WHERE installation=? AND provider=? AND account=? AND stream=?",
         scope_bindings(scope)});
    if (rows.size() != 1)
        return {};
    const auto &r = rows.front();
    return StartupSource{
        scope,
        {r[0].toLongLong(), r[1].toLongLong(), r[2].toLongLong(), r[3].toByteArray()},
        r[4].toInt(),
        optional_blob(r[5]),
        optional_blob(r[6]),
        r[7].toByteArray()};
}

qsizetype source_count(const SqliteWorkSession &session, const QByteArray &installation) {
    const auto rows = session.execute(
        {"SELECT 1 FROM main.subathon_intake_sources WHERE installation=? LIMIT 257",
         {installation}});
    return rows.size();
}

constexpr auto operation_scope = " WHERE installation=? AND provider=? AND account=? AND stream=?";

QList<QVariant> operation_args(const Scope &scope, const QByteArray &key) {
    auto args = scope_bindings(scope);
    args.append(key);
    return args;
}

enum class KeyState { absent, scoped, collision };

KeyState key_state(const SqliteWorkSession &session, const Scope &scope, const QByteArray &key) {
    const auto rows = session.execute(
        {"SELECT provider=? AND account=? AND stream=? FROM main.subathon_intake_operations "
         "WHERE installation=? AND operation_ref=?",
         {scope.provider, scope.account, scope.stream, scope.installation, key}});
    if (rows.isEmpty())
        return KeyState::absent;
    return rows.front().front().toBool() ? KeyState::scoped : KeyState::collision;
}

std::optional<StartupOperation> read_operation(const SqliteWorkSession &session, const Scope &scope,
                                               const QByteArray &key) {
    const auto rows = session.execute(
        {QString("SELECT purpose,epoch,policy_revision,generation,revision,outcome,intent,"
                 "prior_cut,selected_cut,gap_ref,continuation,next_ordinal,slots "
                 "FROM main.subathon_intake_operations") +
             operation_scope + " AND operation_ref=?",
         operation_args(scope, key)});
    if (rows.size() != 1)
        return {};
    const auto &r = rows.front();
    if (!r[9].isNull() || !r[10].isNull() || r[11].toLongLong() != 1 || r[12].toLongLong() != 0)
        return {};
    return StartupOperation{key,
                            scope,
                            r[0].toInt(),
                            r[1].toLongLong(),
                            r[2].toLongLong(),
                            r[3].toByteArray(),
                            r[4].toLongLong(),
                            r[5].toInt(),
                            r[6].toByteArray(),
                            optional_blob(r[7]),
                            optional_blob(r[8])};
}

bool active_history(const SqliteWorkSession &session, const Scope &scope) {
    return !session
                .execute({QString("SELECT 1 FROM main.subathon_intake_operations") +
                              operation_scope + " AND outcome=0 AND purpose IN (0,1) LIMIT 1",
                          scope_bindings(scope)})
                .isEmpty();
}

bool open_gap(const SqliteWorkSession &session, const Scope &scope) {
    return !session
                .execute({QString("SELECT 1 FROM main.subathon_intake_gaps") + operation_scope +
                              " AND status=0 LIMIT 1",
                          scope_bindings(scope)})
                .isEmpty();
}

StartupSource pending_source(const StartupLane &lane) {
    return {lane.member.scope,   {1, 1, 1, lane.enrollment_generation}, 0, {}, {},
            lane.cursor_contract};
}

const ActivationSource &activation_for(const ActivateRequest &request, const Scope &scope) {
    return *std::ranges::find_if(request.sources,
                                 [&](const auto &source) { return source.scope == scope; });
}

CutRequest first_cut_request(const StartupLane &lane) {
    return {{lane.member.scope, {2, 2, 2, lane.planned_generation}, lane.profile_revision},
            CutPurpose::first_connect,
            {}};
}

bool valid_cut(const CutRequest &request, const CutObservation &observation) {
    const auto &cut = observation.cut;
    return same_cut_request(request, observation.request) &&
           (observation.boundary == Endpoint::inclusive ||
            observation.boundary == Endpoint::exclusive) &&
           !cut.format.isEmpty() && !cut.position_namespace.isEmpty() &&
           cut.qualified_generation == request.source.fence.generation && !cut.proof.isEmpty();
}

bool next_revision(qint64 before, qint64 after) {
    return before < std::numeric_limits<qint64>::max() && after == before + 1;
}

std::optional<History> activation_history(const StartupOperation &operation,
                                          const StartupLane &lane) {
    if (operation.purpose != 2 || operation.prior_cut || !operation.cut)
        return {};
    const auto retained = decode_history(operation, "SH1");
    const auto boundary = decode_cut(*operation.cut);
    if (!retained || !boundary || retained->before != head(pending_source(lane)) ||
        retained->after != Head{lane.member.scope, first_cut_request(lane).source.fence, {}} ||
        retained->cursor != lane.cursor_contract || retained->proof != boundary->cut.proof ||
        boundary->cut.qualified_generation != retained->after.fence.generation)
        return {};
    return retained;
}

std::optional<History> reconnect_history(const StartupOperation &operation, const Head &before,
                                         const QByteArray &cut, const QByteArray &cursor) {
    if (operation.purpose != 4 || operation.cut != cut || operation.prior_cut != cut)
        return {};
    const auto retained = decode_history(operation, "RC1");
    if (!retained || retained->before != before || retained->after.frontier ||
        retained->after.fence.epoch != before.fence.epoch ||
        retained->after.fence.policy_revision != before.fence.policy_revision ||
        retained->after.fence.generation == before.fence.generation ||
        !next_revision(before.fence.revision, retained->after.fence.revision) ||
        retained->cursor != cursor)
        return {};
    return retained;
}

struct Chain {
    StartupOperation activation;
    QList<StartupOperation> reconnects;
    Boundary boundary;
    QSet<QByteArray> generations;
};

std::optional<Chain> read_chain(const SqliteWorkSession &session, const CheckpointRequest &request,
                                const StartupLane &lane, const StartupSource &before) {
    const auto start = read_operation(session, request.scope, request.activation_ref);
    if (!start || !before.cut || start->cut != before.cut)
        return {};
    const auto activation = activation_history(*start, lane);
    const auto boundary = decode_cut(*before.cut);
    if (!activation || !boundary)
        return {};
    Chain chain{*start, {}, *boundary, {activation->after.fence.generation}};
    auto prior = activation->after;
    for (const auto &key : request.reconnect_refs) {
        const auto operation = read_operation(session, request.scope, key);
        if (!operation)
            return {};
        const auto link = reconnect_history(*operation, prior, *before.cut, before.cursor_contract);
        if (!link || chain.generations.contains(link->after.fence.generation))
            return {};
        prior = link->after;
        chain.generations.insert(prior.fence.generation);
        chain.reconnects.append(*operation);
    }
    if (prior != head(before))
        return {};
    return chain;
}

LiveProgressRequest live_request(const StartupSource &source, const Chain &chain,
                                 const StartupProfile &profile) {
    return {source.scope,
            source.fence,
            profile.registry_revision,
            profile.profile_revision,
            profile.cursor_contract,
            chain.activation,
            chain.reconnects};
}

bool contiguous(const Boundary &boundary, const LiveProgressObservation &observation) {
    const auto &cut = boundary.cut;
    const auto &lower = observation.lower;
    const auto &upper = observation.upper;
    return observation.coverage == Coverage::contiguous &&
           lower.boundary == (boundary.endpoint == Endpoint::inclusive ? Endpoint::exclusive
                                                                       : Endpoint::inclusive) &&
           upper.boundary == Endpoint::inclusive && lower.format == cut.format &&
           upper.format == cut.format && lower.position_namespace == cut.position_namespace &&
           upper.position_namespace == cut.position_namespace && lower.position == cut.position &&
           lower.position && !lower.position->isEmpty() && upper.position &&
           !upper.position->isEmpty() && !observation.proof.isEmpty();
}

constexpr auto entry_scope = " WHERE installation=? AND provider=? AND account=? AND stream=?";

struct EntryKeys {
    QList<QByteArray> keys;
    bool over_limit{};
    bool missing{};
};

// Cap the unfiltered scope scan first. Recovery instead budgets only the
// original exact keys, including all their raw input and provenance bytes.
EntryKeys entry_keys(const SqliteWorkSession &session, const Scope &scope,
                     const QList<LiveDispositionClaim> *original = nullptr) {
    const auto metadata = QString("SELECT intake_ref,length(input),length(provenance) "
                                  "FROM main.subathon_intake_entries") +
                          entry_scope;
    SqlRows rows;
    if (original) {
        for (const auto &claim : *original) {
            if (claim.scope != scope || !bounded(claim.intake_ref, reference_limit))
                return {{}, false, true};
            const auto exact = session.execute(
                {metadata + " AND intake_ref=?", operation_args(scope, claim.intake_ref)});
            if (exact.size() != 1)
                return {{}, false, true};
            rows.append(exact.front());
        }
    } else {
        rows = session.execute({metadata + " ORDER BY intake_ref LIMIT 17", scope_bindings(scope)});
    }

    if (rows.size() > selection_limit)
        return {{}, true};
    EntryKeys result;
    qint64 bytes = 0;
    for (const auto &row : rows) {
        result.keys.append(row[0].toByteArray());
        bytes += row[1].toLongLong() + row[2].toLongLong();
    }
    result.over_limit = bytes > page_limit;
    return result;
}

bool input_claims_match(const LiveProgressObservation &observation) {
    if (observation.ordered_inputs.size() != observation.dispositions.size())
        return false;
    // The profile attests observation order and keyed claims separately. An
    // input must have a disposition even when multiple inputs encode identically.
    QList<QByteArray> claimed;
    QSet<QByteArray> keys;
    for (const auto &claim : observation.dispositions) {
        if (claim.scope != observation.request.scope || claim.history ||
            !bounded(claim.intake_ref, reference_limit) || keys.contains(claim.intake_ref))
            return false;
        keys.insert(claim.intake_ref);
        claimed.append(claim.normalized);
    }
    return std::ranges::is_permutation(observation.ordered_inputs, claimed);
}

enum EntryColumn {
    epoch_col,
    generation_col,
    policy_col,
    operation_col,
    page_col,
    item_col,
    input_col,
    provenance_col,
    arrival_col,
    phase_col,
    disposition_col,
    charge_col,
    receipt_col,
    attempt_col,
    contribution_col,
    configuration_col,
    context_col,
    activity_namespace_col,
    activity_id_col,
    verification_col
};

bool verified_receipt_disposition(const SqliteWorkSession &session,
                                  const LiveDispositionClaim &claim, const SqlRow &row) {
    const auto locator = *claim.attempt;
    if (locator.receipt.value == 0 || locator.ordinal <= 0 || claim.quarantine_bytes ||
        !row[charge_col].isNull() ||
        row[receipt_col].toByteArray() != encode_receipt_uint64(locator.receipt.value) ||
        row[attempt_col].toLongLong() != locator.ordinal || claim.phase < 2 || claim.phase > 3)
        return false;

    IntakeAdmission admission;
    switch (claim.disposition) {
    case 0:
        admission = IntakeAdmission::creation_eligible_verified_support;
        break;
    case 3:
        admission = IntakeAdmission::audit_only;
        break;
    case 4:
        admission = IntakeAdmission::confirmation_only;
        break;
    default:
        return false;
    }

    // Bound and verify the selected receipt/context closure before reading
    // its exact immutable attempt. The attestation binds the owning locator.
    const auto selected = inspect(session, {claim.scope, claim.intake_ref, false, true});
    if (selected.status != SelectionStatus::selected)
        return false;
    const auto receipt = read_receipt_observation(session, locator.receipt, locator.ordinal);
    if (!receipt || receipt->attempt.arrival.server_timestamp_utc_ms != claim.arrival_ms)
        return false;
    std::optional<VerifiedActivityLink> activity;
    if (!row[activity_namespace_col].isNull()) {
        activity = VerifiedActivityLink{{receipt->original.identity.origin,
                                         row[activity_namespace_col].toByteArray().toStdString(),
                                         row[activity_id_col].toByteArray().toStdString()},
                                        row[verification_col].toByteArray().toStdString()};
    }
    const ManagedInput input{
        claim.scope,
        claim.intake_ref,
        claim.normalized,
        claim.provenance,
        {receipt->original.identity, receipt->attempt.observed_facts, receipt->attempt.arrival},
        activity,
        admission,
        claim.history};
    const auto attempt = read_attempt(session, input, locator);
    if (attempt.status != ManagedStatus::exact_replay || !attempt.selected)
        return false;
    const bool created = attempt.selected->disposition == core::ReceiptDisposition::created;
    if (claim.phase == 2)
        return row[contribution_col].isNull() &&
               (!created || admission == IntakeAdmission::audit_only);

    if (!created || !activity || admission == IntakeAdmission::audit_only)
        return false;
    // Evidence belongs to the contribution owner. Read the exact original or
    // confirmation observation for this receipt and require the linked CID.
    const bool confirmation = admission == IntakeAdmission::confirmation_only;
    const auto observation = session.execute(
        {"SELECT contribution_id FROM main.subathon_contribution_observations WHERE receipt_id=? "
         "AND activity_provider=? AND activity_source_account=? AND activity_namespace=? "
         "AND activity_id=? AND verification_ref=? AND kind=?",
         {row[receipt_col], claim.scope.provider, claim.scope.account, row[activity_namespace_col],
          row[activity_id_col], row[verification_col], confirmation ? 1 : 0}});
    if (observation.size() != 1 || observation.front().front() != row[contribution_col])
        return false;
    if (row[contribution_col].isNull())
        return confirmation;

    // Close over the owning contribution and original observation without
    // reading any intake key besides the one in the attested claim.
    const auto original = session.execute(
        {"SELECT 1 FROM main.subathon_contributions c "
         "JOIN main.subathon_contribution_observations o ON o.receipt_id=c.original_receipt_id "
         "JOIN main.subathon_receipts r ON r.receipt_id=c.original_receipt_id "
         "WHERE c.contribution_id=? AND c.activity_provider=? AND c.activity_source_account=? "
         "AND c.activity_namespace=? AND c.activity_id=? AND o.kind=0 "
         "AND o.contribution_id=c.contribution_id AND o.activity_provider=c.activity_provider "
         "AND o.activity_source_account=c.activity_source_account "
         "AND o.activity_namespace=c.activity_namespace AND o.activity_id=c.activity_id "
         "AND typeof(o.verification_ref)='blob' AND length(o.verification_ref) BETWEEN 1 AND 4096 "
         "AND r.provider=c.activity_provider AND r.source_account=c.activity_source_account "
         "AND (?=1 OR c.original_receipt_id=?)",
         {row[contribution_col], claim.scope.provider, claim.scope.account,
          row[activity_namespace_col], row[activity_id_col], confirmation ? 1 : 0,
          row[receipt_col]}});
    return original.size() == 1;
}

bool disposition_matches(const SqliteWorkSession &session, const Fence &fence,
                         const QSet<QByteArray> &generations, const LiveDispositionClaim &claim,
                         bool replay) {
    if (claim.history || !generations.contains(claim.generation))
        return false;
    const auto selected = session.execute(
        {QString(
             "SELECT epoch,generation,policy_revision,operation_ref,page_ordinal,item_ordinal,"
             "input,provenance,arrival_ms,phase,disposition,quarantine_bytes,receipt_id,"
             "attempt_ordinal,contribution_id,configuration_ref,modifier_context_ref,"
             "activity_namespace,activity_id,verification_ref FROM main.subathon_intake_entries") +
             entry_scope + " AND intake_ref=?",
         operation_args(claim.scope, claim.intake_ref)});
    if (selected.size() != 1)
        return false;
    const auto &row = selected.front();
    if (row[epoch_col].toLongLong() != fence.epoch ||
        row[policy_col].toLongLong() != fence.policy_revision ||
        row[generation_col].toByteArray() != claim.generation || !row[operation_col].isNull() ||
        !row[page_col].isNull() || !row[item_col].isNull() ||
        row[input_col].toByteArray() != claim.normalized ||
        row[provenance_col].toByteArray() != claim.provenance ||
        row[arrival_col].toLongLong() != claim.arrival_ms)
        return false;

    const auto phase = row[phase_col].toInt();
    const auto disposition = row[disposition_col].toInt();
    if (claim.attempt)
        return phase == claim.phase && disposition == claim.disposition &&
               verified_receipt_disposition(session, claim, row);

    const qint64 charge = 9 + claim.normalized.size() + claim.provenance.size();
    if (claim.phase != 2 || claim.disposition != 1 || claim.quarantine_bytes != charge ||
        charge > item_limit ||
        !std::all_of(row.cbegin() + receipt_col, row.cend(),
                     [](const auto &value) { return value.isNull(); }))
        return false;
    return (phase == 2 && disposition == 1 && !row[charge_col].isNull() &&
            row[charge_col].toLongLong() == charge) ||
           (replay && phase == 3 && disposition == 2 && row[charge_col].isNull());
}

bool complete_dispositions(const SqliteWorkSession &session, const Fence &fence, const Chain &chain,
                           const EntryKeys &selection, const LiveProgressObservation &observation,
                           bool replay) {
    if (selection.missing || selection.over_limit || !input_claims_match(observation) ||
        selection.keys.size() != observation.dispositions.size())
        return false;
    // Decision 200: every row selected for a new checkpoint needs an exact
    // eligible disposition. The installed profile has no durable Outside proof.
    for (const auto &claim : observation.dispositions)
        if (!selection.keys.contains(claim.intake_ref) ||
            !disposition_matches(session, fence, chain.generations, claim, replay))
            return false;
    return true;
}

StartupOperation make_operation(const QByteArray &key, const StartupSource &before,
                                const StartupSource &after, const QByteArray &decision,
                                const QByteArray &proof) {
    const auto &fence = after.fence;
    return {key,
            after.scope,
            2,
            fence.epoch,
            fence.policy_revision,
            fence.generation,
            1,
            1,
            QByteArray("SH1") + encode_head(head(before)) + encode_head(head(after)) +
                frame(after.cursor_contract) + frame(decision) + frame(proof),
            before.cut,
            after.cut};
}

struct Plan {
    StartupProjection before;
    Permits permits;
    std::optional<Chain> chain;
    Profile profile{};
};

class RegistryProofs;

} // namespace

class QualifiedFirstCut final {
  public:
    const Profile profile;
    const CutObservation observation;
    const QByteArray document;

  private:
    friend RegistryProofs;
    QualifiedFirstCut(Profile binding, CutObservation observed)
        : profile(std::move(binding)), observation(std::move(observed)),
          document(cut_document(observation)) {}
};

class QualifiedLivePrefix final {
  public:
    const Profile profile;
    const LiveProgressObservation observation;
    const QByteArray document;

  private:
    friend RegistryProofs;
    QualifiedLivePrefix(Profile binding, LiveProgressObservation observed)
        : profile(std::move(binding)), observation(std::move(observed)),
          document(coverage_document(observation)) {}
};

namespace {

struct PreparedCut {
    QByteArray key;
    std::shared_ptr<const QualifiedFirstCut> proof;
};

// Only a selected registry binding and its independent invocation verifier
// can issue a qualification. Request echoes never enter these methods.
class RegistryProofs {
  public:
    static std::shared_ptr<const QualifiedFirstCut> qualify(Profile profile, const Fence &before,
                                                            const CutRequest &request,
                                                            CutObservation observation) {
        if (!valid_cut(request, observation) ||
            !profile->validate_cut(before, request, observation))
            return {};
        return std::shared_ptr<const QualifiedFirstCut>(
            new QualifiedFirstCut(std::move(profile), std::move(observation)));
    }

    static std::shared_ptr<const QualifiedLivePrefix> qualify(Profile profile, const Boundary &cut,
                                                              const LiveProgressRequest &request,
                                                              LiveProgressObservation observation) {
        if (observation.request != request || !contiguous(cut, observation) ||
            !input_claims_match(observation) || !profile->validate_live(request, observation))
            return {};
        return std::shared_ptr<const QualifiedLivePrefix>(
            new QualifiedLivePrefix(std::move(profile), std::move(observation)));
    }
};

} // namespace

class PreparedStartupIntent final {
  public:
    const QByteArray configuration;
    const CatchUpMode mode;
    const QByteArray principal;
    const StartupProjection before;
    const QList<PreparedCut> cuts;
    const std::shared_ptr<const QualifiedLivePrefix> live;
    const QByteArray checkpoint;
};

namespace {

using Preparation = std::shared_ptr<const PreparedStartupIntent>;
using Evaluation = std::variant<StartupResult, Plan>;

bool same_configuration(const StartupConfiguration &config, const PreparedStartupIntent &intent) {
    return config.revision == intent.configuration && config.mode == intent.mode &&
           config.execution_principal == intent.principal;
}

const StartupPermit &permit_for(const Permits &permits, const Scope &scope) {
    return *std::ranges::find_if(permits,
                                 [&](const auto &permit) { return permit.scope == scope; });
}

struct EvaluationContext {
    const SqliteWorkSession &session;
    const Owner &owner;
    bool inspecting;
    qint64 policy_head;
    QList<Scope> scopes;
    Permits permits;
    Preparation prepared;

    std::optional<Status> replay_gate() const {
        if (inspecting)
            return {};
        auto decision = authorize(session, owner, StartupAction::inspect, scopes);
        if (const auto *failure = std::get_if<Status>(&decision))
            return *failure;
        return {};
    }
};

Evaluation evaluate(const EvaluationContext &context, const RegisterRequest &request) {
    const auto &session = context.session;
    const auto &config = context.owner.configuration;
    if (context.prepared)
        return StartupResult{Status::conflict};
    const auto members = configured_members(config);
    if (context.policy_head == 0) {
        if (source_count(session, config.installation) != 0)
            return StartupResult{Status::unavailable};
        for (const auto &lane : config.lanes)
            if (lane.member.document != configured_member_document(config, lane))
                return StartupResult{Status::invalid_input};
        const auto document = policy_document(config, context.permits.front());
        if (request.policy_document && *request.policy_document != document)
            return StartupResult{Status::invalid_input};
        if (context.inspecting)
            return stale(true, StartupReadback::absent_at_before);
        return Plan{{0, {}, members, {}, {}}, context.permits, {}};
    }
    if (context.policy_head != 1)
        return stale(context.inspecting);
    if (const auto refusal = context.replay_gate())
        return StartupResult{*refusal};

    const auto document = read_policy(session, config.installation, 1);
    const auto retained_policy = decode_policy(document);
    const auto retained_members = read_members(session, config.installation, 1);
    if (!retained_policy || !retained_members ||
        source_count(session, config.installation) != retained_members->size())
        return StartupResult{Status::unavailable};
    if (retained_policy->configuration != config.revision ||
        retained_policy->mode != mode_token(config.mode) ||
        !same_members(members, *retained_members) ||
        std::ranges::any_of(config.lanes, [&](const auto &lane) {
            return lane.member.document != configured_member_document(config, lane);
        }))
        return StartupResult{Status::conflict};
    if (request.policy_document && *request.policy_document != document)
        return StartupResult{Status::invalid_input};

    StartupProjection projection{1, document, members, {}, {}};
    for (const auto &lane : config.lanes) {
        const auto source = read_source(session, lane.member.scope);
        if (!source)
            return StartupResult{Status::unavailable};
        if (*source != pending_source(lane))
            return StartupResult{source->fence.epoch == 1 && source->fence.revision == 1 &&
                                         source->fence.policy_revision == 1 && source->state == 0 &&
                                         !source->cut && !source->frontier
                                     ? Status::conflict
                                     : Status::unavailable};
        projection.sources.append(*source);
    }
    return applied(context.inspecting, std::move(projection));
}

Evaluation replay_activation(const EvaluationContext &context, const ActivateRequest &request,
                             StartupProjection projection, const QList<StartupSource> &before) {
    const auto &session = context.session;
    const auto &config = context.owner.configuration;
    const auto retained_policy = decode_policy(projection.policy);
    if (!retained_policy)
        return StartupResult{Status::unavailable};
    if (retained_policy->configuration != config.revision ||
        retained_policy->mode != mode_token(config.mode))
        return StartupResult{Status::conflict};
    if (request.mode && *request.mode != config.mode)
        return StartupResult{Status::invalid_input};

    for (qsizetype i = 0; i < config.lanes.size(); ++i) {
        const auto &lane = config.lanes[i];
        const auto &source = projection.sources[i];
        const auto &entry = activation_for(request, lane.member.scope);
        const auto record = read_operation(session, entry.scope, entry.history_ref);
        if (!record)
            return StartupResult{Status::unavailable};
        const auto history = activation_history(*record, lane);
        if (!history || history->before != head(before[i]) || history->after != head(source) ||
            record->cut != source.cut || (i == 0 && history->decision != retained_policy->decision))
            return StartupResult{Status::unavailable};

        const auto profile = select_profile(context.owner.profiles, lane, source.fence.generation);
        const auto boundary = decode_cut(*record->cut);
        const auto cut_request = first_cut_request(lane);
        const CutObservation observation{cut_request, boundary->cut, boundary->endpoint};
        if (!profile || !profile->attest_cut || exceeds_cut(observation) ||
            !profile->attest_cut(before[i].fence, cut_request, observation))
            return StartupResult{Status::unavailable};
        if (context.prepared) {
            const auto &intent = *context.prepared;
            if (!same_configuration(config, intent) || intent.before.sources != before ||
                intent.cuts.size() != config.lanes.size() || intent.live ||
                intent.cuts[i].key != entry.history_ref ||
                intent.cuts[i].proof->profile->registry_revision != profile->registry_revision ||
                intent.cuts[i].proof->document != record->cut ||
                !same_cut(intent.cuts[i].proof->observation, observation))
                return StartupResult{Status::conflict};
        }
        if (entry.candidate && !same_cut(*entry.candidate, observation))
            return StartupResult{Status::invalid_input};
        projection.operations.append(*record);
    }
    return applied(context.inspecting, std::move(projection));
}

Evaluation evaluate(const EvaluationContext &context, const ActivateRequest &request) {
    const auto &session = context.session;
    const auto &config = context.owner.configuration;
    if (request.expected_policy_head != 1 || context.policy_head < 1 || context.policy_head > 2 ||
        request.configuration_revision != config.revision)
        return stale(context.inspecting);

    const bool replay = context.policy_head == 2;
    if (replay) {
        qsizetype found = 0;
        for (const auto &entry : request.sources)
            found += key_state(session, entry.scope, entry.history_ref) == KeyState::scoped;
        if (found == 0)
            return stale(context.inspecting);
        if (const auto refusal = context.replay_gate())
            return StartupResult{*refusal};
        if (found != request.sources.size())
            return StartupResult{Status::unavailable};
    } else if (request.mode && *request.mode != config.mode) {
        return StartupResult{Status::invalid_input};
    }

    auto members = configured_members(config);
    QList<StartupSource> before, current;
    for (const auto &lane : config.lanes) {
        const auto &entry = activation_for(request, lane.member.scope);
        auto pending = pending_source(lane);
        const auto source = read_source(session, entry.scope);
        if (entry.expected != pending.fence)
            return stale(context.inspecting);
        if (!source)
            return StartupResult{Status::unavailable};
        if (replay) {
            if (source->fence != first_cut_request(lane).source.fence || source->frontier)
                return stale(context.inspecting);
            if (source->state != 1 || !source->cut ||
                source->cursor_contract != lane.cursor_contract)
                return StartupResult{Status::unavailable};
        } else {
            if (source->fence != pending.fence)
                return stale(context.inspecting);
            if (*source != pending)
                return StartupResult{Status::unavailable};
        }
        if (entry.successor_generation != lane.planned_generation)
            return StartupResult{Status::invalid_input};
        before.append(std::move(pending));
        current.append(*source);
    }
    if (source_count(session, config.installation) != config.lanes.size())
        return StartupResult{Status::unavailable};

    const auto enrolled_document = read_policy(session, config.installation, 1);
    const auto enrolled = decode_policy(enrolled_document);
    const auto enrolled_members = read_members(session, config.installation, 1);
    if (!enrolled || !enrolled_members)
        return StartupResult{Status::unavailable};
    if (enrolled->configuration != config.revision || !same_members(members, *enrolled_members) ||
        std::ranges::any_of(config.lanes, [&](const auto &lane) {
            return lane.member.document != configured_member_document(config, lane);
        }))
        return replay ? StartupResult{Status::conflict} : stale(context.inspecting);

    if (replay) {
        const auto document = read_policy(session, config.installation, 2);
        const auto retained = read_members(session, config.installation, 2);
        if (!retained || !same_members(members, *retained))
            return StartupResult{Status::unavailable};
        return replay_activation(context, request, {2, document, members, current, {}}, before);
    }
    for (const auto &entry : request.sources) {
        if (key_state(session, entry.scope, entry.history_ref) != KeyState::absent)
            return StartupResult{Status::conflict};
        if (active_history(session, entry.scope))
            return StartupResult{Status::busy};
        if (open_gap(session, entry.scope))
            return StartupResult{Status::unproved};
    }
    if (context.inspecting) {
        if (context.prepared &&
            (!same_configuration(config, *context.prepared) ||
             context.prepared->before !=
                 StartupProjection{1, enrolled_document, members, before, {}} ||
             context.prepared->live || context.prepared->cuts.size() != before.size()))
            return StartupResult{Status::conflict};
        if (context.prepared)
            for (qsizetype i = 0; i < before.size(); ++i) {
                const auto &lane = config.lanes[i];
                const auto &entry = activation_for(request, lane.member.scope);
                const auto &cut = context.prepared->cuts[i];
                const auto profile =
                    select_profile(context.owner.profiles, lane, lane.planned_generation);
                if (cut.key != entry.history_ref || !profile ||
                    cut.proof->profile->registry_revision != profile->registry_revision ||
                    (entry.candidate && !same_cut(*entry.candidate, cut.proof->observation)))
                    return StartupResult{Status::conflict};
            }
        return stale(true, StartupReadback::absent_at_before);
    }
    return Plan{
        {1, enrolled_document, std::move(members), std::move(before), {}}, context.permits, {}};
}

bool attest_activation(const StartupLane &lane, const Chain &chain, const StartupProfile &profile) {
    const auto request = first_cut_request(lane);
    return profile.attest_cut &&
           profile.attest_cut(pending_source(lane).fence, request,
                              {request, chain.boundary.cut, chain.boundary.endpoint});
}

Evaluation replay_checkpoint(const EvaluationContext &context, const CheckpointRequest &request,
                             StartupProjection projection, const StartupLane &lane) {
    const auto &after = projection.sources.front();
    const auto record = read_operation(context.session, request.scope, request.checkpoint_ref);
    if (!record || record->purpose != 2 || !record->cut || record->prior_cut != record->cut ||
        record->cut != after.cut)
        return StartupResult{Status::unavailable};
    const auto history = decode_history(*record, "SH1");
    if (!history || history->before.frontier || history->after != head(after) ||
        history->before.fence != request.expected || history->cursor != after.cursor_contract)
        return StartupResult{Status::unavailable};
    const auto retained = decode_coverage(history->proof);
    if (!retained || retained->upper.position != after.frontier)
        return StartupResult{Status::unavailable};

    auto before = after;
    before.fence = history->before.fence;
    before.frontier.reset();
    const auto chain = read_chain(context.session, request, lane, before);
    if (!chain)
        return StartupResult{Status::unavailable};
    const auto profile = select_profile(context.owner.profiles, lane, before.fence.generation);
    if (!profile || !profile->resolve_retained_live || !profile->attest_live)
        return StartupResult{Status::unavailable};
    if (!context.inspecting && ((context.prepared && context.prepared->live &&
                                 context.prepared->live->profile != profile) ||
                                (request.qualified && request.qualified->profile != profile)))
        return stale(false);
    if (!attest_activation(lane, *chain, *profile))
        return StartupResult{Status::unavailable};

    // Both gates have passed and every retained operation is selected by its
    // original key. Only the exact proof can authenticate the registry revision.
    const auto original = live_request(before, *chain, *profile);
    const auto observation = profile->resolve_retained_live(original, retained->proof);
    if (!observation || exceeds_live(*observation) || observation->request != original ||
        observation->lower != retained->lower || observation->upper != retained->upper ||
        observation->proof != retained->proof || !contiguous(chain->boundary, *observation) ||
        coverage_document(*observation) != history->proof)
        return StartupResult{Status::unavailable};

    const auto keys = entry_keys(context.session, request.scope, &observation->dispositions);
    if (keys.missing || keys.over_limit || !profile->attest_live(original, *observation) ||
        !complete_dispositions(context.session, before.fence, *chain, keys, *observation, true))
        return StartupResult{Status::unavailable};

    if (context.prepared) {
        const auto &intent = *context.prepared;
        if (!same_configuration(context.owner.configuration, intent) || !intent.live ||
            intent.checkpoint != request.checkpoint_ref ||
            intent.before.sources != QList<StartupSource>{before} ||
            intent.before.policy != projection.policy ||
            intent.before.members != projection.members ||
            intent.live->document != history->proof ||
            !same_live(intent.live->observation, *observation))
            return StartupResult{Status::conflict};
    }
    if (request.qualified && !same_live(request.qualified->observation, *observation))
        return StartupResult{Status::conflict};
    if (request.candidate && !same_live(*request.candidate, *observation))
        return StartupResult{Status::invalid_input};

    projection.operations = {chain->activation};
    projection.operations.append(chain->reconnects);
    projection.operations.append(*record);
    return applied(context.inspecting, std::move(projection));
}

Evaluation evaluate(const EvaluationContext &context, const CheckpointRequest &request) {
    const auto &session = context.session;
    const auto &config = context.owner.configuration;
    const auto &lane = *lane_for(config, request.scope);
    if (request.configuration_revision != config.revision ||
        context.policy_head != request.expected.policy_revision ||
        context.permits.front().principal != config.execution_principal)
        return stale(context.inspecting);
    const auto key = key_state(session, request.scope, request.checkpoint_ref);
    if (key == KeyState::scoped)
        if (const auto refusal = context.replay_gate())
            return StartupResult{*refusal};

    const auto source = read_source(session, request.scope);
    if (!source)
        return stale(context.inspecting);
    const bool at_before = source->fence == request.expected && !source->frontier;
    const bool at_after = source->fence.epoch == request.expected.epoch &&
                          source->fence.policy_revision == request.expected.policy_revision &&
                          source->fence.generation == request.expected.generation &&
                          next_revision(request.expected.revision, source->fence.revision) &&
                          source->frontier;
    const bool replay = key == KeyState::scoped && at_after;
    if (!at_before && !replay)
        return stale(context.inspecting);

    const auto document = read_policy(session, config.installation, context.policy_head);
    const auto policy = decode_policy(document);
    const auto members =
        read_members(session, config.installation, context.policy_head, &request.scope);
    if (!policy || !members || members->isEmpty())
        return StartupResult{Status::unavailable};
    if (policy->configuration != config.revision || policy->mode != mode_token(config.mode) ||
        members->front() != lane.member ||
        members->front().document != configured_member_document(config, lane) ||
        source->cursor_contract != lane.cursor_contract)
        return stale(context.inspecting);

    if (source->state != 1 || !source->cut)
        return StartupResult{replay ? Status::unavailable : Status::unproved};
    StartupProjection projection{context.policy_head, document, *members, {*source}, {}};
    if (replay)
        return replay_checkpoint(context, request, std::move(projection), lane);

    Profile profile;
    if (!context.inspecting || context.prepared) {
        profile = select_profile(context.owner.profiles, lane, source->fence.generation);
        if (!profile ||
            (!context.inspecting && context.prepared && context.prepared->live &&
             context.prepared->live->profile != profile) ||
            (!context.inspecting && request.qualified && request.qualified->profile != profile))
            return stale(context.inspecting);
    }
    if (key == KeyState::scoped)
        return StartupResult{Status::unavailable};
    if (key == KeyState::collision)
        return StartupResult{Status::conflict};
    if (source->fence.revision == std::numeric_limits<qint64>::max())
        return StartupResult{Status::limit_exceeded};

    const auto chain = read_chain(session, request, lane, *source);
    if (!chain)
        return StartupResult{Status::unproved};
    projection.operations.append(chain->activation);
    projection.operations.append(chain->reconnects);

    if (context.inspecting) {
        if (context.prepared) {
            const auto &intent = *context.prepared;
            if (!same_configuration(config, intent) || !intent.live ||
                intent.checkpoint != request.checkpoint_ref || intent.before != projection ||
                !profile ||
                intent.live->observation.request != live_request(*source, *chain, *profile) ||
                (request.candidate && !same_live(*request.candidate, intent.live->observation)))
                return StartupResult{Status::conflict};
        }
        return stale(true, StartupReadback::absent_at_before);
    }
    return Plan{std::move(projection), context.permits, chain, std::move(profile)};
}

Evaluation evaluate(const SqliteWorkSession &session, const Owner &owner, const Transition &request,
                    bool inspecting, Preparation prepared) {
    const auto &config = owner.configuration;
    const auto scopes = authorized_scopes(config, request);
    auto authorization = authorize(session, owner, action_for(request), scopes);
    if (const auto *status = std::get_if<Status>(&authorization))
        return StartupResult{*status};
    if (inspecting) {
        const auto inspection = authorize(session, owner, StartupAction::inspect, scopes);
        if (const auto *status = std::get_if<Status>(&inspection))
            return StartupResult{*status};
    }
    if (const auto invalid = configuration_shape(config))
        return StartupResult{*invalid};
    if (const auto invalid =
            std::visit([&](const auto &value) { return request_shape(config, value); }, request))
        return StartupResult{*invalid};
    if (!detail::local_installation(session, config.installation))
        return StartupResult{Status::unavailable};

    EvaluationContext context{session,
                              owner,
                              inspecting,
                              policy_head(session, config.installation),
                              scopes,
                              std::get<Permits>(std::move(authorization)),
                              std::move(prepared)};
    return std::visit([&](const auto &value) { return evaluate(context, value); }, request);
}

StartupProjection registration_projection(const StartupConfiguration &config, const Plan &plan) {
    StartupProjection projection{
        1, policy_document(config, plan.permits.front()), plan.before.members, {}, {}};
    for (const auto &lane : config.lanes)
        projection.sources.append(pending_source(lane));
    return projection;
}

StartupProjection activation_projection(const StartupConfiguration &config, const Plan &plan,
                                        const QList<PreparedCut> &cuts) {
    StartupProjection projection{
        2, policy_document(config, plan.permits.front()), plan.before.members, {}, {}};
    for (qsizetype i = 0; i < cuts.size(); ++i) {
        const auto &cut = cuts[i];
        const auto &before = plan.before.sources[i];
        StartupSource after{before.scope,
                            cut.proof->observation.request.source.fence,
                            1,
                            cut.proof->document,
                            {},
                            before.cursor_contract};
        projection.operations.append(make_operation(cut.key, before, after,
                                                    permit_for(plan.permits, before.scope).decision,
                                                    cut.proof->observation.cut.proof));
        projection.sources.append(std::move(after));
    }
    return projection;
}

StartupProjection checkpoint_projection(const Plan &plan, const QByteArray &key,
                                        const QualifiedLivePrefix &prefix) {
    auto projection = plan.before;
    const auto &before = plan.before.sources.front();
    auto &after = projection.sources.front();
    ++after.fence.revision;
    after.frontier = prefix.observation.upper.position;
    projection.operations.append(
        make_operation(key, before, after, plan.permits.front().decision, prefix.document));
    return projection;
}

bool documents_fit(const StartupProjection &projection) {
    return projection.policy.size() <= document_limit &&
           std::ranges::none_of(projection.operations, exceeds_operation);
}

using PreparedResult = std::variant<Status, Preparation>;

PreparedResult prepare(const Owner &owner, const ActivateRequest &request, const Plan &plan) {
    const auto &config = owner.configuration;
    QList<PreparedCut> cuts;
    for (qsizetype i = 0; i < config.lanes.size(); ++i) {
        const auto &lane = config.lanes[i];
        const auto &entry = activation_for(request, lane.member.scope);
        if (!lane.fresh_lane || lane.enrollment_generation == lane.planned_generation)
            return Status::unproved;
        auto profile = select_profile(owner.profiles, lane, lane.planned_generation);
        if (!profile || !profile->validate_cut)
            return Status::unproved;
        const auto expected = first_cut_request(lane);
        if (encode_head({expected.source.scope, expected.source.fence, {}}).size() +
                frame(expected.source.profile).size() + 1 >
            document_limit)
            return Status::limit_exceeded;
        auto observed = profile->history.observe_cut(expected);
        if (const auto *failure = std::get_if<HistoryFailure>(&observed))
            return history_failure(*failure);
        auto &observation = std::get<CutObservation>(observed);
        if (exceeds_cut(observation))
            return Status::limit_exceeded;
        auto proof = RegistryProofs::qualify(std::move(profile), plan.before.sources[i].fence,
                                             expected, std::move(observation));
        if (!proof)
            return Status::unproved;
        if (entry.candidate && !same_cut(*entry.candidate, proof->observation))
            return Status::invalid_input;
        cuts.append({entry.history_ref, std::move(proof)});
    }
    if (!documents_fit(activation_projection(config, plan, cuts)))
        return Status::limit_exceeded;
    return std::make_shared<const PreparedStartupIntent>(config.revision, config.mode,
                                                         config.execution_principal, plan.before,
                                                         std::move(cuts), nullptr, QByteArray{});
}

PreparedResult prepare(const Owner &owner, const CheckpointRequest &request, const Plan &plan) {
    const auto &config = owner.configuration;
    const auto &lane = *lane_for(config, request.scope);
    const auto &source = plan.before.sources.front();
    const auto &chain = *plan.chain;
    auto profile = plan.profile;
    if (!attest_activation(lane, chain, *profile))
        return Status::unproved;

    const auto expected = live_request(source, chain, *profile);
    auto prefix = request.qualified;
    if (prefix) {
        if (prefix->profile != profile || prefix->observation.request != expected)
            return Status::stale;
        if (exceeds_live(prefix->observation))
            return Status::limit_exceeded;
    } else {
        if (!profile->validate_live)
            return Status::unproved;
        auto observed = profile->live.observe_progress(expected);
        if (const auto *failure = std::get_if<HistoryFailure>(&observed))
            return history_failure(*failure);
        auto &observation = std::get<LiveProgressObservation>(observed);
        if (exceeds_live(observation))
            return Status::limit_exceeded;
        prefix = RegistryProofs::qualify(std::move(profile), chain.boundary, expected,
                                         std::move(observation));
        if (!prefix)
            return Status::unproved;
    }
    if (request.candidate && !same_live(*request.candidate, prefix->observation))
        return Status::invalid_input;
    if (!documents_fit(checkpoint_projection(plan, request.checkpoint_ref, *prefix)))
        return Status::limit_exceeded;

    return std::make_shared<const PreparedStartupIntent>(
        config.revision, config.mode, config.execution_principal, plan.before, QList<PreparedCut>{},
        std::move(prefix), request.checkpoint_ref);
}

void insert_policy(const SqliteWorkSession &session, const QByteArray &installation,
                   const StartupProjection &projection) {
    const auto revision = projection.policy_head;
    (void)session.execute({"INSERT INTO main.subathon_intake_policies VALUES(?,?,?)",
                           {installation, revision, projection.policy}});
    detail::require_changed_one(session);
    for (const auto &member : projection.members) {
        (void)session.execute(
            {"INSERT INTO main.subathon_intake_members VALUES(?,?,?,?,?,?,?,?,?,?)",
             {installation, revision, member.scope.provider, member.scope.account,
              member.scope.stream, member.award_class, member.effect_scope, member.overlap_set,
              member.primary ? QVariant{1} : QVariant{}, member.document}});
        detail::require_changed_one(session);
    }
}

void insert_source(const SqliteWorkSession &session, const StartupSource &source) {
    auto args = scope_bindings(source.scope);
    args.append({source.fence.epoch, source.fence.generation, source.fence.revision,
                 source.fence.policy_revision, source.state, sql_blob(source.cut),
                 sql_blob(source.frontier), source.cursor_contract});
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_sources VALUES(?,?,?,?,?,?,?,?,?,?,?,?)", args});
    detail::require_changed_one(session);
}

void insert_operation(const SqliteWorkSession &session, const StartupOperation &operation) {
    const auto &scope = operation.scope;
    (void)session.execute(
        {"INSERT INTO main.subathon_intake_operations "
         "VALUES(?,?,?,?,?,?,?,NULL,?,?,?,?,NULL,1,0,1,?,?)",
         {scope.installation, operation.key, scope.provider, scope.account, scope.stream,
          operation.epoch, operation.policy_revision, operation.purpose, operation.generation,
          operation.revision, operation.intent, sql_blob(operation.prior_cut),
          sql_blob(operation.cut)}});
    detail::require_changed_one(session);
}

void activate_source(const SqliteWorkSession &session, const StartupSource &before,
                     const StartupSource &after) {
    QList<QVariant> args{after.fence.epoch, after.fence.generation, after.fence.revision,
                         after.fence.policy_revision, sql_blob(after.cut)};
    args.append(scope_bindings(before.scope));
    args.append({before.fence.epoch, before.fence.generation, before.fence.revision,
                 before.fence.policy_revision, before.cursor_contract});
    (void)session.execute(
        {"UPDATE main.subathon_intake_sources SET "
         "epoch=?,generation=?,revision=?,policy_revision=?,"
         "baseline_state=1,cut=? WHERE installation=? AND provider=? AND account=? AND stream=? "
         "AND epoch=? AND generation=? AND revision=? AND policy_revision=? AND document=? "
         "AND baseline_state=0 AND cut IS NULL AND frontier IS NULL",
         args});
    detail::require_changed_one(session);
}

void advance_frontier(const SqliteWorkSession &session, const StartupSource &before,
                      const StartupSource &after) {
    QList<QVariant> args{sql_blob(after.frontier), after.fence.revision};
    args.append(scope_bindings(before.scope));
    args.append({before.fence.epoch, before.fence.generation, before.fence.revision,
                 before.fence.policy_revision, before.cursor_contract, sql_blob(before.cut)});
    (void)session.execute(
        {"UPDATE main.subathon_intake_sources SET frontier=?,revision=? WHERE installation=? "
         "AND provider=? AND account=? AND stream=? AND epoch=? AND generation=? AND revision=? "
         "AND policy_revision=? AND document=? AND cut=? AND baseline_state=1 AND frontier IS NULL",
         args});
    detail::require_changed_one(session);
}

StartupResult stage(const SqliteWorkSession &session, const Owner &owner,
                    const RegisterRequest &request, const Plan &plan, const Preparation &) {
    auto projection = registration_projection(owner.configuration, plan);
    if (request.policy_document && *request.policy_document != projection.policy)
        return {Status::invalid_input};
    if (!documents_fit(projection))
        return {Status::limit_exceeded};
    insert_policy(session, request.installation, projection);
    for (const auto &source : projection.sources)
        insert_source(session, source);
    return {Status::committed, std::move(projection)};
}

StartupResult stage(const SqliteWorkSession &session, const Owner &owner, const ActivateRequest &,
                    const Plan &plan, const Preparation &prepared) {
    const auto &config = owner.configuration;
    if (!prepared || !same_configuration(config, *prepared) || prepared->before != plan.before)
        return {Status::stale};
    for (qsizetype i = 0; i < prepared->cuts.size(); ++i) {
        const auto &lane = config.lanes[i];
        if (!lane.fresh_lane || select_profile(owner.profiles, lane, lane.planned_generation) !=
                                    prepared->cuts[i].proof->profile)
            return {Status::stale};
    }

    auto projection = activation_projection(config, plan, prepared->cuts);
    if (!documents_fit(projection))
        return {Status::limit_exceeded};
    insert_policy(session, config.installation, projection);
    for (qsizetype i = 0; i < projection.sources.size(); ++i) {
        activate_source(session, plan.before.sources[i], projection.sources[i]);
        insert_operation(session, projection.operations[i]);
    }
    return {Status::committed, std::move(projection)};
}

StartupResult stage(const SqliteWorkSession &session, const Owner &owner,
                    const CheckpointRequest &request, const Plan &plan,
                    const Preparation &prepared) {
    const auto &config = owner.configuration;
    const auto &before = plan.before.sources.front();
    if (!prepared || !prepared->live || !same_configuration(config, *prepared) ||
        prepared->before != plan.before || plan.profile != prepared->live->profile)
        return {Status::stale};
    const auto keys = entry_keys(session, request.scope);
    if (keys.over_limit)
        return {Status::limit_exceeded};
    if (active_history(session, request.scope))
        return {Status::busy};
    if (open_gap(session, request.scope) ||
        !complete_dispositions(session, before.fence, *plan.chain, keys,
                               prepared->live->observation, false))
        return {Status::unproved};

    auto projection = checkpoint_projection(plan, request.checkpoint_ref, *prepared->live);
    if (!documents_fit(projection))
        return {Status::limit_exceeded};
    advance_frontier(session, before, projection.sources.front());
    insert_operation(session, projection.operations.back());
    return {Status::committed, std::move(projection)};
}

bool acknowledged(const SqliteWorkResult &work) {
    return work.transaction == TransactionOutcome::committed && work.error == WorkError::none &&
           work.value.has_value();
}

class Submission final : public std::enable_shared_from_this<Submission> {
  public:
    Submission(Owner services, StartupSelector selection, StartupCompletion completion,
               StartupFaults hooks, bool read_only)
        : owner(std::move(services)), selector(std::move(selection)),
          complete(std::move(completion)), faults(std::move(hooks)), inspecting(read_only) {}

    StorageError enqueue_preliminary() {
        auto self = shared_from_this();
        return owner.writer.enqueue_work(
            [self](const auto &context) -> QVariant {
                const auto session = context.begin();
                self->preliminary.emplace(evaluate(session, self->owner, self->selector.transition,
                                                   self->inspecting, self->selector.prepared));
                session.commit();
                return true;
            },
            [self](SqliteWorkResult work) { self->after_preliminary(std::move(work)); });
    }

  private:
    void finish(StartupOutcome outcome) {
        if (auto callback = std::exchange(complete, {}))
            callback(std::move(outcome));
    }

    void refuse(Status status) { finish({StorageError::none, {}, StartupResult{status}}); }

    void after_preliminary(SqliteWorkResult work) {
        if (!acknowledged(work)) {
            finish({StorageError::none, std::move(work), {}});
            return;
        }
        if (auto *result = std::get_if<StartupResult>(&*preliminary)) {
            finish({StorageError::none, std::move(work), std::move(*result)});
            return;
        }

        if (std::holds_alternative<RegisterRequest>(selector.transition)) {
            resume();
            return;
        }

        const auto &plan = std::get<Plan>(*preliminary);
        auto preparation = [&]() -> PreparedResult {
            try {
                if (const auto *activation = std::get_if<ActivateRequest>(&selector.transition))
                    return prepare(owner, *activation, plan);
                return prepare(owner, std::get<CheckpointRequest>(selector.transition), plan);
            } catch (...) {
                return Status::unavailable;
            }
        }();
        if (const auto *status = std::get_if<Status>(&preparation)) {
            refuse(*status);
            return;
        }

        selector.prepared = std::get<Preparation>(std::move(preparation));
        if (faults.capture_prepared_intent)
            faults.capture_prepared_intent(selector.prepared);
        if (!faults.on_prepared) {
            resume();
            return;
        }
        try {
            faults.on_prepared([self = shared_from_this()] { self->resume(); });
        } catch (...) {
            if (!std::exchange(resumed, true))
                refuse(Status::unavailable);
        }
    }

    void retire_fault_writer() {
        (void)owner.writer.shutdown([] {});
    }

    void resume() {
        if (QThread::currentThread() != owner.writer.thread() || std::exchange(resumed, true))
            return;

        // No queue pump or provider calls between arming and the sole
        // decisive enqueue. A non-mutation can never consume the armed fault.
        if (faults.lose_commit_ack) {
            using testing::SqliteWriterWorkAccess;
            using testing::WorkControlFault;
            const auto admission = SqliteWriterWorkAccess::inject_next_control_ack_loss(
                owner.writer, WorkControlFault::lose_next_commit_ack);
            if (admission != StorageError::none) {
                finish({admission, {}, {}});
                return;
            }
        }

        auto self = shared_from_this();
        const auto admission = owner.writer.enqueue_work(
            [self](const auto &context) -> QVariant { return self->decide(context.begin()); },
            [self](SqliteWorkResult work) {
                std::optional<StartupResult> result;
                if (acknowledged(work))
                    result = std::move(self->decision);
                else if (self->faults.lose_commit_ack)
                    self->retire_fault_writer();
                self->finish({StorageError::none, std::move(work), std::move(result)});
            });
        if (admission != StorageError::none) {
            if (faults.lose_commit_ack)
                retire_fault_writer();
            finish({admission, {}, {}});
        }
    }

    QVariant decide(const SqliteWorkSession &session) {
        auto evaluated = evaluate(session, owner, selector.transition, false, selector.prepared);
        if (auto *result = std::get_if<StartupResult>(&evaluated)) {
            decision = std::move(*result);
        } else {
            const auto &plan = std::get<Plan>(evaluated);
            decision = std::visit(
                [&](const auto &request) {
                    return stage(session, owner, request, plan, selector.prepared);
                },
                selector.transition);
        }

        const bool staged = decision->status == Status::committed;
        if ((!staged && faults.lose_commit_ack) ||
            (staged && faults.before_commit && !faults.before_commit())) {
            session.rollback();
            return {};
        }
        session.commit();
        if (staged && faults.after_commit)
            faults.after_commit();
        return true;
    }

    Owner owner;
    StartupSelector selector;
    StartupCompletion complete;
    StartupFaults faults;
    bool inspecting;
    bool resumed{};
    std::optional<Evaluation> preliminary;
    std::optional<StartupResult> decision;
};

StorageError submit(StartupOwner &owner, StartupSelector selector, StartupCompletion complete,
                    StartupFaults faults, bool inspecting) {
    if (!complete)
        return StorageError::invalid_work;
    // Borrow the owner services, without retaining the caller's entry wrapper.
    auto submission = std::make_shared<Submission>(
        Owner{owner.writer, owner.configuration, owner.profiles, owner.authority, owner.binding},
        std::move(selector), std::move(complete), std::move(faults), inspecting);
    return submission->enqueue_preliminary();
}

} // namespace

StorageError register_first(StartupOwner &owner, RegisterRequest request,
                            StartupCompletion complete, StartupFaults faults) {
    return submit(owner, {std::move(request)}, std::move(complete), std::move(faults), false);
}

StorageError activate_first(StartupOwner &owner, ActivateRequest request,
                            StartupCompletion complete, StartupFaults faults) {
    return submit(owner, {std::move(request)}, std::move(complete), std::move(faults), false);
}

StorageError checkpoint_first(StartupOwner &owner, CheckpointRequest request,
                              StartupCompletion complete, StartupFaults faults) {
    return submit(owner, {std::move(request)}, std::move(complete), std::move(faults), false);
}

StorageError inspect_startup(StartupOwner &owner, StartupSelector selector,
                             StartupCompletion complete) {
    return submit(owner, std::move(selector), std::move(complete), {}, true);
}

} // namespace subathon::server::connector_intake
