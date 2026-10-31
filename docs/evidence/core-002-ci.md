# CORE-002 observed automatic timer CI

Coordinator `/root`, 2026-10-31. New `.github/workflows/timer-lifecycle.yml` runs
automatically on main PRs/pushes and manual dispatch, using the existing
`timer-lifecycle-debug` configure/build/CTest lane. Checkout and the selected
GoogleTest fetch remain pinned; permissions are contents:read, timeout15min,
credentials are not persisted, and no path filters, failure inversion or skipped
tests were introduced. No new dependency is required.

Observed [PR18 run41235200798](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41235200798)
for actual head `f2e5a4622cfaa89c83148bbb63fd4464e057c509`: conclusion success,
event pull_request. Actual log shows GNU13.3.0 on Ubuntu24.04.5, successful explicit
`-DSUBATHON_FETCH_GOOGLETEST=ON` configure/build, then all39 timer cases passed.
The remote label summary establishes timer-lifecycle; additional core/acceptance
labels were observed locally, not asserted from the remote summary.

Actual observations: `gh run view 41235200798 --json headSha,conclusion,event,url`
and `gh run view 41235200798 --log`, with the full log retained only under ignored
local-evidence. This demonstrates the new workflow ran, not a future run prediction.
Final PR-head CI still must pass after any further commit or main reconciliation.

This is deterministic domain verification. Qt/state transactions, clocks/segmentation
at application boundaries, real pause reasons, start/reopen, recovery/schedules,
provider/platform/packaging/live checks remain separate. Ruleset26170263 has no
required-status-check rule; all latest checks remain written coordinator merge gates.
No repository rules were changed.
