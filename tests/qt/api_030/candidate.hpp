// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/connector_intake_start.hpp>

namespace subathon::server::connector_intake::recovery_candidate {

// Fixture interface for the reviewed contract. Wider host integration is not
// needed to exercise the bounded writer unit and its existing authority port.
struct Reservation {
    qsizetype statements, bound_bytes, result_rows, result_bytes;
};

enum class Status {
    not_implemented,
    recovered,
    denied,
    unavailable,
    missing,
    not_identified,
    locator_mismatch,
    inconsistent,
    unsupported_legacy,
    resource_limit,
    no_basis
};

struct Closure {
    Scope scope;
    QByteArray intake_ref;
    AttemptLocator locator;
    core::ReceiptIdentity identity;
    core::ReceiptAttempt attempt;
    std::optional<PageSlot> history;
    std::optional<VerifiedActivityLink> activity;
    IntakeAdmission admission;
    QByteArray normalized, capture;
    RetainedDocument configuration, modifier_arrival, detail;
};

struct Recovery {
    Status status;
    std::optional<Closure> selected;
    std::optional<Closure> original;
};

inline bool qualified(const SqliteWriter &, Reservation) { return false; }

inline Recovery recover_intake(const SqliteWorkSession &, StartupAuthority &,
                               const StartupBinding &, bool, Reservation, const Scope &,
                               const QByteArray &, std::optional<AttemptLocator> = {}) {
    return {Status::not_implemented, {}, {}};
}

inline ManagedResult admit(const SqliteWorkSession &session, const ManagedInput &,
                           const RetainedDocument &, const std::function<bool()> & = {}) {
    session.rollback();
    return {ManagedStatus::known_rollback, {}, {}};
}

} // namespace subathon::server::connector_intake::recovery_candidate
