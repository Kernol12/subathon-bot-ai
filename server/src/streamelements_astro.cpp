// SPDX-License-Identifier: GPL-3.0-or-later
#include <limits>
#include <subathon/core/exact_decimal.hpp>
#include <subathon/server/streamelements_astro.hpp>

#include "exchange_rate_api_json.hpp"
#include "exchange_rate_api_time.hpp"

namespace subathon::server::streamelements_astro {
namespace {
namespace wire = exchange_rate_api::detail;
namespace chrono = std::chrono;

constexpr std::string_view tips_topic = "channel.tips";
constexpr std::string_view transaction_namespace = "streamelements.transaction";

const wire::JsonField *member(const wire::JsonFields &fields, std::string_view key,
                              wire::JsonKind kind) {
    const auto it = fields.find(key);
    return it != fields.end() && it->second.kind == kind ? &it->second : nullptr;
}

std::string_view text(const wire::JsonFields &fields, std::string_view key) {
    const auto *field = member(fields, key, wire::JsonKind::string);
    return field ? std::string_view{field->text} : std::string_view{};
}

std::optional<std::string> optional_text(const wire::JsonFields &fields, std::string_view key) {
    const auto *field = member(fields, key, wire::JsonKind::string);
    return field ? std::optional{field->text} : std::nullopt;
}

wire::JsonFields object(const wire::JsonFields &fields, std::string_view key) {
    const auto *field = member(fields, key, wire::JsonKind::object);
    return field ? wire::ReplyJson{field->token}.read_members() : wire::JsonFields{};
}

Status gap(CoverageJournal &journal, Gap reason) {
    journal.record(reason);
    return Status::coverage_gap;
}

Moderation moderation(std::string_view value) {
    if (value == "allowed")
        return Moderation::allowed;
    if (value == "pending")
        return Moderation::pending;
    if (value == "rejected")
        return Moderation::rejected;
    return Moderation::unknown;
}

// JSON grammar has already been validated. Inspect the mantissa before
// resource accounting so zero or negative values do not become overflow holds.
bool positive_number(std::string_view token) {
    if (token.empty() || token.front() == '-')
        return false;
    return token.substr(0, token.find_first_of("eE")).find_first_of("123456789") != token.npos;
}

std::optional<std::int64_t> source_timestamp(std::string_view raw) {
    if (raw.size() < 20 || (raw[10] != 'T' && raw[10] != 't'))
        return {};
    const auto date = wire::iso_date(raw.substr(0, 10));
    const auto day_time = wire::time_of_day(raw.substr(11, 8));
    if (!date || !day_time)
        return {};

    auto zone = raw.substr(19);
    unsigned fraction_ms = 0;
    if (zone.front() == '.') {
        zone.remove_prefix(1);
        std::size_t digits = 0;
        while (!zone.empty() && wire::digit(zone.front())) {
            const auto digit = static_cast<unsigned>(zone.front() - '0');
            if (digits < 3)
                fraction_ms = fraction_ms * 10 + digit;
            else if (digit != 0)
                return {};
            ++digits;
            zone.remove_prefix(1);
        }
        if (digits == 0)
            return {};
        for (; digits < 3; ++digits)
            fraction_ms *= 10;
    }

    chrono::minutes offset{};
    if (zone != "Z" && zone != "z") {
        if (zone.size() != 6 || (zone[0] != '+' && zone[0] != '-') || zone[3] != ':')
            return {};
        const auto hours = wire::digits_at(zone.substr(1, 2));
        const auto minutes = wire::digits_at(zone.substr(4, 2));
        if (!hours || *hours > 23 || !minutes || *minutes > 59)
            return {};
        offset = chrono::hours{*hours} + chrono::minutes{*minutes};
        if (zone[0] == '-')
            offset = -offset;
    }
    const auto timestamp =
        chrono::sys_days{*date} + *day_time - offset + chrono::milliseconds{fraction_ms};
    return timestamp.time_since_epoch().count();
}

TransportResult outcome(TransportStatus status) { return {status, std::nullopt}; }

void clear_pending(TransportState &state) {
    state.pending_nonce.reset();
    state.pending_generation.reset();
}

bool current(const TransportState &state, std::uint64_t generation) {
    return state.tips_read && state.active_generation == generation;
}

bool pending(const TransportState &state, std::uint64_t generation) {
    return state.pending_nonce &&
           (!state.pending_generation || state.pending_generation == generation);
}

bool matches_pending(const TransportState &state, TransportInput input) {
    return pending(state, input.generation) && !input.nonce.empty() &&
           state.pending_nonce == input.nonce;
}

TransportResult interrupt(TransportState &state, CoverageJournal &journal) {
    if (!state.interruption_open) {
        journal.record(Gap::reconnect_interruption);
        state.interruption_open = true;
    }
    state.active_generation.reset();
    state.replacement_generation.reset();
    state.tips_read = false;
    clear_pending(state);
    return outcome(TransportStatus::ordinary_reconnect);
}
} // namespace

Status ingest(std::string_view frame, std::uint64_t generation, const AcknowledgedRoute &route,
              const core::ReceiptArrival &arrival, core::EvidencePolicy policy, HoldOwner &owner,
              CoverageJournal &journal) {
    if (generation < route.generation)
        return gap(journal, Gap::retired_delivery);
    if (generation != route.generation || route.canonical_room.empty() ||
        frame.size() > wire::body_limit)
        return Status::ignored;

    try {
        const auto root = wire::ReplyJson{frame}.read_members();
        if (text(root, "type") != "message" || text(root, "topic") != tips_topic)
            return Status::ignored;

        const auto data = object(root, "data");
        if (text(root, "room") != route.canonical_room ||
            text(data, "channel") != route.canonical_room)
            return Status::source_binding_failure;

        const auto id = text(data, "_id");
        if (id.empty())
            return gap(journal, Gap::missing_tip_identity);
        const auto transaction_id = text(data, "transactionId");
        if (transaction_id.empty())
            return gap(journal, Gap::missing_transaction);
        const auto donation = object(data, "donation");
        const auto *amount = member(donation, "amount", wire::JsonKind::number);
        if (!amount || !positive_number(amount->token))
            return gap(journal, Gap::invalid_amount);
        const auto currency = text(donation, "currency");
        if (!wire::currency_code(currency))
            return gap(journal, Gap::invalid_currency);

        core::ReceiptIdentity identity{{"streamelements", std::string{route.canonical_room}},
                                       "streamelements.tip",
                                       std::string{id}};
        core::ReceiptLinkReference payment{std::string{transaction_namespace},
                                           std::string{transaction_id}};
        try {
            (void)core::parse_decimal(amount->token);
        } catch (const std::length_error &) {
            return owner.retain_overflow({std::string{route.installation},
                                          std::string{route.stream}, arrival, std::move(identity),
                                          std::move(payment), std::string{amount->token},
                                          std::string{currency}});
        }

        const auto created = source_timestamp(text(data, "createdAt"));
        const auto user = object(donation, "user");
        const auto evidence = text(data, "status") == "success" ? core::PaymentEvidence::confirmed
                                                                : core::PaymentEvidence::unknown;
        core::ReceiptFacts facts{
            "streamelements.tip.completed",
            1,
            created,
            std::nullopt,
            optional_text(user, "username"),
            core::ReceiptMoney{std::string{amount->token}, std::string{currency}},
            std::move(payment),
            std::nullopt};
        return owner.submit({std::string{route.installation},
                             std::string{route.stream},
                             {std::move(identity), std::move(facts), arrival},
                             evidence,
                             policy,
                             moderation(text(data, "approved")),
                             created ? SourceTimeQuality::reliable : SourceTimeQuality::absent,
                             optional_text(data, "provider"),
                             optional_text(root, "id")});
    } catch (const exchange_rate_api::FailureKind &) {
        // An unclassified envelope never supplies a trusted observation.
        return Status::ignored;
    }
}

TransportResult advance(TransportState &state, TransportInput input, CoverageJournal &journal) {
    switch (input.event) {
    case TransportEvent::welcome: {
        if (state.tips_read || state.configured_room.empty() || input.generation == 0 ||
            state.nonce_counter == std::numeric_limits<std::uint64_t>::max())
            return outcome(TransportStatus::ignored);
        auto nonce = std::to_string(++state.nonce_counter);
        state.pending_generation = input.generation;
        state.pending_nonce = nonce;
        return {TransportStatus::awaiting_ack,
                SubscriptionRequest{std::string{tips_topic}, "jwt", state.configured_room,
                                    std::move(nonce)}};
    }
    case TransportEvent::subscribe_success:
        if (!matches_pending(state, input) || input.topic != tips_topic || input.room.empty())
            return outcome(TransportStatus::awaiting_ack);
        if (state.canonical_room && *state.canonical_room != input.room) {
            clear_pending(state);
            return outcome(TransportStatus::source_binding_failure);
        }
        if (!state.canonical_room)
            state.canonical_room = input.room;
        state.active_generation = input.generation;
        state.tips_read = true;
        state.interruption_open = false;
        clear_pending(state);
        return outcome(TransportStatus::acknowledged);
    case TransportEvent::authorization_deadline:
    case TransportEvent::rate_limit:
        if (input.event == TransportEvent::rate_limit ? !pending(state, input.generation)
                                                      : !matches_pending(state, input))
            return outcome(TransportStatus::ignored);
        clear_pending(state);
        return outcome(TransportStatus::retryable);
    case TransportEvent::subscribe_refused:
    case TransportEvent::acknowledgement_timeout:
        if (!matches_pending(state, input))
            return outcome(TransportStatus::ignored);
        clear_pending(state);
        return outcome(TransportStatus::subscription_failed);
    case TransportEvent::connection_rate_limit:
        return {TransportStatus::retryable, std::nullopt, input.retry_after_seconds};
    case TransportEvent::draining:
        return outcome(TransportStatus::ordinary_reconnect);
    case TransportEvent::graceful_reconnect:
        if (!state.tips_read || !state.active_generation ||
            input.generation <= *state.active_generation || state.replacement_generation)
            return outcome(TransportStatus::ignored);
        state.replacement_generation = input.generation;
        return outcome(TransportStatus::acknowledged);
    case TransportEvent::replacement_welcome:
        if (state.tips_read && state.replacement_generation == input.generation) {
            state.active_generation = input.generation;
            state.replacement_generation.reset();
            return outcome(TransportStatus::acknowledged);
        }
        return outcome(TransportStatus::ignored);
    case TransportEvent::reconnect_rejected:
        if (state.replacement_generation == input.generation)
            return interrupt(state, journal);
        return outcome(TransportStatus::ignored);
    case TransportEvent::unexpected_disconnect:
        if (current(state, input.generation))
            return interrupt(state, journal);
        return outcome(TransportStatus::ignored);
    case TransportEvent::delivery:
        if (current(state, input.generation))
            return outcome(TransportStatus::current_delivery);
        if (state.active_generation && input.generation < *state.active_generation) {
            journal.record(Gap::retired_delivery);
            return outcome(TransportStatus::retired_delivery);
        }
        return outcome(TransportStatus::ignored);
    case TransportEvent::binary_frame:
        return outcome(TransportStatus::ignored);
    }
    return outcome(TransportStatus::ignored);
}
} // namespace subathon::server::streamelements_astro
