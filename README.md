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

The [epic and child-task index](docs/tasks/README.md) lists the bounded release backlog
and separate test-first gates. Start with the existing initial core baseline; planned
child records do not mean tests or product behavior have been completed.

The scaffold preset needs CMake >=3.25, Ninja, a C++20 compiler and Python >=3.10.
It does not need Qt or GoogleTest. Product code will require the agreed Qt >=6.12.
The separate acceptance preset requires GoogleTest >=1.17, with optional pinned
source fetching only when explicitly enabled. The Qt probe is a separate preset.

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

The only core operation is an explicit unimplemented stub. Six draft GoogleTest
acceptance cases describe cap-limited time credit. They must receive independent
contract/test review and an observed intended failing run **before implementation**.
Do not treat the scaffold smoke test as proof that credit calculation works.
See [test strategy](docs/TEST_STRATEGY.md) and [first contract](docs/contracts/time-credit-v0.md).

## Progress updates

`progress/state.json` is the source of task status. Agents update it and run
`python3 tools/progress.py render`; reopen or reload `PROGRESS.html` to see changes.
The HTML makes no network requests. It has search, status/module filters, detailed
notes, dependencies and verification labels. Task counts are not product completion percentages.
See [progress instructions](docs/PROGRESS.md).

## Repository map

- `core/`: native domain contracts; currently a deliberate stub.
- `server/`, `desktop/`, `web/`, `integrations/`, `schemas/`, `deploy/`: scoped agent instructions and implementation boundaries.
- `tests/`: draft GoogleTest acceptance cases, scaffold checks, optional Qt probe, tooling tests, fixtures.
- `docs/`: full specification, architecture, decisions, test strategy, task/handoff templates and evidence.
- `tools/`: dependency-free Python progress and consistency tools.
- `.github/workflows/`: scaffold checks and a manually invoked, genuinely failing acceptance lane.

The GitHub repository is `git@github.com:Kernol12/subathon-bot-ai.git`; the main
branch is `main`. No live credentials are needed, and no service is contacted by
the default build. Local validation and remaining gates are recorded in
[the publication review](docs/evidence/initial-publication.md).
The GPL-3.0-or-later license is in `LICENSE.md` and `COPYING`.
