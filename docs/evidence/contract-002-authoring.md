# CONTRACT-002 authoring handoff

Author: `/root/domain_contract`; date: 2026-10-30.
Branch: `task/CONTRACT-002-domain-boundaries`, based on main `2bc4e64`.
Contract/specification: internal event/state boundaries revision 1;
[specification v0.45](../specification.md) §§2–5,8,18 and linked decisions.

Changed paths: [contract](../contracts/event-state-boundaries-v1.md),
[task brief](../tasks/CONTRACT-002.md), this note. Coordinator alone owns shared
progress/HTML, architecture and the coordinator checkpoint.

The contract distinguishes intake, eligibility, captured plans, actual effects,
reversal entitlements, jobs and clock checkpoints. It records one Qt command/writer
thread as an engineering choice, explicit monotonic/UTC domains, rollback/retry and
uncertain-delivery boundaries, plus fourteen manually derived examples for subsequent
baselines. Time-credit revision 1, production code, tests, schemas and dependencies
are unchanged. No new capability or provider/platform verification is claimed.

CI assessment: documentation only; existing project/link/progress and reviewed credit
checks apply. No new executable suite or workflow is introduced. Downstream task
baselines/implementations must add their stable suites to CI; current lack of those
suites is planned work, not evidence that the documented behavior passes.

Checks in the isolated author worktree on 2026-10-30:

| Command | Actual outcome |
| --- | --- |
| `python3 tools/check_project.py` | Exit0; 377 records,122 decisions, local links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | Exit0; current coordinator-owned snapshot |
| `python3 -m unittest discover -s tests/tooling -v` | Exit0;19/19 tooling tests pass |
| `git diff --check` | Exit0 |

Independent review is pending on the actual authored revision. No red run was
performed or required for this documentation-only contract. Production child tasks
still need reviewed tests and observed intended red before implementation.

Remaining gates: separate review of the authored contract, coordinator integration
checks/current HTML, latest PR CI and merge. All concrete C++/SQL/API representations,
resource/parser bounds, clock-discontinuity algorithms, production lifecycle and
provider identity/retry evidence remain in downstream baseline/implementation tasks.
Windows/Kubuntu/Ubuntu, GUI/OBS/renderers, packages and live services are unperformed.
