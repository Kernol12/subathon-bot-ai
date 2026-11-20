// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_application_bundle_access.hpp"
#include "sqlite_financial_hold_access.hpp"
#include "sqlite_financial_values.hpp"
#include "sqlite_milestone_edit_detail.hpp"
#include "sqlite_receipt_read.hpp"

#include <QCryptographicHash>
#include <subathon/core/refund_entitlement.hpp>
#include <subathon/server/sqlite_modifier_activations.hpp>

namespace subathon::server {
MigrationManifest financial_refunds_repository_manifest() {
    auto manifest = modifier_activations_repository_manifest();
    MigrationStep step{15, "financial-refunds-v1", {}, manifest.steps.back().expected_schema};
    step.sql =
        {
            R"sql(CREATE TABLE subathon_financial_refunds(operation_scope BLOB NOT NULL, operation_ref BLOB NOT NULL, payment BLOB NOT NULL, position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), receipt_id BLOB NOT NULL UNIQUE, economic BLOB NOT NULL UNIQUE, subject BLOB NOT NULL CHECK(typeof(subject)='blob' AND length(subject)=8), time_ordinal INTEGER CHECK(time_ordinal IS NULL OR (typeof(time_ordinal)='integer' AND time_ordinal>0)), restored INTEGER NOT NULL CHECK(typeof(restored)='integer' AND restored>=0), hold_ref BLOB UNIQUE, document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 262144), PRIMARY KEY(operation_scope,operation_ref), UNIQUE(payment,position), FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_app_operations(operation_scope,operation_ref), FOREIGN KEY(receipt_id) REFERENCES subathon_receipts(receipt_id)))sql",
            R"sql(CREATE TABLE subathon_financial_payment_heads(payment BLOB NOT NULL PRIMARY KEY, support_receipt BLOB NOT NULL UNIQUE, position INTEGER NOT NULL, FOREIGN KEY(support_receipt) REFERENCES subathon_receipts(receipt_id), FOREIGN KEY(payment,position) REFERENCES subathon_financial_refunds(payment,position)))sql",
            R"sql(CREATE TABLE subathon_financial_bases(original_scope BLOB NOT NULL, original_operation BLOB NOT NULL, original_ordinal INTEGER NOT NULL, kind INTEGER NOT NULL CHECK(kind IN(0,2,3)), position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), operation_scope BLOB NOT NULL, operation_ref BLOB NOT NULL, PRIMARY KEY(original_scope,original_operation,original_ordinal,kind), FOREIGN KEY(original_scope,original_operation,original_ordinal) REFERENCES subathon_app_effects(operation_scope,operation_ref,request_ordinal), FOREIGN KEY(operation_scope,operation_ref) REFERENCES subathon_financial_refunds(operation_scope,operation_ref)))sql",
            R"sql(CREATE TABLE subathon_financial_resolutions(operation_scope BLOB NOT NULL, operation_ref BLOB NOT NULL, hold_ref BLOB NOT NULL UNIQUE, before_token BLOB NOT NULL, advanced_token BLOB NOT NULL, after_token BLOB NOT NULL UNIQUE, document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 1 AND 262144), PRIMARY KEY(operation_scope,operation_ref), FOREIGN KEY(hold_ref) REFERENCES subathon_financial_refunds(hold_ref), FOREIGN KEY(before_token) REFERENCES subathon_app_states(token), FOREIGN KEY(advanced_token) REFERENCES subathon_app_states(token), FOREIGN KEY(after_token) REFERENCES subathon_app_states(token)))sql",
            R"sql(CREATE INDEX subathon_financial_holds_by_subject ON subathon_financial_refunds(subject,hold_ref) WHERE hold_ref IS NOT NULL)sql",
    };
    const std::array tables{
        std::pair{"subathon_financial_refunds", 5},
        std::pair{"subathon_financial_payment_heads", 2},
        std::pair{"subathon_financial_bases", 1},
        std::pair{"subathon_financial_resolutions", 3},
    };
    for (qsizetype i = 0; i < qsizetype(tables.size()); ++i) {
        const QString table = QString::fromLatin1(tables[size_t(i)].first);
        step.expected_schema.append({"table", table, table, step.sql[i]});
        for (int index = 1; index <= tables[size_t(i)].second; ++index)
            step.expected_schema.append(
                {"index", "sqlite_autoindex_" + table + "_" + QString::number(index), table, {}});
    }
    step.expected_schema.append({"index", "subathon_financial_holds_by_subject",
                                 "subathon_financial_refunds", step.sql[4]});
    std::ranges::sort(step.expected_schema, [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(15);
    return manifest;
}

namespace {
namespace fs = financial_sql;
using fs::blob;
using fs::corrupt;
using fs::integer;
using fs::reference;
using D = FinancialDisposition;
using Life = core::TimerLifecycle;
using TD = RefundTimerDisposition;
using Policy = core::RefundDeficitPolicy;
using Number = std::int64_t;
constexpr Number maximum = std::numeric_limits<Number>::max();
constexpr auto refund_format = "financial-refund-v1";
constexpr auto key_filter = "operation_scope=? AND operation_ref=?";
constexpr auto basis_filter =
    "original_scope=? AND original_operation=? AND original_ordinal=? AND kind=?";

FinancialWorkReply refused(D disposition) { return {disposition, std::monostate{}}; }
bool same(const ApplicationOperationKey &a, const ApplicationOperationKey &b) {
    return a.operation_scope == b.operation_scope && a.operation_ref == b.operation_ref;
}
bool same(const ApplicationHoldVersionLocator &a, const ApplicationHoldVersionLocator &b) {
    return a.hold_ref == b.hold_ref && a.version_ref == b.version_ref;
}
bool same(const std::vector<ApplicationHoldVersionLocator> &a,
          const std::vector<ApplicationHoldVersionLocator> &b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(),
                      [](const auto &x, const auto &y) { return same(x, y); });
}
bool same_plan(const ApplicationPlanKey &a, const ApplicationPlanKey &b) {
    return a.plan_ref == b.plan_ref && a.revision == b.revision;
}
bool same_timer(const core::TimerState &a, const core::TimerState &b) {
    return std::tie(a.lifecycle, a.phase, a.main_remaining_ms, a.grace_remaining_ms,
                    a.awaiting_confirmation) == std::tie(b.lifecycle, b.phase, b.main_remaining_ms,
                                                         b.grace_remaining_ms,
                                                         b.awaiting_confirmation);
}
bool valid_ref(const std::string &s) { return milestone_sql::bounded_ref(s); }

std::optional<SqlRow> one(const SqliteWorkSession &s, const QString &query,
                          QList<QVariant> values) {
    auto rows = s.execute({query, std::move(values)});
    if (rows.size() > 1)
        corrupt();
    if (rows.empty())
        return {};
    return std::move(rows.front());
}

QByteArray document(const QVariant &cell) {
    if (cell.metaType().id() != QMetaType::QByteArray || cell.toByteArray().isEmpty() ||
        cell.toByteArray().size() > fs::document_limit)
        corrupt();
    return cell.toByteArray();
}

std::string generated_ref(const ApplicationOperationKey &key, const std::string &purpose) {
    fs::Document doc;
    doc.operation(key);
    return purpose + "-" +
           QCryptographicHash::hash(doc.finish(), QCryptographicHash::Sha256).toHex().toStdString();
}
QList<QVariant> basis_values(const ApplicationOriginalComponent &basis) {
    auto values = fs::operation_values(basis.operation);
    values.append(qint64(basis.request_ordinal));
    values.append(int(basis.component_kind));
    return values;
}
ApplicationOperationKey row_key(const SqlRow &row, qsizetype start = 0) {
    return {reference(row[start]), reference(row[start + 1])};
}

void require_catalog(const SqliteWorkSession &session) {
    if (!fs::selected(session))
        corrupt();
}
void write_correlation(fs::Document &doc, const VerifiedRefundCorrelation &c) {
    doc.payment(c.payment);
    doc.link(c.refund);
    doc.string(c.evidence_ref);
    doc.unsigned_number(c.contribution.value);
    doc.operation(c.original);
    doc.plan(c.original_plan);
}
VerifiedRefundCorrelation read_correlation(fs::Document &doc) {
    return {doc.payment(),   doc.link(), doc.string(), {doc.unsigned_number(0)},
            doc.operation(), doc.plan()};
}
QByteArray refund_identity(const FinancialRefundCommand &command) {
    fs::Document doc;
    write_correlation(doc, *command.correlation);
    doc.number(command.expected_payment_position);
    doc.principal(command.principal);
    return doc.finish();
}
QByteArray time_identity(const FinancialTimerCommand &c) {
    fs::Document doc;
    doc.payment(c.payment);
    doc.locator(c.origin);
    doc.range(qint64(c.expected_active.size()), fs::collection_limit);
    for (const auto &locator : c.expected_active)
        doc.locator(locator);
    doc.string(c.expected_state_token);
    doc.number(c.override_policy ? int(*c.override_policy) : -1);
    doc.principal(c.principal);
    doc.audit(c.audit);
    return doc.finish();
}
FinancialTimerCommand read_timer_command(const ApplicationOperationKey &key,
                                         const QByteArray &data) {
    fs::Document doc(data);
    FinancialTimerCommand c{};
    c.operation = key;
    c.payment = doc.payment();
    c.origin = doc.locator();
    auto count = doc.range(0, fs::collection_limit);
    for (qint64 i = 0; i < count; ++i)
        c.expected_active.push_back(doc.locator());
    c.expected_state_token = doc.string();
    const auto policy = doc.number();
    if (policy != -1) {
        if (policy < 0 || policy > int(Policy::CarryTimeDebt))
            corrupt();
        c.override_policy = Policy(policy);
    }
    c.principal = doc.principal();
    c.audit = doc.audit();
    doc.finish();
    return c;
}

QByteArray refund_facts(const FinancialEntitlement &p,
                        const std::optional<RefundTimerOutcome> &timer,
                        const std::optional<FinancialTimerHoldReason> &hold,
                        const std::vector<MilestoneRefundObservation> &observations) {
    fs::Document doc;
    doc.exact(p.previous_money);
    doc.exact(p.accepted_money);
    doc.exact(p.fraction);
    doc.range(p.full, 1);
    doc.range(p.time.has_value(), 1);
    if (p.time) {
        doc.time(*p.time);
        doc.timer(timer.value());
    }
    doc.number(hold ? int(*hold) : -1);
    doc.range(qint64(observations.size()), fs::collection_limit);
    for (const auto &m : observations) {
        doc.string(m.occurrence.ref);
        doc.string(m.refund_ref);
        doc.number(m.score_before_micro_points);
        doc.number(m.score_after_micro_points);
        doc.number(m.current_target_micro_points);
        doc.range(int(m.policy), 2);
    }
    return doc.finish();
}
bool same_entitlement(const FinancialEntitlement &a, const FinancialEntitlement &b) {
    fs::Document x, y;
    const auto encode = [](fs::Document &doc, const FinancialEntitlement &p) {
        doc.bytes(refund_facts(p, RefundTimerOutcome{}, {}, {}));
        doc.range(qint64(p.components.size()), fs::collection_limit);
        for (const auto &c : p.components) {
            doc.number(c.request_ordinal);
            doc.operation(c.original.operation);
            doc.number(c.original.request_ordinal);
            doc.number(int(c.original.component_kind));
            doc.number(c.expected_prior_position);
            doc.number(c.expected_prior_cumulative);
            doc.number(c.cumulative);
            doc.number(c.requested_increment);
            doc.exact(c.cumulative_fraction);
            doc.exact(c.exact_entitlement);
            doc.exact(c.rounding_residual);
            doc.range(c.full_refund, 1);
        }
        return doc.finish();
    };
    return encode(x, a) == encode(y, b);
}

bool match_refund(const FinancialRefundRecord &old, const FinancialRefundCommand &candidate,
                  const FinancialEntitlement &entitlement) {
    const auto &c = old.command;
    return c.receipt == candidate.receipt &&
           same_plan(c.financial_plan, candidate.financial_plan) &&
           c.expected_state_token == candidate.expected_state_token &&
           c.expected_contribution == candidate.expected_contribution &&
           bundle_detail::same_envelope(c.audit.envelope, candidate.audit.envelope) &&
           c.audit.actor_ref == candidate.audit.actor_ref &&
           c.audit.decision_ref == candidate.audit.decision_ref &&
           c.audit.reason == candidate.audit.reason &&
           refund_identity(c) == refund_identity(candidate) &&
           same_entitlement(old.entitlement, entitlement);
}

RefundTimerActual unapplied(const GroupedRefundEntitlement &time) {
    return {0, 0, 0, -time.main_request, time.restoration_request};
}

bool validate_timer_effect(const GroupedRefundEntitlement &time, const RefundTimerOutcome &outcome,
                           const core::TimerState &observed, Policy policy) {
    if (time.main_request > 0 || time.main_request == std::numeric_limits<Number>::min() ||
        time.restoration_request < 0)
        return false;
    if (outcome.disposition != TD::applied)
        return outcome.actual == unapplied(time) &&
               (outcome.disposition == TD::finished) == (observed.lifecycle == Life::Finished);
    if (observed.lifecycle != Life::Active)
        return false;
    const auto removed = std::min(observed.main_remaining_ms, -time.main_request);
    const auto deficit = -time.main_request - removed;
    if (deficit && policy == Policy::RequireReview)
        return false;
    return outcome.actual == RefundTimerActual{removed, time.restoration_request,
                                               policy == Policy::CarryTimeDebt ? deficit : 0,
                                               policy == Policy::StopAtZero ? deficit : 0, 0};
}

std::optional<FinancialEffectObservation> observe(const SqliteWorkSession &s,
                                                  FinancialRefundEffectPort &port,
                                                  const ApplicationStateSnapshot &head) {
    auto o = port.observe(s, head);
    const auto &clock = o.clock;
    if (!valid_ref(clock.clock_lineage_ref) || !valid_ref(clock.observation_ref) ||
        !valid_ref(clock.policy_revision_ref) ||
        clock.configuration_ref != head.configuration_ref ||
        clock.checked_head_token != head.token || clock.segments.empty() ||
        clock.segments.size() > size_t(fs::collection_limit) ||
        clock.observed_utc_ms != clock.segments.back().utc_now_ms ||
        !bundle_detail::enum_in(o.deficit_policy, 2) ||
        !bundle_detail::enum_in(o.config.end_mode, 2) ||
        !bundle_detail::enum_in(o.config.grace_reuse, 1) || o.config.grace_duration_ms < 0)
        return {};
    auto timer = head.timer;
    std::vector<core::TimerTransition> transitions;
    for (const auto &step : clock.segments) {
        const bool hard_end =
            o.config.hard_end_utc_ms && step.utc_now_ms >= *o.config.hard_end_utc_ms;
        // A zero-elapsed observation is stationary unless hard end is due.
        // Validate the reported transitions only after replaying the inputs.
        if (step.elapsed_ms == 0 && !hard_end)
            continue;
        auto advance = core::advance_timer(timer, o.config, step);
        timer = advance.state;
        transitions.insert(transitions.end(), advance.transitions.begin(),
                           advance.transitions.end());
    }
    if (!same_timer(timer, o.observed_timer) || transitions != o.transitions)
        return {};
    return o;
}

std::optional<Number> checked_units(const core::Exact &value) {
    if (value < 0 || value > maximum)
        return {};
    return value.convert_to<Number>();
}
void apply_timer(ApplicationStateSnapshot &state, const RefundTimerActual &actual) {
    const auto debt =
        checked_units(core::Exact(state.time_debt_ms) + actual.restored + actual.deficit_debt);
    if (!debt)
        throw std::overflow_error("Refund time debt exceeds signed 64-bit range");
    state.timer.main_remaining_ms -= actual.main_removed;
    state.time_debt_ms = *debt;
}
ApplicationStateSnapshot observed_state(const ApplicationStateSnapshot &head,
                                        const FinancialEffectObservation &observation,
                                        const ApplicationOperationKey &key) {
    auto advanced = head;
    advanced.timer = observation.observed_timer;
    if (!same_timer(head.timer, advanced.timer) || !observation.transitions.empty())
        advanced.token = generated_ref(key, "financial-observation");
    return advanced;
}

FinancialEntitlement decode_facts(const QByteArray &data, const RecordedApplicationBundle &app,
                                  std::optional<RefundTimerOutcome> &timer,
                                  std::optional<FinancialTimerHoldReason> &hold,
                                  std::vector<MilestoneRefundObservation> &milestones) {
    fs::Document doc(data);
    FinancialEntitlement p{};
    p.previous_money = doc.exact();
    p.accepted_money = doc.exact();
    p.fraction = doc.exact();
    p.full = doc.range(0, 1) != 0;
    if (doc.range(0, 1)) {
        p.time = doc.time();
        timer = doc.timer();
    }
    const auto reason = doc.number();
    if (reason != -1) {
        if (reason < 0 || reason > int(FinancialTimerHoldReason::prior_request))
            corrupt();
        hold = FinancialTimerHoldReason(reason);
    }
    const auto count = doc.range(0, fs::collection_limit);
    for (qint64 i = 0; i < count; ++i)
        milestones.push_back({{doc.string()},
                              doc.string(),
                              doc.number(),
                              doc.number(),
                              doc.number(),
                              MilestoneRefundPolicy(doc.range(0, 2))});
    doc.finish();
    for (const auto &link : app.reversals)
        p.components.push_back({link.request_ordinal, link.original, link.basis_position - 1,
                                link.previous_cumulative, link.cumulative, link.requested_increment,
                                link.cumulative_fraction, link.exact_entitlement,
                                link.rounding_residual, link.full_refund});
    return p;
}

bool time_matches(const GroupedRefundEntitlement &p,
                  const core::RefundTimeEntitlement &calculated) {
    return p.cumulative == RefundTimeAllocation{calculated.cumulative.main_ms,
                                                calculated.cumulative.debt_repayment_ms} &&
           p.exact_cumulative == calculated.total.exact_entitlement &&
           p.residual == calculated.total.rounding_residual &&
           p.main_request == calculated.requested_main_adjustment_ms &&
           p.restoration_request == calculated.requested_debt_restoration_ms;
}
bool component_matches(const ApplicationReversalLink &link, const core::RefundEntitlement &calc) {
    return link.cumulative == calc.cumulative_units &&
           link.requested_increment == calc.requested_increment &&
           link.cumulative_fraction == calc.cumulative_fraction &&
           link.exact_entitlement == calc.exact_entitlement &&
           link.rounding_residual == calc.rounding_residual && link.full_refund == calc.full_refund;
}
bool money_matches(const FinancialEntitlement &p, const core::RefundAmounts &amounts) {
    const core::Exact accepted = amounts.previously_accepted + amounts.incoming;
    return p.previous_money == amounts.previously_accepted && p.accepted_money == accepted &&
           p.fraction == accepted / amounts.original && p.full == (accepted == amounts.original);
}

std::optional<core::RefundAmounts> money(const core::ReceiptRecord &support,
                                         const core::ReceiptRecord &refund,
                                         const FinancialPaymentKey &payment,
                                         const core::Exact &previous) {
    const auto &a = support.original_facts;
    const auto &b = refund.original_facts;
    if (support.id != payment.support_receipt || support.id == refund.id ||
        support.identity.origin != payment.origin || refund.identity.origin != payment.origin ||
        a.payment_reference != payment.payment ||
        (b.payment_reference && b.payment_reference != payment.payment) || !a.money || !b.money ||
        a.money->currency.empty() || a.money->currency != b.money->currency)
        return {};
    try {
        core::RefundAmounts amounts{core::parse_decimal(a.money->original_decimal).value(),
                                    previous,
                                    core::parse_decimal(b.money->original_decimal).value()};
        if (amounts.original > 0 && amounts.incoming > 0 && previous >= 0)
            return amounts;
    } catch (const std::invalid_argument &) {
    } catch (const std::length_error &) {
    }
    return {};
}

const ApplicationEffect *find_effect(const std::vector<ApplicationEffect> &effects,
                                     Number ordinal) {
    if (ordinal < 1 || std::uintmax_t(ordinal) > effects.size())
        return nullptr;
    const auto &effect = effects[size_t(ordinal - 1)];
    return effect.request_ordinal == ordinal ? &effect : nullptr;
}
std::optional<core::RefundTimeCredit> time_credit(const ApplicationEffect *effect) {
    if (!effect || effect->arithmetic_role != AR::time_addition || effect->components.size() != 3 ||
        effect->components[0].units < 0 || effect->components[1].units > 0 ||
        effect->components[1].units == std::numeric_limits<Number>::min())
        return {};
    return core::RefundTimeCredit{effect->components[0].units, -effect->components[1].units};
}

// Reconcile the selected occurrence through its immutable membership and the
// shipped delivery decoder. Jobs may have legitimately advanced since acceptance.
void verify_milestone_jobs(const SqliteWorkSession &s, const MilestoneOccurrenceKey &occurrence) {
    const auto key = blob(occurrence.ref);
    auto group = one(s,
                     "SELECT operation_scope,operation_ref FROM main.subathon_milestone_groups "
                     "WHERE occurrence_ref=?",
                     {key});
    if (!group)
        corrupt();
    const auto origin = row_key(*group);
    const auto rows = s.execute(
        {"SELECT action_ordinal,kind,action_ref,version_ref,operation_scope,operation_ref,job_ref "
         "FROM main.subathon_milestone_actions WHERE occurrence_ref=? ORDER BY action_ordinal "
         "LIMIT ?",
         {key, fs::collection_limit + 1}});
    if (rows.size() > fs::collection_limit)
        throw SqliteBundleStorageFailure(Reason::result_limit);
    qint64 position = 0;
    const SqliteDeliveryJobStore jobs(s);
    for (const auto &row : rows) {
        if (integer(row[0]) != ++position)
            corrupt();
        const auto kind = integer(row[1]);
        const auto action = reference(row[2]);
        const auto version = reference(row[3]);
        if (kind == int(MilestoneActionKind::automation)) {
            const auto operation = row_key(row, 4);
            const auto job = jobs.lookup({operation, reference(row[6])});
            if (!same(operation, origin) || !job || job->capture.action_ref != action ||
                job->capture.action_version_ref != version)
                corrupt();
        } else if (kind != int(MilestoneActionKind::bonus) || !row[4].isNull() ||
                   !row[5].isNull() || !row[6].isNull()) {
            corrupt();
        }
    }
}

std::optional<FinancialRefundRecord> read_refund(const SqliteWorkSession &s,
                                                 const ApplicationOperationKey &key) {
    auto route = one(s,
                     "SELECT payment,position,receipt_id,economic,subject,time_ordinal,restored,"
                     "hold_ref,document FROM main.subathon_financial_refunds WHERE " +
                         QString(key_filter),
                     fs::operation_values(key));
    Access access(s, {});
    if (!route) {
        // A branded bundle without its journal cannot prove absence of the unit.
        auto operation = one(s,
                             "SELECT choices_format_ref FROM main.subathon_app_operations WHERE " +
                                 QString(key_filter),
                             fs::operation_values(key));
        if (operation && reference(operation->front()) == refund_format)
            corrupt();
        return {};
    }
    auto app = access.lookup(key);
    if (!app || app->intent.kind != ApplicationOperationKind::reversal ||
        app->intent.choices.format_ref != refund_format || !app->intent.contribution_id ||
        !app->intent.expected_contribution_state)
        corrupt();

    fs::Document intent(app->intent.choices.document);
    const auto c = read_correlation(intent);
    const auto previous_position = intent.number();
    const auto principal = intent.principal();
    intent.finish();
    const auto position = integer((*route)[1]);
    const auto receipt = app->intent.receipt_id;
    if (previous_position < 0 || previous_position == maximum ||
        position != previous_position + 1 || document((*route)[0]) != fs::payment_key(c.payment) ||
        (*route)[2].toByteArray() != encode_receipt_uint64(receipt.value) ||
        document((*route)[3]) != fs::economic_key(c) ||
        (*route)[4].toByteArray() != encode_plan_revision_uint64(c.contribution.value) ||
        c.contribution != *app->intent.contribution_id ||
        app->intent.reversal_ref != generated_ref(key, "financial-refund"))
        corrupt();

    const auto source = access.lookup(c.original);
    auto original_plan = access.plan(c.original_plan);
    auto financial_plan = access.plan(app->intent.plan);
    auto refund = read_receipt(s, receipt);
    auto support = read_receipt(s, c.payment.support_receipt);
    if (!source || source->intent.kind != ApplicationOperationKind::original ||
        source->intent.contribution_id != c.contribution ||
        source->intent.receipt_id != c.payment.support_receipt ||
        !same_plan(source->intent.plan, c.original_plan) || !original_plan || !financial_plan ||
        !refund || !support)
        corrupt();
    const auto &captured = financial_plan->selected;
    const auto &original_event = original_plan->selected.captured.event;
    const auto &financial_event = captured.captured.event;
    if (financial_event.event_ref != generated_ref(key, "financial-event") ||
        financial_event.parent_event_ref != original_event.event_ref ||
        financial_event.root_event_ref != original_event.root_event_ref ||
        captured.rules != original_plan->selected.rules ||
        captured.modifiers != original_plan->selected.modifiers ||
        original_event.facts != support->original_facts ||
        financial_event.facts != refund->original_facts ||
        captured.captured.valuation.state != core::PlanValuationState::not_applicable)
        corrupt();

    std::optional<RefundTimerOutcome> timer;
    std::optional<FinancialTimerHoldReason> hold_reason;
    std::vector<MilestoneRefundObservation> milestones;
    auto p = decode_facts(document((*route)[8]), *app, timer, hold_reason, milestones);
    const auto amounts = money(*support, *refund, c.payment, p.previous_money);
    if (!amounts || p.accepted_money > amounts->original || !money_matches(p, *amounts) ||
        p.time.has_value() != !(*route)[5].isNull() ||
        hold_reason.has_value() != !(*route)[7].isNull() ||
        hold_reason.has_value() != (timer && timer->disposition == TD::held) ||
        integer((*route)[6]) != (timer ? timer->actual.restored : 0))
        corrupt();
    auto head =
        one(s,
            "SELECT position FROM main.subathon_financial_payment_heads "
            "WHERE payment=? AND support_receipt=?",
            {fs::payment_key(c.payment), encode_receipt_uint64(c.payment.support_receipt.value)});
    if (!head || integer(head->front()) < position)
        corrupt();
    std::optional<GroupedRefundEntitlement> previous_time;
    if (previous_position != 0) {
        auto previous = one(s,
                            "SELECT document FROM main.subathon_financial_refunds "
                            "WHERE payment=? AND position=?",
                            {fs::payment_key(c.payment), qint64(previous_position)});
        if (!previous)
            corrupt();
        fs::Document prior(document(previous->front()));
        (void)prior.exact();
        if (prior.exact() != p.previous_money)
            corrupt();
        (void)prior.exact();
        (void)prior.range(0, 1);
        if (prior.range(0, 1))
            previous_time = prior.time();
    } else if (p.previous_money != 0) {
        corrupt();
    }
    std::optional<FinancialTimerHold> hold;
    if (p.time) {
        const auto &time = *p.time;
        const auto *effect = find_effect(app->effects, time.request_ordinal);
        const auto credit =
            time_credit(find_effect(source->effects, time.original.request_ordinal));
        if (time.expected_basis_position != previous_position ||
            (previous_time
                 ? !same(previous_time->original.operation, time.original.operation) ||
                       previous_time->original.request_ordinal != time.original.request_ordinal ||
                       time.previous != previous_time->cumulative
                 : previous_position != 0 || time.previous != RefundTimeAllocation{}) ||
            !credit || !same(time.original.operation, c.original) ||
            time.original.component_kind != CK::main_delta ||
            integer((*route)[5]) != time.request_ordinal || !effect || !effect->deficit_policy ||
            !time_matches(
                time, core::calculate_refund_time_entitlement(
                          *amounts, *credit, {time.previous.main, time.previous.restoration})) ||
            !validate_timer_effect(time, *timer, app->advanced_before.timer,
                                   *effect->deficit_policy) ||
            effect->requested_units != time.main_request ||
            effect->components[0].units != -timer->actual.main_removed ||
            effect->components[1].units !=
                core::Exact(timer->actual.restored) + timer->actual.deficit_debt ||
            effect->components[2].units != -timer->actual.unapplied_main)
            corrupt();
        if (hold_reason) {
            const auto hold_ref = reference((*route)[7]);
            if (hold_ref != generated_ref(key, "financial-hold"))
                corrupt();
            hold = FinancialTimerHold{
                {hold_ref, "v1"}, c.contribution, c.payment, position, key, time, *hold_reason};
        }
    }
    for (const auto &link : p.components) {
        if (link.expected_prior_position != previous_position)
            corrupt();
        const auto *effect = find_effect(source->effects, link.original.request_ordinal);
        if (!effect || !same(link.original.operation, c.original) ||
            effect->components.size() != 1 ||
            effect->components.front().kind != link.original.component_kind ||
            !component_matches(link, core::calculate_refund_entitlement(
                                         *amounts, effect->unit, effect->components.front().units,
                                         link.expected_prior_cumulative)))
            corrupt();
    }
    const bool edits =
        !milestones.empty() && milestone_sql::catalog(s) == milestone_sql::Catalog::edits;
    const auto before_score = edits ? milestone_edit_sql::total_score(app->advanced_before) : 0;
    const auto after_score = edits ? milestone_edit_sql::total_score(app->after) : 0;
    std::set<std::string_view> seen;
    for (const auto &m : milestones) {
        const auto edit =
            edits ? milestone_edit_sql::occurrence(s, blob(m.occurrence.ref), true) : std::nullopt;
        if (edits && (m.score_before_micro_points != before_score ||
                      m.score_after_micro_points != after_score ||
                      m.refund_ref != key.operation_ref || !seen.insert(m.occurrence.ref).second ||
                      (edit && edit->edit.track.installation_ref != principal.installation_ref)))
            corrupt();
        const QString table = edit ? "main.subathon_milestone_edit_refund_decisions"
                                   : "main.subathon_milestone_refund_decisions";
        auto saved = one(s,
                         "SELECT score_before_micro_points,score_after_micro_points,"
                         "current_target_micro_points,policy FROM " +
                             table + " WHERE occurrence_ref=? AND refund_ref=?",
                         {blob(m.occurrence.ref), blob(m.refund_ref)});
        if (!saved || integer((*saved)[0]) != m.score_before_micro_points ||
            integer((*saved)[1]) != m.score_after_micro_points ||
            integer((*saved)[2]) != m.current_target_micro_points ||
            integer((*saved)[3]) != int(m.policy))
            corrupt();
        const SqliteMilestoneGroupDecisions decisions(s);
        (void)decisions.pending_disposition(m.occurrence);
        if (edits && !decisions.refund_audit(m.occurrence, m.refund_ref))
            corrupt();
        if (!edit)
            verify_milestone_jobs(s, m.occurrence);
    }
    FinancialRefundCommand command{key,
                                   receipt,
                                   c,
                                   app->intent.plan,
                                   previous_position,
                                   app->intent.expected_state_token,
                                   *app->intent.expected_contribution_state,
                                   principal,
                                   app->audit};
    return FinancialRefundRecord{std::move(command),
                                 std::move(*refund),
                                 std::move(*support),
                                 std::move(original_plan->selected),
                                 std::move(financial_plan->selected),
                                 position,
                                 std::move(p),
                                 std::move(*app),
                                 std::move(timer),
                                 std::move(hold),
                                 std::move(milestones)};
}

std::optional<FinancialTimerResolution> read_resolution(const SqliteWorkSession &s,
                                                        const ApplicationOperationKey &key) {
    auto route = one(s,
                     "SELECT hold_ref,before_token,advanced_token,after_token,document "
                     "FROM main.subathon_financial_resolutions WHERE " +
                         QString(key_filter),
                     fs::operation_values(key));
    if (!route)
        return {};
    const auto hold_ref = reference((*route)[0]);
    auto source = one(s,
                      "SELECT operation_scope,operation_ref FROM main.subathon_financial_refunds "
                      "WHERE hold_ref=?",
                      {blob(hold_ref)});
    if (!source)
        corrupt();
    auto refund = read_refund(s, row_key(*source));
    if (!refund || !refund->hold)
        corrupt();
    fs::Document data(document((*route)[4]));
    auto command = read_timer_command(key, data.bytes());
    const auto timer = data.timer();
    const auto policy = Policy(data.range(0, 2));
    auto transitions = data.transitions();
    data.finish();
    Access access(s, {});
    auto before = access.state(reference((*route)[1]));
    auto advanced = access.state(reference((*route)[2]));
    auto after = access.state(reference((*route)[3]));
    if (std::ranges::count_if(command.expected_active,
                              [&](const auto &loc) { return same(loc, command.origin); }) != 1 ||
        !same(command.origin, refund->hold->locator) || command.payment != refund->hold->payment ||
        command.expected_state_token != before.token ||
        after.token != generated_ref(key, "financial-state") ||
        !access.head_relation(before, advanced, "financial timer observation") ||
        (command.override_policy && *command.override_policy != policy) ||
        timer.disposition == TD::held ||
        (timer.disposition == TD::applied && !command.override_policy &&
         refund->hold->time.main_request < -advanced.timer.main_remaining_ms) ||
        !validate_timer_effect(refund->hold->time, timer, advanced.timer, policy))
        corrupt();
    auto actual_after = advanced;
    actual_after.token = after.token;
    apply_timer(actual_after, timer.actual);
    if (!bundle_detail::same_state(actual_after, after))
        corrupt();
    ApplicationHoldVersion closed{{hold_ref, "v2"},
                                  2,
                                  command.origin.version_ref,
                                  ApplicationHoldState::resolved,
                                  generated_ref(key, "financial-close"),
                                  {}};
    auto audit = command.audit;
    return FinancialTimerResolution{std::move(command),     std::move(*refund->hold),
                                    std::move(closed),      std::move(before),
                                    std::move(after),       timer,
                                    std::move(transitions), std::move(audit)};
}

std::vector<FinancialTimerHold> active_financial(const SqliteWorkSession &s,
                                                 const ApplicationHoldSubject &subject) {
    const auto *cid = std::get_if<core::ContributionId>(&subject.id);
    if (!cid)
        return {};
    const auto rows = s.execute(
        {"SELECT f.operation_scope,f.operation_ref FROM main.subathon_financial_refunds AS f "
         "WHERE f.subject=? AND f.hold_ref IS NOT NULL AND NOT EXISTS "
         "(SELECT 1 FROM main.subathon_financial_resolutions AS r WHERE r.hold_ref=f.hold_ref) "
         "ORDER BY f.hold_ref LIMIT ?",
         {encode_plan_revision_uint64(cid->value), fs::collection_limit + 1}});
    if (rows.size() > fs::collection_limit)
        throw SqliteBundleStorageFailure(Reason::result_limit);
    std::vector<FinancialTimerHold> result;
    for (const auto &row : rows) {
        const auto record = read_refund(s, row_key(row));
        if (!record || !record->hold || record->hold->subject != *cid)
            corrupt();
        result.push_back(*record->hold);
    }
    return result;
}

struct PaymentHead {
    Number position{};
    core::Exact accepted{};
};
std::optional<PaymentHead> payment_head(const SqliteWorkSession &s,
                                        const FinancialPaymentKey &payment) {
    const auto key = fs::payment_key(payment);
    const auto support = encode_receipt_uint64(payment.support_receipt.value);
    auto head =
        one(s,
            "SELECT payment,position,support_receipt FROM main.subathon_financial_payment_heads "
            "WHERE payment=? OR support_receipt=? LIMIT 1",
            {key, support});
    auto last =
        one(s,
            "SELECT operation_scope,operation_ref,position FROM main.subathon_financial_refunds "
            "WHERE payment=? ORDER BY position DESC LIMIT 1",
            {key});
    if (!head) {
        if (last)
            corrupt();
        return PaymentHead{};
    }
    if (document(head->front()) != key || (*head)[2].toByteArray() != support)
        return {};
    if (!last || integer((*last)[2]) != integer((*head)[1]))
        corrupt();
    const auto latest = read_refund(s, row_key(*last));
    if (!latest || latest->command.correlation->payment != payment ||
        latest->payment_position != integer((*head)[1]))
        corrupt();
    return PaymentHead{latest->payment_position, latest->entitlement.accepted_money};
}

std::vector<ApplicationHoldVersionLocator>
combined_active(const SqliteWorkSession &s, const ApplicationHoldSubject &subject,
                const std::vector<FinancialTimerHold> &financial) {
    auto ordinary = SqliteApplicationHoldStore(s).active(subject);
    if (ordinary.holds.size() + financial.size() > size_t(fs::collection_limit))
        throw SqliteBundleStorageFailure(Reason::result_limit);
    std::vector<ApplicationHoldVersionLocator> result;
    for (const auto &hold : ordinary.holds)
        result.push_back(hold.current_version.locator);
    for (const auto &hold : financial)
        result.push_back(hold.locator);
    std::ranges::sort(result, [](const auto &a, const auto &b) {
        return bundle_detail::raw_less(a.hold_ref, b.hold_ref);
    });
    if (std::adjacent_find(result.begin(), result.end(), [](const auto &a, const auto &b) {
            return a.hold_ref == b.hold_ref;
        }) != result.end())
        corrupt();
    return result;
}

struct BasisHead {
    Number position{};
    std::optional<ApplicationOperationKey> last;
};
BasisHead basis_head(const SqliteWorkSession &s, const ApplicationOriginalComponent &basis) {
    auto head =
        one(s,
            "SELECT position,operation_scope,operation_ref FROM main.subathon_financial_bases "
            "WHERE " +
                QString(basis_filter),
            basis_values(basis));
    if (!head)
        return {};
    const auto position = integer(head->front());
    if (position < 1)
        corrupt();
    return {position, row_key(*head, 1)};
}

bool valid_audit(const ApplicationAudit &audit) {
    return audit.actor_ref && valid_ref(*audit.actor_ref) && audit.decision_ref &&
           valid_ref(*audit.decision_ref) && audit.reason && !audit.reason->empty() &&
           audit.reason->size() <= size_t(fs::document_limit) &&
           bundle_detail::valid_envelope(audit.envelope, {});
}
bool valid_principal(const ApplicationPrincipalBinding &principal) {
    return bundle_detail::enum_in(principal.kind, 1) && valid_ref(principal.installation_ref) &&
           valid_ref(principal.principal_ref);
}
bool valid_payment(const FinancialPaymentKey &key) {
    return key.support_receipt.value && valid_ref(key.origin.provider) &&
           valid_ref(key.origin.source_account) && valid_ref(key.payment.identifier_namespace) &&
           valid_ref(key.payment.opaque_id);
}
bool valid_key(const ApplicationOperationKey &key) {
    return valid_ref(key.operation_scope) && valid_ref(key.operation_ref);
}
bool valid_command(const FinancialRefundCommand &cmd) {
    if (!cmd.correlation)
        return false;
    const auto &c = *cmd.correlation;
    return valid_key(cmd.operation) && cmd.receipt.value && cmd.financial_plan.revision == 1 &&
           valid_ref(cmd.financial_plan.plan_ref) && cmd.expected_payment_position >= 0 &&
           valid_ref(cmd.expected_state_token) && valid_principal(cmd.principal) &&
           valid_audit(cmd.audit) && valid_payment(c.payment) && valid_key(c.original) &&
           c.original_plan.revision && valid_ref(c.original_plan.plan_ref) &&
           valid_ref(c.refund.identifier_namespace) && valid_ref(c.refund.opaque_id) &&
           valid_ref(c.evidence_ref);
}
bool valid_command(const FinancialTimerCommand &cmd) {
    return valid_key(cmd.operation) && valid_payment(cmd.payment) &&
           valid_ref(cmd.origin.hold_ref) && valid_ref(cmd.origin.version_ref) &&
           valid_ref(cmd.expected_state_token) && valid_principal(cmd.principal) &&
           valid_audit(cmd.audit) && cmd.expected_active.size() <= size_t(fs::collection_limit) &&
           std::ranges::all_of(
               cmd.expected_active,
               [](const auto &v) { return valid_ref(v.hold_ref) && valid_ref(v.version_ref); }) &&
           (!cmd.override_policy || *cmd.override_policy == Policy::StopAtZero ||
            *cmd.override_policy == Policy::CarryTimeDebt);
}

template <class Work> auto guarded(const SqliteWorkSession &s, Work work) {
    try {
        return work();
    } catch (const SqliteWorkFailure &) {
        throw;
    } catch (...) {
        if (const auto error = s.cleanup(); error != WorkError::none)
            throw SqliteWorkFailure(error);
        throw;
    }
}

struct Requests {
    std::vector<core::RequestedEffect> capture;
    std::vector<ApplicationEffect> effects;
};
void add_request(Requests &out, const core::CapturedPlanRevision &original, Number ordinal,
                 Number source_ordinal, ApplicationEffect effect) {
    auto request = original.captured.requested_effects.at(size_t(source_ordinal - 1));
    request.requires_historical_valuation = false;
    request.exact_requested_units = effect.requested_units;
    request.requested_units = effect.requested_units;
    request.calculation_explanation = "Finalized financial refund increment";
    effect.request_ordinal = ordinal;
    effect.unit = request.unit;
    effect.exact_requested_units = request.exact_requested_units;
    effect.application_explanation = request.calculation_explanation;
    out.capture.push_back(std::move(request));
    out.effects.push_back(std::move(effect));
}

D validate_bases(const SqliteWorkSession &s, Access &access, const FinancialEntitlement &p,
                 const core::RefundAmounts &amounts, const VerifiedRefundCorrelation &correlation,
                 const RecordedApplicationBundle &source, Number payment_position) {
    if (p.components.size() >= size_t(fs::collection_limit))
        return D::invalid_financial_input;
    std::set<std::tuple<Number, CK>> selected;
    if (p.time) {
        const auto &t = *p.time;
        if (!same(t.original.operation, correlation.original) ||
            t.original.component_kind != CK::main_delta || t.expected_basis_position < 0)
            return D::invalid_financial_input;
        auto credit = time_credit(find_effect(source.effects, t.original.request_ordinal));
        if (!credit)
            return D::invalid_financial_input;
        if (one(s,
                "SELECT basis_position FROM main.subathon_app_reversal_heads WHERE "
                "original_scope=? AND original_operation=? AND original_request_ordinal=? "
                "AND original_component_kind=?",
                basis_values(t.original)))
            return D::basis_conflict;
        const auto head = basis_head(s, t.original);
        RefundTimeAllocation previous{};
        if (head.last) {
            const auto last = read_refund(s, *head.last);
            if (!last || !last->entitlement.time ||
                !same(last->entitlement.time->original.operation, t.original.operation) ||
                last->entitlement.time->original.request_ordinal != t.original.request_ordinal ||
                last->entitlement.time->expected_basis_position != head.position - 1)
                corrupt();
            if (last->command.correlation->payment != correlation.payment)
                return D::basis_conflict;
            previous = last->entitlement.time->cumulative;
        }
        if (t.expected_basis_position != head.position || t.previous != previous)
            return D::stale_basis;
        if (head.position != payment_position)
            return D::basis_conflict;
        if (!time_matches(t, core::calculate_refund_time_entitlement(
                                 amounts, *credit, {previous.main, previous.restoration})))
            return D::invalid_financial_input;
        selected.emplace(t.original.request_ordinal, CK::main_delta);
    }
    for (const auto &link : p.components) {
        const auto &o = link.original;
        if (!same(o.operation, correlation.original) || o.request_ordinal < 1 ||
            (o.component_kind != CK::score && o.component_kind != CK::whole_count) ||
            !selected.emplace(o.request_ordinal, o.component_kind).second ||
            link.expected_prior_position < 0 || link.expected_prior_cumulative < 0)
            return D::invalid_financial_input;
        const auto *effect = find_effect(source.effects, o.request_ordinal);
        if (!effect || effect->components.size() != 1 ||
            effect->components.front().kind != o.component_kind ||
            effect->components.front().units < 0)
            return D::invalid_financial_input;
        const auto selected_basis = access.basis(o);
        if (!selected_basis.basis || !selected_basis.basis->target ||
            selected_basis.basis->contribution_original_receipt_id !=
                correlation.payment.support_receipt)
            return D::invalid_financial_input;
        const auto &progress = selected_basis.basis->progress;
        const auto head = basis_head(s, o);
        if (head.position != progress.position ||
            (head.last && (!progress.last || !same(*head.last, progress.last->operation))))
            return D::basis_conflict;
        if (link.expected_prior_position != progress.position ||
            link.expected_prior_cumulative != progress.cumulative)
            return D::stale_basis;
        if (head.position != payment_position)
            return D::basis_conflict;
        if (!component_matches(link, core::calculate_refund_entitlement(
                                         amounts, effect->unit, effect->components.front().units,
                                         progress.cumulative)))
            return D::invalid_financial_input;
    }
    // A payment advances one fraction for every eligible actual credited occurrence.
    for (const auto &effect : source.effects) {
        if (time_credit(&effect)) {
            if (!selected.contains({effect.request_ordinal, CK::main_delta}))
                return D::invalid_financial_input;
        } else if (effect.arithmetic_role == AR::direct && effect.components.front().units >= 0 &&
                   !selected.contains({effect.request_ordinal, effect.components.front().kind})) {
            return D::invalid_financial_input;
        }
    }
    return D::applied;
}

std::optional<Requests> prepare_requests(const FinancialEntitlement &p,
                                         const std::optional<RefundTimerOutcome> &timer,
                                         Policy policy, const RecordedApplicationBundle &source,
                                         const core::CapturedPlanRevision &original,
                                         ApplicationStateSnapshot &after) {
    Requests out;
    const auto count = p.components.size() + size_t(bool(p.time));
    for (size_t i = 0; i < count; ++i) {
        const auto ordinal = static_cast<Number>(i + 1);
        if (p.time && p.time->request_ordinal == ordinal) {
            const auto &actual = timer->actual;
            apply_timer(after, actual);
            const auto debt_delta =
                checked_units(core::Exact(actual.restored) + actual.deficit_debt);
            if (!debt_delta)
                return {};
            ApplicationEffect effect{};
            effect.arithmetic_role = AR::time_reversal;
            effect.requested_units = p.time->main_request;
            effect.deficit_policy = policy;
            effect.deficit_disposition = timer->disposition == TD::held
                                             ? core::TimeReversalDisposition::RequiresReview
                                             : core::TimeReversalDisposition::Applied;
            effect.components = {{CK::main_delta, -actual.main_removed, {}},
                                 {CK::debt_delta, *debt_delta, {}},
                                 {CK::unapplied_time, -actual.unapplied_main, {}}};
            add_request(out, original, ordinal, p.time->original.request_ordinal,
                        std::move(effect));
            continue;
        }
        const auto link =
            std::ranges::find(p.components, ordinal, &ApplicationReversalLink::request_ordinal);
        if (link == p.components.end())
            return {};
        const auto &component =
            source.effects[size_t(link->original.request_ordinal - 1)].components.front();
        auto balance = std::ranges::find_if(after.balances, [&](const auto &candidate) {
            return int(candidate.kind) == int(component.kind) &&
                   candidate.target_ref == component.target_ref;
        });
        if (balance == after.balances.end())
            return {};
        const core::Exact value = core::Exact(balance->units) + link->requested_increment;
        if (value < std::numeric_limits<Number>::min() || value > maximum ||
            (component.kind == CK::whole_count && value < 0))
            return {};
        balance->units = value.convert_to<Number>();
        ApplicationEffect effect{};
        effect.requested_units = link->requested_increment;
        effect.components = {{component.kind, link->requested_increment, component.target_ref}};
        add_request(out, original, ordinal, link->original.request_ordinal, std::move(effect));
    }
    return out;
}

using milestone_edit_sql::total_score;

void advance_basis(const SqliteWorkSession &s, const ApplicationOriginalComponent &basis,
                   Number previous, const ApplicationOperationKey &current) {
    auto values = basis_values(basis);
    values.append(qint64(previous + 1));
    values.append(fs::operation_values(current));
    (void)s.execute(
        {"INSERT INTO main.subathon_financial_bases VALUES(?,?,?,?,?,?,?) "
         "ON CONFLICT(original_scope,original_operation,original_ordinal,kind) DO UPDATE "
         "SET position=excluded.position,operation_scope=excluded.operation_scope,"
         "operation_ref=excluded.operation_ref WHERE "
         "subathon_financial_bases.position=excluded.position-1",
         std::move(values)});
    const auto changed = s.execute({"SELECT changes()", {}});
    if (integer(changed.front().front()) != 1)
        corrupt();
}

FinancialWorkReply accept_refund(const SqliteWorkSession &s, const FinancialRefundCommand &command,
                                 const FinancialEntitlement &p, FinancialRefundAuthority &authority,
                                 FinancialRefundEffectPort &port) {
    require_catalog(s);
    if (!valid_command(command))
        return refused(D::invalid_financial_input);
    const auto &c = *command.correlation;
    if (!authority.allows(s, FinancialAuthorityUse::accept, command.principal, command.operation,
                          c.payment))
        return refused(D::denied_execution);
    if (auto old = read_refund(s, command.operation)) {
        if (!match_refund(*old, command, p))
            return refused(D::identity_conflict);
        if (!authority.allows(s, FinancialAuthorityUse::disclose, command.principal,
                              command.operation, c.payment))
            return refused(D::denied_disclosure);
        return {D::existing, std::move(*old)};
    }

    Access access(s, {});
    const auto identity = fs::operation_values(command.operation);
    if (one(s,
            "SELECT operation_ref FROM main.subathon_financial_refunds "
            "WHERE receipt_id=? OR economic=? LIMIT 1",
            {encode_receipt_uint64(command.receipt.value), fs::economic_key(c)}) ||
        one(s,
            "SELECT operation_ref FROM main.subathon_financial_resolutions WHERE " +
                QString(key_filter),
            identity) ||
        one(s,
            "SELECT operation_ref FROM main.subathon_app_operations WHERE (" + QString(key_filter) +
                ") OR (operation_kind=1 AND receipt_id=?) LIMIT 1",
            {identity[0], identity[1], encode_receipt_uint64(command.receipt.value)}) ||
        access.plan(command.financial_plan))
        return refused(D::identity_conflict);
    const auto head = payment_head(s, c.payment);
    if (!head)
        return refused(D::identity_conflict);
    if (command.expected_payment_position != head->position)
        return refused(D::stale_payment);
    if (head->position == maximum)
        return refused(D::refused);

    const auto support = read_receipt(s, c.payment.support_receipt);
    const auto refund = read_receipt(s, command.receipt);
    if (!support || !refund)
        return refused(D::invalid_financial_input);
    auto amounts = money(*support, *refund, c.payment, head->accepted);
    if (!amounts)
        return refused(D::invalid_financial_input);
    if (amounts->previously_accepted + amounts->incoming > amounts->original)
        return refused(D::exceeds_original);
    if (!money_matches(p, *amounts))
        return refused(D::invalid_financial_input);

    const auto input = access.inputs(c.original_plan, ApplicationSubjectKind::contribution);
    if (!input.inputs || !input.inputs->current_contribution)
        return refused(D::prerequisite);
    const auto &selected = *input.inputs;
    const auto &current = selected.current_state;
    const auto &contribution = selected.current_contribution->contribution;
    const auto &original = selected.selected_plan;
    if (current.token != command.expected_state_token ||
        contribution.state != command.expected_contribution)
        return refused(D::stale_state);
    if (current.timer.lifecycle == Life::Ready)
        return refused(D::prerequisite);
    const auto source = access.lookup(c.original);
    if (!source || source->intent.kind != ApplicationOperationKind::original ||
        source->intent.receipt_id != c.payment.support_receipt ||
        source->intent.contribution_id != c.contribution ||
        !same_plan(source->intent.plan, c.original_plan) ||
        contribution.original_receipt_id != c.payment.support_receipt ||
        contribution.id != c.contribution ||
        contribution.state.conditions.application != core::ContributionApplication::applied ||
        original.captured.event.facts.money != support->original_facts.money ||
        !contribution.state.conditions.valuation.monetary ||
        contribution.state.conditions.valuation.original_money != support->original_facts.money ||
        contribution.state.conditions.settlement != core::SettlementGate::clear ||
        contribution.state.payment_evidence != core::PaymentEvidence::confirmed)
        return refused(D::invalid_financial_input);

    if (head->position == 0 &&
        contribution.state.conditions.reversal != core::ContributionReversal::none)
        return refused(D::basis_conflict);
    const ApplicationHoldSubject subject{c.contribution};
    const auto active = active_financial(s, subject);
    if (combined_active(s, subject, active).size() != active.size() ||
        std::ranges::any_of(active, [&](const auto &h) { return h.payment != c.payment; }))
        return refused(D::active_hold);
    try {
        const auto validation = validate_bases(s, access, p, *amounts, c, *source, head->position);
        if (validation != D::applied)
            return refused(validation);
    } catch (const std::invalid_argument &) {
        return refused(D::invalid_financial_input);
    } catch (const std::overflow_error &) {
        return refused(D::refused);
    } catch (const std::length_error &) {
        return refused(D::refused);
    }

    if (!authority.allows(s, FinancialAuthorityUse::disclose, command.principal, command.operation,
                          c.payment))
        return refused(D::denied_disclosure);
    const auto observation = observe(s, port, current);
    if (!observation)
        return refused(D::prerequisite);
    const auto policy = observation->deficit_policy;
    const auto advanced = observed_state(current, *observation, command.operation);
    auto after = advanced;
    after.token = generated_ref(command.operation, "financial-state");
    std::optional<RefundTimerOutcome> timer;
    std::optional<FinancialTimerHoldReason> hold;
    if (p.time) {
        const auto &time = *p.time;
        if (advanced.timer.lifecycle == Life::Finished)
            timer = RefundTimerOutcome{TD::finished, unapplied(time)};
        else if (!active.empty() && (time.main_request || time.restoration_request))
            hold = FinancialTimerHoldReason::prior_request;
        else if (-time.main_request > advanced.timer.main_remaining_ms &&
                 policy == Policy::RequireReview)
            hold = FinancialTimerHoldReason::deficit;
        else
            timer =
                RefundTimerOutcome{TD::applied, port.timer_effect(s, time, *observation, policy)};
        if (hold)
            timer = RefundTimerOutcome{TD::held, unapplied(time)};
        if (!validate_timer_effect(time, *timer, advanced.timer, policy))
            return refused(D::invalid_financial_input);
    }

    std::optional<Requests> requests;
    Number score_before, score_after;
    try {
        requests = prepare_requests(p, timer, policy, *source, original, after);
        if (!requests)
            return refused(D::invalid_financial_input);
        score_before = total_score(advanced);
        score_after = total_score(after);
    } catch (const std::overflow_error &) {
        return refused(D::refused);
    }
    auto event = original.captured.event;
    event.event_ref = generated_ref(command.operation, "financial-event");
    event.parent_event_ref = original.captured.event.event_ref;
    event.receipt_id = refund->id;
    event.identity = refund->identity;
    event.facts = refund->original_facts;
    event.arrival = refund->original_arrival;
    core::CapturedPlanRevision capture{1,
                                       {},
                                       {std::move(event),
                                        contribution.state,
                                        {core::PlanValuationState::not_applicable, {}},
                                        std::move(requests->capture)},
                                       original.rules,
                                       original.modifiers,
                                       {}};

    const auto result_ref = generated_ref(command.operation, "financial-refund");
    PreparedApplicationBundle bundle{
        {command.operation,
         ApplicationOperationKind::reversal,
         ApplicationSubjectKind::contribution,
         command.receipt,
         c.contribution,
         command.financial_plan,
         command.expected_state_token,
         command.expected_contribution,
         {refund_format, refund_identity(command)},
         result_ref},
        advanced,
        after,
        advanced.token == current.token ? "" : "Financial refund clock observation",
        std::move(requests->effects),
        {},
        observation->transitions,
        p.components,
        {refund_format, QByteArrayLiteral("accepted")},
        command.audit};
    const auto grouped = p.time ? std::optional{bundle_detail::GroupedTimeEffect{
                                      p.time->request_ordinal, timer->actual.restored}}
                                : std::nullopt;
    try {
        bundle_detail::validate_bundle(bundle, {}, grouped);
    } catch (const SqliteBundleInputFailure &) {
        return refused(D::invalid_financial_input);
    }
    std::vector<FinancialMilestoneTarget> targets;
    if (score_after < score_before)
        targets = port.milestones(s);
    if (targets.size() > size_t(fs::collection_limit))
        return refused(D::invalid_financial_input);

    // Resolve all targets before staging so an unavailable definition can be
    // returned without retaining any part of the refund.
    std::vector<MilestoneRefundObservation> selected_milestones;
    std::set<std::string> seen;
    const bool edits =
        !targets.empty() && milestone_sql::catalog(s) == milestone_sql::Catalog::edits;
    for (const auto &target : targets) {
        if (!seen.insert(target.occurrence.ref).second)
            corrupt();
        const auto selected_target =
            edits ? milestone_edit_sql::current_target(s, target.occurrence,
                                                       command.principal.installation_ref)
                  : milestone_edit_sql::RefundTarget{target.target};
        if (std::holds_alternative<milestone_edit_sql::UnavailableRefundTarget>(selected_target))
            return refused(D::unavailable);
        if (const auto *value = std::get_if<qint64>(&selected_target))
            selected_milestones.push_back({target.occurrence, command.operation.operation_ref,
                                           score_before, score_after, *value, target.policy});
    }

    const auto staged_capture =
        SqliteApplicationPlanStore(s).retain_initial(command.financial_plan.plan_ref, capture);
    if (staged_capture != SqlitePlanInitialDisposition::inserted ||
        access.stage(bundle, true).disposition != SD::staged)
        corrupt();

    const auto position = head->position + 1;
    const auto hold_ref = hold ? generated_ref(command.operation, "financial-hold") : std::string{};
    if (hold && SqliteApplicationHoldStore(s).find(hold_ref))
        corrupt();
    std::vector<MilestoneRefundObservation> milestones;
    const SqliteMilestoneGroupDecisions decisions(s);
    for (auto &milestone : selected_milestones) {
        // An unresolved review remains separate from application holds.
        if (decisions.refund_audit(milestone.occurrence, milestone.refund_ref))
            corrupt();
        decisions.observe_refund(milestone);
        if (decisions.refund_audit(milestone.occurrence, milestone.refund_ref))
            milestones.push_back(std::move(milestone));
    }

    const auto payment = fs::payment_key(c.payment);
    (void)s.execute(
        {"INSERT INTO main.subathon_financial_refunds VALUES(?,?,?,?,?,?,?,?,?,?,?)",
         {identity[0], identity[1], payment, qint64(position),
          encode_receipt_uint64(command.receipt.value), fs::economic_key(c),
          encode_plan_revision_uint64(c.contribution.value),
          grouped ? QVariant(qint64(grouped->ordinal)) : QVariant{},
          qint64(grouped ? grouped->restored : 0), hold ? QVariant(blob(hold_ref)) : QVariant{},
          refund_facts(p, timer, hold, milestones)}});
    (void)s.execute(
        {"INSERT INTO main.subathon_financial_payment_heads VALUES(?,?,?) "
         "ON CONFLICT(payment) DO UPDATE SET position=excluded.position WHERE "
         "subathon_financial_payment_heads.position=excluded.position-1",
         {payment, encode_receipt_uint64(c.payment.support_receipt.value), qint64(position)}});
    if (integer(s.execute({"SELECT changes()", {}}).front().front()) != 1)
        corrupt();
    if (p.time)
        advance_basis(s, p.time->original, p.time->expected_basis_position, command.operation);
    for (const auto &link : p.components)
        advance_basis(s, link.original, link.expected_prior_position, command.operation);

    auto saved = read_refund(s, command.operation);
    if (!saved)
        corrupt();
    return {D::applied, std::move(*saved)};
}

FinancialWorkReply resolve_timer(const SqliteWorkSession &s, const FinancialTimerCommand &c,
                                 FinancialRefundAuthority &authority,
                                 FinancialRefundEffectPort &port) {
    require_catalog(s);
    if (!valid_command(c))
        return refused(D::invalid_financial_input);
    if (!authority.allows(s, FinancialAuthorityUse::continue_timer, c.principal, c.operation,
                          c.payment) ||
        (c.override_policy && !authority.allows(s, FinancialAuthorityUse::override_timer,
                                                c.principal, c.operation, c.payment)))
        return refused(D::denied_execution);
    if (auto old = read_resolution(s, c.operation)) {
        if (time_identity(c) != time_identity(old->command))
            return refused(D::identity_conflict);
        if (!authority.allows(s, FinancialAuthorityUse::disclose, c.principal, c.operation,
                              c.payment))
            return refused(D::denied_disclosure);
        return {D::existing, std::move(*old)};
    }
    const auto identity = fs::operation_values(c.operation);
    if (one(s,
            "SELECT operation_ref FROM main.subathon_app_operations WHERE " + QString(key_filter),
            identity))
        return refused(D::identity_conflict);

    auto origin = one(s, "SELECT subject FROM main.subathon_financial_refunds WHERE hold_ref=?",
                      {blob(c.origin.hold_ref)});
    if (!origin)
        return refused(D::stale_hold);
    const auto cid =
        core::ContributionId{decode_plan_revision_uint64(origin->front().toByteArray())};
    const ApplicationHoldSubject subject{cid};
    const auto holds = active_financial(s, subject);
    const auto at =
        std::ranges::find_if(holds, [&](const auto &h) { return same(h.locator, c.origin); });
    if (at == holds.end() || at->payment != c.payment)
        return refused(D::stale_hold);
    const auto active = combined_active(s, subject, holds);
    if (!same(active, c.expected_active))
        return refused(D::stale_hold);
    if (active.size() != holds.size() ||
        std::ranges::any_of(holds, [&](const auto &h) { return h.payment != c.payment; }))
        return refused(D::active_hold);
    if (std::ranges::any_of(
            holds, [&](const auto &h) { return h.payment_position < at->payment_position; }))
        return refused(D::out_of_order);

    Access access(s, {});
    const auto current = access.current();
    if (!current || current->token != c.expected_state_token)
        return refused(D::stale_state);
    if (current->timer.lifecycle == Life::Ready)
        return refused(D::prerequisite);
    if (!authority.allows(s, FinancialAuthorityUse::disclose, c.principal, c.operation, c.payment))
        return refused(D::denied_disclosure);
    const auto observation = observe(s, port, *current);
    if (!observation)
        return refused(D::prerequisite);
    const auto policy = c.override_policy.value_or(observation->deficit_policy);
    const auto advanced = observed_state(*current, *observation, c.operation);
    auto after = advanced;
    after.token = generated_ref(c.operation, "financial-state");
    const auto &time = at->time;
    const bool finished = advanced.timer.lifecycle == Life::Finished;
    if (!finished && !c.override_policy && time.main_request < -advanced.timer.main_remaining_ms)
        return refused(D::deficit);
    const RefundTimerOutcome outcome =
        finished
            ? RefundTimerOutcome{TD::finished, unapplied(time)}
            : RefundTimerOutcome{TD::applied, port.timer_effect(s, time, *observation, policy)};
    if (!validate_timer_effect(time, outcome, advanced.timer, policy))
        return refused(D::invalid_financial_input);
    try {
        apply_timer(after, outcome.actual);
    } catch (const std::overflow_error &) {
        return refused(D::refused);
    }

    bundle_detail::validate_state_shape(advanced, {});
    bundle_detail::validate_state_shape(after, {});
    fs::Document doc;
    doc.bytes(time_identity(c));
    doc.timer(outcome);
    doc.range(int(policy), 2);
    doc.transitions(observation->transitions);
    const auto payload = doc.finish();
    std::vector<SqlStatement> writes;
    if (advanced.token != current->token)
        access.prepare_state(writes, advanced);
    access.prepare_state(writes, after);
    writes.push_back({"INSERT INTO main.subathon_financial_resolutions VALUES(?,?,?,?,?,?,?)",
                      {identity[0], identity[1], blob(c.origin.hold_ref), blob(current->token),
                       blob(advanced.token), blob(after.token), payload}});
    writes.push_back({"UPDATE main.subathon_app_head SET token=? WHERE singleton=1 AND token=?",
                      {blob(after.token), blob(current->token)}});
    access.write(std::move(writes));
    if (integer(s.execute({"SELECT changes()", {}}).front().front()) != 1)
        corrupt();
    auto saved = read_resolution(s, c.operation);
    if (!saved)
        corrupt();
    return {D::resolved, std::move(*saved)};
}

FinancialWorkReply ordinary_resolution(const SqliteWorkSession &s,
                                       const ApplicationHoldResolutionBatch &batch) {
    require_catalog(s);
    if (!batch.application || batch.entries.empty() ||
        std::ranges::any_of(batch.entries, [](const auto &e) { return bool(e.replacement); }))
        return refused(D::refused);
    const auto financial = active_financial(s, batch.subject);
    const auto active = combined_active(s, batch.subject, financial);
    if (!same(active, batch.expected_active))
        return refused(D::stale_hold);

    auto ordinary = batch;
    std::erase_if(ordinary.expected_active, [&](const auto &locator) {
        return std::ranges::any_of(financial,
                                   [&](const auto &hold) { return same(hold.locator, locator); });
    });
    if (ordinary.entries.size() != ordinary.expected_active.size() ||
        !std::equal(ordinary.entries.begin(), ordinary.entries.end(),
                    ordinary.expected_active.begin(), [](const auto &entry, const auto &locator) {
                        return entry.hold_ref == locator.hold_ref &&
                               entry.expected_version_ref == locator.version_ref;
                    }))
        return refused(D::refused);
    const auto result = stage_checked_ordinary_resolution(s, ordinary);
    if (result.outcome)
        return {D::ordinary_resolved, *result.outcome};
    if (result.disposition == SqliteHoldMutationDisposition::stale_active_set ||
        result.disposition == SqliteHoldMutationDisposition::stale_version)
        return refused(D::stale_hold);
    return refused(D::refused);
}
} // namespace

std::optional<FinancialPaymentHistory>
SqliteFinancialRefundStore::history(const FinancialPaymentKey &payment,
                                    qsizetype max_entries) const {
    return guarded(session_, [&]() -> std::optional<FinancialPaymentHistory> {
        require_catalog(session_);
        if (!valid_payment(payment) || max_entries < 1 ||
            max_entries == std::numeric_limits<qsizetype>::max())
            throw std::invalid_argument("Invalid financial history selector");
        const auto head = payment_head(session_, payment);
        if (!head || head->position == 0)
            return {};
        const auto key = fs::payment_key(payment);
        const auto count = session_.execute(
            {"SELECT count(*) FROM main.subathon_financial_resolutions AS r "
             "JOIN main.subathon_financial_refunds AS f ON f.hold_ref=r.hold_ref WHERE f.payment=?",
             {key}});
        const auto resolutions = integer(count.front().front());
        if (resolutions < 0 || head->position > max_entries ||
            resolutions > max_entries - head->position)
            throw SqliteBundleStorageFailure(Reason::result_limit);
        const auto rows = session_.execute(
            {"SELECT operation_scope,operation_ref FROM main.subathon_financial_refunds "
             "WHERE payment=? ORDER BY position LIMIT ?",
             {key, qint64(max_entries + 1)}});
        if (rows.size() != head->position)
            corrupt();
        FinancialPaymentHistory result{head->position, head->accepted, {}, {}};
        core::Exact prior = 0;
        Number position = 0;
        for (const auto &row : rows) {
            auto refund = read_refund(session_, row_key(row));
            if (!refund || refund->payment_position != ++position ||
                refund->command.correlation->payment != payment ||
                refund->entitlement.previous_money != prior)
                corrupt();
            prior = refund->entitlement.accepted_money;
            result.refunds.push_back(std::move(*refund));
        }
        if (prior != head->accepted)
            corrupt();
        const auto closures =
            session_.execute({"SELECT r.operation_scope,r.operation_ref FROM "
                              "main.subathon_financial_resolutions AS r "
                              "JOIN main.subathon_financial_refunds AS f ON f.hold_ref=r.hold_ref "
                              "WHERE f.payment=? ORDER BY f.position LIMIT ?",
                              {key, qint64(max_entries + 1)}});
        if (closures.size() != resolutions)
            corrupt();
        for (const auto &row : closures) {
            auto closure = read_resolution(session_, row_key(row));
            if (!closure || closure->command.payment != payment)
                corrupt();
            result.resolutions.push_back(std::move(*closure));
        }
        return result;
    });
}

std::optional<FinancialRefundRecord>
SqliteFinancialRefundStore::lookup(const ApplicationOperationKey &key) const {
    return guarded(session_, [&] {
        require_catalog(session_);
        return read_refund(session_, key);
    });
}

std::optional<FinancialTimerResolution>
SqliteFinancialRefundStore::lookup_resolution(const ApplicationOperationKey &key) const {
    return guarded(session_, [&] {
        require_catalog(session_);
        return read_resolution(session_, key);
    });
}

std::vector<FinancialTimerHold>
SqliteFinancialRefundStore::active(const ApplicationHoldSubject &subject) const {
    return guarded(session_, [&] {
        require_catalog(session_);
        return active_financial(session_, subject);
    });
}

FinancialWorkReply stage_financial_refund(const SqliteWorkSession &session,
                                          const FinancialRefundCommand &command,
                                          const FinancialEntitlement &candidate,
                                          FinancialRefundAuthority &authority,
                                          FinancialRefundEffectPort &effects) {
    return guarded(session,
                   [&] { return accept_refund(session, command, candidate, authority, effects); });
}

FinancialWorkReply stage_financial_timer_resolution(const SqliteWorkSession &session,
                                                    const FinancialTimerCommand &command,
                                                    FinancialRefundAuthority &authority,
                                                    FinancialRefundEffectPort &effects) {
    return guarded(session, [&] { return resolve_timer(session, command, authority, effects); });
}

FinancialWorkReply
stage_financial_aware_linked_resolution(const SqliteWorkSession &session,
                                        const ApplicationHoldResolutionBatch &batch) {
    return guarded(session, [&] { return ordinary_resolution(session, batch); });
}

FinancialWorkReply
stage_financial_aware_nonfinancial_reversal(const SqliteWorkSession &session,
                                            const PreparedApplicationBundle &bundle) {
    return guarded(session, [&] {
        require_catalog(session);
        if (bundle.intent.kind != ApplicationOperationKind::reversal)
            return refused(D::refused);
        Access access(session, {});
        for (const auto &link : bundle.reversals)
            if (access.financial_basis(link.original))
                return refused(D::basis_conflict);
        const SqliteApplicationBundleStore store(session);
        const auto result = store.stage(bundle);
        if (result.disposition == SD::already_present && result.existing)
            return FinancialWorkReply{D::existing, *result.existing};
        if (result.disposition == SD::staged) {
            auto saved = store.lookup(bundle.intent.operation);
            if (!saved)
                corrupt();
            return FinancialWorkReply{D::applied, std::move(*saved)};
        }
        return refused(result.disposition == SD::reversal_conflict ? D::basis_conflict
                                                                   : D::refused);
    });
}

std::optional<FinancialWorkReply> acknowledged_financial_reply(const SqliteWorkResult &result) {
    if (result.transaction != TransactionOutcome::committed || result.error != WorkError::none ||
        !result.value || result.value->metaType() != QMetaType::fromType<FinancialWorkReply>())
        return {};
    auto reply = result.value->value<FinancialWorkReply>();
    switch (reply.disposition) {
    case D::applied:
    case D::existing:
        if (!(std::holds_alternative<FinancialRefundRecord>(reply.record) ||
              std::holds_alternative<RecordedApplicationBundle>(reply.record) ||
              (reply.disposition == D::existing &&
               std::holds_alternative<FinancialTimerResolution>(reply.record))))
            return {};
        break;
    case D::resolved:
        if (!std::holds_alternative<FinancialTimerResolution>(reply.record))
            return {};
        break;
    case D::ordinary_resolved:
        if (!std::holds_alternative<ApplicationHoldMutationOutcome>(reply.record))
            return {};
        break;
    default:
        if (!std::holds_alternative<std::monostate>(reply.record))
            return {};
    }
    return reply;
}
} // namespace subathon::server
