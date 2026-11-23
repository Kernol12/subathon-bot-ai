// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace removal_test;

template <class Test> void check(Test test) {
    try {
        test();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

void normal_examples() {
    QTest::addColumn<int>("scenario");
    QTest::newRow("pin-fetched-first") << int(Start::pin_f_first);
    QTest::newRow("pin-triangulated-first") << int(Start::pin_w_first);
    QTest::newRow("temporary-disconnects") << int(Start::temporary);
    QTest::newRow("same-rate-new-provenance") << int(Start::equal_rates);
}

void expect_core_error(const core::EffectiveRateSnapshot &snapshot,
                       const core::RateOverridePair &pair, std::string_view target,
                       std::span<const core::RateOverrideFetchedEdge> edges,
                       std::string_view candidate, core::RateSnapshotError code) {
    try {
        (void)core::preview_rate_override_removal(snapshot, pair, target, edges, candidate);
    } catch (const core::RateSnapshotFailure &failure) {
        require(failure.code() == code, "Pure preview returns the exact snapshot failure code");
        return;
    }
    throw std::runtime_error("Pure preview unexpectedly returned a removal proposal");
}

RateAuthorityFacts role_authority(bool individual_deny_matches, bool role_deny) {
    auto facts = permitted();
    auto allow = grant();
    allow.origin = core::PermissionOrigin::role;
    allow.source = "RoleAllow";
    allow.role = "RAllow";
    auto deny = grant({individual_deny_matches ? pair_eur_usd : pair_eur_gbp});
    deny.source = "IndividualDeny";
    deny.effect = core::PermissionEffect::deny;
    deny.grant.reset();
    auto &use = facts.graph.use;
    use.roles = {{"RAllow", {allow}}};
    use.accounts.front().assigned_roles = {"RAllow"};
    use.accounts.front().individual_entries = {deny};
    if (role_deny) {
        auto blocked = allow;
        blocked.role = "RDeny";
        blocked.source = "RoleDeny";
        blocked.effect = core::PermissionEffect::deny;
        blocked.resources = {{"pair", core::PermissionResourceMode::any, {}}};
        blocked.grant.reset();
        use.roles.push_back({"RDeny", {blocked}});
        use.accounts.front().assigned_roles.push_back("RDeny");
    }
    return facts;
}

RateAuthorityFacts another_actor() {
    auto facts = permitted();
    facts.session.account_ref = "B";
    facts.session.session_ref = "N3";
    facts.session.record_revision_ref = "R3";
    facts.session.snapshot_ref = "P3";
    auto use = grant();
    use.source = "OtherUse";
    use.grant = "OtherGrant";
    facts.graph.use.accounts.push_back({"B", {}, {std::move(use)}});
    facts.graph.grant.accounts.push_back({"B", {}, {}});
    facts.graph.current_accounts.push_back({{"I", "B", "b", true}, "bR"});
    return facts;
}

enum class ReadDenial { revoked, missing_use, owner_only, grant_only, individual_deny, actor };

void read_denials() {
    QTest::addColumn<int>("reason");
    QTest::newRow("revoked-session") << int(ReadDenial::revoked);
    QTest::newRow("missing-use") << int(ReadDenial::missing_use);
    QTest::newRow("owner-only") << int(ReadDenial::owner_only);
    QTest::newRow("grant-only") << int(ReadDenial::grant_only);
    QTest::newRow("matching-individual-deny") << int(ReadDenial::individual_deny);
    QTest::newRow("actor-mismatch") << int(ReadDenial::actor);
}

RateAuthorityResolution denied_read_authority(ReadDenial reason) {
    switch (reason) {
    case ReadDenial::revoked:
        return SessionFailure::revoked;
    case ReadDenial::missing_use:
    case ReadDenial::grant_only: {
        // Keep A as the authenticated actor and put ownership on enabled account B.
        auto facts = another_actor();
        facts.session = permitted().session;
        facts.graph.owner.account = "B";
        facts.graph.use.accounts.front().individual_entries.clear();
        if (reason == ReadDenial::grant_only)
            facts.graph.grant.accounts.front().individual_entries = {grant()};
        return facts;
    }
    case ReadDenial::owner_only: {
        auto facts = permitted();
        facts.graph.use.accounts.front().individual_entries.clear();
        return facts;
    }
    case ReadDenial::individual_deny:
        return role_authority(true, false);
    case ReadDenial::actor:
        return another_actor();
    }
    throw std::logic_error("Unknown removal read denial fixture");
}

class RateOverrideRemovalTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void purePreview_data() { normal_examples(); }
    void purePreview() {
        QFETCH(int, scenario);
        check([=] {
            const auto start = Start(scenario);
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto base = std::get<core::EffectiveRateSnapshot>(head(file));
            const auto expected = contract.expected_preview();

            // The proposal must own the selected facts after all borrowed
            // observations, the pair and input ref leave scope.
            auto actual = [&] {
                std::vector<core::FetchedRateObservation> observations;
                for (const auto &input : contract.basis.observations)
                    observations.push_back(core::make_fetched_rate_observation(input));
                std::vector<core::RateOverrideFetchedEdge> edges;
                for (const auto &entry : contract.basis.edges)
                    edges.push_back(
                        {std::cref(observations.at(entry.observation_index)), entry.quote_ordinal});
                auto pair = expected.edit.pair;
                auto target = expected.edit.removed_override_ref;
                auto successor = expected.edit.successor;
                return core::preview_rate_override_removal(base, pair, target, edges, successor);
            }();
            expect_proposal(actual, expected.proposal);
            expect_quotes(contract, actual.candidate, start);
            expect_snapshot(base, contract.before);
            require(rate_image(file) == before, "Pure preview leaves the shipped start intact");
        });
    }

    void pureRefusals() {
        check([] {
            constexpr auto start = Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto base = std::get<core::EffectiveRateSnapshot>(head(file));
            using Error = core::RateSnapshotError;
            expect_core_error(base, {"USD", "EUR"}, "O", {}, "U", Error::invalid_currency);
            expect_core_error(base, {"EUR", "EUR"}, "O", {}, "U", Error::invalid_currency);
            expect_core_error(base, {"", "USD"}, "O", {}, "U", Error::invalid_currency);
            const std::string high_byte(1, static_cast<char>(0x80));
            expect_core_error(base, {high_byte, "USD"}, "O", {}, "U", Error::invalid_currency);
            expect_core_error(base, {"USD", high_byte}, "O", {}, "U", Error::invalid_override);
            expect_core_error(base, {"EUR", "USD"}, "bogus", {}, "U", Error::invalid_override);
            expect_core_error(base, {"EUR", "GBP"}, "O", {}, "U", Error::invalid_override);
            expect_core_error(base, {"EUR", "USD"}, "", {}, "U", Error::invalid_reference);
            expect_core_error(base, {"EUR", "USD"}, "O", {}, "", Error::invalid_reference);
            expect_core_error(base, {"EUR", "USD"}, "O", {}, "S", Error::invalid_reference);
            const auto observation = core::make_fetched_rate_observation(selected_f());
            const core::RateOverrideFetchedEdge invalid[]{{std::cref(observation), 3}};
            expect_core_error(base, {"EUR", "USD"}, "O", invalid, "U", Error::unresolved_basis);
            require(rate_image(file) == before, "Refused pure inputs preserve the committed image");
        });
    }

    void standalonePreview_data() { normal_examples(); }
    void standalonePreview() {
        QFETCH(int, scenario);
        check([=] {
            const auto start = Start(scenario);
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto result = preview_exact(file, contract.expected_preview());
            expect_quotes(contract, result.proposal.candidate, start);
            require(rate_image(file) == before, "Standalone preview produces no rate writes");
        });
    }

    void standaloneSave_data() { normal_examples(); }
    void standaloneSave() {
        QFETCH(int, scenario);
        check([=] {
            const auto start = Start(scenario);
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            // Submit the literal contract preview so this independently reaches
            // save even if the standalone preview entry point is missing.
            const auto expected = contract.expected_preview();
            const bool known_time = start != Start::temporary;
            const auto result = submit(file, expected, permitted(),
                                       known_time ? std::optional<std::int64_t>(0) : std::nullopt);
            expect_saved(*result, RateReceiptOrigin::newly_staged, expected);
            expect_transition(file, before, contract, start, known_time);
            const auto committed = rate_image(file);
            file.reopen();
            expect_snapshot(head(file), contract.after);
            expect_snapshot(revision(file, "U"), contract.after);
            expect_snapshot(revision(file, contract.before.snapshot_ref), contract.before);
            expect_recovered(file, expected);
            expect_saved(*submit(file, expected), RateReceiptOrigin::retained, expected);
            require(rate_image(file) == committed,
                    "Reopen, recovery and replay never append or publish");
        });
    }

    void authorityAtSave() {
        check([] {
            constexpr auto start = Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto expected = preview_exact(file, contract.expected_preview());
            const auto refuse = [&](RateAuthorityResolution facts) {
                expect_save_refusal(file, expected.edit, expected, expected.basis,
                                    RateStoreRefusal::denied, std::move(facts));
            };
            refuse(role_authority(true, false));
            refuse(role_authority(false, true));
            auto owner = permitted();
            owner.graph.use.accounts.front().individual_entries.clear();
            refuse(owner);
            owner.graph.grant.accounts.front().individual_entries = {grant()};
            refuse(owner);
            refuse(SessionFailure::revoked);
            auto wrong_submission = expected.edit;
            wrong_submission.actor = "B";
            expect_save_refusal(file, wrong_submission, expected, expected.basis,
                                RateStoreRefusal::denied);
            auto wrong_preview = expected;
            wrong_preview.edit.actor = "B";
            expect_save_refusal(file, expected.edit, wrong_preview, expected.basis,
                                RateStoreRefusal::preview_changed);
            (void)preview_exact(file, expected, role_authority(false, false));
            require(rate_image(file) == before,
                    "Authority checks and role-permitted preview are read-only");
            expect_saved(*submit(file, expected, role_authority(false, false)),
                         RateReceiptOrigin::newly_staged, expected);
            expect_transition(file, before, contract, start, true, false, true);
            expect_recovered(file, expected);
        });
    }

    void authorityAtPreview_data() { read_denials(); }
    void authorityAtPreview() {
        QFETCH(int, reason);
        check([=] {
            constexpr auto start = Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto expected = contract.expected_preview();
            expect_refusal(preview(file, expected.edit, expected.basis,
                                   denied_read_authority(ReadDenial(reason))),
                           RateStoreRefusal::denied);
            require(rate_image(file) == before, "Denied preview leaves the committed image intact");
        });
    }

    void freshTargets_data() {
        QTest::addColumn<int>("test");
        QTest::newRow("empty-E") << 0;
        QTest::newRow("wrong-pair-Q") << 1;
        QTest::newRow("unknown-reference") << 2;
        QTest::newRow("same-value-successor-occurrence") << 3;
        QTest::newRow("fresh-key-after-removal") << 4;
    }
    void freshTargets() {
        QFETCH(int, test);
        check([=] {
            const auto start = test == 1 || test == 2 ? Start::pin_f_first : Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            auto expected = contract.expected_preview();
            if (test != 0)
                (void)establish(file, contract, start);
            if (test == 0) {
                expect_snapshot(head(file), {"E", {}, {}, {}, {}, {}, {}});
                expected.edit.base = "E";
            } else if (test == 1) {
                expected.edit.removed_override_ref = "Q";
            } else if (test == 2) {
                expected.edit.removed_override_ref = "unknown";
            } else if (test == 3) {
                auto replacement = edit("L", "S", "T", "O2", "J");
                replacement.replacement = "O";
                (void)introduce(file, replacement, contract.basis);
                expected.edit.base = "T";
            } else {
                expect_saved(*submit(file, expected), RateReceiptOrigin::newly_staged, expected);
                expected.edit.operation = "fresh";
                expected.edit.decision = "Second";
                expected.edit.base = "U";
                expected.edit.successor = "V";
            }
            // Align the submitted identity to isolate the absent/wrong target.
            expected.proposal.removed_override_ref = expected.edit.removed_override_ref;
            expected.proposal.candidate.parent_snapshot_ref = expected.edit.base;
            expected.proposal.candidate.snapshot_ref = expected.edit.successor;
            const auto before = rate_image(file);
            expect_save_refusal(file, expected.edit, expected, expected.basis,
                                RateStoreRefusal::invalid_proposal);
            expect_refusal(preview(file, expected.edit, expected.basis),
                           RateStoreRefusal::invalid_proposal);
            require(rate_image(file) == before, "Fresh target absence is a zero-write refusal");
        });
    }

    void staleHeadAndSourceFacts_data() {
        QTest::addColumn<int>("test");
        QTest::newRow("disjoint-shipped-head-advance") << 0;
        QTest::newRow("trace-order") << 1;
        QTest::newRow("unused-G-time") << 2;
        QTest::newRow("policy-identity-only") << 3;
    }
    void staleHeadAndSourceFacts() {
        QFETCH(int, test);
        check([=] {
            const auto start = test == 3 ? Start::equal_rates : Start::pin_f_first;
            const auto contract = example(start);
            RateFile file;
            (void)establish(file, contract, start);
            const auto expected = preview_exact(file, contract.expected_preview());
            auto current = expected.basis;
            auto why = RateStoreRefusal::preview_changed;
            if (test == 0) {
                (void)introduce(file, pin("N", "T", "V", "P", "J2", "CHF", "SEK", "12.00"),
                                current);
                why = RateStoreRefusal::stale;
                expect_refusal(preview(file, expected.edit, current), why);
            } else if (test == 1) {
                std::swap(current.edges[1], current.edges[2]);
            } else if (test == 2) {
                current.observations[2].context.fetched_at_utc_ms = 0;
            } else {
                current.policy_ref = "B2";
            }
            expect_save_refusal(file, expected.edit, expected, current, why);
        });
    }

    void previewIdentityCollisions_data() {
        QTest::addColumn<QString>("decision");
        QTest::addColumn<QString>("successor");
        QTest::newRow("retained-decision-D") << QStringLiteral("D") << QStringLiteral("U");
        QTest::newRow("reserved-successor-E") << QStringLiteral("M") << QStringLiteral("E");
        QTest::newRow("retained-successor-S") << QStringLiteral("M") << QStringLiteral("S");
    }
    void previewIdentityCollisions() {
        QFETCH(QString, decision);
        QFETCH(QString, successor);
        check([=] {
            constexpr auto start = Start::pin_f_first;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            auto expected = contract.expected_preview();
            expected.edit.decision = decision.toStdString();
            expected.edit.successor = successor.toStdString();
            expect_refusal(preview(file, expected.edit, expected.basis),
                           RateStoreRefusal::identity_conflict);
            require(rate_image(file) == before, "Preview identity collision leaves no writes");
        });
    }

    void freshIdentityAndProposal() {
        check([] {
            constexpr auto start = Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            (void)establish(file, contract, start);
            const auto expected = contract.expected_preview();
            auto collision = expected;
            collision.edit.decision = "D";
            expect_save_refusal(file, collision.edit, collision, expected.basis,
                                RateStoreRefusal::identity_conflict);
            collision = expected;
            collision.edit.successor = "E";
            collision.proposal.candidate.snapshot_ref = "E";
            expect_save_refusal(file, collision.edit, collision, expected.basis,
                                RateStoreRefusal::identity_conflict);

            auto tampered = expected;
            // This candidate selects unindexed F#1. EUR/USD stays at 6/5, but
            // EUR/GBP becomes available while the reviewed delta stays empty.
            tampered.proposal.candidate.constraints.push_back(core::FetchedRateConstraint{"F", 1});
            auto &members = tampered.proposal.candidate.components.front().members;
            members.insert(members.begin() + 1, {"GBP", "9", "10"});
            expect_quote(tampered.proposal.candidate, "EUR", "USD", ratio(6, 5));
            expect_save_refusal(file, expected.edit, tampered, expected.basis,
                                RateStoreRefusal::preview_changed);
        });
    }

    void freshReviewedProposal_data() {
        QTest::addColumn<bool>("candidate");
        QTest::newRow("equal-rate-candidate-only") << true;
        QTest::newRow("reviewed-target-only") << false;
    }
    void freshReviewedProposal() {
        QFETCH(bool, candidate);
        check([=] {
            const auto start = candidate ? Start::equal_rates : Start::pin_f_first;
            const auto contract = example(start);
            RateFile file;
            (void)establish(file, contract, start);
            const auto expected = contract.expected_preview();
            auto tampered = expected;
            if (candidate) {
                // Change one candidate evidence field. Support, weights, review
                // deltas, and both copies of the basis still match.
                auto &snapshot = tampered.proposal.candidate;
                snapshot.sources.front().context.source_evidence_ref = "alternate-evidence";
                expect_quotes(contract, snapshot, start);
                require(snapshot_bytes(snapshot) != snapshot_bytes(expected.proposal.candidate),
                        "Equal-rate candidate has different canonical evidence bytes");
            } else {
                tampered.proposal.removed_override_ref = "Q";
            }
            expect_save_refusal(file, expected.edit, tampered, expected.basis,
                                RateStoreRefusal::preview_changed);
        });
    }

    void exactReplayAndConflicts() {
        check([] {
            constexpr auto start = Start::pin_f_first;
            const auto contract = example(start);
            RateFile file;
            std::optional<RateEditPreview> old_preview;
            const auto before = establish(file, contract, start, &old_preview);
            const auto expected = preview_exact(file, contract.expected_preview());
            expect_saved(*submit(file, expected), RateReceiptOrigin::newly_staged, expected);
            expect_transition(file, before, contract, start);
            const auto committed = rate_image(file);

            auto refreshed = permitted();
            refreshed.session.session_ref = "N2";
            refreshed.session.record_revision_ref = "R2";
            refreshed.session.snapshot_ref = "P2";
            auto unused_live_basis = expected.basis;
            unused_live_basis.policy_ref = "B2";
            std::swap(unused_live_basis.edges[1], unused_live_basis.edges[2]);
            const auto replay =
                enqueue_removal(file, expected.edit, expected, unused_live_basis, refreshed);
            await_removal(replay);
            expect_saved(*replay, RateReceiptOrigin::retained, expected);
            expect_save_refusal(file, expected.edit, expected, expected.basis,
                                RateStoreRefusal::denied, SessionFailure::revoked);
            auto without_use = permitted();
            without_use.graph.use.accounts.front().individual_entries.clear();
            expect_save_refusal(file, expected.edit, expected, expected.basis,
                                RateStoreRefusal::denied, without_use);
            require(rate_image(file) == committed,
                    "Replay and denials leave the first audit unchanged");

            // Removal under an introducing operation key conflicts. The original
            // introducing closure remains a separate verified historical result.
            auto legacy_key = expected;
            legacy_key.edit.operation = old_preview->edit.operation;
            expect_save_refusal(file, legacy_key.edit, legacy_key, legacy_key.basis,
                                RateStoreRefusal::identity_conflict);

            auto legacy_collision = pin("Z", "U", "V", "P", "New", "CHF", "SEK", "12.00");
            const auto legacy = selected<RatePreviewResult>(file, [=](const auto &unit) {
                Authority source(permitted());
                return unit.preview(source, literal_rate_trust(), legacy_collision, expected.basis);
            });
            const auto *legacy_preview = std::get_if<RateEditPreview>(&legacy);
            require(legacy_preview != nullptr, "Shipped preview uses the verified removal head");
            const auto rejected = rate_fixture::save(file, *legacy_preview, permitted());
            expect_refusal(*rejected->staged, RateStoreRefusal::identity_conflict);
            require(rejected->samples == 0 && rate_image(file) == committed,
                    "Legacy key conflict never stages an introduction");

            legacy_collision.operation = "later-key";
            (void)introduce(file, legacy_collision, expected.basis);
            const auto later = rate_image(file);
            const auto old_retry = rate_fixture::save(file, *old_preview, refreshed);
            const auto *staged = std::get_if<RateReceiptSelection>(&*old_retry->staged);
            require(staged != nullptr && old_retry->samples == 0,
                    "Legacy introduction replays its original receipt without a sample");
            const auto old_receipt = finish_rate_receipt(*staged, *old_retry->completion);
            require(old_receipt.has_value(), "Original legacy receipt completes normally");
            expect_receipt(*old_receipt, RateReceiptOrigin::retained, *old_preview);

            expect_saved(*submit(file, expected, refreshed), RateReceiptOrigin::retained, expected);
            require(later[4] == SqlRows{{blob("H"), blob("V")}},
                    "Shipped disjoint introduction advances U");
            const auto conflict = [&](const RateRemovalPreview &changed,
                                      RateAuthorityResolution auth = permitted()) {
                expect_save_refusal(file, changed.edit, changed, changed.basis,
                                    RateStoreRefusal::identity_conflict, std::move(auth));
            };
            auto changed = expected;
            changed.edit.removed_override_ref = "Q";
            changed.proposal.removed_override_ref = "Q";
            conflict(changed);
            changed = expected;
            changed.edit.pair = {"EUR", "GBP"};
            conflict(changed);
            changed = expected;
            changed.edit.actor = "B";
            conflict(changed, another_actor());
            changed = expected;
            changed.edit.base = "S";
            changed.proposal.candidate.parent_snapshot_ref = "S";
            conflict(changed);
            changed = expected;
            changed.edit.decision = "Different";
            conflict(changed);
            changed = expected;
            changed.basis.policy_ref = "B2";
            conflict(changed);
            changed = expected;
            changed.proposal.affected_rates.clear();
            conflict(changed);
            require(rate_image(file) == later,
                    "Repeating the old operation cannot undo or republish V");
        });
    }

    void observationAdmission_data() {
        QTest::addColumn<bool>("append");
        QTest::newRow("same-reference-different-unused-body") << false;
        QTest::newRow("new-unused-observation") << true;
    }
    void observationAdmission() {
        QFETCH(bool, append);
        check([=] {
            constexpr auto start = Start::pin_f_first;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            auto expected = contract.expected_preview();
            if (append) {
                auto x = unused_g;
                x.context.observation_ref = "X";
                expected.basis.observations.push_back(x);
            } else {
                expected.basis.observations[2].context.fetched_at_utc_ms = 0;
            }
            const auto submitted = preview_exact(file, expected);
            require(rate_image(file) == before, "Preview does not stage unindexed observations");
            if (!append) {
                expect_save_refusal(file, submitted.edit, submitted, submitted.basis,
                                    RateStoreRefusal::identity_conflict);
            } else {
                expect_saved(*submit(file, submitted), RateReceiptOrigin::newly_staged, expected);
                expect_transition(file, before, contract, start, true, true);
                const auto committed = rate_image(file);
                file.reopen();
                expect_snapshot(head(file), contract.after);
                expect_recovered(file, expected);
                expect_saved(*submit(file, expected), RateReceiptOrigin::retained, expected);
                require(rate_image(file) == committed, "Replay leaves original X bytes intact");
            }
        });
    }

    void competingWork_data() {
        QTest::addColumn<bool>("same_key");
        QTest::newRow("same-key-retained") << true;
        QTest::newRow("distinct-key-stale") << false;
    }
    void competingWork() {
        QFETCH(bool, same_key);
        check([=] {
            constexpr auto start = Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto expected = preview_exact(file, contract.expected_preview());
            auto next = expected;
            if (!same_key) {
                next.edit.operation = "other";
                next.edit.decision = "Other";
                next.edit.successor = "V";
                next.proposal.candidate.snapshot_ref = "V";
                next = preview_exact(file, next);
            }
            const auto first = enqueue_removal(file, expected.edit, expected, expected.basis);
            const auto second = enqueue_removal(file, next.edit, next, next.basis);
            await_removal(first);
            await_removal(second);
            expect_saved(*first, RateReceiptOrigin::newly_staged, expected);
            if (same_key) {
                expect_saved(*second, RateReceiptOrigin::retained, expected);
            } else {
                finance::normal(*second->completion);
                require(second->staged.has_value(), "Competing save returns a typed result");
                expect_refusal(*second->staged, RateStoreRefusal::stale);
                require(second->samples == 0 && second->authority_reads == 1 &&
                            !completed_receipt(*second),
                        "Spent base never samples or releases a second payload");
            }
            expect_transition(file, before, contract, start);
        });
    }

    void lateRollback_data() {
        QTest::addColumn<bool>("zero_row");
        QTest::newRow("SQL-abort") << false;
        QTest::newRow("zero-row-CAS") << true;
    }
    void lateRollback() {
        QFETCH(bool, zero_row);
        check([=] {
            constexpr auto start = Start::pin_f_first;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            auto expected = contract.expected_preview();
            auto x = unused_g;
            x.context.observation_ref = "X";
            expected.basis.observations.push_back(x);
            const auto submitted = preview_exact(file, expected);
            const auto fault =
                zero_row ? "RAISE(IGNORE)" : "RAISE(ABORT,'synthetic removal CAS fault')";
            file.install_fixture_trigger(
                QString(
                    "CREATE TRIGGER rate_removal_fault BEFORE UPDATE ON subathon_rate_head "
                    "WHEN EXISTS(SELECT 1 FROM subathon_rate_revisions WHERE snapshot_ref=X'55') "
                    "AND EXISTS(SELECT 1 FROM subathon_rate_decisions WHERE decision_ref=X'4d') "
                    "AND EXISTS(SELECT 1 FROM subathon_rate_observations WHERE "
                    "observation_ref=X'58') "
                    "BEGIN SELECT %1; END")
                    .arg(fault));
            const auto failed = submit(file, submitted);
            require(
                failed->completion->transaction == TransactionOutcome::rolled_back &&
                    failed->completion->error ==
                        (zero_row ? WorkError::work_exception : WorkError::sql_failure) &&
                    !failed->completion->value && !completed_receipt(*failed) &&
                    failed->samples == 1,
                "Late head failure preserves writer failure, rolls back and releases no receipt");
            require(rate_image(file) == before,
                    "Observer sees only the pre-attempt committed rows");
            file.reopen();
            require(rate_image(file) == before,
                    "No attempted observation, decision or revision survives reopen");
            require(std::holds_alternative<RateAttemptAbsent>(recover(file, submitted)),
                    "Authorized recovery verifies complete absence of the rolled-back attempt");
            require(rate_image(file) == before, "Absence verification is read-only");
            file.install_fixture_trigger("DROP TRIGGER rate_removal_fault");
            expect_saved(*submit(file, submitted), RateReceiptOrigin::newly_staged, expected);
            expect_transition(file, before, contract, start, true, true);
        });
    }

    void postCommitFailure_data() {
        QTest::addColumn<bool>("lose_ack");
        QTest::newRow("lost-commit-acknowledgement") << true;
        QTest::newRow("body-error-after-commit") << false;
    }
    void postCommitFailure() {
        QFETCH(bool, lose_ack);
        check([=] {
            constexpr auto start = Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto expected = preview_exact(file, contract.expected_preview());
            if (lose_ack)
                require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            *file.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Install the existing seam for real COMMIT acknowledgement loss");
            const auto result = enqueue_removal(file, expected.edit, expected, expected.basis,
                                                permitted(), 0, !lose_ack);
            await_removal(result);
            require(result->staged &&
                        std::holds_alternative<RateRemovalReceiptSelection>(*result->staged) &&
                        result->samples == 1,
                    "Prepared removal had one sample and a provisional receipt");
            require(result->completion->transaction == (lose_ack
                                                            ? TransactionOutcome::outcome_unknown
                                                            : TransactionOutcome::committed) &&
                        result->completion->error ==
                            (lose_ack ? WorkError::uncertain_outcome : WorkError::work_exception) &&
                        !result->completion->value && !completed_receipt(*result),
                    "No normal payload escapes unknown COMMIT or post-commit body failure");
            if (lose_ack)
                require(file.writer->enqueue_work([](auto) -> QVariant { return 1; },
                                                  [](auto) {}) == StorageError::shutting_down,
                        "Uncertain outcome closes new writer admission");
            expect_transition(file, before, contract, start);
            const auto committed = rate_image(file);
            file.reopen();
            expect_recovered(file, expected);
            expect_snapshot(head(file), contract.after);
            require(rate_image(file) == committed,
                    "Reopened recovery reads a single durable removal");
        });
    }

    void recoveryOfUnattemptedPreview() {
        check([] {
            constexpr auto start = Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto expected = contract.expected_preview();
            require(std::holds_alternative<RateAttemptAbsent>(recover(file, expected)),
                    "Authorized recovery verifies absence with an already retained source");
            expect_refusal(recover(file, expected, SessionFailure::revoked),
                           RateStoreRefusal::denied);
            require(rate_image(file) == before,
                    "Recovering a never-submitted preview is read-only");
        });
    }

    void authorityAtCommittedRecovery_data() { read_denials(); }
    void authorityAtCommittedRecovery() {
        QFETCH(int, reason);
        check([=] {
            constexpr auto start = Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            const auto before = establish(file, contract, start);
            const auto expected = contract.expected_preview();
            expect_saved(*submit(file, expected), RateReceiptOrigin::newly_staged, expected);
            expect_transition(file, before, contract, start);
            const auto committed = rate_image(file);
            file.reopen();
            expect_refusal(recover(file, expected, denied_read_authority(ReadDenial(reason))),
                           RateStoreRefusal::denied);
            expect_recovered(file, expected);
            require(rate_image(file) == committed,
                    "Denied and permitted recovery preserve the committed closure");
        });
    }

    void retainedClosure_data() {
        QTest::addColumn<int>("test");
        QTest::newRow("different-RRD1-account") << 0;
        QTest::newRow("missing-removal-decision") << 1;
        QTest::newRow("missing-original-occurrence") << 2;
        QTest::newRow("missing-unselected-observation") << 3;
    }
    void retainedClosure() {
        QFETCH(int, test);
        check([=] {
            const auto start = test == 3 ? Start::pin_f_first : Start::equal_rates;
            const auto contract = example(start);
            RateFile file;
            (void)establish(file, contract, start);
            const auto expected = preview_exact(file, contract.expected_preview());
            expect_saved(*submit(file, expected), RateReceiptOrigin::newly_staged, expected);
            if (test == 0) {
                auto forged = expected_audit(start, true);
                // RRD1 magic, five one-byte refs, two three-byte currencies,
                // O, then the four-byte size of the account field.
                constexpr auto account_offset = 4 + 5 * 5 + 2 * 7 + 5 + 4;
                require(forged[account_offset] == 'A',
                        "Corruption changes only the retained account");
                forged[account_offset] = 'B';
                file.write([=](const auto &session) {
                    (void)session.execute({"UPDATE subathon_rate_decisions SET decision=? "
                                           "WHERE decision_ref=X'4d'",
                                           {forged}});
                });
            } else if (test == 1) {
                file.write([](const auto &session) {
                    (void)session.execute(
                        {"DELETE FROM subathon_rate_decisions WHERE decision_ref=X'4d'", {}});
                });
            } else if (test == 2) {
                file.write([](const auto &session) {
                    (void)session.execute(
                        {"DELETE FROM subathon_rate_occurrences WHERE override_ref=X'4f'", {}});
                });
            } else {
                file.write([](const auto &session) {
                    (void)session.execute(
                        {"DELETE FROM subathon_rate_observations WHERE observation_ref=X'47'", {}});
                });
            }
            const auto corrupt = rate_image(file);
            file.reopen();
            if (test != 1) {
                expect_refusal(head(file), RateStoreRefusal::invalid_retained_data);
                expect_refusal(revision(file, "U"), RateStoreRefusal::invalid_retained_data);
                auto wrong_actor = expected.edit;
                wrong_actor.actor = "B";
                expect_save_refusal(file, wrong_actor, expected, expected.basis,
                                    RateStoreRefusal::denied);
                expect_save_refusal(file, expected.edit, expected, expected.basis,
                                    RateStoreRefusal::invalid_retained_data);
            }
            // The existing decision-less reader may accept a subset successor.
            // Recovery must still reject the partial attempt as retained damage.
            expect_refusal(recover(file, expected), RateStoreRefusal::invalid_retained_data);
            require(rate_image(file) == corrupt,
                    "Failed verification never repairs or rewrites evidence");
        });
    }
};
} // namespace

QTEST_GUILESS_MAIN(RateOverrideRemovalTests)
#include "tests.moc"
