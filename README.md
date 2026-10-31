# Twitch Subathon Bot — agent development baseline

A CLion-ready C++20/CMake foundation for the agreed self-hosted C++/Qt6 project.
This archive is a development starting point, **not a working subathon bot**.
The full product baseline is [specification v0.45](docs/specification.md).

## Start here

1. Extract the complete folder and open its root `CMakeLists.txt` in CLion.
2. Follow [CLion setup](docs/CLION_SETUP.md). Enable the `scaffold-debug` profile.
3. Open **[PROGRESS.html](PROGRESS.html)** in any browser; no web server is needed.
4. Give your CLion agent [FIRST_AGENT_PROMPT.md](FIRST_AGENT_PROMPT.md).
5. Read [AGENTS.md](AGENTS.md), [workflow](docs/AGENT_WORKFLOW.md), and the initial task briefs.

For ongoing development with subagents, use the
[coordinator instructions and starting/continuation prompts](docs/COORDINATOR_PROMPT.md).
The coordinator selects from current task evidence and preserves a durable checkpoint
for resuming after an interruption; it does not repeat completed initial preparation.

The [epic and child-task index](docs/tasks/README.md) lists the bounded release backlog
and separate test-first gates. Start with the existing initial core baseline; planned
child records do not mean tests or product behavior have been completed.

The scaffold preset needs CMake >=3.25, Ninja, a C++20 compiler and Python >=3.10.
It does not need Qt or GoogleTest. Product code will require the agreed Qt >=6.12.
The separate acceptance preset requires GoogleTest >=1.17, with optional pinned
source fetching only when explicitly enabled. The Qt probe is a separate preset.

An optional [Drogon/Qt feasibility experiment](experiments/spike-001/README.md)
uses an explicit pinned local dependency build and isolated loopback tests. It
requires no system-wide Drogon installation on the validated host and is separate
from the numeric credit acceptance and regression suite.

```sh
cmake --preset scaffold-debug
cmake --build --preset scaffold-debug
ctest --preset scaffold-debug
python3 tools/check_project.py
python3 tools/progress.py render
```

Use `python` instead of `python3` on Windows when appropriate. See setup instructions.
CLion imports configure/build presets; run CTest preset commands in its terminal.

## Test first

The numeric `calculate_time_credit` helper implements contract revision 1: it
preserves requested credit, limits actual credit to cap headroom, and rejects invalid
inputs or an unrepresentable final sum. The original 19 reviewed acceptance cases
and four additional reviewed regression cases pass. See the
[implementation handoff](docs/evidence/impl-001-handoff.md) for separate code review
and exact checks, and the [original red baseline](docs/evidence/time-credit-baseline-handoff.md)
for the test-first history. Application transactions remain future work.
CORE-001 also supplies bounded lossless decimal parsing and signed half-even final
conversion:32 reviewed acceptance cases plus one normalized-zero regression pass.
Use the explicit `exact-numeric-debug` preset with installed Boost>=1.83/GoogleTest;
[numeric implementation evidence](docs/evidence/core-001-handoff.md) records code
review and automatic Ubuntu/Boost1.83 CI. Application accounting remains future work.
ACCT-001 supplies identified-receipt retry/conflict decisions and attempt provenance
through an atomic repository port. Its15 reviewed fake-backed simulation cases and
[automatic CI/code review](docs/evidence/acct-001-handoff.md) pass; SQL durability,
Qt writer integration and contribution application remain unimplemented gates.
CORE-002 supplies the pure Main/Grace reducer, including automatic resumption after
positive actual credit during confirmation wait. Its39 reviewed cases pass with
[separate code review/automatic CI](docs/evidence/core-002-handoff.md); use
`timer-lifecycle-debug`. Queue/state wiring, live signal integration, start/reopen,
recovery and schedules remain separate tasks.
ACCT-002 supplies independent eligibility holds and verified-identity confirmation
linkage through a repository port;21 reviewed simulation cases and
[separate review/automatic CI](docs/evidence/acct-002-handoff.md) pass. Use
`contribution-evidence-debug`; application, refunds and actual SQL/provider proof
remain separate gates.
CORE-003 implements independent pause-reason and normalized signal-delay proposals.
Its37 reviewed cases pass with [separate review/automatic CI](docs/evidence/core-003-handoff.md);
use `pause-policy-debug`. Live signals, caller segmentation/timer wiring, Qt/state
transactions, reconfiguration, schedules and recovery remain separate gates.
ACCT-003 implements owned captured plan histories, audited override proposals and
preview freshness;22 reviewed cases and [separate review/pinned CI](docs/evidence/acct-003-handoff.md)
pass via `application-plan-debug`. Actual eligibility/current caps/effects, permission,
evaluator, writer and provider gates remain separate.
Do not treat the scaffold smoke test as proof that credit calculation works.
See [test strategy](docs/TEST_STRATEGY.md) and [first contract](docs/contracts/time-credit-v0.md).

## Progress updates

`progress/state.json` is the source of task status. Agents update it and run
`python3 tools/progress.py render`; reopen or reload `PROGRESS.html` to see changes.
The HTML makes no network requests. It has search, status/module filters, detailed
notes, dependencies and verification labels. Task counts are not product completion percentages.
See [progress instructions](docs/PROGRESS.md).

## Repository map

- `core/`: native domain contracts and the bounded numeric time-credit helper.
- `server/`, `desktop/`, `web/`, `integrations/`, `schemas/`, `deploy/`: scoped agent instructions and implementation boundaries.
- `tests/`: reviewed GoogleTest credit acceptance/regression cases, scaffold checks, optional Qt probe, tooling tests, fixtures.
- `docs/`: full specification, architecture, decisions, test strategy, task/handoff templates and evidence.
- `tools/`: dependency-free Python progress and consistency tools.
- `.github/workflows/`: scaffold checks and automatic credit acceptance/regression
  on PRs to main and pushes to main, with manual dispatch retained.

The GitHub repository is `git@github.com:Kernol12/subathon-bot-ai.git`; the main
branch is `main`. No live credentials are needed, and no service is contacted by
the default build. Local validation and remaining gates are recorded in
[the publication review](docs/evidence/initial-publication.md).
The GPL-3.0-or-later license is in `LICENSE.md` and `COPYING`.
