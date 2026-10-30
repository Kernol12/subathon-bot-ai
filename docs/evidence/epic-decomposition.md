# BOOT-005 — Complete epic decomposition and PR handoff

Date: 2026-10-30. Author/role: Codex, planning/integration.
Branch: task/BOOT-005-split-epics. Base: 55aae95 (main after PR #1).
Pull request: [PR #2](https://github.com/Kernol12/subathon-bot-ai/pull/2), open for review.
Authority: specification v0.45, particularly §17 and §19; initial time-credit
contract revision 0 remains DRAFT. No domain/API contract is approved by this plan.

## What changed

All 27 original epic IDs remain release-scope containers. Added 180 bounded work
packages in per-epic folders, each with scope, acceptance/failure cases, specification
references, ownership boundaries and primary test/verification home. They correspond
to 331 new child records: 151 separate test-baseline gates, 151 paired implementations,
and 29 distinct feasibility, environment, test-preparation or documentation tasks.
BOOT-005 itself is a separate planning record; progress now has 374 records in total.

Each production record depends on its own separately reviewed, observed-red baseline.
Baseline and implementation share a brief so acceptance scope cannot drift between
roles. The existing progress schema already enforces recorded separate approved review
and observed_red for completing test_baseline, and separate review/passing acceptance
for completing implementation. No schema or tooling change was needed.

Epics aggregate all required child completion through depends_on; children depend on
narrow prerequisites, never their parent. Original foundation dependencies remain.
The original cap-credit contract/test/review/red/IMPL-001 sequence is untouched and
IMPL-001 is also required under EPIC-001. The initial fake-event-to-timer/API slice is
separate from full modifier/review/reversal/milestone/recovery integration.

The backlog index records shared responsibility boundaries; a decision-ownership
index links all 122 specification decision IDs to planned tasks. This is planning
coverage, not test coverage or proof that every normative case has an oracle yet.
All normative specification prose and §18 scenarios remain required. CONTRACT-002/003
remain foundation checkpoints; bounded child baselines must resolve/refine relevant
contract questions before production code. No release requirement has been removed.

All seven integrations and all three exchange providers retain adapter, evidence and
verification work. Unresolved StreamElements/Ko-fi/TipeeeStream capabilities have
explicit feasibility gates; live/platform/OBS/package/load evidence remains separate.
Credential versus backup-key rotation, server versus desktop packages, and native
versus browser renderers have distinct owners. Localization/themes, guided setup,
viewer privacy/statistics/retention, diagnostics/notices/source delivery remain in scope.

## Checks actually run

Environment: Arch Linux x86-64, existing Python/CMake/Ninja/GCC scaffold toolchain.
These are planning/tooling/scaffold checks only. All commands below exited 0:

| Command | Observed result |
| --- | --- |
| `python3 tools/progress.py render` | Regenerated PROGRESS.html from state |
| `python3 tools/check_project.py` | 374 records, 122 decisions, dependency DAG, links/presets/current HTML pass |
| `python3 tools/progress.py validate` | Progress records valid |
| `python3 tools/progress.py render --check` | Snapshot current |
| `python3 -m unittest discover -s tests/tooling -v` | 12 tests passed |
| `node tools/check_progress_ui.mjs` | Offline controller DOM simulation passed; real browser not run |
| `cmake --preset scaffold-debug` | Configured |
| `cmake --build --preset scaffold-debug` | Built; no product compilation changes |
| `ctest --preset scaffold-debug` | 3/3 passed |
| `git diff --check` | No whitespace errors |

One-off structural audit also passed: 27 epics, all 331 children attached exactly once,
all 151 production slices gated, prior non-epic task/evidence records preserved and
all 122 decision IDs assigned. Reproduce the core audit from the repository root:

```sh
python3 - <<'PY'
import json, re, subprocess
from pathlib import Path
state = json.loads(Path('progress/state.json').read_text())
before = json.loads(subprocess.check_output(
    ['git', 'show', '55aae95:progress/state.json'], text=True))
old = {t['id']: t for t in before['tasks']}
tasks = {t['id']: t for t in state['tasks']}
epics = [t for t in tasks.values() if t['kind'] == 'epic']
children = [t for t in tasks.values()
            if t['id'] not in old and t['id'] != 'BOOT-005']
assert len(epics) == 27 and len(children) == 331
assert state['verification'] == before['verification']
for task_id, task in old.items():
    now = tasks[task_id]
    if task['kind'] != 'epic':
        assert task == now, task_id
    else:
        assert set(task['depends_on']) <= set(now['depends_on'])
        assert now['status'] == task['status'] == 'planned'
for child in children:
    assert child['status'] == 'planned' and not child['verification']
    assert not child['evidence'] and child['review'] is None
    parents = [e for e in epics if child['id'] in e['depends_on']]
    assert len(parents) == 1 and parents[0]['id'] not in child['depends_on']
    if child['kind'] == 'implementation':
        baselines = [tasks[d] for d in child['depends_on']
                     if tasks[d]['kind'] == 'test_baseline']
        assert len(baselines) == 1 and baselines[0]['brief'] == child['brief']
        assert child['id'] not in baselines[0]['depends_on']
decisions = set(re.findall(r'^\| ([A-Z][A-Z0-9-]*-\d+) \|',
                          Path('docs/specification.md').read_text(), re.M))
assert len(decisions) == 122
assert decisions <= {r for t in children for r in t['spec_refs']}
print('PASS: epic ownership, test-first gates, preserved evidence, decision ownership')
PY
```

## Remaining gates and handoff

Planning is ready for review, not product implementation approval. Every new child
remains planned with no test/review/evidence outcome claimed. No independent planning
or product reviewer is invented. No new tests were authored for this documentation
change; the 151 baseline records are future assignments, not 151 test suites run.
The six existing draft GoogleTest cases, acceptance map, source, tools, build/schema
code and first-agent instructions remain unchanged.

No acceptance-debug, live-provider, Qt/OBS/platform, package or performance test was
run for this planning change. The existing initial core review/intended-red gate
remains open. The generated HTML is a static snapshot; controller simulation does
not establish real-browser layout or accessibility with the larger task list.

PR #2 targets main and is left open for owner review as requested. GitHub check
results for the latest revision are available on the PR; no merge is performed.
Next execution starts with the existing first-agent baseline and
dependency-ready child briefs; refine any remaining bounded contract ambiguities
before claiming implementation.
