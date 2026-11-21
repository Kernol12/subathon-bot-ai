// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace rates_003_fixture;
using enum fx::FailureKind;

constexpr std::int64_t report_tick = 29'000;

void exercise_g4(Fixture &f) {
    const auto report_at = clock(report_tick, january);
    const auto request = quota("g4-sample");
    const auto report =
        f.exchange(request, clock(25'000), quota_reply(30'000, 25'623, report_at, "17"));
    const auto *sample = std::get_if<fx::Quota>(&report.outcome);
    ASSERT_NE(sample, nullptr);
    expect_counts(*sample, 30'000, 25'623, hint17);
    EXPECT_EQ(sample->receipt.utc_ms, report_at.utc_ms);
    EXPECT_EQ(sample->receipt.elapsed, report_at.elapsed);
    ASSERT_EQ(f.transport.calls.size(), 1u);
    expect_get(f.transport.calls[0], std::string(keyed_root) + "quota",
               f.selection.offered.selection, request);

    const auto latest_at = clock(report_tick + 2'000);
    const auto primary = f.start(latest("g4-auto"), latest_at, sent(report_tick + 2'000));
    expect_blocked(f, latest("g4-manual", {"EUR"}, fx::RequestOrigin::manual), latest_at,
                   coalesced);
    const auto rates =
        f.finish(primary, frame(std::string(keyed_latest_body), clock(report_tick + 3'000)));
    ASSERT_TRUE(std::holds_alternative<fx::Rate>(rates.outcome));

    const auto timeout =
        f.exchange(history("g4-history-timeout"), clock(report_tick + 4'000),
                   fx::FailedTransport{fx::TransportFailure::timeout, clock(report_tick + 5'000)});
    expect_failure(timeout, failed_sync);
    EXPECT_EQ(timeout.dispatch.kind, fx::DispatchKind::sent);
    ASSERT_EQ(f.transport.calls.size(), 3u);
    const auto view = f.view(clock(report_tick + 20 * minute_ms, january - day_ms));
    expect_report(view, 30'000, 25'623, hint17, 3, 2);
    EXPECT_EQ(view.sample_age_ms, 20 * minute_ms);
    EXPECT_TRUE(view.provider_may_lag);
    EXPECT_FALSE(view.quota_held);
    EXPECT_FALSE(f.view({january + 20 * minute_ms, {}}).sample_age_ms);
    EXPECT_FALSE(
        f.view({{}, fx::Elapsed{"new-clock-epoch", report_tick + 20 * minute_ms}}).sample_age_ms);
}

TEST(ExchangeRateApi, G4_QuotaReportUsageAndReceiptAgeRemainSeparate) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    ASSERT_NO_FATAL_FAILURE(exercise_g4(f));
}

TEST(ExchangeRateApi, G8_MalformedQuotaKeepsPriorReportAndInvalidHintDoesNotDiscardCounts) {
    struct Variant {
        std::string_view body;
        std::string_view invalid_hint;
    };
    for (
        const auto &[body, invalid_hint] : {
            Variant{R"({"result":"success","plan_quota":30000,"requests_remaining":)", "32"},
            Variant{
                R"({"result":"success","plan_quota":30000,"requests_remaining":24000,"requests_remaining":25000})",
                "0"},
            Variant{
                R"({"result":"success","plan_quota":30000,"requests_remaining":24000,"refresh_day_of_month":017})",
                R"("17")"},
            Variant{R"({"result":"success","plan_quota":3e4,"requests_remaining":24000})", "17.0"},
        }) {
        SCOPED_TRACE(body);
        Fixture f;
        ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
        ASSERT_NO_FATAL_FAILURE(exercise_g4(f));

        const auto failed_at = report_tick + 20 * minute_ms + 1;
        const auto failed =
            f.exchange(quota("g8-bad-report"), clock(failed_at),
                       frame(std::string(body), clock(failed_at, january + 20 * minute_ms)));
        expect_failure(failed, invalid_document);
        EXPECT_EQ(failed.dispatch.kind, fx::DispatchKind::sent);
        expect_report(f.view(clock(failed_at)), 30'000, 25'623, hint17, 4, 3);

        // The history timeout and malformed report are consecutive failures on one scope.
        const auto retry = failed_at + 5 * minute_ms;
        expect_blocked(f, quota("g8-retry-early"), clock(retry - 1), retry_wait);
        const auto fresh = f.exchange(
            quota("g8-fresh-report"), clock(retry),
            quota_reply(30'000, 24'000, clock(retry, january + 25 * minute_ms), invalid_hint));
        const auto *sample = std::get_if<fx::Quota>(&fresh.outcome);
        ASSERT_NE(sample, nullptr);
        expect_counts(*sample, 30'000, 24'000, bad_hint);
        expect_report(f.view(clock(retry)), 30'000, 24'000, bad_hint, 5, 0);
        EXPECT_EQ(f.view(clock(retry)).sample_age_ms, 0);
        EXPECT_FALSE(f.view(clock(retry)).quota_held);
        EXPECT_EQ(f.transport.calls.size(), 5u);
    }
}

void initial_exhaustion(Fixture &f) {
    const auto at = clock(0, january);
    const auto result = f.exchange(quota("initial-exhaustion"), at, quota_reply(30'000, 0, at));
    const auto *sample = std::get_if<fx::Quota>(&result.outcome);
    ASSERT_NE(sample, nullptr);
    expect_counts(*sample, 30'000, 0, no_hint);
    expect_report(f.view(at), 30'000, 0, no_hint, 1, 0);
    EXPECT_TRUE(f.view(at).quota_held);
}

fx::SelectionUpdate entitlement_b() {
    auto b = setup(fx::Profile::pro, "B", "S", fx::AuthorizedChange::credential_entitlement);
    b.credential->private_segment = "FxSecretCanary_EntitlementB";
    return b;
}

TEST(ExchangeRateApi, UnknownScopeContinuityDoesNotGrantAnEntitlementProbeOrKnownUsage) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    ASSERT_NO_FATAL_FAILURE(initial_exhaustion(f));
    auto candidate = entitlement_b();
    candidate.continuity = fx::ScopeContinuity::unknown;
    ASSERT_EQ(f.select(candidate), fx::SelectionStatus::selected);
    const auto at = clock(2 * day_ms, january + 2 * day_ms);
    expect_blocked(f, quota("unknown-continuity-probe"), at, quota_hold);
    expect_blocked(f, latest("unknown-continuity-latest"), at, quota_hold);
    const auto view = f.adapter.quota_view(candidate.selection.scope, at);
    EXPECT_FALSE(view.local_total);
    EXPECT_FALSE(view.local_since_report);
    EXPECT_TRUE(view.quota_held);
    EXPECT_EQ(f.transport.calls.size(), 1u);
}

TEST(ExchangeRateApi, G5_OnlyFreshPositiveCurrentProbeClearsSameScopeExhaustion) {
    Fixture f;
    const auto a = setup(fx::Profile::pro);
    ASSERT_EQ(f.select(a), fx::SelectionStatus::selected);
    ASSERT_NO_FATAL_FAILURE(initial_exhaustion(f));

    ASSERT_EQ(f.adapter.edit_reference_allowance(50'000), fx::SelectionStatus::selected);
    const auto edited = f.view(clock(1));
    EXPECT_EQ(edited.reference_allowance, 50'000u);
    EXPECT_TRUE(edited.quota_held);
    expect_report(edited, 30'000, 0, no_hint, 1, 0);
    ASSERT_TRUE(f.adapter.current());
    EXPECT_EQ(f.adapter.current()->binding, a.selection);
    expect_blocked(f, latest("allowance-edit-latest"), clock(1), quota_hold);
    expect_blocked(f, quota("allowance-edit-quota"), clock(1), quota_hold);

    auto revision_only = a;
    revision_only.selection.configuration_ref = "config-only";
    revision_only.selection.selection_revision = "selection-only";
    revision_only.credential->binding = revision_only.selection;
    ASSERT_EQ(f.select(revision_only), fx::SelectionStatus::selected);
    expect_blocked(f, quota("revision-only"), clock(2), quota_hold);
    ASSERT_EQ(f.transport.calls.size(), 1u);

    const auto b = entitlement_b();
    ASSERT_EQ(f.select(b), fx::SelectionStatus::selected);
    const auto b_request = quota("g5-B");
    const auto outstanding = f.start(b_request, clock(10'000), sent(10'000));
    EXPECT_EQ(outstanding.dispatch.time, sent(10'000).time);
    ASSERT_EQ(f.transport.calls.size(), 2u);
    expect_get(f.transport.calls.back(),
               "https://v6.exchangerate-api.com/v6/FxSecretCanary_EntitlementB/quota", b.selection,
               b_request, true);
    expect_report(f.view(clock(10'000)), 30'000, 0, no_hint, 2, 1);

    const auto c = setup(fx::Profile::business, "C", "S", fx::AuthorizedChange::plan_entitlement);
    ASSERT_EQ(f.select(c), fx::SelectionStatus::selected);
    expect_blocked(f, quota("g5-C-pending"), clock(11'000), quota_hold);
    const auto stale = f.finish(outstanding, quota_reply(30'000, 99, clock(12'000)));
    expect_failure(stale, superseded);
    EXPECT_EQ(stale.admission, outstanding.admission);
    EXPECT_EQ(stale.dispatch.kind, fx::DispatchKind::sent);
    expect_report(f.view(clock(12'000)), 30'000, 0, no_hint, 2, 1);
    EXPECT_TRUE(f.view(clock(12'000)).quota_held);

    expect_blocked(f, quota("g5-C-early"), clock(86'409'999), quota_hold);
    expect_blocked(f, latest("g5-C-rates-held"), clock(86'409'999), quota_hold);
    const auto c_request = quota("g5-C-probe");
    const auto result =
        f.exchange(c_request, clock(86'410'000), quota_reply(125'000, 7, clock(86'410'000)));
    ASSERT_EQ(f.transport.calls.size(), 3u);
    expect_get(f.transport.calls.back(), std::string(keyed_root) + "quota", c.selection, c_request,
               true);
    const auto *sample = std::get_if<fx::Quota>(&result.outcome);
    ASSERT_NE(sample, nullptr);
    ASSERT_GT(sample->requests_remaining, 0u);
    const auto view = f.view(clock(86'410'000));
    expect_report(view, 125'000, 7, no_hint, 3, 0);
    EXPECT_FALSE(view.quota_held);
    EXPECT_TRUE(view.provider_may_lag);

    const auto resumed = f.exchange(latest("g5-C-resumed"), clock(86'410'001),
                                    frame(std::string(keyed_latest_body), clock(86'410'002)));
    EXPECT_TRUE(std::holds_alternative<fx::Rate>(resumed.outcome));
}

TEST(ExchangeRateApi, G6_AuthStopWinsOverTimeAndRequiresCorrectedCredential) {
    Fixture f;
    const auto old = setup(fx::Profile::pro);
    ASSERT_EQ(f.select(old), fx::SelectionStatus::selected);
    const auto invalid_request = latest("g6-invalid");
    const auto invalid = f.exchange(
        invalid_request, clock(10),
        frame(
            R"({"result":"error","error-type":"invalid-key","detail":"FxSecretCanary_Original/+?"})",
            clock(20), 403));
    const auto *failure = std::get_if<fx::Failure>(&invalid.outcome);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, authentication_hold);
    EXPECT_EQ(failure->status, 403);
    EXPECT_EQ(invalid.admission.selection, old.selection);
    EXPECT_EQ(f.view(clock(20)).local_total, (fx::Usage{1, 0}));
    expect_blocked(f, latest("g6-before-retry"), clock(21), authentication_hold);
    expect_blocked(f, latest("g6-unchanged"), clock(20 + 60 * minute_ms), authentication_hold);

    auto revision_only = old;
    revision_only.selection.selection_revision = "new-selection-same-key";
    revision_only.credential->binding = revision_only.selection;
    ASSERT_EQ(f.select(revision_only), fx::SelectionStatus::selected);
    expect_blocked(f, quota("g6-revision-only"), clock(21 + 60 * minute_ms), authentication_hold);
    ASSERT_EQ(f.transport.calls.size(), 1u);
    expect_get(f.transport.calls.front(), std::string(keyed_root) + "latest/USD", old.selection,
               invalid_request);

    auto corrected = setup(fx::Profile::pro, "B", "S", fx::AuthorizedChange::corrected_credential);
    corrected.credential->private_segment = "FxSecretCanary_Corrected";
    ASSERT_EQ(f.select(corrected), fx::SelectionStatus::selected);
    const auto request = latest("g6-corrected");
    const auto body =
        R"({"result":"success","base_code":"USD","conversion_rates":{"EUR":0.9000},)"
        R"("time_last_update_unix":1767225600,"time_last_update_utc":"Thu, 01 Jan 2026 00:00:00 +0000",)"
        R"("time_next_update_unix":1767312000,"time_next_update_utc":"Fri, 02 Jan 2026 00:00:00 +0000",)"
        R"("ignored":"https://v6.exchangerate-api.com/v6/FxSecretCanary_Corrected/latest/USD"})";
    const auto repaired =
        f.exchange(request, clock(22 + 60 * minute_ms), frame(body, clock(23 + 60 * minute_ms)));
    ASSERT_TRUE(std::holds_alternative<fx::Rate>(repaired.outcome));
    ASSERT_EQ(f.transport.calls.size(), 2u);
    expect_get(f.transport.calls.back(),
               "https://v6.exchangerate-api.com/v6/FxSecretCanary_Corrected/latest/USD",
               corrected.selection, request);
    EXPECT_EQ(f.view(clock(23 + 60 * minute_ms)).local_total, (fx::Usage{2, 0}));
}

TEST(ExchangeRateApi, StructuredProviderStopsAndHttpFailureKeepTheirDistinctDispositions) {
    struct Variant {
        std::string_view code;
        int status;
        fx::FailureKind expected;
    };
    for (const auto &v : {
             Variant{"inactive-account", 403, authentication_hold},
             Variant{"plan-upgrade-required", 403, operation_unavailable},
             Variant{"unsupported-code", 400, request_domain},
             Variant{"no-data-available", 200, request_domain},
             Variant{"malformed-request", 400, request_domain},
             Variant{"", 503, failed_sync},
         }) {
        SCOPED_TRACE(v.code);
        Fixture f;
        ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
        SnapshotOwner owner;
        const auto receipt = clock(20, january);
        const auto reply = v.code.empty() ? frame(std::string(keyed_latest_body), receipt, v.status)
                                          : provider_error(v.code, receipt, v.status);
        const auto result = f.exchange(latest("provider-signal"), clock(10), reply);
        expect_failure(result, v.expected);
        owner.consider(result);
        owner.expect_cached();
        EXPECT_EQ(result.dispatch.kind, fx::DispatchKind::sent);
        EXPECT_EQ(f.transport.calls.size(), 1u);
        const auto view = f.view(receipt);
        EXPECT_EQ(view.local_total, (fx::Usage{1, 0}));
        EXPECT_FALSE(view.sample);
        EXPECT_FALSE(view.quota_held);

        const auto next = latest("after-provider-signal");
        const auto retry = clock(20 + minute_ms, january + minute_ms);
        if (v.expected == authentication_hold || v.expected == operation_unavailable) {
            expect_blocked(f, next, retry, v.expected);
        } else {
            const auto fresh =
                f.exchange(next, retry, frame(std::string(keyed_latest_body), retry));
            ASSERT_TRUE(std::holds_alternative<fx::Rate>(fresh.outcome));
            EXPECT_EQ(f.transport.calls.size(), 2u);
        }
    }
}

TEST(ExchangeRateApi, G7_OpenAndKeyedRateLimitsUseTheirOwnExactRetryBoundaries) {
    struct Variant {
        fx::Profile profile;
        fx::Headers headers;
        std::int64_t at;
        std::optional<std::int64_t> provider_delay;
        std::optional<std::int64_t> provider_deadline{};
    };
    for (const Variant &v : {
             Variant{fx::Profile::open, {{"Retry-After", "1800"}}, 1'805'000, 1'800'000},
             Variant{fx::Profile::open, {{"Retry-After", "tomorrow"}}, 1'205'000, {}},
             Variant{fx::Profile::pro, {{"Retry-After", "tomorrow"}}, 65'000, {}},
             Variant{fx::Profile::pro, {{"Retry-After", "90"}}, 95'000, 90'000},
             Variant{fx::Profile::pro,
                     {{"Retry-After", "Thu, 01 Jan 2026 00:02:00 GMT"}},
                     125'000,
                     {},
                     january + 120'000},
             Variant{
                 fx::Profile::pro, {{"Retry-After", "1800"}, {"retry-after", "3600"}}, 65'000, {}},
         }) {
        SCOPED_TRACE(v.at);
        Fixture f;
        ASSERT_EQ(f.select(setup(v.profile)), fx::SelectionStatus::selected);
        const auto limited =
            f.exchange(latest("g7-throttled"), clock(4'900),
                       frame("<bad provider frame>", clock(5'000, january), 429, v.headers));
        const auto *failure = std::get_if<fx::Failure>(&limited.outcome);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, rate_limited);
        EXPECT_EQ(failure->status, 429);
        EXPECT_EQ(failure->retry_after.delay_ms, v.provider_delay);
        const auto evidence = v.provider_deadline ? fx::RetryEvidence::utc_deadline
                              : v.provider_delay  ? fx::RetryEvidence::delay
                                                  : fx::RetryEvidence::invalid;
        EXPECT_EQ(failure->retry_after.kind, evidence);
        EXPECT_EQ(failure->retry_after.deadline_utc_ms, v.provider_deadline);
        const auto early_utc = v.provider_deadline ? *v.provider_deadline - 1 : january + day_ms;
        expect_blocked(f, latest("g7-early"), clock(v.at - 1, early_utc), retry_wait);
        if (v.provider_deadline) {
            expect_blocked(f, latest("g7-utc-early-with-elapsed-ready"), clock(v.at, early_utc),
                           retry_wait);
        }
        expect_blocked(f, latest("g7-no-elapsed"), {january + day_ms, {}}, retry_wait);

        const auto request = latest("g7-manual", {"EUR"}, fx::RequestOrigin::manual);
        const auto ready = clock(v.at, v.provider_deadline.value_or(january - day_ms));
        const auto pending = f.start(request, ready, sent(v.at));
        expect_blocked(f, latest("g7-auto"), ready, coalesced);
        const auto success =
            f.finish(pending, frame(v.profile == fx::Profile::open ? open_body(R"("time_eol":0)")
                                                                   : std::string(keyed_latest_body),
                                    clock(v.at + 1)));
        EXPECT_TRUE(std::holds_alternative<fx::Rate>(success.outcome));
        ASSERT_EQ(f.transport.calls.size(), 2u);
        EXPECT_EQ(f.view(clock(v.at + 1)).local_total, (fx::Usage{2, 0}));
    }
}

TEST(ExchangeRateApi, G10_WorldwideDayEndAndRollingElapsedGateRequireSeparateClocks) {
    for (const bool reset_epoch : {false, true}) {
        SCOPED_TRACE(reset_epoch);
        Fixture f;
        ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
        const auto initial = clock(0, utc(2026y / November / 16, 10h));
        const auto boundary = utc(2026y / November / 18, 12h);
        constexpr std::int64_t first_tick = 180'000'000;
        const auto exhausted =
            f.exchange(quota("g10-initial"), initial, quota_reply(30'000, 0, initial, "17"));
        ASSERT_TRUE(std::holds_alternative<fx::Quota>(exhausted.outcome));
        expect_report(f.view(initial), 30'000, 0, hint17, 1, 0);
        expect_blocked(f, quota("g10-before-day-end"), clock(first_tick - 1, boundary - 1),
                       quota_hold);

        const auto first = f.start(quota("g10-first-probe", fx::RequestOrigin::manual),
                                   clock(first_tick, boundary), sent(first_tick));
        EXPECT_TRUE(first.admission.recovery_probe);
        expect_same_selection_probe_blocks(f, quota("g10-auto"), clock(first_tick, boundary));
        const auto failed = f.finish(
            first, provider_error("quota-reached", clock(first_tick + 250, boundary + 250), 429));
        expect_failure(failed, quota_hold);
        expect_report(f.view(clock(first_tick + 250)), 30'000, 0, hint17, 2, 1);
        EXPECT_TRUE(f.view(clock(first_tick + 250)).quota_held);

        expect_blocked(f, quota("g10-before-slot"),
                       clock(first_tick + day_ms - 1, boundary + day_ms), quota_hold);
        expect_blocked(f, latest("g10-rate-still-held"),
                       clock(first_tick + day_ms, boundary + day_ms), quota_hold);
        const auto eligible = clock(first_tick + day_ms, boundary + day_ms);
        if (reset_epoch) {
            const fx::Clocks reset{eligible.utc_ms,
                                   fx::Elapsed{"new-clock-epoch", first_tick + day_ms}};
            expect_blocked(f, quota("g10-new-epoch"), reset, quota_hold);
            EXPECT_TRUE(f.view(reset).quota_held);
            EXPECT_EQ(f.transport.calls.size(), 2u);
            continue;
        }
        const auto recovered =
            f.exchange(quota("g10-next-probe"), eligible, quota_reply(30'000, 100, eligible, "17"));
        const auto *sample = std::get_if<fx::Quota>(&recovered.outcome);
        ASSERT_NE(sample, nullptr);
        ASSERT_GT(sample->requests_remaining, 0u);
        EXPECT_TRUE(recovered.admission.recovery_probe);
        EXPECT_EQ(recovered.dispatch.kind, fx::DispatchKind::sent);
        expect_report(f.view(eligible), 30'000, 100, hint17, 3, 0);
        EXPECT_FALSE(f.view(eligible).quota_held);
        EXPECT_EQ(f.transport.calls.size(), 3u);
    }
}

TEST(ExchangeRateApi, G11_DispatchEvidenceAndReplayDriveUsageIndependentlyOfFailures) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    const auto first_request = latest("g11-first");
    const auto unknown = f.exchange(first_request, clock(4'900),
                                    provider_error("future-upstream-error", clock(5'000)));
    const auto *failure = std::get_if<fx::Failure>(&unknown.outcome);
    ASSERT_NE(failure, nullptr);
    EXPECT_EQ(failure->kind, failed_sync);
    EXPECT_NE(failure->safe_code, "future-upstream-error");
    EXPECT_EQ(unknown.dispatch.kind, fx::DispatchKind::sent);
    EXPECT_FALSE(f.view(clock(5'000)).quota_held);
    EXPECT_EQ(f.view(clock(5'000)).local_total, (fx::Usage{1, 0}));
    expect_blocked(f, latest("g11-early"), clock(64'999), retry_wait);

    const auto second_request = latest("g11-handoff-timeout");
    const fx::SendTime upper{fx::SendTimeKind::upper_bound, {"fixture-epoch", 65'020}};
    const auto second =
        f.start(second_request, clock(65'000), {fx::DispatchKind::possibly_sent, upper});
    const auto timed_out =
        f.finish(second, fx::FailedTransport{fx::TransportFailure::timeout, clock(65'030)});
    expect_failure(timed_out, failed_sync);
    EXPECT_EQ(timed_out.dispatch.kind, fx::DispatchKind::possibly_sent);
    EXPECT_EQ(timed_out.dispatch.time, upper);
    EXPECT_EQ(f.view(clock(65'030)).local_total, (fx::Usage{2, 1}));

    const auto third_request = latest("g11-pre-send-cancellation");
    const auto tick = 65'030 + 5 * minute_ms;
    expect_blocked(f, latest("g11-second-retry-early"), clock(tick - 1), retry_wait);
    const auto third = f.start(third_request, clock(tick), {fx::DispatchKind::not_sent});
    const auto unsent =
        f.finish(third, fx::FailedTransport{fx::TransportFailure::cancelled, clock(tick)});
    expect_failure(unsent, failed_sync);
    EXPECT_EQ(unsent.dispatch.kind, fx::DispatchKind::not_sent);
    EXPECT_FALSE(unsent.dispatch.time);

    for (const auto &request : {first_request, second_request, third_request})
        expect_blocked(f, request, clock(tick + 60 * minute_ms), replay);
    const auto view = f.view(clock(tick + 60 * minute_ms));
    EXPECT_EQ(view.local_total, (fx::Usage{2, 1}));
    EXPECT_FALSE(view.sample);
    EXPECT_FALSE(view.quota_held);
    EXPECT_EQ(f.transport.calls.size(), 3u);
}

TEST(ExchangeRateApi, CompleteHttpReplyConfirmsAnIndeterminateHandoffWithoutAnotherCharge) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    const auto pending =
        f.start(latest("send-then-confirm"), clock(10), {fx::DispatchKind::possibly_sent});
    EXPECT_EQ(pending.dispatch.kind, fx::DispatchKind::possibly_sent);
    EXPECT_EQ(f.view(clock(10)).local_total, (fx::Usage{1, 1}));
    const auto result = f.finish(pending, frame(std::string(keyed_latest_body), clock(20)));
    EXPECT_EQ(result.dispatch.kind, fx::DispatchKind::sent);
    EXPECT_TRUE(std::holds_alternative<fx::Rate>(result.outcome));
    EXPECT_EQ(f.view(clock(20)).local_total, (fx::Usage{1, 0}));
    EXPECT_EQ(f.transport.calls.size(), 1u);
}

TEST(ExchangeRateApi, G13_SkipMonthWithoutDay31AndMissingHintSupersedesOldDay) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    const auto start = utc(2026y / April / 29, 10h);
    const auto threshold = utc(2026y / June / 1, 12h);
    const auto initial_at = clock(0, start);
    const auto initial =
        f.exchange(quota("g13-first"), initial_at, quota_reply(30'000, 0, initial_at, "31"));
    ASSERT_TRUE(std::holds_alternative<fx::Quota>(initial.outcome));
    expect_report(f.view(initial_at), 30'000, 0, hint31, 1, 0);

    const auto april_end = utc(2026y / May / 1, 12h);
    expect_blocked(f, quota("g13-not-april"), clock(april_end - start, april_end), quota_hold);
    const auto probe_tick = threshold - start;
    expect_blocked(f, quota("g13-before-may-end"), clock(probe_tick - 1, threshold - 1),
                   quota_hold);
    const auto at = clock(probe_tick, threshold);
    const auto exhausted = f.exchange(quota("g13-zero-no-hint"), at, quota_reply(30'000, 0, at));
    ASSERT_TRUE(std::holds_alternative<fx::Quota>(exhausted.outcome));
    EXPECT_TRUE(exhausted.admission.recovery_probe);
    expect_report(f.view(at), 30'000, 0, no_hint, 2, 0);
    EXPECT_TRUE(f.view(at).quota_held);
    expect_blocked(f, quota("g13-slot-without-hint"),
                   clock(probe_tick + day_ms, threshold + day_ms), quota_hold);
    EXPECT_EQ(f.transport.calls.size(), 2u);
}

TEST(ExchangeRateApi, G14_LaterUtcAnchorAtExactBoundarySkipsToNextMonth) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    const auto admission_at = clock(0);
    const auto result =
        f.exchange(quota("g14-first"), admission_at, quota_reply(30'000, 0, admission_at, "17"));
    const auto *sample = std::get_if<fx::Quota>(&result.outcome);
    ASSERT_NE(sample, nullptr);
    EXPECT_FALSE(sample->receipt.utc_ms);
    EXPECT_EQ(sample->receipt.elapsed, admission_at.elapsed);
    expect_report(f.view(admission_at), 30'000, 0, hint17, 1, 0);

    expect_blocked(f, quota("g14-no-utc"), clock(day_ms), quota_hold);
    const auto november_end = utc(2026y / November / 18, 12h);
    const auto december_end = utc(2026y / December / 18, 12h);
    expect_blocked(f, quota("g14-first-utc"), clock(2 * day_ms, november_end), quota_hold);
    const auto december_tick = 2 * day_ms + december_end - november_end;
    expect_blocked(f, quota("g14-early-december"), clock(december_tick - 1, december_end - 1),
                   quota_hold);
    const auto eligible = clock(december_tick, december_end);
    const auto probe =
        f.exchange(quota("g14-probe"), eligible, provider_error("quota-reached", eligible, 429));
    expect_failure(probe, quota_hold);
    EXPECT_TRUE(probe.admission.recovery_probe);
    expect_report(f.view(eligible), 30'000, 0, hint17, 2, 1);
    EXPECT_TRUE(f.view(eligible).quota_held);
    expect_blocked(f, quota("g14-slot"), clock(december_tick + day_ms - 1, december_end + day_ms),
                   quota_hold);
    EXPECT_EQ(f.transport.calls.size(), 2u);
}

TEST(ExchangeRateApi, G15_IndependentQuotaScopeIgnoresSupersededReplyAndPreservesOriginalGate) {
    Fixture f;
    const auto a = setup(fx::Profile::pro);
    ASSERT_EQ(f.select(a), fx::SelectionStatus::selected);
    ASSERT_NO_FATAL_FAILURE(initial_exhaustion(f));
    ASSERT_EQ(f.select(entitlement_b()), fx::SelectionStatus::selected);
    const auto b = f.start(quota("g15-B"), clock(10'000), sent(10'000));
    EXPECT_TRUE(b.admission.recovery_probe);

    const auto t = setup(fx::Profile::pro, "T", "T");
    ASSERT_EQ(f.select(t), fx::SelectionStatus::selected);
    const auto t_at = clock(11'000);
    const auto empty = f.view(t_at);
    EXPECT_EQ(empty.local_total, (fx::Usage{0, 0}));
    EXPECT_FALSE(empty.sample);
    EXPECT_FALSE(empty.quota_held);
    const auto fresh_t = f.exchange(quota("g15-first-T"), t_at, quota_reply(30'000, 4, t_at));
    ASSERT_TRUE(std::holds_alternative<fx::Quota>(fresh_t.outcome));
    EXPECT_FALSE(fresh_t.admission.recovery_probe);
    expect_report(f.view(t_at), 30'000, 4, no_hint, 1, 0);

    const auto stale_b = f.finish(b, quota_reply(30'000, 99, clock(12'000)));
    expect_failure(stale_b, superseded);
    EXPECT_EQ(stale_b.admission, b.admission);
    EXPECT_EQ(stale_b.dispatch.time, sent(10'000).time);
    expect_report(f.view(clock(12'000)), 30'000, 4, no_hint, 1, 0);
    EXPECT_FALSE(f.view(clock(12'000)).quota_held);
    const auto s = f.adapter.quota_view(a.selection.scope, clock(12'000));
    expect_report(s, 30'000, 0, no_hint, 2, 1);
    EXPECT_TRUE(s.quota_held);
    ASSERT_EQ(f.transport.calls.size(), 3u);

    const auto changed_s =
        setup(fx::Profile::business, "S-return", "S", fx::AuthorizedChange::plan_entitlement);
    ASSERT_EQ(f.select(changed_s), fx::SelectionStatus::selected);
    expect_blocked(f, quota("g15-S-early"), clock(86'409'999), quota_hold);
    const auto again = f.start(quota("g15-S-eligible"), clock(86'410'000), sent(86'410'000));
    EXPECT_TRUE(again.admission.recovery_probe);
    EXPECT_EQ(again.admission.selection, changed_s.selection);
    EXPECT_EQ(f.transport.calls.size(), 4u);
}

TEST(ExchangeRateApi, UnsentProbePreservesEligibilityAndUncertainProbeUsesSendUpperBound) {
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    ASSERT_NO_FATAL_FAILURE(initial_exhaustion(f));
    ASSERT_EQ(f.select(entitlement_b()), fx::SelectionStatus::selected);
    expect_blocked(f, quota("missing-elapsed"), {january, {}}, quota_hold);
    const auto unsent = f.start(quota("pre-send"), clock(100), {fx::DispatchKind::not_sent});
    const auto cancelled =
        f.finish(unsent, fx::FailedTransport{fx::TransportFailure::cancelled, clock(100)});
    EXPECT_TRUE(cancelled.admission.recovery_probe);
    EXPECT_EQ(cancelled.dispatch.kind, fx::DispatchKind::not_sent);
    expect_failure(cancelled, failed_sync);
    expect_report(f.view(clock(100)), 30'000, 0, no_hint, 1, 0);
    expect_blocked(f, quota("before-retry"), clock(60'099), retry_wait);

    const fx::SendTime upper{fx::SendTimeKind::upper_bound, {"fixture-epoch", 60'150}};
    const auto uncertain =
        f.start(quota("possibly-sent"), clock(60'100), {fx::DispatchKind::possibly_sent, upper});
    const auto timeout =
        f.finish(uncertain, fx::FailedTransport{fx::TransportFailure::timeout, clock(60'200)});
    EXPECT_TRUE(timeout.admission.recovery_probe);
    EXPECT_EQ(timeout.dispatch.kind, fx::DispatchKind::possibly_sent);
    EXPECT_EQ(timeout.dispatch.time, upper);
    EXPECT_EQ(f.view(clock(60'200)).local_total, (fx::Usage{2, 1}));
    EXPECT_EQ(f.view(clock(60'200)).local_since_report, (fx::Usage{1, 1}));
    const auto c = setup(fx::Profile::business, "C", "S", fx::AuthorizedChange::plan_entitlement);
    ASSERT_EQ(f.select(c), fx::SelectionStatus::selected);
    // A new entitlement cannot borrow a slot from the reservation tick or receipt tick.
    expect_blocked(f, quota("slot-no-elapsed"), {january + 2 * day_ms, {}}, quota_hold);
    expect_blocked(f, quota("before-upper-bound"), clock(60'150 + day_ms - 1), quota_hold);
    const auto eligible = clock(60'150 + day_ms);
    const auto recovered =
        f.exchange(quota("at-upper-bound"), eligible, quota_reply(125'000, 1, eligible));
    const auto *sample = std::get_if<fx::Quota>(&recovered.outcome);
    ASSERT_NE(sample, nullptr);
    ASSERT_GT(sample->requests_remaining, 0u);
    EXPECT_TRUE(recovered.admission.recovery_probe);
    const auto view = f.view(eligible);
    EXPECT_EQ(view.local_total, (fx::Usage{3, 1}));
    EXPECT_EQ(view.local_since_report, (fx::Usage{0, 0}));
    EXPECT_FALSE(view.quota_held);
}

TEST(ExchangeRateApi, QuotaCountsRequireCanonicalBoundedIntegers) {
    for (const std::string remaining :
         {"1e0", "1.0", "-0", "\"1\"", "18446744073709551616", "30001"}) {
        SCOPED_TRACE(remaining);
        Fixture f;
        ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
        const auto body =
            R"({"result":"success","plan_quota":30000,"requests_remaining":)" + remaining + "}";
        expect_failure(f.exchange(quota("invalid-count"), clock(0), frame(body, clock(1))),
                       invalid_document);
        const auto view = f.view(clock(1));
        EXPECT_EQ(view.local_total, (fx::Usage{1, 0}));
        EXPECT_FALSE(view.sample);
        EXPECT_FALSE(view.quota_held);
    }
    Fixture f;
    ASSERT_EQ(f.select(setup(fx::Profile::pro)), fx::SelectionStatus::selected);
    constexpr std::uint64_t largest = 18'446'744'073'709'551'615ull;
    const auto filled =
        f.exchange(quota("largest-count"), clock(0), quota_reply(largest, largest, clock(1), "1"));
    ASSERT_TRUE(std::holds_alternative<fx::Quota>(filled.outcome));
    expect_report(f.view(clock(1)), largest, largest, {fx::RenewalState::valid, 1}, 1, 0);
}

} // namespace
