// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_rate_override_pressure_fixture.hpp"

#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace rate_fixture;

template <class Test> void check(Test test) {
    try {
        test();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

RateRevisionResult head(RateFile &file) {
    return selected<RateRevisionResult>(file, [](const auto &unit) { return unit.head(); });
}
RateRevisionResult revision(RateFile &file, std::string ref) {
    return selected<RateRevisionResult>(file, [=](const auto &unit) { return unit.revision(ref); });
}
RateRecoveryResult recover(RateFile &file, const RateEditPreview &preview,
                           RateAuthorityResolution authority = authorized()) {
    return selected<RateRecoveryResult>(file, [=](const auto &unit) {
        Authority source(authority);
        auto result = unit.recover(source, literal_rate_trust(), preview.edit, preview);
        require(source.reads == 1, "Recovery resolves current authority once");
        return result;
    });
}
RatePreviewResult propose(RateFile &file, const RateOverrideEdit &input,
                          const RateSourceBasis &basis,
                          RateAuthorityResolution authority = authorized()) {
    return selected<RatePreviewResult>(file, [=](const auto &unit) {
        Authority source(authority);
        auto result = unit.preview(source, literal_rate_trust(), input, basis);
        require(source.reads == 1, "Preview resolves current authority once");
        return result;
    });
}
void expect_first_image(const RateFile &file, const RateEditPreview &preview,
                        bool fetched = false) {
    const auto revisions = file.rows(revision_rows);
    require(revisions.size() == 2, "First save keeps E and adds exactly one revision");
    expect_reserved_root(revisions.front());
    require(
        revisions.back() == SqlRow{blob("S"), blob("E"), rsv_format,
                                   snapshot_bytes(preview.proposal.candidate)} &&
            file.rows(head_rows) == SqlRows{{blob("H"), blob("S")}} &&
            file.rows(decision_rows) ==
                SqlRows{{blob("D"), blob("I"), blob("K"), blob("E"), blob("S"), rin_format,
                         fetched ? fetched_intent : first_intent, rdc_format, first_decision}} &&
            file.rows(occurrence_rows) == SqlRows{{blob("O"), blob("D"), blob("S"), first_body}},
        "Exactly one immutable decision, occurrence and successor linked to root and head");
    const SqlRows expected = fetched ? SqlRows{{blob("F"), rob_format, selected_rob},
                                               {blob("G"), rob_format, unused_rob}}
                                     : SqlRows{};
    require(file.rows(observation_rows) == expected, "Exact observation rows including unused G");
}
RateEditPreview surviving_preview() {
    auto preview = first_preview();
    preview.edit = edit("K2", "S", "T", "P2", "D2");
    auto &input = preview.edit.override_input;
    input.to_currency = "GBP";
    input.original_decimal = "0.90";
    auto &candidate = preview.proposal.candidate;
    candidate.snapshot_ref = "T";
    candidate.parent_snapshot_ref = "S";
    candidate.overrides.push_back(input);
    candidate.constraints.push_back(core::ManualRateConstraint{"P2"});
    candidate.components = {{{{"EUR", "1", "1"}, {"GBP", "9", "10"}, {"USD", "6", "5"}}}};
    preview.proposal.affected_rates = {change("EUR", "GBP", 9, 10), change("GBP", "EUR", 10, 9),
                                       change("GBP", "USD", 4, 3), change("USD", "GBP", 3, 4)};
    return preview;
}
void seed_survivor(RateFile &file) {
    const auto next = surviving_preview();
    file.write([=](const auto &session) {
        seed_revision(session, next.proposal.candidate);
        const auto intent = QByteArray::fromHex(
            "52494e31 0000000149 000000024b32 0000000153 0000000154 "
            "000000025032 01 00000003455552 00000003474250 00000004302e3930 "
            "000000024432 01 0000000141 0000000172 00 00 0000000142 00000000 00000000");
        const auto decision = QByteArray::fromHex(
            "52444331 0000000149 000000024b32 000000024432 0000000153 0000000154 000000025032 "
            "00 0000000141 000000014e 0000000152 0000000150 "
            "000000137065726d697373696f6e2d72617465732d7631 0000000000000001 "
            "00 0000000155 0000000147 01 0000000000000000");
        const auto occurrence =
            QByteArray::fromHex("000000025032 01 00000003455552 00000003474250 00000004302e3930 "
                                "000000024432 01 0000000141 0000000172 00");
        (void)session.execute({"INSERT INTO subathon_rate_decisions VALUES(?,?,?,?,?,?,?,?,?)",
                               {blob("D2"), blob("I"), blob("K2"), blob("S"), blob("T"), rin_format,
                                intent, rdc_format, decision}});
        (void)session.execute({"INSERT INTO subathon_rate_occurrences VALUES(?,?,?,?)",
                               {blob("P2"), blob("D2"), blob("T"), occurrence}});
    });
}
RateAuthorityFacts all_pairs() {
    auto auth = authorized();
    auth.graph.use.accounts.front().individual_entries = {
        grant({pair_eur_usd, pair_eur_gbp, pair_gbp_usd})};
    return auth;
}

class SqliteRateOverrideWriterTests : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void rootSelectedAfterReopen_R01() {
        check([] {
            RateFile file;
            file.reopen();
            auto result = head(file);
            require(snapshot_bytes(std::get<core::EffectiveRateSnapshot>(result)) == root_bytes,
                    "Head reopens the literal reserved root");
            const core::EffectiveRateSnapshotInput root{"E", {}, {}, {}, {}, {}, {}};
            require(core::quote_effective_rate(root, "EUR", "USD").data().kind ==
                            core::RatePairKind::missing &&
                        core::quote_effective_rate(root, "EUR", "EUR").data().rate == ratio(1),
                    "Root has missing cross-rates and exact currency identity");
        });
    }

    void firstPreviewAndCommit_R01() {
        check([] {
            RateFile file;
            const auto expected = first_preview();
            const auto result = propose(file, expected.edit, empty_basis);
            const auto &actual = std::get<RateEditPreview>(result);
            require(actual.edit.installation == "I" &&
                        actual.edit.override_input.actor_ref == "A" && actual.edit.base == "E" &&
                        actual.edit.operation == "K" && actual.basis.policy_ref == "B" &&
                        actual.basis.edges.empty() && actual.basis.observations.empty() &&
                        !actual.proposal.replaced_override_ref &&
                        snapshot_bytes(actual.proposal.candidate) ==
                            snapshot_bytes(expected.proposal.candidate),
                    "Owned preview binds the exact edit, basis and canonical candidate");
            expect_changes(actual.proposal.affected_rates, first_changes);
            const auto accepted = save(file, expected);
            const auto selection = std::get<RateReceiptSelection>(*accepted->staged);
            expect_receipt(selection, RateReceiptOrigin::newly_staged, expected);
            require(accepted->samples == 1, "New decision samples once, excluding preview");
            const auto delivered = finish_rate_receipt(selection, *accepted->completion);
            require(delivered.has_value(), "Acknowledged normal COMMIT delivers the receipt");
            expect_receipt(*delivered, RateReceiptOrigin::newly_staged, expected);
            expect_first_image(file, expected);
            file.reopen();
            require(
                snapshot_bytes(std::get<core::EffectiveRateSnapshot>(revision(file, "S"))) ==
                    snapshot_bytes(expected.proposal.candidate),
                "Selected committed successor reconstructs exact canonical snapshot after reopen");
        });
    }

    void deniedBeforeLookup_R02_data() {
        QTest::addColumn<int>("reason");
        QTest::newRow("different-pair") << 0;
        QTest::newRow("individual-deny-over-role") << 1;
        QTest::newRow("revoked-session") << 2;
        QTest::newRow("unavailable-graph") << 3;
        QTest::newRow("unsynchronized") << 4;
        QTest::newRow("missing-grant-owner-has-no-shortcut") << 5;
        QTest::newRow("reversed-typed-scope") << 6;
        QTest::newRow("uppercase-typed-scope") << 7;
    }
    void deniedBeforeLookup_R02() {
        QFETCH(int, reason);
        check([=] {
            RateFile file;
            auto preview = first_preview();
            auto facts = authorized();
            auto &who = facts.graph.use.accounts.front();
            RateAuthorityResolution authority = facts;
            if (reason == 0 || reason == 7) {
                const auto *currency = reason == 7 ? "JPY" : "GBP";
                preview.edit.override_input.to_currency = currency;
                preview.proposal.candidate.overrides.front().to_currency = currency;
                preview.proposal.candidate.components.front().members.back().currency = currency;
                preview.proposal.affected_rates = {change("EUR", currency, 6, 5),
                                                   change(currency, "EUR", 5, 6)};
                if (reason == 7)
                    who.individual_entries = {grant({"00000003455552000000034A5059"})};
            } else if (reason == 1) {
                auto role_allow = grant();
                role_allow.origin = core::PermissionOrigin::role;
                role_allow.role = "role";
                role_allow.source = "role";
                facts.graph.use.roles = {{"role", {role_allow}}};
                who.assigned_roles = {"role"};
                who.individual_entries = {{core::PermissionOrigin::individual,
                                           "A",
                                           {},
                                           operation,
                                           core::PermissionEffect::deny,
                                           {},
                                           {{"pair", core::PermissionResourceMode::any, {}}},
                                           {}}};
            } else if (reason == 4) {
                facts.graph.catalog = reference_permission_graph_trust().catalog;
            } else if (reason == 5) {
                who.individual_entries.clear();
            } else if (reason == 6) {
                who.individual_entries = {grant({"0000000355534400000003455552"})};
            }
            authority = facts;
            if (reason == 2)
                authority = SessionFailure::revoked;
            if (reason == 3)
                authority = PermissionGraphFailure::unavailable;

            // Poison the protected lookup, without changing the valid catalog marker.
            // An actual SELECT of this relation would execute the real failing view.
            file.write([](const auto &session) {
                (void)session.execute({"DROP TABLE subathon_rate_occurrences", {}});
                (void)session.execute({"DROP TABLE subathon_rate_decisions", {}});
                (void)session.execute(
                    {"CREATE VIEW subathon_rate_decisions AS SELECT * FROM absent_rate_data", {}});
            });
            const auto probe = qt_test_support::run_work(
                *file.writer,
                [](auto context) -> QVariant {
                    const auto session = context.begin();
                    (void)session.execute({"SELECT * FROM subathon_rate_decisions", {}});
                    session.commit();
                    return 1;
                },
                10000);
            require(probe.transaction == TransactionOutcome::rolled_back &&
                        probe.error == WorkError::sql_failure && !probe.value,
                    "Looking up the protected relation really fails at SQLite");
            const auto mark = file.rows(history_sql);
            const auto existing_head = file.rows(head_rows);
            const auto denied = save(file, preview, authority);
            const auto *refusal = std::get_if<RateStoreRefusal>(&*denied->staged);
            require(
                refusal &&
                    (*refusal == RateStoreRefusal::denied ||
                     ((reason == 3 || reason == 4) && *refusal == RateStoreRefusal::unavailable)) &&
                    denied->samples == 0 && file.rows(history_sql) == mark &&
                    file.rows(head_rows) == existing_head,
                "Live refusal precedes a failing protected operation lookup and any acceptance "
                "sample");
        });
    }

    void newEditMustMatchAuthenticatedSession_R02_data() {
        QTest::addColumn<int>("mismatch");
        QTest::newRow("different-actor") << 0;
        QTest::newRow("absent-actor") << 1;
        QTest::newRow("different-installation") << 2;
    }
    void newEditMustMatchAuthenticatedSession_R02() {
        QFETCH(int, mismatch);
        check([=] {
            RateFile file;
            auto preview = first_preview();
            if (mismatch == 2)
                preview.edit.installation = "other-installation";
            else if (mismatch == 1)
                preview.edit.override_input.actor_ref.reset();
            else
                preview.edit.override_input.actor_ref = "B";
            preview.proposal.candidate.overrides.front() = preview.edit.override_input;

            const auto before = rate_image(file);
            const auto result = save(file, preview);
            const auto *refusal = std::get_if<RateStoreRefusal>(&*result->staged);
            require(refusal &&
                        (*refusal == RateStoreRefusal::denied ||
                         *refusal == RateStoreRefusal::invalid_proposal) &&
                        result->samples == 0 && rate_image(file) == before,
                    "Unbound actor or installation leaves no receipt, sample or stored effect");
        });
    }

    void explicitReverseReplacementAndExactChanges_R03() {
        check([] {
            RateFile file;
            seed_first(file, true);
            // Equal to the existing manual constraint in the old revision. Keep
            // the independently stored F document, original ordinal and rule order.
            auto old = fetched_preview();
            old.basis.observations.front().quotes.back().original_decimal = "1.20";
            old.proposal.candidate.sources = {old.basis.observations.front()};
            old.proposal.candidate.constraints.push_back(core::FetchedRateConstraint{"F", 2});
            auto matching_rob = selected_rob;
            matching_rob.replace("1.30", "1.20");
            file.write([=](const auto &session) {
                (void)session.execute(
                    {"UPDATE subathon_rate_observations SET document=? WHERE observation_ref=X'46'",
                     {matching_rob}});
                (void)session.execute(
                    {"UPDATE subathon_rate_revisions SET document=? WHERE snapshot_ref=X'53'",
                     {snapshot_bytes(old.proposal.candidate)}});
            });

            auto next = replacement_preview();
            next.basis = old.basis;
            auto &input = next.edit.override_input;
            input.from_currency = "USD";
            input.to_currency = "EUR";
            input.original_decimal = "0.80";
            auto &candidate = next.proposal.candidate;
            candidate.sources = old.proposal.candidate.sources;
            candidate.overrides = {input};
            candidate.constraints.push_back(core::FetchedRateConstraint{"F", 1});
            candidate.components = {{{{"EUR", "1", "1"}, {"GBP", "9", "10"}, {"USD", "5", "4"}}}};
            next.proposal.affected_rates = {
                change("EUR", "USD", 5, 4, ratio(6, 5)), change("GBP", "USD", 25, 18, ratio(4, 3)),
                change("USD", "EUR", 4, 5, ratio(5, 6)), change("USD", "GBP", 18, 25, ratio(3, 4))};

            const auto before = rate_image(file);
            for (const auto &replacement :
                 {std::optional<std::string>{}, std::optional<std::string>{"wrong"}}) {
                auto invalid = next.edit;
                invalid.replacement = replacement;
                expect_refusal(propose(file, invalid, next.basis),
                               RateStoreRefusal::invalid_proposal);
            }
            for (const auto *rate : {"0", "-1"}) {
                auto invalid = next.edit;
                invalid.override_input.original_decimal = rate;
                expect_refusal(propose(file, invalid, next.basis),
                               RateStoreRefusal::invalid_proposal);
            }
            require(rate_image(file) == before, "Rejected replacement previews preserve all rows");
            const auto result = save(file, next);
            expect_receipt(std::get<RateReceiptSelection>(*result->staged),
                           RateReceiptOrigin::newly_staged, next);
            require(file.rows(observation_rows) == before[1] &&
                        file.rows(occurrence_rows).front() == before[3].front(),
                    "Whole fetched facts and the replaced occurrence survive");
            file.reopen();
            require(snapshot_bytes(std::get<core::EffectiveRateSnapshot>(head(file))) ==
                        snapshot_bytes(candidate),
                    "Reverse replacement persists the exact graph and retains both ordered source "
                    "quotes");
        });
    }

    void contradictoryManualAndOtherPairTarget_R03() {
        check([] {
            RateFile file;
            seed_first(file);
            seed_survivor(file);
            // Use the frozen contradictory cross-pair example. Update both
            // embedded copies and the original occurrence/intent consistently.
            auto first = first_preview().proposal.candidate;
            auto second = surviving_preview().proposal.candidate;
            first.overrides.front().original_decimal = "1.25";
            second.overrides.front().original_decimal = "1.25";
            first.components.front().members.back() = {"USD", "5", "4"};
            second.components.front().members.back() = {"USD", "5", "4"};
            auto occurrence = first_body;
            auto intent = first_intent;
            occurrence.replace("1.20", "1.25");
            intent.replace("1.20", "1.25");
            file.write([=](const auto &session) {
                (void)session.execute(
                    {"UPDATE subathon_rate_revisions SET document=? WHERE snapshot_ref=X'53'",
                     {snapshot_bytes(first)}});
                (void)session.execute(
                    {"UPDATE subathon_rate_revisions SET document=? WHERE snapshot_ref=X'54'",
                     {snapshot_bytes(second)}});
                (void)session.execute(
                    {"UPDATE subathon_rate_occurrences SET body=? WHERE override_ref=X'4F'",
                     {occurrence}});
                (void)session.execute(
                    {"UPDATE subathon_rate_decisions SET intent=? WHERE decision_ref=X'44'",
                     {intent}});
            });
            auto input = edit("K3", "T", "U", "O3", "D3");
            input.override_input.from_currency = "USD";
            input.override_input.to_currency = "GBP";
            input.override_input.original_decimal = "0.72000000000000000001";
            const auto before = rate_image(file);
            expect_refusal(propose(file, input, empty_basis, all_pairs()),
                           RateStoreRefusal::invalid_proposal);
            input.override_input.original_decimal = "0.72";
            const auto control = propose(file, input, empty_basis, all_pairs());
            require(std::get<RateEditPreview>(control).proposal.affected_rates.empty(),
                    "The exact compatible cross-rate can be pinned without a directed change");
            input.replacement = "O";
            expect_refusal(propose(file, input, empty_basis, all_pairs()),
                           RateStoreRefusal::invalid_proposal);
            require(rate_image(file) == before,
                    "Contradiction and other-pair target write nothing");
        });
    }

    void replayUsesOriginalIntentAndWitness_R04() {
        check([] {
            RateFile file;
            seed_first(file);
            seed_second(file);
            file.reopen();
            const auto earlier = first_preview();
            const auto before = rate_image(file);
            auto refreshed = authorized();
            refreshed.session.session_ref = "new-session";
            refreshed.session.record_revision_ref = "new-record";
            refreshed.session.snapshot_ref = "new-snapshot";
            refreshed.session.fresh = false;
            refreshed.graph.revision = 9;
            refreshed.graph.use.accounts.front().individual_entries.front().grant = "new-grant";
            auto changed_live_basis = empty_basis;
            changed_live_basis.policy_ref = "new-policy";
            const auto retry =
                enqueue_save(*file.writer, earlier.edit, earlier, changed_live_basis, refreshed);
            wait_for_save(retry);
            finance::normal(*retry->completion);
            expect_receipt(std::get<RateReceiptSelection>(*retry->staged),
                           RateReceiptOrigin::retained, earlier);
            require(retry->authority_reads == 1 && retry->samples == 0 &&
                        rate_image(file) == before,
                    "Authorized replay uses original audit bytes and keeps T active");

            const auto same_rate = replacement_preview();
            expect_receipt(std::get<RateReceiptSelection>(recover(file, same_rate)),
                           RateReceiptOrigin::retained, same_rate);
            auto stale = earlier;
            stale.edit.operation = "K3";
            expect_refusal(*save(file, stale)->staged, RateStoreRefusal::stale);
            auto revoked = refreshed;
            revoked.graph.use.accounts.front().individual_entries.clear();
            expect_refusal(*save(file, earlier, revoked)->staged, RateStoreRefusal::denied);
            expect_refusal(recover(file, earlier, revoked), RateStoreRefusal::denied);
            expect_refusal(recover(file, stale, revoked), RateStoreRefusal::denied);
            expect_refusal(*save(file, stale, revoked)->staged, RateStoreRefusal::denied);
            require(rate_image(file) == before,
                    "Revoked historical and unknown-key results disclose nothing");
        });
    }

    void historicalRecoveryIgnoresLiveHead_R04() {
        check([] {
            RateFile file;
            seed_first(file);
            seed_second(file);
            file.write([](const auto &session) {
                (void)session.execute({"DELETE FROM subathon_rate_head", {}});
            });
            const auto before = rate_image(file);
            const auto wanted = first_preview();
            expect_receipt(std::get<RateReceiptSelection>(recover(file, wanted)),
                           RateReceiptOrigin::retained, wanted);
            require(rate_image(file) == before,
                    "A selected historical read does not consult or reconstruct the live head");
        });
    }

    void previewEqualityOperands_R04_data() {
        QTest::addColumn<int>("mutation");
        QTest::newRow("absent-construction-time-becomes-utc-zero") << 0;
        QTest::newRow("absent-override-time-becomes-utc-zero") << 1;
        QTest::newRow("directed-change-order") << 2;
    }
    void previewEqualityOperands_R04() {
        QFETCH(int, mutation);
        check([=] {
            RateFile file;
            auto expected = first_preview();
            if (mutation == 0)
                expected.proposal.candidate.constructed_at_utc_ms = 0;
            else if (mutation == 1)
                expected.proposal.candidate.overrides.front().recorded_at_utc_ms = 0;
            else
                std::reverse(expected.proposal.affected_rates.begin(),
                             expected.proposal.affected_rates.end());
            const auto before = rate_image(file);
            const auto refused = save(file, expected);
            expect_refusal(*refused->staged, RateStoreRefusal::preview_changed);
            require(refused->samples == 0 && rate_image(file) == before,
                    "Equal displayed rates cannot replace exact preview meaning");
        });
    }

    void changedIntentAndCollisions_R04_data() {
        QTest::addColumn<int>("mutation");
        QTest::newRow("original-spelling") << 0;
        QTest::newRow("actor") << 1;
        QTest::newRow("decision") << 2;
        QTest::newRow("replacement") << 3;
        QTest::newRow("reused-override") << 4;
        QTest::newRow("reused-decision") << 5;
        QTest::newRow("reused-successor") << 6;
        QTest::newRow("reserved-root") << 7;
    }
    void changedIntentAndCollisions_R04() {
        QFETCH(int, mutation);
        check([=] {
            RateFile file;
            seed_first(file);
            auto input = mutation < 4 ? first_preview() : replacement_preview();
            if (mutation == 6) {
                seed_second(file);
                input.edit = edit("K3", "T", "S", "O3", "D3");
                input.edit.replacement = "O2";
                input.proposal.replaced_override_ref = "O2";
                input.proposal.candidate.parent_snapshot_ref = "T";
            }
            if (mutation == 0)
                input.edit.override_input.original_decimal = "1.200";
            auto authority = authorized();
            if (mutation == 1) {
                input.edit.override_input.actor_ref = "B";
                authority.session = {"I", "B", "NB", "RB", "PB", 0, 100, true};
                auto account_grant = grant();
                account_grant.source = "UB";
                account_grant.grant = "GB";
                authority.graph.use.accounts.push_back({"B", {}, {account_grant}});
                authority.graph.grant.accounts.push_back({"B", {}, {}});
                authority.graph.current_accounts.push_back({{"I", "B", "b", true}, "bR"});
            }
            if (mutation == 2 || mutation == 5)
                input.edit.override_input.decision_ref = mutation == 2 ? "changed" : "D";
            if (mutation == 3) {
                input.edit.replacement = "O";
                input.proposal.replaced_override_ref = "O";
            }
            if (mutation == 4)
                input.edit.override_input.override_ref = "O";
            if (mutation >= 6)
                input.edit.successor = mutation == 6 ? "S" : "E";
            input.proposal.candidate.snapshot_ref = input.edit.successor;
            input.proposal.candidate.overrides = {input.edit.override_input};
            input.proposal.candidate.constraints = {
                core::ManualRateConstraint{input.edit.override_input.override_ref}};
            const auto before = rate_image(file);
            const auto conflict = save(file, input, authority);
            expect_refusal(*conflict->staged, RateStoreRefusal::identity_conflict);
            require(conflict->samples == 0 && rate_image(file) == before,
                    "Conflict preserves complete original and active evidence without sampling");
        });
    }

    void sameValueReplacementAndLiveFactPrecedence_R04() {
        check([] {
            RateFile file;
            seed_first(file);
            const auto next = replacement_preview();
            const auto before = rate_image(file);
            auto revoked = authorized();
            revoked.graph.use.accounts.front().individual_entries.clear();
            expect_refusal(*save(file, next, revoked)->staged, RateStoreRefusal::denied);
            auto new_policy = next.basis;
            new_policy.policy_ref = "B2";
            auto changed = enqueue_save(*file.writer, next.edit, next, new_policy, authorized());
            wait_for_save(changed);
            finance::normal(*changed->completion);
            expect_refusal(*changed->staged, RateStoreRefusal::preview_changed);
            require(
                changed->samples == 0 && rate_image(file) == before,
                "Revocation and same-value policy change leave base and all audit rows unchanged");
            const auto accepted = save(file, next);
            expect_receipt(std::get<RateReceiptSelection>(*accepted->staged),
                           RateReceiptOrigin::newly_staged, next);
            const auto now = rate_image(file);
            require(now[0].mid(0, 2) == before[0] &&
                        now[0].back() == SqlRow{blob("T"), blob("S"), rsv_format,
                                                snapshot_bytes(next.proposal.candidate)} &&
                        now[1] == before[1] && now[2].mid(0, 1) == before[2] &&
                        now[2].back() == SqlRow{blob("D2"), blob("I"), blob("K2"), blob("S"),
                                                blob("T"), rin_format, second_intent, rdc_format,
                                                second_decision} &&
                        now[3].mid(0, 1) == before[3] &&
                        now[3].back() == SqlRow{blob("O2"), blob("D2"), blob("T"), second_body} &&
                        now[4] == SqlRows{{blob("H"), blob("T")}},
                    "Same-value replacement commits new exact bytes and retains its predecessor");
        });
    }

    void observationBytesAndSameRefConflict_R05() {
        check([] {
            RateFile file;
            const auto expected = fetched_preview();
            const auto accepted = save(file, expected);
            expect_receipt(std::get<RateReceiptSelection>(*accepted->staged),
                           RateReceiptOrigin::newly_staged, expected);
            file.reopen();
            expect_first_image(file, expected, true);
        });
    }

    void observationConflictAndTrace_R05_data() {
        QTest::addColumn<int>("mutation");
        QTest::newRow("selected-excluded-quote-lexeme") << 0;
        QTest::newRow("selected-present-empty-to-absent") << 1;
        QTest::newRow("unused-lexeme") << 2;
        QTest::newRow("fresh-live-edge-order") << 3;
        QTest::newRow("new-operation-same-ref-unused-body") << 4;
    }
    void observationConflictAndTrace_R05() {
        QFETCH(int, mutation);
        check([=] {
            RateFile file;
            if (mutation != 3)
                seed_first(file, true);
            auto expected = fetched_preview();
            auto current = expected.basis;
            if (mutation == 0)
                expected.basis.observations.front().quotes.back().original_decimal = "1.300";
            if (mutation == 1)
                expected.basis.observations.front().context.publication_time.original_text.reset();
            if (mutation < 2)
                expected.proposal.candidate.sources = {expected.basis.observations.front()};
            if (mutation == 2 || mutation == 4)
                expected.basis.observations.back().quotes.front().original_decimal = "2.00";
            if (mutation == 3)
                std::reverse(current.edges.begin(), current.edges.end());
            if (mutation == 4) {
                const auto replacement = replacement_preview();
                expected.edit = replacement.edit;
                auto &candidate = expected.proposal.candidate;
                candidate.snapshot_ref = "T";
                candidate.parent_snapshot_ref = "S";
                candidate.overrides = replacement.proposal.candidate.overrides;
                candidate.constraints.front() = core::ManualRateConstraint{"O2"};
                expected.proposal.replaced_override_ref = "O";
                expected.proposal.affected_rates.clear();
                current = expected.basis;
            }
            const auto before = rate_image(file);
            const auto result =
                enqueue_save(*file.writer, expected.edit, expected, current, authorized());
            wait_for_save(result);
            finance::normal(*result->completion);
            expect_refusal(*result->staged, mutation == 3 ? RateStoreRefusal::preview_changed
                                                          : RateStoreRefusal::identity_conflict);
            require(rate_image(file) == before && result->samples == 0,
                    "Equal numeric rates cannot repair changed immutable bytes or edge order");
        });
    }

    void competingWorkItems_R07_data() {
        QTest::addColumn<bool>("same_intent");
        QTest::newRow("distinct-keys-first-commit-second-stale") << false;
        QTest::newRow("duplicate-first-commit-original-receipt") << true;
    }
    void competingWorkItems_R07() {
        QFETCH(bool, same_intent);
        check([=] {
            RateFile file;
            const auto first = first_preview();
            auto second = first;
            if (!same_intent) {
                second.edit = edit("J", "E", "T", "P", "C");
                second.proposal.candidate.snapshot_ref = "T";
                second.proposal.candidate.overrides = {second.edit.override_input};
                second.proposal.candidate.constraints = {core::ManualRateConstraint{"P"}};
            }
            const auto a = enqueue_save(*file.writer, first.edit, first, empty_basis, authorized());
            const auto b =
                enqueue_save(*file.writer, second.edit, second, empty_basis, authorized());
            wait_for_save(a);
            wait_for_save(b);
            finance::normal(*a->completion);
            finance::normal(*b->completion);
            expect_receipt(std::get<RateReceiptSelection>(*a->staged),
                           RateReceiptOrigin::newly_staged, first);
            if (same_intent)
                expect_receipt(std::get<RateReceiptSelection>(*b->staged),
                               RateReceiptOrigin::retained, first);
            else
                expect_refusal(*b->staged, RateStoreRefusal::stale);
            require(
                a->samples == 1 && b->samples == 0 && a->authority_reads == 1 &&
                    b->authority_reads == 1,
                "Competing work uses two current authority resolutions and one acceptance sample");
            expect_first_image(file, first);
        });
    }

    void lateFailureRollback_R07_data() {
        QTest::addColumn<bool>("ignore_cas");
        QTest::newRow("abort-at-staged-head-update") << false;
        QTest::newRow("zero-row-CAS-after-staging") << true;
    }
    void lateFailureRollback_R07() {
        QFETCH(bool, ignore_cas);
        check([=] {
            RateFile file;
            seed_first(file);
            auto next = replacement_preview();
            const auto with_sources = fetched_preview();
            next.basis = with_sources.basis;
            next.proposal.candidate.sources = with_sources.proposal.candidate.sources;
            next.proposal.candidate.constraints.push_back(core::FetchedRateConstraint{"F", 1});
            next.proposal.candidate.components = with_sources.proposal.candidate.components;
            next.proposal.affected_rates = surviving_preview().proposal.affected_rates;
            const auto fault =
                ignore_cas ? "RAISE(IGNORE)" : "RAISE(ABORT,'synthetic late CAS fault')";
            file.install_fixture_trigger(
                QString(
                    "CREATE TRIGGER rate_fault BEFORE UPDATE ON subathon_rate_head "
                    "WHEN EXISTS(SELECT 1 FROM subathon_rate_decisions WHERE decision_ref=X'4432') "
                    "AND EXISTS(SELECT 1 FROM subathon_rate_occurrences WHERE "
                    "override_ref=X'4F32') "
                    "AND (SELECT count(*) FROM subathon_rate_observations)=2 "
                    "BEGIN SELECT %1; END")
                    .arg(fault));
            const auto before = rate_image(file);
            const auto result =
                enqueue_save(*file.writer, next.edit, next, next.basis, authorized());
            wait_for_save(result);
            require(result->completion->transaction == TransactionOutcome::rolled_back &&
                        !result->completion->value && result->samples == 1,
                    "Reached the late head write and confirmed the entire attempted append rolled "
                    "back");
            if (!ignore_cas)
                require(result->completion->error == WorkError::sql_failure,
                        "Real failing SQL retains the writer's first error");
            else
                require(result->completion->error == WorkError::work_exception,
                        "Zero affected rows requires failure and rollback");
            require(rate_image(file) == before, "Independent observer sees only preexisting rows");
            file.reopen();
            require(rate_image(file) == before,
                    "Reopen sees no staged revisions, decisions, occurrences or observations");
        });
    }

    void physicalCommitAndKnownRollback_R08_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("lost-physical-commit-acknowledgement") << 0;
        QTest::newRow("body-failure-after-confirmed-commit") << 1;
        QTest::newRow("confirmed-rollback") << 2;
    }
    void physicalCommitAndKnownRollback_R08() {
        QFETCH(int, fault);
        check([=] {
            RateFile file;
            const auto before = rate_image(file);
            const auto preview = first_preview();
            if (fault == 0)
                require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            *file.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Existing fault seam loses acknowledgment only after physical COMMIT");
            const auto result = enqueue_save(*file.writer, preview.edit, preview, empty_basis,
                                             authorized(), fault == 1, fault == 2);
            wait_for_save(result);
            const auto expected_outcome = fault == 0   ? TransactionOutcome::outcome_unknown
                                          : fault == 1 ? TransactionOutcome::committed
                                                       : TransactionOutcome::rolled_back;
            const auto expected_error = fault == 0   ? WorkError::uncertain_outcome
                                        : fault == 1 ? WorkError::work_exception
                                                     : WorkError::not_committed;
            require(result->completion->transaction == expected_outcome &&
                        result->completion->error == expected_error && !result->completion->value &&
                        !finish_rate_receipt(std::get<RateReceiptSelection>(*result->staged),
                                             *result->completion),
                    "No rate success can publish from unknown, postcommit body error or rollback");
            if (fault == 0)
                require(file.writer->enqueue_work([](auto) -> QVariant { return 1; },
                                                  [](auto) {}) == StorageError::shutting_down,
                        "Unknown outcome closes new writer admission");
            if (fault == 2) {
                require(rate_image(file) == before, "Known rollback retained exact root");
                file.reopen();
                require(std::holds_alternative<RateAttemptAbsent>(recover(file, preview)),
                        "Fresh authorized committed read verifies complete absence");
                expect_refusal(*save(file, preview, SessionFailure::revoked)->staged,
                               RateStoreRefusal::denied);
                const auto accepted = save(file, preview);
                expect_receipt(std::get<RateReceiptSelection>(*accepted->staged),
                               RateReceiptOrigin::newly_staged, preview);
                const auto spent = replacement_preview();
                auto stale = spent;
                stale.edit.base = "E";
                stale.proposal.candidate.parent_snapshot_ref = "E";
                expect_refusal(*save(file, stale)->staged, RateStoreRefusal::stale);
            } else {
                expect_first_image(file, preview);
                file.reopen();
                seed_second(file);
                const auto committed = rate_image(file);
                expect_receipt(std::get<RateReceiptSelection>(recover(file, preview)),
                               RateReceiptOrigin::retained, preview);
                auto wrong = preview;
                wrong.edit.override_input.original_decimal = "1.200";
                wrong.proposal.candidate.overrides = {wrong.edit.override_input};
                expect_refusal(recover(file, wrong), RateStoreRefusal::identity_conflict);
                require(rate_image(file) == committed, "Historical recovery does not republish S");
                file.write([](const auto &session) {
                    (void)session.execute(
                        {"DELETE FROM subathon_rate_occurrences WHERE override_ref=X'4F'", {}});
                });
                expect_refusal(recover(file, preview), RateStoreRefusal::invalid_retained_data);
            }
        });
    }

    void missingOperationWithPartialWriteIsUnresolved_R08() {
        check([] {
            RateFile file;
            seed_first(file);
            file.write([](const auto &session) {
                (void)session.execute(
                    {"DELETE FROM subathon_rate_occurrences WHERE override_ref=X'4F'", {}});
                (void)session.execute(
                    {"DELETE FROM subathon_rate_decisions WHERE decision_ref=X'44'", {}});
            });
            file.reopen();
            const auto before = rate_image(file);
            expect_refusal(recover(file, first_preview()), RateStoreRefusal::invalid_retained_data);
            require(rate_image(file) == before,
                    "An absent operation index cannot certify absence when its successor exists");
        });
    }

    void acceptedObservationShapes_R06_data() {
        QTest::addColumn<int>("shape");
        QTest::newRow("derived-same-value") << 0;
        QTest::newRow("direct-same-value") << 1;
        QTest::newRow("unrelated-only") << 2;
        QTest::newRow("no-accepted-observation") << 3;
    }
    void acceptedObservationShapes_R06() {
        QFETCH(int, shape);
        check([=] {
            RateFile file;
            // Seed the three original occurrences as frozen, independently
            // selected revisions. The supplied witness is outside edit authority.
            auto first = first_preview().proposal.candidate;
            first.overrides = {
                {"O", core::RateOverrideKind::temporary, "USD", "EUR", "1.40", "D", "A", "r", {}}};
            first.components = {{{{"EUR", "1", "1"}, {"USD", "5", "7"}}}};
            auto second = first;
            second.snapshot_ref = "T";
            second.parent_snapshot_ref = "S";
            second.overrides.push_back(
                {"P", core::RateOverrideKind::pinned, "EUR", "GBP", "0.5", "D2", "A", "r", {}});
            second.constraints.push_back(core::ManualRateConstraint{"P"});
            second.components = {{{{"EUR", "1", "1"}, {"GBP", "1", "2"}, {"USD", "5", "7"}}}};
            auto base = second;
            base.snapshot_ref = "U";
            base.parent_snapshot_ref = "T";
            base.overrides.push_back({"Q",
                                      core::RateOverrideKind::temporary,
                                      "JPY",
                                      "CAD",
                                      "0.012",
                                      "D3",
                                      "A",
                                      "r",
                                      {}});
            base.constraints.push_back(core::ManualRateConstraint{"Q"});
            base.components.insert(
                base.components.begin(),
                core::RateComponentInput{{{"CAD", "1", "1"}, {"JPY", "250", "3"}}});
            file.write(
                [=](const auto &session) { seed_expiry_base(session, first, second, base); });
            auto observation_input = unused_g;
            if (shape == 0)
                observation_input.quotes = {{"USD", "CHF", "0.70"}, {"CHF", "EUR", "2.0"}};
            else if (shape == 1)
                observation_input.quotes = {{"USD", "EUR", "1.40"}};
            else
                observation_input.quotes = {{"CAD", "GBP", "2.0"}};
            const auto observation = core::make_fetched_rate_observation(observation_input);
            std::vector<core::RateOverrideFetchedEdge> edges{{observation, 1}};
            if (shape == 0)
                edges.push_back({observation, 2});
            const auto input = core::make_effective_rate_snapshot(base);
            const auto result =
                core::propose_rate_update(input, shape == 3 ? nullptr : &observation, edges, "V");
            if (shape == 3) {
                require(!result && file.rows(head_rows) == SqlRows{{blob("H"), blob("U")}},
                        "No accepted observation produces no proposed successor");
                return;
            }
            require(result && result->overrides.size() == (shape < 2 ? 2u : 3u) &&
                        std::ranges::none_of(result->overrides,
                                             [](const auto &o) { return o.override_ref == "O"; }) ==
                            (shape < 2) &&
                        core::quote_effective_rate(*result, "USD", "GBP").data().rate ==
                            ratio(7, 10),
                    "Supplied witness expires only the relevant temporary, preserving pin and "
                    "other temporary");
            const auto original_occurrences = file.rows(occurrence_rows);
            const auto rob_context =
                QByteArray::fromHex("524f4231 0000000147 0000000170 0000000163 0000000171 "
                                    "0000000165 00 00 00 00 00 00");
            const auto quote_bytes =
                shape == 0
                    ? QByteArray::fromHex("00000002 00000003555344 00000003434846 00000004302e3730 "
                                          "00000003434846 00000003455552 00000003322e30")
                : shape == 1
                    ? QByteArray::fromHex("00000001 00000003555344 00000003455552 00000004312e3430")
                    : QByteArray::fromHex("00000001 00000003434144 00000003474250 00000003322e30");
            file.write([=](const auto &session) {
                (void)session.execute({"INSERT INTO subathon_rate_observations VALUES(?,?,?)",
                                       {blob("G"), rob_format, rob_context + quote_bytes}});
                seed_revision(session, *result);
            });
            file.reopen();
            const auto reopened = std::get<core::EffectiveRateSnapshot>(head(file));
            require(snapshot_bytes(reopened) == snapshot_bytes(*result) &&
                        std::get<core::EffectiveRateSnapshot>(revision(file, "U"))
                                .data()
                                .overrides.size() == 3 &&
                        file.rows(occurrence_rows) == original_occurrences,
                    "Shorter successor round-trips and the expired occurrence remains immutable");
        });
    }

    void survivingOccurrenceIsSelected_R09_data() {
        QTest::addColumn<bool>("missing");
        QTest::addColumn<bool>("retry");
        QTest::newRow("fresh-save-missing-survivor") << true << false;
        QTest::newRow("fresh-save-equal-value-changed-spelling") << false << false;
        QTest::newRow("replay-missing-survivor") << true << true;
        QTest::newRow("replay-equal-value-changed-spelling") << false << true;
    }
    void survivingOccurrenceIsSelected_R09() {
        QFETCH(bool, missing);
        QFETCH(bool, retry);
        check([=] {
            RateFile file;
            seed_first(file);
            seed_survivor(file);
            auto input = surviving_preview();
            if (!retry) {
                input.edit = edit("K3", "T", "U", "P3", "D3");
                input.edit.override_input.from_currency = "USD";
                input.edit.override_input.to_currency = "GBP";
                input.edit.override_input.original_decimal = "0.75";
                input.proposal.candidate.snapshot_ref = "U";
                input.proposal.candidate.parent_snapshot_ref = "T";
                input.proposal.candidate.overrides.push_back(input.edit.override_input);
                input.proposal.candidate.constraints.push_back(core::ManualRateConstraint{"P3"});
                input.proposal.affected_rates.clear();
            }
            file.write([=](const auto &session) {
                if (missing) {
                    (void)session.execute(
                        {"DELETE FROM subathon_rate_occurrences WHERE override_ref=X'4F'", {}});
                } else {
                    const auto changed = QByteArray::fromHex(
                        "000000014f 01 00000003455552 00000003555344 00000005312e323030 "
                        "0000000144 01 0000000141 0000000172 00");
                    (void)session.execute(
                        {"UPDATE subathon_rate_occurrences SET body=? WHERE override_ref=X'4F'",
                         {changed}});
                }
            });
            const auto before = rate_image(file);
            expect_refusal(*save(file, input, all_pairs())->staged,
                           RateStoreRefusal::invalid_retained_data);
            require(rate_image(file) == before,
                    "Mismatch stays unrepaired even when embedded revisions still agree");
        });
    }

    void corruptSelectedClosure_R09_data() {
        QTest::addColumn<QString>("sql");
        QTest::addColumn<QByteArray>("value");
        QTest::addColumn<bool>("unsupported");
        QTest::newRow("unknown-format")
            << QString("UPDATE subathon_rate_observations SET format=? WHERE observation_ref=X'47'")
            << QByteArray("unrecognized") << true;
        auto valid_changed = selected_rob;
        valid_changed.replace("1.30", "1.31");
        QTest::newRow("selected-complete-ROB-disagrees-with-RSV")
            << QString(
                   "UPDATE subathon_rate_observations SET document=? WHERE observation_ref=X'46'")
            << valid_changed << false;
        QTest::newRow("malformed-unused-observation")
            << QString(
                   "UPDATE subathon_rate_observations SET document=? WHERE observation_ref=X'47'")
            << QByteArray("ROB1") << false;
        QTest::newRow("wrong-introducing-revision")
            << QString(
                   "UPDATE subathon_rate_occurrences SET revision_ref=? WHERE override_ref=X'4F'")
            << QByteArray("E") << false;
        QTest::newRow("malformed-decision")
            << QString("UPDATE subathon_rate_decisions SET decision=? WHERE decision_ref=X'44'")
            << QByteArray("RDC1") << false;
        QTest::newRow("noncanonical-potential")
            << QString("UPDATE subathon_rate_revisions SET document=? WHERE snapshot_ref=X'53'")
            << fetched_snapshot_with_unreduced_weight() << false;
        auto ordinal = fetched_intent;
        ordinal[ordinal.size() - 1] = char(0);
        QTest::newRow("zero-basis-ordinal")
            << QString("UPDATE subathon_rate_decisions SET intent=? WHERE decision_ref=X'44'")
            << ordinal << false;
    }
    void corruptSelectedClosure_R09() {
        QFETCH(QString, sql);
        QFETCH(QByteArray, value);
        QFETCH(bool, unsupported);
        check([=] {
            RateFile file;
            seed_first(file, true);
            file.write([=](const auto &session) { (void)session.execute({sql, {value}}); });
            const auto before = rate_image(file);
            expect_refusal(recover(file, fetched_preview()),
                           unsupported ? RateStoreRefusal::unsupported_retained_data
                                       : RateStoreRefusal::invalid_retained_data);
            require(rate_image(file) == before,
                    "Selected closure corruption never repairs or publishes");
        });
    }

    void malformedHead_R09_data() {
        QTest::addColumn<int>("shape");
        QTest::newRow("missing-singleton") << 0;
        QTest::newRow("nonterminal-head") << 1;
        QTest::newRow("legacy-model-duplicate-pair") << 2;
        QTest::newRow("root-with-trailing-bytes") << 3;
        QTest::newRow("well-sized-text-in-BLOB-slot") << 4;
    }
    void malformedHead_R09() {
        QFETCH(int, shape);
        check([=] {
            RateFile file;
            if (shape == 1 || shape == 2)
                seed_first(file);
            file.write([=](const auto &session) {
                if (shape == 0)
                    (void)session.execute({"DELETE FROM subathon_rate_head", {}});
                if (shape == 1)
                    (void)session.execute({"UPDATE subathon_rate_head SET revision_ref=X'45'", {}});
                if (shape == 2) {
                    auto candidate = first_preview().proposal.candidate;
                    auto duplicate = candidate.overrides.front();
                    duplicate.override_ref = "Z";
                    duplicate.decision_ref = "D2";
                    duplicate.from_currency = "USD";
                    duplicate.to_currency = "EUR";
                    // Two different override identities can encode the same
                    // unordered pair in the legacy model. Both exact edges use rate 1.
                    candidate.overrides.front().original_decimal = "1";
                    duplicate.original_decimal = "1";
                    candidate.overrides.push_back(duplicate);
                    candidate.constraints.push_back(core::ManualRateConstraint{"Z"});
                    candidate.components.front().members.back() = {"USD", "1", "1"};
                    (void)session.execute(
                        {"UPDATE subathon_rate_revisions SET document=? WHERE snapshot_ref=X'53'",
                         {snapshot_bytes(candidate)}});
                }
                if (shape == 3)
                    (void)session.execute(
                        {"UPDATE subathon_rate_revisions SET document=? WHERE snapshot_ref=X'45'",
                         {root_bytes + QByteArray(1, '\0')}});
                if (shape == 4) {
                    (void)session.execute({"DROP TABLE subathon_rate_head", {}});
                    (void)session.execute(
                        {"CREATE TABLE subathon_rate_head(slot,revision_ref)", {}});
                    (void)session.execute({"INSERT INTO subathon_rate_head VALUES('H','E')", {}});
                }
            });
            const auto before = rate_image(file);
            expect_refusal(head(file), RateStoreRefusal::invalid_retained_data);
            require(rate_image(file) == before, "Refused head stays untouched");
        });
    }

    void reverseTemporaryAndRoleAudit_R02() {
        check([] {
            RateFile file;
            auto input = first_preview();
            input.edit.override_input.kind = core::RateOverrideKind::temporary;
            input.edit.override_input.from_currency = "USD";
            input.edit.override_input.to_currency = "EUR";
            input.edit.override_input.original_decimal = "0.80";
            input.proposal.candidate.overrides = {input.edit.override_input};
            input.proposal.candidate.components.front().members.back() = {"USD", "5", "4"};
            input.proposal.affected_rates = {change("EUR", "USD", 5, 4),
                                             change("USD", "EUR", 4, 5)};
            auto auth = authorized();
            auto entry = grant();
            entry.origin = core::PermissionOrigin::role;
            entry.role = "L";
            entry.source = "U";
            auth.graph.use.roles = {{"L", {entry}}};
            auth.graph.use.accounts.front().assigned_roles = {"L"};
            auth.graph.use.accounts.front().individual_entries.clear();
            const auto result = enqueue_save(*file.writer, input.edit, input, empty_basis, auth,
                                             false, false, std::nullopt);
            wait_for_save(result);
            finance::normal(*result->completion);
            expect_receipt(std::get<RateReceiptSelection>(*result->staged),
                           RateReceiptOrigin::newly_staged, input);
            const auto audit = QByteArray::fromHex(
                "52444331 0000000149 000000014b 0000000144 0000000145 0000000153 000000014f "
                "00 0000000141 000000014e 0000000152 0000000150 "
                "000000137065726d697373696f6e2d72617465732d7631 0000000000000001 "
                "01 0000000155 0000000147 000000014c 00");
            require(file.rows(decision_rows).front().back().toByteArray() == audit &&
                        result->samples == 1,
                    "The unordered pair grants reverse Temporary and stores the actual role "
                    "witness with absent time");
        });
    }

    void opaqueMaximumOccurrence_data() {
        QTest::addColumn<int>("over");
        QTest::newRow("exact-5535-body-and-272-pair-scope") << 0;
        QTest::newRow("513-byte-decimal") << 1;
        QTest::newRow("4097-byte-reason") << 2;
        QTest::newRow("257-byte-operation-key") << 3;
        QTest::newRow("4097-byte-installation-ref") << 4;
    }
    void opaqueMaximumOccurrence() {
        QFETCH(int, over);
        check([=] {
            RateFile file;
            auto preview = first_preview();
            auto &input = preview.edit.override_input;
            auto auth = authorized();
            const std::string account = std::string("A\0\xff", 3) + std::string(253, 'x');
            const std::string installation =
                std::string("I\0\x80", 3) + std::string(4093 + (over == 4), 'i');
            const std::string low = std::string("A\0", 2) + std::string(62, 'a');
            const std::string high = std::string("Z\xff", 2) + std::string(62, 'z');
            auth.session.account_ref = auth.graph.current_accounts.front().current.account_ref =
                account;
            auth.graph.owner.account = account;
            auth.graph.use.accounts.front().account = auth.graph.grant.accounts.front().account =
                account;
            auth.session.installation_ref = auth.graph.installation = installation;
            auth.graph.current_installation.current.installation_ref = installation;
            auth.graph.current_accounts.front().current.installation_ref = installation;
            const auto scope = QByteArray::fromHex("00000040") + blob(low) +
                               QByteArray::fromHex("00000040") + blob(high);
            require(scope.toHex().size() == 272,
                    "Two opaque 64-byte currencies yield the maximal pair scope");
            auth.graph.use.accounts.front().individual_entries = {
                grant({scope.toHex().toStdString()})};
            preview.edit.installation = installation;
            preview.edit.operation = std::string(256 + (over == 3), 'k');
            input.actor_ref = account;
            input.override_ref = std::string("O\0\x80", 3) + std::string(253, 'o');
            input.decision_ref = std::string(256, 'd');
            input.from_currency = low;
            input.to_currency = high;
            input.original_decimal = "1e" + std::string(510 + (over == 1), '0');
            input.reason = std::string(4096 + (over == 2), 'r');
            auto &candidate = preview.proposal.candidate;
            candidate.overrides = {input};
            candidate.constraints = {core::ManualRateConstraint{input.override_ref}};
            candidate.components = {{{{low, "1", "1"}, {high, "1", "1"}}}};
            preview.proposal.affected_rates = {change(low, high, 1, 1), change(high, low, 1, 1)};
            const auto before = rate_image(file);
            const auto result = save(file, preview, auth);
            if (over) {
                expect_refusal(*result->staged, RateStoreRefusal::resource_limit);
                require(rate_image(file) == before, "One-over input never writes");
                return;
            }
            expect_receipt(std::get<RateReceiptSelection>(*result->staged),
                           RateReceiptOrigin::newly_staged, preview);
            const auto expected =
                QByteArray::fromHex("00000100") + blob(input.override_ref) +
                QByteArray::fromHex("0100000040") + blob(low) + QByteArray::fromHex("00000040") +
                blob(high) + QByteArray::fromHex("00000200") + blob(input.original_decimal) +
                QByteArray::fromHex("00000100") + blob(input.decision_ref) +
                QByteArray::fromHex("0100000100") + blob(account) +
                QByteArray::fromHex("00001000") + blob(input.reason) + QByteArray(1, '\0');
            require(
                expected.size() == 5535 &&
                    file.rows(occurrence_rows) ==
                        SqlRows{{blob(input.override_ref), blob(input.decision_ref), blob("S"),
                                 expected}},
                "Maximum canonical body preserves opaque BLOB identities and original spelling");
            file.reopen();
            expect_receipt(std::get<RateReceiptSelection>(recover(file, preview, auth)),
                           RateReceiptOrigin::retained, preview);
        });
    }

    void basisBounds_data() {
        QTest::addColumn<int>("shape");
        QTest::newRow("16384-edges-preserve-unused-multiplicity") << 0;
        QTest::newRow("16385-edges") << 1;
        QTest::newRow("unused-edge-index") << 2;
        QTest::newRow("unused-edge-ordinal") << 3;
        QTest::newRow("16-admitted-observations") << 4;
        QTest::newRow("17-observations") << 5;
        QTest::newRow("duplicate-observation-key") << 6;
    }
    void basisBounds() {
        QFETCH(int, shape);
        check([=] {
            RateFile file;
            auto input = first_preview();
            input.basis.observations = {selected_f()};
            // Every repeated edge is incompatible with the pin and never enters
            // the candidate. The exact original trace is still retained.
            input.basis.edges = {{0, 2}};
            if (shape <= 1)
                input.basis.edges.resize(16384 + unsigned(shape), RateSourceEdge{0, 2});
            else if (shape == 2)
                input.basis.edges.back().observation_index = 1;
            else if (shape == 3)
                input.basis.edges.back().quote_ordinal = 3;
            else if (shape <= 5) {
                for (unsigned i = 0; i < (shape == 4 ? 15u : 16u); ++i) {
                    auto unused = unused_g;
                    unused.context.observation_ref = hex_token('g', i);
                    input.basis.observations.push_back(std::move(unused));
                }
            } else {
                input.basis.observations.push_back(input.basis.observations.front());
            }
            const auto old = rate_image(file);
            const auto accepted = save(file, input);
            if (shape == 0 || shape == 4) {
                expect_receipt(std::get<RateReceiptSelection>(*accepted->staged),
                               RateReceiptOrigin::newly_staged, input);
                require(file.rows(observation_rows).size() == (shape == 0 ? 1 : 16),
                        "All admitted observations survive even when every edge is excluded");
                if (shape == 0) {
                    const auto prefix =
                        first_intent.first(75) + QByteArray::fromHex("00000001000000014600004000");
                    const auto edge = QByteArray::fromHex("000000000000000000000002");
                    const auto stored = file.rows(decision_rows).front()[6].toByteArray();
                    require(stored.size() == 196696 && stored == prefix + edge.repeated(16384),
                            "RIN1 records 16,384 exact repeated 12-byte edges after its 88-byte "
                            "prefix");
                }
            } else {
                expect_refusal(*accepted->staged, shape == 1 || shape == 5
                                                      ? RateStoreRefusal::resource_limit
                                                      : RateStoreRefusal::invalid_proposal);
                require(rate_image(file) == old, "Bounds and edge validity guard the entire basis");
            }
        });
    }

    void observationPressure_data() {
        QTest::addColumn<bool>("parsed");
        QTest::addColumn<bool>("over");
        QTest::newRow("exact-encoded-32768") << false << false;
        QTest::newRow("encoded-32769") << false << true;
        QTest::newRow("exact-parsed-24576") << true << false;
        QTest::newRow("parsed-24577") << true << true;
    }
    void observationPressure() {
        QFETCH(bool, parsed);
        QFETCH(bool, over);
        check([=] {
            RateFile file;
            auto [observation, expected] = standalone_pressure(parsed, over);
            require(expected.size() == (parsed ? 14812 : 32768) + int(over),
                    "Independent ROB1 framed byte count");
            auto preview = first_preview();
            preview.basis.observations.push_back(std::move(observation));
            const auto before = rate_image(file);
            const auto result = save(file, preview);
            if (over) {
                expect_refusal(*result->staged, RateStoreRefusal::resource_limit);
                require(rate_image(file) == before,
                        "Oversized unused basis row refuses the entire operation");
            } else {
                expect_receipt(std::get<RateReceiptSelection>(*result->staged),
                               RateReceiptOrigin::newly_staged, preview);
                require(file.rows(observation_rows) == SqlRows{{blob("J"), rob_format, expected}},
                        "At-bound unused ROB1 is retained byte-for-byte");
                file.reopen();
                expect_receipt(std::get<RateReceiptSelection>(recover(file, preview)),
                               RateReceiptOrigin::retained, preview);
            }
        });
    }

    void frozenSnapshotPressureControls() {
        check([] {
            const auto expect_bridge_limit = [](const auto &input) {
                try {
                    (void)snapshot_bytes(input);
                } catch (const core::ExactValuationFailure &error) {
                    require(error.code() == core::ExactValuationError::resource_limit,
                            "Bridge resource refusal");
                    return;
                }
                require(false, "Expected shared codec to refuse frozen pressure shape");
            };
            require(snapshot_bytes(encoded_pressure(202, 1)).size() == 32766 &&
                        snapshot_bytes(encoded_pressure(202, 3)).size() == 32768 &&
                        snapshot_bytes(parsed_pressure(213, 69)).size() == 22526,
                    "Independent exact encoded and parsed bridge endpoints");
            expect_bridge_limit(encoded_pressure(210, 1));
            expect_bridge_limit(encoded_pressure(202, 4));
            expect_bridge_limit(parsed_pressure(220, 0));
            expect_bridge_limit(parsed_pressure(213, 70));
        });
    }

    void selectedClosureSharesWriterResultBudget_R09() {
        check([] {
            RateFile file;
            seed_first(file);
            require(std::holds_alternative<RateReceiptSelection>(recover(file, first_preview())),
                    "Default-budget selected history is readable");
            const auto before = rate_image(file);
            StorageLimits limits;
            // Each document fits 256. The five selected documents alone need
            // 27 + 121 + 83 + 106 + 45 = 382 before SQL keys and preflight metadata.
            limits.result_value_bytes = 256;
            file.reopen(limits);
            bool refused_by_preflight = false;
            const auto outcome = qt_test_support::run_work(
                *file.writer,
                [&](auto context) -> QVariant {
                    const auto session = context.begin();
                    Authority authority;
                    const auto preview = first_preview();
                    const auto result = SqliteRateOverrideUnit(session).recover(
                        authority, literal_rate_trust(), preview.edit, preview);
                    expect_refusal(result, RateStoreRefusal::resource_limit);
                    refused_by_preflight = true;
                    session.commit();
                    return 1;
                },
                10000);
            if (refused_by_preflight)
                finance::normal(outcome);
            else
                require(
                    outcome.transaction == TransactionOutcome::rolled_back &&
                        outcome.error == WorkError::result_limit && !outcome.value,
                    "A reached cumulative result limit is the original writer error with rollback");
            require(rate_image(file) == before,
                    "No partial selected receipt or writes after budget refusal");
        });
    }

  private:
    static QByteArray fetched_snapshot_with_unreduced_weight() {
        auto bytes = snapshot_bytes(fetched_preview().proposal.candidate);
        const auto reduced = QByteArray::fromHex("0000000355534400000001360000000135");
        require(bytes.endsWith(reduced), "Independent final RSV1 member is USD 6/5");
        bytes.chop(reduced.size());
        return bytes + QByteArray::fromHex("00000003555344000000023132000000023130");
    }

    static void seed_expiry_base(const SqliteWorkSession &session,
                                 const core::EffectiveRateSnapshotInput &first,
                                 const core::EffectiveRateSnapshotInput &second,
                                 const core::EffectiveRateSnapshotInput &third) {
        // Audit rows use independently enumerated edit bodies. No provider consent
        // or user-facing expiry command is fabricated by this stored-shape fixture.
        const std::array revisions{first, second, third};
        const std::array keys{"K", "L", "M"};
        const std::array bodies{
            QByteArray::fromHex("000000014f 00 00000003555344 00000003455552 00000004312e3430 "
                                "0000000144 01 0000000141 0000000172 00"),
            QByteArray::fromHex("0000000150 01 00000003455552 00000003474250 00000003302e35 "
                                "000000024432 01 0000000141 0000000172 00"),
            QByteArray::fromHex("0000000151 00 000000034a5059 00000003434144 00000005302e303132 "
                                "000000024433 01 0000000141 0000000172 00")};
        const auto field = [](const QByteArray &b) {
            require(b.size() <= 255, "Small fixture field length");
            return QByteArray(3, '\0') + char(b.size()) + b;
        };
        for (std::size_t i = 0; i < revisions.size(); ++i) {
            const auto &snapshot = revisions[i];
            const auto &introduced = snapshot.overrides.back();
            const auto decision = blob(introduced.decision_ref);
            const auto occurrence = blob(introduced.override_ref);
            const auto base = blob(*snapshot.parent_snapshot_ref);
            const auto successor = blob(snapshot.snapshot_ref);
            const auto intent = QByteArray("RIN1") + field("I") + field(keys[i]) + field(base) +
                                field(successor) + bodies[i] +
                                QByteArray::fromHex("00 0000000142 00000000 00000000");
            const auto audit = QByteArray("RDC1") + field("I") + field(keys[i]) + field(decision) +
                               field(base) + field(successor) + field(occurrence) +
                               first_decision.mid(34);
            seed_revision(session, snapshot);
            (void)session.execute({"INSERT INTO subathon_rate_decisions VALUES(?,?,?,?,?,?,?,?,?)",
                                   {decision, blob("I"), blob(keys[i]), base, successor, rin_format,
                                    intent, rdc_format, audit}});
            (void)session.execute({"INSERT INTO subathon_rate_occurrences VALUES(?,?,?,?)",
                                   {occurrence, decision, successor, bodies[i]}});
        }
    }
};
} // namespace

QTEST_GUILESS_MAIN(SqliteRateOverrideWriterTests)
#include "sqlite_rate_override_writer_tests.moc"
