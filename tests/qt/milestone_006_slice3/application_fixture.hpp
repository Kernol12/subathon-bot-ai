// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../milestone_006_slice2/refund_fixture.hpp"

namespace milestone_006_slice3 {
using namespace storage_027_fixture;
using milestone_006_slice2::advance_to_protected_successor;
using milestone_006_slice2::reopen_financial;

inline const MilestoneOccurrenceKey first{"application-occurrence-A"};
inline const MilestoneOccurrenceKey later{"application-occurrence-B"};
inline constexpr auto bonus_ordinal = 2;

struct ApplicationFixture {
    finance::File file;
    Occurrence a{edit::track, first, ApplicationOperationKey{"test", "group-A"}};
    Occurrence b{edit::track, later, ApplicationOperationKey{"test", "group-B"}};

    explicit ApplicationFixture(bool mixed = true, bool deferred = false) {
        report(migrate(*file.writer,
                       {MigrationMode::advance, contribution_review_repository_manifest(), {}, {}}),
               CatalogState::current, 19, 19, 4);
        report(migrate(*file.writer,
                       {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
               CatalogState::current, 20, 20, 1);
        reopen_financial(file);
        file.write([&](const auto &s) {
            require(SqliteAccountSessionUnit(s, {}).insert_installation(
                        {{"install", "epoch"}, "revision"}) == LifecycleWriteResult::staged,
                    "Real installation owns the track");
        });

        const std::vector<MilestoneAction> full{
            {MilestoneActionKind::automation, "audio", "v1"},
            {MilestoneActionKind::bonus, "award", "bonus-v1"},
            {MilestoneActionKind::automation, "chat", "v1"},
            {MilestoneActionKind::automation, "in-flight", "v1"},
            {MilestoneActionKind::automation, "queued-1", "v1"},
            {MilestoneActionKind::automation, "queued-2", "v1"}};
        const std::vector<MilestoneAction> single{full.front()};
        const auto left =
            create(a, "A", 35000000, mixed ? full : single,
                   deferred ? MilestoneActionChoice::defer : MilestoneActionChoice::run);
        const auto right = create(b, "B", 36000000, single);
        file.write([&](const auto &s) {
            const MilestoneTrackSeed seed{edit::track,
                                          "cfg",
                                          "application-v1",
                                          "application-prefix",
                                          {{"A", 35000000}, {"B", 36000000}},
                                          std::vector{left, right}};
            require(initialize_milestone_track(s, seed),
                    "Exact application members are enrolled in normal track order");
        });
        advance_to_protected_successor(file);
    }

    MilestoneTrackSeed::Member create(const Occurrence &target, const char *name, qint64 threshold,
                                      const std::vector<MilestoneAction> &actions,
                                      MilestoneActionChoice choice = MilestoneActionChoice::run) {
        const auto operation = std::get<ApplicationOperationKey>(target.origin);
        const auto receipt = file.intake(operation.operation_ref, {}, "synthetic-owner-command");
        const subathon::core::MilestoneCompletion achievement{name, "application-v1", threshold,
                                                              40000000, "ctx"};
        file.write([&](const auto &s) {
            auto bundle = finance::File::receipt_application(s, receipt, operation.operation_ref);
            finance::File::retain_plan(s, bundle.intent.plan,
                                       finance::File::plain_capture(receipt, {}, {}));
            std::vector<DeliveryCapture> captures;
            std::vector<MilestoneJobMembership> membership;
            for (std::size_t i = 0; i < actions.size(); ++i) {
                const auto &action = actions[i];
                if (action.kind == MilestoneActionKind::bonus)
                    continue;
                const auto ordinal = static_cast<qint64>(i + 1);
                const auto key = job(target, ordinal);
                bundle.jobs.push_back(
                    {key.job_ref,
                     {},
                     target.occurrence.ref,
                     ordinal,
                     {delivery_job_format,
                      QByteArray::fromStdString("captured-" + action.action_ref)}});
                captures.push_back({key.job_ref, "local/route",
                                    DeliverySenderSelection::explicit_identity, "original-sender",
                                    action.action_ref, action.version_ref});
                membership.push_back({ordinal, key});
            }
            require(SqliteDeliveryJobStore(s).stage(bundle, captures).disposition ==
                        SqliteBundleStageDisposition::staged,
                    "Normal bundle store enrolls the exact original automation jobs");
            SqliteMilestoneGroupStore(s).stage(
                operation, {{{achievement, actions, {}, target.occurrence}, membership}});
            SqliteMilestoneGroupDecisions(s).record_acceptance(
                operation, {PausedAcceptanceMode::normal,
                            {"operator", "accept-" + target.occurrence.ref},
                            {{target.occurrence, choice}}});
        });
        return {achievement, operation, target.occurrence};
    }

    static DeliveryJobKey job(const Occurrence &target, qint64 ordinal) {
        return {std::get<ApplicationOperationKey>(target.origin), "job-" + std::to_string(ordinal)};
    }

    MilestoneReleaseDisposition release(const Occurrence &target) {
        const auto reply = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
            return release_milestone_head(*file.writer, target.occurrence, std::move(done));
        });
        finance::normal(reply.work);
        require(reply.disposition.has_value(), "Acknowledged normal release returns a disposition");
        return *reply.disposition;
    }

    Decision command(Authority &authority, const Command &command,
                     const ReplacePreview *preview = nullptr) {
        std::optional<Decision> staged;
        file.write([&](const auto &s) { staged = stage_command(s, authority, command, preview); });
        return *staged;
    }

    std::shared_ptr<const ReplacePreview> preview(Authority &authority, const Command &command) {
        Preview answer{Disposition::not_implemented};
        file.write([&](const auto &s) { answer = preview_replace(s, authority, command); });
        const auto *value = std::get_if<std::shared_ptr<const ReplacePreview>>(&answer);
        require(value && *value, "Exact scoped preview is confirmable");
        return *value;
    }
};

struct OriginalAdapter final : DeliveryDispatchAuthority, DeliveryAdapter {
    std::vector<DeliveryJob> starts;
    std::size_t selections{};

    bool allows(const DeliveryJob &job) override {
        ++selections;
        return job.capture.sender_ref == "original-sender" &&
               job.capture.route_ref == "local/route";
    }

    void start(DeliveryJob job) override { starts.push_back(std::move(job)); }
};

inline DeliveryDisposition dispatch(ApplicationFixture &f, const DeliveryJobKey &key,
                                    const std::shared_ptr<OriginalAdapter> &port) {
    const auto reply = finance::completed<DeliveryDispatchCompletion>([&](auto done) {
        return dispatch_delivery_job(*f.file.writer, key, 1, port, port, std::move(done));
    });
    finance::normal(reply.work);
    require(reply.result && !reply.adapter_threw, "Dispatch acknowledges the exact transition");
    return reply.result->disposition;
}

struct SuccessorAuthority final : ReplacementDispatchAuthority {
    int calls{};
    bool allows(const ReplacementJob &job) override {
        ++calls;
        return job.payload == QByteArray::fromHex("0081") && job.qualified_sender == "S";
    }
};

inline Command replacement(Authority &owner, const Occurrence &target, QByteArray operation,
                           const std::vector<Replacement> &patch) {
    owner.captures.clear();
    for (const auto &slot : patch)
        owner.captures.push_back({slot, QByteArray::fromHex("0081"), "local/next", "S"});
    return {edit::track, std::move(operation), edit::actor, ReplaceIntent{target, patch}};
}

} // namespace milestone_006_slice3
