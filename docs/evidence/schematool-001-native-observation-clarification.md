# SCHEMATOOL-001 — final native observation meaning

Root,6 November2026. Proposed private clarification, pending DIFFERENT actual
document/type review before dependent execution. B identified the concrete gap
during nonvalidating fixture authoring; no owner implementation or negative test
body has been written/run against this proposed meaning.

Approved matrix25 requires actual native interrupted18 where observable, because
pending SIGINT can make ctypes raise instead of exposing the returned integer.
The existing journal already defines final-commit kind8, but calls cell5 only
"syscall result". The private header additionally described RawResult as raw
syscall-only while using that alias for every Observation.result. Treating kind8
as an outcome without clarification would turn an unspecified field into known
data, or encourage silently dropping the existing18 oracle.

Native contract revision2 explicitly gives the EXISTING scalar a kind-specific
meaning: syscall-return carries an actual kernel return; final-commit carries
the ACTUAL common owner's committed ABI integer AFTER all resource cleanup and
final decision, before C return/conversion. Its errno is unavailable-1. Other
undefined fields remain-1. This observation cannot invent/override the result,
replace independent FD/reap proof or serve as a successful injected validator.

Observation.result becomes plain std::int64_t with that comment. RawResult remains
syscall-only in the Ops interface. Both types have identical representation;
no struct layout, C ABI, four exports,128-record/12288-map limit, outcome enum,
resource ceiling, pending-signal behavior or public requirement changes. The
literal31 stub remains EXACT and emits no artificial final-commit event.

This resolves a private instrumentation ambiguity, preserving the matrix's actual
18 assertion and unknown-field rules. It is not permission for owner code or
tests to rely on an unreviewed future body. Actual different delta review, helper
source/foundations, negative tests/red and all remaining source/CI/merge gates
still apply. No owner decision or new dependency is needed. Finish PR77 then stop.
