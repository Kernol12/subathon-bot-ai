# STORAGE-002 — independent terminal-session oracle review

Author of proposed correction: root coordinator. Reviewer: bundle_reviewer, separate
agent/worktree, 2026-11-03. Verdict: **approved for the exact baseline correction**.
This approves no runtime source, new byte-boundary test or full task integration.

Actual reviewed root diff changes ONE call in
`borrowedStoreLifetimeAndCapabilityErrorsStayTruthful`:
`clean_rollback(r)` → `clean_rollback(r, WorkError::session_closed)`.
Original frozen test blob4b60fcaa06dec8136f0b81e7ac6d01374df94a48 becomes root
proposal0024778 (full hash to be recorded by integrator). Every other test assertion,
fixture, public port and product source remains intact in this correction.

The [writer revision1](../contracts/writer-transactions-v1.md) section "Errors are
sticky within a live owner session" expressly preserves the first framework error
even if the body catches its exception. Its completion matrix separately preserves
transaction outcome versus work error. The test explicitly performs real rollback,
then invokes read_state through the terminal session and asserts synchronous
session_closed. Therefore its completion must be rolled_back/session_closed with
no normal payload; not_committed only applies when no framework failure occurred.

Reviewed actual unchanged writer implementation: execute→require_active detects
terminal phase; fail(session_closed) records first_error through remember; finish
selects first_error before its default not_committed branch. Foreign-thread/stale
calls do not poison the owner's transaction. Existing independently reviewed writer
case `retainedCapabilitiesCannotReachLaterWork` checks the analogous committed/
session_closed pair and rejects effects on a later work item. That actual method
name corrects the proposed author's initial stale spelling.

The one-line correction strengthens truthful outcome attribution, preserving all
wrong-thread, pure-invalid-key, terminal, later-stale, physical abandonment and
no-payload assertions. It neither relaxes rollback nor changes writer behavior.
Historical original32 red90fa/SHA1e677e remains intact; it is not repeated/relabelled.

Actual reviewer validation in isolated storage002-review:

- Imported actual helper824f58d and runtimef969ba6; copied ONLY the reviewed root
  terminal assertion locally. Own separately authored byte test291 is present but
  was NOT executed by this focused check; root separately reviews that new test.
- cmake --preset sqlite-bundle-debug; exit0. Fresh full build exit0; follow-up
  cmake --build --preset sqlite-bundle-debug after local assertion change exit0,
  no compiler warning/error in either actual log.
- Initial CTest regex incorrectly used acceptance.sqlite_bundle prefix and selected
  zero cases; this was NOT counted as validation. Corrected exact command:
  `ctest --preset sqlite-bundle-debug -R '^sqlite-bundle.borrowedStoreLifetimeAndCapabilityErrorsStayTruthful$' --output-on-failure`
  **1/1 PASS,0.04s CTest**, actual exit0. No case skipped or inverted.

Raw local diagnostics: /tmp/storage002-review-configure.txt,
/tmp/storage002-review-build.txt,
/tmp/storage002-review-build-corrected-baseline.txt,
/tmp/storage002-review-terminal-correct-selection.txt. These paths carry synthetic
local diagnostics only. Full source/config review and unchanged32+additive suite/
full preset/selected official latest-head CI remain separate required gates.
