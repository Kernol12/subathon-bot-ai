// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/connector_intake_start.hpp>

namespace subathon::server::connector_intake {
namespace {

// Stage 1 enters an acknowledged owner turn without staging any state.
StorageError missing_startup_owner(StartupOwner &owner, StartupCompletion complete) {
    if (!complete)
        return StorageError::invalid_work;
    return owner.writer.enqueue_work(
        [](auto context) -> QVariant {
            const auto session = context.begin();
            session.commit();
            return true;
        },
        [complete = std::move(complete)](SqliteWorkResult work) {
            std::optional<StartupResult> result;
            if (work.transaction == TransactionOutcome::committed &&
                work.error == WorkError::none && work.value)
                result.emplace();
            complete({StorageError::none, std::move(work), std::move(result)});
        });
}

} // namespace

StorageError register_first(StartupOwner &owner, RegisterRequest, StartupCompletion complete,
                            StartupFaults) {
    return missing_startup_owner(owner, std::move(complete));
}

StorageError activate_first(StartupOwner &owner, ActivateRequest, StartupCompletion complete,
                            StartupFaults) {
    return missing_startup_owner(owner, std::move(complete));
}

StorageError checkpoint_first(StartupOwner &owner, CheckpointRequest, StartupCompletion complete,
                              StartupFaults) {
    return missing_startup_owner(owner, std::move(complete));
}

StorageError inspect_startup(StartupOwner &owner, StartupSelector, StartupCompletion complete) {
    return missing_startup_owner(owner, std::move(complete));
}

} // namespace subathon::server::connector_intake
