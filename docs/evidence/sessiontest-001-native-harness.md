# SESSIONTEST-001 SDK-only native preparation harness

Coordinator /root, 2026-11-04. Preliminary author fc62e6375828a36d98b4e63703367e83326e1088,
import ad9c27bd43ccb729a99c52e3fd997d47e0218543. Actual source blobs remain foundation
9f688e3d9353311913af9dac8bab4043e1f5b51e and generator f705227eb4460c551fdabd14cf6460b2b6849299.
[Author evidence](sessiontest-001-test-author.md) describes their exact proof boundaries.
Logical/API/header/immediate-three-throw production source are unchanged.

The password option adds a standalone SDK-only foundation executable and the single
CTest name `scaffold.password_hashing_environment`, labels scaffold and
password-hashing-environment, timeout30. Its private link is only Sodium::Sodium,
which imports the explicit static SDK and Threads; no project target or GTest.
The separate generator is EXCLUDE_FROM_ALL, has the same SDK-only link, and has NO
CTest/custom-command/build-time execution. It must be built explicitly and run
only after actual different review and green foundation. It emits synthetic input
credentials; expected policy results stay independently specified literals.

Actual commands from this task worktree:

```sh
cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug --target password_hashing_environment password_hashing_fixture_generator
ctest --preset password-hashing-debug --show-only=json-v1
nm -C build/password-hashing-debug/tests/password_hashing_environment
nm -C build/password-hashing-debug/tests/password_hashing_fixture_generator
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Configure and four compile/link steps passed, no project warnings. Discovery has
only three existing infrastructure cases plus the foundation, correct exact name,
labels/timeout/command; generator is absent. Both symbol tables contain no subathon
symbol or C++ global initializer. These commands NEVER executed either program,
any SDK function or project feature/marker body. Different actual program/harness
review is next. Root then owns FIRST foundation execution with child core limits,
raw log/JUnit/clean revision and SDK identity; only after green may root execute
and retain the two complete successful generator input records. Full feature tests,
private error harness, different full review, current selected CI, FIRST intended
feature red and production are still later gates. Host foundation is not supported
server benchmark, Ubuntu CI or platform/packaging acceptance.
