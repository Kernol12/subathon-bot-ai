# SPDX-License-Identifier: GPL-3.0-or-later
include_guard(GLOBAL)

# Resolve the real acceptance selection used by target creation. Read the common
# SUBATHON_BUILD_ACCEPTANCE_TESTS gate and optional SUBATHON_ACCEPTANCE_SUITES list;
# return sorted, unique canonical suite names through output. OFF selects none;
# bare ON selects time-credit. Explicit empty/unknown selections and any enabled
# retired per-feature cache option fail with an actionable diagnostic. The named
# suite closures are the existing preset contract in TESTCLEAN-003's proposal.
function(subathon_resolve_acceptance_suites output)
    message(FATAL_ERROR "FeatureNotImplemented: acceptance suite selection")
endfunction()
