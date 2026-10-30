# BOOT-003 — current local Qt environment probe

Author: `/root/qt_validator`, environment validation, 2026-10-30.
Branch: `task/BOOT-003-probe-evidence`, based on main
`2bc4e6420ac7c56ce9fb29bc678798d2987ae8df`.
Specification: [v0.45 §2](../specification.md#2-architecture).
Scope: local native scaffold and Qt Core/Test event-loop environment only.

## Acceptance and observed result

The existing `qt-probe-debug` preset must configure against Qt >=6.12, build,
and pass its Qt Test queued-work probe. Record actual tools and distinguish this
check from application, QML and platform support. The existing
[BOOT-002 evidence](coordinator-environment.md) already contains a historical
successful run; this fresh run closes the separate BOOT-003 environment task.
It changes no contract, test, production source or dependency pin.

The worktree had no `build/qt-probe-debug` before configuration. All commands below
ran there against the revision above, with no package installation or download.

| Command | Exit and observed result |
| --- | --- |
| `uname -srmo` | 0; Linux `7.2.11-arch1-1 x86_64 GNU/Linux` |
| `c++ --version` | 0; GCC `16.2.1 20260810` |
| `cmake --version` | 0; `4.4.5` |
| `ninja --version` | 0; `1.13.2` |
| `python3 --version` | 0; `3.14.8` |
| `pkg-config --modversion Qt6Core Qt6Test` | 0; `6.12.0` for each |
| `cmake --preset qt-probe-debug` | 0; GNU C++ 16.2.1 and Python 3.14.8 found, Qt >=6.12 configuration succeeded |
| `cmake --build --preset qt-probe-debug` | 0; all ten build steps completed, including AUTOMOC and `qt_environment_probe` |
| `ctest --preset qt-probe-debug` | 0; 4/4 tests passed: `scaffold.contract_boundary`, `scaffold.python_tools`, `scaffold.project_consistency`, `scaffold.qt_event_loop` |

The Qt test schedules zero-delay queued work with `QTimer::singleShot` and observes
it using `QTRY_VERIFY` under `QTEST_GUILESS_MAIN`. This demonstrates a functioning
local Qt Core event loop and Qt Test executable. The three other CTest checks are
infrastructure checks. The suite contains no product acceptance behavior.

## Revision and independent review

Unchanged probe blob: `008c5907224d08e5a3697d91f282c2fea5b5bec4`
(`tests/qt/qt_environment_probe.cpp`). Unchanged preset blob:
`0ef5895266aac78c31169356eff3897745b6c007` (`CMakePresets.json`).
Independent evidence review is pending coordinator assignment; the author does not
approve its own gate. The coordinator owns progress/state.json, generated
PROGRESS.html, the task PR and integration checks.

## CI assessment and remaining gates

No executable suite or behavior is added by this environment-only task. Existing
GitHub workflows run scaffold checks and reviewed credit acceptance/regression;
neither enables `SUBATHON_BUILD_QT_PROBE`. Remote Qt configuration/build/probe
coverage remains an explicit CI gap. This evidence establishes the current local
environment only; it does not establish an Ubuntu/Windows runner or release build.
The first executable Qt production slice must add suitable Qt CI coverage, with
pinned dependencies and actual workflow observations, before claiming that gate.

No Windows, Kubuntu/Ubuntu, Wayland, QML/Qt Quick, OBS, GUI product, package or live
provider checks ran. No credentials, supporter state or external side effects were
used. Domain acceptance is unchanged and is covered by the separately reviewed
credit task. Next work may use the validated local Qt environment while retaining
the per-behavior contract/test/review/red sequence.

Changed path: this evidence file only. Progress and HTML are integrated by the
coordinator; no separate environment-support claim is implied for another host.
