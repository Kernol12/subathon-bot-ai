// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_reference_capture.hpp>

#include "reference_capture_codec_p.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <span>
#include <string_view>
#include <subathon/server/sqlite_application_holds.hpp>
#include <subathon/server/sqlite_application_plans.hpp>
#include <tuple>

namespace subathon::server {
namespace {
using Reason = ReferenceCaptureStorageReason;
using Disposition = ReferenceCaptureRetainDisposition;
using Input = ReferenceInputError;
using Views = std::span<const std::string_view>;
const QString capture_ddl = QString::fromLatin1(
    R"SQL(CREATE TABLE subathon_reference_initial_captures(installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), provider BLOB NOT NULL CHECK(typeof(provider)='blob' AND length(provider)>0), source_account BLOB NOT NULL CHECK(typeof(source_account)='blob' AND length(source_account)>0), identifier_namespace BLOB NOT NULL CHECK(typeof(identifier_namespace)='blob' AND length(identifier_namespace)>0), opaque_id BLOB NOT NULL CHECK(typeof(opaque_id)='blob' AND length(opaque_id)>0), receipt_id BLOB NOT NULL CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000'), contribution_id BLOB NOT NULL CHECK(typeof(contribution_id)='blob' AND length(contribution_id)=8), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0), arrival_context_ref BLOB NOT NULL CHECK(typeof(arrival_context_ref)='blob' AND length(arrival_context_ref)>0), event_ref BLOB NOT NULL CHECK(typeof(event_ref)='blob' AND length(event_ref)>0), plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), plan_revision BLOB NOT NULL CHECK(typeof(plan_revision)='blob' AND length(plan_revision)=8 AND plan_revision=X'0000000000000001'), execution_context_ref BLOB NOT NULL CHECK(typeof(execution_context_ref)='blob' AND length(execution_context_ref)>0), PRIMARY KEY(installation_ref,provider,source_account,identifier_namespace,opaque_id), UNIQUE(installation_ref,receipt_id), FOREIGN KEY(receipt_id) REFERENCES subathon_receipts(receipt_id), FOREIGN KEY(contribution_id) REFERENCES subathon_contributions(contribution_id), FOREIGN KEY(configuration_ref) REFERENCES subathon_retained_configurations(reference), FOREIGN KEY(arrival_context_ref) REFERENCES subathon_retained_contexts(reference), FOREIGN KEY(plan_ref,plan_revision) REFERENCES subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(execution_context_ref) REFERENCES subathon_retained_contexts(reference)))SQL");
[[noreturn]] void input_failure(Input e) { throw ReferenceInputFailure(e); }
struct Count {
    std::size_t limit, total{0};
    bool add(std::size_t n) {
        if (n > limit - total)
            return false;
        total += n;
        return true;
    }
};
void options(const ReferenceCaptureLimits &l) {
    const auto maximum =
        std::min({std::uintmax_t(std::numeric_limits<qsizetype>::max()),
                  std::uintmax_t(std::numeric_limits<qint64>::max()),
                  std::uintmax_t(std::numeric_limits<std::size_t>::max()),
                  std::uintmax_t(std::string{}.max_size()), std::uintmax_t(QByteArray::maxSize())});
    for (auto n : {l.reference_bytes, l.document_bytes, l.owned_bytes})
        if (n <= 0 || std::uintmax_t(n) >= maximum)
            input_failure(Input::invalid_limits);
}
auto key_views(const ReferenceCaptureKey &k) {
    return std::array<std::string_view, 5>{k.installation_ref, k.source.origin.provider,
                                           k.source.origin.source_account,
                                           k.source.identifier_namespace, k.source.opaque_id};
}
auto record_views(const ReferenceInitialCapture &r) {
    const auto k = key_views(r.key);
    return std::array<std::string_view, 10>{k[0],
                                            k[1],
                                            k[2],
                                            k[3],
                                            k[4],
                                            r.configuration_ref,
                                            r.arrival_context_ref,
                                            r.event_ref,
                                            r.plan.plan_ref,
                                            r.execution_context_ref};
}
void preflight(Views refs, std::size_t fixed, const ReferenceCaptureLimits &l) {
    options(l);
    Count n{std::size_t(l.owned_bytes)};
    if (!n.add(fixed))
        input_failure(Input::input_limit);
    for (const auto ref : refs)
        if (ref.size() > std::size_t(l.reference_bytes) || !n.add(ref.size()))
            input_failure(Input::input_limit);
    for (const auto ref : refs)
        if (ref.empty())
            input_failure(Input::invalid_reference);
}
QByteArray blob(std::string_view v) {
    return v.empty() ? QByteArray("", 0) : QByteArray(v.data(), qsizetype(v.size()));
}
bool same(const ReferenceInitialCapture &a, const ReferenceInitialCapture &b) {
    return a.key.installation_ref == b.key.installation_ref && a.key.source == b.key.source &&
           a.receipt_id == b.receipt_id && a.contribution_id == b.contribution_id &&
           std::tie(a.configuration_ref, a.arrival_context_ref, a.event_ref, a.plan.plan_ref,
                    a.plan.revision, a.execution_context_ref) ==
               std::tie(b.configuration_ref, b.arrival_context_ref, b.event_ref, b.plan.plan_ref,
                        b.plan.revision, b.execution_context_ref);
}
bool same(const ReferenceRule &a, const ReferenceRule &b) {
    return std::tie(a.rule_ref, a.rule_version_ref, a.action_ref, a.action_version_ref, a.timer_ref,
                    a.add_ms, a.automation.kind, a.automation.installation_ref,
                    a.automation.principal_ref, a.automation_capability_ref) ==
           std::tie(b.rule_ref, b.rule_version_ref, b.action_ref, b.action_version_ref, b.timer_ref,
                    b.add_ms, b.automation.kind, b.automation.installation_ref,
                    b.automation.principal_ref, b.automation_capability_ref);
}
// Descriptors refer only to fixed adapter-owned SQL identifiers. Scalars carry
// their semantic width; payloads are NEVER returned by the metadata query.
enum class Kind { reference, value, document, id, integer };
struct Column {
    const char *name;
    Kind kind;
    bool nullable{false};
    std::size_t scalar_bytes{8};
    bool required{false};
};
using Columns = std::span<const Column>;
constexpr Column ref(const char *n, bool optional = false) {
    return {n, Kind::reference, optional};
}
constexpr Column required_ref(const char *n, bool optional = false) {
    return {n, Kind::reference, optional, 8, true};
}
constexpr Column value(const char *n, bool optional = false) { return {n, Kind::value, optional}; }
constexpr Column id(const char *n, bool optional = false) { return {n, Kind::id, optional}; }
constexpr Column number(const char *n, std::size_t width = 8, bool optional = false) {
    return {n, Kind::integer, optional, width};
}
constexpr std::array index_columns{required_ref("installation_ref"),
                                   required_ref("provider"),
                                   required_ref("source_account"),
                                   required_ref("identifier_namespace"),
                                   required_ref("opaque_id"),
                                   id("receipt_id"),
                                   id("contribution_id"),
                                   required_ref("configuration_ref"),
                                   required_ref("arrival_context_ref"),
                                   required_ref("event_ref"),
                                   required_ref("plan_ref"),
                                   id("plan_revision"),
                                   required_ref("execution_context_ref")};
constexpr std::array receipt_columns{id("receipt_id"),
                                     required_ref("provider"),
                                     required_ref("source_account"),
                                     required_ref("identifier_namespace"),
                                     required_ref("opaque_id"),
                                     value("event_type"),
                                     number("schema_version", 4),
                                     number("source_timestamp_utc_ms", 8, true),
                                     value("actor_id", true),
                                     value("actor_display_name", true),
                                     value("money_original_decimal", true),
                                     value("money_currency", true),
                                     ref("payment_namespace", true),
                                     ref("payment_opaque_id", true),
                                     ref("activity_namespace", true),
                                     ref("activity_opaque_id", true),
                                     number("server_timestamp_utc_ms"),
                                     ref("connection_ref"),
                                     required_ref("configuration_ref"),
                                     required_ref("modifier_context_ref")};
constexpr std::array contribution_columns{id("contribution_id"),
                                          id("original_receipt_id"),
                                          required_ref("activity_provider", true),
                                          required_ref("activity_source_account", true),
                                          required_ref("activity_namespace", true),
                                          required_ref("activity_id", true)};
constexpr std::array observation_columns{id("receipt_id"),
                                         id("contribution_id", true),
                                         number("kind", 1),
                                         number("evidence", 1),
                                         required_ref("activity_provider", true),
                                         required_ref("activity_source_account", true),
                                         required_ref("activity_namespace", true),
                                         required_ref("activity_id", true),
                                         required_ref("verification_ref", true),
                                         number("support_monetary", 1, true),
                                         value("support_original_decimal", true),
                                         value("support_currency", true),
                                         ref("support_resolution_ref", true),
                                         number("support_missing_rate", 1, true),
                                         number("support_review", 1, true),
                                         number("support_evidence_policy", 1, true),
                                         ref("support_policy_ref", true),
                                         number("support_decision_choice", 1, true),
                                         ref("support_decision_ref", true),
                                         ref("support_decision_actor_ref", true),
                                         number("support_application", 1, true),
                                         number("support_reversal", 1, true),
                                         number("support_settlement", 1, true)};
constexpr std::array configuration_columns{required_ref("reference"),
                                           Column{"document", Kind::document}};
constexpr std::array context_columns{required_ref("reference"), Column{"document", Kind::document},
                                     required_ref("configuration_ref", true)};
constexpr std::array header_columns{
    required_ref("plan_ref"),       required_ref("event_ref"), ref("parent_event_ref", true),
    required_ref("root_event_ref"), id("receipt_id"),          id("contribution_id", true)};
constexpr std::array revision_columns{required_ref("plan_ref"),
                                      id("revision"),
                                      id("parent_revision", true),
                                      number("payment_evidence", 1),
                                      number("monetary", 1),
                                      value("original_decimal", true),
                                      value("currency", true),
                                      ref("resolution_ref", true),
                                      number("missing_rate", 1),
                                      number("review", 1),
                                      number("evidence_policy", 1),
                                      ref("policy_ref"),
                                      number("decision_choice", 1, true),
                                      ref("decision_ref", true),
                                      ref("decision_actor_ref", true),
                                      number("application", 1),
                                      number("reversal", 1),
                                      number("settlement", 1),
                                      number("valuation_state", 1),
                                      ref("historical_resolution_ref", true),
                                      value("historical_original_decimal", true),
                                      value("historical_currency", true),
                                      value("comparison_currency", true),
                                      value("effective_rate_decimal", true),
                                      value("fetched_rate_decimal", true),
                                      ref("snapshot_ref", true),
                                      ref("rate_source_ref", true),
                                      number("rate_timestamp_utc_ms", 8, true),
                                      number("fetch_timestamp_utc_ms", 8, true),
                                      ref("manual_decision_ref", true),
                                      number("override_authority", 1, true),
                                      ref("override_actor_ref", true),
                                      value("override_reason", true),
                                      ref("override_decision_ref", true)};
constexpr std::array rule_columns{
    required_ref("plan_ref"), id("revision"),        number("ordinal"),   ref("rule_ref"),
    ref("version_ref"),       number("selected", 1), value("explanation")};
constexpr std::array request_columns{required_ref("plan_ref"),
                                     id("revision"),
                                     number("ordinal"),
                                     ref("action_ref"),
                                     ref("action_version_ref"),
                                     ref("rule_ref"),
                                     ref("target_ref"),
                                     number("unit", 1),
                                     number("requires_historical_valuation", 1),
                                     value("exact_numerator"),
                                     value("exact_denominator"),
                                     number("requested_units"),
                                     value("calculation_explanation"),
                                     ref("reversal_behavior_ref")};
struct Metadata {
    bool null{false};
    qint64 length{0}, integer{0};
    std::uint64_t identifier{0};
};
struct Selection {
    const char *table;
    Columns columns;
    QString where;
    QList<QVariant> bindings;
    std::array<Metadata, 34> fields{};
    std::size_t bytes{0};
};
struct Refused {
    Disposition disposition;
};
class Access final {
  public:
    Access(SqliteWorkSession s, ReferenceCaptureLimits l) : session_(std::move(s)), limits_(l) {}
    template <class F> auto guarded(F body) -> decltype(body()) {
        try {
            return body();
        } catch (const ReferenceCaptureStorageFailure &) {
            throw;
        } catch (const SqliteWorkFailure &) {
            throw;
        } catch (...) {
            if (wrote_) {
                const auto error = session_.cleanup();
                if (error != WorkError::none)
                    throw SqliteWorkFailure(error);
            }
            throw;
        }
    }
    [[noreturn]] void fail(Reason reason) {
        session_.rollback();
        wrote_ = false;
        throw ReferenceCaptureStorageFailure(reason);
    }
    SqlRows execute(const SqlStatement &statement) {
        try {
            return session_.execute(statement);
        } catch (const SqliteWorkFailure &e) {
            std::optional<Reason> reason;
            switch (e.error()) {
            case WorkError::busy:
                reason = Reason::busy;
                break;
            case WorkError::sql_failure:
                reason = Reason::sql_failure;
                break;
            case WorkError::resource_limit:
                reason = Reason::resource_limit;
                break;
            case WorkError::result_limit:
                reason = Reason::result_limit;
                break;
            default:
                throw;
            }
            // The real writer has already established rollback for these errors.
            // Do not perform an inactive second control operation.
            wrote_ = false;
            throw ReferenceCaptureStorageFailure(*reason);
        }
    }
    static bool integer_type(const QVariant &v) {
        return !v.isNull() &&
               (v.metaType().id() == QMetaType::Int || v.metaType().id() == QMetaType::LongLong);
    }
    std::optional<Selection> metadata(const char *table, Columns cols, const QString &where,
                                      QList<QVariant> bindings) {
        QStringList projection;
        for (const auto &c : cols) {
            const auto n = QString::fromLatin1(c.name);
            projection.append("typeof(" + n + ")");
            projection.append("length(" + n + ")");
            projection.append(c.kind == Kind::id
                                  ? "CASE WHEN typeof(" + n + ")='blob' AND length(" + n +
                                        ")=8 THEN " + n + " ELSE NULL END"
                                  : "CASE WHEN typeof(" + n + ")='integer' THEN " + n +
                                        " ELSE NULL END");
        }
        const auto rows = execute({"SELECT " + projection.join(',') + " FROM main." + table +
                                       " WHERE " + where + " LIMIT 2",
                                   std::move(bindings)});
        if (rows.empty())
            return {};
        if (rows.size() != 1 || rows.front().size() != qsizetype(cols.size() * 3))
            fail(Reason::invalid_stored_data);
        Selection result{table, cols, where, {}, {}, 0};
        const auto &row = rows.front();
        Count size{std::size_t(limits_.owned_bytes)};
        for (std::size_t i = 0; i < cols.size(); ++i) {
            const auto &c = cols[i];
            const auto &type = row[qsizetype(i * 3)], &length = row[qsizetype(i * 3 + 1)],
                       &scalar = row[qsizetype(i * 3 + 2)];
            auto &m = result.fields[i];
            if (type.metaType().id() != QMetaType::QString)
                fail(Reason::invalid_stored_data);
            m.null = type.toString() == "null";
            if (m.null) {
                if (!c.nullable || !length.isNull() || !scalar.isNull())
                    fail(Reason::invalid_stored_data);
                continue;
            }
            if (!integer_type(length) || length.toLongLong() < 0)
                fail(Reason::invalid_stored_data);
            m.length = length.toLongLong();
            if (c.kind == Kind::integer) {
                if (type.toString() != "integer" || !integer_type(scalar))
                    fail(Reason::invalid_stored_data);
                m.integer = scalar.toLongLong();
                if (!size.add(c.scalar_bytes))
                    fail(Reason::result_limit);
            } else {
                if (type.toString() != "blob" || (c.kind != Kind::id && !scalar.isNull()) ||
                    (c.kind == Kind::id &&
                     (m.length != 8 || scalar.metaType().id() != QMetaType::QByteArray ||
                      scalar.toByteArray().size() != 8)) ||
                    (c.required && m.length == 0))
                    fail(Reason::invalid_stored_data);
                if (c.kind == Kind::id) {
                    // This CASE returns at most eight actual bytes, a fixed-width
                    // scalar rather than unchecked user payload.
                    m.identifier = decode_plan_revision_uint64(scalar.toByteArray());
                    if ((m.identifier == 0 && std::string_view(c.name) != "contribution_id") ||
                        (std::string_view(c.name) == "plan_revision" && m.identifier != 1))
                        fail(Reason::invalid_stored_data);
                }
                if ((c.kind == Kind::reference && m.length > limits_.reference_bytes) ||
                    (c.kind == Kind::document && m.length > limits_.document_bytes) ||
                    std::uintmax_t(m.length) >= std::uintmax_t(QByteArray::maxSize()) ||
                    !size.add(std::size_t(m.length)))
                    fail(Reason::result_limit);
            }
        }
        result.bytes = size.total;
        return result;
    }
    std::optional<Selection> select(const char *table, Columns cols, QString where,
                                    QList<QVariant> bindings) {
        auto result = metadata(table, cols, where, bindings);
        if (result)
            result->bindings = std::move(bindings);
        return result;
    }
    SqlRow payload(const Selection &s) {
        QStringList names;
        auto guards = s.where;
        auto bindings = s.bindings;
        for (std::size_t i = 0; i < s.columns.size(); ++i) {
            const auto &c = s.columns[i];
            const auto n = QString::fromLatin1(c.name);
            const auto &m = s.fields[i];
            names.append(n);
            guards += " AND typeof(" + n + ")='" +
                      (m.null                    ? "null"
                       : c.kind == Kind::integer ? "integer"
                                                 : "blob") +
                      "'";
            if (!m.null) {
                guards += " AND length(" + n + ")=?";
                bindings.append(m.length);
                if (c.kind == Kind::integer) {
                    guards += " AND " + n + "=?";
                    bindings.append(m.integer);
                } else if (c.kind == Kind::id) {
                    guards += " AND " + n + "=?";
                    bindings.append(encode_plan_revision_uint64(m.identifier));
                }
            }
        }
        auto rows = execute({"SELECT " + names.join(',') + " FROM main." + s.table + " WHERE " +
                                 guards + " LIMIT 2",
                             std::move(bindings)});
        if (rows.size() != 1 || rows.front().size() != qsizetype(s.columns.size()))
            fail(Reason::invalid_stored_data);
        auto result = std::move(rows.front());
        for (std::size_t i = 0; i < s.columns.size(); ++i) {
            const auto &v = result[qsizetype(i)];
            if (s.fields[i].null) {
                if (!v.isNull())
                    fail(Reason::invalid_stored_data);
            } else if (s.columns[i].kind == Kind::integer) {
                if (!integer_type(v) || v.toLongLong() != s.fields[i].integer)
                    fail(Reason::invalid_stored_data);
            } else if (!(s.fields[i].length == 0 && v.isNull()) &&
                       (v.metaType().id() != QMetaType::QByteArray ||
                        v.toByteArray().size() != s.fields[i].length))
                fail(Reason::invalid_stored_data);
        }
        return result;
    }
    // Semantic ownership is projected from declared DTO fields, not SQL keys,
    // ordinals, framing, QVariant buffers or a lifetime sum of separate reads.
    void add(Count &n, std::size_t bytes) {
        if (!n.add(bytes))
            fail(Reason::result_limit);
    }
    void field(Count &n, const Selection &s, std::size_t i) {
        if (!s.fields[i].null)
            add(n, std::size_t(s.fields[i].length));
    }
    void optional_field(Count &n, const Selection &s, std::size_t i) {
        add(n, 1);
        field(n, s, i);
    }
    void optional_scalar(Count &n, const Selection &s, std::size_t i, std::size_t width) {
        add(n, 1);
        if (!s.fields[i].null)
            add(n, width);
    }
    void identity_bytes(Count &n, const Selection &receipt) {
        for (std::size_t i = 1; i <= 4; ++i)
            field(n, receipt, i);
    }
    void facts_bytes(Count &n, const Selection &receipt) {
        field(n, receipt, 5);                // event type
        add(n, 4);                           // schema_version
        optional_scalar(n, receipt, 7, 8);   // source UTC
        optional_field(n, receipt, 8);       // actor ID
        optional_field(n, receipt, 9);       // display name
        for (std::size_t i : {10, 12, 14}) { // money/payment/activity optional groups
            add(n, 1);
            if (!receipt.fields[i].null) {
                field(n, receipt, i);
                field(n, receipt, i + 1);
            }
        }
    }
    void arrival_bytes(Count &n, const Selection &receipt) {
        add(n, 8);
        for (std::size_t i = 17; i <= 19; ++i)
            field(n, receipt, i);
    }
    void receipt_bytes(Count &n, const Selection &receipt) {
        add(n, 8); // ContributionReceipt.id
        identity_bytes(n, receipt);
        facts_bytes(n, receipt);
        arrival_bytes(n, receipt);
    }
    void conditions_bytes(Count &n, const Selection &s, std::size_t first) {
        // monetary, missing_rate, review, policy, application, reversal, settlement.
        add(n, 7);
        add(n, 1); // optional original money
        if (!s.fields[first + 1].null) {
            field(n, s, first + 1);
            field(n, s, first + 2);
        }
        optional_field(n, s, first + 3); // optional resolution
        field(n, s, first + 7);          // policy_ref
        add(n, 1);                       // optional review decision
        if (!s.fields[first + 8].null) {
            add(n, 1); // choice enum
            field(n, s, first + 9);
            field(n, s, first + 10);
        }
    }
    void valuation_bytes(Count &n, const Selection &revision) {
        add(n, 2); // state + optional HistoricalValuation presence
        if (revision.fields[18].integer != 2)
            return;
        for (std::size_t i : {19, 20, 21, 22, 23, 25})
            field(n, revision, i);
        for (std::size_t i : {24, 26, 29})
            optional_field(n, revision, i);
        optional_scalar(n, revision, 27, 8);
        optional_scalar(n, revision, 28, 8);
    }
    void bound_receipt(const Selection &receipt) {
        Count n{std::size_t(limits_.owned_bytes)};
        receipt_bytes(n, receipt);
    }
    void bound_contribution(const Selection &contribution) {
        Count n{std::size_t(limits_.owned_bytes)};
        add(n, 16); // immutable CID and original ReceiptId
        add(n, 1);  // optional verified activity identity
        if (!contribution.fields[2].null)
            for (std::size_t i = 2; i <= 5; ++i)
                field(n, contribution, i);
    }
    void bound_support(const Selection &support, const Selection &receipt) {
        Count n{std::size_t(limits_.owned_bytes)};
        receipt_bytes(n, receipt); // ContributionEvidenceRecord.observation.receipt
        add(n, 2);                 // observation kind and payment evidence
        add(n, 1);                 // optional verified activity/proof
        if (!support.fields[4].null)
            for (std::size_t i = 4; i <= 8; ++i)
                field(n, support, i);
        add(n, 1); // optional support conditions
        if (!support.fields[9].null)
            conditions_bytes(n, support, 9);
        optional_scalar(n, support, 1, 8); // optional associated CID (including0)
    }
    void bound_retained(const Selection &document, bool configuration) {
        Count n{std::size_t(limits_.owned_bytes)};
        add(n, 1); // RetainedKey.kind
        field(n, document, 0);
        field(n, document, 1);
        add(n, 1); // optional ConfigurationReference
        if (!configuration)
            field(n, document, 2);
    }
    void bound_plan(const Selection &header, const Selection &receipt, const Selection &revision,
                    const Selection &rule, const Selection &request) {
        Count n{std::size_t(limits_.owned_bytes)};
        field(n, header, 0); // Selected snapshot's plan_ref, once
        add(n, 8);           // CapturedPlanRevision.revision
        optional_scalar(n, revision, 2, 8);
        // PlanEvent owns its complete original receipt projection.
        field(n, header, 1);
        optional_field(n, header, 2);
        field(n, header, 3);
        add(n, 8); // event.receipt_id
        identity_bytes(n, receipt);
        facts_bytes(n, receipt);
        arrival_bytes(n, receipt);
        optional_scalar(n, header, 5, 8);
        add(n, 1); // captured ContributionState.payment_evidence
        conditions_bytes(n, revision, 4);
        valuation_bytes(n, revision);
        // The selected vectors are already proven exactly1 rule/0 modifiers/1 request.
        field(n, rule, 3);
        field(n, rule, 4);
        add(n, 1); // RuleSelection.selected
        field(n, rule, 6);
        for (std::size_t i = 3; i <= 6; ++i)
            field(n, request, i);
        add(n, 10);            // RequestedEffect unit + requires-historical + requested_units
        field(n, request, 9);  // canonical exact numerator
        field(n, request, 10); // canonical exact denominator
        field(n, request, 12);
        field(n, request, 13);
        add(n, 1); // optional override decision
        if (!revision.fields[30].null) {
            add(n, 1); // authority
            for (std::size_t i = 31; i <= 33; ++i)
                field(n, revision, i);
        }
    }
    Selection required(std::optional<Selection> s, bool indexed, bool intrinsic = false) {
        if (!s) {
            if (indexed || intrinsic)
                fail(Reason::invalid_stored_data);
            throw Refused{Disposition::missing_reference};
        }
        return std::move(*s);
    }
    void match(bool okay, bool indexed) {
        if (!okay) {
            if (indexed)
                fail(Reason::invalid_stored_data);
            throw Refused{Disposition::source_mismatch};
        }
    }
    void domain(const Selection &s, std::size_t i, qint64 low, qint64 high) {
        if (s.fields[i].null || s.fields[i].integer < low || s.fields[i].integer > high)
            fail(Reason::invalid_stored_data);
    }
    void group(const Selection &s, std::size_t first, std::size_t last) {
        for (auto i = first + 1; i <= last; ++i)
            if (s.fields[first].null != s.fields[i].null)
                fail(Reason::invalid_stored_data);
    }
    static QByteArray data(const SqlRow &r, std::size_t i) {
        const auto b = r[qsizetype(i)].toByteArray();
        return b.isEmpty() ? QByteArray("", 0) : b;
    }
    static std::string text(const SqlRow &r, std::size_t i) {
        const auto b = data(r, i);
        return {b.constData(), std::size_t(b.size())};
    }
    std::uint64_t identifier(const SqlRow &r, std::size_t i, bool positive = false) {
        const auto n = decode_plan_revision_uint64(data(r, i));
        if (positive && n == 0)
            fail(Reason::invalid_stored_data);
        return n;
    }
    static qint64 integer(const SqlRow &r, std::size_t i) { return r[qsizetype(i)].toLongLong(); }
    template <class Decode> auto decode(const QByteArray &document, Decode decoder) {
        ReferenceAdapterLimits l;
        l.reference_bytes = std::uint64_t(limits_.reference_bytes);
        l.payload_bytes = l.document_bytes = std::uint64_t(limits_.document_bytes);
        l.value_owned_bytes = std::uint64_t(limits_.owned_bytes);
        try {
            return decoder(std::string_view(document.constData(), std::size_t(document.size())), l,
                           reference_capture_detail::BodyBounds::retained);
        } catch (const ReferenceInputFailure &e) {
            if (e.code() == Input::allocation_failure || e.code() == Input::invalid_limits) {
                // Allocation failure is not evidence that persisted bytes were corrupt.
                const auto error = session_.cleanup();
                if (error != WorkError::none)
                    throw SqliteWorkFailure(error);
                throw;
            }
            fail(e.code() == Input::result_limit ? Reason::result_limit
                                                 : Reason::invalid_stored_data);
        }
    }
    struct Document {
        QByteArray bytes;
        std::optional<std::string> configuration;
    };
    Document retained(std::string_view reference, bool configuration, bool indexed,
                      bool intrinsic = false) {
        auto s = required(
            select(configuration ? "subathon_retained_configurations"
                                 : "subathon_retained_contexts",
                   configuration ? Columns(configuration_columns) : Columns(context_columns),
                   "reference=?", {blob(reference)}),
            indexed, intrinsic);
        bound_retained(s, configuration);
        const auto row = payload(s);
        if (text(row, 0) != reference)
            fail(Reason::invalid_stored_data);
        Document result{data(row, 1), {}};
        if (!configuration && !s.fields[2].null) {
            result.configuration = text(row, 2);
            // A context's own FK target must exist even before candidate comparison.
            (void)required(select("subathon_retained_configurations", configuration_columns,
                                  "reference=?", {blob(*result.configuration)}),
                           indexed, true);
        }
        return result;
    }
    std::optional<ReferenceInitialCapture> index(const ReferenceCaptureKey &k,
                                                 std::optional<core::ReceiptId> receipt = {}) {
        QString where;
        QList<QVariant> bindings;
        if (receipt) {
            where = "installation_ref=? AND receipt_id=?";
            bindings = {blob(k.installation_ref), encode_plan_revision_uint64(receipt->value)};
        } else {
            where = "installation_ref=? AND provider=? AND source_account=? AND "
                    "identifier_namespace=? AND opaque_id=?";
            for (auto v : key_views(k))
                bindings.append(blob(v));
        }
        auto s = select("subathon_reference_initial_captures", index_columns, where, bindings);
        if (!s)
            return {};
        const auto row = payload(*s);
        ReferenceInitialCapture r{
            {text(row, 0), {{text(row, 1), text(row, 2)}, text(row, 3), text(row, 4)}},
            {identifier(row, 5, true)},
            {identifier(row, 6)},
            text(row, 7),
            text(row, 8),
            text(row, 9),
            {text(row, 10), identifier(row, 11, true)},
            text(row, 12)};
        if (r.plan.revision != 1 || r.key.installation_ref != k.installation_ref ||
            (receipt ? r.receipt_id != *receipt : r.key.source != k.source))
            fail(Reason::invalid_stored_data);
        return r;
    }
    std::pair<Selection, SqlRow> receipt_target(std::uint64_t receipt) {
        const auto target = required(select("subathon_receipts", receipt_columns, "receipt_id=?",
                                            {encode_plan_revision_uint64(receipt)}),
                                     true, true);
        domain(target, 6, 0, std::numeric_limits<std::uint32_t>::max());
        group(target, 10, 11);
        group(target, 12, 13);
        group(target, 14, 15);
        bound_receipt(target);
        const auto row = payload(target);
        (void)retained(text(row, 18), true, true, true);
        (void)retained(text(row, 19), false, true, true);
        return {target, row};
    }
    void closure(const ReferenceInitialCapture &r, bool indexed);
    void stage(const ReferenceInitialCapture &r) {
        QStringList names, marks;
        for (const auto &c : index_columns) {
            names.append(c.name);
            marks.append("?");
        }
        QList<QVariant> values;
        for (auto v : key_views(r.key))
            values.append(blob(v));
        values.append(encode_plan_revision_uint64(r.receipt_id.value));
        values.append(encode_plan_revision_uint64(r.contribution_id.value));
        for (auto v :
             {std::string_view(r.configuration_ref), std::string_view(r.arrival_context_ref),
              std::string_view(r.event_ref), std::string_view(r.plan.plan_ref)})
            values.append(blob(v));
        values.append(encode_plan_revision_uint64(1));
        values.append(blob(r.execution_context_ref));
        wrote_ = true;
        (void)execute({"INSERT INTO main.subathon_reference_initial_captures(" + names.join(',') +
                           ") VALUES(" + marks.join(',') + ")",
                       std::move(values)});
    }
    struct ChildShape {
        qint64 count;
        bool dependent;
    };
    ChildShape children(const char *table, std::string_view plan) {
        const bool requests = std::string_view(table) == "subathon_plan_requests";
        const auto dependency =
            requests ? QString("coalesce(max(CASE WHEN typeof(requires_historical_valuation)="
                               "'integer' THEN requires_historical_valuation ELSE NULL END),0)")
                     : QString("0");
        const auto malformed =
            requests ? QString(" OR typeof(requires_historical_valuation)<>'integer' OR "
                               "requires_historical_valuation NOT IN (0,1)")
                     : QString{};
        // SQLite must project only fixed INTEGER/NULL scalars into Qt. The
        // independent bad-type count still inspects every selected row, including
        // malformed values mixed with otherwise valid extrema/dependency flags.
        const auto rows =
            execute({"SELECT count(*),"
                     "min(CASE WHEN typeof(ordinal)='integer' THEN ordinal ELSE NULL END),"
                     "max(CASE WHEN typeof(ordinal)='integer' THEN ordinal ELSE NULL END),"
                     "coalesce(sum(CASE WHEN typeof(ordinal)<>'integer' OR ordinal<1" +
                         malformed + " THEN 1 ELSE 0 END),0)," + dependency + " FROM main." +
                         QString::fromLatin1(table) + " WHERE plan_ref=? AND revision=?",
                     {blob(plan), encode_plan_revision_uint64(1)}});
        if (rows.size() != 1 || rows.front().size() != 5 || !integer_type(rows[0][0]) ||
            !integer_type(rows[0][3]) || !integer_type(rows[0][4]) || rows[0][3].toLongLong() != 0)
            fail(Reason::invalid_stored_data);
        const auto count = rows[0][0].toLongLong();
        if (count < 0 ||
            (count > 0 && (!integer_type(rows[0][1]) || !integer_type(rows[0][2]) ||
                           rows[0][1].toLongLong() != 1 || rows[0][2].toLongLong() != count)))
            fail(Reason::invalid_stored_data);
        return {count, rows[0][4].toLongLong() != 0};
    }

  private:
    SqliteWorkSession session_;
    ReferenceCaptureLimits limits_;
    bool wrote_{false};
};
void Access::closure(const ReferenceInitialCapture &r, bool indexed) {
    // Selected roots validate their own storage structure/targets before any
    // comparison with a newly proposed binding. Current state is never read.
    const auto receipt_shape = [&](const Selection &s) {
        domain(s, 6, 0, std::numeric_limits<std::uint32_t>::max());
        group(s, 10, 11);
        group(s, 12, 13);
        group(s, 14, 15);
    };
    const auto receipt = required(select("subathon_receipts", receipt_columns, "receipt_id=?",
                                         {encode_plan_revision_uint64(r.receipt_id.value)}),
                                  indexed);
    receipt_shape(receipt);
    bound_receipt(receipt);
    const auto original = payload(receipt);
    if (identifier(original, 0, true) != r.receipt_id.value)
        fail(Reason::invalid_stored_data);
    const auto cfg_ref = text(original, 18), arrival_ref = text(original, 19);
    auto cfg_document = retained(cfg_ref, true, indexed, true);
    auto arrival_document = retained(arrival_ref, false, indexed, true);
    if (!arrival_document.configuration)
        fail(Reason::invalid_stored_data);
    match(*arrival_document.configuration == cfg_ref, indexed);
    const core::ReceiptIdentity source{
        {text(original, 1), text(original, 2)}, text(original, 3), text(original, 4)};
    match(source == r.key.source && cfg_ref == r.configuration_ref &&
              arrival_ref == r.arrival_context_ref,
          indexed);
    bool absent_receipt_fields = true;
    for (std::size_t i = 8; i <= 15; ++i)
        absent_receipt_fields &= receipt.fields[i].null;
    match(text(original, 5) == "synthetic.reference.support.v1" && integer(original, 6) == 1 &&
              absent_receipt_fields,
          indexed);

    const auto contribution =
        required(select("subathon_contributions", contribution_columns, "contribution_id=?",
                        {encode_plan_revision_uint64(r.contribution_id.value)}),
                 indexed);
    group(contribution, 2, 5);
    bound_contribution(contribution);
    const auto c = payload(contribution);
    const auto original_receipt_id = identifier(c, 1, true);
    if (identifier(c, 0) != r.contribution_id.value)
        fail(Reason::invalid_stored_data);
    // An existing contribution's original support anchor is intrinsic, even if
    // the proposed source subsequently turns out to be a different receipt.
    std::optional<std::pair<Selection, SqlRow>> support_event;
    if (original_receipt_id != r.receipt_id.value)
        support_event = receipt_target(original_receipt_id);
    const auto support =
        required(select("subathon_contribution_observations", observation_columns, "receipt_id=?",
                        {encode_plan_revision_uint64(original_receipt_id)}),
                 indexed, true);
    domain(support, 2, 0, 1);
    domain(support, 3, 0, 3);
    group(support, 4, 8);
    if (support.fields[1].null || support.fields[2].integer != 0)
        fail(Reason::invalid_stored_data);
    for (auto [i, hi] : std::array<std::pair<std::size_t, qint64>, 7>{
             {{9, 1}, {13, 1}, {14, 3}, {15, 2}, {20, 1}, {21, 2}, {22, 1}}})
        domain(support, i, 0, hi);
    if (support.fields[16].null)
        fail(Reason::invalid_stored_data);
    group(support, 10, 11);
    group(support, 17, 19);
    if (!support.fields[17].null)
        domain(support, 17, 0, 1);
    bound_support(support, support_event ? support_event->first : receipt);
    const auto o = payload(support);
    if (identifier(o, 0, true) != original_receipt_id ||
        identifier(o, 1) != r.contribution_id.value)
        fail(Reason::invalid_stored_data);
    // The original support row names this same actual receipt (checked above).
    // Validate both activity origins and the complete anchor key before deciding
    // whether the healthy generic activity is supported by the reference profile.
    const auto &anchor_receipt = support_event ? support_event->second : original;
    const bool activity = !contribution.fields[2].null;
    if (activity != !support.fields[4].null)
        fail(Reason::invalid_stored_data);
    if (activity) {
        if (data(c, 2) != data(anchor_receipt, 1) || data(c, 3) != data(anchor_receipt, 2) ||
            data(o, 4) != data(anchor_receipt, 1) || data(o, 5) != data(anchor_receipt, 2))
            fail(Reason::invalid_stored_data);
        for (std::size_t i = 0; i < 4; ++i)
            if (data(c, 2 + i) != data(o, 4 + i))
                fail(Reason::invalid_stored_data);
    }
    match(original_receipt_id == r.receipt_id.value && contribution.fields[2].null &&
              support.fields[4].null && integer(o, 3) == 3 && integer(o, 9) == 0 &&
              support.fields[10].null && support.fields[12].null && integer(o, 13) == 0 &&
              integer(o, 14) == 1 && integer(o, 15) == 0 && text(o, 16) == cfg_ref &&
              support.fields[17].null && integer(o, 20) == 0 && integer(o, 21) == 0 &&
              integer(o, 22) == 1,
          indexed);

    const auto cfg =
        decode(cfg_document.bytes, reference_capture_detail::decode_configuration_body);
    const auto arrival =
        decode(arrival_document.bytes, reference_capture_detail::decode_arrival_body);
    // The retained bytes, not re-encoded output, supply the embedded-byte comparison.
    const auto embedded = [&]() -> std::string_view {
        const auto b = std::string_view(arrival_document.bytes.constData(),
                                        std::size_t(arrival_document.bytes.size()));
        std::size_t pos = 4;
        auto field = [&]() {
            if (b.size() - pos < 4)
                fail(Reason::invalid_stored_data);
            std::uint32_t n = 0;
            for (int i = 0; i < 4; ++i)
                n = (n << 8) | std::uint8_t(b[pos++]);
            if (n > b.size() - pos)
                fail(Reason::invalid_stored_data);
            const auto v = b.substr(pos, n);
            pos += n;
            return v;
        };
        (void)field();
        (void)field();
        if (b.size() - pos < 8)
            fail(Reason::invalid_stored_data);
        pos += 8;
        return field();
    }();
    const auto cfg_bytes =
        std::string_view(cfg_document.bytes.constData(), std::size_t(cfg_document.bytes.size()));
    const auto complete = ReferenceFactsState::complete;
    match(embedded == cfg_bytes && cfg.configuration_ref == cfg_ref &&
              cfg.synthetic_origin == source.origin &&
              cfg.receipt_namespace == source.identifier_namespace &&
              cfg.synthetic_origin.provider == "synthetic.reference.v1" &&
              cfg.other_rules == complete && cfg.modifiers == complete &&
              cfg.milestones == complete && cfg.outbound_actions == complete &&
              cfg.generated_events == complete && !cfg.timer_config.hard_end_utc_ms &&
              cfg.rule.automation.installation_ref == r.key.installation_ref &&
              arrival.arrival_context_ref == arrival_ref && arrival.event_ref == r.event_ref &&
              arrival.server_arrival_utc_ms == integer(original, 16) &&
              arrival.observation.connection_ref == text(original, 17) &&
              arrival.observation.event_id == source.opaque_id &&
              arrival.observation.source_utc_ms ==
                  (receipt.fields[7].null ? std::optional<std::int64_t>{}
                                          : std::optional<std::int64_t>{integer(original, 7)}),
          indexed);
    auto execution_document = retained(r.execution_context_ref, false, indexed);
    if (!execution_document.configuration)
        fail(Reason::invalid_stored_data);
    const auto execution =
        decode(execution_document.bytes, reference_capture_detail::decode_execution_body);
    match(*execution_document.configuration == cfg_ref &&
              execution.execution_context_ref == r.execution_context_ref &&
              execution.arrival_context_ref == arrival_ref &&
              execution.installation_ref == r.key.installation_ref && execution.source == source &&
              execution.receipt_id == r.receipt_id &&
              execution.contribution_id == r.contribution_id &&
              execution.plan.plan_ref == r.plan.plan_ref && execution.plan.revision == 1 &&
              same(execution.rule, cfg.rule),
          indexed);

    const auto header = required(
        select("subathon_plan_headers", header_columns, "plan_ref=?", {blob(r.plan.plan_ref)}),
        indexed);
    const auto header_receipt = header.fields[4].identifier;
    std::optional<std::pair<Selection, SqlRow>> other_event;
    if (header_receipt != r.receipt_id.value)
        other_event = receipt_target(header_receipt);
    const auto &event_metadata = other_event ? other_event->first : receipt;
    const auto &event = other_event ? other_event->second : original;
    if (!header.fields[5].null && header.fields[5].identifier != r.contribution_id.value) {
        const auto target =
            required(select("subathon_contributions", contribution_columns, "contribution_id=?",
                            {encode_plan_revision_uint64(header.fields[5].identifier)}),
                     indexed, true);
        group(target, 2, 5);
        receipt_target(target.fields[1].identifier);
    }
    const auto revision =
        required(select("subathon_plan_revisions", revision_columns, "plan_ref=? AND revision=?",
                        {blob(r.plan.plan_ref), encode_plan_revision_uint64(1)}),
                 indexed, true);
    domain(revision, 3, 0, 3);
    for (auto [i, hi] : std::array<std::pair<std::size_t, qint64>, 7>{
             {{4, 1}, {8, 1}, {9, 3}, {10, 2}, {15, 1}, {16, 2}, {17, 1}}})
        domain(revision, i, 0, hi);
    domain(revision, 18, 0, 2);
    group(revision, 5, 6);
    group(revision, 12, 14);
    if (!revision.fields[12].null)
        domain(revision, 12, 0, 1);
    if (!revision.fields[2].null)
        fail(Reason::invalid_stored_data);
    for (std::size_t i = 30; i < 34; ++i)
        if (!revision.fields[i].null)
            fail(Reason::invalid_stored_data);
    if (revision.fields[18].integer != 2) {
        for (std::size_t i = 19; i <= 29; ++i)
            if (!revision.fields[i].null)
                fail(Reason::invalid_stored_data);
    } else {
        for (std::size_t i : {19, 20, 21, 22, 23, 25})
            if (revision.fields[i].null)
                fail(Reason::invalid_stored_data);
    }
    const auto rule_shape = children("subathon_plan_rules", r.plan.plan_ref);
    const auto modifier_shape = children("subathon_plan_modifiers", r.plan.plan_ref);
    const auto request_shape = children("subathon_plan_requests", r.plan.plan_ref);
    if (revision.fields[18].integer == 1 && request_shape.dependent)
        fail(Reason::invalid_stored_data);
    const bool supported_shape =
        rule_shape.count == 1 && modifier_shape.count == 0 && request_shape.count == 1;
    std::optional<Selection> selected_rule, selected_request;
    if (supported_shape) {
        selected_rule = required(select("subathon_plan_rules", rule_columns,
                                        "plan_ref=? AND revision=? AND ordinal=1",
                                        {blob(r.plan.plan_ref), encode_plan_revision_uint64(1)}),
                                 indexed, true);
        selected_request = required(select("subathon_plan_requests", request_columns,
                                           "plan_ref=? AND revision=? AND ordinal=1",
                                           {blob(r.plan.plan_ref), encode_plan_revision_uint64(1)}),
                                    indexed, true);
        domain(*selected_rule, 5, 0, 1);
        domain(*selected_request, 7, 0, 2);
        domain(*selected_request, 8, 0, 1);
        bound_plan(header, event_metadata, revision, *selected_rule, *selected_request);
    }
    // An off-profile shape owns no selected typed snapshot. Its bounded revision
    // row can still establish intrinsic quote coupling before the narrow refusal.
    const auto v = payload(revision);
    if (revision.fields[18].integer == 2 &&
        (event_metadata.fields[10].null || data(v, 20) != data(event, 10) ||
         data(v, 21) != data(event, 11)))
        fail(Reason::invalid_stored_data);
    match(supported_shape, indexed);
    const auto &rule = *selected_rule, &request = *selected_request;
    const auto h = payload(header), rules = payload(rule), q = payload(request);
    if (identifier(v, 1, true) != 1 || text(v, 0) != r.plan.plan_ref ||
        identifier(rules, 1, true) != 1 || identifier(q, 1, true) != 1 ||
        text(rules, 0) != r.plan.plan_ref || text(q, 0) != r.plan.plan_ref)
        fail(Reason::invalid_stored_data);
    try {
        (void)decode_plan_exact({data(q, 9), data(q, 10)}, limits_.owned_bytes);
    } catch (const SqlitePlanInputFailure &e) {
        fail(e.error() == SqlitePlanInputError::exact_component_limit
                 ? Reason::result_limit
                 : Reason::invalid_stored_data);
    }
    bool state_equal = integer(v, 3) == integer(o, 3);
    for (std::size_t i = 0; i < 14; ++i) {
        const auto a = i + 4, b = i + 9;
        state_equal &= revision.fields[a].null == support.fields[b].null;
        if (!revision.fields[a].null && !support.fields[b].null)
            state_equal &= revision_columns[a].kind == Kind::integer
                               ? integer(v, a) == integer(o, b)
                               : data(v, a) == data(o, b);
    }
    std::array<char, 20> decimal{};
    const auto spelling =
        std::to_chars(decimal.data(), decimal.data() + decimal.size(), cfg.rule.add_ms);
    if (spelling.ec != std::errc{})
        fail(Reason::invalid_stored_data);
    const auto exact = std::string_view(decimal.data(), std::size_t(spelling.ptr - decimal.data()));
    const auto equals = [&](std::size_t field, std::string_view expected) {
        const auto bytes = data(q, field);
        return std::string_view(bytes.constData(), std::size_t(bytes.size())) == expected;
    };
    match(text(h, 0) == r.plan.plan_ref && text(h, 1) == r.event_ref && header.fields[2].null &&
              text(h, 3) == r.event_ref && header_receipt == r.receipt_id.value &&
              !header.fields[5].null && identifier(h, 5) == r.contribution_id.value &&
              state_equal && integer(v, 18) == 0 && text(rules, 3) == cfg.rule.rule_ref &&
              text(rules, 4) == cfg.rule.rule_version_ref && integer(rules, 5) == 1 &&
              text(rules, 6) == "reference.single-rule.match.v1" &&
              text(q, 3) == cfg.rule.action_ref && text(q, 4) == cfg.rule.action_version_ref &&
              text(q, 5) == cfg.rule.rule_ref && text(q, 6) == cfg.rule.timer_ref &&
              integer(q, 7) == 0 && integer(q, 8) == 0 && equals(9, exact) && equals(10, "1") &&
              integer(q, 11) == cfg.rule.add_ms && equals(12, "reference.integer-ms.v1") &&
              equals(13, "reference.original-time.v1"),
          indexed);
}
} // namespace

MigrationManifest reference_capture_repository_manifest() {
    auto manifest = application_hold_repository_manifest();
    MigrationStep step;
    step.version = 7;
    step.id = "reference-initial-captures-v1";
    step.sql = {capture_ddl};
    step.expected_schema = manifest.steps.back().expected_schema;
    const QString table = "subathon_reference_initial_captures";
    step.expected_schema.append({"table", table, table, capture_ddl});
    for (int n : {1, 2})
        step.expected_schema.append(
            {"index", "sqlite_autoindex_" + table + "_" + QString::number(n), table, std::nullopt});
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return std::tie(a.type, a.name) < std::tie(b.type, b.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions = {0, 1, 2, 3, 4, 5, 6, 7};
    return manifest;
}
std::optional<ReferenceInitialCapture>
SqliteReferenceCaptureStore::find(const ReferenceCaptureKey &key) const {
    const auto refs = key_views(key);
    preflight(refs, 0, limits_);
    Access access(session_, limits_);
    return access.guarded([&] {
        auto old = access.index(key);
        if (old)
            access.closure(*old, true);
        return old;
    });
}
ReferenceCaptureRetainDisposition
SqliteReferenceCaptureStore::retain_initial(const ReferenceInitialCapture &record) const {
    const auto refs = record_views(record);
    preflight(refs, 24, limits_);
    if (record.receipt_id.value == 0 || record.plan.revision != 1)
        input_failure(Input::invalid_value);
    Access access(session_, limits_);
    return access.guarded([&] {
        const auto source = access.index(record.key);
        if (source)
            access.closure(*source, true);
        const auto receipt = access.index(record.key, record.receipt_id);
        if (receipt && (!source || !same(*source, *receipt)))
            access.closure(*receipt, true);
        if (source || receipt)
            return source && receipt && same(*source, *receipt) && same(*source, record)
                       ? Disposition::already_present
                       : Disposition::conflict;
        try {
            access.closure(record, false);
        } catch (const Refused &e) {
            return e.disposition;
        }
        access.stage(record);
        return Disposition::inserted;
    });
}
} // namespace subathon::server
