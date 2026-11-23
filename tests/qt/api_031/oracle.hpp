// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QtEndian>
#include <subathon/server/connector_intake_start.hpp>

namespace api_031_fixture {
using namespace subathon::server;
using namespace subathon::server::connector_intake;

inline const Scope a{"install", "P", "A", "S"};
inline const Scope b{"install", "P", "B", "S"};
inline const QByteArray cursor{"cursor-contract"};
inline const QByteArray profile{"history-v1"};
inline const QByteArray config{"cfg-1"};
inline const Fence pending{1, 1, 1, "g0"};
inline const Fence active{2, 2, 2, "g2"};
inline const Fence progressed{2, 3, 2, "g2"};
inline const Fence reconnected{2, 3, 2, "g3"};
inline const Fence reconnected_progress{2, 4, 2, "g3"};
inline const Fence pending_b{1, 1, 1, "gB0"};
inline const Fence active_b{2, 2, 2, "gB"};

// Specification examples in network byte order. These helpers never use the
// storage codecs or candidate-produced bytes as expectations.
inline QByteArray f(const QByteArray &value) {
    char bytes[4];
    qToBigEndian(quint32(value.size()), bytes);
    return QByteArray(bytes, 4) + value;
}
inline QByteArray integer(qint64 value) {
    char bytes[8];
    qToBigEndian(quint64(value), bytes);
    return QByteArray(bytes, 8);
}
inline QByteArray opt(const std::optional<QByteArray> &blob) {
    return blob ? QByteArray(1, '\x01') + f(*blob) : QByteArray(1, '\x00');
}
inline QByteArray source_head(const Scope &scope, const Fence &fence,
                              const std::optional<QByteArray> &frontier = {}) {
    return f(scope.installation) + f(scope.provider) + f(scope.account) + f(scope.stream) +
           integer(fence.epoch) + integer(fence.revision) + integer(fence.policy_revision) +
           f(fence.generation) + opt(frontier);
}
inline QByteArray policy(const QByteArray &decision) {
    return f("review") + f("cfg-1") + f(decision);
}
inline PolicyMember member(const Scope &scope, bool disjoint = false) {
    const bool primary = scope == a || disjoint;
    const auto lane = scope == a ? "review-lane" : disjoint ? "disjoint-lane" : "mirror-lane";
    return {scope,
            "individual",
            disjoint ? "main-B" : "main",
            disjoint ? "overlap-B" : "overlap",
            primary,
            f(lane) + f("operator") + f("history-v1") + f("cursor-contract")};
}
inline QByteArray cut(const QByteArray &generation, const QByteArray &evidence) {
    return QByteArray("CUT1\x01", 5) + f("key-v1") + f("S/history") + opt(QByteArray("10")) +
           f(generation) + f(evidence);
}
inline QByteArray covered(const QByteArray &evidence, const QByteArray &upper = "11") {
    return QByteArray("CV1\x00\x02\x01", 6) + f("key-v1") + f("S/history") + f("10") + f(upper) +
           f(QByteArray{}) + f(evidence);
}
inline StartupOperation operation(const Scope &scope, const Fence &before, const Fence &after,
                                  QByteArray key, QByteArray decision, QByteArray boundary,
                                  std::optional<QByteArray> prior_cut, QByteArray selected_cut,
                                  int purpose = 2, std::optional<QByteArray> frontier = {}) {
    const QByteArray magic = purpose == 4 ? "RC1" : "SH1";
    return {std::move(key),
            scope,
            purpose,
            2,
            2,
            after.generation,
            1,
            1,
            magic + source_head(scope, before) + source_head(scope, after, frontier) + f(cursor) +
                f(decision) + f(boundary),
            std::move(prior_cut),
            std::move(selected_cut)};
}

inline const QByteArray k10 = cut("g2", "cut10");
inline const QByteArray kb10 = cut("gB", "cutB10");
inline const QByteArray v11 = covered("live-empty-11");
inline const QByteArray vr11 = covered("live-empty-r1-11");
inline const StartupOperation b0 =
    operation(a, pending, active, "B0", "operator/D1", "cut10", {}, k10);
inline const StartupOperation bb0 =
    operation(b, pending_b, active_b, "BB0", "operator/D1", "cutB10", {}, kb10);
inline const StartupOperation r0 =
    operation(a, active, reconnected, "R0", "operator/D3", "connected", k10, k10, 4);
inline const StartupOperation k0 =
    operation(a, active, progressed, "K0", "operator/D2", v11, k10, k10, 2, QByteArray("11"));
inline const StartupOperation kr0 = operation(a, reconnected, reconnected_progress, "K0",
                                              "operator/D2", vr11, k10, k10, 2, QByteArray("11"));

inline StartupProjection registration(bool two = false, bool disjoint = false) {
    StartupProjection result{
        1, policy("operator/D0"), {member(a)}, {{a, pending, 0, {}, {}, cursor}}, {}};
    if (two) {
        result.members.append(member(b, disjoint));
        result.sources.append({b, pending_b, 0, {}, {}, cursor});
    }
    return result;
}
inline StartupProjection activation(bool two = false, bool disjoint = false) {
    StartupProjection result{
        2, policy("operator/D1"), {member(a)}, {{a, active, 1, k10, {}, cursor}}, {b0}};
    if (two) {
        result.members.append(member(b, disjoint));
        result.sources.append({b, active_b, 1, kb10, {}, cursor});
        result.operations.append(bb0);
    }
    return result;
}
inline StartupProjection checkpoint(bool with_reconnect = false) {
    const QList<StartupOperation> history =
        with_reconnect ? QList<StartupOperation>{b0, r0, kr0} : QList<StartupOperation>{b0, k0};
    return {
        2,
        policy("operator/D1"),
        {member(a)},
        {{a, with_reconnect ? reconnected_progress : progressed, 1, k10, QByteArray("11"), cursor}},
        history};
}

} // namespace api_031_fixture
