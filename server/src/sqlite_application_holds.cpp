// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_application_bundle_access.hpp"
#include "sqlite_application_hold_values.hpp"
#include <subathon/server/sqlite_application_holds.hpp>

namespace subathon::server {
namespace {
using HR = SqliteHoldStorageReason;
using HD = SqliteHoldMutationDisposition;
using HS = ApplicationHoldState;
using HM = ApplicationHoldMutationKind;
using HC = ApplicationHoldCauseKind;
using HV = ApplicationHoldVersion;
const QStringList hold_mutations_columns{"mutation_ref",
                                         "mutation_kind",
                                         "subject_kind",
                                         "subject_id",
                                         "expected_count",
                                         "entry_count",
                                         "actor_ref",
                                         "decision_ref",
                                         "reason",
                                         "options_format_ref",
                                         "options_document",
                                         "application_link_kind",
                                         "application_scope",
                                         "application_ref",
                                         "application_receipt_id",
                                         "application_plan_ref",
                                         "application_plan_revision",
                                         "held_hold_ref",
                                         "held_version_ref",
                                         "held_request_ordinal",
                                         "held_component_kind",
                                         "remaining_units"};
const QStringList hold_causes_columns{"hold_ref",
                                      "subject_kind",
                                      "subject_id",
                                      "contribution_id",
                                      "source_receipt_id",
                                      "event_ref",
                                      "parent_event_ref",
                                      "root_event_ref",
                                      "configuration_ref",
                                      "arrival_context_ref",
                                      "command_scope",
                                      "command_ref",
                                      "command_format_ref",
                                      "command_document",
                                      "cause_kind",
                                      "supersedes_hold_ref",
                                      "first_detection_ref",
                                      "plan_evidence_kind",
                                      "plan_ref",
                                      "plan_revision",
                                      "extent",
                                      "stopped_for",
                                      "failed_capture_format_ref",
                                      "failed_capture_document",
                                      "occurrence_count",
                                      "failure_stage",
                                      "failing_ordinal",
                                      "failing_target_ref",
                                      "failure_numerator",
                                      "failure_denominator",
                                      "failure_format_ref",
                                      "failure_document",
                                      "timer_request_ordinal",
                                      "timer_component_kind",
                                      "timer_request_units",
                                      "deficit_policy",
                                      "deficit_disposition",
                                      "unapplied_units",
                                      "composition",
                                      "composition_format_ref",
                                      "composition_document",
                                      "missing_policy_reason",
                                      "associated_scope",
                                      "associated_ref",
                                      "associated_request_ordinal"};
const QStringList hold_cause_observations_columns{
    "hold_ref",  "persisted_state_token", "format_ref",
    "document",  "policy_revision_ref",   "permission_revision_ref",
    "actor_ref", "decision_ref",          "observed_utc_ms",
    "elapsed_ms"};
const QStringList hold_occurrences_columns{
    "hold_ref",   "ordinal", "action_ref",          "action_version_ref",    "rule_ref",
    "target_ref", "unit",    "attempted_numerator", "attempted_denominator", "finalized_units",
    "explanation"};
const QStringList hold_detections_columns{"mutation_ref",         "hold_ref",
                                          "expected_version_ref", "proposed_version_ref",
                                          "audit_format_ref",     "audit_document"};
const QStringList hold_detection_observations_columns{
    "mutation_ref", "persisted_state_token", "format_ref",
    "document",     "policy_revision_ref",   "permission_revision_ref",
    "actor_ref",    "decision_ref",          "observed_utc_ms",
    "elapsed_ms"};
const QStringList hold_versions_columns{"hold_ref",
                                        "version_ref",
                                        "position",
                                        "parent_version_ref",
                                        "state",
                                        "mutation_ref",
                                        "replacement_hold_ref",
                                        "replacement_detection_ref"};
const QStringList hold_heads_columns{"hold_ref", "version_ref", "version_count"};
const QStringList hold_resolution_expected_columns{"mutation_ref", "hold_ref", "version_ref"};
const QStringList hold_resolution_entries_columns{
    "mutation_ref",         "hold_ref",
    "expected_version_ref", "proposed_version_ref",
    "decision_format_ref",  "decision_document",
    "replacement_hold_ref", "replacement_detection_ref"};
// All strict reads and writes use this one borrowed revocable writer capability.
// Metadata is streamed; full event ownership never recurses through current views.
class HoldAccess : public Access {
  public:
    SqliteHoldLimits holds;
    explicit HoldAccess(SqliteWorkSession s, SqliteHoldLimits l)
        : Access(std::move(s), l.references), holds(l) {}
    [[noreturn]] void fail(HR r) {
        session_.rollback();
        wrote_ = false;
        throw SqliteHoldStorageFailure(r);
    }
    static HR map(SqliteBundleStorageReason r) {
        switch (r) {
        case Reason::busy:
            return HR::busy;
        case Reason::sql_failure:
            return HR::sql_failure;
        case Reason::resource_limit:
            return HR::resource_limit;
        case Reason::result_limit:
            return HR::result_limit;
        default:
            return HR::invalid_stored_data;
        }
    }
    template <class F> auto guarded(F f) -> decltype(f()) {
        try {
            return f();
        } catch (const SqliteHoldStorageFailure &) {
            throw;
        } catch (const SqliteBundleStorageFailure &e) {
            throw SqliteHoldStorageFailure(map(e.reason()));
        } catch (const SqliteWorkFailure &) {
            throw;
        } catch (...) {
            if (wrote_)
                session_.rollback();
            throw;
        }
    }
    template <class F> void stored_shape(F f) {
        try {
            f();
        } catch (const SqliteHoldInputFailure &e) {
            if (e.error() == SqliteHoldInputError::exact_component_limit ||
                e.error() == SqliteHoldInputError::collection_limit ||
                e.error() == SqliteHoldInputError::owned_byte_limit)
                fail(HR::result_limit);
            fail(HR::invalid_stored_data);
        }
    }
    template <class T> void returned(const T &v) {
        try {
            if (hold_detail::items(v) > size_t(holds.result_items) ||
                hold_detail::bytes(v) > size_t(holds.owned_bytes))
                fail(HR::result_limit);
        } catch (const std::length_error &) {
            fail(HR::result_limit);
        }
    }
    size_t add(size_t a, size_t b) {
        if (b > std::numeric_limits<size_t>::max() - a)
            fail(HR::result_limit);
        return a + b;
    }
    SqlRow required(const QString &t, const QStringList &c, const QString &w, Values b) {
        auto r = one(t, c, w, std::move(b));
        if (!r)
            fail(HR::invalid_stored_data);
        return std::move(*r);
    }
    std::string ref(const SqlRow &r, qsizetype i) {
        return string(r, i, limits_.reference_bytes, true);
    }
    std::optional<std::string> optref(const SqlRow &r, qsizetype i) {
        return null(r, i) ? std::nullopt : std::optional{ref(r, i)};
    }
    std::optional<std::int64_t> optint(const SqlRow &r, qsizetype i) {
        return null(r, i) ? std::nullopt : std::optional<std::int64_t>{integer(r, i)};
    }
    template <class E> std::optional<E> optenum(const SqlRow &r, qsizetype i, qint64 max) {
        return null(r, i) ? std::nullopt : std::optional{static_cast<E>(enumeration(r, i, max))};
    }
    std::optional<ApplicationEnvelope> optenv(const SqlRow &r, qsizetype i) {
        if (null(r, i) != null(r, i + 1))
            fail(HR::invalid_stored_data);
        return null(r, i) ? std::nullopt : std::optional{envelope(r, i)};
    }
    void all_null(const SqlRow &r, qsizetype first, qsizetype last) {
        for (auto i = first; i <= last; ++i)
            if (!null(r, i))
                fail(HR::invalid_stored_data);
    }
    std::optional<core::Exact> optional_exact(const SqlRow &r, qsizetype i) {
        const bool a = null(r, i), b = null(r, i + 1);
        if (a != b)
            fail(HR::invalid_stored_data);
        if (a)
            return {};
        // Both storage classes/nonempty shapes precede BOTH size checks.
        for (qsizetype n = i; n < i + 2; ++n) {
            const auto &v = r[n * 3], &length = r[n * 3 + 2];
            if (!type(r, n, "blob") ||
                (length.metaType().id() != QMetaType::Int &&
                 length.metaType().id() != QMetaType::LongLong) ||
                length.toLongLong() <= 0 || v.metaType().id() != QMetaType::QByteArray ||
                v.toByteArray().size() != length.toLongLong())
                fail(HR::invalid_stored_data);
        }
        if (r[i * 3 + 2].toLongLong() > limits_.component_bytes ||
            r[(i + 1) * 3 + 2].toLongLong() > limits_.component_bytes)
            fail(HR::result_limit);
        try {
            return decode_plan_exact({r[i * 3].toByteArray(), r[(i + 1) * 3].toByteArray()},
                                     limits_.component_bytes);
        } catch (const SqlitePlanInputFailure &e) {
            if (e.error() == SqlitePlanInputError::exact_component_limit)
                fail(HR::result_limit);
            fail(HR::invalid_stored_data);
        }
    }
    static int tag(const ApplicationHoldSubject &s) { return int(s.id.index()); }
    static std::uint64_t subject_id(const ApplicationHoldSubject &s) {
        return std::visit([](auto x) { return x.value; }, s.id);
    }
    ApplicationHoldSubject read_subject(const SqlRow &r, qsizetype i) {
        ApplicationHoldSubject s;
        const auto k = enumeration(r, i, 1);
        const auto n = id(r, i + 1, k == 1);
        if (k == 0)
            s.id = core::ContributionId{n};
        else
            s.id = core::ReceiptId{n};
        return s;
    }
    qint64 count(const QString &t, const QString &where, Values b) {
        auto rows =
            execute({"SELECT " + projection({"count(*)"}) + " FROM main." + t + " WHERE " + where,
                     std::move(b)});
        if (rows.size() != 1)
            fail(HR::invalid_stored_data);
        shape(rows.front(), 1);
        auto n = integer(rows.front(), 0);
        if (n < 0)
            fail(HR::invalid_stored_data);
        return n;
    }
    // Unfiltered counts and key/ordinal domains are proven before any full vector allocation.
    qint64 collection(const QString &t, const QString &where, Values b, qint64 declared,
                      const QString &ordinal = {}) {
        if (declared < 0)
            fail(HR::invalid_stored_data);
        if (ordinal.isEmpty()) {
            QStringList keys;
            if (t == "subathon_hold_resolution_expected")
                keys = {"mutation_ref", "hold_ref", "version_ref"};
            else if (t == "subathon_hold_resolution_entries")
                keys = {"mutation_ref", "hold_ref", "expected_version_ref", "proposed_version_ref"};
            if (keys.isEmpty()) {
                const auto n = count(t, where, std::move(b));
                if (n != declared)
                    fail(HR::invalid_stored_data);
                return n;
            }
            QStringList bad;
            for (const auto &k : keys)
                bad.append("typeof(" + k + ")<>'blob' OR length(" + k + ")=0");
            if (t == "subathon_hold_resolution_entries")
                bad.append(
                    "(replacement_hold_ref IS NULL)<>(replacement_detection_ref IS NULL) OR "
                    "(replacement_hold_ref IS NOT NULL AND (typeof(replacement_hold_ref)<>'blob' "
                    "OR length(replacement_hold_ref)=0 OR "
                    "typeof(replacement_detection_ref)<>'blob' OR "
                    "length(replacement_detection_ref)=0))");
            auto rows =
                execute({"SELECT " +
                             projection({"count(*)", "coalesce(max(CASE WHEN " + bad.join(" OR ") +
                                                         " THEN 1 ELSE 0 END),0)"}) +
                             " FROM main." + t + " WHERE " + where,
                         std::move(b)});
            if (rows.size() != 1)
                fail(HR::invalid_stored_data);
            shape(rows.front(), 2);
            const auto n = integer(rows.front(), 0);
            if (n != declared || integer(rows.front(), 1) != 0)
                fail(HR::invalid_stored_data);
            return n;
        }
        const auto bad = "coalesce(max(CASE WHEN typeof(" + ordinal + ")<>'integer' OR " + ordinal +
                         "<1 THEN 1 ELSE 0 END),0)";
        auto rows = execute(
            {"SELECT " +
                 projection({"count(*)", "min(" + ordinal + ")", "max(" + ordinal + ")", bad}) +
                 " FROM main." + t + " WHERE " + where,
             std::move(b)});
        if (rows.size() != 1)
            fail(HR::invalid_stored_data);
        const auto &r = rows.front();
        shape(r, 4);
        auto n = integer(r, 0);
        if (n != declared || integer(r, 3) != 0 ||
            (n == 0 ? !null(r, 1) || !null(r, 2) : integer(r, 1) != 1 || integer(r, 2) != n))
            fail(HR::invalid_stored_data);
        return n;
    }
    template <class F>
    void keyed(const QString &table, const QStringList &cols, const QString &where, Values values,
               const QString &key, F f) {
        std::optional<std::string> previous;
        for (;;) {
            auto b = values;
            auto w = where;
            if (previous) {
                w += " AND " + key + ">?";
                b.append(bytes(*previous));
            }
            auto rows = execute({"SELECT " + projection(cols) + " FROM main." + table + " WHERE " +
                                     w + " ORDER BY " + key + " LIMIT 65",
                                 std::move(b)});
            const auto n = std::min<qsizetype>(64, rows.size());
            for (qsizetype i = 0; i < n; ++i) {
                shape(rows[i], cols.size());
                auto current = ref(rows[i], cols.indexOf(key));
                if (previous && !hold_detail::raw_less(*previous, current))
                    fail(HR::invalid_stored_data);
                previous = current;
                f(rows[i]);
            }
            if (rows.size() <= 64)
                break;
        }
    }
    struct MutationMeta {
        std::string ref;
        HM kind;
        ApplicationHoldSubject subject;
        qint64 expected, entries;
    };
    std::optional<MutationMeta> mutation_meta(const std::string &key) {
        const QStringList cols{"mutation_ref", "mutation_kind",  "subject_kind",
                               "subject_id",   "expected_count", "entry_count"};
        auto row = one("subathon_hold_mutations", cols, "mutation_ref=?", {bytes(key)});
        if (!row)
            return {};
        const auto &registry = *row;
        MutationMeta m{ref(registry, 0), static_cast<HM>(enumeration(registry, 1, 1)),
                       read_subject(registry, 2), integer(registry, 4), integer(registry, 5)};
        if (m.ref != key || m.expected < 0 || m.entries < 1)
            fail(HR::invalid_stored_data);
        const auto dc = count("subathon_hold_detections", "mutation_ref=?", {bytes(key)});
        if (m.kind == HM::detection) {
            if (m.expected != 0 || m.entries != 1 || dc != 1 ||
                count("subathon_hold_resolution_expected", "mutation_ref=?", {bytes(key)}) != 0 ||
                count("subathon_hold_resolution_entries", "mutation_ref=?", {bytes(key)}) != 0 ||
                count("subathon_hold_detection_observations", "mutation_ref=?", {bytes(key)}) !=
                    1 ||
                count("subathon_hold_versions", "mutation_ref=?", {bytes(key)}) != 1)
                fail(HR::invalid_stored_data);
        } else {
            if (dc != 0 ||
                count("subathon_hold_detection_observations", "mutation_ref=?", {bytes(key)}) != 0)
                fail(HR::invalid_stored_data);
            collection("subathon_hold_resolution_expected", "mutation_ref=?", {bytes(key)},
                       m.expected);
            collection("subathon_hold_resolution_entries", "mutation_ref=?", {bytes(key)},
                       m.entries);
            if (count("subathon_hold_versions", "mutation_ref=?", {bytes(key)}) != m.entries)
                fail(HR::invalid_stored_data);
            // Audit ALL scalar batch ownership without loading old event documents
            // or recursively expanding another hold's current view.
            qint64 expected_seen = 0, entries_seen = 0;
            keyed("subathon_hold_resolution_expected", hold_resolution_expected_columns,
                  "mutation_ref=?", {bytes(key)}, "hold_ref", [&](const SqlRow &r) {
                      if (ref(r, 0) != key)
                          fail(HR::invalid_stored_data);
                      const auto h = ref(r, 1);
                      auto a = anchor(h);
                      if (!a || !hold_detail::same(a->subject, m.subject) ||
                          version(h, ref(r, 2)).state != HS::active)
                          fail(HR::invalid_stored_data);
                      ++expected_seen;
                  });
            keyed("subathon_hold_resolution_entries",
                  {"mutation_ref", "hold_ref", "expected_version_ref", "proposed_version_ref",
                   "replacement_hold_ref", "replacement_detection_ref"},
                  "mutation_ref=?", {bytes(key)}, "hold_ref", [&](const SqlRow &r) {
                      if (ref(r, 0) != key || null(r, 4) != null(r, 5))
                          fail(HR::invalid_stored_data);
                      const auto h = ref(r, 1), before = ref(r, 2), after = ref(r, 3);
                      auto a = anchor(h);
                      if (!a || !hold_detail::same(a->subject, m.subject))
                          fail(HR::invalid_stored_data);
                      auto old = version(h, before), produced = version(h, after);
                      if (old.state != HS::active ||
                          old.position == std::numeric_limits<qint64>::max() ||
                          produced.position != old.position + 1 ||
                          produced.parent_version_ref != std::optional{before} ||
                          produced.state != HS::resolved || produced.producer_mutation_ref != key)
                          fail(HR::invalid_stored_data);
                      auto expected =
                          required("subathon_hold_resolution_expected", {"version_ref"},
                                   "mutation_ref=? AND hold_ref=?", {bytes(key), bytes(h)});
                      if (ref(expected, 0) != before || bool(produced.replacement) != !null(r, 4))
                          fail(HR::invalid_stored_data);
                      if (produced.replacement) {
                          auto next = anchor(ref(r, 4));
                          if (!next || !hold_detail::same(next->subject, m.subject) ||
                              next->supersedes != std::optional{h} || next->first != ref(r, 5) ||
                              produced.replacement->hold_ref != ref(r, 4) ||
                              produced.replacement->detection_ref != ref(r, 5))
                              fail(HR::invalid_stored_data);
                          auto d =
                              required("subathon_hold_detections",
                                       {"hold_ref", "expected_version_ref", "proposed_version_ref"},
                                       "mutation_ref=?", {bytes(next->first)});
                          auto first = version(ref(r, 4), ref(d, 2));
                          if (ref(d, 0) != ref(r, 4) || !null(d, 1) || first.position != 1 ||
                              first.producer_mutation_ref != next->first)
                              fail(HR::invalid_stored_data);
                      }
                      ++entries_seen;
                  });
            if (expected_seen != m.expected || entries_seen != m.entries)
                fail(HR::invalid_stored_data);
        }
        return m;
    }
    struct Anchor {
        ApplicationHoldSubject subject;
        std::string first;
        std::optional<std::string> supersedes;
    };
    std::optional<Anchor> anchor(const std::string &h) {
        const QStringList cols{"hold_ref",        "subject_kind",        "subject_id",
                               "contribution_id", "first_detection_ref", "supersedes_hold_ref"};
        auto r = one("subathon_hold_causes", cols, "hold_ref=?", {bytes(h)});
        if (!r)
            return {};
        if (ref(*r, 0) != h)
            fail(HR::invalid_stored_data);
        auto s = read_subject(*r, 1);
        if (tag(s) == 0 ? null(*r, 3) || id(*r, 3) != subject_id(s) : !null(*r, 3))
            fail(HR::invalid_stored_data);
        return Anchor{std::move(s), ref(*r, 4), optref(*r, 5)};
    }
    HV read_version(const SqlRow &r, const std::string &h = {}) {
        HV v{{ref(r, 0), ref(r, 1)},
             integer(r, 2),
             optref(r, 3),
             static_cast<HS>(enumeration(r, 4, 1)),
             ref(r, 5),
             {}};
        if (!h.empty() && v.locator.hold_ref != h)
            fail(HR::invalid_stored_data);
        if (null(r, 6) != null(r, 7))
            fail(HR::invalid_stored_data);
        if (!null(r, 6))
            v.replacement = ApplicationHoldReplacementLocator{ref(r, 6), ref(r, 7)};
        if (v.position < 1 ||
            (v.position == 1 ? bool(v.parent_version_ref) || v.state != HS::active
                             : !v.parent_version_ref) ||
            (v.state == HS::active && v.replacement))
            fail(HR::invalid_stored_data);
        return v;
    }
    HV version(const std::string &h, const std::string &v) {
        auto r = required("subathon_hold_versions", hold_versions_columns,
                          "hold_ref=? AND version_ref=?", {bytes(h), bytes(v)});
        auto out = read_version(r, h);
        if (out.locator.version_ref != v)
            fail(HR::invalid_stored_data);
        return out;
    }
    std::optional<std::string> lineage_step(const std::optional<std::string> &h,
                                            const ApplicationHoldSubject &s) {
        if (!h)
            return {};
        auto m = metadata_core(*h);
        if (!m || !hold_detail::same(m->anchor.subject, s) || m->current.state != HS::resolved)
            fail(HR::invalid_stored_data);
        reciprocal(*h, *m);
        return m->anchor.supersedes;
    }
    void lineage(const std::string &h, const ApplicationHoldSubject &s) {
        std::optional<std::string> slow = h, fast = h;
        do {
            slow = lineage_step(slow, s);
            fast = lineage_step(lineage_step(fast, s), s);
            if (slow && fast && *slow == *fast)
                fail(HR::invalid_stored_data);
        } while (fast);
    }
    void owner(const HV &v, const ApplicationHoldSubject &s) {
        auto m = mutation_meta(v.producer_mutation_ref);
        if (!m || !hold_detail::same(m->subject, s))
            fail(HR::invalid_stored_data);
        if (m->kind == HM::detection) {
            const QStringList cols{"mutation_ref", "hold_ref", "expected_version_ref",
                                   "proposed_version_ref"};
            auto r = required("subathon_hold_detections", cols, "mutation_ref=?", {bytes(m->ref)});
            if (ref(r, 0) != m->ref || ref(r, 1) != v.locator.hold_ref ||
                optref(r, 2) != v.parent_version_ref || ref(r, 3) != v.locator.version_ref ||
                v.state != HS::active || v.replacement)
                fail(HR::invalid_stored_data);
        } else {
            auto r = required(
                "subathon_hold_resolution_entries",
                {"mutation_ref", "hold_ref", "expected_version_ref", "proposed_version_ref",
                 "replacement_hold_ref", "replacement_detection_ref"},
                "mutation_ref=? AND hold_ref=?", {bytes(m->ref), bytes(v.locator.hold_ref)});
            if (ref(r, 0) != m->ref || ref(r, 1) != v.locator.hold_ref || !v.parent_version_ref ||
                ref(r, 2) != *v.parent_version_ref || ref(r, 3) != v.locator.version_ref ||
                v.state != HS::resolved || null(r, 4) != null(r, 5))
                fail(HR::invalid_stored_data);
            if (bool(v.replacement) != !null(r, 4))
                fail(HR::invalid_stored_data);
            if (v.replacement &&
                (v.replacement->hold_ref != ref(r, 4) || v.replacement->detection_ref != ref(r, 5)))
                fail(HR::invalid_stored_data);
            auto e = required("subathon_hold_resolution_expected", hold_resolution_expected_columns,
                              "mutation_ref=? AND hold_ref=?",
                              {bytes(m->ref), bytes(v.locator.hold_ref)});
            if (ref(e, 0) != m->ref || ref(e, 1) != v.locator.hold_ref ||
                ref(e, 2) != *v.parent_version_ref)
                fail(HR::invalid_stored_data);
            if (v.replacement) {
                auto next = anchor(v.replacement->hold_ref);
                if (!next || !hold_detail::same(next->subject, s) ||
                    next->supersedes != std::optional{v.locator.hold_ref} ||
                    next->first != v.replacement->detection_ref)
                    fail(HR::invalid_stored_data);
                auto d = required(
                    "subathon_hold_detections",
                    {"mutation_ref", "hold_ref", "expected_version_ref", "proposed_version_ref"},
                    "mutation_ref=?", {bytes(next->first)});
                if (ref(d, 0) != next->first || ref(d, 1) != v.replacement->hold_ref || !null(d, 2))
                    fail(HR::invalid_stored_data);
                auto first = version(v.replacement->hold_ref, ref(d, 3));
                if (first.position != 1 || first.producer_mutation_ref != next->first ||
                    first.state != HS::active)
                    fail(HR::invalid_stored_data);
            }
        }
    }
    struct Metadata {
        Anchor anchor;
        HV first, current;
        qint64 count;
    };
    // Complete scalar certificate, deliberately without following supersedes.
    // Ancestor walking composes this primitive without recursion through views.
    std::optional<Metadata> metadata_core(const std::string &h) {
        auto a = anchor(h);
        if (!a)
            return {};
        auto head = required("subathon_hold_heads", hold_heads_columns, "hold_ref=?", {bytes(h)});
        if (ref(head, 0) != h)
            fail(HR::invalid_stored_data);
        const auto declared = integer(head, 2);
        if (declared < 1)
            fail(HR::invalid_stored_data);
        collection("subathon_hold_versions", "hold_ref=?", {bytes(h)}, declared, "position");
        std::optional<HV> first, previous;
        qint64 seen = 0;
        for (;;) {
            Values b{bytes(h)};
            QString w = "hold_ref=?";
            if (previous) {
                w += " AND position>?";
                b.append(qint64(previous->position));
            }
            auto rows = execute({"SELECT " + projection(hold_versions_columns) +
                                     " FROM main.subathon_hold_versions WHERE " + w +
                                     " ORDER BY position LIMIT 65",
                                 std::move(b)});
            auto n = std::min<qsizetype>(64, rows.size());
            for (qsizetype i = 0; i < n; ++i) {
                shape(rows[i], 8);
                auto v = read_version(rows[i], h);
                if (seen == std::numeric_limits<qint64>::max() || v.position != seen + 1 ||
                    (previous &&
                     (previous->state == HS::resolved ||
                      v.parent_version_ref != std::optional{previous->locator.version_ref})))
                    fail(HR::invalid_stored_data);
                owner(v, a->subject);
                if (!first)
                    first = v;
                previous = std::move(v);
                ++seen;
            }
            if (rows.size() <= 64)
                break;
        }
        if (seen != declared || !first || !previous ||
            ref(head, 1) != previous->locator.version_ref ||
            first->producer_mutation_ref != a->first)
            fail(HR::invalid_stored_data);
        return Metadata{std::move(*a), std::move(*first), std::move(*previous), seen};
    }
    void reciprocal(const std::string &h, const Metadata &m) {
        const QStringList cols{"mutation_ref",         "hold_ref",
                               "expected_version_ref", "proposed_version_ref",
                               "replacement_hold_ref", "replacement_detection_ref"};
        auto row =
            one("subathon_hold_resolution_entries", cols, "replacement_hold_ref=?", {bytes(h)});
        // A standalone later reoverflow legitimately has no producing replacement
        // entry. Its supplied predecessor still receives the full scalar audit.
        if (!row)
            return;
        const auto &r = *row;
        if (ref(r, 4) != h || ref(r, 5) != m.anchor.first ||
            m.anchor.supersedes != std::optional{ref(r, 1)})
            fail(HR::invalid_stored_data);
        auto old = metadata_core(ref(r, 1));
        if (!old || !hold_detail::same(old->anchor.subject, m.anchor.subject) ||
            old->current.state != HS::resolved || old->current.locator.version_ref != ref(r, 3) ||
            old->current.parent_version_ref != std::optional{ref(r, 2)} ||
            old->current.producer_mutation_ref != ref(r, 0) || !old->current.replacement ||
            old->current.replacement->hold_ref != h ||
            old->current.replacement->detection_ref != m.anchor.first)
            fail(HR::invalid_stored_data);
        auto producer = mutation_meta(ref(r, 0));
        if (!producer || producer->kind != HM::resolution ||
            !hold_detail::same(producer->subject, m.anchor.subject))
            fail(HR::invalid_stored_data);
    }
    std::optional<Metadata> metadata(const std::string &h) {
        auto m = metadata_core(h);
        if (!m)
            return {};
        reciprocal(h, *m);
        if (m->anchor.supersedes)
            lineage(*m->anchor.supersedes, m->anchor.subject);
        return m;
    }
    [[noreturn]] void reference_failure(bool incoming, bool missing = false) {
        fail(incoming ? (missing ? HR::missing_reference : HR::source_mismatch)
                      : HR::invalid_stored_data);
    }
    std::optional<RetainedDocument> retained(const RetainedKey &k) {
        try {
            return SqliteRetainedContextStore(session_,
                                              {limits_.reference_bytes, limits_.value_bytes})
                .find(k);
        } catch (const RetentionFailure &e) {
            // The retained primitive has already established rollback for stored faults.
            throw SqliteHoldStorageFailure(e.error() == RetentionError::result_limit
                                               ? HR::result_limit
                                               : HR::invalid_stored_data);
        } catch (const SqliteWorkFailure &e) {
            // Known query failures already rolled back; preserve all other work truth.
            map_work_failure(e);
        }
    }
    void known_state(const std::optional<std::string> &token, bool incoming) {
        if (!token)
            return;
        if (!one("subathon_app_states", {"token"}, "token=?", {bytes(*token)}))
            reference_failure(incoming, true);
        auto s = Access::state(*token);
        Access::result_size(s.balances.size(), bundle_detail::logical_bytes(s));
    }
    void source_references(const ApplicationHoldCause &c, bool incoming) {
        const auto &s = c.source;
        auto cfg = retained({RetainedKind::configuration, s.configuration_ref});
        if (!cfg)
            reference_failure(incoming, true);
        auto ctx = retained({RetainedKind::arrival_context, s.arrival_context_ref});
        if (!ctx)
            reference_failure(incoming, true);
        if (ctx->configuration && ctx->configuration->reference != s.configuration_ref)
            reference_failure(incoming);
        auto receipt = Access::original(s.receipt_id);
        if (!receipt)
            reference_failure(incoming, true);
        if (receipt->arrival.configuration_ref != s.configuration_ref ||
            receipt->arrival.modifier_context_ref != s.arrival_context_ref)
            reference_failure(incoming);
        Access::result_bytes(0, [&](auto &n) {
            n.add(8);
            bundle_detail::count_bytes(n, receipt->identity);
            bundle_detail::count_bytes(n, receipt->facts);
            bundle_detail::count_bytes(n, receipt->arrival);
        });
        if (tag(s.subject) == 0) {
            auto cid = std::get<core::ContributionId>(s.subject.id);
            auto current = Access::contribution_by_id(cid);
            if (!current)
                reference_failure(incoming, true);
            auto support = Access::observation(current->original_receipt_id, false);
            if (!support || !anchor_matches(*current, support))
                fail(HR::invalid_stored_data);
            auto event = Access::observation(s.receipt_id, false);
            if (!event || event->contribution_id != std::optional{cid})
                reference_failure(incoming);
            Access::result_size(
                0, bundle_detail::logical_bytes(ApplicationContributionInput{*current, *support}));
        } else if (subject_id(s.subject) != s.receipt_id.value)
            reference_failure(incoming);
        if (c.plan_evidence.plan) {
            auto p = Access::plan(*c.plan_evidence.plan);
            if (!p)
                reference_failure(incoming, true);
            Access::result_size(Access::plan_items(p->selected),
                                bundle_detail::logical_bytes(p->selected));
            const auto &e = p->selected.captured.event;
            std::optional<core::ContributionId> wanted;
            if (tag(s.subject) == 0)
                wanted = std::get<core::ContributionId>(s.subject.id);
            if (e.receipt_id != s.receipt_id || e.contribution_id != wanted ||
                (s.event_ref && *s.event_ref != e.event_ref) ||
                (s.parent_event_ref && s.parent_event_ref != e.parent_event_ref) ||
                (s.root_event_ref && *s.root_event_ref != e.root_event_ref))
                reference_failure(incoming);
            if (c.plan_evidence.kind == ApplicationHoldPlanKind::captured_revision) {
                const auto &requests = p->selected.captured.requested_effects;
                if (requests.size() != c.plan_evidence.occurrences.size())
                    reference_failure(incoming);
                for (size_t i = 0; i < requests.size(); ++i) {
                    const auto &q = requests[i];
                    const auto &o = c.plan_evidence.occurrences[i];
                    if ((o.action_ref && *o.action_ref != q.action_ref) ||
                        (o.action_version_ref && *o.action_version_ref != q.action_version_ref) ||
                        (o.rule_ref && *o.rule_ref != q.rule_ref) ||
                        (o.target_ref && *o.target_ref != q.target_ref) ||
                        o.unit != std::optional{q.unit} || !o.attempted_units ||
                        *o.attempted_units != q.exact_requested_units ||
                        o.finalized_units != std::optional{q.requested_units} ||
                        (o.explanation && *o.explanation != q.calculation_explanation))
                        reference_failure(incoming);
                }
            }
        }
        known_state(c.first_observation.persisted_state_token, incoming);
        if (c.timer && c.timer->associated) {
            auto app = Access::lookup(c.timer->associated->operation);
            if (!app)
                reference_failure(incoming, true);
            if (app->intent.kind != ApplicationOperationKind::original ||
                app->intent.receipt_id != s.receipt_id || !same_subject(app->intent, s.subject) ||
                !c.plan_evidence.plan ||
                app->intent.plan.plan_ref != c.plan_evidence.plan->plan_ref ||
                app->intent.plan.revision != c.plan_evidence.plan->revision ||
                app->intent.operation.operation_scope != c.command.operation.operation_scope ||
                app->intent.operation.operation_ref != c.command.operation.operation_ref)
                reference_failure(incoming);
            const auto n = c.timer->request_ordinal;
            if (n < 1 || std::uintmax_t(n) > app->effects.size())
                reference_failure(incoming);
            const auto &e = app->effects[size_t(n - 1)];
            if (e.request_ordinal != n || e.arithmetic_role != AR::time_reversal ||
                e.requested_units != c.timer->request_units ||
                e.deficit_policy != std::optional{core::RefundDeficitPolicy::RequireReview} ||
                e.deficit_disposition !=
                    std::optional{core::TimeReversalDisposition::RequiresReview} ||
                e.components.size() != 3 || e.components[0].kind != CK::main_delta ||
                e.components[0].units != 0 || e.components[1].kind != CK::debt_delta ||
                e.components[1].units != 0 || e.components[2].kind != CK::unapplied_time ||
                e.components[2].units != c.timer->unapplied_units)
                reference_failure(incoming);
        }
    }
    static bool same_subject(const ApplicationIntent &i, const ApplicationHoldSubject &s) {
        return tag(s) == 0
                   ? i.subject_kind == ApplicationSubjectKind::contribution &&
                         i.contribution_id == std::optional{std::get<core::ContributionId>(s.id)}
                   : i.subject_kind == ApplicationSubjectKind::receipt && !i.contribution_id &&
                         i.receipt_id.value == subject_id(s);
    }
    ApplicationHoldObservation load_observation(const QString &t, const QString &key,
                                                const std::string &value) {
        const auto &cols = t == "subathon_hold_cause_observations"
                               ? hold_cause_observations_columns
                               : hold_detection_observations_columns;
        auto r = required(t, cols, key + "=?", {bytes(value)});
        if (ref(r, 0) != value)
            fail(HR::invalid_stored_data);
        ApplicationHoldObservation o{optref(r, 1), envelope(r, 2), optref(r, 4), optref(r, 5),
                                     optref(r, 6), optref(r, 7),   optint(r, 8), optint(r, 9)};
        stored_shape([&] { hold_detail::shape_observation(o, holds); });
        return o;
    }
    ApplicationHoldCause full_cause(const std::string &h) {
        auto r = required("subathon_hold_causes", hold_causes_columns, "hold_ref=?", {bytes(h)});
        ApplicationHoldCause c;
        c.hold_ref = ref(r, 0);
        c.source.subject = read_subject(r, 1);
        if (c.hold_ref != h ||
            (tag(c.source.subject) == 0 ? null(r, 3) || id(r, 3) != subject_id(c.source.subject)
                                        : !null(r, 3)))
            fail(HR::invalid_stored_data);
        c.source.receipt_id = {id(r, 4, true)};
        c.source.event_ref = optional_string(r, 5, limits_.reference_bytes);
        c.source.parent_event_ref = optional_string(r, 6, limits_.reference_bytes);
        c.source.root_event_ref = optional_string(r, 7, limits_.reference_bytes);
        c.source.configuration_ref = ref(r, 8);
        c.source.arrival_context_ref = ref(r, 9);
        c.command = {{ref(r, 10), ref(r, 11)}, envelope(r, 12)};
        c.kind = static_cast<HC>(enumeration(r, 14, 2));
        c.supersedes_hold_ref = optref(r, 15);
        const auto first = ref(r, 16);
        c.plan_evidence.kind = static_cast<ApplicationHoldPlanKind>(enumeration(r, 17, 1));
        if (null(r, 18) != null(r, 19))
            fail(HR::invalid_stored_data);
        if (!null(r, 18))
            c.plan_evidence.plan = ApplicationPlanKey{ref(r, 18), id(r, 19, true)};
        c.plan_evidence.extent = static_cast<ApplicationHoldExtent>(enumeration(r, 20, 2));
        c.plan_evidence.stopped_for = optenum<ApplicationHoldStoppedFor>(r, 21, 1);
        c.plan_evidence.failed_capture = optenv(r, 22);
        const auto n = integer(r, 24);
        collection("subathon_hold_occurrences", "hold_ref=?", {bytes(h)}, n, "ordinal");
        if (n > holds.known_occurrences_per_cause)
            fail(HR::result_limit);
        // No full occurrence vector is grown until actual count/domain/ordinals agree.
        c.plan_evidence.occurrences.reserve(size_t(n));
        qint64 seen = 0;
        for (;;) {
            Values b{bytes(h)};
            QString w = "hold_ref=?";
            if (seen) {
                w += " AND ordinal>?";
                b.append(seen);
            }
            auto rows = execute({"SELECT " + projection(hold_occurrences_columns) +
                                     " FROM main.subathon_hold_occurrences WHERE " + w +
                                     " ORDER BY ordinal LIMIT 65",
                                 std::move(b)});
            const auto take = std::min<qsizetype>(64, rows.size());
            for (qsizetype i = 0; i < take; ++i) {
                auto &o = rows[i];
                shape(o, 11);
                if (ref(o, 0) != h || integer(o, 1) != seen + 1)
                    fail(HR::invalid_stored_data);
                ApplicationHoldProposedOccurrence v{integer(o, 1),
                                                    optional_string(o, 2, limits_.reference_bytes),
                                                    optional_string(o, 3, limits_.reference_bytes),
                                                    optional_string(o, 4, limits_.reference_bytes),
                                                    optional_string(o, 5, limits_.reference_bytes),
                                                    optenum<core::RequestedUnit>(o, 6, 2),
                                                    optional_exact(o, 7),
                                                    optint(o, 9),
                                                    optional_string(o, 10, limits_.value_bytes)};
                c.plan_evidence.occurrences.push_back(std::move(v));
                ++seen;
            }
            if (rows.size() <= 64)
                break;
        }
        if (seen != n)
            fail(HR::invalid_stored_data);
        if (null(r, 25)) {
            all_null(r, 26, 31);
        } else {
            ApplicationHoldFailureEvidence f;
            f.stage = static_cast<ApplicationHoldFailureStage>(enumeration(r, 25, 1));
            f.failing_ordinal = optint(r, 26);
            f.failing_target_ref = optional_string(r, 27, limits_.reference_bytes);
            f.attempted_units = optional_exact(r, 28);
            f.annotation = envelope(r, 30);
            c.failure = std::move(f);
        }
        if (null(r, 32)) {
            all_null(r, 33, 40);
        } else {
            ApplicationHoldTimerEvidence t;
            t.request_ordinal = integer(r, 32);
            t.component_kind = static_cast<CK>(enumeration(r, 33, 5));
            t.request_units = integer(r, 34);
            t.deficit_policy = static_cast<core::RefundDeficitPolicy>(enumeration(r, 35, 2));
            t.deficit_disposition =
                static_cast<core::TimeReversalDisposition>(enumeration(r, 36, 1));
            t.unapplied_units = integer(r, 37);
            t.composition = optenum<ApplicationHoldComposition>(r, 38, 1);
            t.composition_decision = envelope(r, 39);
            c.timer = std::move(t);
        }
        c.missing_policy_reason = optional_string(r, 41, limits_.value_bytes);
        const bool associated = !null(r, 42);
        if (associated != !null(r, 43) || associated != !null(r, 44))
            fail(HR::invalid_stored_data);
        if (associated) {
            if (!c.timer)
                fail(HR::invalid_stored_data);
            c.timer->associated =
                ApplicationHoldAssociatedApplication{{ref(r, 42), ref(r, 43)}, integer(r, 44)};
        }
        c.first_observation = load_observation("subathon_hold_cause_observations", "hold_ref", h);
        stored_shape([&] { hold_detail::shape_cause(c, holds); });
        auto a = anchor(h);
        if (!a || a->first != first)
            fail(HR::invalid_stored_data);
        source_references(c, false);
        return c;
    }
    ApplicationHoldDetectionData full_data(const std::string &d, const ApplicationHoldCause &c) {
        auto registry = required("subathon_hold_mutations", hold_mutations_columns,
                                 "mutation_ref=?", {bytes(d)});
        if (ref(registry, 0) != d || enumeration(registry, 1, 1) != 0 ||
            !hold_detail::same(read_subject(registry, 2), c.source.subject) ||
            integer(registry, 4) != 0 || integer(registry, 5) != 1)
            fail(HR::invalid_stored_data);
        all_null(registry, 6, 21);
        auto r = required("subathon_hold_detections", hold_detections_columns, "mutation_ref=?",
                          {bytes(d)});
        if (ref(r, 0) != d || ref(r, 1) != c.hold_ref)
            fail(HR::invalid_stored_data);
        ApplicationHoldDetectionData data{
            ref(r, 0), optref(r, 2), ref(r, 3),
            load_observation("subathon_hold_detection_observations", "mutation_ref", d),
            envelope(r, 4)};
        stored_shape([&] { hold_detail::shape_detection_data(data, holds); });
        if (c.plan_evidence.kind == ApplicationHoldPlanKind::captured_revision &&
            !data.observation.persisted_state_token)
            fail(HR::invalid_stored_data);
        known_state(data.observation.persisted_state_token, false);
        return data;
    }
    ApplicationHoldDetection full_detection(const std::string &d, const Metadata *checked = nullptr,
                                            const ApplicationHoldCause *checked_cause = nullptr) {
        auto m = mutation_meta(d);
        if (!m || m->kind != HM::detection)
            fail(HR::invalid_stored_data);
        auto r = required("subathon_hold_detections", {"hold_ref", "proposed_version_ref"},
                          "mutation_ref=?", {bytes(d)});
        const auto h = ref(r, 0);
        const auto meta = checked ? std::optional<Metadata>{} : metadata(h);
        const auto *actual = checked ? checked : (meta ? &*meta : nullptr);
        if (!actual || actual->current.locator.hold_ref != h ||
            !hold_detail::same(actual->anchor.subject, m->subject))
            fail(HR::invalid_stored_data);
        auto cause = checked_cause ? *checked_cause : full_cause(h);
        if (cause.hold_ref != h)
            fail(HR::invalid_stored_data);
        auto data = full_data(d, cause);
        auto v = version(h, data.proposed_version_ref);
        if (v.producer_mutation_ref != d || v.parent_version_ref != data.expected_version_ref ||
            v.state != HS::active)
            fail(HR::invalid_stored_data);
        ApplicationHoldDetection out{std::move(cause), std::move(data)};
        stored_shape([&] { hold_detail::shape_detection(out, holds); });
        return out;
    }
    void application_link(const ApplicationHoldResolutionBatch &b, bool incoming) {
        if (!b.application)
            return;
        const auto &link = *b.application;
        auto app = Access::lookup(link.operation);
        if (!app)
            reference_failure(incoming, true);
        if (app->intent.kind != ApplicationOperationKind::original ||
            app->intent.receipt_id != link.receipt_id ||
            app->intent.plan.plan_ref != link.plan.plan_ref ||
            app->intent.plan.revision != link.plan.revision)
            reference_failure(incoming);
        if (link.kind == ApplicationHoldApplicationLinkKind::whole_input) {
            if (!same_subject(app->intent, b.subject))
                reference_failure(incoming);
            for (const auto &e : b.entries) {
                auto c = full_cause(e.hold_ref);
                if (c.timer &&
                    c.timer->composition == ApplicationHoldComposition::beside_complete_application)
                    reference_failure(incoming);
            }
        } else {
            if (!link.held_origin)
                reference_failure(incoming);
            const auto &o = *link.held_origin;
            auto entry = std::find_if(b.entries.begin(), b.entries.end(), [&](const auto &e) {
                return e.hold_ref == o.version.hold_ref;
            });
            if (entry == b.entries.end() || entry->expected_version_ref != o.version.version_ref)
                reference_failure(incoming);
            auto cause = full_cause(entry->hold_ref);
            if (!cause.timer || o.request_ordinal != cause.timer->request_ordinal ||
                o.component_kind != cause.timer->component_kind ||
                app->intent.subject_kind != ApplicationSubjectKind::receipt ||
                app->intent.contribution_id || app->intent.receipt_id == cause.source.receipt_id ||
                (link.operation.operation_scope == cause.command.operation.operation_scope &&
                 link.operation.operation_ref == cause.command.operation.operation_ref) ||
                (cause.timer->associated &&
                 link.operation.operation_scope ==
                     cause.timer->associated->operation.operation_scope &&
                 link.operation.operation_ref == cause.timer->associated->operation.operation_ref))
                reference_failure(incoming);
        }
    }
    ApplicationHoldRecordedResolution full_resolution(const std::string &key) {
        auto m = mutation_meta(key);
        if (!m || m->kind != HM::resolution)
            fail(HR::invalid_stored_data);
        auto r = required("subathon_hold_mutations", hold_mutations_columns, "mutation_ref=?",
                          {bytes(key)});
        if (ref(r, 0) != key || enumeration(r, 1, 1) != 1)
            fail(HR::invalid_stored_data);
        ApplicationHoldRecordedResolution out;
        auto &b = out.request;
        b.mutation_ref = key;
        b.subject = read_subject(r, 2);
        if (m->expected > holds.active_holds_per_result || m->entries > holds.resolution_entries)
            fail(HR::result_limit);
        // Actual counts were audited in mutation_meta, before either collection reserve.
        b.expected_active.reserve(size_t(m->expected));
        b.entries.reserve(size_t(m->entries));
        b.actor_ref = ref(r, 6);
        b.decision_ref = ref(r, 7);
        b.reason = string(r, 8, limits_.value_bytes);
        b.options = envelope(r, 9);
        if (null(r, 11)) {
            all_null(r, 12, 21);
        } else {
            ApplicationHoldApplicationLink link{
                static_cast<ApplicationHoldApplicationLinkKind>(enumeration(r, 11, 1)),
                {ref(r, 12), ref(r, 13)},
                {id(r, 14, true)},
                {ref(r, 15), id(r, 16, true)},
                {}};
            if (null(r, 17)) {
                all_null(r, 18, 21);
            } else {
                ApplicationHoldHeldOrigin o{{ref(r, 17), ref(r, 18)},
                                            integer(r, 19),
                                            static_cast<CK>(enumeration(r, 20, 5)),
                                            optint(r, 21)};
                link.held_origin = std::move(o);
            }
            b.application = std::move(link);
        }
        qint64 expected_seen = 0;
        keyed("subathon_hold_resolution_expected", hold_resolution_expected_columns,
              "mutation_ref=?", {bytes(key)}, "hold_ref", [&](const SqlRow &v) {
                  if (ref(v, 0) != key)
                      fail(HR::invalid_stored_data);
                  const auto h = ref(v, 1), token = ref(v, 2);
                  auto a = metadata(h);
                  if (!a || !hold_detail::same(a->anchor.subject, b.subject))
                      fail(HR::invalid_stored_data);
                  auto version_ = version(h, token);
                  if (version_.state != HS::active)
                      fail(HR::invalid_stored_data);
                  b.expected_active.push_back({h, token});
                  ++expected_seen;
              });
        if (expected_seen != m->expected)
            fail(HR::invalid_stored_data);
        // Derive complete replacement cardinality before allocating produced versions/full
        // replacements.
        auto totals = execute(
            {"SELECT " +
                 projection({"count(*)",
                             "coalesce(sum(CASE WHEN replacement_hold_ref IS NOT NULL THEN 1 ELSE "
                             "0 END),0)",
                             "coalesce(max(CASE WHEN (replacement_hold_ref IS "
                             "NULL)<>(replacement_detection_ref IS NULL) THEN 1 ELSE 0 END),0)"}) +
                 " FROM main.subathon_hold_resolution_entries WHERE mutation_ref=?",
             {bytes(key)}});
        if (totals.size() != 1)
            fail(HR::invalid_stored_data);
        shape(totals.front(), 3);
        auto replacements = integer(totals.front(), 1);
        if (integer(totals.front(), 0) != m->entries || integer(totals.front(), 2) != 0 ||
            replacements < 0 || replacements > m->entries)
            fail(HR::invalid_stored_data);
        out.produced_versions.reserve(add(size_t(m->entries), size_t(replacements)));
        qint64 entries_seen = 0;
        keyed("subathon_hold_resolution_entries", hold_resolution_entries_columns, "mutation_ref=?",
              {bytes(key)}, "hold_ref", [&](const SqlRow &v) {
                  ApplicationHoldResolutionEntry e{
                      ref(v, 1), ref(v, 2), ref(v, 3), envelope(v, 4), {}};
                  if (ref(v, 0) != key || null(v, 6) != null(v, 7))
                      fail(HR::invalid_stored_data);
                  auto a = metadata(e.hold_ref);
                  if (!a || !hold_detail::same(a->anchor.subject, b.subject))
                      fail(HR::invalid_stored_data);
                  const auto old = version(e.hold_ref, e.expected_version_ref),
                             produced = version(e.hold_ref, e.proposed_version_ref);
                  if (old.state != HS::active ||
                      old.position == std::numeric_limits<qint64>::max() ||
                      produced.position != old.position + 1 ||
                      produced.parent_version_ref != std::optional{old.locator.version_ref} ||
                      produced.state != HS::resolved || produced.producer_mutation_ref != key)
                      fail(HR::invalid_stored_data);
                  if (std::none_of(b.expected_active.begin(), b.expected_active.end(),
                                   [&](const auto &x) {
                                       return x.hold_ref == e.hold_ref &&
                                              x.version_ref == e.expected_version_ref;
                                   }))
                      fail(HR::invalid_stored_data);
                  out.produced_versions.push_back(produced);
                  if (!null(v, 6)) {
                      auto replacement = full_detection(ref(v, 7));
                      if (replacement.cause.hold_ref != ref(v, 6) ||
                          replacement.cause.supersedes_hold_ref != std::optional{e.hold_ref} ||
                          replacement.request.expected_version_ref ||
                          !hold_detail::same(replacement.cause.source.subject, b.subject) ||
                          !produced.replacement ||
                          produced.replacement->hold_ref != replacement.cause.hold_ref ||
                          produced.replacement->detection_ref != replacement.request.mutation_ref)
                          fail(HR::invalid_stored_data);
                      auto initial = version(replacement.cause.hold_ref,
                                             replacement.request.proposed_version_ref);
                      if (initial.position != 1 ||
                          initial.producer_mutation_ref != replacement.request.mutation_ref)
                          fail(HR::invalid_stored_data);
                      out.produced_versions.push_back(initial);
                      e.replacement = std::move(replacement);
                  } else if (produced.replacement)
                      fail(HR::invalid_stored_data);
                  b.entries.push_back(std::move(e));
                  ++entries_seen;
              });
        if (entries_seen != m->entries)
            fail(HR::invalid_stored_data);
        stored_shape([&] { hold_detail::shape_resolution(b, holds); });
        if (b.application &&
            (b.entries.size() != b.expected_active.size() ||
             std::any_of(b.entries.begin(), b.entries.end(),
                         [](const auto &e) { return bool(e.replacement); }) ||
             std::any_of(b.expected_active.begin(), b.expected_active.end(), [&](const auto &x) {
                 return std::none_of(b.entries.begin(), b.entries.end(),
                                     [&](const auto &e) { return e.hold_ref == x.hold_ref; });
             })))
            fail(HR::invalid_stored_data);
        application_link(b, false);
        return out;
    }
    std::optional<ApplicationHoldView> find(const std::string &h,
                                            const Metadata *checked = nullptr) {
        const auto loaded = checked ? std::optional<Metadata>{} : metadata(h);
        const auto *m = checked ? checked : (loaded ? &*loaded : nullptr);
        if (!m)
            return {};
        auto cause = full_cause(h);
        auto first = full_data(m->anchor.first, cause);
        if (first.expected_version_ref ||
            first.proposed_version_ref != m->first.locator.version_ref)
            fail(HR::invalid_stored_data);
        ApplicationHoldView out{std::move(cause), std::move(first), m->first, m->current, {}};
        if (m->current.locator.version_ref == m->first.locator.version_ref) {
            if (!hold_detail::same(m->first, m->current))
                fail(HR::invalid_stored_data);
        } else {
            auto event = mutation_meta(m->current.producer_mutation_ref);
            if (!event)
                fail(HR::invalid_stored_data);
            if (event->kind == HM::detection)
                out.current_event = full_data(event->ref, out.cause);
            else
                out.current_event = full_resolution(event->ref);
        }
        return out;
    }
    template <class F> qint64 subject_anchors(const ApplicationHoldSubject &s, F f) {
        const QString where = "subject_kind=? AND subject_id=?";
        Values b{tag(s), encode_plan_revision_uint64(subject_id(s))};
        const auto total = count("subathon_hold_causes", where, b);
        qint64 seen = 0;
        keyed("subathon_hold_causes",
              {"hold_ref", "subject_kind", "subject_id", "contribution_id", "first_detection_ref",
               "supersedes_hold_ref"},
              where, b, "hold_ref", [&](const SqlRow &r) {
                  const auto h = ref(r, 0);
                  auto a = anchor(h);
                  if (!a || !hold_detail::same(a->subject, s))
                      fail(HR::invalid_stored_data);
                  if (seen == std::numeric_limits<qint64>::max())
                      fail(HR::invalid_stored_data);
                  ++seen;
                  f(h);
              });
        if (seen != total)
            fail(HR::invalid_stored_data);
        return seen;
    }
    std::vector<ApplicationHoldVersionLocator> active_locators(const ApplicationHoldSubject &s) {
        size_t n = 0;
        subject_anchors(s, [&](const std::string &h) {
            auto m = metadata(h);
            if (!m)
                fail(HR::invalid_stored_data);
            if (m->current.state == HS::active)
                n = add(n, 1);
        });
        if (n > size_t(holds.active_holds_per_result))
            fail(HR::result_limit);
        std::vector<ApplicationHoldVersionLocator> out;
        out.reserve(n);
        subject_anchors(s, [&](const std::string &h) {
            auto r = required("subathon_hold_heads", hold_heads_columns, "hold_ref=?", {bytes(h)});
            auto v = version(h, ref(r, 1));
            if (v.state == HS::active)
                out.push_back(v.locator);
        });
        if (out.size() != n)
            fail(HR::invalid_stored_data);
        return out;
    }
    size_t occurrence_items(const std::string &h) {
        auto r = required("subathon_hold_causes", {"hold_ref", "occurrence_count"}, "hold_ref=?",
                          {bytes(h)});
        if (ref(r, 0) != h)
            fail(HR::invalid_stored_data);
        const auto n = integer(r, 1);
        collection("subathon_hold_occurrences", "hold_ref=?", {bytes(h)}, n, "ordinal");
        if (n > holds.known_occurrences_per_cause)
            fail(HR::result_limit);
        return size_t(n);
    }
    size_t resolution_items(const std::string &key) {
        auto m = mutation_meta(key);
        if (!m || m->kind != HM::resolution)
            fail(HR::invalid_stored_data);
        if (m->expected > holds.active_holds_per_result || m->entries > holds.resolution_entries)
            fail(HR::result_limit);
        size_t total = add(size_t(m->expected), size_t(m->entries));
        total = add(total, size_t(m->entries));
        keyed("subathon_hold_resolution_entries",
              {"mutation_ref", "hold_ref", "replacement_hold_ref", "replacement_detection_ref"},
              "mutation_ref=?", {bytes(key)}, "hold_ref", [&](const SqlRow &r) {
                  if (ref(r, 0) != key || null(r, 2) != null(r, 3))
                      fail(HR::invalid_stored_data);
                  if (!null(r, 2)) {
                      const auto h = ref(r, 2), d = ref(r, 3);
                      auto target = required("subathon_hold_detections", {"hold_ref"},
                                             "mutation_ref=?", {bytes(d)});
                      if (ref(target, 0) != h)
                          fail(HR::invalid_stored_data);
                      total = add(total, add(1, occurrence_items(h)));
                  }
              });
        return total;
    }
    size_t view_items(const Metadata &m) {
        auto total = occurrence_items(m.current.locator.hold_ref);
        if (m.current.locator.version_ref != m.first.locator.version_ref) {
            auto event = mutation_meta(m.current.producer_mutation_ref);
            if (!event)
                fail(HR::invalid_stored_data);
            if (event->kind == HM::resolution)
                total = add(total, resolution_items(event->ref));
        }
        return total;
    }
    std::optional<ApplicationHoldView> find_output(const std::string &h) {
        auto m = metadata(h);
        if (!m)
            return {};
        if (view_items(*m) > size_t(holds.result_items))
            fail(HR::result_limit);
        return find(h, &*m);
    }
    ApplicationHoldActiveSet active(const ApplicationHoldSubject &s) {
        auto locators = active_locators(s);
        ApplicationHoldActiveSet out{s, {}};
        size_t total = locators.size();
        for (const auto &v : locators) {
            auto m = metadata(v.hold_ref);
            if (!m)
                fail(HR::invalid_stored_data);
            total = add(total, view_items(*m));
        }
        if (total > size_t(holds.result_items))
            fail(HR::result_limit);
        out.holds.reserve(locators.size());
        for (const auto &v : locators) {
            auto h = find(v.hold_ref);
            if (!h || h->current_version.locator.version_ref != v.version_ref ||
                h->current_version.state != HS::active)
                fail(HR::invalid_stored_data);
            out.holds.push_back(std::move(*h));
        }
        return out;
    }
    ApplicationHoldHistoryResult history(const ApplicationHoldHistoryQuery &q) {
        auto m = metadata(q.hold_ref);
        if (!m)
            return {SqliteHoldHistoryDisposition::missing_hold, {}};
        if (q.checked_version_ref && *q.checked_version_ref != m->current.locator.version_ref)
            return {SqliteHoldHistoryDisposition::stale_history, {}};
        if (q.after_position != 0 &&
            !one("subathon_hold_versions", {"position"}, "hold_ref=? AND position=?",
                 {bytes(q.hold_ref), qint64(q.after_position)}))
            return {SqliteHoldHistoryDisposition::cursor_conflict, {}};
        ApplicationHoldHistoryPage page{q.hold_ref, m->current.locator.version_ref, {}, {}, false};
        auto rows =
            execute({"SELECT " + projection(hold_versions_columns) +
                         " FROM main.subathon_hold_versions WHERE hold_ref=? AND position>? ORDER "
                         "BY position LIMIT ?",
                     {bytes(q.hold_ref), qint64(q.after_position), qint64(q.page_count + 1)}});
        const auto take = std::min(q.page_count, rows.size());
        size_t total = size_t(take);
        std::optional<size_t> cause_items;
        for (qsizetype i = 0; i < take; ++i) {
            shape(rows[i], 8);
            auto v = read_version(rows[i], q.hold_ref);
            auto event = mutation_meta(v.producer_mutation_ref);
            if (!event)
                fail(HR::invalid_stored_data);
            if (event->kind == HM::detection) {
                if (!cause_items)
                    cause_items = occurrence_items(q.hold_ref);
                total = add(total, *cause_items);
            } else
                total = add(total, resolution_items(event->ref));
        }
        if (total > size_t(holds.result_items))
            fail(HR::result_limit);
        page.entries.reserve(size_t(take));
        // One immutable cause and one complete metadata audit qualify this page.
        // Each returned event still owns its own copy; no caller budget is reset.
        std::optional<ApplicationHoldCause> checked_cause;
        for (qsizetype i = 0; i < take; ++i) {
            shape(rows[i], 8);
            auto v = read_version(rows[i], q.hold_ref);
            auto event = mutation_meta(v.producer_mutation_ref);
            if (!event)
                fail(HR::invalid_stored_data);
            ApplicationHoldHistoryEntry e;
            e.version = v;
            if (event->kind == HM::detection) {
                if (!checked_cause)
                    checked_cause = full_cause(q.hold_ref);
                e.event = full_detection(event->ref, &*m, &*checked_cause);
            } else
                e.event = full_resolution(event->ref);
            page.entries.push_back(std::move(e));
        }
        page.has_more = rows.size() > q.page_count;
        if (page.has_more)
            page.next_position = page.entries.back().version.position;
        return {SqliteHoldHistoryDisposition::ready, std::move(page)};
    }
    ApplicationHoldMutationResult normal(HD d) { return {d, {}}; }
    ApplicationHoldMutationResult normal(HD d, const std::string &key, HM kind,
                                         std::vector<HV> versions) {
        return {d, ApplicationHoldMutationOutcome{key, kind, std::move(versions)}};
    }
    template <class T>
    std::optional<ApplicationHoldMutationResult> replay(const std::string &key, HM kind,
                                                        const T &request) {
        auto m = mutation_meta(key);
        if (!m)
            return {};
        if (m->kind == HM::detection) {
            auto old = full_detection(key);
            if constexpr (std::is_same_v<T, ApplicationHoldDetection>) {
                if (kind == HM::detection && hold_detail::same(old, request))
                    return normal(HD::already_recorded, key, HM::detection,
                                  {version(old.cause.hold_ref, old.request.proposed_version_ref)});
            }
        } else {
            auto old = full_resolution(key);
            if constexpr (std::is_same_v<T, ApplicationHoldResolutionBatch>) {
                if (kind == HM::resolution && hold_detail::same(old.request, request))
                    return normal(HD::already_recorded, key, HM::resolution,
                                  std::move(old.produced_versions));
            }
        }
        return normal(HD::mutation_conflict);
    }
    using Prepared = std::pair<SqlStatement, bool>;
    void append(std::vector<Prepared> &out, const QString &t, const QStringList &c, Values values) {
        out.emplace_back(insertion(t, c, std::move(values)), false);
    }
    static QVariant optional_value(const std::optional<std::string> &s) {
        return s ? QVariant(bytes(*s)) : QVariant{};
    }
    template <class T> static QVariant optional_number(const std::optional<T> &n) {
        return n ? QVariant(qint64(*n)) : QVariant{};
    }
    static QVariant optional_enum(const auto &n) { return n ? QVariant(int(*n)) : QVariant{}; }
    static QVariant id_value(const std::optional<ApplicationPlanKey> &p) {
        return p ? QVariant(encode_plan_revision_uint64(p->revision)) : QVariant{};
    }
    void prepare_observation(std::vector<Prepared> &out, const QString &t, const QStringList &cols,
                             const std::string &key, const ApplicationHoldObservation &o) {
        append(out, t, cols,
               {bytes(key), optional_value(o.persisted_state_token), bytes(o.annotation.format_ref),
                o.annotation.document, optional_value(o.policy_revision_ref),
                optional_value(o.permission_revision_ref), optional_value(o.actor_ref),
                optional_value(o.decision_ref), optional_number(o.observed_utc_ms),
                optional_number(o.elapsed_ms)});
    }
    void prepare_detection_registry(std::vector<Prepared> &out, const ApplicationHoldDetection &d) {
        Values v{bytes(d.request.mutation_ref),
                 0,
                 tag(d.cause.source.subject),
                 encode_plan_revision_uint64(subject_id(d.cause.source.subject)),
                 qint64(0),
                 qint64(1)};
        while (v.size() < hold_mutations_columns.size())
            v.append(QVariant{});
        append(out, "subathon_hold_mutations", hold_mutations_columns, std::move(v));
    }
    void prepare_cause(std::vector<Prepared> &out, const ApplicationHoldCause &c,
                       const std::string &first) {
        const auto &s = c.source;
        const auto &p = c.plan_evidence;
        Values v{bytes(c.hold_ref),
                 tag(s.subject),
                 encode_plan_revision_uint64(subject_id(s.subject)),
                 tag(s.subject) == 0 ? QVariant(encode_plan_revision_uint64(subject_id(s.subject)))
                                     : QVariant{},
                 encode_plan_revision_uint64(s.receipt_id.value),
                 optional_value(s.event_ref),
                 optional_value(s.parent_event_ref),
                 optional_value(s.root_event_ref),
                 bytes(s.configuration_ref),
                 bytes(s.arrival_context_ref),
                 bytes(c.command.operation.operation_scope),
                 bytes(c.command.operation.operation_ref),
                 bytes(c.command.intent.format_ref),
                 c.command.intent.document,
                 int(c.kind),
                 optional_value(c.supersedes_hold_ref),
                 bytes(first),
                 int(p.kind),
                 p.plan ? QVariant(bytes(p.plan->plan_ref)) : QVariant{},
                 id_value(p.plan),
                 int(p.extent),
                 optional_enum(p.stopped_for),
                 p.failed_capture ? QVariant(bytes(p.failed_capture->format_ref)) : QVariant{},
                 p.failed_capture ? QVariant(p.failed_capture->document) : QVariant{},
                 qint64(p.occurrences.size())};
        if (c.failure) {
            const auto &f = *c.failure;
            v.append(int(f.stage));
            v.append(optional_number(f.failing_ordinal));
            v.append(optional_value(f.failing_target_ref));
            if (f.attempted_units) {
                auto e = encode_plan_exact(*f.attempted_units, limits_.component_bytes);
                v.append(e.numerator);
                v.append(e.denominator);
            } else {
                v.append(QVariant{});
                v.append(QVariant{});
            }
            v.append(bytes(f.annotation.format_ref));
            v.append(f.annotation.document);
        } else
            for (int n = 0; n < 7; ++n)
                v.append(QVariant{});
        if (c.timer) {
            const auto &t = *c.timer;
            v.append(qint64(t.request_ordinal));
            v.append(int(t.component_kind));
            v.append(qint64(t.request_units));
            v.append(int(t.deficit_policy));
            v.append(int(t.deficit_disposition));
            v.append(qint64(t.unapplied_units));
            v.append(optional_enum(t.composition));
            v.append(bytes(t.composition_decision.format_ref));
            v.append(t.composition_decision.document);
        } else
            for (int n = 0; n < 9; ++n)
                v.append(QVariant{});
        v.append(optional_value(c.missing_policy_reason));
        if (c.timer && c.timer->associated) {
            const auto &a = *c.timer->associated;
            v.append(bytes(a.operation.operation_scope));
            v.append(bytes(a.operation.operation_ref));
            v.append(qint64(a.request_ordinal));
        } else
            for (int n = 0; n < 3; ++n)
                v.append(QVariant{});
        append(out, "subathon_hold_causes", hold_causes_columns, std::move(v));
        prepare_observation(out, "subathon_hold_cause_observations",
                            hold_cause_observations_columns, c.hold_ref, c.first_observation);
        for (const auto &o : p.occurrences) {
            Values values{bytes(c.hold_ref),
                          qint64(o.ordinal),
                          optional_value(o.action_ref),
                          optional_value(o.action_version_ref),
                          optional_value(o.rule_ref),
                          optional_value(o.target_ref),
                          optional_enum(o.unit)};
            if (o.attempted_units) {
                auto e = encode_plan_exact(*o.attempted_units, limits_.component_bytes);
                values.append(e.numerator);
                values.append(e.denominator);
            } else {
                values.append(QVariant{});
                values.append(QVariant{});
            }
            values.append(optional_number(o.finalized_units));
            values.append(optional_value(o.explanation));
            append(out, "subathon_hold_occurrences", hold_occurrences_columns, std::move(values));
        }
    }
    void prepare_detection(std::vector<Prepared> &out, const ApplicationHoldDetection &d) {
        const auto &r = d.request;
        append(out, "subathon_hold_detections", hold_detections_columns,
               {bytes(r.mutation_ref), bytes(d.cause.hold_ref),
                optional_value(r.expected_version_ref), bytes(r.proposed_version_ref),
                bytes(r.audit.format_ref), r.audit.document});
        prepare_observation(out, "subathon_hold_detection_observations",
                            hold_detection_observations_columns, r.mutation_ref, r.observation);
    }
    void prepare_version(std::vector<Prepared> &out, const HV &v) {
        append(out, "subathon_hold_versions", hold_versions_columns,
               {bytes(v.locator.hold_ref), bytes(v.locator.version_ref), qint64(v.position),
                optional_value(v.parent_version_ref), int(v.state), bytes(v.producer_mutation_ref),
                v.replacement ? QVariant(bytes(v.replacement->hold_ref)) : QVariant{},
                v.replacement ? QVariant(bytes(v.replacement->detection_ref)) : QVariant{}});
    }
    void prepare_head(std::vector<Prepared> &out, const HV &v,
                      const std::optional<Metadata> &previous) {
        if (previous)
            out.emplace_back(
                SqlStatement{
                    "UPDATE main.subathon_hold_heads SET version_ref=?,version_count=? WHERE "
                    "hold_ref=? AND version_ref=? AND version_count=?",
                    {bytes(v.locator.version_ref), qint64(v.position), bytes(v.locator.hold_ref),
                     bytes(previous->current.locator.version_ref), qint64(previous->count)}},
                false);
        else
            append(out, "subathon_hold_heads", hold_heads_columns,
                   {bytes(v.locator.hold_ref), bytes(v.locator.version_ref), qint64(v.position)});
        out.emplace_back(SqlStatement{"SELECT changes()", {}}, true);
    }
    void write(std::vector<Prepared> out) {
        wrote_ = true;
        for (const auto &[statement, check] : out) {
            const auto rows = execute(statement);
            if (check && (rows.size() != 1 || rows.front().size() != 1 ||
                          (rows.front().front().metaType().id() != QMetaType::Int &&
                           rows.front().front().metaType().id() != QMetaType::LongLong) ||
                          rows.front().front().toLongLong() != 1))
                fail(HR::invalid_stored_data);
        }
    }
    void incoming_detection_references(const ApplicationHoldDetection &d) {
        source_references(d.cause, true);
        known_state(d.request.observation.persisted_state_token, true);
    }
    ApplicationHoldMutationResult record(const ApplicationHoldDetection &d) {
        if (auto r = replay(d.request.mutation_ref, HM::detection, d))
            return *r;
        auto old = metadata(d.cause.hold_ref);
        if (old) {
            auto selected = find(d.cause.hold_ref);
            if (!selected)
                fail(HR::invalid_stored_data);
            if (old->current.state == HS::resolved)
                return normal(HD::closed_hold);
            if (!hold_detail::same(selected->cause, d.cause))
                return normal(HD::hold_conflict);
            if (d.request.expected_version_ref != std::optional{old->current.locator.version_ref})
                return normal(HD::stale_version);
            if (one("subathon_hold_versions", {"version_ref"}, "hold_ref=? AND version_ref=?",
                    {bytes(d.cause.hold_ref), bytes(d.request.proposed_version_ref)}))
                return normal(HD::version_conflict);
        } else if (d.request.expected_version_ref)
            return normal(HD::stale_version);
        if (d.cause.supersedes_hold_ref) {
            auto before = metadata(*d.cause.supersedes_hold_ref);
            if (!before)
                reference_failure(true, true);
            if (!hold_detail::same(before->anchor.subject, d.cause.source.subject))
                reference_failure(true);
            if (before->current.state == HS::active)
                return normal(HD::hold_conflict);
        }
        incoming_detection_references(d);
        if (old && old->count == std::numeric_limits<qint64>::max())
            fail(HR::sequence_exhausted);
        HV v{{d.cause.hold_ref, d.request.proposed_version_ref},
             old ? old->count + 1 : 1,
             d.request.expected_version_ref,
             HS::active,
             d.request.mutation_ref,
             {}};
        auto result = normal(HD::recorded, d.request.mutation_ref, HM::detection, {v});
        returned(result);
        std::vector<Prepared> out;
        prepare_detection_registry(out, d);
        if (!old)
            prepare_cause(out, d.cause, d.request.mutation_ref);
        prepare_detection(out, d);
        prepare_version(out, v);
        prepare_head(out, v, old);
        write(std::move(out));
        return result;
    }
    void prepare_resolution_registry(std::vector<Prepared> &out,
                                     const ApplicationHoldResolutionBatch &b) {
        Values v{bytes(b.mutation_ref),
                 1,
                 tag(b.subject),
                 encode_plan_revision_uint64(subject_id(b.subject)),
                 qint64(b.expected_active.size()),
                 qint64(b.entries.size()),
                 bytes(b.actor_ref),
                 bytes(b.decision_ref),
                 bytes(b.reason),
                 bytes(b.options.format_ref),
                 b.options.document};
        if (b.application) {
            const auto &a = *b.application;
            v.append(int(a.kind));
            v.append(bytes(a.operation.operation_scope));
            v.append(bytes(a.operation.operation_ref));
            v.append(encode_plan_revision_uint64(a.receipt_id.value));
            v.append(bytes(a.plan.plan_ref));
            v.append(encode_plan_revision_uint64(a.plan.revision));
            if (a.held_origin) {
                const auto &h = *a.held_origin;
                v.append(bytes(h.version.hold_ref));
                v.append(bytes(h.version.version_ref));
                v.append(qint64(h.request_ordinal));
                v.append(int(h.component_kind));
                v.append(optional_number(h.remaining_units));
            } else
                for (int n = 0; n < 5; ++n)
                    v.append(QVariant{});
        } else
            for (int n = 0; n < 11; ++n)
                v.append(QVariant{});
        append(out, "subathon_hold_mutations", hold_mutations_columns, std::move(v));
    }
    ApplicationHoldMutationResult resolve(const ApplicationHoldResolutionBatch &b) {
        if (auto r = replay(b.mutation_ref, HM::resolution, b))
            return *r;
        auto active = active_locators(b.subject);
        if (active.size() != b.expected_active.size() ||
            !std::equal(active.begin(), active.end(), b.expected_active.begin(),
                        [](const auto &a, const auto &v) {
                            return a.hold_ref == v.hold_ref && a.version_ref == v.version_ref;
                        }))
            return normal(HD::stale_active_set);
        std::vector<std::optional<Metadata>> previous;
        previous.reserve(b.entries.size());
        std::vector<HV> produced;
        produced.reserve(add(b.entries.size(), b.entries.size()));
        for (const auto &e : b.entries) {
            auto m = metadata(e.hold_ref);
            if (!m)
                return normal(HD::missing_hold);
            if (m->current.state == HS::resolved)
                return normal(HD::closed_hold);
            if (m->current.locator.version_ref != e.expected_version_ref)
                return normal(HD::stale_version);
            if (one("subathon_hold_versions", {"version_ref"}, "hold_ref=? AND version_ref=?",
                    {bytes(e.hold_ref), bytes(e.proposed_version_ref)}))
                return normal(HD::version_conflict);
            if (!hold_detail::same(m->anchor.subject, b.subject))
                reference_failure(true);
            if (m->count == std::numeric_limits<qint64>::max())
                fail(HR::sequence_exhausted);
            previous.push_back(m);
            HV v{{e.hold_ref, e.proposed_version_ref},
                 m->count + 1,
                 e.expected_version_ref,
                 HS::resolved,
                 b.mutation_ref,
                 {}};
            if (e.replacement)
                v.replacement = ApplicationHoldReplacementLocator{
                    e.replacement->cause.hold_ref, e.replacement->request.mutation_ref};
            produced.push_back(std::move(v));
            if (e.replacement)
                produced.push_back(
                    {{e.replacement->cause.hold_ref, e.replacement->request.proposed_version_ref},
                     1,
                     {},
                     HS::active,
                     e.replacement->request.mutation_ref,
                     {}});
        }
        for (const auto &e : b.entries)
            if (e.replacement) {
                const auto &d = *e.replacement;
                if (auto old = mutation_meta(d.request.mutation_ref)) {
                    if (old->kind == HM::detection)
                        (void)full_detection(old->ref);
                    else
                        (void)full_resolution(old->ref);
                    return normal(old->kind == HM::resolution ? HD::mutation_conflict
                                                              : HD::replacement_conflict);
                }
                if (metadata(d.cause.hold_ref))
                    return normal(HD::replacement_conflict);
            }
        if (b.application) {
            if (b.entries.size() != active.size() ||
                std::any_of(b.entries.begin(), b.entries.end(),
                            [](const auto &e) { return bool(e.replacement); }) ||
                std::any_of(active.begin(), active.end(), [&](const auto &x) {
                    return std::none_of(b.entries.begin(), b.entries.end(),
                                        [&](const auto &e) { return x.hold_ref == e.hold_ref; });
                }))
                reference_failure(true);
        }
        for (const auto &e : b.entries)
            if (e.replacement)
                incoming_detection_references(*e.replacement);
        application_link(b, true);
        auto result = normal(HD::recorded, b.mutation_ref, HM::resolution, produced);
        returned(result);
        std::vector<Prepared> out;
        prepare_resolution_registry(out, b);
        for (const auto &e : b.entries)
            if (e.replacement)
                prepare_detection_registry(out, *e.replacement);
        for (const auto &e : b.entries)
            if (e.replacement) {
                prepare_cause(out, e.replacement->cause, e.replacement->request.mutation_ref);
                prepare_detection(out, *e.replacement);
            }
        // Replacement initial versions exist before old resolved versions name them.
        for (const auto &v : produced)
            if (v.state == HS::active)
                prepare_version(out, v);
        for (const auto &v : produced)
            if (v.state == HS::resolved)
                prepare_version(out, v);
        for (const auto &e : b.expected_active)
            append(out, "subathon_hold_resolution_expected", hold_resolution_expected_columns,
                   {bytes(b.mutation_ref), bytes(e.hold_ref), bytes(e.version_ref)});
        for (const auto &e : b.entries)
            append(out, "subathon_hold_resolution_entries", hold_resolution_entries_columns,
                   {bytes(b.mutation_ref), bytes(e.hold_ref), bytes(e.expected_version_ref),
                    bytes(e.proposed_version_ref), bytes(e.decision.format_ref),
                    e.decision.document,
                    e.replacement ? QVariant(bytes(e.replacement->cause.hold_ref)) : QVariant{},
                    e.replacement ? QVariant(bytes(e.replacement->request.mutation_ref))
                                  : QVariant{}});
        size_t index = 0;
        for (size_t i = 0; i < b.entries.size(); ++i) {
            prepare_head(out, produced[index++], previous[i]);
            if (b.entries[i].replacement)
                prepare_head(out, produced[index++], {});
        }
        write(std::move(out));
        return result;
    }
};
} // namespace
MigrationManifest application_hold_repository_manifest() { return hold_detail::make_manifest(); }
SqliteApplicationHoldStore::SqliteApplicationHoldStore(SqliteWorkSession s, SqliteHoldLimits l)
    : session_(std::move(s)), limits_(l) {}
std::optional<ApplicationHoldView> SqliteApplicationHoldStore::find(const std::string &h) const {
    hold_detail::validate_find(h, limits_);
    HoldAccess a(session_, limits_);
    return a.guarded([&] {
        auto v = a.find_output(h);
        a.returned(v);
        return v;
    });
}
ApplicationHoldActiveSet SqliteApplicationHoldStore::active(const ApplicationHoldSubject &s) const {
    hold_detail::validate_active(s, limits_);
    HoldAccess a(session_, limits_);
    return a.guarded([&] {
        auto v = a.active(s);
        a.returned(v);
        return v;
    });
}
ApplicationHoldHistoryResult
SqliteApplicationHoldStore::history(const ApplicationHoldHistoryQuery &q) const {
    hold_detail::validate_history(q, limits_);
    HoldAccess a(session_, limits_);
    return a.guarded([&] {
        auto v = a.history(q);
        a.returned(v);
        return v;
    });
}
ApplicationHoldMutationResult
SqliteApplicationHoldStore::record_detection(const ApplicationHoldDetection &d) const {
    hold_detail::validate_detection(d, limits_);
    HoldAccess a(session_, limits_);
    return a.guarded([&] {
        auto v = a.record(d);
        a.returned(v);
        return v;
    });
}
ApplicationHoldMutationResult
SqliteApplicationHoldStore::stage_resolution(const ApplicationHoldResolutionBatch &b) const {
    hold_detail::validate_resolution(b, limits_);
    HoldAccess a(session_, limits_);
    return a.guarded([&] {
        auto v = a.resolve(b);
        a.returned(v);
        return v;
    });
}
} // namespace subathon::server
