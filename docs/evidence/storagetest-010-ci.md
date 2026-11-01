# STORAGETEST-010 CI assessment

Coordinator `/root`, 2026-11-01. This baseline adds a compiled, separately reviewed
transaction-work boundary before STORAGE-010 implements it. It preserves the old
25 writer assertions and their ordinary automatic acceptance.

The selected `sqlite-work-debug` preset enables the old writer suite, the new work
suite and the Qt environment probe. The existing `sqlite-writer-build` job retains
its stable name and selected official Qt6.12/ICU archives, hashes and action pin.
Its proposed extension builds both boundaries, checks the Qt event loop, runs the
stable 25 cases, and checks complete new-case discovery against the compiled Qt
`-functions` output with nonempty/name/count equality. Registration is distinct
from running the new behavior.

The new cases remain intentionally red until STORAGE-010. Ordinary PR/main checks
do not run known-red bodies or claim they pass. Manual `run_work_acceptance=true`
runs real work cases with normal failure propagation after independent review;
there is no WILL_FAIL, failure inversion or continue-on-error. STORAGE-010 must
promote the unchanged reviewed cases to automatic acceptance and observe actual
Ubuntu execution before merge. This is a tracked dependency gate, not deferred
coverage of implemented behavior.

Root actionlint1.7.12 on the proposed workflow passed. Compilation, exact case
registration, independent harness review and actual changed Ubuntu workflow
execution remain pending at this draft stage. No new transaction behavior,
provider capability, release platform, hard execution deadline or domain schema
has been verified. Existing ruleset configuration does not enforce status checks;
all applicable latest-head checks remain written merge requirements.
