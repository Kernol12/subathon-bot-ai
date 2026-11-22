// SPDX-License-Identifier: GPL-3.0-or-later
#include "modifier_capture_codec.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <set>

namespace subathon::server::modifier_capture_detail {
namespace {
using namespace core;
using Refusal = ModifierCaptureRefusal;
constexpr std::string_view magic = "MCA1";
constexpr ModifierStackLimits owner_limits{};

void check(bool condition, Refusal refusal = Refusal::invalid) {
    if (!condition)
        throw refusal;
}

void debit(std::size_t &left, std::size_t amount) {
    check(amount <= left, Refusal::resource_limit);
    left -= amount;
}

class Codec {
  public:
    Codec(ModifierStackLimits &budget, std::size_t byte_limit)
        : limits(budget), ref_limit(std::min(budget.reference_bytes, owner_limits.reference_bytes)),
          value_limit(std::min(budget.value_bytes, owner_limits.value_bytes)),
          entries(std::min(budget.policy_entries, owner_limits.policy_entries)), left(byte_limit) {}

    void count_entries(std::size_t size) { debit(entries, size); }

    void text_shape(std::string_view value, bool required, bool reason) const {
        check(!required || !value.empty());
        if (reason)
            check(QByteArrayView(value.data(), qsizetype(value.size())).isValidUtf8());
    }

    ModifierStackLimits &limits;
    const std::size_t ref_limit;
    const std::size_t value_limit;

  protected:
    std::size_t entries;
    std::size_t left;
};

class Writer : public Codec {
  public:
    Writer(ModifierStackLimits &budget, std::size_t byte_limit, QByteArray *destination = nullptr)
        : Codec(budget, byte_limit), output(destination) {}

    std::size_t size() const { return size_; }

    void raw(std::string_view bytes) {
        debit(left, bytes.size());
        size_ += bytes.size();
        if (output && !bytes.empty())
            output->append(bytes.data(), qsizetype(bytes.size()));
    }

    void number(std::uint64_t value, std::size_t width) {
        debit(limits.operations, 1);
        char bytes[8];
        for (std::size_t i = width; i > 0; --i) {
            bytes[i - 1] = static_cast<char>(value & 0xff);
            value >>= 8;
        }
        raw(std::string_view(bytes, width));
    }

    void text(std::string_view value, bool required = true, bool reason = false) {
        check(value.size() <= (reason ? value_limit : ref_limit), Refusal::resource_limit);
        text_shape(value, required, reason);
        number(value.size(), 4);
        raw(value);
    }

    template <class T, class F> void optional(const std::optional<T> &value, F field) {
        number(value.has_value(), 1);
        if (value)
            field(*value);
    }

    template <class T, class F>
    void list(const std::vector<T> &values, std::size_t max, std::size_t, bool policy, F field) {
        check(values.size() <= max, Refusal::resource_limit);
        if (policy)
            count_entries(values.size());
        number(values.size(), 4);
        for (const auto &entry : values)
            field(entry);
    }

  private:
    QByteArray *output;
    std::size_t size_{};
};

class Reader : public Codec {
  public:
    Reader(ModifierStackLimits &budget, std::string_view source)
        : Codec(budget, source.size()), input(source) {}

    std::string_view raw(std::size_t size) {
        check(size <= left);
        auto bytes = input.substr(input.size() - left, size);
        left -= size;
        return bytes;
    }

    std::uint64_t number(std::size_t width) {
        debit(limits.operations, 1);
        std::uint64_t value = 0;
        for (unsigned char byte : raw(width))
            value = (value << 8) | byte;
        return value;
    }

    void text(std::string &value, bool required = true, bool reason = false) {
        const auto size = number(4);
        check(size <= (reason ? value_limit : ref_limit), Refusal::resource_limit);
        const auto bytes = raw(std::size_t(size));
        text_shape(bytes, required, reason);
        value.assign(bytes);
    }

    template <class T, class F> void optional(std::optional<T> &value, F field) {
        const auto marker = number(1);
        check(marker <= 1);
        if (marker == 1)
            field(value.emplace());
    }

    template <class T, class F>
    void list(std::vector<T> &values, std::size_t max, std::size_t minimum_entry, bool policy,
              F field) {
        const auto count = number(4);
        check(count <= max, Refusal::resource_limit);
        if (policy)
            count_entries(std::size_t(count));
        check(count <= left / minimum_entry);
        check(count <= limits.operations, Refusal::resource_limit);
        values.reserve(std::size_t(count));
        for (std::uint64_t i = 0; i < count; ++i)
            field(values.emplace_back());
    }

    void finish() const { check(left == 0); }

  private:
    std::string_view input;
};

template <class T> void number(Writer &io, const T &value, std::size_t width = 8) {
    io.number(static_cast<std::uint64_t>(value), width);
}

template <class T> void number(Reader &io, T &value, std::size_t width = 8) {
    const auto raw = io.number(width);
    if constexpr (std::is_same_v<T, std::int64_t>)
        value = std::bit_cast<std::int64_t>(raw);
    else {
        check(raw <= std::numeric_limits<T>::max());
        value = static_cast<T>(raw);
    }
}

template <class IO, class T> void code(IO &io, T &value, int max) {
    if constexpr (std::is_same_v<IO, Reader>)
        value = static_cast<T>(io.number(1));
    check(int(value) >= 0 && int(value) <= max);
    if constexpr (std::is_same_v<IO, Writer>)
        io.number(std::uint64_t(value), 1);
}

template <class IO, class Key, class Config, class Identity, class Arrival, class Event>
void envelope(IO &io, Key &key, Config &config, Identity &identity, Arrival &arrival,
              Event &event) {
    io.text(key);
    io.text(config);
    io.text(arrival.modifier_context_ref);
    io.text(event.event_ref);
    io.text(event.root_ref);
    io.optional(event.parent_ref, [&](auto &ref) { io.text(ref); });
    io.text(identity.origin.provider);
    io.text(identity.origin.source_account);
    io.text(identity.identifier_namespace);
    io.text(identity.opaque_id);
    number(io, arrival.server_timestamp_utc_ms);
    io.text(arrival.connection_ref, false);
}

template <class IO, class R, class C, class P, class N, class D>
void decision_fields(IO &io, R &receipt, C &cid, P &plan, N &revision, D &decision) {
    number(io, receipt.value);
    number(io, cid.value);
    io.text(plan);
    number(io, revision);
    io.text(decision.decision_ref);
    io.text(decision.actor_ref);
    io.text(decision.reason, true, true);
    check(receipt.value != 0 && revision > 1);
}

template <class IO, class P> void priority(IO &io, P &value) {
    io.optional(value, [&](auto &p) {
        number(io, p.rank);
        number(io, p.order);
    });
}

template <class IO, class Profile, class Policy, class Ref, class Utc, class Turn, class Qualifiers,
          class Eligible>
void evidence(IO &io, Ref &observation, Utc &utc, Turn &turn, Profile &profile, Policy &policy,
              Qualifiers &qualifiers, Eligible &eligible) {
    io.text(observation);
    number(io, utc);
    number(io, turn);
    io.text(profile.producer_ref);
    io.text(profile.profile_ref);
    io.text(policy.policy_ref);
    io.list(qualifiers, std::min(io.limits.policy_entries, owner_limits.policy_entries), 5, false,
            [&](auto &ref) { io.text(ref); });
    const auto less = [&](std::string_view left, std::string_view right) {
        debit(io.limits.operations, 1);
        return left < right;
    };
    std::set<std::string_view, decltype(less)> instances(less), revisions(less);
    io.list(eligible, std::min(io.limits.occurrences, owner_limits.occurrences), 23, false,
            [&](auto &entry) {
                io.text(entry.key.instance_ref);
                io.text(entry.key.revision_ref);
                io.text(entry.definition_revision_ref);
                if constexpr (std::is_same_v<IO, Reader>) {
                    const auto value = io.number(8);
                    check(value <= std::uint64_t(std::numeric_limits<std::int64_t>::max()));
                    entry.remaining_ms = std::int64_t(value);
                } else {
                    number(io, entry.remaining_ms);
                }
                check(entry.remaining_ms > 0);
                check(instances.insert(entry.key.instance_ref).second &&
                      revisions.insert(entry.key.revision_ref).second);
            });
    io.list(policy.targets, std::min(io.limits.targets, owner_limits.targets), 11, true,
            [&](auto &target) {
                code(io, target.target.kind, 2);
                io.text(target.target.ref);
                io.optional(target.operation, [&](auto &op) { code(io, op, 4); });
                io.list(target.groups, owner_limits.policy_entries, 11, true, [&](auto &group) {
                    io.text(group.group_ref);
                    io.optional(group.operation, [&](auto &op) { code(io, op, 4); });
                    priority(io, group.priority);
                    io.list(group.members, owner_limits.policy_entries, 24, true, [&](auto &m) {
                        auto &key = m.occurrence;
                        io.text(key.instance_ref);
                        io.text(key.instance_revision_ref);
                        io.text(key.definition_revision_ref);
                        number(io, key.target_ordinal);
                        check(key.target_ordinal > 0);
                        priority(io, m.priority);
                    });
                });
            });
}
} // namespace

RetainedDocument encode(const ReceiptIdentity &identity, const ReceiptArrival &arrival,
                        const IssuedModifierEvent &event, const QualifiedModifierCut *cut,
                        const PlanEvent *original, const ApplicationPlanKey *successor,
                        const PlanOverrideDecision *decision, ModifierStackLimits &limits) {
    const auto &key = decision ? decision->decision_ref : arrival.modifier_context_ref;
    const auto &config = decision ? cut->policy.configuration_ref : arrival.configuration_ref;
    const auto max_size = std::min(
        {limits.input_bytes, limits.output_bytes, limits.value_bytes, owner_limits.value_bytes});
    const auto write = [&](Writer &io) {
        io.raw(magic);
        io.number(decision != nullptr, 1);
        envelope(io, key, config, identity, arrival, event);
        if (decision) {
            check(key != arrival.modifier_context_ref);
            decision_fields(io, original->receipt_id, *original->contribution_id,
                            successor->plan_ref, successor->revision, *decision);
        }
        io.number(cut == nullptr, 1);
        if (cut)
            evidence(io, cut->observation.ref, cut->observation.utc_ms, cut->turn, cut->profile,
                     cut->policy, cut->matched_qualifiers, cut->eligible);
    };
    // Validate and meter the whole write, including the emission pass, before
    // allocating the retained payload or copying its metadata.
    Writer preflight(limits, max_size);
    const auto operations_before = limits.operations;
    write(preflight);
    debit(limits.operations, operations_before - limits.operations);
    auto emitted_limits = limits;
    emitted_limits.operations = operations_before;
    debit(limits.input_bytes, preflight.size());
    debit(limits.output_bytes, preflight.size());
    for (const auto *ref : {&key, &config})
        debit(limits.output_bytes, ref->size());

    QByteArray bytes;
    bytes.reserve(qsizetype(preflight.size()));
    Writer output(emitted_limits, preflight.size(), &bytes);
    write(output);
    return {{RetainedKind::arrival_context, key}, std::move(bytes), ConfigurationReference{config}};
}

DecodedCapture decode(const RetainedDocument &document, bool expect_recalculation,
                      ModifierStackLimits &limits) {
    if (!document.document.startsWith(QByteArrayView(magic.data(), qsizetype(magic.size()))))
        throw Refusal::unresolved;
    const auto bytes =
        std::string_view(document.document.constData(), std::size_t(document.document.size()));
    check(bytes.size() <= std::min(limits.value_bytes, owner_limits.value_bytes),
          Refusal::resource_limit);
    debit(limits.input_bytes, bytes.size());
    debit(limits.output_bytes, bytes.size());
    Reader io(limits, bytes);
    (void)io.raw(magic.size());
    check(io.number(1) == unsigned(expect_recalculation));

    DecodedCapture decoded{};
    std::string key, config;
    envelope(io, key, config, decoded.identity, decoded.arrival, decoded.event);
    check(document.key.kind == RetainedKind::arrival_context && document.key.reference == key &&
          document.configuration && document.configuration->reference == config);
    if (expect_recalculation) {
        auto &binding = decoded.decision.emplace();
        auto &mode = binding.mode;
        mode.decision.authority = OverrideAuthority::authorized;
        decision_fields(io, binding.receipt, binding.contribution, mode.plan_ref, mode.revision,
                        mode.decision);
        check(key == mode.decision.decision_ref && key != decoded.arrival.modifier_context_ref);
    } else {
        check(key == decoded.arrival.modifier_context_ref);
        decoded.arrival.configuration_ref = config;
    }

    const auto unresolved = io.number(1);
    check(unresolved <= 1 && (!expect_recalculation || unresolved == 0));
    if (!unresolved) {
        auto &cut = decoded.cut.emplace();
        cut.policy.configuration_ref = std::move(config);
        evidence(io, cut.observation_ref, cut.utc, cut.turn, cut.profile, cut.policy,
                 cut.matched_qualifiers, cut.eligible);
        check(expect_recalculation || cut.utc == decoded.arrival.server_timestamp_utc_ms);
    }
    io.finish();
    return decoded;
}

} // namespace subathon::server::modifier_capture_detail
