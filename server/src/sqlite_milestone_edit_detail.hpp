// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_delivery_event_read.hpp"
#include "sqlite_milestone_detail.hpp"
#include <QDataStream>
#include <QIODevice>
#include <span>
#include <subathon/server/sqlite_milestone_target_edits.hpp>
#include <variant>

namespace subathon::server::milestone_edit_sql {

using namespace milestone_sql;
inline constexpr qint64 collection_limit = 4096;
inline constexpr qsizetype document_limit = 8'388'608;

struct Occurrence {
    MilestoneEditKey edit;
    CommittedMilestoneGroup group;
    MilestoneActionChoice choice;
    MilestoneDecisionEvidence evidence;
    qint64 order;
    bool released;
};

qint64 queue_tail(const SqliteWorkSession &);
std::optional<Occurrence> occurrence(const SqliteWorkSession &, const QByteArray &,
                                     bool actions = false,
                                     const MilestoneEditKey *expected = nullptr);
std::optional<std::pair<QByteArray, bool>> pending_head(const SqliteWorkSession &);
std::vector<std::pair<qint64, CapturedMilestoneGroup>> pending(const SqliteWorkSession &);
struct UnavailableRefundTarget {};
using RefundTarget = std::variant<std::monostate, qint64, UnavailableRefundTarget>;

RefundTarget current_target(const SqliteWorkSession &, const MilestoneOccurrenceKey &,
                            const std::string &installation);

inline std::size_t membership_bytes(const MilestoneEditKey &origin,
                                    const MilestoneOccurrenceKey &occurrence, std::size_t jobs) {
    const auto origin_bytes = origin.track.installation_ref.size() + origin.track.track_ref.size() +
                              origin.operation_ref.size();
    return origin_bytes + jobs * (origin_bytes + occurrence.ref.size() + sizeof(std::int64_t));
}

struct GroupResultSize {
    std::size_t groups{};
    std::size_t actions{};
    std::size_t bytes{};

    void include(GroupResultSize next) {
        groups += next.groups;
        actions += next.actions;
        bytes += next.bytes;
        if (groups > std::size_t(collection_limit) || actions > std::size_t(collection_limit) ||
            bytes > std::size_t(document_limit))
            throw SqliteBundleStorageFailure(SqliteBundleStorageReason::result_limit);
    }
};

inline GroupResultSize group_result_size(const MilestoneEditKey &origin,
                                         const MilestoneOccurrenceKey &occurrence,
                                         const core::MilestoneCompletion &completion,
                                         std::span<const MilestoneAction> actions,
                                         std::size_t jobs) {
    auto bytes = membership_bytes(origin, occurrence, jobs) + occurrence.ref.size() +
                 completion.milestone_ref.size() + completion.definition_revision_ref.size() +
                 completion.completion_context_ref.size();
    for (const auto &action : actions)
        bytes += action.action_ref.size() + action.version_ref.size();
    return {1, actions.size(), bytes};
}

GroupResultSize pending_result_size(const SqliteWorkSession &);
std::int64_t total_score(const ApplicationStateSnapshot &);
void check_application_identity(const SqliteWorkSession &, const QByteArray &, qint64 order);

inline const SqlRow *unique(const SqlRows &rows) {
    if (rows.size() > 1)
        corrupt();
    return rows.empty() ? nullptr : &rows.front();
}

template <typename Enum> Enum enumeration(const QVariant &cell, Enum last) {
    const auto value = integer(cell);
    if (value < 0 || value > qint64(last))
        corrupt();
    return Enum(value);
}

class Decoding {
  public:
    explicit Decoding(const QVariant &cell) : data_(cell.toByteArray()), stream_(data_) {
        if (cell.metaType().id() != QMetaType::QByteArray || data_.size() > document_limit ||
            number() != 1)
            corrupt();
    }

    qint64 number() {
        qint64 value = 0;
        stream_ >> value;
        if (stream_.status() != QDataStream::Ok)
            corrupt();
        return value;
    }

    QByteArray bytes(qint64 maximum = document_limit) {
        const auto size = number();
        const auto pos = stream_.device()->pos();
        if (size < 0 || size > maximum || size > data_.size() - pos)
            corrupt();
        auto part = data_.sliced(pos, size);
        if (!stream_.device()->seek(pos + size))
            corrupt();
        return part;
    }

    std::string ref() { return reference(bytes(4096)); }

    void end() {
        if (!stream_.atEnd())
            corrupt();
    }

  private:
    QByteArray data_;
    QDataStream stream_;
};

inline const QString edit_event_source =
    "main.subathon_milestone_edit_events WHERE occurrence_ref=? AND action_ordinal=?";

// Join exact capture, job, occurrence and action. A missing automation
// enrollment is corruption, while an unassigned or bonus position is absent.
inline std::optional<MilestoneEditJob> job(const SqliteWorkSession &session,
                                           const Occurrence &selected, qint64 ordinal, bool body) {
    const auto occurrence_ref = blob(selected.group.occurrence->ref);
    const auto rows = session.execute(
        {"SELECT a.kind," + ref_column("a.action_ref") + "," + ref_column("a.version_ref") + "," +
             ref_column("c.action_ref") + "," + ref_column("c.version_ref") + "," +
             ref_column("c.route_ref") + ",c.sender_selection," + ref_column("c.sender_ref") +
             ",j.action_ordinal,CASE WHEN typeof(j.job_format)='blob' AND j.job_format=? "
             "AND typeof(j.action_document)='blob' AND length(j.action_document)<=262144 "
             "AND j.kind=0 AND (j.installation_ref,j.track_ref,j.edit_ref)=(?,?,?) THEN 1 ELSE 0 "
             "END" +
             (body ? ",CASE WHEN typeof(j.action_document)='blob' AND length(j.action_document)"
                     "<=262144 THEN j.action_document END"
                   : "") +
             " FROM main.subathon_milestone_edit_actions a "
             "LEFT JOIN main.subathon_milestone_edit_jobs j USING(occurrence_ref,action_ordinal) "
             "LEFT JOIN main.subathon_milestone_edit_captures c "
             "USING(occurrence_ref,action_ordinal) "
             "WHERE a.occurrence_ref=? AND a.action_ordinal=?",
         {blob(delivery_job_format), blob(selected.edit.track.installation_ref),
          blob(selected.edit.track.track_ref), blob(selected.edit.operation_ref), occurrence_ref,
          ordinal}});
    const auto *row = unique(rows);
    if (!row) {
        const auto orphan =
            session.execute({"SELECT 1 FROM main.subathon_milestone_edit_jobs WHERE "
                             "occurrence_ref=? AND action_ordinal=?",
                             {occurrence_ref, ordinal}});
        if (!orphan.empty())
            corrupt();
        return {};
    }
    const auto kind = enumeration((*row)[0], MilestoneActionKind::bonus);
    if (kind == MilestoneActionKind::bonus) {
        if (!(*row)[8].isNull())
            corrupt();
        return {};
    }
    if (integer((*row)[8]) != ordinal || integer((*row)[9]) != 1 ||
        reference((*row)[1]) != reference((*row)[3]) ||
        reference((*row)[2]) != reference((*row)[4]))
        corrupt();
    QByteArray document;
    if (body) {
        if ((*row)[10].metaType().id() != QMetaType::QByteArray)
            corrupt();
        document = (*row)[10].toByteArray();
        if (document.isNull())
            document = QByteArray("", 0);
    }
    const auto event =
        delivery_sql::latest_event(session, edit_event_source, {occurrence_ref, ordinal});
    if (event.revision != 1 && !selected.released)
        corrupt();
    return MilestoneEditJob{{selected.edit, *selected.group.occurrence, ordinal},
                            {reference((*row)[5]),
                             enumeration((*row)[6], DeliverySenderSelection::explicit_identity),
                             reference((*row)[7]), reference((*row)[3]), reference((*row)[4])},
                            std::move(document),
                            event.state,
                            event.revision,
                            event.attempt};
}

} // namespace subathon::server::milestone_edit_sql
