// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "oracle.hpp"

#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace removal_test {
using RateImage = decltype(rate_image(std::declval<const RateFile &>()));

inline void expect_snapshot(const RateRevisionResult &result,
                            const core::EffectiveRateSnapshotInput &expected) {
    const auto *snapshot = std::get_if<core::EffectiveRateSnapshot>(&result);
    require(snapshot && snapshot_bytes(*snapshot) == snapshot_bytes(expected),
            "Verified revision matches literal provenance, evidence, and weights");
}

inline RateRevisionResult head(RateFile &file) {
    return selected<RateRevisionResult>(file, [](const auto &unit) { return unit.head(); });
}

inline RateRevisionResult revision(RateFile &file, std::string ref) {
    return selected<RateRevisionResult>(file, [=](const auto &unit) { return unit.revision(ref); });
}

inline RateEditPreview introduce(RateFile &file, const RateOverrideEdit &input,
                                 const RateSourceBasis &basis) {
    const auto result = selected<RatePreviewResult>(file, [=](const auto &unit) {
        Authority source(permitted());
        return unit.preview(source, literal_rate_trust(), input, basis);
    });
    const auto *preview = std::get_if<RateEditPreview>(&result);
    require(preview != nullptr, "SETUP: shipped introduction preview succeeds");
    const auto capture = rate_fixture::save(file, *preview, permitted());
    const auto *staged = std::get_if<RateReceiptSelection>(&*capture->staged);
    require(staged != nullptr, "SETUP: shipped introduction stages a receipt");
    const auto finished = finish_rate_receipt(*staged, *capture->completion);
    require(finished && capture->samples == 1, "SETUP: shipped introduction commits normally");
    expect_receipt(*finished, RateReceiptOrigin::newly_staged, *preview);
    return *preview;
}

inline RateImage establish(RateFile &file, const Example &contract, Start start,
                           std::optional<RateEditPreview> *first_preview = nullptr) {
    const auto root = rate_image(file);
    require(root[0].size() == 1 && root[1].isEmpty() && root[2].isEmpty() && root[3].isEmpty() &&
                root[4] == SqlRows{{blob("H"), blob("E")}},
            "SETUP: v17 has only committed root E");
    expect_reserved_root(root[0].front());
    expect_snapshot(head(file), {"E", {}, {}, {}, {}, {}, {}});
    for (const auto &input : contract.introductions) {
        auto introduced = introduce(file, input, contract.basis);
        if (first_preview && !*first_preview)
            first_preview->emplace(std::move(introduced));
    }
    expect_snapshot(head(file), contract.before);
    expect_snapshot(revision(file, contract.before.snapshot_ref), contract.before);
    const auto before = rate_image(file);
    const auto edits = qsizetype(contract.introductions.size());
    require(before[0].size() == 1 + edits && before[2].size() == edits &&
                before[3].size() == edits &&
                before[4] == SqlRows{{blob("H"), blob(contract.before.snapshot_ref)}},
            "SETUP: exact independently observed legacy revision/decision/occurrence counts");
    require(before[0].front() == root[0].front(), "SETUP: original E row stays immutable");
    require(before[3].front().size() == 4 && before[3].front()[0] == blob("O") &&
                before[3].front()[1] == blob("D") && before[3].front()[2] == blob("S"),
            "SETUP: retained target occurrence O was introduced by D in S");
    SqlRows expected_sources;
    if (start == Start::equal_rates)
        expected_sources = {{blob("F"), rob_format, equal_f_rob}};
    else if (start != Start::temporary)
        expected_sources = {{blob("F"), rob_format, selected_rob},
                            {blob("G"), rob_format, unused_rob},
                            {blob("W"), rob_format, w_rob}};
    require(before[1] == expected_sources,
            "SETUP: committed exact ROB1 rows, including unused facts");
    return before;
}

inline RateRemovalPreviewResult preview(RateFile &file, const RateOverrideRemovalEdit &edit,
                                        const RateSourceBasis &basis,
                                        RateAuthorityResolution authority = permitted()) {
    return selected<RateRemovalPreviewResult>(file, [=](const auto &unit) {
        Authority source(authority);
        return std::invoke(preview_port, unit, source, literal_rate_trust(), edit, basis);
    });
}

inline RateRemovalRecoveryResult recover(RateFile &file, const RateRemovalPreview &submitted,
                                         RateAuthorityResolution authority = permitted()) {
    return selected<RateRemovalRecoveryResult>(file, [=](const auto &unit) {
        Authority source(authority);
        return std::invoke(recover_port, unit, source, literal_rate_trust(), submitted.edit,
                           submitted);
    });
}

inline auto context_fields(const core::RateObservationContext &ctx) {
    return std::tie(ctx.observation_ref, ctx.provider_ref, ctx.configuration_ref, ctx.request_ref,
                    ctx.source_evidence_ref, ctx.provider_snapshot_ref, ctx.fetched_at_utc_ms,
                    ctx.source_rate_time.original_text, ctx.source_rate_time.utc_ms,
                    ctx.publication_time.original_text, ctx.publication_time.utc_ms);
}

inline void expect_basis(const RateSourceBasis &actual, const RateSourceBasis &expected) {
    require(actual.policy_ref == expected.policy_ref &&
                actual.observations.size() == expected.observations.size() &&
                actual.edges.size() == expected.edges.size(),
            "Exact preview policy, observation count and trace count");
    for (std::size_t i = 0; i < expected.observations.size(); ++i) {
        const auto &a = actual.observations[i];
        const auto &e = expected.observations[i];
        require(context_fields(a.context) == context_fields(e.context) &&
                    a.quotes.size() == e.quotes.size(),
                "All preview observation metadata is preserved in supplied order");
        for (std::size_t j = 0; j < e.quotes.size(); ++j) {
            const auto fields = [](const auto &q) {
                return std::tie(q.from_currency, q.to_currency, q.original_decimal);
            };
            require(fields(a.quotes[j]) == fields(e.quotes[j]),
                    "Complete original quote at ordinal");
        }
    }
    for (std::size_t i = 0; i < expected.edges.size(); ++i)
        require(actual.edges[i].observation_index == expected.edges[i].observation_index &&
                    actual.edges[i].quote_ordinal == expected.edges[i].quote_ordinal,
                "Preview preserves ordered observation-index/original-ordinal trace");
}

inline auto edit_fields(const RateOverrideRemovalEdit &e) {
    return std::tie(e.installation, e.operation, e.actor, e.decision, e.base, e.successor,
                    e.pair.low_currency, e.pair.high_currency, e.removed_override_ref);
}

inline void expect_proposal(const core::RateOverrideRemovalProposal &actual,
                            const core::RateOverrideRemovalProposal &expected) {
    require(actual.removed_override_ref == expected.removed_override_ref,
            "Preview binds the exact removed occurrence");
    require(snapshot_bytes(actual.candidate) == snapshot_bytes(expected.candidate),
            "Candidate has exact normalized weights, parent and complete selected evidence");
    expect_changes(actual.affected_rates, expected.affected_rates);
}

inline RateRemovalPreview preview_exact(RateFile &file, const RateRemovalPreview &expected,
                                        RateAuthorityResolution authority = permitted()) {
    const auto result = preview(file, expected.edit, expected.basis, std::move(authority));
    const auto *actual = std::get_if<RateRemovalPreview>(&result);
    require(actual != nullptr, "Removal preview returns an owned proposal for the active target");
    require(edit_fields(actual->edit) == edit_fields(expected.edit),
            "Preview copies complete edit");
    expect_basis(actual->basis, expected.basis);
    expect_proposal(actual->proposal, expected.proposal);
    return *actual;
}

inline void expect_quote(const core::EffectiveRateSnapshotInput &snapshot, std::string_view from,
                         std::string_view to, std::optional<core::Exact> rate) {
    const auto quote = core::quote_effective_rate(snapshot, from, to);
    const auto &data = quote.data();
    const auto kind = from == to ? core::RatePairKind::identity
                      : rate     ? core::RatePairKind::available
                                 : core::RatePairKind::missing;
    require(data.kind == kind && data.rate == rate, "Quote has exact availability and ratio");
    if (kind == core::RatePairKind::available) {
        require(data.provenance && data.provenance->snapshot_ref == snapshot.snapshot_ref &&
                    data.provenance->from_currency == from && data.provenance->to_currency == to,
                "Available quote binds revision and directed endpoints");
    } else {
        require(!data.provenance, "Missing and identity quotes carry no provenance");
    }
}

inline void expect_quotes(const Example &example, const core::EffectiveRateSnapshotInput &candidate,
                          Start start) {
    for (const auto &change : example.changes) {
        expect_quote(example.before, change.from_currency, change.to_currency, change.before);
        expect_quote(candidate, change.from_currency, change.to_currency, change.after);
    }
    for (const auto *snapshot : {&example.before, &candidate}) {
        expect_quote(*snapshot, "USD", "USD", ratio(1));
        if (start == Start::equal_rates) {
            expect_quote(*snapshot, "EUR", "USD", ratio(6, 5));
            expect_quote(*snapshot, "USD", "EUR", ratio(5, 6));
            expect_quote(*snapshot, "EUR", "GBP", {});
            expect_quote(*snapshot, "GBP", "EUR", {});
        } else if (start == Start::temporary) {
            expect_quote(*snapshot, "EUR", "GBP", ratio(1, 2));
            expect_quote(*snapshot, "GBP", "EUR", ratio(2));
        } else {
            expect_quote(*snapshot, "EUR", "GBP", ratio(9, 10));
            expect_quote(*snapshot, "GBP", "EUR", ratio(10, 9));
            expect_quote(*snapshot, "CAD", "JPY", ratio(3));
            expect_quote(*snapshot, "JPY", "CAD", ratio(1, 3));
            expect_quote(*snapshot, "CAD", "USD", {});
            expect_quote(*snapshot, "USD", "CAD", {});
        }
    }
}

struct RemovalCapture {
    std::optional<RateRemovalSaveResult> staged;
    std::optional<SqliteWorkResult> completion;
    int samples = 0;
    int authority_reads = 0;
};

inline auto enqueue_removal(RateFile &file, const RateOverrideRemovalEdit &edit,
                            const RateRemovalPreview &preview, const RateSourceBasis &current,
                            RateAuthorityResolution authority = permitted(),
                            std::optional<std::int64_t> acceptance_time = 0,
                            bool throw_after_commit = false) {
    const auto capture = std::make_shared<RemovalCapture>();
    require(
        file.writer->enqueue_work(
            [=](const auto &context) -> QVariant {
                const auto session = context.begin();
                Authority source(authority);
                const std::function<std::optional<std::int64_t>()> sample = [&] {
                    ++capture->samples;
                    return acceptance_time;
                };
                capture->staged = std::invoke(save_port, SqliteRateOverrideUnit(session), source,
                                              literal_rate_trust(), edit, preview, current, sample);
                capture->authority_reads = source.reads;
                session.commit();
                if (throw_after_commit)
                    throw std::runtime_error("Synthetic body failure after acknowledged COMMIT");
                return 1;
            },
            [capture](auto result) { capture->completion = std::move(result); }) ==
            StorageError::none,
        "Writer admits a removal work item");
    return capture;
}

inline void await_removal(const std::shared_ptr<RemovalCapture> &capture) {
    require(QTest::qWaitFor([capture] { return capture->completion.has_value(); }, 10000),
            "Writer delivers the removal work outcome");
}

inline std::optional<RateRemovalReceiptSelection> completed_receipt(const RemovalCapture &capture) {
    std::optional<RateRemovalReceiptSelection> provisional;
    if (capture.staged)
        if (const auto *receipt = std::get_if<RateRemovalReceiptSelection>(&*capture.staged))
            provisional = *receipt;
    return finish_rate_removal_receipt(std::move(provisional), *capture.completion);
}

inline void expect_receipt(const RateRemovalReceiptSelection &actual, RateReceiptOrigin origin,
                           const RateRemovalPreview &expected) {
    const auto &result = actual.receipt;
    const auto &edit = expected.edit;
    require(actual.origin == origin && result.decision == edit.decision &&
                result.base == edit.base && result.successor == edit.successor &&
                result.pair.low_currency == edit.pair.low_currency &&
                result.pair.high_currency == edit.pair.high_currency &&
                result.removed_override_ref == edit.removed_override_ref,
            "Receipt binds exact removal, revision links and selected origin");
    expect_changes(result.changes, expected.proposal.affected_rates);
}

inline void expect_saved(const RemovalCapture &capture, RateReceiptOrigin origin,
                         const RateRemovalPreview &expected) {
    finance::normal(*capture.completion);
    require(capture.staged && std::holds_alternative<RateRemovalReceiptSelection>(*capture.staged),
            "Removal save stages a typed receipt for the exact active target or committed retry");
    const auto receipt = completed_receipt(capture);
    require(receipt.has_value(), "Normal acknowledged COMMIT releases the removal receipt");
    expect_receipt(*receipt, origin, expected);
    require(capture.authority_reads == 1 &&
                capture.samples == (origin == RateReceiptOrigin::newly_staged),
            "Resolve current authority once and sample only the new acceptance");
}

inline void expect_save_refusal(RateFile &file, const RateOverrideRemovalEdit &edit,
                                const RateRemovalPreview &submitted, const RateSourceBasis &current,
                                RateStoreRefusal why,
                                RateAuthorityResolution authority = permitted()) {
    const auto before = rate_image(file);
    const auto capture = enqueue_removal(file, edit, submitted, current, std::move(authority));
    await_removal(capture);
    finance::normal(*capture->completion);
    require(capture->staged.has_value(), "Save returns a typed refusal");
    expect_refusal(*capture->staged, why);
    require(capture->authority_reads == 1 && capture->samples == 0 &&
                !completed_receipt(*capture) && rate_image(file) == before,
            "Refusal authorizes once and leaves no sample, receipt, or durable changes");
}

inline auto submit(RateFile &file, const RateRemovalPreview &preview,
                   RateAuthorityResolution authority = permitted(),
                   std::optional<std::int64_t> time = 0) {
    const auto capture =
        enqueue_removal(file, preview.edit, preview, preview.basis, std::move(authority), time);
    await_removal(capture);
    return capture;
}

inline void expect_recovered(RateFile &file, const RateRemovalPreview &expected) {
    const auto result = recover(file, expected);
    const auto *receipt = std::get_if<RateRemovalReceiptSelection>(&result);
    require(receipt != nullptr, "Recovery verifies the exact committed removal closure");
    expect_receipt(*receipt, RateReceiptOrigin::retained, expected);
}

inline void expect_transition(const RateFile &file, RateImage before, const Example &contract,
                              Start start, bool known_time = true, bool new_source = false,
                              bool role_witness = false) {
    const QByteArray intent_format = "subathon.internal.rate-override-removal-intent.v1";
    const QByteArray decision_format = "subathon.internal.rate-override-removal-decision.v1";
    const auto observed = rate_image(file);
    auto prior = observed;
    const auto remove_append = [](SqlRows &rows, const SqlRow &row) {
        for (const auto &value : row)
            require(value.metaType().id() == QMetaType::QByteArray, "Literal row has BLOB columns");
        const auto index = rows.indexOf(row);
        require(index >= 0, "Exact appended row is present");
        for (const auto &value : rows[index])
            require(value.metaType().id() == QMetaType::QByteArray, "Appended row stores BLOBs");
        rows.removeAt(index);
    };
    const auto base = blob(contract.before.snapshot_ref);
    remove_append(prior[0], {blob("U"), base, rsv_format, snapshot_bytes(contract.after)});
    remove_append(prior[2], {blob("M"), blob("I"), blob("Z"), base, blob("U"), intent_format,
                             expected_intent(start, new_source), decision_format,
                             expected_audit(start, known_time, role_witness)});
    if (new_source)
        remove_append(prior[1], {blob("X"), rob_format, x_rob});
    require(observed[4] == SqlRows{{blob("H"), blob("U")}}, "Exactly one active head moves to U");
    before[4] = observed[4];
    require(prior == before,
            "All preexisting rows are unchanged, including introducing decisions and occurrences");
}
} // namespace removal_test
