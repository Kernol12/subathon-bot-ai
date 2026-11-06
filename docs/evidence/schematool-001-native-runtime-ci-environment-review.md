# SCHEMATOOL-001 — runtime CI environment correction review

Reviewer C, 6 November 2026, fresh isolated review from actual root
`bf45ae9990d36b9e8d2f0dcbd2086ab1502309ba`. ONLY this note changes.

**APPROVED: exact configuration correction and conditional diagnostic PR77
publication after the separately released LOCAL56 actually passes.** This closes
the environment-continuity finding in C's c6be0cf binding attribution. Reuse B's
actual a230d857 review for the otherwise unchanged conditional workflow; actual
hosted results, owner baseline/red, implementation and merge remain separate.

Reviewed actual workflow blob `4f92fd23627c9f7bd2f59e6bb3327070156b9c3f` and
author-note blob `8edc4b8c9ef1d49af6648edfdecc657a8b687f1a`. The actual diff
only adds the exact three-key environment to inventory's check_output and wraps
final CTest in env -i with those same keys: PATH=/usr/bin:/bin, LC_ALL=C.UTF-8,
LANG=C.UTF-8. Bootstrap already uses that exact environment. Thus discovery and
every native registration inherit the admitted loader/test environment rather
than runner LD_/GLIBC_TUNABLES/GTEST_FILTER values. No test/body/name/count,
assertion, matrix, deadline, artifact or dependency changes.

The mandatory original/fault static gates inspect each host's actual artifacts
before candidate execution. Exact-one no-child XML binding then precedes strict
55-native-plus-loader discovery and unconditional all56 CTest. These concrete
reviewed automatic gates may replace a manual stop on each host: unknown code,
missing binding, disabled/inverted/missing registration and actual failed cases
remain failures. They do not turn local success into either hosted capability or
owner acceptance. BOTH actual selected-host foundation greens and independently
reviewed57 behavior tests still precede intended31 red/owner implementation.

Actual checks: complete diff/source/author-note read; four inline Python bodies
parsed by stdlib AST without execution; exact bootstrap/inventory environment
dictionaries checked; all12 literal/folded shell blocks bash-n PASS. Project426/
122, render--check, diff--check PASS. No workflow body, CMake/CTest, audit/control,
candidate native/DSO, import, SDK, thread, child or probe ran from this review.
