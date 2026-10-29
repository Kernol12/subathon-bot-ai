# Baseline preparation verification

Date: 29 October 2026. Scope: developer scaffold only.
Environment: Linux x86-64, Python 3.12.14, g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0, Node v24.22.1.
No Git commit or remote exists for this prepared folder.

| Command | Observed result | Scope |
| --- | --- | --- |
| python3 tools/progress.py render | Passed; generated root snapshot | Static generation |
| python3 tools/check_project.py | Passed; 41 tasks, 122 decision records, presets/links/current snapshot | Consistency, not compilation |
| python3 -m unittest discover -s tests/tooling -v | 12 tests passed | Developer tools only |
| python3 tools/verify_scaffold.py | Passed with GCC C++20 and warnings as errors | Contract/stub/scaffold compilation and smoke execution |
| node tools/check_progress_ui.mjs | Passed filter, empty-state, expand/collapse and theme checks | DOM simulation, not actual browser |
| Workflow YAML parse | Both workflow documents parsed | Syntax only; not GitHub execution |

Commands were run from the preparation workspace with the project prefix where needed;
the relative forms above are the equivalent commands from the project root.

Not run: CMake configuration/build, CTest, real GoogleTest acceptance baseline, Qt/QML,
CLion itself, Windows/MSVC, Wayland/KDE/OBS, real-browser rendering/accessibility checks,
Docker/Podman/native deployment, live OAuth/provider/payment checks, restore and performance.
CMake, Ninja, Qt and GoogleTest were unavailable in this environment.

No independent test review is claimed. Six cases remain draft. The production credit
function remains an explicit FeatureNotImplemented stub. The direct smoke success is not
evidence that the cap-credit feature or full intended application works.

Next: BOOT-002 local CLion/toolchain validation, CONTRACT-001, TEST-001, REVIEW-001, then
TEST-002 observed intended failures before IMPL-001.
