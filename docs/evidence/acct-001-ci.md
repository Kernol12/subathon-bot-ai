# ACCT-001 receipt simulation CI

Coordinator `/root`, 2026-10-31, branch `task/ACCT-001-receipt-intake`.
Base main7900a61; specification v0.45 §§3/14/18 and evolving-CI policy.

The new `.github/workflows/receipt-intake.yml` runs every PR to main/main push,
with manual dispatch retained. The same `receipt-intake-debug` preset configures,
builds and runs all15 reviewed `receipt-intake` cases. Checkout and opt-in GoogleTest
source retain existing immutable pins. Only contents:read permission and a15-minute
job budget are used. No new dependency, package install, Qt, Boost, provider credentials
or real database is needed for the framework-free repository-port simulation.

CTest metadata now labels receipt cases as simulation/server/acceptance instead of
historical intentional-red; assertions/fake are unchanged. No skip/failure inversion
or continue-on-error is introduced. Numeric/credit/scaffold workflows remain enabled.
Actual pull_request [run41207963538](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41207963538)
passed for PR16 head018e286792a9a61ca28771858d3e66b631ae744a on2026-10-31.
Its log identifies GCC13.3.0 and15/15 real receipt cases selected by receipt-intake.
The remote Label Time Summary shows receipt-intake only. Extra simulation/server/
acceptance labels were observed in local configured runs, not established by that
remote summary; the fake-backed verification level follows the actual suite/store.
The other four PR checks passed too (numeric, credit, PR/push scaffold). This verifies
that CI compilation/simulation combination only; latest completion-record revision
CI must still be rechecked before merge.

Ruleset26170263 was rechecked before PR15 merge: zero required GitHub approvals,
resolved discussions/PR requirement, no force/delete bypass, and no required-status
check rule. The coordinator requires all latest checks regardless; no protection
settings changed. Adding receipt-intake does not configure required-status enforcement.

Passing these cases establishes generic service logic over a primitive disposable
store only. SQL uniqueness/durability/restart, real commit uncertainty/concurrency,
Qt writer/thread ownership, acknowledgements, actual provider IDs/source authority,
contribution eligibility and one-application effects remain separate STORAGE/API/
connector/accounting gates. Ubuntu CI compilation is not a full deployment/platform
support claim. No live provider or real supporter data is used.
