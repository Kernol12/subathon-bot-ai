// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/connector_intake_start.hpp>

namespace subathon::server::connector_intake::recovery {

// Maximum cumulative writer charges for one authority call.
struct Reservation {
    qsizetype statements{}, bound_bytes{}, result_rows{}, result_bytes{};
};

enum class Status {
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

// Qualification uses the writer’s captured profile and reserves two authority calls.
[[nodiscard]] bool qualified(const SqliteWriter &, Reservation);
// Read in the owning writer unit. A distinct first basis requires its own scope permit.
[[nodiscard]] Recovery recover_intake(const SqliteWorkSession &, StartupAuthority &,
                                      const StartupBinding &, bool require_fresh, Reservation,
                                      const Scope &, const QByteArray &intake_ref,
                                      std::optional<AttemptLocator> = {});
// Consume the existing managed admission turn and retain detail with receipt custody.
[[nodiscard]] ManagedResult admit(const SqliteWorkSession &, const ManagedInput &,
                                  const RetainedDocument &detail,
                                  const std::function<bool()> &before_commit = {});

} // namespace subathon::server::connector_intake::recovery
