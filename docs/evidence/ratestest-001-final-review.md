# RATESTEST-001 independent final metadata review

**APPROVED — actual baseline metadata at
`0968be26e4eb6195c285d9f7c7a0e704a63197c1` only.**
Reviewer `service_reviewer`, different from metadata author/coordinator `/root`,
2026-11-04. Isolated `task/RATESTEST-001-final-review`; only this note changes.
No new requirement, owner decision or access action is needed.

| Actual reviewed artifact | Git blob |
| --- | --- |
| [Task state](../../progress/state.json) | 6584ef54cf275338e91ea3dadf45df8bbb882163 |
| [Acceptance map](../../tests/acceptance-map.json) | d20eafdc80a7b4bdd0417c568a28ff91af8b6d01 |
| [Progress snapshot](../../PROGRESS.html) | f9b96a843b8eace0f88c12b4d54dce3a38770a78 |
| [Rate handoff](ratestest-001-handoff.md) | cc2fd2f93cae95d9a55474eba55c3f7d1865edba |
| [Coordinator checkpoint](coordinator-handoff.md) | 6148b92be5e4ba8687e227ead0cdf125bd312096 |
| [Selected CI record](ratestest-001-ci.md) | 6d5c91f1b888c9b89abce12efb94aaea7812a7e5 |

RATESTEST-001 is correctly done as a test baseline: independently reviewed and
observed red, implementation not_run. RATES-001 and both valuation-bridge tasks
remain planned. NUM-04 remains partial. Its old test/evidence lists and entire
primitive/storage/service scope text are preserved as exact prefixes; only new
rate-baseline evidence is appended. All other map records/scenarios are unchanged.
No passing rate, current-loader, valuation, provider or platform behavior is claimed.

The graph retains 408 IDs / 1508 edges and every main406 ID / 1497 edge, including
completed password/permission statuses. The only additional identities are required
RATESTEST-009/RATES-009 with their bridge/EPIC-017 dependencies. Relative to selected
`bf5986d`, graph and all other task records are unchanged; only RATESTEST-001's record
is completed. Relative to main `fc28558`, no other task status changes. The checkpoint
accurately preserves PR52's separate active timer test branch, PR56's selected-host
25-fact proof and remaining integration/CI/FIRST-red gates, and parked asset work.
It explicitly requires inspecting agents/processes on resume rather than assuming
an old assignment is still running.

Handoff and checkpoint correctly link unchanged full-test approval `dcf5dcaa`,
root integration/harness approval `7ae17c55`, selected host 1.83 proof and Ubuntu
`41897596827` at `bf5986d5ae418fb2c921b147b18927aa3f3ae996`: exact 53 discovery,
SDK 14 facts and old 36 pass. The recorded 15 successful checks apply to that
published head, including the unchanged-workflow failed-job-only SQLite apt-403
retry. They do not establish final metadata HEAD CI; the documents retain fresh
latest-head checks, PR readiness, actual rules/reviews/discussions and normal merge
as later gates.

The previously approved FIRST attribution remains exact blob
`f71ccdf149e34dfe0b13eee39820c444090ffcce` (review `d6c06b42`, import `481cadf`).
The metadata preserves native 53 failed / 0 skipped, 364 marker failure events,
141 traced call sites and the **unknown direct CTest exit status**. Wrapper exit 1
is a separate UTF-8 collector defect. No replay, fictitious exit 8 or claim that
later assertions after throwing calls ran is introduced. This review does not
repeat that settled raw/source/SDK/test audit.

Read-only Git/JSON comparisons confirm the complete `bf5986d` to candidate diff is
only evidence/state/map/generated HTML; source, tests, fixtures, contracts, spec,
build, label scripts and workflows are unchanged. `python3 tools/check_project.py`
(408 tasks / 122 decisions), `python3 tools/progress.py render --check` and
`git diff --check` pass. No native/build/test/SDK/foundation/red body ran here.

Approval is limited to this actual metadata revision. Production RATES-001,
RATESTEST-009/RATES-009 valuation work, all three FX providers and remaining release
scope stay required. Future CI, implementation, PR readiness, rules/discussion
resolution and merge outcomes are not approved prospectively.
