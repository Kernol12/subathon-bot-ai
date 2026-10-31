// SPDX-License-Identifier: GPL-3.0-or-later
// Specification v0.45 §§3.2–4,14,18; receipt-intake-v1.md revision 1.
// All identifiers/namespaces are synthetic. Expected decisions come from the contract.
#include "../fakes/receipt_repository.hpp"
#include <array>
#include <functional>
#include <gtest/gtest.h>
#include <string>
#include <subathon/core/receipt.hpp>
#include <subathon/server/receipt_intake.hpp>
#include <utility>
#include <vector>

namespace {
using namespace subathon::core;
using subathon::server::ReceiptIntake;
using subathon::server::ReceiptStorageError;
using subathon::tests::FakeReceiptRepository;

ReceiptInput synthetic_input() {
    return {{{"synthetic.provider-a", "source-account-7"}, "synthetic.event.v1", "event-42"},
            {"synthetic.payment.received", 1, 1'700'000'000'000, "actor-3", "Synthetic Supporter",
             ReceiptMoney{"12.3400", "EUR"},
             ReceiptLinkReference{"synthetic.payment.v1", "payment-9"},
             ReceiptLinkReference{"synthetic.activity.v1", "activity-2"}},
            {1'700'000'000'100, "connection-1", "configuration-r5", "modifier-context-r8"}};
}

class ReceiptIntakeAcceptance : public ::testing::Test {
  protected:
    FakeReceiptRepository repository;
    ReceiptIntake intake{repository};
};

// §3.2–4: first arrival stores owned source facts separately from server context.
TEST_F(ReceiptIntakeAcceptance, FirstArrivalCommitsIdentityFactsContextAndInitialAttempt) {
    auto input = synthetic_input();
    const auto result = intake.ingest(input);
    input.identity.opaque_id = "caller-buffer-reused";
    input.facts.money->original_decimal = "999";
    input.arrival.configuration_ref = "caller-context-reused";

    ASSERT_EQ(repository.records().size(), 1U);
    const auto &record = repository.records().front();
    EXPECT_EQ(result.disposition, ReceiptDisposition::created);
    EXPECT_EQ(result.receipt_id, record.id);
    EXPECT_EQ(record.identity.origin.provider, "synthetic.provider-a");
    EXPECT_EQ(record.identity.origin.source_account, "source-account-7");
    EXPECT_EQ(record.identity.identifier_namespace, "synthetic.event.v1");
    EXPECT_EQ(record.identity.opaque_id, "event-42");
    EXPECT_EQ(record.original_facts.event_type, "synthetic.payment.received");
    EXPECT_EQ(record.original_facts.schema_version, 1U);
    EXPECT_EQ(record.original_facts.source_timestamp_utc_ms, 1'700'000'000'000);
    EXPECT_EQ(record.original_facts.actor_id, "actor-3");
    EXPECT_EQ(record.original_facts.actor_display_name, "Synthetic Supporter");
    ASSERT_TRUE(record.original_facts.money);
    EXPECT_EQ(record.original_facts.money->original_decimal, "12.3400");
    EXPECT_EQ(record.original_facts.money->currency, "EUR");
    EXPECT_EQ(record.original_arrival.server_timestamp_utc_ms, 1'700'000'000'100);
    EXPECT_EQ(record.original_arrival.connection_ref, "connection-1");
    EXPECT_EQ(record.original_arrival.configuration_ref, "configuration-r5");
    EXPECT_EQ(record.original_arrival.modifier_context_ref, "modifier-context-r8");
    ASSERT_EQ(record.attempts.size(), 1U);
    EXPECT_EQ(record.attempts[0].disposition, ReceiptDisposition::created);
    EXPECT_EQ(record.attempts[0].observed_facts, record.original_facts);
    EXPECT_EQ(record.attempts[0].arrival, record.original_arrival);
}

// §18 "Same input delivered twice": one receipt, with both attempts traceable.
TEST_F(ReceiptIntakeAcceptance, ExactRetryReturnsExistingReceiptAndAppendsItsOwnArrival) {
    auto input = synthetic_input();
    const auto first = intake.ingest(input);
    input.arrival.server_timestamp_utc_ms = 1'700'000'003'000;
    const auto second = intake.ingest(input);

    EXPECT_EQ(first.disposition, ReceiptDisposition::created);
    EXPECT_EQ(second.disposition, ReceiptDisposition::retry);
    EXPECT_EQ(second.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 1U);
    const auto &record = repository.records().front();
    ASSERT_EQ(record.attempts.size(), 2U);
    EXPECT_EQ(record.original_arrival.server_timestamp_utc_ms, 1'700'000'000'100);
    EXPECT_EQ(record.attempts[0].arrival.server_timestamp_utc_ms, 1'700'000'000'100);
    EXPECT_EQ(record.attempts[1].arrival.server_timestamp_utc_ms, 1'700'000'003'000);
    EXPECT_EQ(record.attempts[1].disposition, ReceiptDisposition::retry);
    EXPECT_EQ(record.attempts[1].observed_facts, record.original_facts);
}

// Reviewed event-state revision 1: reconnect cannot erase source-scoped deduplication.
TEST_F(ReceiptIntakeAcceptance, ReconnectRetainsOriginalContextAndCapturesNewAttemptContext) {
    auto input = synthetic_input();
    const auto first = intake.ingest(input);
    input.arrival = {1'700'000'060'000, "connection-2", "configuration-r6", "modifier-context-r9"};
    const auto retry = intake.ingest(input);

    EXPECT_EQ(retry.disposition, ReceiptDisposition::retry);
    EXPECT_EQ(retry.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 1U);
    const auto &record = repository.records().front();
    EXPECT_EQ(record.original_arrival.connection_ref, "connection-1");
    EXPECT_EQ(record.original_arrival.configuration_ref, "configuration-r5");
    EXPECT_EQ(record.original_arrival.modifier_context_ref, "modifier-context-r8");
    EXPECT_EQ(record.original_facts.source_timestamp_utc_ms, 1'700'000'000'000);
    ASSERT_EQ(record.attempts.size(), 2U);
    EXPECT_EQ(record.attempts[1].arrival.connection_ref, "connection-2");
    EXPECT_EQ(record.attempts[1].arrival.configuration_ref, "configuration-r6");
    EXPECT_EQ(record.attempts[1].arrival.modifier_context_ref, "modifier-context-r9");
    EXPECT_EQ(record.attempts[1].arrival.server_timestamp_utc_ms, 1'700'000'060'000);
}

// §3.2: each part of the documented source identity scopes an otherwise equal ID.
TEST_F(ReceiptIntakeAcceptance, ProviderAccountNamespaceAndOpaqueIdEachScopeIdentity) {
    const auto original = synthetic_input();
    auto provider = original;
    provider.identity.origin.provider = "synthetic.provider-b";
    auto account = original;
    account.identity.origin.source_account = "source-account-8";
    auto name_space = original;
    name_space.identity.identifier_namespace = "synthetic.delivery.v1";
    auto opaque_id = original;
    opaque_id.identity.opaque_id = "EVENT-42";

    std::vector<ReceiptId> ids;
    for (const auto &input : {original, provider, account, name_space, opaque_id}) {
        SCOPED_TRACE(input.identity.origin.provider + "/" + input.identity.origin.source_account +
                     "/" + input.identity.identifier_namespace + "/" + input.identity.opaque_id);
        const auto result = intake.ingest(input);
        EXPECT_EQ(result.disposition, ReceiptDisposition::created);
        for (const auto id : ids) {
            EXPECT_NE(result.receipt_id, id);
        }
        ids.push_back(result.receipt_id);
    }
    ASSERT_EQ(repository.records().size(), 5U);
    for (const auto &record : repository.records()) {
        EXPECT_EQ(record.attempts.size(), 1U);
    }
}

// §3.2: equal names, exact amounts and source/arrival timestamps are not correlation.
TEST_F(ReceiptIntakeAcceptance, EqualNamesAmountsAndTimestampsDoNotMergeDistinctIds) {
    auto input = synthetic_input();
    const auto first = intake.ingest(input);
    input.identity.opaque_id = "event-43";
    const auto second = intake.ingest(input);

    EXPECT_EQ(second.disposition, ReceiptDisposition::created);
    EXPECT_NE(second.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 2U);
    EXPECT_EQ(repository.records()[0].original_facts, repository.records()[1].original_facts);
    EXPECT_EQ(repository.records()[0].original_arrival, repository.records()[1].original_arrival);
}

// §3.3: payment/activity references are linkage evidence, not delivery/event IDs.
TEST_F(ReceiptIntakeAcceptance, SharedPaymentAndActivityReferencesDoNotMergeDistinctEvents) {
    auto input = synthetic_input();
    const auto received = intake.ingest(input);
    input.identity.opaque_id = "event-44";
    input.facts.event_type = "synthetic.payment.confirmed";
    const auto confirmed = intake.ingest(input);

    EXPECT_EQ(confirmed.disposition, ReceiptDisposition::created);
    EXPECT_NE(confirmed.receipt_id, received.receipt_id);
    ASSERT_EQ(repository.records().size(), 2U);
    ASSERT_TRUE(repository.records()[0].original_facts.payment_reference);
    EXPECT_EQ(repository.records()[0].original_facts.payment_reference,
              repository.records()[1].original_facts.payment_reference);
    EXPECT_EQ(repository.records()[0].original_facts.activity_reference,
              repository.records()[1].original_facts.activity_reference);
    // Contribution linkage and eligibility are ACCTTEST-002, not implied by two receipts.
}

// §3.2 and §14: absent source fields must remain absent, even on retry.
TEST_F(ReceiptIntakeAcceptance, UnknownOptionalSourceFactsRemainAbsent) {
    auto input = synthetic_input();
    input.facts = {"synthetic.activity", 1, {}, {}, {}, {}, {}, {}};
    const auto first = intake.ingest(input);
    input.arrival.server_timestamp_utc_ms = 1'700'000'001'000;
    const auto retry = intake.ingest(input);

    EXPECT_EQ(retry.disposition, ReceiptDisposition::retry);
    EXPECT_EQ(retry.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 1U);
    const auto &record = repository.records().front();
    EXPECT_FALSE(record.original_facts.source_timestamp_utc_ms);
    EXPECT_FALSE(record.original_facts.actor_id);
    EXPECT_FALSE(record.original_facts.actor_display_name);
    EXPECT_FALSE(record.original_facts.money);
    EXPECT_FALSE(record.original_facts.payment_reference);
    EXPECT_FALSE(record.original_facts.activity_reference);
    ASSERT_EQ(record.attempts.size(), 2U);
    for (const auto &attempt : record.attempts) {
        EXPECT_EQ(attempt.observed_facts, input.facts);
    }
}

// Conflicting duplicate evidence cannot overwrite the first arrival or become a retry.
TEST_F(ReceiptIntakeAcceptance, ConflictRetainsOriginalAndSubsequentRetryStillUsesOriginalFacts) {
    const auto original = synthetic_input();
    const auto first = intake.ingest(original);
    auto changed = original;
    changed.facts.money->original_decimal = "99.0000";
    changed.arrival = {1'700'000'002'000, "connection-2", "configuration-r6",
                       "modifier-context-r9"};
    const auto conflict = intake.ingest(changed);
    auto repeated_original = original;
    repeated_original.arrival.server_timestamp_utc_ms = 1'700'000'003'000;
    const auto retry = intake.ingest(repeated_original);

    EXPECT_EQ(conflict.disposition, ReceiptDisposition::conflict);
    EXPECT_EQ(conflict.receipt_id, first.receipt_id);
    EXPECT_EQ(retry.disposition, ReceiptDisposition::retry);
    EXPECT_EQ(retry.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 1U);
    const auto &record = repository.records().front();
    EXPECT_EQ(record.original_facts, original.facts);
    EXPECT_EQ(record.original_arrival, original.arrival);
    ASSERT_EQ(record.attempts.size(), 3U);
    EXPECT_EQ(record.attempts[1].disposition, ReceiptDisposition::conflict);
    ASSERT_TRUE(record.attempts[1].observed_facts.money);
    EXPECT_EQ(record.attempts[1].observed_facts.money->original_decimal, "99.0000");
    EXPECT_EQ(record.attempts[1].arrival, changed.arrival);
    EXPECT_EQ(record.attempts[2].disposition, ReceiptDisposition::retry);
    ASSERT_TRUE(record.attempts[2].observed_facts.money);
    EXPECT_EQ(record.attempts[2].observed_facts.money->original_decimal, "12.3400");
}

// Revision 1 compares the complete normalized projection, including presence/precision.
TEST_F(ReceiptIntakeAcceptance, ChangedNormalizedFactsAreAuditedAsConflicts) {
    using Mutation = std::pair<const char *, std::function<void(ReceiptFacts &)>>;
    const std::array<Mutation, 14> mutations{{
        {"type", [](auto &facts) { facts.event_type = "synthetic.payment.confirmed"; }},
        {"schema", [](auto &facts) { facts.schema_version = 2; }},
        {"source time", [](auto &facts) { facts.source_timestamp_utc_ms = 1'699'999'999'999; }},
        {"actor id", [](auto &facts) { facts.actor_id = "actor-4"; }},
        {"actor name", [](auto &facts) { facts.actor_display_name = "Other Synthetic Name"; }},
        {"source precision", [](auto &facts) { facts.money->original_decimal = "12.34"; }},
        {"currency", [](auto &facts) { facts.money->currency = "USD"; }},
        {"payment", [](auto &facts) { facts.payment_reference->opaque_id = "payment-10"; }},
        {"payment namespace",
         [](auto &facts) {
             facts.payment_reference->identifier_namespace = "synthetic.payment.v2";
         }},
        {"activity", [](auto &facts) { facts.activity_reference->opaque_id = "activity-3"; }},
        {"activity namespace",
         [](auto &facts) {
             facts.activity_reference->identifier_namespace = "synthetic.activity.v2";
         }},
        {"missing source time", [](auto &facts) { facts.source_timestamp_utc_ms.reset(); }},
        {"missing amount", [](auto &facts) { facts.money.reset(); }},
        {"missing linkage", [](auto &facts) { facts.payment_reference.reset(); }},
    }};
    for (const auto &[name, mutate] : mutations) {
        SCOPED_TRACE(name);
        FakeReceiptRepository store;
        ReceiptIntake service{store};
        const auto original = synthetic_input();
        const auto first = service.ingest(original);
        auto changed = original;
        mutate(changed.facts);
        const auto result = service.ingest(changed);

        EXPECT_EQ(result.disposition, ReceiptDisposition::conflict);
        EXPECT_EQ(result.receipt_id, first.receipt_id);
        ASSERT_EQ(store.records().size(), 1U);
        const auto &record = store.records().front();
        EXPECT_EQ(record.original_facts, original.facts);
        ASSERT_EQ(record.attempts.size(), 2U);
        EXPECT_EQ(record.attempts[1].disposition, ReceiptDisposition::conflict);
        EXPECT_EQ(record.attempts[1].observed_facts, changed.facts);
    }
}

// Unknown cannot silently become known under the same receipt; a later event needs linkage.
TEST_F(ReceiptIntakeAcceptance, NewlySuppliedOptionalFactsPreserveTheUnknownOriginal) {
    auto input = synthetic_input();
    input.facts.source_timestamp_utc_ms.reset();
    input.facts.actor_id.reset();
    const auto first = intake.ingest(input);
    input.facts.source_timestamp_utc_ms = 1'700'000'000'000;
    input.facts.actor_id = "actor-3";
    const auto result = intake.ingest(input);

    EXPECT_EQ(result.disposition, ReceiptDisposition::conflict);
    EXPECT_EQ(result.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 1U);
    const auto &record = repository.records().front();
    EXPECT_FALSE(record.original_facts.source_timestamp_utc_ms);
    EXPECT_FALSE(record.original_facts.actor_id);
    ASSERT_EQ(record.attempts.size(), 2U);
    EXPECT_FALSE(record.attempts[0].observed_facts.actor_id);
    EXPECT_EQ(record.attempts[1].observed_facts.actor_id, "actor-3");
    EXPECT_EQ(record.attempts[1].observed_facts.source_timestamp_utc_ms, 1'700'000'000'000);
}

// Deduplication must consult the repository, not only an individual service's cache.
TEST_F(ReceiptIntakeAcceptance, NewServiceInstanceFindsExistingRepositoryReceipt) {
    const auto input = synthetic_input();
    const auto first = intake.ingest(input);
    ReceiptIntake another_service{repository};
    const auto retry = another_service.ingest(input);

    EXPECT_EQ(retry.disposition, ReceiptDisposition::retry);
    EXPECT_EQ(retry.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 1U);
    EXPECT_EQ(repository.records().front().attempts.size(), 2U);
    // This is service recreation over a fake, not process restart or disk durability.
}

// §3.4: a failed intake cannot leave a receipt without its first attempt or vice versa.
TEST_F(ReceiptIntakeAcceptance, FirstArrivalWriteFailureRollsBackReceiptAndAttempt) {
    for (const auto failure : {FakeReceiptRepository::Failure::after_create,
                               FakeReceiptRepository::Failure::after_append}) {
        SCOPED_TRACE(static_cast<int>(failure));
        FakeReceiptRepository store;
        ReceiptIntake service{store};
        store.fail_next = failure;
        EXPECT_THROW(static_cast<void>(service.ingest(synthetic_input())), ReceiptStorageError);
        EXPECT_TRUE(store.records().empty());

        const auto retry = service.ingest(synthetic_input());
        EXPECT_EQ(retry.disposition, ReceiptDisposition::created);
        ASSERT_EQ(store.records().size(), 1U);
        EXPECT_EQ(store.records().front().attempts.size(), 1U);
    }
}

TEST_F(ReceiptIntakeAcceptance, RetryWriteFailureLeavesOnlyPreviouslyCommittedEvidence) {
    auto input = synthetic_input();
    const auto first = intake.ingest(input);
    const auto before = repository.records();
    input.arrival.server_timestamp_utc_ms = 1'700'000'003'000;
    repository.fail_next = FakeReceiptRepository::Failure::after_append;
    EXPECT_THROW(static_cast<void>(intake.ingest(input)), ReceiptStorageError);
    EXPECT_EQ(repository.records(), before);

    const auto retry = intake.ingest(input);
    EXPECT_EQ(retry.disposition, ReceiptDisposition::retry);
    EXPECT_EQ(retry.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 1U);
    ASSERT_EQ(repository.records().front().attempts.size(), 2U);
    EXPECT_EQ(repository.records().front().attempts[1].arrival.server_timestamp_utc_ms,
              1'700'000'003'000);
}

TEST_F(ReceiptIntakeAcceptance, ConflictWriteFailurePreservesOriginalUntilEvidenceCanCommit) {
    auto input = synthetic_input();
    const auto first = intake.ingest(input);
    const auto before = repository.records();
    input.facts.money->original_decimal = "99.0000";
    repository.fail_next = FakeReceiptRepository::Failure::after_append;
    EXPECT_THROW(static_cast<void>(intake.ingest(input)), ReceiptStorageError);
    EXPECT_EQ(repository.records(), before);

    const auto conflict = intake.ingest(input);
    EXPECT_EQ(conflict.disposition, ReceiptDisposition::conflict);
    EXPECT_EQ(conflict.receipt_id, first.receipt_id);
    ASSERT_EQ(repository.records().size(), 1U);
    const auto &record = repository.records().front();
    ASSERT_TRUE(record.original_facts.money);
    EXPECT_EQ(record.original_facts.money->original_decimal, "12.3400");
    ASSERT_EQ(record.attempts.size(), 2U);
    ASSERT_TRUE(record.attempts[1].observed_facts.money);
    EXPECT_EQ(record.attempts[1].observed_facts.money->original_decimal, "99.0000");
}

// No successful new/retry/conflict result may escape before its commit succeeds.
TEST_F(ReceiptIntakeAcceptance, CommitFailureRollsBackEveryDispositionAndAllowsRetry) {
    for (const auto expected :
         {ReceiptDisposition::created, ReceiptDisposition::retry, ReceiptDisposition::conflict}) {
        SCOPED_TRACE(static_cast<int>(expected));
        FakeReceiptRepository store;
        ReceiptIntake service{store};
        auto input = synthetic_input();
        if (expected != ReceiptDisposition::created) {
            static_cast<void>(service.ingest(input));
        }
        const auto before = store.records();
        input.arrival.server_timestamp_utc_ms = 1'700'000'003'000;
        if (expected == ReceiptDisposition::conflict) {
            input.facts.money->original_decimal = "99.0000";
        }
        store.fail_next = FakeReceiptRepository::Failure::before_commit;
        EXPECT_THROW(static_cast<void>(service.ingest(input)), ReceiptStorageError);
        EXPECT_EQ(store.records(), before);

        const auto result = service.ingest(input);
        EXPECT_EQ(result.disposition, expected);
        ASSERT_EQ(store.records().size(), 1U);
        EXPECT_EQ(store.records().front().attempts.size(),
                  expected == ReceiptDisposition::created ? 1U : 2U);
    }
}
} // namespace
