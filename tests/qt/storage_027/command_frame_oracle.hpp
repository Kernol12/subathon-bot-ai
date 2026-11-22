// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "missing_storage.hpp"

#include <QByteArray>
#include <QtEndian>

#include <algorithm>
#include <limits>

namespace storage_027_fixture::frame_oracle {

enum class Kind : quint8 { replace = 1, stop = 2, resume = 3 };

struct Bytes {
    QByteArray value;
    void tag(quint8 n) { value.append(static_cast<char>(n)); }
    template <class U> void big(U n) {
        const auto bytes = qToBigEndian(n);
        value.append(reinterpret_cast<const char *>(&bytes), sizeof bytes);
    }
    void count(qsizetype n) {
        if (n < 0 || n > std::numeric_limits<quint32>::max())
            throw std::invalid_argument("Fixture frame count overflow");
        big(static_cast<quint32>(n));
    }
    void number(qint64 n) { big(static_cast<quint64>(n)); }
    void document(QByteArrayView v) {
        count(v.size());
        value.append(v.data(), v.size());
    }
    void ref(QByteArrayView r) {
        if (r.isEmpty() || r.size() > 4096)
            throw std::invalid_argument("Fixture reference outside exact opaque BLOB bound");
        document(r);
    }
    void ref(const std::string &r) { ref(QByteArrayView(r.data(), r.size())); }
    void version(const QByteArray &ref_value) {
        tag(ref_value.isEmpty() ? 0 : 1);
        if (!ref_value.isEmpty())
            ref(ref_value);
    }
};

inline void scope(Bytes &out, const MilestoneTrackKey &track) {
    out.ref(track.installation_ref);
    out.ref(track.track_ref);
}

inline void occurrence(Bytes &out, const MilestoneTrackKey &outer, const Occurrence &target) {
    if (outer != target.track)
        throw std::invalid_argument("Fixture target scope mismatch");
    if (const auto *app = std::get_if<ApplicationOperationKey>(&target.origin)) {
        out.tag(1);
        out.ref(app->operation_scope);
        out.ref(app->operation_ref);
    } else {
        const auto &edit = std::get<MilestoneEditKey>(target.origin);
        if (edit.track != outer)
            throw std::invalid_argument("Fixture edit scope mismatch");
        out.tag(2);
        out.ref(edit.operation_ref);
    }
    out.ref(target.occurrence.ref);
}

struct Capture {
    MilestoneAction action;
    QByteArray automation;
    std::string route, sender;
    std::optional<qint64> bonus;
};

inline void capture(Bytes &out, const Capture &v) {
    const bool is_bonus = v.action.kind == MilestoneActionKind::bonus;
    if (is_bonus != v.bonus.has_value())
        throw std::invalid_argument("Fixture capture type mismatch");
    out.tag(is_bonus ? 2 : 1);
    out.ref(v.action.action_ref);
    out.ref(v.action.version_ref);
    if (is_bonus) {
        out.number(*v.bonus);
    } else {
        out.document(v.automation);
        out.ref(v.route);
        out.ref(v.sender);
    }
}

struct PreviewMember {
    qint64 ordinal;
    QByteArray prior_version;
    qint64 execution_revision;
    Capture before, after;
};
struct HoldWitness {
    quint8 kind;
    QByteArray cause, revision;
};
struct PreviewWitness {
    QByteArray grant_revision, configuration_revision, capability_revision, queue_revision;
    bool released;
    std::vector<HoldWitness> holds;
    std::vector<PreviewMember> members;
};

inline void preview(Bytes &out, const ReplaceIntent &wanted, PreviewWitness witness) {
    out.ref(witness.grant_revision);
    out.ref(witness.configuration_revision);
    out.ref(witness.capability_revision);
    out.ref(witness.queue_revision);
    out.tag(witness.released ? 1 : 0);
    std::ranges::sort(witness.holds, {}, [](const auto &h) { return std::tie(h.kind, h.cause); });
    out.count(qsizetype(witness.holds.size()));
    for (const auto &h : witness.holds) {
        out.tag(h.kind);
        out.ref(h.cause);
        out.ref(h.revision);
    }
    std::ranges::sort(witness.members, {}, &PreviewMember::ordinal);
    auto requested = wanted.patch;
    std::ranges::sort(requested, {}, &Replacement::ordinal);
    if (requested.empty() || requested.size() != witness.members.size())
        throw std::invalid_argument("Fixture replacement coverage mismatch");
    out.count(qsizetype(requested.size()));
    qint64 last = 0;
    for (std::size_t i = 0; i < requested.size(); ++i) {
        const auto &slot = witness.members[i];
        if (slot.ordinal <= last || slot.ordinal != requested[i].ordinal ||
            slot.after.action != requested[i].action ||
            slot.before.action.kind != slot.after.action.kind ||
            slot.before.action == slot.after.action)
            throw std::invalid_argument("Fixture preview has no exact changed same-kind selection");
        last = slot.ordinal;
        out.number(slot.ordinal);
        out.version(slot.prior_version);
        out.number(slot.execution_revision);
        capture(out, slot.before);
        capture(out, slot.after);
    }
}

inline QByteArray intent(const Command &cmd, const PreviewWitness *bound = nullptr) {
    Bytes out{"MSI1"};
    if (const auto *stop = std::get_if<StopIntent>(&cmd.intent)) {
        out.tag(static_cast<quint8>(Kind::stop));
        scope(out, cmd.track);
        out.ref(cmd.operation);
        occurrence(out, cmd.track, stop->target);
        switch (stop->choice.value_or(QueueChoice::hold)) {
        case QueueChoice::hold:
            out.tag(1);
            break;
        case QueueChoice::continue_queue:
            out.tag(2);
            break;
        default:
            throw std::invalid_argument("Fixture Stop choice is not admitted");
        }
    } else if (const auto *resume = std::get_if<ResumeIntent>(&cmd.intent)) {
        out.tag(static_cast<quint8>(Kind::resume));
        scope(out, cmd.track);
        out.ref(cmd.operation);
        std::vector<QByteArray> refs;
        for (const auto &cause : resume->causes) {
            if (cause.track != cmd.track || std::ranges::find(refs, cause.ref) != refs.end())
                throw std::invalid_argument("Fixture Resume has duplicate or foreign locator");
            refs.push_back(cause.ref);
        }
        if (refs.empty())
            throw std::invalid_argument("Fixture Resume is empty");
        std::ranges::sort(refs);
        out.count(qsizetype(refs.size()));
        for (const auto &ref : refs)
            out.ref(ref);
    } else {
        if (!bound)
            throw std::invalid_argument("Fixture Replace preview is absent");
        const auto &replace = std::get<ReplaceIntent>(cmd.intent);
        out.tag(static_cast<quint8>(Kind::replace));
        scope(out, cmd.track);
        out.ref(cmd.operation);
        occurrence(out, cmd.track, replace.target);
        preview(out, replace, *bound);
    }
    return out.value;
}

inline void references(Bytes &out, std::vector<QByteArray> values) {
    std::ranges::sort(values);
    if (std::adjacent_find(values.begin(), values.end()) != values.end())
        throw std::invalid_argument("Fixture duplicate result reference");
    out.count(qsizetype(values.size()));
    for (const auto &ref : values)
        out.ref(ref);
}

inline void versions(Bytes &out, std::vector<SelectedVersion> values) {
    std::ranges::sort(values, {}, &SelectedVersion::ordinal);
    out.count(qsizetype(values.size()));
    qint64 last = 0;
    for (const auto &version : values) {
        if (version.ordinal <= last)
            throw std::invalid_argument("Fixture duplicate result slot");
        last = version.ordinal;
        out.number(version.ordinal);
        out.version(version.version);
    }
}

inline QByteArray result(Kind kind, const Decision &saved) {
    // A journal stores the success payload. Replay is a current observed
    // disposition; it never changes the canonical saved payload.
    Bytes out{"MSR1"};
    out.tag(static_cast<quint8>(kind));
    switch (kind) {
    case Kind::replace:
        versions(out, saved.versions);
        break;
    case Kind::stop:
        versions(out, saved.cancellations);
        references(out, saved.added_causes);
        references(out, saved.playback_intents);
        break;
    case Kind::resume:
        references(out, saved.resolved_causes);
        break;
    }
    return out.value;
}

} // namespace storage_027_fixture::frame_oracle
