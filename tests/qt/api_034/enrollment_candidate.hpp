// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../api_029/startup_fixture.hpp"

namespace api_034_fixture {
using namespace api_029_fixture;

enum class EnrollmentAction { enroll_source, inspect };

struct EnrollmentRequest {
    Scope existing;
    Fence expected;
    qint64 expected_policy;
    StartupLane incoming;
    QByteArray configuration;
};

struct EnrollmentPermit {
    QByteArray decision;
};
using EnrollmentAuthority = std::function<
    std::variant<StartupDenied, StartupUnavailable, EnrollmentPermit>(
        const SqliteWorkSession &, EnrollmentAction, const Scope &, qint64)>;

// Replace this missing-behavior adapter with the reviewed production entry.
inline StartupResult enroll(Fixture &fixture, const EnrollmentRequest &,
                            EnrollmentAction, const EnrollmentAuthority &) {
    fixture.store.work([](const auto &) -> QVariant { return true; });
    return {IntakeStatus::not_implemented};
}
} // namespace api_034_fixture
