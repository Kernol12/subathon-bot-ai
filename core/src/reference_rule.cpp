// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/reference_rule.hpp>
namespace subathon::core {
ReferenceRuleDerivation derive_reference_rule(const ReferenceRuleInput &,
                                              const ReferenceRuleLimits &) {
    throw ReferenceRuleNotImplemented{};
}
} // namespace subathon::core
