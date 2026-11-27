// SPDX-License-Identifier: GPL-3.0-or-later
#include "astro_fixture_api.hpp"

#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace {
using namespace astro_fixture;
namespace core = subathon::core;

constexpr std::string_view base = R"json({
  "id":"delivery-1", "ts":"2026-01-02T03:04:06Z",
  "type":"message", "topic":"channel.tips", "room":"synthetic-room",
  "data":{
    "_id":"tip-1", "channel":"synthetic-room", "transactionId":"transaction-1",
    "provider":"synthetic-payment-provider", "status":"success", "approved":"allowed",
    "createdAt":"2026-01-02T04:04:05.123+01:00", "updatedAt":"2026-01-02T03:04:06Z",
    "donation":{"amount":4.2, "currency":"USD",
                "user":{"username":"Synthetic Donor"}}
  }
})json";

constexpr AcknowledgedRoute route{"synthetic-room", 1, "installation-1", "stream-1"};
const core::ReceiptArrival arrival{1767323050100, "connection-1", "configuration-1", "modifiers-1"};

void require(bool ok, const char *message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class RecordingOwner final : public HoldOwner, public CoverageJournal {
public:
    explicit RecordingOwner(Status verdict) : verdict_(verdict) {}

    Status submit(Attempt attempt) override {
        calls.push_back(std::move(attempt));
        return verdict_;
    }

    Status retain_overflow(NumericOverflow overflow) override {
        overflows.push_back(std::move(overflow));
        return verdict_;
    }
    void record(Gap gap) override { gaps.push_back(gap); }

    std::vector<Attempt> calls;
    std::vector<NumericOverflow> overflows;
    std::vector<Gap> gaps;

private:
    Status verdict_;
};

std::string changed(std::string input, std::string_view old, std::string_view replacement) {
    const auto index = input.find(old);
    require(index != std::string::npos && input.find(old, index + old.size()) == std::string::npos,
            "Fixture patch must match exactly once");
    input.replace(index, old.size(), replacement);
    return input;
}

void projection() {
    RecordingOwner owner(Status::admitted);
    const auto policy = core::EvidencePolicy::accept_provider_report;
    require(ingest(base, route.generation, route, arrival, policy, owner, owner) == Status::admitted,
            "A complete provider report reaches and returns the owning admission verdict");
    require(owner.calls.size() == 1, "A complete observation calls the owner exactly once");
    const auto &attempt = owner.calls.front();
    const auto &receipt = attempt.receipt;
    require(attempt.installation == route.installation && attempt.stream == route.stream &&
                receipt.identity == core::ReceiptIdentity{
                    {"streamelements", "synthetic-room"}, "streamelements.tip", "tip-1"},
            "Receipt identity uses the configured installation and stream and the canonical room");
    require(receipt.facts == core::ReceiptFacts{
                "streamelements.tip.completed", 1, 1767323045123, std::nullopt, "Synthetic Donor",
                core::ReceiptMoney{"4.2", "USD"},
                core::ReceiptLinkReference{"streamelements.transaction", "transaction-1"},
                std::nullopt},
            "Retained facts preserve the exact lexeme, timestamp, and disjoint identity fields");
    require(receipt.arrival == arrival, "Arrival is taken from the supplied server context");
    require(attempt.evidence == core::PaymentEvidence::confirmed && attempt.policy == policy &&
                attempt.moderation == Moderation::allowed &&
                attempt.source_time_quality == SourceTimeQuality::reliable &&
                attempt.reported_provider == "synthetic-payment-provider" &&
                attempt.delivery_id == "delivery-1",
            "Evidence is provider-reported and distinct from moderation and attempt metadata");
}

void optional_source_fields(std::string_view test) {
    RecordingOwner owner(Status::admitted);
    const auto policy = core::EvidencePolicy::accept_provider_report;
    const auto payload = test == "missing-optional-source-fields"
        ? changed(changed(std::string(base), R"("approved":"allowed",)", ""),
                  R"("createdAt":"2026-01-02T04:04:05.123+01:00",)", "")
        : changed(std::string(base), "04:04:05.123+01:00", "04:04:05.1234+01:00");
    require(ingest(payload, route.generation, route, arrival, policy, owner, owner) == Status::admitted,
            "An optional source-time omission still reaches the owning lane");
    require(owner.calls.size() == 1, "One optional-field handoff");
    const auto &attempt = owner.calls.front();
    require(!attempt.receipt.facts.source_timestamp_utc_ms &&
                attempt.source_time_quality == SourceTimeQuality::absent,
            "Absent and sub-millisecond source times stay absent");
    require(attempt.moderation == (test == "missing-optional-source-fields"
                                     ? Moderation::unknown : Moderation::allowed),
            "Moderation retains its separately supplied state");
}

void owner_handoff(std::string_view row) {
    const auto policy = core::EvidencePolicy::accept_provider_report;
    RecordingOwner first(Status::admitted);
    require(ingest(base, route.generation, route, arrival, policy, first, first) == Status::admitted,
            "The first completed tip reaches owning admission");
    require(first.calls.size() == 1, "First tip forwarded exactly once");
    const auto &prior = first.calls.front();

    auto next = changed(std::string(base), "delivery-1", "delivery-2");
    auto verdict = Status::retry;
    if (row == "same-tip-redelivery") {
        next = changed(std::move(next), R"("approved":"allowed")", R"("approved":"pending")");
        next = changed(std::move(next), R"("updatedAt":"2026-01-02T03:04:06Z")",
                       R"("updatedAt":"2026-01-02T03:04:10Z")");
    } else if (row == "cross-tip-payment-hold") {
        next = changed(std::move(next), "tip-1", "tip-2");
        next = changed(std::move(next), "synthetic-payment-provider", "synthetic-other-label");
        verdict = Status::held;
    } else {
        next = changed(std::move(next), R"("amount":4.2)", R"("amount":42e-1)");
        verdict = Status::conflict;
    }

    // The fake prescribes the owner's verdict. Real collision and durability
    // behavior remains an independent #882 prerequisite.
    RecordingOwner subsequent(verdict);
    require(ingest(next, route.generation, route, arrival, policy, subsequent, subsequent) == verdict,
            "The adapter returns the owner's retry, hold, or conflict verdict");
    require(subsequent.calls.size() == 1, "Subsequent observation forwarded exactly once");
    const auto &attempt = subsequent.calls.front();
    require(attempt.delivery_id == "delivery-2", "The new delivery remains separate metadata");
    if (verdict == Status::retry) {
        require(attempt.receipt.identity == prior.receipt.identity &&
                    attempt.receipt.facts == prior.receipt.facts &&
                    attempt.moderation == Moderation::pending,
                "Delivery, update time and moderation do not change retained facts");
    } else if (verdict == Status::held) {
        require(attempt.receipt.identity == core::ReceiptIdentity{
                    prior.receipt.identity.origin, "streamelements.tip", "tip-2"} &&
                    attempt.receipt.facts == prior.receipt.facts &&
                    attempt.reported_provider == "synthetic-other-label",
                "The distinct tip and changed provider label do not replace the payment reference");
    } else {
        require(attempt.receipt.identity == prior.receipt.identity &&
                    attempt.receipt.facts.money == core::ReceiptMoney{"42e-1", "USD"} &&
                    attempt.receipt.facts.payment_reference == prior.receipt.facts.payment_reference,
                "A differently spelled exact amount reaches the owner verbatim under the same identity");
    }
}

void unknown_policy(core::EvidencePolicy policy, Status owner_verdict, bool absent) {
    RecordingOwner owner(owner_verdict);
    const auto frame = absent
        ? changed(std::string(base), R"("status":"success", )", "")
        : changed(std::string(base), "success", "vendor-future-status");

    require(ingest(frame, route.generation, route, arrival, policy, owner, owner) == owner_verdict,
            "A complete tip with unknown status returns the configured owner policy verdict");
    require(owner.calls.size() == 1, "Unknown payment evidence enters eligibility exactly once");
    const auto &attempt = owner.calls.front();
    require(attempt.policy == policy && attempt.evidence == core::PaymentEvidence::unknown,
            "The existing EvidencePolicy sees unknown evidence and retains its configured choice");
    require(attempt.receipt.facts.money == core::ReceiptMoney{"4.2", "USD"} &&
                attempt.moderation == Moderation::allowed,
            "Unknown status preserves usable money and does not borrow payment proof from moderation");
}


struct GapCase {
    std::string_view name;
    std::string_view before;
    std::string_view after;
    Gap reason;
};

constexpr GapCase gap_cases[] = {
    {"missing-tip-id", R"("_id":"tip-1", )", "", Gap::missing_tip_identity},
    {"empty-tip-id", R"("_id":"tip-1")", R"("_id":"")", Gap::missing_tip_identity},
    {"missing-transaction", R"("transactionId":"transaction-1",)", "", Gap::missing_transaction},
    {"missing-donation", R"(,
    "donation":{"amount":4.2, "currency":"USD",
                "user":{"username":"Synthetic Donor"}})", "", Gap::invalid_amount},
    {"missing-amount", R"("amount":4.2, )", "", Gap::invalid_amount},
    {"amount-is-string", R"("amount":4.2)", R"("amount":"4.2")", Gap::invalid_amount},
    {"amount-is-zero", R"("amount":4.2)", R"("amount":0.00)", Gap::invalid_amount},
    {"currency-case", R"("currency":"USD")", R"("currency":"usd")", Gap::invalid_currency},
    {"currency-absent", R"(, "currency":"USD")", "", Gap::invalid_currency},
};

void incomplete_tip(std::string_view payload, Gap reason) {
    RecordingOwner calls(Status::admitted);
    require(ingest(payload, route.generation, route, arrival,
                   core::EvidencePolicy::accept_provider_report, calls, calls) == Status::coverage_gap,
            "An incomplete tip yields a typed coverage gap");
    require(calls.gaps == std::vector{reason},
            "Exactly one gap with the expected reason is recorded");
    require(calls.calls.empty() && calls.overflows.empty(),
            "An incomplete tip never enters eligibility or the hold owner");
}

void overflow(std::string_view row) {
    const auto payload = changed(std::string(base), R"("amount":4.2)", R"("amount":1e257)");
    if (row == "invalid-currency-before-overflow") {
        incomplete_tip(changed(payload, R"("currency":"USD")", R"("currency":"usd")"),
                       Gap::invalid_currency);
        return;
    }
    RecordingOwner calls(Status::held);
    require(ingest(payload, route.generation, route, arrival,
                   core::EvidencePolicy::accept_provider_report, calls, calls) == Status::held,
            "An otherwise complete numeric overflow yields the retained owner hold");
    require(calls.calls.empty() && calls.gaps.empty() && calls.overflows.size() == 1,
            "Overflow only enters the separate retained review hold path");
    const auto &hold = calls.overflows.front();
    require(hold.installation == route.installation && hold.stream == route.stream &&
                hold.arrival == arrival && hold.identity == core::ReceiptIdentity{
                {"streamelements", "synthetic-room"}, "streamelements.tip", "tip-1"} &&
                hold.payment_reference == core::ReceiptLinkReference{
                    "streamelements.transaction", "transaction-1"} &&
                hold.original_decimal == "1e257" && hold.currency == "USD",
            "Hold has the unchanged identity, payment reference and bounded amount provenance");
}

void rejected_ingest(std::string_view row) {
    const bool moderation = row == "moderation-topic-ignored";
    const bool stale = row == "retired-ingest-gap";
    auto payload = std::string(base);
    if (moderation)
        payload = changed(payload, R"("topic":"channel.tips")", R"("topic":"channel.tips.moderation")");
    else if (row == "mismatched-envelope-room")
        payload = changed(payload, R"("room":"synthetic-room")", R"("room":"other-synthetic-room")");
    else if (!stale)
        payload = changed(payload, R"("channel":"synthetic-room")", R"("channel":"other-synthetic-room")");
    RecordingOwner calls(Status::admitted);
    const auto expected = stale ? Status::coverage_gap :
        (moderation ? Status::ignored : Status::source_binding_failure);
    auto current = route;
    if (stale)
        current.generation = 2;
    require(ingest(payload, route.generation, current, arrival,
                   core::EvidencePolicy::accept_provider_report, calls, calls) == expected,
            "Unsubscribed, mismatched and retired deliveries have distinct dispositions");
    const auto expected_gaps = stale ? std::vector{Gap::retired_delivery} : std::vector<Gap>{};
    require(calls.calls.empty() && calls.overflows.empty() && calls.gaps == expected_gaps,
            "Excluded deliveries never reach intake and a retired delivery records its gap");
}

void valid_field_variants(std::string_view row) {
    RecordingOwner calls(Status::admitted);
    auto payload = std::string(base);
    if (row == "rejected-moderation")
        payload = changed(payload, R"("approved":"allowed")", R"("approved":"rejected")");
    else if (row == "unusual-currency")
        payload = changed(payload, R"("currency":"USD")", R"("currency":"XYZ")");
    else
        payload = changed(payload, ".123+01:00", ".1230+01:00");
    require(ingest(payload, route.generation, route, arrival,
                   core::EvidencePolicy::accept_provider_report, calls, calls) == Status::admitted,
            "Well-formed variants still enter owning admission");
    require(calls.calls.size() == 1 && calls.gaps.empty() && calls.overflows.empty(),
            "Well-formed variants produce one owning attempt");
    const auto &attempt = calls.calls.front();
    require(attempt.evidence == core::PaymentEvidence::confirmed &&
                attempt.moderation == (row == "rejected-moderation" ? Moderation::rejected : Moderation::allowed),
            "Moderation never changes successful payment evidence");
    require(attempt.receipt.facts.money &&
                attempt.receipt.facts.money->currency == (row == "unusual-currency" ? "XYZ" : "USD"),
            "Syntactically valid currency reaches valuation unchanged");
    require(attempt.receipt.facts.source_timestamp_utc_ms == 1767323045123,
            "A trailing fractional zero preserves the exact millisecond instant");
}

void unclassifiable_text_gap() {
    RecordingOwner calls(Status::admitted);
    constexpr std::string_view frame = R"({"type":"message","topic":"channel.tips","data":)";
    require(ingest(frame, route.generation, route, arrival, core::EvidencePolicy::accept_provider_report,
                   calls, calls) == Status::coverage_gap,
            "Unclassifiable text on an acknowledged route reports unknown coverage");
    require(calls.gaps == std::vector{Gap::unclassifiable_text} &&
                calls.calls.empty() && calls.overflows.empty(),
            "Unclassifiable text records only a typed gap, with no credit or owner handoff");
}

void transport_case(std::string_view row) {
    RecordingOwner calls(Status::admitted);
    constexpr std::string_view topic = "channel.tips";
    constexpr std::string_view nonce = "synthetic-request";
    constexpr std::string_view room = "synthetic-room";
    TransportState state{};
    state.configured_room = "synthetic-alias";

    if (row == "welcome-requests-tips") {
        const auto result = advance(state, {TransportEvent::welcome, 1}, calls);
        require(result.status == TransportStatus::awaiting_ack,
                "A welcome requests subscription without establishing capability");
        require(result.subscribe && result.subscribe->topic == topic &&
                    result.subscribe->token_type == "jwt" &&
                    result.subscribe->room == "synthetic-alias" &&
                    !result.subscribe->nonce.empty() &&
                    state.pending_nonce == result.subscribe->nonce && !state.tips_read,
                "Only the configured channel tips subscription is requested with a fresh nonce");
        return;
    }

    if (row == "canonical-ack-binds" || row == "nonce-less-ack-refused" ||
        row == "wrong-nonce-ack-refused" || row == "wrong-topic-ack-refused" ||
        row == "empty-room-ack-refused" || row == "canonical-ack-change-refused" ||
        row == "rate-limit-retryable" || row == "authorization-deadline-retryable" ||
        row == "subscribe-refused" || row == "acknowledgement-timeout") {
        state.pending_nonce = nonce;
        auto input = TransportInput{TransportEvent::subscribe_success, 1, nonce, topic, room};
        auto expected = TransportStatus::acknowledged;
        if (row == "nonce-less-ack-refused" || row == "wrong-nonce-ack-refused") {
            input.nonce = row == "nonce-less-ack-refused" ? "" : "different-request";
            expected = TransportStatus::awaiting_ack;
        } else if (row == "wrong-topic-ack-refused") {
            input.topic = "channel.activities";
            expected = TransportStatus::awaiting_ack;
        } else if (row == "empty-room-ack-refused") {
            input.room = "";
            expected = TransportStatus::awaiting_ack;
        } else if (row == "canonical-ack-change-refused") {
            state.canonical_room = room;
            input.room = "different-room";
            expected = TransportStatus::source_binding_failure;
        } else if (row == "rate-limit-retryable") {
            input = {TransportEvent::rate_limit, 1};
            expected = TransportStatus::retryable;
        } else if (row == "authorization-deadline-retryable") {
            input = {TransportEvent::authorization_deadline, 1, nonce};
            expected = TransportStatus::retryable;
        } else if (row == "subscribe-refused" || row == "acknowledgement-timeout") {
            input = {row == "subscribe-refused" ? TransportEvent::subscribe_refused
                                                : TransportEvent::acknowledgement_timeout,
                     1, nonce};
            expected = TransportStatus::subscription_failed;
        }

        const auto result = advance(state, input, calls);
        require(result.status == expected, "Subscription event has the expected typed disposition");
        require(state.tips_read == (expected == TransportStatus::acknowledged),
                "Only a matching successful acknowledgement establishes tips capability");
        if (expected == TransportStatus::acknowledged)
            require(state.canonical_room == room && state.active_generation == 1,
                    "The canonical result of acknowledging a requested alias is stored once");
        else if (expected == TransportStatus::source_binding_failure)
            require(state.canonical_room == room, "An established canonical room cannot be replaced");
        else
            require(!state.canonical_room, "A nonacknowledgement cannot set the source binding");
        return;
    }

    if (row == "connection-rate-limit" || row == "draining-reconnect") {
        const bool limited = row == "connection-rate-limit";
        const auto delay = limited ? std::optional<std::uint32_t>{2} : std::nullopt;
        const auto result = advance(state,
            {limited ? TransportEvent::connection_rate_limit : TransportEvent::draining,
             1, {}, {}, {}, delay}, calls);
        require(result.status == (limited ? TransportStatus::retryable
                                          : TransportStatus::ordinary_reconnect),
                "Connection failure selects a retry or the shared reconnect path");
        require(result.retry_after_seconds == delay && !state.tips_read && !result.subscribe && calls.gaps.empty(),
                "Retry-After remains specific to connection rate limits");
        return;
    }

    if (row == "acknowledged-disconnect-reconnect") {
        const auto initial = advance(state, {TransportEvent::welcome, 1}, calls);
        require(initial.status == TransportStatus::awaiting_ack && initial.subscribe,
                "The initial welcome issues an unacknowledged subscription");
        const auto old_nonce = initial.subscribe->nonce;
        require(!old_nonce.empty() && advance(state,
                    {TransportEvent::subscribe_success, 1, old_nonce, topic, room}, calls).status ==
                    TransportStatus::acknowledged,
                "The original subscription is acknowledged using its nonce");
        require(advance(state, {TransportEvent::unexpected_disconnect, 1}, calls).status ==
                    TransportStatus::ordinary_reconnect,
                "An acknowledged disconnect requires an ordinary reconnect");
        require(!state.tips_read && !state.active_generation && state.interruption_open &&
                    calls.gaps == std::vector{Gap::reconnect_interruption},
                "The disconnect notifies the gap owner of one unknown-coverage interruption");
        require(advance(state, {TransportEvent::draining, 2}, calls).status ==
                    TransportStatus::ordinary_reconnect && state.interruption_open &&
                    calls.gaps.size() == 1,
                "An unacknowledged 502 retry leaves the existing gap open");
        const auto fresh = advance(state, {TransportEvent::welcome, 3}, calls);
        require(fresh.status == TransportStatus::awaiting_ack && fresh.subscribe &&
                    !fresh.subscribe->nonce.empty() && fresh.subscribe->nonce != old_nonce &&
                    state.interruption_open && !state.tips_read,
                "Ordinary reconnect issues a distinct nonce before restoring coverage");
        require(advance(state, {TransportEvent::subscribe_success, 3, old_nonce, topic, room},
                        calls).status == TransportStatus::awaiting_ack &&
                    state.interruption_open && !state.tips_read,
                "An old acknowledgement cannot restore coverage on a fresh subscription");
        require(advance(state, {TransportEvent::subscribe_success, 3, fresh.subscribe->nonce, topic, room},
                        calls).status == TransportStatus::acknowledged &&
                    state.tips_read && state.active_generation == 3 && !state.interruption_open,
                "Only the new acknowledgement restores current coverage");
        return;
    }

    state.canonical_room = room;
    state.active_generation = 1;
    state.tips_read = true;
    if (row == "graceful-reconnect-keeps-old") {
        const auto result = advance(state, {TransportEvent::graceful_reconnect, 2}, calls);
        require(result.status == TransportStatus::acknowledged,
                "Starting graceful reconnect retains acknowledgement on the old route");
        require(state.active_generation == 1 && state.replacement_generation == 2 &&
                    state.tips_read && !result.subscribe,
                "Waiting for replacement welcome leaves the old generation active");
        require(advance(state, {TransportEvent::delivery, 1}, calls).status ==
                    TransportStatus::current_delivery && calls.gaps.empty(),
                "A delivery on the old active route remains current until replacement welcome");
    } else if (row == "old-disconnect-during-handoff") {
        state.replacement_generation = 2;
        require(advance(state, {TransportEvent::unexpected_disconnect, 1}, calls).status ==
                    TransportStatus::ordinary_reconnect,
                "Old socket loss before replacement welcome requires ordinary reconnect");
        require(!state.active_generation && !state.replacement_generation && !state.tips_read &&
                    state.interruption_open && calls.gaps == std::vector{Gap::reconnect_interruption},
                "Old socket loss abandons the pending replacement and records one gap");
        const auto late = advance(state, {TransportEvent::replacement_welcome, 2}, calls);
        require(late.status == TransportStatus::ignored && !state.tips_read &&
                    !state.active_generation && state.interruption_open && calls.gaps.size() == 1,
                "Late welcome on an abandoned replacement cannot restore coverage");
    } else if (row == "replacement-welcome-handoff") {
        state.replacement_generation = 2;
        const auto result = advance(state, {TransportEvent::replacement_welcome, 2}, calls);
        require(result.status == TransportStatus::acknowledged,
                "Replacement welcome restores acknowledgement");
        require(state.active_generation == 2 && state.canonical_room == room && state.tips_read &&
                    !result.subscribe,
                "Handoff switches generations without a second subscription");
        require(advance(state, {TransportEvent::delivery, 2}, calls).status ==
                    TransportStatus::current_delivery,
                "Delivery resumes on the current route");
        require(advance(state, {TransportEvent::delivery, 1}, calls).status ==
                    TransportStatus::retired_delivery,
                "The actual handoff retires the former generation");
        require(calls.gaps == std::vector{Gap::retired_delivery},
                "The retired delivery records an unknown-coverage gap");
    } else if (row == "reconnect-rejection-fallback") {
        state.replacement_generation = 2;
        const auto result = advance(state, {TransportEvent::reconnect_rejected, 2}, calls);
        require(result.status == TransportStatus::ordinary_reconnect,
                "Rejected or expired reconnect authorization falls back to ordinary reconnect");
        require(!state.tips_read && !state.active_generation &&
                    calls.gaps == std::vector{Gap::reconnect_interruption},
                "The interrupted interval gets a retained coverage gap");
    } else if (row == "binary-transport-ignored") {
        require(advance(state, {TransportEvent::binary_frame, 1}, calls).status ==
                    TransportStatus::ignored && calls.gaps.empty(),
                "Binary input is ignored without recording a coverage gap");
    } else {
        std::exit(2);
    }
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    const std::string_view test(argv[1]);
    if (test == "completed-tip")
        projection();
    else if (test == "unclassifiable-text-gap")
        unclassifiable_text_gap();
    else if (test == "missing-optional-source-fields" || test == "submillisecond-source-time")
        optional_source_fields(test);
    else if (test == "same-tip-redelivery" || test == "cross-tip-payment-hold" ||
             test == "exact-decimal-spelling-conflict")
        owner_handoff(test);
    else if (test == "unknown-accept")
        unknown_policy(core::EvidencePolicy::accept_provider_report, Status::admitted, false);
    else if (test == "unknown-review")
        unknown_policy(core::EvidencePolicy::require_evidence_review, Status::pending_review, true);
    else if (test == "unknown-record-only")
        unknown_policy(core::EvidencePolicy::record_only, Status::recorded, false);
    else if (test == "positive-overflow" || test == "invalid-currency-before-overflow")
        overflow(test);
    else if (test == "rejected-moderation" || test == "unusual-currency" ||
             test == "exact-millisecond-trailing-zero")
        valid_field_variants(test);
    else if (test == "mismatched-payload-channel" || test == "mismatched-envelope-room" ||
             test == "retired-ingest-gap" || test == "moderation-topic-ignored")
        rejected_ingest(test);
    else {
        for (const auto &candidate : gap_cases) {
            if (candidate.name == test) {
                incomplete_tip(changed(std::string(base), candidate.before, candidate.after),
                               candidate.reason);
                return 0;
            }
        }
        transport_case(test);
    }
}
