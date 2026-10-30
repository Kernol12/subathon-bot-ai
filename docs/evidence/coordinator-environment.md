# BOOT-002 — coordinator environment validation

Date: 2026-10-30. Author: coordinator `/root`.
Branch: `task/BOOT-002-coordinator-validation`, based on refreshed `origin/main`
`9982001`. Specification v0.45; initial time-credit contract revision 0.
Scope: native toolchain, scaffold and Qt environment checks only. Coordinator owns
progress/state.json and PROGRESS.html throughout this multi-agent session; the
contract/test author, independent reviewer and feasibility investigator use distinct
worktrees. No production calculation is implemented.

## Environment and observed checks

Linux `7.2.11-arch1-1`, x86-64; GCC `16.2.1 20260810`, CMake `4.4.5`,
Ninja `1.13.2`, Python `3.14.8`, Qt6Core `6.12.0`, GoogleTest `1.18.0`.
Versions obtained using `uname -srmo`, `c++ --version`, `cmake --version`,
`ninja --version`, `python3 --version`, and
`pkg-config --modversion Qt6Core gtest`. No dependency downloaded or installed.

Commands in the isolated coordinator worktree:

| Command | Exit / observation |
| --- | --- |
| `cmake --preset scaffold-debug` | 0; GNU C++ and Python discovered |
| `cmake --build --preset scaffold-debug` | 0 |
| `ctest --preset scaffold-debug` | 0; 3/3 scaffold checks passed |
| `cmake --preset qt-probe-debug` | 0; Qt >=6.12 found |
| `cmake --build --preset qt-probe-debug` | 0 |
| `ctest --preset qt-probe-debug` | 0; 4/4 passed, including queued Qt event loop |
| `python3 tools/check_project.py` | 0; 375 task records, 122 decisions, links/current HTML |
| `python3 -m unittest discover -s tests/tooling -v` | 0; 17 passed |
| `python3 tools/progress.py render --check` | 0; current snapshot |

These tooling checks used the initial coordinator claims; final integrated checks
are recorded separately in the baseline handoff.

## CLion evidence

Read-only IDE `get_compiler_info` for `scaffold/main.cpp` reported the selected
`subathon_scaffold_info | Debug` configuration, GCC `/usr/bin/c++`, `-std=c++20`,
`-g` and repository warning options. IDE `get_run_configurations` listed
`subathon_scaffold_info`, `scaffold_contract_smoke` and `All CTest`.
`execute_run_configuration --configurationName subathon_scaffold_info` exited 0
and printed the expected development-scaffold notice. This is execution through
CLion's selected native configuration, not merely reading an IDE settings file.

The IDE terminal also executed a disposable shell script containing exactly:

```sh
set -eu
cd /tmp/subathon-coordinator
cmake --preset scaffold-debug
cmake --build --preset scaffold-debug
ctest --preset scaffold-debug
```

That IDE invocation exited 0 with 3/3 CTest checks passed and used the same
`/usr/bin/c++` toolchain identified above. It did not change the owner's selected
profile or machine-specific settings. The imported preset selection in the graphical
settings UI was not inspected; this is not a claim about every IDE profile.

An initial underspecified universal IDE call was rejected by automatic review.
After inspecting the available command metadata, explicit read-only toolchain
inspection and scaffold execution were permitted. Earlier terminal attempts failed
because the executor does not launch a shell implicitly and nested quoting was
not preserved; those attempts ran no build and are not counted as checks.

## Remaining gates

GoogleTest pkg-config availability is distinct from acceptance compilation and red
execution; those belong to TEST-002 after independent baseline review. Scaffold and
Qt probes establish no timer feature, durable transaction, server or platform support.
No live provider, Windows, Wayland capture, OBS, package or GUI product checks ran.
No credentials or live supporter state were used. BOOT-002's executable native
checks and selected CLion toolchain execution are satisfied; later baseline readiness
still depends on REVIEW-001 and TEST-002.
