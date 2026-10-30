# TEST-001 follow-up — Stable test-first gate fixtures

Author: `/root/test_author`. Date: 2026-10-30.
Branch: `task/TEST-001-credit-tests`; baseline before this correction: `3f7c0d3`.
Scope: one existing tooling test fixture and one complementary positive case;
no tooling implementation, C++ source/contract, or shared progress changes.
Coordinator `/root` owns shared state and authorized this scoped fixture correction
after TEST-002's recorded intended red.

## Specification-based defect and correction

§17 and TEST-04/05 require implementation to wait for a separately reviewed,
observed-red baseline. They allow a legitimate test defect to be corrected with a
written specification basis and separate review. The existing tooling case
`test_cannot_start_implementation_before_red_baseline` changes live `IMPL-001` to
`in_progress` and assumes live `TEST-002` is unfinished. Once the intended baseline
is honestly completed, that state is valid and its expected dependency error is
incorrect. Keeping the live baseline incomplete to make this test pass would
misrepresent evidence and violate the task protocol.

The corrected case uses explicit synthetic `TEST-999` (planned test baseline) and
`IMPL-999` (in-progress implementation depending on it). It preserves the original
`assertRaisesRegex(ValueError, "dependency")` assertion. Synthetic task lists clear
activity references to unrelated live tasks. The positive case uses the same
dependency but records a done baseline with `verification.baseline = observed_red`,
an existing public evidence-note path and a separate approved author/reviewer pair.
Validation must accept starting its dependent implementation. This paired fixture
tests the gate in both directions without depending on current project completion.
The evidence path is only a fixture to exercise structural validation; it is not
a new claim about a real test run or review.

All 19 reviewed C++ acceptance cases, their source blob, the contract and explicit
`FeatureNotImplemented` stub remain unchanged. No assertion was skipped, weakened
or changed to make a product implementation pass. This is developer-tool testing,
not product, provider or platform verification.

## Actual reproduction and checks

The coordinator reported the original test failing once TEST-002 was marked done.
During the author's inspection, TEST-002 was again `in_review` with observed-red
verification and an approved separate review already recorded. A one-off author
reproduction therefore loaded that JSON read-only and changed TEST-002 to `done`
**in memory only** to represent the handoff-ready state. It used the original test
from `3f7c0d3` and the coordinator root for evidence-link validation. This exited 1:
`AssertionError: ValueError not raised` at the retained dependency assertion.
No copied state was written into this worktree or shared progress.

The initial attempt without a coordinator root failed for a different reason
(the author's worktree does not have the coordinator evidence notes); that result
was discarded as a reproduction of this defect. Running the original test with
unmodified `in_review` state passed, confirming the status-dependent fixture.

Post-correction commands/results, all exit 0:

- `python3 -m unittest discover -s tests/tooling -v`: 18/18 tooling tests passed.
- A unittest subclass reran the two corrected gate cases after loading coordinator
  JSON and changing TEST-002 to `done` in memory: 2/2 passed. Their explicit
  synthetic tasks replace live task records, so no coordinator evidence files
  are needed by the corrected cases.
- `git diff --check`: no whitespace errors.
- `git diff 3f7c0d3 -- tests/acceptance/time_credit_tests.cpp core/include/subathon/core/time_credit.hpp core/src/time_credit.cpp docs/contracts/time-credit-v0.md`:
  empty; reviewed product baseline inputs unchanged.

The original-defect reproduction command was:

```sh
python3 - <<'PY'
import json, subprocess, sys, types, unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, 'tests/tooling')
import test_progress
original = types.ModuleType('original_test_progress')
original.__file__ = str(Path('tests/tooling/test_progress.py').resolve())
exec(subprocess.check_output(['git', 'show', '3f7c0d3:tests/tooling/test_progress.py'],
                             text=True), original.__dict__)
coordinator = Path('/tmp/subathon-coordinator')
class CompletedBaselineState(original.ProgressTests):
    def setUp(self):
        self.state = json.loads((coordinator / 'progress/state.json').read_text())
        next(t for t in self.state['tasks'] if t['id'] == 'TEST-002')['status'] = 'done'
validate = original.progress.validate
with patch.object(original.progress, 'validate', lambda state: validate(state, root=coordinator)):
    result = unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite([
        CompletedBaselineState('test_cannot_start_implementation_before_red_baseline')]))
sys.exit(not result.wasSuccessful())
PY
```

Separate reviewer
`/root/reviewer` must approve this exact fixture revision before integration;
the prior C++ approval does not self-approve this follow-up.
