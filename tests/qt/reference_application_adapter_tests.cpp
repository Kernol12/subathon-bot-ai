// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/reference_application_adapter_fixture.hpp"
#include <chrono>
#include <future>
using namespace reference_adapter_fixture;
namespace {
ApplicationReferenceBinding
reference_binding(ApplicationReferencePurpose purpose = ApplicationReferencePurpose::after_state) {
    const auto c = command();
    return {c.operation, {c.subject_kind, c.receipt_id, c.contribution_id}, c.plan, "a", purpose,
            {}};
}
void denied(const AccessDecision &r) {
    require(std::holds_alternative<ApplicationAccessDenied>(r), "Fresh complete authority denies");
}
void unavailable(const ReferenceConfigurationResolution &r, ReferenceGate g) {
    const auto *u = std::get_if<ReferenceUnavailable>(&r);
    require(u && u->gate == g &&
                u->reason_ref == (g == ReferenceGate::not_ready
                                      ? "reference.configuration-unavailable.v1"
                                      : "reference.configuration-unsupported.v1"),
            "Exact unknown/unsupported gate and reason");
}
} // namespace
class ReferenceApplicationAdapterTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qt_test_support::fail_on_sql_warnings(); }
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void archiveOwnsWholeImmutableVersionsAndOriginalOwnerProvenance() {
        scenario([] {
            auto input = versions();
            ReferenceConfigurationArchive archive(std::move(input));
            auto first = archive.snapshot("C");
            const auto *f = std::get_if<ReferenceConfigurationFacts>(&first);
            require(f && f->configuration_ref == "C" &&
                        f->configuration_owner_revision_ref == "owner-C" &&
                        f->enabled_rules.size() == 1 && f->enabled_rules[0].rule.add_ms == 600000 &&
                        !f->projection.maximum_main_ms && f->modifiers.enabled_refs.empty(),
                    "Complete original version, never today's narrow projection");
            std::get<ReferenceConfigurationFacts>(first).enabled_rules[0].rule.add_ms = 1;
            const auto again = archive.snapshot("C");
            require(std::get<ReferenceConfigurationFacts>(again).enabled_rules[0].rule.add_ms ==
                            600000 &&
                        std::get<ReferenceConfigurationFacts>(archive.snapshot("D"))
                                .projection.maximum_main_ms == 3600000,
                    "Owned output cannot mutate retained old/current versions");
            unavailable(archive.snapshot("missing"), ReferenceGate::not_ready);
        });
    }
    void archiveUnknownUnsupportedAndLaterMalformedPriority_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("unknown-whole-category") << 0;
        QTest::newRow("enabled-unsupported-modifier") << 1;
        QTest::newRow("later-malformed-after-unsupported") << 2;
        QTest::newRow("projection-complete-while-unknown") << 3;
        QTest::newRow("duplicate-owned-version") << 4;
        QTest::newRow("enabled-outbound-action") << 5;
        QTest::newRow("enabled-milestone") << 6;
        QTest::newRow("second-rule") << 7;
        QTest::newRow("negative-add-ms") << 8;
    }
    void archiveUnknownUnsupportedAndLaterMalformedPriority() {
        QFETCH(int, kind);
        scenario([&] {
            auto v = versions();
            auto &f = std::get<ReferenceConfigurationFacts>(v[1].contents);
            if (kind == 0) {
                f.generated_events.state = ReferenceFactsState::unknown;
                f.projection.generated_events = ReferenceFactsState::unknown;
            }
            if (kind == 1 || kind == 2)
                f.modifiers.enabled_refs = {"enabled"};
            if (kind == 2)
                f.generated_events.state = static_cast<ReferenceFactsState>(255);
            if (kind == 3)
                f.generated_events.state = ReferenceFactsState::unknown;
            if (kind == 4)
                v.push_back(v[0]);
            if (kind == 5)
                f.outbound_actions.enabled_refs = {"action"};
            if (kind == 6)
                f.milestones.enabled_refs = {"milestone"};
            if (kind == 7) {
                auto second = f.enabled_rules[0];
                second.rule.rule_ref = "second";
                f.enabled_rules.push_back(second);
            }
            if (kind == 8) {
                f.projection.rule.add_ms = -1;
                f.enabled_rules[0].rule.add_ms = -1;
            }
            ReferenceConfigurationArchive archive(std::move(v));
            if (kind < 2 || (kind >= 5 && kind <= 7))
                unavailable(archive.snapshot("D"), kind == 0
                                                       ? ReferenceGate::not_ready
                                                       : ReferenceGate::unsupported_configuration);
            else
                input_failure([&] { (void)archive.snapshot("C"); },
                              kind == 2   ? ReferenceInputError::invalid_tag
                              : kind == 8 ? ReferenceInputError::invalid_value
                                          : ReferenceInputError::invalid_binding);
        });
    }
    void archiveSnapshotInvalidLimitsBeforeMalformedInput() {
        scenario([] {
            ReferenceAdapterLimits l;
            l.value_items = 0;
            ReferenceConfigurationArchive archive(versions(), l);
            input_failure([&] { (void)archive.snapshot(""); }, ReferenceInputError::invalid_limits);
        });
    }
    void archiveWholeInputByteAndItemBoundaryPairs() {
        scenario([] {
            // Independent tally for ONE C version: facts B=130, I=4 (one rule,
            // one trigger/action/unit each); outer version ref C adds1, lookup C
            // adds1, outer vector adds1 item => B132/I5. See handoff derivation.
            const std::vector<ReferenceConfigurationVersion> v{{"C", facts()}};
            ReferenceAdapterLimits l;
            l.value_owned_bytes = 132;
            l.value_items = 5;
            ReferenceConfigurationArchive exact(v, l);
            require(std::holds_alternative<ReferenceConfigurationFacts>(exact.snapshot("C")),
                    "Exact whole archive B132/I5 fits");
            --l.value_owned_bytes;
            ReferenceConfigurationArchive bytes(v, l);
            input_failure([&] { (void)bytes.snapshot("C"); }, ReferenceInputError::input_limit);
            ++l.value_owned_bytes;
            --l.value_items;
            ReferenceConfigurationArchive items(v, l);
            input_failure([&] { (void)items.snapshot("C"); }, ReferenceInputError::input_limit);
        });
    }
    void archiveLoadUsesActualFiniteViewAndExactRetainedVersion() {
        scenario([] {
            Rig r;
            r.probe([&](const auto &v, const auto &) {
                const auto f = r.configuration.load(v, "C");
                require(
                    std::get<ReferenceConfigurationFacts>(f).projection.rule.add_ms == 600000 &&
                        std::get<ReferenceConfigurationFacts>(f).configuration_owner_revision_ref ==
                            "owner-C",
                    "Actual finite read validates captured SRC1 against complete owner DATA");
            });
        });
    }
    void archiveDefaultCopiedExpiredAndForeignViewsPreserveLifetime() {
        scenario([] {
            Rig r;
            ApplicationReferenceReadView escaped;
            r.probe([&](const auto &v, const auto &) {
                const auto copy = v;
                escaped = copy;
                require(std::holds_alternative<ReferenceConfigurationFacts>(
                            r.configuration.load(copy, "C")),
                        "Live copied view works in callback");
                const auto code = std::async(std::launch::async, [&] {
                                      try {
                                          (void)r.configuration.load(copy, "C");
                                      } catch (const ApplicationReferenceViewFailure &e) {
                                          return int(e.error());
                                      }
                                      return -1;
                                  }).get();
                require(code == int(ApplicationReferenceViewError::wrong_thread),
                        "Foreign valid view preserves original error");
            });
            for (const auto &view : {ApplicationReferenceReadView{}, escaped}) {
                bool caught = false;
                try {
                    (void)r.configuration.load(view, "C");
                } catch (const ApplicationReferenceViewFailure &e) {
                    caught = e.error() == ApplicationReferenceViewError::expired_callback;
                }
                require(caught, "Default/expired copied view cannot extend callback");
            }
        });
    }
    void archiveMissingRetainedAndContradictoryFactsPreserveChannels_data() {
        QTest::addColumn<bool>("contradictory");
        QTest::newRow("missing-selected-retained") << false;
        QTest::newRow("retained-SRC1-contradicts-facts") << true;
    }
    void archiveMissingRetainedAndContradictoryFactsPreserveChannels() {
        QFETCH(bool, contradictory);
        scenario([&] {
            Rig r;
            auto f = facts(contradictory ? "C" : "E");
            if (contradictory) {
                f.projection.rule.add_ms = 1;
                f.enabled_rules[0].rule.add_ms = 1;
            }
            ReferenceConfigurationArchive archive({{f.configuration_ref, f}});
            r.probe([&](const auto &v, const auto &) {
                if (!contradictory) {
                    const auto loaded = archive.load(v, "E");
                    const auto *u = std::get_if<ReferenceUnavailable>(&loaded);
                    require(u && u->gate == ReferenceGate::not_ready &&
                                u->reason_ref == "reference.configuration-unavailable.v1",
                            "Missing exact retained row, not a narrow facts permit");
                } else {
                    bool caught = false;
                    try {
                        (void)archive.load(v, "C");
                    } catch (const ReferenceExecutionFailure &e) {
                        caught = e.code() == ReferenceAuthorityFailure::invalid_data;
                    }
                    require(caught, "Contradictory retained semantics retain typed invalid_data");
                }
            });
        });
    }
    void archivePureInputThenViewThenConstructionAffinity() {
        scenario([] {
            Rig r;
            r.probe([&](const auto &v, const auto &) {
                auto pure = std::async(std::launch::async, [&] {
                    input_failure([&] { (void)r.configuration.load(v, ""); },
                                  ReferenceInputError::invalid_reference);
                });
                // get also preserves the exact test failure rather than laundering it.
                pure.get();
                auto foreign =
                    std::async(std::launch::async, [] {
                        return std::make_unique<ReferenceConfigurationArchive>(versions());
                    }).get();
                bool caught = false;
                try {
                    (void)foreign->load(v, "C");
                } catch (const SqliteWorkFailure &e) {
                    caught = e.error() == WorkError::wrong_thread;
                }
                require(caught, "Valid owner-thread view then foreign archive affinity error");
            });
        });
    }
    void stateDecodeOwning90ByteBasisPresentAnchor() {
        scenario([] {
            const ApplicationStateSnapshot snapshot{
                "b",
                {"subathon.internal.application-state.v1",
                 QByteArray::fromHex(
                     "534153310000000163010000000161000000016c000000017400000000000003e8"
                     "00000001000000000000000000000000000003e80000000001760100"
                     "000000000000003a000000000000005a00000000000000000000000000")},
                "c",
                {TimerLifecycle::Active, CountdownPhase::Main, 58, 90, false},
                0,
                {}};
            require(snapshot.envelope.document.size() == 90,
                    "Owning §E independent literal length");
            const ReferenceCommittedState expected{
                snapshot, "a", {"l", "t", "c", "v", "a", 1000, {{0, 1000, false}}}, "v"};
            same_state(decode_reference_state(snapshot), expected);
        });
    }
    void stateEncodeMatchesLiteralBytesWithAbsentInitialBasis() {
        scenario([] {
            const auto literal = committed("initial");
            const auto encoded = encode_reference_state(literal);
            require(encoded.envelope.format_ref == "subathon.internal.application-state.v1" &&
                        encoded.envelope.document == literal.snapshot.envelope.document &&
                        encoded.envelope.document.size() == 68,
                    "Exact independent SAS1, not a round-trip oracle");
        });
    }
    void stateDecodeLiteralSeparatelyChecksDuplicates() {
        scenario([] {
            const auto literal = committed("initial");
            const auto decoded = decode_reference_state(literal.snapshot);
            same_state(decoded, literal);
            require(!decoded.basis && decoded.policy_revision_ref == "W" &&
                        decoded.observation.clock_lineage_ref == "L" &&
                        decoded.observation.observation_ref == "O" &&
                        decoded.observation.observed_utc_ms == 1000 &&
                        decoded.snapshot.timer.main_remaining_ms == 3480000,
                    "Literal physical SAS1 meaning, no fixture encoder");
            auto bad = literal.snapshot;
            bad.timer.main_remaining_ms = 2;
            input_failure([&] { (void)decode_reference_state(bad); },
                          ReferenceInputError::invalid_binding);
            bad = literal.snapshot;
            bad.envelope.document += 'x';
            input_failure([&] { (void)decode_reference_state(bad); },
                          ReferenceInputError::invalid_document);
        });
    }
    void stateDocumentExactBoundaryAndFormatPriority() {
        scenario([] {
            const auto s = committed("initial").snapshot;
            ReferenceAdapterLimits l;
            l.document_bytes = 68;
            require(decode_reference_state(s, l).snapshot.timer.main_remaining_ms == 3480000,
                    "Exact document68 fits");
            --l.document_bytes;
            input_failure([&] { (void)decode_reference_state(s, l); },
                          ReferenceInputError::input_limit);
            auto unknown = s;
            unknown.envelope.format_ref = "future.state.v2";
            input_failure([&] { (void)decode_reference_state(unknown, l); },
                          ReferenceInputError::input_limit);
            ++l.document_bytes;
            input_failure([&] { (void)decode_reference_state(unknown, l); },
                          ReferenceInputError::unsupported_format);
        });
    }
    void productionClockSamplesIndependentSystemAndSteadyRanges() {
        scenario([] {
            SystemReferenceClockSampler actual;
            const auto before_utc = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count();
            const auto before_mono = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now().time_since_epoch())
                                         .count();
            const auto got = actual.sample();
            const auto after_mono = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now().time_since_epoch())
                                        .count();
            const auto after_utc = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();
            const auto *s = std::get_if<ReferenceClockSample>(&got);
            require(s && s->monotonic_ms >= before_mono && s->monotonic_ms <= after_mono &&
                        s->utc_ms >= std::min(before_utc, after_utc) &&
                        s->utc_ms <= std::max(before_utc, after_utc),
                    "Two actual clock domains bounded by independent enclosing samples, no exact "
                    "timing assertion");
        });
    }
    void productionOsEntropyReturnsExact32Bytes() {
        scenario([] {
            OsReferenceEntropy actual;
            const auto got = actual.read32();
            require(got.error == ReferenceEntropyError::none && got.received == 32 &&
                        got.bytes.size() == 32,
                    "Actual Linux OS read, not random-statistics uniqueness proof");
        });
    }
    void issuerUsesPurposeDomainAndWholeBindingOncePerOutstandingScope() {
        scenario([] {
            Rig r;
            r.entropy.advance = false;
            r.entropy.next.bytes.fill(0xa5);
            const auto b = reference_binding();
            const auto got = r.adapters.issue(ApplicationReferencePurpose::after_state, b);
            const auto *v = std::get_if<IssuedApplicationReference>(&got);
            require(v && v->reference == std::string("RRF1\1", 5) + std::string(32, char(0xa5)) &&
                        v->reference.size() == 37 &&
                        v->binding.operation.operation_ref == "attempt" &&
                        v->binding.persisted_head_token == "a" && v->binding.plan.plan_ref == "P" &&
                        v->binding.source.contribution_id->value == 0 && r.entropy.calls == 1,
                    "Exact37 bytes, purpose1, actual binding, one entropy call");
            require(std::holds_alternative<ApplicationUnavailable>(
                        r.adapters.issue(ApplicationReferencePurpose::after_state, b)) &&
                        r.entropy.calls == 1,
                    "Same outstanding reservation scope consumes binding/purpose once; no entropy "
                    "retry");
        });
    }
    void issuerRollbackClosesScopeButPreviouslyIssuedBytesRemainSpent() {
        scenario([] {
            Rig r;
            r.entropy.advance = false;
            r.entropy.next.bytes.fill(0x10);
            require(!r.adapters.reserve(committed(), command(), ReferenceZeroStep::fresh),
                    "First live outstanding reservation scope");
            r.entropy.next.bytes.fill(0x11);
            const auto first =
                r.adapters.issue(ApplicationReferencePurpose::after_state, reference_binding());
            require(std::holds_alternative<IssuedApplicationReference>(first),
                    "First after-token issued once");
            ApplicationServiceCompletion rollback;
            rollback.transaction = TransactionOutcome::rolled_back;
            rollback.work_error = WorkError::sql_failure;
            r.adapters.settle(rollback, {});
            r.entropy.next.bytes.fill(0x12);
            require(!r.adapters.reserve(committed(), command(), ReferenceZeroStep::fresh),
                    "Explicit same-command retry has new reservation bytes");
            r.entropy.next.bytes.fill(0x11);
            const auto before = r.entropy.calls;
            const auto repeated =
                r.adapters.issue(ApplicationReferencePurpose::after_state, reference_binding());
            const auto *u = std::get_if<ApplicationUnavailable>(&repeated);
            require(
                u && u->reason_ref == "reference.collision.v1" && r.entropy.calls == before + 1,
                "Spent after-token bytes refuse even after known rollback; no automatic redraw");
        });
    }
    void issuerShortErrorAndTooSmallProfileRefuseWithoutFallback_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("short31") << 0;
        QTest::newRow("os-error") << 1;
        QTest::newRow("reference-ceiling36") << 2;
    }
    void issuerShortErrorAndTooSmallProfileRefuseWithoutFallback() {
        QFETCH(int, kind);
        scenario([&] {
            ReferenceRuntimeLimits l;
            if (kind == 2)
                l.values.reference_bytes = 36;
            Rig r("standard", l);
            if (kind == 0)
                r.entropy.next = {{}, 31, ReferenceEntropyError::short_read};
            if (kind == 1)
                r.entropy.next = {{}, 0, ReferenceEntropyError::unavailable};
            require(std::holds_alternative<ApplicationUnavailable>(r.adapters.issue(
                        ApplicationReferencePurpose::after_state, reference_binding())) &&
                        r.entropy.calls == (kind == 2 ? 0u : 1u),
                    "Typed entropy/profile refusal, bounded one attempt");
        });
    }
    void issuerInvalidPurposeBeforeEntropy() {
        scenario([] {
            Rig r;
            input_failure(
                [&] {
                    (void)r.adapters.issue(static_cast<ApplicationReferencePurpose>(255),
                                           reference_binding());
                },
                ReferenceInputError::invalid_tag);
            require(r.entropy.calls == 0, "No entropy for invalid tag");
        });
    }
    void reserveZeroBranchesElapsedAndUtcAreIndependent_data() {
        QTest::addColumn<qlonglong>("elapsed");
        QTest::addColumn<qlonglong>("utc");
        QTest::addColumn<bool>("fresh");
        QTest::newRow("RC03-reuse-old") << qlonglong(0) << qlonglong(1000) << false;
        QTest::newRow("RC03-explicit-zero") << qlonglong(0) << qlonglong(1000) << true;
        QTest::newRow("RC03-changed-utc") << qlonglong(0) << qlonglong(1001) << false;
        QTest::newRow("RC01-independent-utc-backward")
            << qlonglong(30000) << qlonglong(-5000) << true;
    }
    void reserveZeroBranchesElapsedAndUtcAreIndependent() {
        QFETCH(qlonglong, elapsed);
        QFETCH(qlonglong, utc);
        QFETCH(bool, fresh);
        scenario([&] {
            Rig r;
            const auto input = r.file.inputs();
            r.sampler.next = ReferenceClockSample{100 + elapsed, utc};
            require(!r.adapters.reserve(committed(), command(),
                                        fresh ? ReferenceZeroStep::fresh
                                              : ReferenceZeroStep::reuse_unchanged),
                    "Live reservation admitted");
            const auto sampled = r.sampler.calls;
            require(r.entropy.calls == 1,
                    "EVERY successful reservation draws one RCO1, even reuse");
            r.probe([&](const auto &v, const auto &c) {
                auto p = policy(r, v, c, input);
                const auto got = r.adapters.observe(v, c, input, p);
                const auto *o = std::get_if<ApplicationClockObservation>(&got);
                require(o && o->observed_utc_ms == utc && r.sampler.calls == sampled &&
                            o->checked_head_token == "a" && o->configuration_ref == "D" &&
                            o->policy_revision_ref == "W" && r.entropy.calls == 1,
                        "Consume exact reserved candidate, never resample or redraw");
                const bool reuse = elapsed == 0 && utc == 1000 && !fresh;
                require(o->segments.size() == (reuse ? 0u : 1u) &&
                            (reuse ? o->observation_ref == "O"
                                   : o->observation_ref.starts_with("RCO1")),
                        "Explicit empty/reuse versus fresh/step branches");
                if (!reuse)
                    require(o->segments[0].elapsed_ms == elapsed &&
                                o->segments[0].utc_now_ms == utc && !o->segments[0].paused,
                            "Actual independent elapsed segment");
            });
        });
    }
    void observeWithoutReservationRefusesAtOwnApi() {
        scenario([] {
            Rig r;
            const auto i = r.file.inputs();
            r.probe([&](const auto &v, const auto &c) {
                require(std::holds_alternative<ApplicationUnavailable>(
                            r.adapters.observe(v, c, i, policy(r, v, c, i))) &&
                            r.sampler.calls == 0,
                        "Missing candidate is not fabricated elapsed0; direct own NI witness");
            });
        });
    }
    void reserveDiscontinuousExpiredMissingAndWrongHead_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("monotonic-backward") << 0;
        QTest::newRow("elapsed-equals-main") << 1;
        QTest::newRow("clock-unavailable") << 2;
        QTest::newRow("wrong-head") << 3;
        QTest::newRow("wrong-policy") << 4;
    }
    void reserveDiscontinuousExpiredMissingAndWrongHead() {
        QFETCH(int, kind);
        scenario([&] {
            Rig r;
            auto c = committed();
            if (kind == 0)
                r.sampler.next = ReferenceClockSample{99, 1000};
            if (kind == 1)
                r.sampler.next = ReferenceClockSample{3480100, 1000};
            if (kind == 2)
                r.sampler.next =
                    ReferenceUnavailable{ReferenceGate::not_ready, "reference.clock.v1"};
            if (kind == 3)
                c.snapshot.token = "other";
            if (kind == 4)
                c.policy_revision_ref = "other";
            if (kind < 3) {
                const auto refused = r.adapters.reserve(c, command());
                require(refused && refused->gate == ReferenceGate::not_ready &&
                            refused->reason_ref == (kind == 2 ? "reference.clock.v1"
                                                              : "reference.runtime-unavailable.v1"),
                        "Exact unknown/discontinuity/expiry gate and reason");
            } else
                input_failure([&] { (void)r.adapters.reserve(c, command()); },
                              ReferenceInputError::invalid_binding);
        });
    }
    void journalExactCapacityPreservesAnchorAndOutstandingReservation() {
        scenario([] {
            ReferenceRuntimeLimits l;
            l.values.clock_journal_entries = 2;
            l.pending_reservations = 2;
            Rig r("standard", l);
            require(!r.adapters.reserve(committed(), command(), ReferenceZeroStep::fresh),
                    "Anchor+one exact reservation fits2");
            const auto clocks = r.sampler.calls, entropy = r.entropy.calls;
            const auto refusal =
                r.adapters.reserve(committed(), command(), ReferenceZeroStep::fresh);
            require(refusal && refusal->gate == ReferenceGate::not_ready &&
                        refusal->reason_ref == "reference.journal-capacity.v1" &&
                        r.sampler.calls == clocks && r.entropy.calls == entropy,
                    "One over refuses BEFORE sampling, no eviction of anchor/outstanding work");
        });
    }
    void journalPendingSlotsAreIndependentOfEntryCapacity() {
        scenario([] {
            ReferenceRuntimeLimits l;
            l.values.clock_journal_entries = 3;
            l.pending_reservations = 1;
            Rig r("standard", l);
            require(!r.adapters.reserve(committed(), command(), ReferenceZeroStep::fresh),
                    "One pending slot fits with a spare journal entry");
            const auto clocks = r.sampler.calls, entropy = r.entropy.calls;
            const auto refused =
                r.adapters.reserve(committed(), command("second"), ReferenceZeroStep::fresh);
            require(refused && refused->gate == ReferenceGate::not_ready &&
                        refused->reason_ref == "reference.journal-capacity.v1" &&
                        r.sampler.calls == clocks && r.entropy.calls == entropy,
                    "Pending1 bound alone rejects before work");
        });
    }
    void journalKnownRollbackRetainsSpentTombstone() {
        scenario([] {
            ReferenceRuntimeLimits l;
            l.values.clock_journal_entries = 2;
            l.pending_reservations = 2;
            Rig r("standard", l);
            require(!r.adapters.reserve(committed(), command(), ReferenceZeroStep::fresh),
                    "Anchor plus fresh reservation exact2");
            ApplicationServiceCompletion rolled;
            rolled.transaction = TransactionOutcome::rolled_back;
            rolled.work_error = WorkError::sql_failure;
            r.adapters.settle(rolled, {});
            const auto live = r.adapters.current(committed().snapshot);
            require(std::holds_alternative<ReferenceCommittedState>(live),
                    "Known rollback retains old anchor");
            const auto clocks = r.sampler.calls, entropy = r.entropy.calls;
            const auto refused = r.adapters.reserve(committed(), command("explicit-retry"),
                                                    ReferenceZeroStep::fresh);
            require(refused && refused->reason_ref == "reference.journal-capacity.v1" &&
                        r.sampler.calls == clocks && r.entropy.calls == entropy,
                    "Spent observation still fills entry2 even though pending slot released");
        });
    }
    void committedSettlementMismatchCannotMutateJournal() {
        scenario([] {
            Rig r;
            const auto completion = r.attempt(command(), {}, false);
            const auto &record = applied(completion);
            const auto expected = checked_after(record, committed(), {100, 1000}, 1);
            auto wrong = expected;
            wrong.snapshot.token = "foreign";
            input_failure([&] { r.adapters.settle(completion, wrong); },
                          ReferenceInputError::invalid_binding);
            const auto still_old = r.adapters.current(committed().snapshot);
            require(std::holds_alternative<ReferenceCommittedState>(still_old),
                    "Mismatched committed completion leaves live anchor unchanged");
            same_state(std::get<ReferenceCommittedState>(still_old), committed());
            r.adapters.settle(completion, expected);
            same_state(std::get<ReferenceCommittedState>(r.adapters.current(expected.snapshot)),
                       expected);
        });
    }
    void runtimeProfilesRefuseCurrentAndReserve_data() {
        QTest::addColumn<QString>("profile");
        for (const auto *name :
             {"debt", "grace", "wait", "balances", "ready", "finished", "deadline"})
            QTest::newRow(name) << QString(name);
    }
    void runtimeProfilesRefuseCurrentAndReserve() {
        QFETCH(QString, profile);
        scenario([&] {
            const bool deadline = profile == "deadline";
            const auto state = committed(deadline ? "standard" : profile.toUtf8().constData());
            auto v = versions();
            if (deadline)
                std::get<ReferenceConfigurationFacts>(v[1].contents)
                    .projection.timer_config.hard_end_utc_ms = 999999;
            if (profile == "wait")
                std::get<ReferenceConfigurationFacts>(v[1].contents)
                    .projection.timer_config.end_mode = TimerEndMode::Confirmation;
            ReferenceConfigurationArchive config(std::move(v));
            SqliteReferenceAuthoritySource authority;
            Sampler sampler;
            Entropy entropy;
            ReferenceApplicationAdapters runtime(config, authority, sampler, entropy,
                                                 ReferenceClockAnchor{state, {100, 1000}});
            const auto current = runtime.current(state.snapshot);
            const auto *u = std::get_if<ReferenceUnavailable>(&current);
            const auto gate =
                deadline ? ReferenceGate::unsupported_configuration : ReferenceGate::not_ready;
            const auto reason = deadline ? "reference.configuration-unsupported.v1"
                                         : "reference.runtime-unavailable.v1";
            require(u && u->gate == gate && u->reason_ref == reason,
                    "Exact current profile refusal");
            const auto refused = runtime.reserve(state, command());
            require(refused && refused->gate == gate && refused->reason_ref == reason &&
                        sampler.calls == 0 && entropy.calls == 0,
                    "No sampling/entropy for coherent unsupported runtime/configuration");
        });
    }
    void journalEntropyCollisionIsExplicitNoRetry() {
        scenario([] {
            Rig r;
            r.entropy.advance = false;
            r.entropy.next.bytes.fill(0x5a);
            require(!r.adapters.reserve(committed(), command(), ReferenceZeroStep::fresh),
                    "First unique reservation");
            const auto refused =
                r.adapters.reserve(committed(), command(), ReferenceZeroStep::fresh);
            require(refused && refused->gate == ReferenceGate::conflict &&
                        refused->reason_ref == "reference.collision.v1" && r.entropy.calls == 2,
                    "Occupied annotation namespace, no probability/retry assertion");
        });
    }
    void settleUnknownStopsAndNeverPromotesGuessedAnchor() {
        scenario([] {
            Rig r;
            // Direct settlement of an actual unknown completion must stop even
            // without a tentative state; never requires a new reserve in setup.
            ApplicationServiceCompletion completion;
            completion.transaction = TransactionOutcome::outcome_unknown;
            completion.work_error = WorkError::uncertain_outcome;
            r.adapters.settle(completion, {});
            require(r.adapters.reserve(committed(), command()).has_value(),
                    "Unknown permanently stops new admission");
            input_failure([&] { r.adapters.reconcile(committed()); },
                          ReferenceInputError::invalid_binding);
        });
    }
    void reconcileCannotAdoptForeignLineageOrHead() {
        scenario([] {
            Rig r;
            auto bad = committed();
            bad.observation.clock_lineage_ref = "foreign-process";
            input_failure([&] { r.adapters.reconcile(bad); }, ReferenceInputError::invalid_binding);
        });
    }
    void accessUsesActualCurrentAuthorUseGrantAndExecutionUse_data() {
        QTest::addColumn<int>("channel");
        QTest::addColumn<QString>("account");
        QTest::newRow("author-USE-full600000") << 0 << QString("A");
        QTest::newRow("author-GRANT-full600000") << 1 << QString("A");
        QTest::newRow("execution-USE-full600000") << 0 << QString("E");
    }
    void accessUsesActualCurrentAuthorUseGrantAndExecutionUse() {
        QFETCH(int, channel);
        QFETCH(QString, account);
        scenario([&] {
            Rig r;
            r.probe([&](const auto &v, const auto &c) {
                require(std::holds_alternative<ApplicationAccessAllowed>(r.adapters.check(v, c)),
                        "Independent positive actual current authority control");
            });
            auto g = graph(channel == 1);
            for (auto &a : g.accounts)
                if (a.account == account.toStdString()) {
                    for (auto &e : a.individual_entries)
                        if (e.operation == "reference.time-add.v1")
                            e.quantities[0].upper = PermissionRational{"120000", "1"};
                }
            r.file.db.replace_graph(channel, g);
            r.probe([&](const auto &v, const auto &c) { denied(r.adapters.check(v, c)); });
        });
    }
    void accessQueuedRevocationUsesNewRegistryState() {
        scenario([] {
            Rig r;
            std::optional<SqliteWorkResult> revoked;
            require(
                r.file.db.writer.enqueue_work(
                    [&](auto context) -> QVariant {
                        const auto session = context.begin();
                        Cleanup cleanup{session};
                        staged(SqliteReferenceAuthorityUnit(session).set_state(
                                   actor('O'), r.file.db.clock, 3, "K",
                                   ReferenceAuthorityState::revoked),
                               4);
                        session.commit();
                        return 1;
                    },
                    [&](auto result) { revoked = std::move(result); }) == StorageError::none,
                "Revocation queues before target; neither executes until target is queued below");
            r.probe([&](const auto &v, const auto &c) { denied(r.adapters.check(v, c)); });
            require(revoked.has_value(), "Queued revocation completed before target access");
            normal(*revoked);
            require(r.file.count("subathon_app_operations") == 0,
                    "No protected result/award under revoked registry");
        });
    }
    void policyUsesCurrentCapAndRetainsHistoricalFullRequest() {
        scenario([] {
            Rig r("lower_cap");
            const auto i = r.file.inputs();
            r.probe([&](const auto &v, const auto &c) {
                auto facts = r.authority.resolve(v, request());
                const auto *f = std::get_if<ReferenceAuthorityFacts>(&facts.value);
                require(f, "SETUP: actual independent current authority producer");
                const auto got = r.adapters.resolve(
                    v, c, i, ApplicationAccessAllowed{f->permission_revision_ref});
                const auto *p = std::get_if<BoundApplicationPolicy>(&got);
                require(p && p->maximum_main_ms == 3540000 && p->configuration_ref == "D" &&
                            p->checked_state_token == "a" && p->policy_revision_ref == "W" &&
                            p->permission_revision_ref == f->permission_revision_ref &&
                            p->pause_recovery_revision == "reference.fresh-runtime.v1" &&
                            p->admission.disposition == ApplicationPolicyAdmission::allow_active &&
                            !p->paused && !p->recovery_pending && p->correction_policies.empty(),
                        "Actual current config cap, immutable captured600000 remains separate");
            });
        });
    }
    void actionsProveEmptyFromBothCompleteConfigurations() {
        scenario([] {
            Rig r;
            const auto i = r.file.inputs();
            ApplicationCalculatedProposal q;
            q.clock_advanced_timer = committed().snapshot.timer;
            q.timer_after = {TimerLifecycle::Active, CountdownPhase::Main, 3600000, 120000, false};
            q.observation = committed().observation;
            q.effects = {{1,
                          ApplicationArithmeticRole::time_addition,
                          RequestedUnit::milliseconds,
                          Exact{600000},
                          600000,
                          Exact{0},
                          3600000,
                          {},
                          {},
                          "literal",
                          {{ApplicationComponentKind::main_delta, 120000, {}},
                           {ApplicationComponentKind::debt_delta, 0, {}},
                           {ApplicationComponentKind::cap_limited_time, 480000, {}}}}};
            r.probe([&](const auto &v, const auto &c) {
                const auto got = r.adapters.prepare(v, c, i, policy(r, v, c, i), q);
                const auto *a = std::get_if<PreparedApplicationActions>(&got);
                require(a && a->jobs.empty() &&
                            a->explanation == "reference.no-external-actions.v1",
                        "Proven empty, not missing action producer default");
            });
        });
    }
    void disclosureApplicationRechecksActualHistoricalRecord() {
        scenario([] {
            Database db;
            seed_history(db);
            ReferenceConfigurationArchive config(versions());
            SqliteReferenceAuthoritySource authority;
            Sampler sampler;
            Entropy entropy;
            ReferenceApplicationAdapters adapters(config, authority, sampler, entropy, {});
            RecordedApplicationBundle historical;
            db.work([&](const auto &s) {
                const auto found = SqliteApplicationBundleStore(s).lookup({"history", "app"});
                require(found.has_value(), "SETUP: literal historical complete bundle");
                historical = *found;
            });
            Probe probe;
            bool expected = true;
            probe.f = [&](const auto &v, const auto &c) {
                require(adapters.disclose_application(v, c, historical) == expected,
                        "Actual record read permission separately revoked");
            };
            Ports ports(adapters);
            ApplicationServicePorts p{probe, ports, ports, ports, ports};
            OriginalApplicationService service(db.writer, p);
            bool done = false;
            require(service.enqueue(command(), [&](auto) { done = true; }).error ==
                        StorageError::none,
                    "OLD service callback probe");
            require(QTest::qWaitFor([&] { return done; }, 10000), "Probe completion");
            probe.witness.rethrow();
            auto g = graph(false);
            for (auto &a : g.accounts)
                if (a.account == "E")
                    std::erase_if(a.individual_entries, [](const auto &e) {
                        return e.operation == "reference.application-read.v1";
                    });
            db.replace_graph(0, g);
            expected = false;
            done = false;
            require(service.enqueue(command(), [&](auto) { done = true; }).error ==
                        StorageError::none,
                    "Second OLD probe after actual read revocation");
            require(QTest::qWaitFor([&] { return done; }, 10000), "Denied probe completes");
            probe.witness.rethrow();
            if (probe.witness.error)
                std::rethrow_exception(probe.witness.error);
        });
    }
    void disclosureHoldsNeverTreatsMissingSetAsKnownEmpty() {
        scenario([] {
            Rig r;
            ApplicationHoldActiveSet malformed;
            r.probe([&](const auto &v, const auto &c) {
                input_failure([&] { (void)r.adapters.disclose_holds(v, c, malformed); },
                              ReferenceInputError::invalid_shape);
            });
        });
    }
    void actualServiceCapLimitedCreditAndSourceNewKeyDedup() {
        scenario([] {
            Rig r;
            const auto first = r.attempt();
            accounting(applied(first), 600000, 120000, 480000, 3600000);
            for (const auto *t :
                 {"subathon_app_operations", "subathon_app_claims", "subathon_app_effects",
                  "subathon_app_results", "subathon_app_audit"})
                require(r.file.count(t) == 1,
                        "Exactly one durable accounting record per owning table");
            const auto snapshots = r.file.count("subathon_app_states");
            const auto clock_calls = r.ports.clocks, refs = r.ports.refs;
            const auto exact = r.attempt();
            require(exact.response && exact.response->disposition ==
                                          ApplicationServiceDisposition::existing_operation,
                    "Exact retry historical application");
            require(std::holds_alternative<RecordedApplicationBundle>(exact.response->payload),
                    "Historical retry has whole record");
            accounting(std::get<RecordedApplicationBundle>(exact.response->payload), 600000, 120000,
                       480000, 3600000);
            const auto new_key = r.attempt(command("new-key"));
            require(new_key.response &&
                        new_key.response->disposition ==
                            ApplicationServiceDisposition::subject_claimed &&
                        r.file.count("subathon_app_claims") == 1 &&
                        r.file.count("subathon_app_states") == snapshots &&
                        r.ports.clocks == clock_calls && r.ports.refs == refs,
                    "Same source under new operation key cannot award again or create new token");
        });
    }
    void actualServiceCurrentCapAndActualZeroStillNewToken_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<qlonglong>("actual");
        QTest::addColumn<qlonglong>("capped");
        QTest::addColumn<qlonglong>("main");
        QTest::newRow("RP09-current-cap3540000")
            << QString("lower_cap") << qlonglong(60000) << qlonglong(540000) << qlonglong(3540000);
        QTest::newRow("RP10-actual0-full-cap")
            << QString("full") << qlonglong(0) << qlonglong(600000) << qlonglong(3600000);
    }
    void actualServiceCurrentCapAndActualZeroStillNewToken() {
        QFETCH(QString, name);
        QFETCH(qlonglong, actual);
        QFETCH(qlonglong, capped);
        QFETCH(qlonglong, main);
        scenario([&] {
            Rig r(name.toUtf8().constData());
            const auto got = r.attempt();
            accounting(applied(got), 600000, actual, capped, main);
            require(r.file.count("subathon_app_claims") == 1, "Actual0 still commits source claim");
        });
    }
    void actualServiceElapsedBeforeCreditUses150000Not120000() {
        scenario([] {
            Rig r;
            r.sampler.next = ReferenceClockSample{30100, -5000};
            const auto got = r.attempt();
            accounting(applied(got), 600000, 150000, 450000, 3600000);
            require(applied(got).advanced_before.timer.main_remaining_ms == 3450000,
                    "Actual elapsed CORE-002 reduction BEFORE credit");
        });
    }
    void actualSqlLateAuditFailureRollsBackAndExplicitRetryAppliesOnce() {
        scenario([] {
            Rig r;
            (void)r.file.observer->rows(
                "CREATE TRIGGER api011_audit_fault BEFORE INSERT ON subathon_app_audit "
                "WHEN EXISTS(SELECT 1 FROM subathon_app_effects) "
                "AND EXISTS(SELECT 1 FROM subathon_app_states WHERE "
                "main_remaining_ms=3600000) "
                "AND EXISTS(SELECT 1 FROM subathon_app_head WHERE token=X'61') "
                "BEGIN SELECT RAISE(ABORT,'API011 after state/effect before head'); END");
            require(
                r.file.observer
                        ->rows(
                            "SELECT count(*) FROM sqlite_schema WHERE name='api011_audit_fault'")[0]
                                                                                                 [0]
                        .toInt() == 1,
                "SETUP: idle observer installs the genuine SQLite trigger");

            const auto got = r.attempt();
            require(got.transaction == TransactionOutcome::rolled_back &&
                        got.work_error == WorkError::sql_failure && got.failure &&
                        got.failure->source == ApplicationServiceFailureSource::bundle &&
                        std::get<SqliteBundleStorageReason>(got.failure->reason) ==
                            SqliteBundleStorageReason::sql_failure &&
                        !got.response,
                    "ORIGINAL actual SQL failure survives; real late staging rollback");
            for (const auto *t :
                 {"subathon_app_operations", "subathon_app_claims", "subathon_app_effects",
                  "subathon_app_results", "subathon_app_audit", "subathon_app_jobs"})
                require(r.file.count(t) == 0, "Whole application transaction rolled back");
            require(
                r.file.count("subathon_receipts") == 1 &&
                    r.file.count("subathon_reference_initial_captures") == 1 &&
                    r.file.observer->rows("SELECT main_remaining_ms FROM subathon_app_states")[0][0]
                            .toLongLong() == 3480000,
                "Earlier intake/capture durable and original head unchanged");
            const auto rolled_back_draws = r.entropy.calls;
            require(rolled_back_draws == 3,
                    "RCO1 reservation, advanced_state and after_state each drew once");
            const auto spent_after = std::string("RRF1\1", 5) + std::string(32, char(3));
            r.adapters.settle(got, {});
            r.file.db.work(
                [](const auto &s) { (void)s.execute({"DROP TRIGGER api011_audit_fault", {}}); });
            const auto retried = r.attempt();
            accounting(applied(retried), 600000, 120000, 480000, 3600000);
            require(r.entropy.calls == rolled_back_draws + 3 &&
                        applied(retried).advanced_before.token ==
                            std::string("RRF1\0", 5) + std::string(32, char(5)) &&
                        applied(retried).after.token ==
                            std::string("RRF1\1", 5) + std::string(32, char(6)) &&
                        applied(retried).after.token != spent_after,
                    "Explicit retry has new entropy and new after-token; spent value never reused");
        });
    }
    void actualCommitAckLossStopsWithoutNormalPublication() {
        scenario([] {
            Rig r;
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        r.file.db.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Existing real writer fault installs");
            const auto got = r.attempt();
            require(got.transaction == TransactionOutcome::outcome_unknown &&
                        got.work_error == WorkError::uncertain_outcome && !got.response &&
                        r.file.count("subathon_app_claims") == 1,
                    "Unknown acknowledgement, physical accounting exists, no response");
            r.adapters.settle(got, {});
            require(r.adapters.reserve(committed(), command()).has_value(),
                    "Unknown stops, no fresh-key blind replay");
        });
    }
    void actualAcknowledgedCommitTailThrowRequiresIndependentRead() {
        scenario([] {
            Rig r;
            bool tail = false;
            const auto got = r.attempt(command(), [&](QVariant) -> QVariant {
                require(r.file.count("subathon_app_claims") == 1,
                        "Actual acknowledged durable body precedes throw");
                tail = true;
                throw std::runtime_error("synthetic postcommit body tail");
            });
            require(tail && got.transaction == TransactionOutcome::committed &&
                        got.work_error == WorkError::work_exception && !got.response,
                    "Committed body error truth, no normal response/publication");
            r.adapters.settle(got, {});
            require(r.adapters.reserve(committed(), command()).has_value(),
                    "Admission blocks pending independent read");
            Completion<ReferenceSnapshotCompletion> out;
            out.run(r.file.db.writer, r.intake.make_snapshot_work(actor('O'), out.callback()));
            normal(out.result->work);
            require(out.result->snapshot &&
                        out.result->snapshot->committed.snapshot.timer.main_remaining_ms ==
                            3600000 &&
                        r.file.count("subathon_app_claims") == 1,
                    "Separately authorized real read resolves durable head once");
            auto recovered = out.result->snapshot->committed;
            require(recovered.snapshot.token ==
                            std::string("RRF1\1", 5) + std::string(32, char(3)) &&
                        recovered.basis == "a" && recovered.observation.checked_head_token == "a" &&
                        recovered.observation.observation_ref == "O" &&
                        recovered.observation.segments.empty(),
                    "Independent committed b binding");
            r.adapters.reconcile(recovered);
            const auto live = r.adapters.current(recovered.snapshot);
            require(std::holds_alternative<ReferenceCommittedState>(live),
                    "Authorized read promotes b and reopens journal");
            same_state(std::get<ReferenceCommittedState>(live), recovered);
            Completion<ReferenceReceiptCompletion> receipt;
            receipt.run(r.file.db.writer, r.intake.make_receipt_work(
                                              "D", {"Q", "after-reconcile", {}, QByteArray("", 0)},
                                              receipt.callback()));
            normal(receipt.result->work);
            require(receipt.result->receipt.has_value() && r.file.count("subathon_receipts") == 2,
                    "Borrowed live runtime admits receipt on advanced b");
            ApplicationServiceCompletion unknown;
            unknown.transaction = TransactionOutcome::outcome_unknown;
            unknown.work_error = WorkError::uncertain_outcome;
            r.adapters.settle(unknown, {});
            Completion<ReferenceReceiptCompletion> blocked;
            blocked.run(r.file.db.writer, r.intake.make_receipt_work(
                                              "D", {"Q", "after-unknown", {}, QByteArray("", 0)},
                                              blocked.callback()));
            require(blocked.result->refusal && !blocked.result->receipt &&
                        blocked.result->refusal->gate == ReferenceGate::not_ready &&
                        blocked.result->refusal->reason_ref == "reference.runtime-unavailable.v1" &&
                        r.file.count("subathon_receipts") == 2,
                    "Same borrower observes stop after unknown; stale startup copy cannot pass");
        });
    }
    void issuerWholeImportedResultByteCeilingExact77AndOneLess() {
        scenario([] {
            ReferenceRuntimeLimits l;
            l.values.value_owned_bytes = 77;
            Rig exact("standard", l);
            require(std::holds_alternative<IssuedApplicationReference>(exact.adapters.issue(
                        ApplicationReferencePurpose::after_state, reference_binding())),
                    "Owning service Binding40 + reference37 = result77 fits (absent optional0)");
            --l.values.value_owned_bytes;
            Rig short_profile("standard", l);
            input_failure(
                [&] {
                    (void)short_profile.adapters.issue(ApplicationReferencePurpose::after_state,
                                                       reference_binding());
                },
                ReferenceInputError::result_limit);
            require(short_profile.entropy.calls == 0,
                    "Known output length projected before entropy/allocation");
        });
    }
    void actualZeroConfiguredRequestIsAppliedOnceWithFreshToken() {
        scenario([] {
            Rig r("standard", {}, 0);
            const auto got = r.attempt();
            accounting(applied(got), 0, 0, 0, 3480000);
            require(r.file.count("subathon_app_claims") == 1 &&
                        r.file.count("subathon_app_results") == 1,
                    "RC02 exact0/1 is a real application, not cancellation");
        });
    }
    void actualMaxRequestOverflowsAsWholeNum04Hold() {
        scenario([] {
            Rig r("overflow", {}, std::numeric_limits<std::int64_t>::max());
            const auto got = r.attempt();
            require(got.transaction == TransactionOutcome::committed &&
                        got.work_error == WorkError::none && got.response &&
                        got.response->disposition == ApplicationServiceDisposition::held_numeric,
                    "MAX valid exact request; Main1+MAX gives whole numeric hold");
            const auto causes = r.file.observer->rows(
                "SELECT cause_kind,failure_stage,failing_ordinal,failing_target_ref,"
                "failure_numerator,failure_denominator,composition,missing_policy_reason "
                "FROM subathon_hold_causes");
            require(
                causes.size() == 1 && causes[0][0].toInt() == 0 && causes[0][1].toInt() == 1 &&
                    causes[0][2].toLongLong() == 1 && causes[0][3].toByteArray() == "T" &&
                    causes[0][4].toByteArray() == "9223372036854775808" &&
                    causes[0][5].toByteArray() == "1" && causes[0][6].isNull() &&
                    causes[0][7].isNull(),
                "Exact durable whole numeric_overflow/aggregate ordinal1 targetT attempted2^63/1");
            const auto observations =
                r.file.observer->rows("SELECT "
                                      "persisted_state_token,format_ref,document,policy_revision_"
                                      "ref,observed_utc_ms,elapsed_ms "
                                      "FROM subathon_hold_cause_observations");
            require(observations.size() == 1 && observations[0][0].toByteArray() == "a" &&
                        observations[0][1].toByteArray() ==
                            "subathon.internal.application-hold-observation.v1" &&
                        observations[0][3].toByteArray() == "W" &&
                        observations[0][4].toLongLong() == 1000 &&
                        observations[0][5].toLongLong() == 0,
                    "Durable retained-head/Policy/Clock evidence");
            // Independent owning SAH1 grammar reader. Never derive an oracle from
            // held result or candidate encoder; require every literal leaf and end.
            const auto doc = observations[0][2].toByteArray();
            qsizetype offset = 4;
            auto number = [&](int width) {
                require(offset <= doc.size() && width <= doc.size() - offset,
                        "SAH1 complete frame");
                std::uint64_t n = 0;
                for (int k = 0; k < width; ++k)
                    n = (n << 8) | static_cast<unsigned char>(doc[offset++]);
                return n;
            };
            auto bytes = [&] {
                const auto n = number(4);
                require(n <= std::uint64_t(doc.size() - offset), "SAH1 bounded string");
                const auto v = doc.mid(offset, qsizetype(n));
                offset += qsizetype(n);
                return v;
            };
            require(doc.startsWith("SAH1") && bytes().startsWith("SAC1") && bytes() == "a" &&
                        number(1) == 1,
                    "SAH1 command, actual head and PRESENT Policy, no SAH2 correction plans");
            require(bytes() == "C" && bytes() == "W" && !bytes().isEmpty() && number(1) == 0 &&
                        number(1) == 1 && number(8) == 120000 && number(1) == 0 && number(1) == 0 &&
                        number(1) == 0 && number(1) == 0 &&
                        bytes() == "reference.fresh-runtime.v1" &&
                        bytes() == "reference.current-policy.v1",
                    "Whole exact no-cap fresh Policy");
            require(number(1) == 1 && bytes() == "L" && bytes() == "O" && number(8) == 1000 &&
                        number(4) == 0,
                    "PRESENT reused Clock, zero segments");
            require(number(1) == 1 && number(1) == 1 && number(1) == 1 && number(8) == 1 &&
                        number(1) == 1 && bytes() == "T" && number(1) == 1 &&
                        bytes() == "9223372036854775808" && bytes() == "1" && number(1) == 0 &&
                        bytes().isEmpty() && offset == doc.size(),
                    "Exact Failure and no suffix/partial correction");
            for (const auto *table : {"subathon_app_claims", "subathon_app_operations",
                                      "subathon_app_effects", "subathon_app_jobs"})
                require(r.file.count(table) == 0, "No partial MAX-1 award/claim/jobs");
            require(
                r.file.count("subathon_app_states") == 1 &&
                    r.file.observer->rows("SELECT main_remaining_ms FROM subathon_app_states")[0][0]
                            .toLongLong() == 1 &&
                    r.file.observer
                            ->rows("SELECT exact_numerator,requested_units FROM "
                                   "subathon_plan_requests")[0][0]
                            .toByteArray() == "9223372036854775807",
                "Original head and exact captured request preserved; no float/narrowing");
        });
    }
    void completeIntakeFourAcknowledgedUnitsReachActualApplicationService() {
        scenario([] {
            Rig r;
            Completion<ReferenceReceiptCompletion> receipt;
            receipt.run(r.file.db.writer, r.intake.make_receipt_work(
                                              "D", {"Q", "pipeline-event", {}, QByteArray("", 0)},
                                              receipt.callback()));
            normal(receipt.result->work);
            require(receipt.result->receipt && receipt.result->original,
                    "First acknowledged receipt/retained arrival unit");
            const auto rid = receipt.result->receipt->receipt_id;
            Completion<ReferenceEvidenceCompletion> evidence;
            evidence.run(r.file.db.writer, r.intake.make_evidence_work(rid, evidence.callback()));
            normal(evidence.result->work);
            require(evidence.result->evidence && evidence.result->evidence->contribution_id,
                    "Second acknowledged original evidence unit");
            Completion<ReferenceCaptureCompletion> capture;
            capture.run(r.file.db.writer,
                        r.intake.make_capture_work(rid, *evidence.result->evidence->contribution_id,
                                                   capture.callback()));
            normal(capture.result->work);
            require(capture.result->capture.has_value(), "Third acknowledged plan/SRX/index unit");
            auto c = command("pipeline-operation");
            c.receipt_id = rid;
            c.contribution_id = evidence.result->evidence->contribution_id;
            c.plan = capture.result->capture->plan;
            const auto &ref = capture.result->capture->execution_context_ref;
            // Independent SRF1 framing, not a candidate encoder or numeric oracle.
            QByteArray choice("SRF1");
            for (int shift = 24; shift >= 0; shift -= 8)
                choice += char((ref.size() >> shift) & 255);
            choice += blob(ref);
            c.choices[0].document = choice;
            accounting(applied(r.attempt(c)), 600000, 120000, 480000, 3600000);
            require(r.file.count("subathon_app_claims") == 1 &&
                        r.file.count("subathon_reference_initial_captures") == 2 &&
                        r.file.count("subathon_receipts") == 2,
                    "Four separate true units, earlier fixture plus one new source");
        });
    }
    void accessNestedWholeAuthorityResultExact2440ThenOneLess() {
        scenario([] {
            ReferenceRuntimeLimits l;
            l.authority.result_bytes = 2440;
            Rig exact("standard", l);
            exact.probe([&](const auto &v, const auto &c) {
                require(
                    std::holds_alternative<ApplicationAccessAllowed>(exact.adapters.check(v, c)),
                    "Independent authority Facts2440 fits");
            });
            --l.authority.result_bytes;
            Rig limited("standard", l);
            limited.probe([&](const auto &v, const auto &c) {
                require(
                    std::holds_alternative<ApplicationAccessUnknown>(limited.adapters.check(v, c)),
                    "Nested whole2440 does not fit2439; never partial grant");
            });
        });
    }
    void historicalR1AndDistinctR2CaptureKeepIndependentAmountsAndCapabilities() {
        scenario([] {
            File file;
            auto original = facts();
            auto current = facts("D", 3600000);
            current.projection.rule.rule_version_ref = "s";
            current.projection.rule.automation_capability_ref = "J";
            current.projection.rule.add_ms = 900000;
            current.enabled_rules[0].rule = current.projection.rule;
            // Independent complete current D fixture BEFORE new API calls; no
            // editorial endpoint and no overwrite of captured immutable C/r1.
            file.db.work([&](const auto &session) {
                auto d = QByteArray::fromHex(literals()["configurations"]
                                                 .toObject()["standard"]
                                                 .toObject()["document_hex"]
                                                 .toString()
                                                 .toLatin1());
                d.replace(QByteArray::fromHex("0000000172"), QByteArray::fromHex("0000000173"));
                d.replace(QByteArray::fromHex("000000014b"), QByteArray::fromHex("000000014a"));
                d.replace(QByteArray::fromHex("00000000000927c0"),
                          QByteArray::fromHex("00000000000dbba0"));
                (void)session.execute(
                    {"UPDATE subathon_retained_configurations SET document=? WHERE reference=X'44'",
                     {d}});
            });
            for (int channel = 0; channel != 2; ++channel) {
                auto g = graph(channel == 1);
                for (auto &a : g.accounts)
                    for (auto &e : a.individual_entries)
                        if (!e.quantities.empty())
                            e.quantities[0].upper = PermissionRational{"900000", "1"};
                file.db.replace_graph(channel, g);
            }
            file.db.work([&](const auto &session) {
                auto b = binding();
                b.capability = "J";
                b.rule_version = "s";
                b.requested_ms = 900000;
                SqliteReferenceAuthorityUnit registry(session);
                staged(registry.enroll(actor(), file.db.clock, 3, b, "E"), 4);
                staged(registry.set_state(actor('O'), file.db.clock, 4, "J",
                                          ReferenceAuthorityState::authorized),
                       5);
            });
            ReferenceConfigurationArchive config({{"C", original}, {"D", current}});
            Sampler sampler;
            Entropy entropy;
            struct CapturedAuthority final : ReferenceAuthoritySource {
                SqliteReferenceAuthoritySource actual;
                std::vector<std::string> capabilities;
                ReferenceAuthorityResolution resolve(const ApplicationReferenceReadView &v,
                                                     const ReferenceAuthorityRequest &q) override {
                    capabilities.push_back(q.captured.rule.automation_capability_ref);
                    require((q.captured.rule.automation_capability_ref == "K" &&
                             q.captured.rule.rule_version_ref == "r" &&
                             q.captured.rule.add_ms == 600000) ||
                                (q.captured.rule.automation_capability_ref == "J" &&
                                 q.captured.rule.rule_version_ref == "s" &&
                                 q.captured.rule.add_ms == 900000),
                            "Fresh actual authority sees immutable K/r/600000 OR independent "
                            "J/s/900000");
                    return actual.resolve(v, q);
                }
            } authority;
            ReferenceApplicationAdapters runtime(config, authority, sampler, entropy,
                                                 ReferenceClockAnchor{committed(), {100, 1000}});
            ReferenceIntake intake(config, sampler, entropy, file.db.clock, connector(), &runtime);
            Completion<ReferenceCaptureCompletion> old;
            old.run(file.db.writer, intake.make_capture_work({7}, {0}, old.callback()));
            normal(old.result->work);
            require(old.result->capture && old.result->capture->configuration_ref == "C" &&
                        old.result->capture->plan.plan_ref == "P",
                    "RP11 original e1/r1 capture unchanged despite distinct r2/current config");
            Completion<ReferenceReceiptCompletion> receipt;
            receipt.run(file.db.writer,
                        intake.make_receipt_work("D", {"Q", "r2-event", {}, QByteArray("", 0)},
                                                 receipt.callback()));
            normal(receipt.result->work);
            require(receipt.result->receipt.has_value(), "New r2 original receipt");
            Completion<ReferenceEvidenceCompletion> evidence;
            evidence.run(file.db.writer,
                         intake.make_evidence_work(receipt.result->receipt->receipt_id,
                                                   evidence.callback()));
            normal(evidence.result->work);
            require(evidence.result->evidence && evidence.result->evidence->contribution_id,
                    "New r2 contribution");
            Completion<ReferenceCaptureCompletion> captured;
            captured.run(file.db.writer,
                         intake.make_capture_work(receipt.result->receipt->receipt_id,
                                                  *evidence.result->evidence->contribution_id,
                                                  captured.callback()));
            normal(captured.result->work);
            require(captured.result->capture && captured.result->capture->configuration_ref == "D",
                    "New event captures distinct version");
            const auto requests =
                file.observer->rows("SELECT requested_units,exact_numerator,exact_denominator FROM "
                                    "subathon_plan_requests ORDER BY requested_units");
            require(requests.size() == 2 && requests[0][0].toLongLong() == 600000 &&
                        requests[0][1].toByteArray() == "600000" &&
                        requests[1][0].toLongLong() == 900000 &&
                        requests[1][1].toByteArray() == "900000" &&
                        requests[0][2].toByteArray() == "1" && requests[1][2].toByteArray() == "1",
                    "Independent exact600000/900000 literals, separate actual capabilities, no "
                    "current-rule replacement");
            Ports ports(runtime);
            ApplicationServicePorts service_ports{ports, ports, ports, ports, ports};
            OriginalApplicationService service(file.db.writer, service_ports);
            std::optional<ApplicationServiceCompletion> applied_original;
            auto prepared = service.make_work(
                command(), [&](auto result) { applied_original = std::move(result); });
            Witness queue_witness;
            const auto raw = qt_test_support::run_work(
                file.db.writer,
                [&](auto context) -> QVariant {
                    queue_witness.call([&] {
                        require(!runtime.reserve(committed(), command()),
                                "Live original-r1 reservation against current r2");
                    });
                    return prepared.body(context);
                },
                10000);
            prepared.completion(raw);
            queue_witness.rethrow();
            ports.witness.rethrow();
            require(applied_original.has_value(),
                    "Actual r1 service completion under coherent current r2");
            accounting(applied(*applied_original), 600000, 120000, 480000, 3600000);
            require(std::find(authority.capabilities.begin(), authority.capabilities.end(), "K") !=
                        authority.capabilities.end(),
                    "RP11 actual current K authority validated for original capture");
            auto second = command("second-capture-check");
            second.receipt_id = receipt.result->receipt->receipt_id;
            second.contribution_id = evidence.result->evidence->contribution_id;
            second.plan = captured.result->capture->plan;
            const auto ref = captured.result->capture->execution_context_ref;
            QByteArray choice("SRF1");
            for (int shift = 24; shift >= 0; shift -= 8)
                choice += char((ref.size() >> shift) & 255);
            choice += blob(ref);
            second.choices[0].document = choice;
            auto query = [&](const OriginalApplicationCommand &selected, bool allow) {
                Probe probe;
                probe.f = [&](const auto &view, const auto &) {
                    const auto access = runtime.check(view, selected);
                    require(allow ? std::holds_alternative<ApplicationAccessAllowed>(access)
                                  : std::holds_alternative<ApplicationAccessDenied>(access),
                            "Captured capability must STILL have current actual authority");
                };
                ApplicationServicePorts selected_ports{probe, ports, ports, ports, ports};
                OriginalApplicationService selected_service(file.db.writer, selected_ports);
                bool done = false;
                require(
                    selected_service.enqueue(command("probe"), [&](auto) { done = true; }).error ==
                        StorageError::none,
                    "OLD finite-view callback probe");
                require(QTest::qWaitFor([&] { return done; }, 10000), "Actual probe completes");
                probe.witness.rethrow();
            };
            query(second, true);
            require(std::find(authority.capabilities.begin(), authority.capabilities.end(), "J") !=
                        authority.capabilities.end(),
                    "Independent r2 input resolves its separately enrolled J");
            file.db.work([&](const auto &session) {
                staged(SqliteReferenceAuthorityUnit(session).set_state(
                           actor('O'), file.db.clock, 5, "K", ReferenceAuthorityState::revoked),
                       6);
            });
            query(command(), false);
            query(second, true); // Current J can never rescue revoked captured K.
        });
    }
    void wholeHoldDisclosureChecksEveryActualMemberAndCurrentReadRights() {
        scenario([] {
            Database db;
            seed_history(db, true);
            ApplicationHoldActiveSet set;
            db.work([&](const auto &session) {
                SqliteApplicationHoldStore store(session);
                const auto first = store.find("hold");
                require(first.has_value(), "SETUP: OLD complete stored hold");
                ApplicationHoldDetection second;
                second.cause = first->cause;
                second.cause.hold_ref = "hold2";
                second.cause.missing_policy_reason = "independent.second-policy";
                second.request = {"detect2",
                                  {},
                                  "version2",
                                  second.cause.first_observation,
                                  {"opaque.audit", QByteArray("", 0)}};
                require(store.record_detection(second).disposition ==
                            SqliteHoldMutationDisposition::recorded,
                        "SETUP: independent second real hold");
                set = store.active({ContributionId{0}});
                require(set.holds.size() == 2, "SETUP: actual complete two-member active set");
            });
            struct ObservedAuthority final : ReferenceAuthoritySource {
                SqliteReferenceAuthoritySource actual;
                std::vector<std::string> holds;
                bool deny_second = false;
                ReferenceAuthorityResolution resolve(const ApplicationReferenceReadView &v,
                                                     const ReferenceAuthorityRequest &r) override {
                    require(r.use == ReferenceAuthorityUse::disclose_holds &&
                                r.historical_hold.has_value(),
                            "Separate actual quantity-free per-hold disclosure request");
                    holds.push_back(*r.historical_hold);
                    auto result = actual.resolve(v, r);
                    if (deny_second && *r.historical_hold == "hold2") {
                        require(std::holds_alternative<ReferenceAuthorityFacts>(result.value),
                                "Actual second record resolves before typed denial wrapper");
                        result.value = ReferenceAuthorityFailure::denied;
                    }
                    return result;
                }
            } authority;
            ReferenceConfigurationArchive config(versions());
            Sampler sampler;
            Entropy entropy;
            ReferenceApplicationAdapters actual(config, authority, sampler, entropy, {});
            auto query = [&](bool expected) {
                Probe probe;
                Ports ports(actual);
                probe.f = [&](const auto &view, const auto &c) {
                    require(actual.disclose_holds(view, c, set) == expected,
                            "Whole actual hold set visibility, never partial detail success");
                };
                ApplicationServicePorts p{probe, ports, ports, ports, ports};
                OriginalApplicationService service(db.writer, p);
                bool done = false;
                require(service.enqueue(command(), [&](auto) { done = true; }).error ==
                            StorageError::none,
                        "OLD finite-view probe admits");
                require(QTest::qWaitFor([&] { return done; }, 10000), "Actual callback completion");
                probe.witness.rethrow_ni();
                if (probe.witness.error)
                    std::rethrow_exception(probe.witness.error);
            };
            query(true);
            std::sort(authority.holds.begin(), authority.holds.end());
            require(authority.holds == std::vector<std::string>{"hold", "hold2"},
                    "Each actual hold resolved independently, no execute permit or "
                    "representative-only check");
            authority.deny_second = true;
            authority.holds.clear();
            query(false);
            require(std::find(authority.holds.begin(), authority.holds.end(), "hold2") !=
                        authority.holds.end(),
                    "Mixed real allowed/denied pair rejects WHOLE set; no any-of permit");
            authority.deny_second = false;
            auto grant = graph(true);
            for (auto &a : grant.accounts)
                if (a.account == "A")
                    std::erase_if(a.individual_entries, [](const auto &e) {
                        return e.operation == "reference.hold-read.v1";
                    });
            db.replace_graph(1, grant);
            query(false);
        });
    }
    void actualStateTokenCollisionUsesDurableConflictWithoutRegeneration() {
        scenario([] {
            Rig r;
            r.entropy.advance = false;
            r.entropy.next.bytes.fill(0x7f);
            const auto occupied = QByteArray("RRF1\1", 5) + QByteArray(32, char(0x7f));
            r.file.db.work([&](const auto &session) {
                (void)session.execute({"INSERT INTO subathon_app_states SELECT "
                                       "?,format_ref,document,configuration_ref,lifecycle,phase,"
                                       "main_remaining_ms,grace_remaining_ms,awaiting_confirmation,"
                                       "time_debt_ms,balance_count "
                                       "FROM subathon_app_states WHERE token=X'61'",
                                       {occupied}});
            });
            const auto got = r.attempt();
            require(got.response &&
                        got.response->disposition ==
                            ApplicationServiceDisposition::state_token_conflict &&
                        r.entropy.calls == 3 && r.file.count("subathon_app_states") == 2 &&
                        r.file.count("subathon_app_claims") == 0,
                    "Actual occupied SQL state namespace refuses, no regeneration/second award");
        });
    }
    void queuedDuplicateWorkUsesFifoCurrentHeadAndOneSourceClaim() {
        scenario([] {
            Rig r;
            std::vector<ApplicationServiceCompletion> results;
            std::vector<std::string> completion_order;
            auto enqueue = [&](std::string key) {
                const auto c = command(key);
                auto reservation_draw = std::make_shared<unsigned>(0);
                auto before = std::make_shared<ReferenceCommittedState>();
                auto prepared =
                    r.service.make_work(c, [&, key, before, reservation_draw](auto completion) {
                        try {
                            if (completion.response)
                                if (const auto *record = std::get_if<RecordedApplicationBundle>(
                                        &completion.response->payload)) {
                                    if (completion.response->disposition ==
                                        ApplicationServiceDisposition::applied) {
                                        r.current = checked_after(*record, *before, {100, 1000},
                                                                  *reservation_draw);
                                        r.adapters.settle(completion, r.current);
                                    } else
                                        r.adapters.settle(completion, {});
                                }
                        } catch (...) {
                            r.ports.witness.error = std::current_exception();
                        }
                        completion_order.push_back(key);
                        results.push_back(std::move(completion));
                    });
                auto body = std::move(prepared.body);
                auto wrapper = [&, c, body, before, reservation_draw](auto context) -> QVariant {
                    r.ports.witness.call([&] {
                        *before = r.current;
                        require(!r.adapters.reserve(r.current, c),
                                "FIFO sample immediately before actual execution");
                        *reservation_draw = r.entropy.calls;
                    });
                    return body(context);
                };
                require(r.file.db.writer.enqueue_work(std::move(wrapper),
                                                      std::move(prepared.completion)) ==
                            StorageError::none,
                        "Two actual queued units admitted without pumping between them");
            };
            enqueue("first");
            enqueue("second");
            require(QTest::qWaitFor([&] { return results.size() == 2; }, 10000),
                    "Both actual FIFO completions");
            r.ports.witness.rethrow();
            require(completion_order == std::vector<std::string>{"first", "second"},
                    "Writer FIFO completion order");
            accounting(applied(results[0]), 600000, 120000, 480000, 3600000);
            require(results[1].transaction == TransactionOutcome::committed &&
                        results[1].work_error == WorkError::none && results[1].response &&
                        results[1].response->disposition ==
                            ApplicationServiceDisposition::subject_claimed &&
                        r.file.count("subathon_app_claims") == 1 &&
                        r.file.count("subathon_app_effects") == 1,
                    "Queued duplicate/current head cannot create second award");
        });
    }
    void currentRuntimeReadsLiveOwnedAnchorAndDoesNotResample() {
        scenario([] {
            Rig r;
            const auto got = r.adapters.current(committed().snapshot);
            const auto *state = std::get_if<ReferenceCommittedState>(&got);
            require(state && state->snapshot.token == "a" &&
                        state->observation.observed_utc_ms == 1000 && r.sampler.calls == 0 &&
                        r.entropy.calls == 0,
                    "Current runtime uses actual live acknowledged anchor, not request clock or "
                    "false defaults");
        });
    }
};
QTEST_GUILESS_MAIN(ReferenceApplicationAdapterTests)
#include "reference_application_adapter_tests.moc"
