// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Independent frozen foundation: raw SQL/old primitives and new owned DTOs only.
#include "sqlite_original_claim_fixture.hpp"
#include <subathon/server/sqlite_application_holds.hpp>

namespace application_hold_fixture {
namespace core = subathon::core;
using namespace subathon::server;
using original_claim_fixture::blob;
using original_claim_fixture::check_rows;
using original_claim_fixture::execute;
using original_claim_fixture::limits;
using original_claim_fixture::Observer;
using original_claim_fixture::require;
using original_claim_fixture::run;
inline std::string text(const QJsonValue &v) {
    const auto bytes = v.toString().toUtf8();
    return {bytes.constData(), size_t(bytes.size())};
}
inline std::optional<std::string> optional_text(const QJsonValue &v) {
    return v.isNull() || v.isUndefined() ? std::nullopt : std::optional{text(v)};
}
inline std::int64_t integer(const QJsonValue &v) {
    bool ok = false;
    const auto n = v.toString().toLongLong(&ok);
    require(ok, "Frozen signed64 decimal string");
    return n;
}
inline std::uint64_t identifier(const QJsonValue &v) {
    bool ok = false;
    const auto n = v.toString().toULongLong(&ok);
    require(ok, "Frozen full uint64 decimal string");
    return n;
}
inline ApplicationEnvelope envelope(const QJsonValue &v) {
    const auto o = v.toObject();
    const auto hex = o["document_hex"].toString().toLatin1();
    auto document = QByteArray::fromHex(hex);
    require(document.toHex() == hex, "Exact frozen envelope hex");
    if (document.isEmpty())
        document = QByteArray("", 0);
    return {text(o["format"]), document};
}
inline ApplicationOperationKey operation(const QJsonValue &v) {
    const auto a = v.toArray();
    return {text(a[0]), text(a[1])};
}
inline ApplicationPlanKey plan_key(const QJsonValue &v) {
    const auto a = v.toArray();
    return {text(a[0]), identifier(a[1])};
}
inline core::Exact exact(const QJsonValue &v) {
    const auto a = v.toArray();
    return core::Exact(text(a[0])) / core::Exact(text(a[1]));
}
inline ApplicationHoldSubject subject(const QJsonValue &v) {
    const auto o = v.toObject();
    ApplicationHoldSubject s;
    if (o["kind"].toInt() == 0)
        s.id = core::ContributionId{identifier(o["id"])};
    else
        s.id = core::ReceiptId{identifier(o["id"])};
    return s;
}
inline ApplicationHoldObservation observation(const QJsonValue &v) {
    const auto o = v.toObject();
    ApplicationHoldObservation r;
    r.persisted_state_token = optional_text(o["token"]);
    r.annotation = envelope(o["annotation"]);
    r.policy_revision_ref = optional_text(o["policy"]);
    r.permission_revision_ref = optional_text(o["permission"]);
    r.actor_ref = optional_text(o["actor"]);
    r.decision_ref = optional_text(o["decision"]);
    if (!o["utc"].isNull())
        r.observed_utc_ms = integer(o["utc"]);
    if (!o["elapsed"].isNull())
        r.elapsed_ms = integer(o["elapsed"]);
    return r;
}
inline ApplicationHoldProposedOccurrence occurrence(const QJsonValue &v) {
    const auto o = v.toObject();
    ApplicationHoldProposedOccurrence r;
    r.ordinal = integer(o["ordinal"]);
    r.action_ref = optional_text(o["action"]);
    r.action_version_ref = optional_text(o["action_version"]);
    r.rule_ref = optional_text(o["rule"]);
    r.target_ref = optional_text(o["target"]);
    if (!o["unit"].isNull())
        r.unit = static_cast<core::RequestedUnit>(o["unit"].toInt());
    if (!o["exact"].isNull())
        r.attempted_units = exact(o["exact"]);
    if (!o["final"].isNull())
        r.finalized_units = integer(o["final"]);
    r.explanation = optional_text(o["explanation"]);
    return r;
}
inline ApplicationHoldVersion version(const QJsonValue &v) {
    const auto o = v.toObject();
    ApplicationHoldVersion r;
    r.locator = {text(o["hold"]), text(o["version"])};
    r.position = integer(o["position"]);
    r.parent_version_ref = optional_text(o["parent"]);
    r.state = static_cast<ApplicationHoldState>(o["state"].toInt());
    r.producer_mutation_ref = text(o["producer"]);
    if (!o["replacement"].isNull()) {
        const auto a = o["replacement"].toArray();
        r.replacement = ApplicationHoldReplacementLocator{text(a[0]), text(a[1])};
    }
    return r;
}
inline ApplicationHoldApplicationLink application(const QJsonValue &v) {
    const auto o = v.toObject();
    ApplicationHoldApplicationLink r;
    r.kind = static_cast<ApplicationHoldApplicationLinkKind>(o["kind"].toInt());
    r.operation = operation(o["operation"]);
    r.receipt_id = {identifier(o["receipt"])};
    r.plan = plan_key(o["plan"]);
    if (!o["held"].isNull()) {
        const auto h = o["held"].toObject();
        ApplicationHoldHeldOrigin origin;
        origin.version = {text(h["hold"]), text(h["version"])};
        origin.request_ordinal = integer(h["ordinal"]);
        origin.component_kind = static_cast<ApplicationComponentKind>(h["component"].toInt());
        if (!h["remaining"].isNull())
            origin.remaining_units = integer(h["remaining"]);
        r.held_origin = origin;
    }
    return r;
}
struct Data {
    original_claim_fixture::Data old;
    QJsonObject frozen{original_claim_fixture::load(SUBATHON_HOLD_VECTOR_PATH)};
    void schema(const SqliteWorkSession &s) const {
        old.schema(s);
        const auto m = frozen["manifest"].toObject();
        for (const auto &sql : m["sql"].toArray())
            (void)s.execute({sql.toString(), {}});
        (void)s.execute(
            {"INSERT INTO main.subathon_migration_history VALUES(?,?,?)",
             {6, m["id"].toString(), QByteArray::fromHex(m["sha256_hex"].toString().toLatin1())}});
    }
    void populate(const SqliteWorkSession &s, const char *arrangement) const {
        schema(s);
        execute(s, frozen["arrangements"].toObject()[arrangement].toObject()["setup"].toArray());
    }
    ApplicationHoldCause cause(const char *name) const {
        const auto c = frozen["causes"].toObject()[name].toObject();
        const auto so = c["source"].toObject(), p = c["plan"].toObject();
        ApplicationHoldCause r;
        r.hold_ref = text(c["hold"]);
        r.source = {subject(so["subject"]),     {identifier(so["receipt"])},
                    optional_text(so["event"]), optional_text(so["parent"]),
                    optional_text(so["root"]),  text(so["configuration"]),
                    text(so["context"])};
        const auto cmd = c["command"].toObject();
        r.command = {operation(cmd["operation"]), envelope(cmd["intent"])};
        r.first_observation = observation(c["observation"]);
        r.plan_evidence.kind = static_cast<ApplicationHoldPlanKind>(p["kind"].toInt());
        if (!p["key"].isNull())
            r.plan_evidence.plan = plan_key(p["key"]);
        r.plan_evidence.extent = static_cast<ApplicationHoldExtent>(p["extent"].toInt());
        if (!p["stopped"].isNull())
            r.plan_evidence.stopped_for =
                static_cast<ApplicationHoldStoppedFor>(p["stopped"].toInt());
        if (!p["failed"].isNull())
            r.plan_evidence.failed_capture = envelope(p["failed"]);
        for (const auto &o : p["occurrences"].toArray())
            r.plan_evidence.occurrences.push_back(occurrence(o));
        r.kind = static_cast<ApplicationHoldCauseKind>(c["kind"].toInt());
        if (!c["failure"].isNull()) {
            const auto f = c["failure"].toObject();
            ApplicationHoldFailureEvidence e;
            e.stage = static_cast<ApplicationHoldFailureStage>(f["stage"].toInt());
            if (!f["ordinal"].isNull())
                e.failing_ordinal = integer(f["ordinal"]);
            e.failing_target_ref = optional_text(f["target"]);
            if (!f["exact"].isNull())
                e.attempted_units = exact(f["exact"]);
            e.annotation = envelope(f["annotation"]);
            r.failure = e;
        }
        if (!c["timer"].isNull()) {
            const auto t = c["timer"].toObject();
            ApplicationHoldTimerEvidence e;
            e.request_ordinal = integer(t["ordinal"]);
            e.component_kind = static_cast<ApplicationComponentKind>(t["component"].toInt());
            e.request_units = integer(t["request"]);
            e.deficit_policy = static_cast<core::RefundDeficitPolicy>(t["policy"].toInt());
            e.deficit_disposition =
                static_cast<core::TimeReversalDisposition>(t["disposition"].toInt());
            e.unapplied_units = integer(t["unapplied"]);
            e.composition = static_cast<ApplicationHoldComposition>(t["composition"].toInt());
            e.composition_decision = envelope(t["decision"]);
            if (!t["associated"].isNull()) {
                const auto a = t["associated"].toObject();
                e.associated = ApplicationHoldAssociatedApplication{operation(a["operation"]),
                                                                    integer(a["ordinal"])};
            }
            r.timer = e;
        }
        r.missing_policy_reason = optional_text(c["missing_policy"]);
        r.supersedes_hold_ref = optional_text(c["supersedes"]);
        return r;
    }
    ApplicationHoldDetectionData detection_data(const char *name) const {
        const auto d = frozen["detections"].toObject()[name].toObject();
        return {text(d["mutation"]), optional_text(d["expected"]), text(d["proposed"]),
                observation(d["observation"]), envelope(d["audit"])};
    }
    ApplicationHoldDetection detection(const char *name) const {
        const auto id =
            frozen["detections"].toObject()[name].toObject()["cause_id"].toString().toUtf8();
        return {cause(id.constData()), detection_data(name)};
    }
    ApplicationHoldResolutionBatch batch(const QJsonObject &r) const {
        ApplicationHoldResolutionBatch b;
        b.mutation_ref = text(r["mutation"]);
        b.subject = subject(r["subject"]);
        for (const auto &x : r["expected"].toArray()) {
            const auto a = x.toArray();
            b.expected_active.push_back({text(a[0]), text(a[1])});
        }
        for (const auto &x : r["entries"].toArray()) {
            const auto e = x.toObject();
            ApplicationHoldResolutionEntry entry{text(e["hold"]),
                                                 text(e["expected"]),
                                                 text(e["proposed"]),
                                                 envelope(e["decision"]),
                                                 {}};
            if (!e["replacement"].isNull()) {
                const auto id = e["replacement"].toString().toUtf8();
                entry.replacement = detection(id.constData());
            }
            b.entries.push_back(std::move(entry));
        }
        b.actor_ref = text(r["actor"]);
        b.decision_ref = text(r["decision"]);
        b.reason = text(r["reason"]);
        b.options = envelope(r["options"]);
        if (!r["application"].isNull())
            b.application = application(r["application"]);
        return b;
    }
    ApplicationHoldRecordedResolution resolution(const char *name) const {
        const auto r = frozen["resolutions"].toObject()[name].toObject();
        ApplicationHoldRecordedResolution result;
        result.request = batch(r["request"].toObject());
        for (const auto &v : r["produced"].toArray())
            result.produced_versions.push_back(version(v));
        return result;
    }
    ApplicationHoldResolutionBatch proposal(const char *name) const {
        return batch(frozen["proposals"].toObject()[name].toObject());
    }
    ApplicationHoldView view(const char *name) const {
        const auto v = frozen["views"].toObject()[name].toObject();
        const auto d = v["first_detection"].toString().toUtf8();
        ApplicationHoldView result{cause(name),
                                   detection_data(d.constData()),
                                   version(v["first_version"]),
                                   version(v["current_version"]),
                                   {}};
        if (!v["current_event"].isNull()) {
            const auto e = v["current_event"].toObject();
            if (e["kind"].toInt() == 0) {
                const auto id = e["detection"].toString().toUtf8();
                result.current_event = detection_data(id.constData());
            } else {
                const auto id = e["resolution"].toString().toUtf8();
                result.current_event = resolution(id.constData());
            }
        }
        return result;
    }
    ApplicationHoldHistoryPage history_page(const char *name) const {
        const auto p = frozen["history_pages"].toObject()[name].toObject();
        ApplicationHoldHistoryPage result;
        result.hold_ref = text(p["hold"]);
        result.checked_version_ref = text(p["checked"]);
        for (const auto &value : p["entries"].toArray()) {
            const auto e = value.toObject();
            const auto id = e["event"].toString().toUtf8();
            ApplicationHoldHistoryEntry entry;
            entry.version = version(e["version"]);
            if (e["kind"].toInt() == 0)
                entry.event = detection(id.constData());
            else
                entry.event = resolution(id.constData());
            result.entries.push_back(std::move(entry));
        }
        if (!p["next"].isNull())
            result.next_position = integer(p["next"]);
        result.has_more = p["has_more"].toBool();
        return result;
    }
    QJsonObject oracle(const char *name) const {
        return frozen["byte_item_oracles"].toObject()[name].toObject();
    }
};
} // namespace application_hold_fixture
