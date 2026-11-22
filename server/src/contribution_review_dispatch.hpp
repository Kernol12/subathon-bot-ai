// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "contribution_review_reconciliation_budget.hpp"

#include <new>
#include <utility>

namespace subathon::server::review_detail {

using test_unit = testing::ReviewUnit;
using phase = testing::ReviewUnitPhase;
using Observer = testing::ContributionReviewObserver;
using Result = ContributionReview::Result;

inline bool normal(const SqliteWorkResult &result, bool observed) {
    return observed && result.transaction == TransactionOutcome::committed &&
           result.error == WorkError::none && result.value.has_value();
}

inline void event(const Observer &observer, test_unit unit, phase p, int ordinal = 0,
                  const SqliteWorkResult *result = nullptr) {
    if (observer)
        observer({unit, p, ordinal, result});
}

template <class Work, class Done>
StorageError submit(SqliteWriter &writer, Observer observer, test_unit unit, int ordinal, Work work,
                    Done done) {
    event(observer, unit, phase::before_enqueue, ordinal);
    return writer.enqueue_work(
        [observer, unit, ordinal,
         work = std::move(work)](const SqliteWorkContext &context) mutable {
            const auto s = context.begin();
            detail::ContributionReviewWriterAccess::meter(s, unit == test_unit::row);
            event(observer, unit, phase::began, ordinal);
            const bool staged = work(s);
            if (staged)
                event(observer, unit, phase::before_commit, ordinal);
            s.commit();
            if (staged)
                event(observer, unit, phase::after_commit, ordinal);
            return QVariant(1);
        },
        [observer, unit, ordinal, done = std::move(done)](const SqliteWorkResult &r) mutable {
            bool observed = true;
            try {
                event(observer, unit, phase::completed, ordinal, &r);
            } catch (...) {
                observed = false;
            }
            done(r, observed);
        });
}

// Owning stores can establish rollback for a decoded size bound before the
// writer's own meters trip. Preserve that distinction from invalid history.
inline bool size_failure(const std::exception_ptr &failure) {
    try {
        std::rethrow_exception(failure);
    } catch (const SqliteBundleStorageFailure &e) {
        using R = SqliteBundleStorageReason;
        return e.reason() == R::resource_limit || e.reason() == R::result_limit;
    } catch (const SqliteHoldStorageFailure &e) {
        using R = SqliteHoldStorageReason;
        return e.reason() == R::resource_limit || e.reason() == R::result_limit;
    } catch (const RetentionFailure &e) {
        return e.error() == RetentionError::result_limit;
    } catch (...) {
        return false;
    }
}

// Keep only owned canonical input and the validated prefix across owner turns.
// A queued closure owns the attempt until that unit's completion is delivered.
class Execute : public std::enable_shared_from_this<Execute> {
  public:
    Execute(SqliteWriter &writer, ContributionReviewFacts &facts, Observer observer,
            ReviewCommand command, std::string proof, QByteArray document,
            std::function<void(Result)> done)
        : writer_(&writer), facts_(facts), observer_(std::move(observer)),
          command_(std::move(command)), proof_(std::move(proof)), manifest_(std::move(document)),
          done_(std::move(done)),
          selection_(selectors(command_.entries, [](const auto &x) { return x.intent.selector; })) {
    }

    ~Execute() { release(); }

    void start() {
        auto self = shared_from_this();
        if (!enqueue(
                test_unit::initial_read, 0,
                [self](const auto &s) {
                    try {
                        self->initial(s);
                    } catch (const RetainedAuthorityUnavailable &) {
                        self->refusal_ = ReviewRefusal::unavailable;
                        throw;
                    } catch (...) {
                        if (size_failure(std::current_exception()))
                            self->refusal_ = ReviewRefusal::batch_too_large;
                        throw;
                    }
                    return false;
                },
                [self](const auto &r, bool observed) { self->after_initial(r, observed); }))
            finish(ReviewRefusal::unavailable);
    }

    void acquired() { active_ = true; }

  private:
    QPointer<SqliteWriter> writer_;
    ContributionReviewFacts &facts_;
    Observer observer_;
    ReviewCommand command_;
    std::string proof_;
    QByteArray manifest_;
    std::function<void(Result)> done_;
    std::vector<ReviewSelector> selection_;
    std::vector<std::optional<Basis>> bases_;
    bool basis_selection_{};
    bool active_{};
    int initial_cursor_{1};
    Prefix prefix_;
    std::array<ReadCost, row_limit> reservations_{};
    ReadCost committed_cost_;
    std::pair<qint64, qint64> checked_snapshot_;
    std::optional<ReviewRefusal> refusal_;
    std::optional<ReviewStop> stop_;
    std::optional<int> failed_;
    std::optional<StoredOutcome> staged_;
    std::string staged_head_;
    bool disclosed_{};
    bool staging_{};

    Authority authority(bool retained = false) {
        return {facts_,        command_.principal, proof_,  command_.actor,
                &command_.key, selection_,         retained};
    }

    void release() noexcept {
        if (active_ && writer_)
            detail::ContributionReviewWriterAccess::release(*writer_);
        active_ = false;
    }

    void finish(Result result) {
        release();
        auto done = std::move(done_);
        done(std::move(result));
    }

    bool enqueue(test_unit unit, int ordinal, auto work, auto done) {
        if (!writer_)
            return false;
        try {
            return submit(*writer_, observer_, unit, ordinal, std::move(work), std::move(done)) ==
                   StorageError::none;
        } catch (...) {
            return false;
        }
    }

    void bind_bases() {
        std::vector<std::string> intents;
        for (const auto &entry : command_.entries) {
            try {
                bases_.push_back(Basis::decode(entry.basis));
            } catch (const Invalid &) {
                bases_.push_back({});
            }
            const auto &basis = bases_.back();
            intents.push_back(digest_intent(entry.intent, basis && basis->discover_choices));
        }
        const auto digest = selection_digest(command_.principal, intents);
        basis_selection_ = true;
        std::optional<std::string_view> root;
        for (size_t n = 0; n < intents.size(); ++n) {
            const auto &basis = bases_[n];
            if (!basis || basis->intent != intents[n] || basis->selection != digest ||
                (root && *root != basis->root)) {
                basis_selection_ = false;
                break;
            }
            root = basis->root;
        }
    }

    void initial(const SqliteWorkSession &s) {
        const auto retained = authority(true);
        refusal_ = retained.scope(s);
        if (refusal_)
            return;
        catalog(s);
        const auto h = header(s, command_.principal, command_.key);
        if (h.count != 0 && h.manifest != manifest_) {
            // Validate canonical framing before treating the retained key as a
            // well-formed conflicting identity.
            const auto old = decode_manifest(h.manifest);
            require(old.entries.size() == size_t(h.count) && same(old.key, command_.key) &&
                    old.principal.installation_ref == command_.principal.installation_ref);
            refusal_ = ReviewRefusal::identity_conflict;
            return;
        }
        require(h.count == 0 || h.count == int(command_.entries.size()));
        initial_cursor_ = h.cursor;
        prefix_ = read_prefix(s, command_, h, initial_cursor_, &retained);
        if (prefix_.cursor > int(command_.entries.size()))
            return;
        bind_bases();
        try {
            committed_cost_ = prefix_.closure;
            auto total = committed_cost_;
            const auto auth = authority();
            const auto inputs = preview_inputs(s);
            std::optional<ApplicationStateSnapshot> projected;
            std::set<std::string> completed;
            for (size_t n = size_t(h.cursor - 1); n < command_.entries.size(); ++n) {
                const auto &entry = command_.entries[n];
                // Retain a cheap journal possibility even if current row
                // authority, eligibility or the selected anchor is absent.
                ReadCost plan;
                auto row = read_row(s, auth, entry.intent, projected, completed, &inputs, &plan);
                const auto data = proposed_outcome(command_, int(n + 1), row, {}, {}, false);
                auto cost = reconciliation_row(s, command_, entry, row, data, plan);
                reservations_[n] = cost;
                total += cost;
                if (!reconciliation_fits(total, manifest_.size()))
                    throw Oversize{};
                if (data.outcome.facts && data.outcome.facts->bundle) {
                    projected = std::move(row.after);
                    for (const auto &group : row.groups)
                        completed.insert(group.group.achievement.milestone_ref);
                }
            }
            checked_snapshot_ = reconciliation_snapshot(s);
        } catch (const Oversize &) {
            refusal_ = ReviewRefusal::batch_too_large;
        } catch (const ConflictingAnchor &) {
            refusal_ = ReviewRefusal::invalid_selection;
        }
    }

    void after_initial(const SqliteWorkResult &r, bool observed) {
        if (!normal(r, observed)) {
            if (refusal_ == ReviewRefusal::unavailable) {
                finish(ReviewRefusal::unavailable);
                return;
            }
            const bool bounded_failure =
                observed && r.transaction == TransactionOutcome::rolled_back &&
                (refusal_ == ReviewRefusal::batch_too_large ||
                 r.error == WorkError::resource_limit || r.error == WorkError::result_limit);
            finish(bounded_failure ? ReviewRefusal::batch_too_large : ReviewRefusal::unavailable);
        } else if (refusal_) {
            finish(*refusal_);
        } else if (prefix_.cursor > int(command_.entries.size())) {
            finish(std::move(prefix_.reply));
        } else {
            qualify_next();
        }
    }

    bool current_scope(const SqliteWorkSession &s, const Authority &auth,
                       bool verify_live_work = false) {
        if (const auto refusal = auth.scope(s, verify_live_work)) {
            stop_ =
                *refusal == ReviewRefusal::denied ? ReviewStop::denied : ReviewStop::unavailable;
            return false;
        }
        return true;
    }

    bool live_scope(const SqliteWorkSession &s, const Authority &auth) {
        try {
            return current_scope(s, auth, true);
        } catch (...) {
            stop_ = ReviewStop::unavailable;
            throw;
        }
    }

    bool finish_scope_stop(const SqliteWorkResult &result, bool observed) {
        if (stop_ != ReviewStop::denied && stop_ != ReviewStop::unavailable)
            return false;
        if (!normal(result, observed))
            stop_ = ReviewStop::unavailable;
        safe_stop();
        return true;
    }

    bool suffix_fits(size_t n, ReadCost candidate) const {
        candidate += committed_cost_;
        for (size_t later = n + 1; later < command_.entries.size(); ++later)
            candidate += reservations_[later];
        return reconciliation_fits(candidate, manifest_.size());
    }

    void qualify_next() {
        const auto ordinal = prefix_.cursor;
        auto self = shared_from_this();
        if (!enqueue(
                test_unit::qualification_read, ordinal,
                [self, ordinal](const auto &s) {
                    const auto auth = self->authority();
                    if (!self->live_scope(s, auth))
                        return false;
                    const auto n = size_t(ordinal - 1);
                    const auto &i = self->command_.entries.at(n).intent;
                    try {
                        if (allowed(auth.check(s, ReviewAuthorityUse::execute_row, &i)) &&
                            anchor(s, i.selector))
                            bounded_shape(s, i, auth.policy(s, i.selector));
                        const auto snapshot = reconciliation_snapshot(s);
                        if (snapshot != self->checked_snapshot_) {
                            const auto h = header(s, self->command_.principal, self->command_.key);
                            require(h.cursor == self->prefix_.cursor &&
                                    (h.count == 0 ? h.cursor == 1 : h.manifest == self->manifest_));
                            self->committed_cost_ =
                                read_prefix(s, self->command_, h, self->initial_cursor_).closure;
                            self->checked_snapshot_ = snapshot;
                            if (!self->suffix_fits(n, self->reservations_[n]))
                                throw Oversize{};
                        }
                    } catch (const Oversize &) {
                        self->stop_ = ReviewStop::row_budget_exceeded;
                    } catch (...) {
                        if (size_failure(std::current_exception()))
                            self->stop_ = ReviewStop::row_budget_exceeded;
                        throw;
                    }
                    return false;
                },
                [self, ordinal](const auto &r, bool observed) {
                    if (self->finish_scope_stop(r, observed))
                        return;
                    if (!normal(r, observed)) {
                        const bool bounded_failure =
                            observed && r.transaction == TransactionOutcome::rolled_back &&
                            (self->stop_ == ReviewStop::row_budget_exceeded ||
                             r.error == WorkError::resource_limit ||
                             r.error == WorkError::result_limit);
                        self->stop_ = bounded_failure ? ReviewStop::row_budget_exceeded
                                                      : ReviewStop::unavailable;
                    }
                    if (!self->stop_)
                        self->forward(ordinal);
                    else if (self->stop_ == ReviewStop::row_budget_exceeded)
                        self->reconcile();
                    else
                        self->safe_stop();
                })) {
            stop_ = ReviewStop::admission_refused;
            safe_stop();
        }
    }

    bool row(const SqliteWorkSession &s, int ordinal) {
        const auto auth = authority();
        if (!live_scope(s, auth))
            return false;
        const auto h = header(s, command_.principal, command_.key);
        require(h.cursor == ordinal && (ordinal == 1 ? h.count == 0
                                                     : h.count == int(command_.entries.size()) &&
                                                           h.manifest == manifest_));
        auto args = key_args(command_.principal, command_.key);
        args.append(ordinal);
        require(s.execute({QString("SELECT 1 FROM main.subathon_review_entries WHERE ") +
                               key_where + " AND ordinal=?",
                           args})
                    .empty());
        const auto n = size_t(ordinal - 1);
        const auto &entry = command_.entries[n];
        Row input;
        StoredOutcome data;
        ReadCost cost;
        try {
            if (reconciliation_snapshot(s) != checked_snapshot_)
                committed_cost_ = read_prefix(s, command_, h, initial_cursor_).closure;
            ReadCost plan;
            input = read_row(s, auth, entry.intent, {}, {}, nullptr, &plan);
            data = proposed_outcome(command_, ordinal, input, prefix_.last_state, bases_[n],
                                    basis_selection_);
            cost = reconciliation_row(s, command_, entry, input, data, plan);
            if (!suffix_fits(n, cost))
                throw Oversize{};
        } catch (const Oversize &) {
            stop_ = ReviewStop::row_budget_exceeded;
            return false;
        } catch (...) {
            if (size_failure(std::current_exception()))
                stop_ = ReviewStop::row_budget_exceeded;
            throw;
        }
        staging_ = true;
        stage_row(s, command_, ordinal, input, data);
        const auto status = data.outcome.status;
        const auto verb = command_.entries[n].intent.verb;
        const auto kind = decision_kind(command_.entries[n].intent, status,
                                        verb == V::reject_held && !input.credited);
        append_journal(s, command_, ordinal, manifest_, data, kind);
        disclosed_ = input.disclose || !data.outcome.facts;
        if (data.outcome.facts && data.outcome.facts->bundle)
            staged_head_ = input.after.token;
        staged_ = std::move(data);
        reservations_[n] = cost;
        checked_snapshot_ = reconciliation_snapshot(s);
        return true;
    }

    void forward(int ordinal) {
        staging_ = false;
        auto self = shared_from_this();
        if (!enqueue(
                test_unit::row, ordinal,
                [self, ordinal](const auto &s) { return self->row(s, ordinal); },
                [self, ordinal](const auto &r, bool observed) {
                    self->after_row(r, ordinal, observed);
                })) {
            stop_ = ReviewStop::admission_refused;
            safe_stop();
        }
    }

    void after_row(const SqliteWorkResult &r, int ordinal, bool observed) {
        // Scope has no row effects to classify, even if rollback lost acknowledgment.
        if (finish_scope_stop(r, observed))
            return;
        if (r.transaction == TransactionOutcome::outcome_unknown) {
            stop_ = ReviewStop::commit_uncertain;
            failed_ = ordinal;
            safe_stop();
            return;
        }
        if (!normal(r, observed)) {
            failed_ = ordinal;
            if (r.transaction == TransactionOutcome::committed) {
                stop_ = ReviewStop::committed_result_unavailable;
                safe_stop();
            } else if (r.transaction == TransactionOutcome::rolled_back) {
                if (!staging_ &&
                    (stop_ == ReviewStop::row_budget_exceeded ||
                     r.error == WorkError::resource_limit || r.error == WorkError::result_limit)) {
                    stop_ = ReviewStop::row_budget_exceeded;
                    failed_.reset();
                } else {
                    stop_ = ReviewStop::write_failed;
                }
                reconcile();
            } else {
                stop_ = ReviewStop::unavailable;
                failed_.reset();
                safe_stop();
            }
            return;
        }
        if (stop_) {
            if (stop_ == ReviewStop::row_budget_exceeded)
                reconcile();
            else
                safe_stop();
            return;
        }
        if (!staged_) {
            stop_ = ReviewStop::committed_result_unavailable;
            failed_ = ordinal;
            safe_stop();
            return;
        }
        if (!staged_head_.empty()) {
            prefix_.last_state = std::move(staged_head_);
            staged_head_.clear();
            try {
                event(observer_, test_unit::row, phase::published, ordinal);
            } catch (...) {
                stop_ = ReviewStop::committed_result_unavailable;
                failed_ = ordinal;
                safe_stop();
                return;
            }
        }
        prefix_.reply.rows.push_back(
            disclosed_ ? ReviewRowResult{ordinal, D::committed, std::move(staged_->outcome)}
                       : ReviewRowResult{ordinal, {}, ReviewOutcome{S::denied, {}}});
        staged_.reset();
        const auto index = size_t(ordinal - 1);
        committed_cost_ += reservations_[index];
        ++prefix_.cursor;
        if (prefix_.cursor <= int(command_.entries.size()))
            qualify_next();
        else
            reconcile();
    }

    void reconcile() {
        auto self = shared_from_this();
        // The acceptance fault shares the provisional allocation's pre-enqueue boundary.
        if (const auto *probe = observer_.target<testing::FinalReviewAllocationObserver>();
            probe && probe->fault && std::exchange(probe->fault->armed, false)) {
            ++probe->fault->hits;
            throw std::bad_alloc{};
        }
        auto pending =
            std::make_shared<std::variant<ReviewRefusal, Prefix>>(ReviewRefusal::unavailable);
        if (!enqueue(
                test_unit::final_read, 0,
                [self, pending](const auto &s) {
                    const auto auth = self->authority(true);
                    if (const auto refusal = auth.scope(s)) {
                        *pending = *refusal;
                        return false;
                    }
                    catalog(s);
                    const auto h = header(s, self->command_.principal, self->command_.key);
                    require(h.cursor == self->prefix_.cursor &&
                            (h.count == 0 ? h.cursor == 1 : h.manifest == self->manifest_));
                    *pending = read_prefix(s, self->command_, h, self->initial_cursor_, &auth);
                    return false;
                },
                [self, pending](const auto &r, bool observed) {
                    if (!normal(r, observed)) {
                        if (self->stop_)
                            self->safe_stop();
                        else
                            self->finish(ReviewRefusal::unavailable);
                        return;
                    }
                    if (const auto *refusal = std::get_if<ReviewRefusal>(pending.get())) {
                        self->stop_ = *refusal == ReviewRefusal::denied ? ReviewStop::denied
                                                                        : ReviewStop::unavailable;
                        self->safe_stop();
                        return;
                    }
                    self->prefix_ = std::get<Prefix>(std::move(*pending));
                    if (self->stop_)
                        self->add_suffix(self->prefix_.reply);
                    self->finish(std::move(self->prefix_.reply));
                })) {
            if (stop_)
                safe_stop();
            else
                finish(ReviewRefusal::unavailable);
        }
    }

    void add_suffix(ReviewReply &reply) const {
        reply.stop = stop_;
        for (int ordinal = prefix_.cursor; ordinal <= int(command_.entries.size()); ++ordinal) {
            auto disposition = D::unattempted;
            if (ordinal == failed_) {
                if (stop_ == ReviewStop::commit_uncertain)
                    disposition = D::commit_uncertain;
                else if (stop_ == ReviewStop::committed_result_unavailable)
                    disposition = D::committed_result_unavailable;
                else if (stop_ == ReviewStop::write_failed)
                    disposition = D::write_failed;
            }
            reply.rows.push_back({ordinal, disposition, {}});
        }
    }

    void safe_stop() {
        ReviewReply safe;
        for (const auto &row : prefix_.reply.rows)
            safe.rows.push_back({row.ordinal, row.disposition, {}});
        add_suffix(safe);
        finish(std::move(safe));
    }
};

} // namespace subathon::server::review_detail
