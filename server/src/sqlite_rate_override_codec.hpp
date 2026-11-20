// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <bit>
#include <subathon/core/valuation_snapshot_codec.hpp>
#include <subathon/server/sqlite_rate_overrides.hpp>

namespace subathon::server::rate_sql {
using Refusal = RateStoreRefusal;
using namespace std::string_view_literals;

inline constexpr auto intent_format = "subathon.internal.rate-override-intent.v1"sv;
inline constexpr auto decision_format = "subathon.internal.rate-override-decision.v1"sv;
inline constexpr auto observation_format = "subathon.internal.rate-observation.v1"sv;
inline constexpr std::size_t intent_bytes = 229376;
inline constexpr std::size_t document_bytes = 32768;
inline constexpr std::size_t occurrence_bytes = 5535;
inline constexpr std::size_t logical_bytes = 24576;
inline constexpr std::size_t observation_count = 16;
inline constexpr std::size_t edge_count = 16384;

inline void require(bool ok, Refusal refusal = Refusal::invalid_retained_data) {
    if (!ok)
        throw refusal;
}
inline void fit(bool ok) { require(ok, Refusal::resource_limit); }
inline void charge(std::size_t &total, std::size_t amount, std::size_t ceiling) {
    fit(amount <= ceiling - total);
    total += amount;
}
inline bool byte_less(std::string_view left, std::string_view right) {
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end(),
                                        [](unsigned char a, unsigned char b) { return a < b; });
}
inline QByteArray blob(std::string_view bytes) {
    return QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size()));
}
inline std::string_view view(const QByteArray &bytes) {
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

struct Intent {
    RateOverrideEdit edit;
    std::string policy;
    std::vector<std::string> observations;
    std::vector<RateSourceEdge> edges;
};
struct Audit {
    std::string installation, operation, decision, base, successor, introduced;
    std::optional<std::string> replaced;
    std::string account, session, record_revision, snapshot, catalog;
    std::uint64_t graph_revision{};
    std::string source, grant;
    std::optional<std::string> role;
    std::optional<std::int64_t> accepted_at;
};

// Each grammar runs first as a no-growth scan, then as an owning read or write.
// The ROB1 raw metric counts payload strings and actual instants. The core
// observation factory independently charges the additional parsed decimals.
class WireWriter {
  public:
    WireWriter(std::size_t ceiling, Refusal invalid, QByteArray *output = nullptr)
        : ceiling_(ceiling), invalid_(invalid), output_(output) {}
    void literal(std::string_view value) {
        charge(size_, value.size(), ceiling_);
        if (output_)
            output_->append(value.data(), static_cast<qsizetype>(value.size()));
    }
    void scalar(std::uint64_t number, unsigned width) {
        char bytes[8];
        for (unsigned i = 0; i < width; ++i)
            bytes[width - 1 - i] = static_cast<char>(number >> (8 * i));
        literal({bytes, width});
    }
    void bounded(std::string_view value, std::size_t max, bool nonempty = true) {
        fit(value.size() <= max);
        require(!nonempty || !value.empty(), invalid_);
        scalar(value.size(), 4);
        charge(raw_, value.size(), ceiling_);
        literal(value);
    }
    void optional_ref(const std::optional<std::string> &value, std::size_t max,
                      bool nonempty = true) {
        scalar(value.has_value(), 1);
        if (value)
            bounded(*value, max, nonempty);
    }
    void instant(const std::optional<std::int64_t> &value) {
        scalar(value.has_value(), 1);
        if (value) {
            charge(raw_, 8, ceiling_);
            scalar(std::bit_cast<std::uint64_t>(*value), 8);
        }
    }
    void kind(core::RateOverrideKind value) {
        require(value == core::RateOverrideKind::pinned ||
                    value == core::RateOverrideKind::temporary,
                invalid_);
        scalar(value == core::RateOverrideKind::pinned, 1);
    }
    template <class Range, class Emit> void list(const Range &values, std::size_t max, Emit visit) {
        fit(values.size() <= max);
        scalar(values.size(), 4);
        for (const auto &value : values)
            visit(value);
    }
    void witness(const Audit &value) {
        scalar(value.role.has_value(), 1);
        bounded(value.source, 4096);
        bounded(value.grant, 4096);
        if (value.role)
            bounded(*value.role, 4096);
    }
    void major(std::uint64_t value) { scalar(value, 8); }
    void edge(const RateSourceEdge &value) {
        scalar(value.observation_index, 4);
        scalar(value.quote_ordinal, 8);
    }
    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] std::size_t raw_size() const { return raw_; }
    void observation_budget() const { fit(raw_ <= logical_bytes); }

  private:
    std::size_t ceiling_, size_{}, raw_{};
    Refusal invalid_;
    QByteArray *output_;
};

class WireReader {
  public:
    WireReader(std::string_view bytes, bool own, std::size_t raw_ceiling)
        : bytes_(bytes), own_(own), raw_ceiling_(raw_ceiling) {}
    void literal(std::string_view magic) { require(take(magic.size()) == magic); }
    void bounded(std::string &value, std::size_t max, bool nonempty = true) {
        const auto size = scalar(4);
        fit(size <= max);
        charge(raw_, size, raw_ceiling_);
        const auto bytes = take(size);
        empty_required_ |= nonempty && bytes.empty();
        if (own_)
            value.assign(bytes);
    }
    void optional_ref(std::optional<std::string> &value, std::size_t max, bool nonempty = true) {
        if (tag()) {
            std::string entry;
            bounded(entry, max, nonempty);
            if (own_)
                value = std::move(entry);
        }
    }
    void instant(std::optional<std::int64_t> &value) {
        if (tag()) {
            charge(raw_, 8, raw_ceiling_);
            value = std::bit_cast<std::int64_t>(scalar(8));
        }
    }
    void kind(core::RateOverrideKind &value) {
        value = tag() ? core::RateOverrideKind::pinned : core::RateOverrideKind::temporary;
    }
    template <class T, class Read> void list(std::vector<T> &values, std::size_t max, Read read) {
        const auto count = scalar(4);
        fit(count <= max);
        if (own_)
            values.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            T entry{};
            read(entry);
            if (own_)
                values.push_back(std::move(entry));
        }
    }
    void witness(Audit &value) {
        const auto has_role = tag();
        bounded(value.source, 4096);
        bounded(value.grant, 4096);
        if (has_role) {
            std::string role;
            bounded(role, 4096);
            if (own_)
                value.role = std::move(role);
        }
    }
    void major(std::uint64_t &value) { value = scalar(8); }
    void edge(RateSourceEdge &value) {
        value.observation_index = static_cast<std::uint32_t>(scalar(4));
        value.quote_ordinal = scalar(8);
    }
    void finish() const { require(bytes_.empty() && !empty_required_); }

  private:
    std::string_view take(std::size_t count) {
        require(count <= bytes_.size());
        const auto result = bytes_.substr(0, count);
        bytes_.remove_prefix(count);
        return result;
    }
    std::uint64_t scalar(unsigned width) {
        std::uint64_t number = 0;
        for (unsigned char byte : take(width))
            number = (number << 8) | byte;
        return number;
    }
    bool tag() {
        const auto number = scalar(1);
        require(number <= 1);
        return number != 0;
    }
    std::string_view bytes_;
    bool own_, empty_required_{};
    std::size_t raw_ceiling_, raw_{};
};

inline constexpr auto override_fields = [](auto &wire, auto &entry) {
    wire.bounded(entry.override_ref, 256);
    wire.kind(entry.kind);
    wire.bounded(entry.from_currency, 64);
    wire.bounded(entry.to_currency, 64);
    if constexpr (requires { entry.original_decimal; })
        wire.bounded(entry.original_decimal, 512);
    else
        wire.bounded(entry.rate.original(), 512);
    wire.bounded(entry.decision_ref, 256);
    wire.optional_ref(entry.actor_ref, 256);
    wire.bounded(entry.reason, 4096, false);
    wire.instant(entry.recorded_at_utc_ms);
};
inline constexpr auto observation_fields = [](auto &wire, auto &source) {
    auto &context = source.context;
    wire.bounded(context.observation_ref, 256);
    wire.bounded(context.provider_ref, 256);
    wire.bounded(context.configuration_ref, 256);
    wire.bounded(context.request_ref, 256);
    wire.bounded(context.source_evidence_ref, 256);
    wire.optional_ref(context.provider_snapshot_ref, 256);
    wire.instant(context.fetched_at_utc_ms);
    const auto time = [&](auto &value) {
        wire.optional_ref(value.original_text, 512, false);
        wire.instant(value.utc_ms);
    };
    time(context.source_rate_time);
    time(context.publication_time);
    wire.list(source.quotes, 4096, [&](auto &quote) {
        wire.bounded(quote.from_currency, 64);
        wire.bounded(quote.to_currency, 64);
        if constexpr (requires { quote.original_decimal; })
            wire.bounded(quote.original_decimal, 512);
        else
            wire.bounded(quote.rate.original(), 512);
    });
};
inline constexpr auto edit_fields = [](auto &wire, auto &edit) {
    wire.bounded(edit.installation, 4096);
    wire.bounded(edit.operation, 256);
    wire.bounded(edit.base, 256);
    wire.bounded(edit.successor, 256);
    override_fields(wire, edit.override_input);
    wire.optional_ref(edit.replacement, 256);
};
inline constexpr auto intent_fields = [](auto &wire, auto &intent) {
    edit_fields(wire, intent.edit);
    wire.bounded(intent.policy, 256);
    wire.list(intent.observations, observation_count, [&](auto &ref) { wire.bounded(ref, 256); });
    wire.list(intent.edges, edge_count, [&](auto &edge) { wire.edge(edge); });
};
inline constexpr auto audit_fields = [](auto &wire, auto &audit) {
    wire.bounded(audit.installation, 4096);
    wire.bounded(audit.operation, 256);
    wire.bounded(audit.decision, 256);
    wire.bounded(audit.base, 256);
    wire.bounded(audit.successor, 256);
    wire.bounded(audit.introduced, 256);
    wire.optional_ref(audit.replaced, 256);
    wire.bounded(audit.account, 256);
    wire.bounded(audit.session, 4096);
    wire.bounded(audit.record_revision, 4096);
    wire.bounded(audit.snapshot, 4096);
    wire.bounded(audit.catalog, 256);
    wire.major(audit.graph_revision);
    wire.witness(audit);
    wire.instant(audit.accepted_at);
};

template <class T, class Fields>
QByteArray encode(std::string_view magic, std::size_t maximum, const T &value, Fields fields,
                  Refusal invalid) {
    WireWriter scan(maximum, invalid);
    scan.literal(magic);
    fields(scan, value);
    if (magic == "ROB1")
        scan.observation_budget();
    QByteArray bytes;
    bytes.reserve(static_cast<qsizetype>(scan.size()));
    WireWriter writer(maximum, invalid, &bytes);
    writer.literal(magic);
    fields(writer, value);
    return bytes;
}
template <class T, class Fields>
T decode(std::string_view magic, std::size_t maximum, const QByteArray &bytes, Fields fields) {
    fit(view(bytes).size() <= maximum);
    const auto raw_ceiling = magic == "ROB1" ? logical_bytes : maximum;
    T discarded{};
    WireReader scan(view(bytes), false, raw_ceiling);
    scan.literal(magic);
    fields(scan, discarded);
    scan.finish();
    T result{};
    WireReader reader(view(bytes), true, raw_ceiling);
    reader.literal(magic);
    fields(reader, result);
    reader.finish();
    return result;
}

inline constexpr core::RateSnapshotLimits storage_limits{.owned_bytes = logical_bytes};
} // namespace subathon::server::rate_sql
