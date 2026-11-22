// SPDX-License-Identifier: GPL-3.0-or-later
#include "rate_credentials.hpp"

#include "credential_repository_internal.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

namespace subathon::server::rate_credentials {
namespace {
namespace storage = credential_detail;
using Bytes = std::span<const unsigned char>;
constexpr std::size_t ref_limit = 4096;
constexpr std::array<unsigned char, 4> magic{'F', 'X', 'R', '1'};
constexpr std::size_t prefix_size = magic.size() + Revision{}.size();
constexpr std::size_t app_id_size = 32;
constexpr auto exchange_rate_api = KeyedRateProvider::exchange_rate_api;
constexpr auto open_exchange_rates = KeyedRateProvider::open_exchange_rates;
static_assert(prefix_size == 28);

bool valid_provider(KeyedRateProvider provider) {
    return provider == exchange_rate_api || provider == open_exchange_rates;
}

bool oversized_scope(const RateCredentialScope &scope) {
    return std::max({scope.installation.size(), scope.account.size(), scope.slot.size()}) >
           ref_limit;
}

bool valid_scope(const RateCredentialScope &scope) {
    return valid_provider(scope.provider) && !scope.installation.empty() &&
           !scope.account.empty() && !scope.slot.empty();
}

bool oversized_reservation(const Reservation &reservation) {
    const auto &s = reservation.selection;
    return oversized_scope(s.scope) ||
           std::max({s.generation.size(), s.configuration.size(), s.selection.size(),
                     s.quota_scope.size(), reservation.request.size()}) > ref_limit;
}

bool valid_selection(const RateCredentialSelection &s) {
    if (!valid_scope(s.scope) || s.generation.empty() || s.configuration.empty() ||
        s.selection.empty() || s.quota_scope.empty())
        return false;
    if (s.scope.provider == exchange_rate_api) {
        const auto *profile = std::get_if<era::Profile>(&s.profile);
        return profile && (*profile == era::Profile::free || *profile == era::Profile::pro ||
                           *profile == era::Profile::business || *profile == era::Profile::volume);
    }
    const auto *profile = std::get_if<oer::Profile>(&s.profile);
    return profile && (*profile == oer::Profile::free || *profile == oer::Profile::developer ||
                       *profile == oer::Profile::enterprise || *profile == oer::Profile::unlimited);
}

bool valid_reservation(const Reservation &r) {
    if (!valid_selection(r.selection) || r.request.empty() || r.order == 0)
        return false;
    if (r.selection.scope.provider == exchange_rate_api) {
        const auto *route = std::get_if<era::Operation>(&r.route);
        return route && (*route == era::Operation::latest || *route == era::Operation::history ||
                         *route == era::Operation::quota);
    }
    const auto *route = std::get_if<oer::Operation>(&r.route);
    return route && (*route == oer::Operation::latest || *route == oer::Operation::historical_day ||
                     *route == oer::Operation::usage);
}

std::size_t value_limit(KeyedRateProvider provider) {
    return provider == open_exchange_rates ? app_id_size
                                           : storage::max_plaintext_size - prefix_size;
}

bool valid_value(KeyedRateProvider provider, Bytes value) {
    if (provider == exchange_rate_api)
        return !value.empty() && value.size() <= value_limit(provider);
    return provider == open_exchange_rates && value.size() == app_id_size &&
           std::ranges::all_of(value, [](unsigned char c) {
               return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
           });
}

bool oversized_change(const RateCredentialChange &change) {
    return oversized_scope(change.scope) || change.decision.size() > ref_limit;
}

Result check_change(const RateCredentialChange &change, Bytes value) {
    if (oversized_change(change) || value.size() > value_limit(change.scope.provider))
        return Result::resource_limit;
    if (!valid_scope(change.scope) || change.decision.empty())
        return Result::invalid_input;
    switch (change.verb) {
    case Verb::put:
        return valid_value(change.scope.provider, value) ? Result::admitted : Result::invalid_input;
    case Verb::erase:
        return change.expected && value.empty() ? Result::admitted : Result::invalid_input;
    case Verb::inspect:
        return !change.expected && value.empty() ? Result::admitted : Result::invalid_input;
    }
    return Result::invalid_input;
}

void append_ref(storage::RecordBytes &out, std::string_view ref) {
    out.push_back(static_cast<unsigned char>(ref.size() >> 8));
    out.push_back(static_cast<unsigned char>(ref.size()));
    out.insert(out.end(), ref.begin(), ref.end());
}

storage::RecordBytes encode_scope(const RateCredentialScope &scope) {
    storage::RecordBytes out;
    out.reserve(8 + scope.installation.size() + scope.account.size() + scope.slot.size());
    append_ref(out, scope.installation);
    out.push_back(static_cast<unsigned char>(scope.provider));
    out.push_back(1);
    append_ref(out, scope.account);
    append_ref(out, scope.slot);
    return out;
}

bool take_ref(Bytes &scope) {
    if (scope.size() < 2)
        return false;
    const auto length = (std::size_t{scope[0]} << 8) | scope[1];
    if (length == 0 || length > ref_limit || length > scope.size() - 2)
        return false;
    scope = scope.subspan(2 + length);
    return true;
}

std::optional<KeyedRateProvider> decode_scope(Bytes scope) {
    if (!take_ref(scope) || scope.size() < 2 || scope[1] != 1)
        return std::nullopt;
    const auto provider = static_cast<KeyedRateProvider>(scope[0]);
    scope = scope.subspan(2);
    if (!valid_provider(provider) || !take_ref(scope) || !take_ref(scope) || !scope.empty())
        return std::nullopt;
    return provider;
}

std::optional<Revision> payload_revision(KeyedRateProvider provider, Bytes plain) {
    if (plain.size() <= prefix_size || !std::ranges::equal(plain.first(magic.size()), magic) ||
        !valid_value(provider, plain.subspan(prefix_size)))
        return std::nullopt;
    Revision revision;
    std::ranges::copy(plain.subspan(magic.size(), revision.size()), revision.begin());
    return revision;
}

Result disposition(CredentialError error) {
    switch (error) {
    case CredentialError::invalid_input:
        return Result::invalid_input;
    case CredentialError::resource_limit:
        return Result::resource_limit;
    case CredentialError::invalid_record:
        return Result::invalid_record;
    case CredentialError::authentication_failed:
        return Result::authentication_failed;
    case CredentialError::crypto_unavailable:
        return Result::crypto_unavailable;
    case CredentialError::key_unavailable:
    case CredentialError::key_file_rejected:
    case CredentialError::invalid_key_file:
        return Result::key_unavailable;
    case CredentialError::feature_not_implemented:
        return Result::feature_not_implemented;
    }
    return Result::invalid_record;
}

struct OpenedCredential {
    QByteArray ciphertext;
    CredentialValue payload;
    Revision revision;

    Bytes value() const { return payload.bytes().subspan(prefix_size); }
};

using LoadedCredential = std::variant<OpenedCredential, Result>;

LoadedCredential load_exact(const SqliteWorkSession &session, const CredentialKey *key,
                            const RateCredentialScope &scope, Bytes canonical,
                            const QByteArray &scope_blob) {
    if (!storage::RepositoryAccess::usable(key))
        return Result::key_unavailable;
    auto selected = storage::select_record(session, scope_blob);
    if (const auto *error = std::get_if<CredentialError>(&selected))
        return disposition(*error);
    auto *ciphertext = std::get_if<QByteArray>(&selected);
    if (!ciphertext)
        return Result::missing;
    auto opened = storage::RepositoryAccess::open(*key, canonical, *ciphertext);
    if (const auto *error = std::get_if<CredentialError>(&opened))
        return disposition(*error);
    auto &payload = std::get<CredentialValue>(opened);
    const auto revision = payload_revision(scope.provider, payload.bytes());
    if (!revision)
        return Result::invalid_record;
    return OpenedCredential{std::move(*ciphertext), std::move(payload), *revision};
}

MutationResult stage_change(const SqliteWorkSession &session, const CredentialKey *key,
                            const RateCredentialChange &change, Bytes value) {
    const auto canonical = encode_scope(change.scope);
    const auto scope_blob = storage::blob(canonical);
    QByteArray prior;
    {
        auto loaded = load_exact(session, key, change.scope, canonical, scope_blob);
        if (const auto *status = std::get_if<Result>(&loaded)) {
            if (*status != Result::missing)
                return {*status};
            if (change.expected)
                return {Result::stale};
        } else {
            auto &present = std::get<OpenedCredential>(loaded);
            if (change.expected != present.revision)
                return {Result::stale};
            if (change.verb == Verb::put && value.size() == present.value().size() &&
                sodium_memcmp(value.data(), present.value().data(), value.size()) == 0)
                return {Result::unchanged, present.revision};
            prior = std::move(present.ciphertext);
        }
    } // Erase the old plaintext before preparing the replacement.

    if (change.verb == Verb::erase) {
        storage::erase_record(session, scope_blob, prior);
        return {Result::staged_erase};
    }
    if (!storage::sodium_ready())
        return {Result::crypto_unavailable};
    Revision revision;
    randombytes_buf(revision.data(), revision.size());
    if (change.expected == revision)
        return {Result::crypto_unavailable};

    storage::GuardedBytes payload(prefix_size + value.size());
    auto inner = payload.bytes();
    if (inner.empty())
        return {Result::crypto_unavailable};
    std::ranges::copy(magic, inner.begin());
    std::ranges::copy(revision, inner.begin() + magic.size());
    std::ranges::copy(value, inner.begin() + prefix_size);
    const auto sealed = storage::RepositoryAccess::seal_guarded(*key, canonical, payload);
    if (const auto *error = std::get_if<CredentialError>(&sealed))
        return {disposition(*error)};
    storage::store_record(session, scope_blob, prior.isNull() ? nullptr : &prior,
                          storage::blob(std::get<storage::RecordBytes>(sealed)));
    return {Result::staged_put, revision};
}

bool acknowledged(const SqliteWorkResult &done) {
    return done.transaction == TransactionOutcome::committed && done.error == WorkError::none &&
           done.value && done.value->metaType().id() == QMetaType::Int && done.value->toInt() == 1;
}

// The writer publishes only this marker. Per-work facts travel through the
// private handles paired to the callback by the serialized owner.
struct OwnerIdentity {};
struct MutationIdentity {};

struct Mutation {
    RateCredentialChange change;
    std::shared_ptr<const MutationIdentity> identity = std::make_shared<MutationIdentity>();
    bool started{};
    bool uncertain{};
    std::optional<MutationResult> staged{};
};

struct GuardedUse {
    Reservation reservation;
    std::optional<CredentialValue> payload;
};

struct ExactRowState {
    std::optional<Revision> revision;
};

enum class UsePhase { reserved, prepared, accepted };
struct Outstanding {
    Reservation reservation;
    UsePhase phase{UsePhase::reserved};
    std::weak_ptr<GuardedUse> guarded;
};

struct ScopeState {
    std::optional<RateCredentialSelection> selection;
    std::set<std::string> retired_generations;
    bool enabled{};
    bool needs_selection{};
    bool in_recovery{};
    bool recovery_approved{};
    std::optional<Mutation> mutation;
    // Volatile eligibility after a mutation or Recovery readback.
    std::optional<ExactRowState> acknowledged_row;
    std::uint64_t latest_order{};
    std::map<std::uint64_t, Outstanding> outstanding;

    ~ScopeState() { invalidate_pending(); }

    void expire(const Reservation &reservation) {
        const auto it = outstanding.find(reservation.order);
        if (it == outstanding.end() || it->second.reservation != reservation)
            return;
        erase_pending(it->second);
        outstanding.erase(it);
    }

    void invalidate_pending() {
        for (auto &[order, entry] : outstanding)
            erase_pending(entry);
        outstanding.clear();
    }

    void require_selection(std::optional<Revision> revision) {
        invalidate_pending();
        acknowledged_row = ExactRowState{revision};
        needs_selection = true;
        recovery_approved = false;
    }

    Outstanding *find(const Reservation &r) {
        const auto it = outstanding.find(r.order);
        if (it == outstanding.end() || it->second.reservation != r)
            return nullptr;
        return &it->second;
    }

    Result authority(const Reservation &r) const {
        if (retired_generations.contains(r.selection.generation))
            return Result::not_authorized;
        if (r.selection != *selection)
            return Result::stale;
        if (mutation)
            return Result::held;
        return needs_selection ? Result::stale : Result::admitted;
    }

  private:
    static void erase_pending(Outstanding &entry) {
        if (entry.phase == UsePhase::prepared) {
            if (auto live = entry.guarded.lock())
                live->payload.reset();
        }
    }
};

bool eligible_to_publish(const RateCredentialSelection &s) {
    return !oversized_scope(s.scope) &&
           std::max({s.generation.size(), s.configuration.size(), s.selection.size(),
                     s.quota_scope.size()}) <= ref_limit &&
           valid_selection(s);
}

bool is_usage(const Reservation &r) {
    const auto *route = std::get_if<oer::Operation>(&r.route);
    return route && *route == oer::Operation::usage;
}

oer::AdmissionIdentity usage_admission(const Reservation &r) {
    const auto &s = r.selection;
    return {{s.scope.installation,
             s.quota_scope,
             s.configuration,
             {reinterpret_cast<const char *>(s.revision.data()), s.revision.size()},
             std::get<oer::Profile>(s.profile)},
            r.request,
            r.order};
}

} // namespace

class OwnerState {
  public:
    const std::shared_ptr<const OwnerIdentity> identity = std::make_shared<OwnerIdentity>();
    std::map<RateCredentialScope, ScopeState> scopes;

    ScopeState *find(const RateCredentialScope &scope) {
        const auto it = scopes.find(scope);
        return it == scopes.end() ? nullptr : &it->second;
    }

    Result authorize(const Reservation &request, const UseGrant &grant) {
        if (oversized_reservation(request) || (grant && oversized_reservation(*grant)))
            return Result::resource_limit;
        if (!valid_reservation(request))
            return Result::invalid_input;
        const auto *scope = find(request.selection.scope);
        if (!grant || !scope || !scope->enabled || !scope->selection)
            return Result::not_authorized;
        if (request != *grant)
            return Result::stale;
        return scope->authority(request);
    }

    ScopeState *update_selection(const RateCredentialSelection &s) {
        if (!eligible_to_publish(s))
            return nullptr;
        auto &scope = scopes[s.scope];
        if (scope.retired_generations.contains(s.generation))
            return nullptr;
        if (scope.selection != s) {
            if (scope.selection && scope.selection->generation != s.generation) {
                scope.retired_generations.insert(scope.selection->generation);
                scope.latest_order = 0;
            }
            scope.invalidate_pending();
            scope.selection = s;
            scope.enabled = true;
            scope.needs_selection = false;
            scope.recovery_approved = false;
        }
        return &scope;
    }

    ScopeState *disable(const RateCredentialScope &key, std::string generation) {
        if (oversized_scope(key) || !valid_scope(key) || generation.size() > ref_limit ||
            generation.empty())
            return nullptr;
        auto &scope = scopes[key];
        if ((scope.selection && scope.selection->generation == generation) ||
            scope.retired_generations.contains(generation))
            return nullptr;
        scope.retired_generations.insert(std::move(generation));
        if (scope.selection)
            scope.retired_generations.insert(scope.selection->generation);
        scope.invalidate_pending();
        scope.enabled = false;
        scope.recovery_approved = false;
        return &scope;
    }

    Outstanding *find_use(const std::shared_ptr<GuardedUse> &use) {
        if (auto *scope = find(use->reservation.selection.scope)) {
            auto *entry = scope->find(use->reservation);
            if (entry && entry->guarded.lock() == use)
                return entry;
        }
        return nullptr;
    }

    void discard_use(const std::shared_ptr<GuardedUse> &use) {
        if (find_use(use))
            find(use->reservation.selection.scope)->expire(use->reservation);
    }
};

struct Readback::Evidence {
    std::weak_ptr<const MutationIdentity> mutation;
    Result status;
    std::optional<Revision> revision;
};

struct SlotState {
    std::shared_ptr<GuardedUse> use;
};
struct PermitState {
    std::shared_ptr<GuardedUse> use;
};
struct AttemptState {
    std::weak_ptr<const OwnerIdentity> owner;
    std::shared_ptr<GuardedUse> use;
};

template <class State> void Destroy<State>::operator()(State *state) const noexcept {
    delete state;
}
template struct Destroy<SlotState>;
template struct Destroy<PermitState>;
template struct Destroy<AttemptState>;

CredentialOwner::CredentialOwner() : state_(std::make_unique<OwnerState>()) {}
CredentialOwner::~CredentialOwner() = default;

Result CredentialOwner::admit_change(const RateCredentialChange &request,
                                     const ManagementGrant &grant, Bytes guarded_value) {
    if (grant && oversized_change(*grant))
        return Result::resource_limit;
    if (const auto shape = check_change(request, guarded_value); shape != Result::admitted)
        return shape;
    if (request.verb == Verb::inspect)
        return Result::invalid_input;
    if (grant != request)
        return Result::not_authorized;
    auto &scope = state_->scopes[request.scope];
    if (scope.mutation)
        return Result::held;
    scope.mutation.emplace(Mutation{request});
    scope.invalidate_pending();
    return Result::admitted;
}

MutationResult CredentialOwner::stage_rate_credential_put(const SqliteWorkSession &session,
                                                          const CredentialKey *key,
                                                          const RateCredentialChange &change,
                                                          const storage::GuardedBytes &input) {
    return stage(session, key, change, Verb::put, input.bytes());
}

MutationResult CredentialOwner::stage_rate_credential_erase(const SqliteWorkSession &session,
                                                            const CredentialKey *key,
                                                            const RateCredentialChange &change) {
    return stage(session, key, change, Verb::erase, {});
}

MutationResult CredentialOwner::stage(const SqliteWorkSession &session, const CredentialKey *key,
                                      const RateCredentialChange &change, Verb verb, Bytes value) {
    if (const auto shape = check_change(change, value); shape != Result::admitted)
        return {shape};
    if (change.verb != verb)
        return {Result::invalid_input};
    auto *scope = state_->find(change.scope);
    if (!scope || !scope->mutation || scope->mutation->change != change)
        return {Result::not_authorized};
    auto &mutation = *scope->mutation;
    if (mutation.started || mutation.uncertain)
        return {Result::stale};
    mutation.started = true;
    mutation.staged = stage_change(session, key, change, value);
    return *mutation.staged;
}

Result CredentialOwner::finish_change(const RateCredentialChange &change,
                                      const SqliteWorkResult &done) {
    auto *scope = state_->find(change.scope);
    if (!scope || !scope->mutation || scope->mutation->change != change)
        return Result::stale;
    auto &mutation = *scope->mutation;
    if (mutation.uncertain)
        return Result::held;
    if (done.transaction == TransactionOutcome::rolled_back) {
        scope->mutation.reset();
        return Result::known_rollback;
    }
    if (!acknowledged(done) || !mutation.staged) {
        mutation.uncertain = true;
        return Result::held;
    }
    const auto staged = *mutation.staged;
    scope->mutation.reset();
    if (staged.status == Result::staged_put || staged.status == Result::staged_erase) {
        scope->require_selection(staged.revision);
        return Result::committed_change;
    }
    return staged.status;
}

Readback CredentialOwner::inspect(const SqliteWorkSession &session, const CredentialKey *key,
                                  const RateCredentialChange &request,
                                  const ManagementGrant &grant) {
    if (grant && oversized_change(*grant))
        return {Result::resource_limit};
    if (const auto shape = check_change(request, {}); shape != Result::admitted)
        return {shape};
    if (request.verb != Verb::inspect)
        return {Result::invalid_input};
    if (grant != request)
        return {Result::not_authorized};

    const auto canonical = encode_scope(request.scope);
    const auto loaded =
        load_exact(session, key, request.scope, canonical, storage::blob(canonical));
    const auto *opened = std::get_if<OpenedCredential>(&loaded);
    auto result =
        opened ? Readback{Result::present, opened->revision} : Readback{std::get<Result>(loaded)};
    const auto *scope = state_->find(request.scope);
    if (scope && scope->mutation && scope->mutation->uncertain &&
        (result.status == Result::present || result.status == Result::missing))
        result.evidence_ = std::make_shared<Readback::Evidence>(
            Readback::Evidence{scope->mutation->identity, result.status, result.revision});
    return result;
}

Result CredentialOwner::reconcile(const RateCredentialChange &change, const Readback &readback,
                                  const SqliteWorkResult &done) {
    auto *scope = state_->find(change.scope);
    if (!scope || !scope->mutation || scope->mutation->change != change)
        return Result::stale;
    const auto &mutation = *scope->mutation;
    const auto &evidence = readback.evidence_;
    if (!mutation.uncertain || !acknowledged(done) || !evidence ||
        evidence->mutation.lock() != mutation.identity || evidence->status != readback.status ||
        evidence->revision != readback.revision)
        return Result::held;

    const auto &staged = mutation.staged;
    const bool agrees = readback.status == Result::missing
                            ? !change.expected || (staged && staged->status == Result::staged_erase)
                            : readback.revision == change.expected ||
                                  (staged && staged->status == Result::staged_put &&
                                   readback.revision == staged->revision);
    if (!agrees)
        return Result::held;
    scope->mutation.reset();
    scope->require_selection(readback.revision);
    return Result::reconciled;
}

void CredentialOwner::publish_selection(const RateCredentialSelection &selection) {
    state_->update_selection(selection);
}

void CredentialOwner::publish_reservation(const Reservation &r) {
    if (oversized_reservation(r) || !valid_reservation(r))
        return;
    auto *scope = state_->find(r.selection.scope);
    if (!scope || !scope->enabled || scope->authority(r) != Result::admitted ||
        r.order <= scope->latest_order)
        return;

    // The reservation producer owns unique request refs. Order gives a bounded
    // replay fence for previously issued reservations in this owner lifetime.
    std::erase_if(scope->outstanding, [](const auto &item) {
        const auto &entry = item.second;
        return entry.phase != UsePhase::reserved && entry.guarded.expired();
    });
    if (std::ranges::any_of(scope->outstanding, [&](const auto &item) {
            return item.second.reservation.request == r.request;
        }))
        return;
    scope->outstanding.emplace(r.order, Outstanding{r, UsePhase::reserved, {}});
    scope->latest_order = r.order;
}

void CredentialOwner::expire_reservation(const Reservation &reservation) {
    if (auto *scope = state_->find(reservation.selection.scope))
        scope->expire(reservation);
}

void CredentialOwner::revoke(const RateCredentialScope &scope, std::string disabled_generation) {
    state_->disable(scope, std::move(disabled_generation));
}

void CredentialOwner::begin_recovery(const RateCredentialScope &scope, std::string generation) {
    if (auto *state = state_->disable(scope, std::move(generation))) {
        state->in_recovery = true;
        state->acknowledged_row.reset();
    }
}

void CredentialOwner::publish_recovery_selection(const RateCredentialSelection &selection) {
    auto *scope = state_->find(selection.scope);
    if (!scope || !scope->in_recovery || scope->mutation ||
        (scope->selection && scope->selection->generation == selection.generation))
        return;
    if (state_->update_selection(selection))
        scope->recovery_approved = true;
}

Result CredentialOwner::prepare_keyed_rate_use(const SqliteWorkSession &session,
                                               const CredentialKey *key, const Reservation &r,
                                               const UseGrant &grant, WorkSlot &out) {
    out.reset();
    if (const auto auth = state_->authorize(r, grant); auth != Result::admitted)
        return auth;
    auto &scope = *state_->find(r.selection.scope);
    auto *entry = scope.find(r);
    if (!entry || entry->phase != UsePhase::reserved)
        return Result::stale;

    const auto canonical = encode_scope(r.selection.scope);
    auto loaded = load_exact(session, key, r.selection.scope, canonical, storage::blob(canonical));
    if (const auto *status = std::get_if<Result>(&loaded))
        return *status;
    auto &opened = std::get<OpenedCredential>(loaded);
    if (opened.revision != r.selection.revision ||
        (scope.acknowledged_row && scope.acknowledged_row->revision != opened.revision))
        return Result::stale;
    if (scope.in_recovery && !scope.recovery_approved)
        return Result::held;

    auto guarded = std::make_shared<GuardedUse>(GuardedUse{r, std::move(opened.payload)});
    out.reset(new SlotState{guarded});
    entry->phase = UsePhase::prepared;
    entry->guarded = guarded;
    return Result::prepared;
}

Permit CredentialOwner::finish_keyed_rate_use(WorkSlot slot, const SqliteWorkResult &done) {
    if (!slot || !slot->use->payload || !state_->find_use(slot->use))
        return {};
    if (!acknowledged(done)) {
        state_->discard_use(slot->use);
        return {};
    }
    auto &scope = *state_->find(slot->use->reservation.selection.scope);
    auto permit = Permit(new PermitState{std::move(slot->use)});
    if (scope.in_recovery) {
        scope.acknowledged_row = ExactRowState{permit->use->reservation.selection.revision};
        scope.in_recovery = false;
        scope.recovery_approved = false;
    }
    return permit;
}

Result CredentialOwner::consume_keyed_rate_use(Permit &permit, const UseGrant &grant,
                                               QualifiedTransport &transport, Attempt &out) {
    auto pending = std::move(permit);
    out.reset();
    if (!pending)
        return Result::stale;
    auto &use = pending->use;
    const auto auth = state_->authorize(use->reservation, grant);
    auto *entry = state_->find_use(use);
    if (auth != Result::admitted || !entry || entry->phase != UsePhase::prepared || !use->payload) {
        state_->discard_use(use);
        return auth == Result::admitted ? Result::stale : auth;
    }

    auto accepted = Attempt(new AttemptState{state_->identity, use});
    entry->phase = UsePhase::accepted;
    transport.accept(use->reservation, use->payload->bytes().subspan(prefix_size));
    // Only the OER usage reply needs the guarded bytes after handoff.
    if (!is_usage(use->reservation))
        use->payload.reset();
    out = std::move(accepted);
    return Result::accepted;
}

Result CredentialOwner::settle_attempt(Attempt &attempt, const UseGrant &grant,
                                       KeyedRateProvider provider) {
    auto accepted = std::move(attempt);
    if (!accepted || accepted->owner.lock() != state_->identity)
        return Result::stale;
    const auto &r = accepted->use->reservation;
    const bool current =
        state_->find_use(accepted->use) && state_->authorize(r, grant) == Result::admitted;
    state_->discard_use(accepted->use);
    if (r.selection.scope.provider != provider || !grant || r != *grant)
        return Result::stale;
    return current ? Result::current_reply : Result::accounting_only;
}

Result CredentialOwner::finish_attempt(Attempt &attempt, const UseGrant &grant,
                                       era::Dispatch dispatch) {
    switch (dispatch.kind) {
    case era::DispatchKind::not_sent:
        if (dispatch.time)
            break;
        return settle_attempt(attempt, grant, exchange_rate_api);
    case era::DispatchKind::sent:
    case era::DispatchKind::possibly_sent:
        return settle_attempt(attempt, grant, exchange_rate_api);
    }
    auto invalid = std::move(attempt);
    if (invalid)
        state_->discard_use(invalid->use);
    return Result::invalid_input;
}

Result CredentialOwner::finish_attempt(Attempt &attempt, const UseGrant &grant,
                                       oer::Dispatch dispatch) {
    switch (dispatch) {
    case oer::Dispatch::not_sent:
    case oer::Dispatch::sent:
    case oer::Dispatch::possibly_sent:
        return settle_attempt(attempt, grant, open_exchange_rates);
    }
    auto invalid = std::move(attempt);
    if (invalid)
        state_->discard_use(invalid->use);
    return Result::invalid_input;
}

std::optional<UsageFact>
CredentialOwner::complete_rate_usage_echo(Attempt &attempt, const oer::AdmissionIdentity &reply,
                                          GuardedEcho echo) {
    auto accepted = std::move(attempt);
    if (!accepted || accepted->owner.lock() != state_->identity)
        return std::nullopt;
    const auto &use = *accepted->use;
    const auto &r = use.reservation;
    const bool current =
        state_->find_use(accepted->use) && state_->authorize(r, UseGrant{r}) == Result::admitted;
    state_->discard_use(accepted->use);
    if (!current || !is_usage(r) || !use.payload || usage_admission(r) != reply)
        return std::nullopt;

    auto match = oer::EchoMatch::unknown;
    if (echo.shape == EchoShape::single && echo.value &&
        valid_value(open_exchange_rates, echo.value->bytes())) {
        match = sodium_memcmp(use.payload->bytes().subspan(prefix_size).data(),
                              echo.value->bytes().data(), app_id_size) == 0
                    ? oer::EchoMatch::match
                    : oer::EchoMatch::mismatch;
    }
    return UsageFact{usage_admission(r), r.selection, match};
}

Result validate_rate_scope(Bytes scope) {
    return decode_scope(scope) ? Result::present : Result::invalid_record;
}

Readback inspect_rate_payload(Bytes scope, Bytes authenticated) {
    const auto provider = decode_scope(scope);
    if (provider) {
        if (const auto revision = payload_revision(*provider, authenticated))
            return {Result::present, revision};
    }
    return {Result::invalid_record};
}
} // namespace subathon::server::rate_credentials
