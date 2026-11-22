// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/delivery_jobs.hpp>

#include "sqlite_delivery_event_read.hpp"

#include <QMetaType>
#include <limits>
#include <map>
#include <string_view>

namespace subathon::server {
namespace {

using State = DeliveryJobState;
using Disposition = DeliveryDisposition;
using Input = SqliteBundleInputError;
using Storage = SqliteBundleStorageReason;
constexpr SqliteBundleLimits limits;
constexpr auto interrupted = "interrupted-sending";

[[noreturn]] void invalid(Input reason) { throw SqliteBundleInputFailure(reason); }

[[noreturn]] void corrupt() { throw SqliteBundleStorageFailure(Storage::invalid_stored_data); }

// A repository exception must settle the caller unit before reporting a known
// storage error. Lost rollback acknowledgment supersedes the original error.
template <typename Work> auto guarded(const SqliteWorkSession &session, Work work) {
    try {
        return work();
    } catch (...) {
        const auto error = session.cleanup();
        if (error != WorkError::none)
            throw SqliteWorkFailure(error);
        throw;
    }
}

bool bounded_ref(std::string_view value) {
    return !value.empty() && value.size() <= std::size_t(limits.reference_bytes);
}

bool valid_key(const DeliveryJobKey &key) {
    return bounded_ref(key.operation.operation_scope) && bounded_ref(key.operation.operation_ref) &&
           bounded_ref(key.job_ref);
}

void validate_key(const DeliveryJobKey &key) {
    if (!valid_key(key))
        invalid(Input::invalid_key);
}

void validate_revision(std::int64_t revision) {
    if (revision < 1)
        invalid(Input::invalid_job);
}

QByteArray blob(std::string_view ref) { return QByteArray(ref.data(), qsizetype(ref.size())); }

QList<QVariant> bindings(const DeliveryJobKey &key) {
    return {blob(key.operation.operation_scope), blob(key.operation.operation_ref),
            blob(key.job_ref)};
}

constexpr auto key_match = "operation_scope=? AND operation_ref=? AND job_ref=?";

// Bound the materialized value in SQLite, before the driver allocates a BLOB.
// An out-of-contract ref projects as NULL and is rejected by the decoder.
QString ref_column(const QString &column) {
    return "CASE WHEN typeof(" + column + ")='blob' AND length(" + column +
           ") BETWEEN 1 AND 4096 THEN " + column + " END";
}

std::int64_t integer(const QVariant &cell) {
    const auto type = cell.metaType().id();
    if (cell.isNull() || (type != QMetaType::LongLong && type != QMetaType::Int))
        corrupt();
    return cell.toLongLong();
}

std::string reference(const QVariant &cell) {
    if (cell.isNull() || cell.metaType().id() != QMetaType::QByteArray)
        corrupt();
    const auto bytes = cell.toByteArray();
    const auto ref = std::string_view(bytes.constData(), std::size_t(bytes.size()));
    if (!bounded_ref(ref))
        corrupt();
    return std::string(ref);
}

using delivery_sql::Event;

Event latest_event(const SqliteWorkSession &session, const DeliveryJobKey &key) {
    return delivery_sql::latest_event(
        session,
        QString("main.subathon_delivery_events INDEXED BY "
                "sqlite_autoindex_subathon_delivery_events_1 WHERE ") +
            key_match,
        bindings(key));
}

enum class Document { metadata, body };

std::optional<DeliveryJob> selected_capture(const SqliteWorkSession &session,
                                            const DeliveryJobKey &key, Document mode) {
    QString sql = "SELECT " + ref_column("c.route_ref") +
                  ",CASE WHEN typeof(c.selection)='integer' THEN c.selection END," +
                  ref_column("c.sender_ref") + "," + ref_column("c.action_ref") + "," +
                  ref_column("c.action_version_ref") +
                  ",CASE WHEN typeof(j.format_ref)='blob' AND j.format_ref=? "
                  "AND typeof(j.document)='blob' AND length(j.document)<=262144 THEN 1 ELSE 0 END";
    if (mode == Document::body)
        sql += ",CASE WHEN typeof(j.document)='blob' AND length(j.document)<=262144 "
               "THEN j.document END";
    sql += " FROM main.subathon_delivery_captures AS c LEFT JOIN main.subathon_app_jobs AS j "
           "ON (j.operation_scope,j.operation_ref,j.job_ref)=(c.operation_scope,c.operation_ref,"
           "c.job_ref) WHERE c.operation_scope=? AND c.operation_ref=? AND c.job_ref=?";
    auto args = bindings(key);
    args.prepend(blob(delivery_job_format));
    const auto rows = session.execute({std::move(sql), std::move(args)});
    if (rows.empty())
        return std::nullopt;
    if (rows.size() != 1)
        corrupt();
    const auto &row = rows.front();
    const auto selection = integer(row[1]);
    if (selection < 0 || selection > int(DeliverySenderSelection::explicit_identity) ||
        integer(row[5]) != 1)
        corrupt();
    DeliveryJob job;
    job.key = key;
    job.capture = {key.job_ref,       reference(row[0]), DeliverySenderSelection(selection),
                   reference(row[2]), reference(row[3]), reference(row[4])};
    if (mode == Document::body) {
        if (row[6].metaType().id() != QMetaType::QByteArray)
            corrupt();
        job.action_document = row[6].toByteArray();
        // The validated SQL storage class also distinguishes an empty BLOB
        // when the driver exposes its byte buffer as a null QByteArray.
        if (job.action_document.isNull())
            job.action_document = QByteArray("", 0);
    }
    return job;
}

// A v18 application membership continues to come only from v12 action
// rows. The envelope's generic group metadata supplies no membership.
std::optional<bool> v18_application_release(const SqliteWorkSession &session,
                                            const DeliveryJob &job) {
    QList<QVariant> args{blob(job.capture.action_ref), blob(job.capture.action_version_ref)};
    args.append(bindings(job.key));
    const auto members = session.execute(
        {"SELECT g.released,CASE WHEN typeof(a.action_ordinal)='integer' AND a.action_ordinal>=1 "
         "AND typeof(a.kind)='integer' AND a.kind=0 AND a.action_ref=? AND a.version_ref=? "
         "AND (g.operation_scope,g.operation_ref)=(a.operation_scope,a.operation_ref) "
         "AND NOT EXISTS(SELECT 1 FROM main.subathon_milestone_edit_occurrences e "
         "WHERE e.occurrence_ref=g.occurrence_ref OR e.queue_order=g.queue_order) "
         "THEN 1 ELSE 0 END FROM main.subathon_milestone_actions a "
         "LEFT JOIN main.subathon_milestone_groups g ON g.occurrence_ref=a.occurrence_ref "
         "WHERE a.operation_scope=? AND a.operation_ref=? AND a.job_ref=?",
         args});
    if (members.empty())
        return {};
    if (members.size() != 1 || integer(members.front()[1]) != 1)
        corrupt();
    const auto released = integer(members.front()[0]);
    if (released != 0 && released != 1)
        corrupt();
    return released == 1;
}

std::optional<DeliveryJob> selected_job(const SqliteWorkSession &session,
                                        const DeliveryJobKey &key,
                                        bool *interruptions = nullptr) {
    const auto current = milestone_sql::catalog(session, interruptions);
    auto job = selected_capture(session, key, Document::body);
    if (job) {
        auto event = latest_event(session, key);
        job->state = event.state;
        job->revision = event.revision;
        job->attempt = event.attempt;
        job->evidence_ref = std::move(event.evidence);
        if (current == milestone_sql::Catalog::edits) {
            const auto released = v18_application_release(session, *job);
            if (released == false && job->revision != 1)
                corrupt();
        }
    }
    return job;
}

bool application_start_fenced(const SqliteWorkSession &s, const DeliveryJobKey &key) {
    const auto rows = s.execute(
        {"SELECT 1 FROM main.subathon_milestone_actions a "
         "JOIN main.subathon_milestone_groups g ON g.occurrence_ref=a.occurrence_ref "
         "AND (g.operation_scope,g.operation_ref)=(a.operation_scope,a.operation_ref) "
         "WHERE a.operation_scope=? AND a.operation_ref=? AND a.job_ref=? AND " +
             milestone_sql::original_start_fenced(false, "a.action_ordinal") + " LIMIT 1",
         bindings(key)});
    return !rows.empty();
}

bool milestone_release_allows(const SqliteWorkSession &session, const DeliveryJob &job) {
    bool interruptions = false;
    const auto catalog = milestone_sql::catalog(session, &interruptions);
    if (catalog == milestone_sql::Catalog::independent)
        return true;
    if (catalog == milestone_sql::Catalog::edits)
        return v18_application_release(session, job).value_or(true) &&
               (!interruptions || !application_start_fenced(session, job.key));

    QList<QVariant> args{blob(job.capture.action_ref), blob(job.capture.action_version_ref)};
    args.append(bindings(job.key));
    // Preserve the membership row even when its group is missing. SQL failures
    // here roll back the claim and cannot select the older-catalog path.
    const auto members = session.execute(
        {"SELECT CASE WHEN typeof(a.action_ordinal)='integer' AND a.action_ordinal>=1 "
         "AND typeof(a.kind)='integer' AND a.kind=0 AND a.action_ref=? AND a.version_ref=? "
         "AND typeof(g.released)='integer' AND g.released=1 "
         "AND g.operation_scope=a.operation_scope AND g.operation_ref=a.operation_ref "
         "THEN 1 ELSE 0 END FROM main.subathon_milestone_actions AS a "
         "LEFT JOIN main.subathon_milestone_groups AS g ON g.occurrence_ref=a.occurrence_ref "
         "WHERE a.operation_scope=? AND a.operation_ref=? AND a.job_ref=?",
         std::move(args)});
    if (members.empty())
        return true; // No typed membership for this exact job.
    return members.size() == 1 && integer(members.front().front()) == 1;
}

void append_event(const SqliteWorkSession &session, const DeliveryJobKey &key, const Event &event) {
    auto args = bindings(key);
    args.append({qint64(event.revision), int(event.state),
                 event.attempt ? QVariant(qint64(*event.attempt)) : QVariant{},
                 event.evidence ? QVariant(blob(*event.evidence)) : QVariant{}});
    (void)session.execute(
        {"INSERT INTO main.subathon_delivery_events("
         "operation_scope,operation_ref,job_ref,revision,state,attempt,evidence_ref)"
         " VALUES(?,?,?,?,?,?,?)",
         std::move(args)});
}

DeliveryJobResult transition(const SqliteWorkSession &session, DeliveryJob job, State next,
                             std::optional<std::int64_t> attempt,
                             std::optional<std::string> evidence = {}) {
    if (job.revision == std::numeric_limits<std::int64_t>::max())
        throw SqliteBundleStorageFailure(Storage::sequence_exhausted);
    ++job.revision;
    job.state = next;
    job.attempt = next == State::sending ? job.revision : attempt;
    job.evidence_ref = std::move(evidence);
    append_event(session, job.key, {job.state, job.revision, job.attempt, job.evidence_ref});
    return {Disposition::changed, std::move(job)};
}

template <typename Change>
DeliveryJobResult change_selected(const SqliteWorkSession &session, const DeliveryJobKey &key,
                                  std::int64_t revision, State expected, Change change,
                                  bool *interruptions = nullptr) {
    return guarded(session, [&]() -> DeliveryJobResult {
        auto job = selected_job(session, key, interruptions);
        if (!job)
            return {Disposition::missing, {}};
        if (job->revision != revision)
            return {Disposition::stale, {}};
        if (job->state != expected)
            return {Disposition::refused, {}};
        return change(std::move(*job));
    });
}

using CaptureSet = std::map<std::string_view, const DeliveryCapture *, std::less<>>;
using MarkedJobs = std::map<std::string_view, const ApplicationJobIntent *, std::less<>>;

CaptureSet validate_captures(const PreparedApplicationBundle &bundle,
                             const std::vector<DeliveryCapture> &captures) {
    if (bundle.jobs.size() > std::size_t(limits.jobs_per_bundle))
        invalid(Input::collection_limit);
    if (captures.size() > bundle.jobs.size())
        invalid(Input::invalid_job);
    std::size_t remaining = std::size_t(limits.owned_bytes);
    for (const auto &capture : captures) {
        if (capture.selection != DeliverySenderSelection::configured_default &&
            capture.selection != DeliverySenderSelection::explicit_identity)
            invalid(Input::invalid_job);
        for (const auto *ref : {&capture.job_ref, &capture.route_ref, &capture.sender_ref,
                                &capture.action_ref, &capture.action_version_ref}) {
            if (!bounded_ref(*ref))
                invalid(Input::invalid_job);
            if (ref->size() > remaining)
                invalid(Input::owned_byte_limit);
            remaining -= ref->size();
        }
    }
    CaptureSet matched;
    for (const auto &capture : captures)
        if (!matched.emplace(capture.job_ref, &capture).second)
            invalid(Input::invalid_job);
    std::size_t marked = 0;
    for (const auto &job : bundle.jobs) {
        if (job.envelope.format_ref != delivery_job_format)
            continue;
        if (!bounded_ref(job.job_ref) || !matched.contains(job.job_ref))
            invalid(Input::invalid_job);
        ++marked;
    }
    // The underlying bundle preflight also rejects duplicate intent keys.
    if (marked != matched.size())
        invalid(Input::invalid_job);
    return matched;
}

bool same_capture(const DeliveryCapture &left, const DeliveryCapture &right) {
    return left.job_ref == right.job_ref && left.route_ref == right.route_ref &&
           left.selection == right.selection && left.sender_ref == right.sender_ref &&
           left.action_ref == right.action_ref &&
           left.action_version_ref == right.action_version_ref;
}

MarkedJobs marked_jobs(const std::vector<ApplicationJobIntent> &jobs) {
    MarkedJobs marked;
    for (const auto &job : jobs)
        if (job.envelope.format_ref == delivery_job_format)
            marked.emplace(job.job_ref, &job);
    return marked;
}

bool same_enrollment(const SqliteWorkSession &session, const PreparedApplicationBundle &bundle,
                     const RecordedApplicationBundle &original, const CaptureSet &captures) {
    const auto requested = marked_jobs(bundle.jobs);
    const auto retained = marked_jobs(original.jobs);
    if (requested.size() != retained.size())
        return false;
    for (const auto &[ref, intent] : requested) {
        const auto old = retained.find(ref);
        if (old == retained.end() ||
            old->second->envelope.format_ref != intent->envelope.format_ref ||
            old->second->envelope.document != intent->envelope.document)
            return false;
        const DeliveryJobKey key{bundle.intent.operation, intent->job_ref};
        const auto capture = selected_capture(session, key, Document::metadata);
        if (!capture || !same_capture(capture->capture, *captures.at(ref)))
            return false;
        (void)latest_event(session, key);
    }
    return true;
}

void enroll(const SqliteWorkSession &session, const ApplicationOperationKey &operation,
            const DeliveryCapture &capture) {
    const DeliveryJobKey key{operation, capture.job_ref};
    auto args = bindings(key);
    args.append({blob(capture.route_ref), int(capture.selection), blob(capture.sender_ref),
                 blob(capture.action_ref), blob(capture.action_version_ref)});
    (void)session.execute({"INSERT INTO main.subathon_delivery_captures("
                           "operation_scope,operation_ref,job_ref,route_ref,selection,sender_ref,"
                           "action_ref,action_version_ref) VALUES(?,?,?,?,?,?,?,?)",
                           std::move(args)});
    append_event(session, key, {State::queued, 1, {}, {}});
}

State evidence_state(DeliveryEvidence evidence) {
    switch (evidence) {
    case DeliveryEvidence::delivered:
        return State::delivered;
    case DeliveryEvidence::known_undelivered:
        return State::failed;
    case DeliveryEvidence::indeterminate:
        return State::outcome_unknown;
    }
    invalid(Input::invalid_job);
}

} // namespace

ApplicationBundleStageResult
SqliteDeliveryJobStore::stage(const PreparedApplicationBundle &bundle,
                              const std::vector<DeliveryCapture> &captures) const {
    const auto matched = validate_captures(bundle, captures);
    // The existing store owns bundle preflight and its failure translations.
    auto result = SqliteApplicationBundleStore(session_).stage(bundle);
    return guarded(session_, [&] {
        using Stage = SqliteBundleStageDisposition;
        if (result.disposition == Stage::staged) {
            for (const auto &[ref, capture] : matched)
                enroll(session_, bundle.intent.operation, *capture);
        } else if (result.disposition == Stage::already_present &&
                   !same_enrollment(session_, bundle, *result.existing, matched)) {
            return ApplicationBundleStageResult{Stage::operation_conflict, {}};
        }
        return std::move(result);
    });
}

std::optional<DeliveryJob> SqliteDeliveryJobStore::lookup(const DeliveryJobKey &key) const {
    validate_key(key);
    return guarded(session_, [&] { return selected_job(session_, key); });
}

DeliveryJobResult SqliteDeliveryJobStore::finish(const DeliveryJobKey &key, std::int64_t revision,
                                                 std::int64_t attempt, DeliveryEvidence evidence,
                                                 const std::string &evidence_ref) const {
    validate_key(key);
    validate_revision(revision);
    validate_revision(attempt);
    const auto next = evidence_state(evidence);
    if (!bounded_ref(evidence_ref))
        invalid(Input::invalid_job);
    return change_selected(session_, key, revision, State::sending, [&](DeliveryJob job) {
        if (job.attempt != attempt)
            return DeliveryJobResult{Disposition::stale, {}};
        return transition(session_, std::move(job), next, attempt, evidence_ref);
    });
}

StrandedDeliveryPage SqliteDeliveryJobStore::stranded(const std::optional<DeliveryJobKey> &after,
                                                      std::size_t limit) const {
    if (limit == 0 || limit > stranded_delivery_page_limit)
        invalid(Input::invalid_options);
    if (after)
        validate_key(*after);
    return guarded(session_, [&] {
        const auto current = milestone_sql::catalog(session_);
        // Page by examined capture keys, even when a page has no sending jobs.
        // The ordered PK range has no state predicate or offset.
        QString sql = "SELECT " + ref_column("operation_scope") + "," +
                      ref_column("operation_ref") + "," + ref_column("job_ref") +
                      " FROM main.subathon_delivery_captures INDEXED BY "
                      "sqlite_autoindex_subathon_delivery_captures_1";
        QList<QVariant> args;
        if (after) {
            sql += " WHERE (operation_scope,operation_ref,job_ref)>(?,?,?)";
            args = bindings(*after);
        }
        sql += " ORDER BY operation_scope,operation_ref,job_ref LIMIT ?";
        args.append(qint64(limit));
        const auto rows = session_.execute({std::move(sql), std::move(args)});
        StrandedDeliveryPage page;
        for (const auto &row : rows) {
            DeliveryJobKey key{{reference(row[0]), reference(row[1])}, reference(row[2])};
            const auto job = selected_capture(session_, key, Document::metadata);
            if (!job)
                corrupt();
            const auto event = latest_event(session_, key);
            if (current == milestone_sql::Catalog::edits) {
                const auto released = v18_application_release(session_, *job);
                if (released == false && event.revision != 1)
                    corrupt();
            }
            if (event.state == State::sending)
                page.attempts.push_back({key, event.revision});
            if (rows.size() == qsizetype(limit) && &row == &rows.back())
                page.next_after = std::move(key);
        }
        return page;
    });
}

DeliveryJobResult SqliteDeliveryJobStore::recover(const DeliveryJobKey &key,
                                                  std::int64_t revision) const {
    validate_key(key);
    validate_revision(revision);
    return change_selected(session_, key, revision, State::sending, [&](DeliveryJob job) {
        const auto attempt = job.attempt;
        return transition(session_, std::move(job), State::outcome_unknown, attempt, interrupted);
    });
}

DeliveryJobResult SqliteDeliveryJobStore::retry(const DeliveryJobKey &key, std::int64_t revision,
                                                DeliveryRetryPolicy &policy) const {
    validate_key(key);
    validate_revision(revision);
    bool interruptions = false;
    return change_selected(session_, key, revision, State::failed, [&](DeliveryJob job) {
        if (interruptions && application_start_fenced(session_, job.key))
            return DeliveryJobResult{Disposition::refused, {}};
        auto decision = policy.allows(job);
        if (!decision)
            return DeliveryJobResult{Disposition::refused, {}};
        if (!bounded_ref(*decision))
            invalid(Input::invalid_job);
        return transition(session_, std::move(job), State::queued, {}, std::move(decision));
    }, &interruptions);
}

StorageError dispatch_delivery_job(SqliteWriter &writer, DeliveryJobKey key, std::int64_t revision,
                                   std::shared_ptr<DeliveryDispatchAuthority> authority,
                                   std::shared_ptr<DeliveryAdapter> adapter,
                                   DeliveryDispatchCallback completion) {
    if (!valid_key(key) || revision < 1 || !authority || !adapter)
        return StorageError::invalid_options;
    if (!completion)
        return StorageError::invalid_work;

    struct Publication {
        std::optional<DeliveryJobResult> result;
        std::optional<DeliveryJob> start;
    };
    const auto pending = std::make_shared<Publication>();
    return writer.enqueue_work(
        [key = std::move(key), revision, authority = std::move(authority),
         pending](SqliteWorkContext context) -> QVariant {
            const auto session = context.begin();
            auto result =
                change_selected(session, key, revision, State::queued, [&](DeliveryJob job) {
                    if (!milestone_release_allows(session, job) || !authority->allows(job))
                        return DeliveryJobResult{Disposition::refused, {}};
                    return transition(session, std::move(job), State::sending, {});
                });
            // Prepare both publications before COMMIT. A failed copy must not
            // leave a committed claim waiting for an allocation before start.
            pending->start = result.job;
            auto payload = QVariant::fromValue(result);
            pending->result = std::move(result);
            session.commit();
            return payload;
        },
        [pending, adapter = std::move(adapter),
         completion = std::move(completion)](SqliteWorkResult work) {
            DeliveryDispatchCompletion reply{std::move(work), {}, false};
            if (reply.work.transaction == TransactionOutcome::committed &&
                reply.work.error == WorkError::none && reply.work.value &&
                reply.work.value->metaType() == QMetaType::fromType<DeliveryJobResult>()) {
                reply.result = std::move(pending->result);
                if (pending->start) {
                    try {
                        adapter->start(std::move(*pending->start));
                    } catch (...) {
                        reply.adapter_threw = true;
                    }
                }
            }
            completion(std::move(reply));
        });
}

} // namespace subathon::server
