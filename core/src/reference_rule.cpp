// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <new>
#include <string_view>
#include <subathon/core/reference_rule.hpp>

namespace subathon::core {
namespace {
using Error = ReferenceDerivationError;
[[noreturn]] void fail(Error e) { throw ReferenceDerivationFailure(e); }
void add(std::uint64_t &used, std::uint64_t n, std::uint64_t cap, Error e) {
    if (used > cap || n > cap - used)
        fail(e);
    used += n;
}
} // namespace
ReferenceRuleDerivation derive_reference_rule(const ReferenceRuleInput &v,
                                              const ReferenceRuleLimits &l) {
    const auto maximum = std::min({std::uint64_t(SIZE_MAX), std::uint64_t(PTRDIFF_MAX),
                                   std::uint64_t(std::string{}.max_size())});
    for (auto n : {l.reference_bytes, l.value_owned_bytes, l.value_items})
        if (!n || n >= maximum)
            fail(Error::invalid_limits);
    const std::array<std::string_view, 9> fields{
        v.rule_ref,     v.rule_version_ref, v.action_ref,  v.action_version_ref, v.timer_ref,
        v.trigger_type, v.observed_type,    v.action_kind, v.action_unit};
    std::uint64_t bytes = 0;
    for (auto s : fields) {
        if (s.size() > l.reference_bytes)
            fail(Error::input_limit);
        add(bytes, s.size(), l.value_owned_bytes, Error::input_limit);
    }
    add(bytes, 9, l.value_owned_bytes, Error::input_limit);
    for (auto s : fields)
        if (s.empty())
            fail(Error::invalid_reference);
    if (v.add_ms < 0)
        fail(Error::invalid_value);
    auto state = ReferenceDerivationState::matched;
    std::string_view explanation = "reference.single-rule.match.v1";
    if (v.trigger_type != "synthetic.reference.support.v1" || v.action_kind != "time_add" ||
        v.action_unit != "ms") {
        state = ReferenceDerivationState::unsupported;
        explanation = "reference.single-rule.unsupported.v1";
    } else if (!v.enabled) {
        state = ReferenceDerivationState::disabled;
        explanation = "reference.single-rule.disabled.v1";
    } else if (v.observed_type != v.trigger_type) {
        state = ReferenceDerivationState::nonmatch;
        explanation = "reference.single-rule.nonmatch.v1";
    }
    // Project the complete result before even constructing its default Exact.
    bytes = 0;
    auto output = [&](std::string_view s) {
        add(bytes, s.size(), l.value_owned_bytes, Error::result_limit);
    };
    output(v.rule_ref);
    output(v.rule_version_ref);
    output(explanation);
    add(bytes, 3, l.value_owned_bytes, Error::result_limit); // state, selected, optional
    if (state == ReferenceDerivationState::matched) {
        std::array<char, 20> decimal{};
        const auto end = std::to_chars(decimal.data(), decimal.data() + decimal.size(), v.add_ms);
        output({decimal.data(), std::size_t(end.ptr - decimal.data())});
        output("1");
        output(v.action_ref);
        output(v.action_version_ref);
        output(v.rule_ref);
        output(v.timer_ref);
        output("reference.integer-ms.v1");
        output("reference.original-time.v1");
        add(bytes, 10, l.value_owned_bytes, Error::result_limit); // unit, history, i64
    }
    try {
        ReferenceRuleDerivation out{state,
                                    {v.rule_ref, v.rule_version_ref,
                                     state == ReferenceDerivationState::matched,
                                     std::string(explanation)},
                                    std::nullopt};
        if (state == ReferenceDerivationState::matched)
            out.request.emplace(
                RequestedEffect{v.action_ref, v.action_version_ref, v.rule_ref, v.timer_ref,
                                RequestedUnit::milliseconds, false, Exact{v.add_ms}, v.add_ms,
                                "reference.integer-ms.v1", "reference.original-time.v1"});
        return out;
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    } catch (const std::length_error &) {
        fail(Error::allocation_failure);
    }
}
} // namespace subathon::core
