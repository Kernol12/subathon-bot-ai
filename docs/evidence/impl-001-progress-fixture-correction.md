# IMPL-001 — progress acceptance fixture correction

Author: `/root/credit_implementer`; independent reviewer: `/root/credit_reviewer`.
Date: 2026-10-30. Author branch `task/IMPL-001-credit-implementation`, parent
`29325b5`; isolated worktree `/tmp/subathon-impl001-implementation`.

## Specification-based reason

Specification v0.45 §17, TEST-04/05 and the repository test-integrity rule require
honest completion evidence, passing acceptance and independent review. During
final integration the coordinator recorded IMPL-001's real passing acceptance;
`python3 -m unittest discover -s tests/tooling -v` then exited 1 with 17/18 tests
passing. The existing
`test_completed_implementation_requires_passing_acceptance` borrowed live
IMPL-001 without clearing its acceptance verification. Once that field correctly
became `passed`, its expected `ValueError` was no longer justified. The validator
was correct; the fixture assumed this task would never finish.

This is a tooling fixture defect, not a reason to falsify progress, remove the
completion rule or change the 19-case C++ acceptance baseline. The correction
uses the existing synthetic `task()` helper to create one independent completed
implementation with approved separate review and valid evidence. It retains the
`passing acceptance` rejection assertion for missing verification and adds the
explicit failed-acceptance case. A complementary test accepts passing verification
with separate review. Tests reset activity to avoid references to replaced live
tasks. The adjacent self-review test sets its own relevant verification already
and needs no change.

Only `tests/tooling/test_progress.py` and this note change. The validator,
production source, public header, fixture, all 19 reviewed C++ acceptance cases
and four reviewed regression cases remain unchanged. No tests are skipped or
weakened. Corrected test source blob:
`f606dad3592b2bc2ae329b249f81d3b3c131daf9`.

## Actual reproduction and checks

Environment matches [the implementation evidence](impl-001-implementation.md):
Linux 7.2.11-arch1-1 x86-64, Python 3.14.8, GCC 16.2.1/CMake 4.4.5.

Before changing the test, the author independently reproduced the same failure
without modifying progress JSON using this command. It imports the original test
and injects the future passing acceptance into its input in memory:

```sh
python3 - <<'PY'
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path('tests/tooling').resolve()))
import test_progress
original_set_up = test_progress.ProgressTests.setUp
def completed_state(self):
    original_set_up(self)
    impl = next(t for t in self.state['tasks'] if t['id'] == 'IMPL-001')
    impl['verification']['acceptance'] = 'passed'
test_progress.ProgressTests.setUp = completed_state
suite = unittest.TestSuite([test_progress.ProgressTests(
    'test_completed_implementation_requires_passing_acceptance')])
result = unittest.TextTestRunner(verbosity=2).run(suite)
raise SystemExit(0 if result.wasSuccessful() else 1)
PY
```

Observed exit 1: one test executed, `AssertionError: ValueError not raised` at
the original line 84. This confirms the coordinator's reported cause independently.
After the correction:

| Command | Actual outcome |
| --- | --- |
| `python3 -m unittest discover -s tests/tooling -v` | Exit 0; 19/19 tests pass, including missing/failed rejection subcases and complementary passing case |
| Same in-memory completed-state command above, with a suite containing both completion test names | Exit 0; 2/2 tests pass with live acceptance already `passed` |
| `ctest --preset acceptance-debug` | Exit 0; 26/26 pass: 19 original acceptance, four regression and three scaffold/tooling checks |
| `git diff 29325b5 -- core/src/time_credit.cpp tests/acceptance/time_credit_tests.cpp tests/acceptance/time_credit_regression_tests.cpp core/include/subathon/core/time_credit.hpp tools/progress.py` | Exit 0, empty; product baseline and validator unchanged |
| `git diff --check` | Exit 0 |

Separate review of this exact correction is required before integration.
Coordinator owns central progress/HTML updates and reruns affected final checks
after integrating it. This tooling result makes no live-provider/platform claim.
