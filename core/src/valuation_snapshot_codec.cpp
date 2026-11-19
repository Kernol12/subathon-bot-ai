// SPDX-License-Identifier: GPL-3.0-or-later
#include "rate_snapshot_storage.hpp"

#include <bit>
#include <new>
#include <subathon/core/valuation_snapshot_codec.hpp>
#include <type_traits>
#include <utility>

namespace subathon::core {
namespace {
using Error = ExactValuationError;
constexpr auto limits = detail::stored_snapshot_limits;
constexpr std::string_view magic = "RSV1";
constexpr std::size_t format_bytes = 256;
constexpr std::size_t document_bytes = 32768;
constexpr std::size_t potential_bytes = detail::stored_potential_digits;
static_assert(document_bytes <= std::string{}.max_size());
// Even empty vector entries use at least four frame bytes each. A complete
// borrowed scan therefore bounds the sole owning replay below its scratch cap.
static_assert(document_bytes / 4 <= detail::snapshot_scratch_items);

const std::invalid_argument &failure_message() {
    static const std::invalid_argument message("Invalid exact valuation");
    return message;
}

// Prepare the message at library initialization. Keep its lifetime tied to
// first use in case another translation unit uses the exception during startup.
[[maybe_unused]] const auto &prepared_message = failure_message();

[[noreturn]] void fail(Error error) { throw ExactValuationFailure(error); }

bool charge(std::size_t &used, std::size_t added, std::size_t ceiling) {
    if (added > ceiling - used)
        return false;
    used += added;
    return true;
}

enum class Collection { sources, quotes, overrides, constraints, components, members };

struct Budget {
    std::size_t raw_bytes{};
    std::size_t parsed_bytes{};
    std::size_t items{};
    std::size_t quotes{};
    std::size_t members{};

    template <class Entry> bool collection(std::uint64_t declared, Collection kind) {
        if (!std::in_range<std::size_t>(declared))
            return false;
        const auto count = static_cast<std::size_t>(declared);
        constexpr auto destination_max = std::vector<Entry>{}.max_size();
        if (count > destination_max)
            return false;
        switch (kind) {
        case Collection::sources:
            if (count > limits.observations)
                return false;
            break;
        case Collection::quotes:
            if (count > limits.quotes_per_observation ||
                !charge(quotes, count, limits.total_quotes))
                return false;
            break;
        case Collection::overrides:
            if (count > limits.overrides)
                return false;
            break;
        case Collection::constraints:
            if (count > limits.constraints)
                return false;
            break;
        case Collection::components:
            break;
        case Collection::members:
            if (!charge(members, count, limits.currencies))
                return false;
            break;
        }
        return charge(items, count, limits.items);
    }

    bool raw(std::size_t size) { return charge(raw_bytes, size, limits.owned_bytes); }
    bool parsed(std::size_t size) { return charge(parsed_bytes, size, limits.owned_bytes); }
};

// Discard mode evaluates the same braced field sequence without constructing a
// DTO, string, or collection. It keeps the first error and consumes nothing
// further after it. Public exception construction happens outside the scan.
template <class T> struct Discard {
    template <class... Fields> Discard(Fields &&...) noexcept {}
};

template <bool Own> class Reader {
  public:
    explicit Reader(std::string_view bytes) : remaining_(bytes) {}
    template <class T> using Field = std::conditional_t<Own, T, Discard<T>>;

    Field<EffectiveRateSnapshotInput> model() {
        if (take(magic.size()) != magic)
            fault(Error::malformed_document);
        Field<EffectiveRateSnapshotInput> result{
            ref(),
            optional<std::string>([&] { return ref(); }),
            optional_instant(),
            list<FetchedRateObservationInput>(Collection::sources, [&] { return observation(); }),
            list<ManualRateOverrideInput>(Collection::overrides, [&] { return manual(); }),
            list<RateConstraint>(Collection::constraints, [&] { return constraint(); }),
            list<RateComponentInput>(Collection::components, [&] { return component(); })};
        if (!remaining_.empty())
            fault(Error::malformed_document);
        return result;
    }

    [[nodiscard]] std::optional<Error> error() const noexcept { return error_; }

  private:
    void fault(Error error) noexcept {
        if (!error_)
            error_ = error;
    }

    std::string_view take(std::size_t width) {
        if (error_)
            return {};
        if (width > remaining_.size()) {
            fault(Error::malformed_document);
            return {};
        }
        auto token = remaining_.substr(0, width);
        remaining_.remove_prefix(width);
        return token;
    }

    std::uint64_t fixed(std::size_t width) {
        std::uint64_t result = 0;
        for (const unsigned char byte : take(width))
            result = (result << 8) | byte;
        return result;
    }

    bool tag(bool scalar) {
        const auto value = fixed(1);
        if (value > 1)
            fault(Error::malformed_document);
        if (!error_ && scalar && !budget_.raw(1))
            fault(Error::resource_limit);
        return !error_ && value == 1;
    }

    std::uint64_t scalar64() {
        const auto value = fixed(8);
        if (!error_ && !budget_.raw(8))
            fault(Error::resource_limit);
        return value;
    }

    Field<std::string> field(std::size_t ceiling) {
        const auto size = fixed(4);
        if (error_)
            return {};
        // The fixed per-field ceiling also fits the owning string's size type.
        if (size > ceiling || !budget_.raw(static_cast<std::size_t>(size))) {
            fault(Error::resource_limit);
            return {};
        }
        const auto bytes = take(static_cast<std::size_t>(size));
        if constexpr (Own)
            return std::string(bytes);
        else
            return {};
    }

    Field<std::string> ref() { return field(limits.reference_bytes); }
    Field<std::string> currency() { return field(limits.currency_bytes); }
    Field<std::string> decimal() { return field(maximum_decimal_bytes); }

    template <class T, class Read> Field<std::optional<T>> optional(Read read) {
        if (tag(false))
            return {read()};
        return {};
    }

    Field<std::optional<std::int64_t>> optional_instant() {
        return optional<std::int64_t>([&] { return std::bit_cast<std::int64_t>(scalar64()); });
    }

    template <class T, class Read> Field<std::vector<T>> list(Collection kind, Read read) {
        const auto count = fixed(4);
        Field<std::vector<T>> result;
        if (error_)
            return result;
        if (!budget_.collection<T>(count, kind)) {
            fault(Error::resource_limit);
            return result;
        }
        if constexpr (Own)
            result.reserve(static_cast<std::size_t>(count));
        for (std::uint64_t index = 0; index < count && !error_; ++index) {
            auto entry = read();
            if constexpr (Own)
                result.push_back(std::move(entry));
        }
        return result;
    }

    Field<RateTimeEvidence> time() {
        return {optional<std::string>([&] { return field(limits.raw_time_bytes); }),
                optional_instant()};
    }

    Field<FetchedRateObservationInput> observation() {
        return {Field<RateObservationContext>{ref(), ref(), ref(), ref(), ref(),
                                              optional<std::string>([&] { return ref(); }),
                                              optional_instant(), time(), time()},
                list<RateQuoteInput>(Collection::quotes, [&] {
                    return Field<RateQuoteInput>{currency(), currency(), decimal()};
                })};
    }

    Field<ManualRateOverrideInput> manual() {
        return {ref(),
                tag(true) ? RateOverrideKind::pinned : RateOverrideKind::temporary,
                currency(),
                currency(),
                decimal(),
                ref(),
                optional<std::string>([&] { return ref(); }),
                field(limits.reason_bytes),
                optional_instant()};
    }

    Field<RateConstraint> constraint() {
        if (tag(true))
            return Field<ManualRateConstraint>{ref()};
        return Field<FetchedRateConstraint>{ref(), scalar64()};
    }

    Field<RateComponentInput> component() {
        return {list<RatePotentialInput>(Collection::members, [&] {
            return Field<RatePotentialInput>{currency(), field(potential_bytes),
                                             field(potential_bytes)};
        })};
    }

    std::string_view remaining_;
    std::optional<Error> error_;
    Budget budget_;
};

// A size-only pass and the output pass use exactly the same grammar. Only the
// first pass measures parsed decimal expansions, which are not stored in RSV1.
class Writer {
  public:
    explicit Writer(std::string *output = nullptr) : output_(output) {}

    std::size_t model(const EffectiveRateSnapshotData &snapshot) {
        append(magic);
        ref(snapshot.snapshot_ref);
        optional_ref(snapshot.parent_snapshot_ref);
        optional_instant(snapshot.constructed_at_utc_ms);
        list(snapshot.sources, Collection::sources,
             [&](const auto &source) { observation(source.data()); });
        list(snapshot.overrides, Collection::overrides, [&](const auto &entry) { manual(entry); });
        list(snapshot.constraints, Collection::constraints,
             [&](const auto &entry) { constraint(entry); });
        list(snapshot.components, Collection::components,
             [&](const auto &entry) { component(entry); });
        return framed_;
    }

  private:
    void frame(std::size_t size) {
        if (!charge(framed_, size, document_bytes))
            fail(Error::resource_limit);
    }

    void append(std::string_view bytes) {
        frame(bytes.size());
        if (output_)
            output_->append(bytes);
    }

    void fixed(std::uint64_t value, unsigned width) {
        frame(width);
        if (output_)
            for (unsigned index = width; index > 0; --index)
                output_->push_back(static_cast<char>((value >> (8 * (index - 1))) & 0xff));
    }

    void raw(std::size_t size) {
        if (!budget_.raw(size) || !budget_.parsed(size))
            fail(Error::resource_limit);
    }

    void field(std::string_view bytes, std::size_t ceiling) {
        if (bytes.size() > ceiling)
            fail(Error::resource_limit);
        raw(bytes.size());
        fixed(bytes.size(), 4);
        append(bytes);
    }

    void ref(std::string_view bytes) { field(bytes, limits.reference_bytes); }
    void currency(std::string_view bytes) { field(bytes, limits.currency_bytes); }

    template <class T, class Write> void optional(const std::optional<T> &value, Write write) {
        fixed(value.has_value(), 1);
        if (value)
            write(*value);
    }

    void optional_ref(const std::optional<std::string> &value) {
        optional(value, [&](const auto &present) { ref(present); });
    }

    void scalar64(std::uint64_t value) {
        raw(8);
        fixed(value, 8);
    }

    void optional_instant(const std::optional<std::int64_t> &value) {
        optional(value, [&](auto present) { scalar64(std::bit_cast<std::uint64_t>(present)); });
    }

    void tag(bool value) {
        raw(1);
        fixed(value, 1);
    }

    template <class T, class Write>
    void list(const std::vector<T> &entries, Collection kind, Write write) {
        if (!budget_.collection<T>(entries.size(), kind))
            fail(Error::resource_limit);
        fixed(entries.size(), 4);
        for (const auto &entry : entries)
            write(entry);
    }

    void decimal(const ExactDecimal &value) {
        field(value.original(), maximum_decimal_bytes);
        if (!output_ &&
            (!budget_.parsed(boost::multiprecision::numerator(value.value()).str().size()) ||
             !budget_.parsed(boost::multiprecision::denominator(value.value()).str().size())))
            fail(Error::resource_limit);
    }

    void time(const RateTimeEvidence &value) {
        optional(value.original_text,
                 [&](const auto &present) { field(present, limits.raw_time_bytes); });
        optional_instant(value.utc_ms);
    }

    void observation(const FetchedRateObservationData &source) {
        const auto &context = source.context;
        ref(context.observation_ref);
        ref(context.provider_ref);
        ref(context.configuration_ref);
        ref(context.request_ref);
        ref(context.source_evidence_ref);
        optional_ref(context.provider_snapshot_ref);
        optional_instant(context.fetched_at_utc_ms);
        time(context.source_rate_time);
        time(context.publication_time);
        list(source.quotes, Collection::quotes, [&](const auto &quote) {
            currency(quote.from_currency);
            currency(quote.to_currency);
            decimal(quote.rate);
        });
    }

    void manual(const ManualRateOverrideData &entry) {
        ref(entry.override_ref);
        tag(entry.kind == RateOverrideKind::pinned);
        currency(entry.from_currency);
        currency(entry.to_currency);
        decimal(entry.rate);
        ref(entry.decision_ref);
        optional_ref(entry.actor_ref);
        field(entry.reason, limits.reason_bytes);
        optional_instant(entry.recorded_at_utc_ms);
    }

    void constraint(const RateConstraint &entry) {
        if (const auto *fetched = std::get_if<FetchedRateConstraint>(&entry)) {
            tag(false);
            ref(fetched->observation_ref);
            scalar64(fetched->quote_ordinal);
        } else {
            tag(true);
            ref(std::get<ManualRateConstraint>(entry).override_ref);
        }
    }

    void component(const RateComponent &entry) {
        list(entry.members, Collection::members, [&](const auto &member) {
            currency(member.currency);
            field(boost::multiprecision::numerator(member.weight).str(), potential_bytes);
            field(boost::multiprecision::denominator(member.weight).str(), potential_bytes);
        });
    }

    std::string *output_;
    Budget budget_;
    std::size_t framed_{};
};

[[noreturn]] void translate(const RateSnapshotFailure &failure) {
    switch (failure.code()) {
    case RateSnapshotError::resource_limit:
        fail(Error::resource_limit);
    case RateSnapshotError::invalid_reference:
        fail(Error::invalid_reference);
    case RateSnapshotError::invalid_decimal:
    case RateSnapshotError::nonpositive_rate:
    case RateSnapshotError::invalid_potential:
        fail(Error::invalid_rate);
    case RateSnapshotError::contradictory_constraint:
        fail(Error::inconsistent_rate);
    case RateSnapshotError::invalid_currency:
    case RateSnapshotError::invalid_override:
    case RateSnapshotError::duplicate_identity:
    case RateSnapshotError::duplicate_quote:
    case RateSnapshotError::duplicate_constraint:
    case RateSnapshotError::unresolved_basis:
    case RateSnapshotError::noncanonical_component:
    case RateSnapshotError::unbacked_component:
        fail(Error::invalid_basis);
    case RateSnapshotError::invalid_options:
        break;
    }
    throw;
}
} // namespace

const std::string_view valuation_snapshot_format_ref = "subathon.internal.valuation-snapshot.v1";

ExactValuationFailure::ExactValuationFailure(ExactValuationError code)
    : std::invalid_argument(failure_message()), code_(code) {}

ExactValuationError ExactValuationFailure::code() const noexcept { return code_; }

ExactValuationDocument encode_valuation_snapshot(const EffectiveRateSnapshot &snapshot) {
    try {
        const auto size = Writer().model(snapshot.data());
        ExactValuationDocument output{std::string(valuation_snapshot_format_ref), {}};
        output.document.reserve(size);
        Writer(&output.document).model(snapshot.data());
        return output;
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    }
}

EffectiveRateSnapshot decode_valuation_snapshot(std::string_view format_ref,
                                                std::string_view document) {
    if (format_ref.size() > format_bytes || document.size() > document_bytes)
        fail(Error::resource_limit);
    if (format_ref != valuation_snapshot_format_ref)
        fail(Error::unsupported_format);
    Reader<false> scan(document);
    scan.model();
    if (const auto error = scan.error())
        fail(*error);

    try {
        Reader<true> reader(document);
        const auto input = reader.model();
        if (const auto error = reader.error())
            fail(*error);
        return detail::make_stored_rate_snapshot(input);
    } catch (const RateSnapshotFailure &failure) {
        translate(failure);
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    }
}
} // namespace subathon::core
