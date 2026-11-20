// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_milestone_detail.hpp"
#include <subathon/server/delivery_jobs.hpp>

namespace subathon::server::delivery_sql {
using State = DeliveryJobState;
using milestone_sql::corrupt;
using milestone_sql::integer;
using milestone_sql::reference;

struct Event {
    State state;
    std::int64_t revision;
    std::optional<std::int64_t> attempt;
    std::optional<std::string> evidence;
};

inline Event read_event(const SqliteWorkSession &session, const QString &source,
                        QList<QVariant> args, std::optional<std::int64_t> revision = {}) {
    QString sql = "SELECT CASE WHEN typeof(revision)='integer' THEN revision END,"
                  "CASE WHEN typeof(state)='integer' THEN state END,"
                  "CASE WHEN attempt IS NULL OR typeof(attempt)='integer' THEN attempt ELSE 0 END,"
                  "CASE WHEN evidence_ref IS NULL THEN NULL "
                  "WHEN typeof(evidence_ref)='blob' AND length(evidence_ref) BETWEEN 1 AND 4096 "
                  "THEN evidence_ref ELSE X'' END FROM " +
                  source;
    if (revision) {
        sql += " AND revision=?";
        args.append(qint64(*revision));
    } else {
        sql += " ORDER BY revision DESC LIMIT 1";
    }
    const auto rows = session.execute({std::move(sql), std::move(args)});
    if (rows.size() != 1)
        corrupt();
    const auto &row = rows.front();
    const auto number = integer(row[0]);
    const auto state = integer(row[1]);
    if (number < 1 || state < 0 || state > int(State::outcome_unknown))
        corrupt();
    Event event{State(state), number, {}, {}};
    if (!row[2].isNull())
        event.attempt = integer(row[2]);
    if (event.attempt && *event.attempt < 2)
        corrupt();
    if (!row[3].isNull())
        event.evidence = reference(row[3]);
    if (number == 1 && event.state != State::queued)
        corrupt();
    switch (event.state) {
    case State::queued:
        if (event.attempt || event.evidence.has_value() != (number != 1))
            corrupt();
        break;
    case State::sending:
        if (event.attempt != number || event.evidence)
            corrupt();
        break;
    case State::delivered:
    case State::failed:
    case State::outcome_unknown:
        if (!event.attempt || *event.attempt < 2 || *event.attempt != number - 1 || !event.evidence)
            corrupt();
        break;
    }
    return event;
}

inline Event latest_event(const SqliteWorkSession &session, const QString &source,
                          const QList<QVariant> &args) {
    auto event = read_event(session, source, args);
    if (event.revision == 1)
        return event;
    const auto previous = read_event(session, source, args, event.revision - 1);
    switch (event.state) {
    case State::queued:
        if (previous.state != State::failed ||
            read_event(session, source, args, previous.attempt).state != State::sending)
            corrupt();
        break;
    case State::sending:
        if (previous.state != State::queued)
            corrupt();
        break;
    case State::delivered:
    case State::failed:
    case State::outcome_unknown:
        if (previous.state != State::sending || previous.attempt != event.attempt)
            corrupt();
        break;
    }
    return event;
}

} // namespace subathon::server::delivery_sql
