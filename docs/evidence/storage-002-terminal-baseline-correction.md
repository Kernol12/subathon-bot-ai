# STORAGE-002 — terminal-session completion oracle correction

Author: root coordinator, 2026-11-03. Status: independently APPROVED by bundle_reviewer at a24141ab99724de6a15f5c997330df95283fe30d.
Specification v0.45 §3.4/§16 DB-01 requires honest controlled transaction outcomes;
concrete bundle revision1 preserves original capability/protocol WorkFailure.

The original reviewed testCPP4b60fcaa, method
borrowedStoreLifetimeAndCapabilityErrorsStayTruthful, explicitly rolls back then
calls read_state on the now-terminal ticket and catches session_closed. Its final
clean_rollback(r) default expects not_committed, which describes a healthy explicit
rollback with NO framework failure. That expectation contradicts the earlier
[writer revision1](../contracts/writer-transactions-v1.md) completion matrix row
"Illegal second begin/terminal-session use in same body" and first-framework-error
precedence. Catching the exception does not erase that work's recorded error.

Existing unchanged writer source require_active→fail(session_closed)→remember→finish
preserves first_error. Existing independently reviewed writer acceptance method
retainedCapabilitiesCannotReachLaterWork likewise
asserts a terminal call produces completion session_closed while preserving its
physical committed fact. This is not a new writer behavior or a bundle workaround.

Proposed correction changes ONLY this final call to
clean_rollback(r, WorkError::session_closed). All synchronous wrong-thread/invalid-key/
terminal/stale assertions, physical rolled_back outcome, absent success payload,
no storage/input/synchronous exception and later stale-capability check remain.
No fixture or product source changes; no skip/delete/weaker cleanup claim.
The first abandonment branch still explicitly expects WorkError::uncommitted.

Actual implementer development run of compiling real source reached this assertion:
29/32 initially passed, with two separate implementation errors then corrected.
Its original output remains /tmp/storage002-test1.txt in this session. This note
makes no independent rerun/pass claim; actual reviewer acceptance and the revised
baseline blob/validation results must be appended before completion.
Original reviewed32 red90fa/raw1e677e remains unchanged historical evidence; it is
not rerun or reattributed to the corrected assertion.

Actual proposed revised CPP blob: `0024778edf866770d044d10c2bdd0678b2c95b65`
from original `4b60fcaa06dec8136f0b81e7ac6d01374df94a48`.

Automatic approval review rejected an attempted pre-review commit. Root left the
proposal uncommitted and requested actual independent review before retrying.

Separate [actual review](storage-002-terminal-baseline-review.md) accepts EXACT
one-call correction0024778, after fresh warning-free build and focused corrected
case1/1PASS0.04s. Initial zero-match regex was NOT counted. Review is now complete
before this correction commit; original automatic-review objection is resolved
through the required safer independent-review gate, without owner override.
