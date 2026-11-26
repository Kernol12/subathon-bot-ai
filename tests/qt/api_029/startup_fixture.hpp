// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../api_031/fixture.hpp"

namespace api_029_fixture {
using namespace api_031_fixture;

inline void require_startup(const StartupOutcome &result, const char *transition) {
    require(result.admission == StorageError::none && result.work && result.result, transition);
    catalog::finance::normal(*result.work);
    require(result.result->status == IntakeStatus::committed, transition);
}

struct HistoryFixture {
    Fixture startup;

    explicit HistoryFixture(bool checkpoint = true, bool two_sources = false)
        : startup(two_sources) {
        require_startup(startup.submit(startup.register_request()), "Register first source");
        require_startup(startup.submit(startup.activate_request()), "Activate first source");
        if (checkpoint)
            require_startup(startup.submit(startup.checkpoint_request()),
                            "Commit first checkpoint");
    }

    void open_catch_up() {
        startup.store.work([](const auto &work) -> QVariant {
            const GapOpening gap{a,    progressed, "G",         "g3",          "11",
                                 "14", cursor,     "gap-basis", "gap-upper:14"};
            require(stage_gap(work, gap) == HistoryStatus::staged, "Open gap on the exact fence");
            const OperationIntent intent{
                a,
                "O",
                2,
                2,
                "g3",
                QByteArray("T"),
                {},
                {},
                {},
                0,
                {Endpoint::exclusive, Endpoint::inclusive, "ordered-membership", "ordered-key"},
                {"key-v1", "S/history", QByteArray("14"), "g3", "cut14"},
                profile,
                "actor/decision",
                "actor/decision",
                HistoryPurpose::catch_up,
                QByteArray("G"),
                4};
            require(stage_operation(work, intent) == HistoryStatus::staged,
                    "Bind reviewed Catch-up intent");
            return true;
        });
    }
};
} // namespace api_029_fixture
