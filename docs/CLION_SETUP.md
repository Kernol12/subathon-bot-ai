# Open and run in CLion

## Arch Linux / KDE reference development machine

Install a C++20 compiler, Python >=3.10, CMake >=3.25 and Ninja using your normal
package manager. On Arch the base command is:

```sh
sudo pacman -S --needed base-devel cmake ninja python gtest
```

Package installation is a local owner action if the agent lacks permission. Verify the
actual installed GoogleTest version (the preset accepts >=1.17). The scaffold needs no
Qt. Product/Qt probe work requires Qt >=6.12; install the appropriate public Qt modules
and Wayland dependencies for the agreed native application. Do not assume the Arch
version alone establishes Ubuntu/package compatibility.

## CLion

1. File → Open: select this folder/root CMakeLists.txt.
2. Select your local C++ toolchain in Settings → Build, Execution, Deployment → Toolchains.
3. CLion can import CMakePresets.json. Enable `scaffold-debug` in the CMake settings
   if the imported profile is disabled. Do not enable every optional profile immediately.
4. Build/run `subathon_scaffold_info`; this identifies the scaffold, not a running server.
5. Run CTest preset commands below in CLion's terminal. CLion supports configure/build
   preset import; its test UI is additional, not a replacement for the documented commands.
6. Give the agent FIRST_AGENT_PROMPT.md. Agent plugins differ, so explicitly ask it
   to read AGENTS.md rather than relying on automatic discovery.

```sh
cmake --preset scaffold-debug
cmake --build --preset scaffold-debug
ctest --preset scaffold-debug
```

## Acceptance tests

Use an installed GoogleTest package, or pass a source checkout path:

```sh
cmake --preset acceptance-debug -DSUBATHON_GOOGLETEST_SOURCE_DIR=/path/to/googletest
```

Alternatively explicitly permit a pinned source download (requires Git/network):

```sh
cmake --preset acceptance-debug -DSUBATHON_FETCH_GOOGLETEST=ON
cmake --build --preset acceptance-debug
ctest --preset acceptance-debug -L acceptance
```

The initial six drafts have been expanded to 19 cases for contract revision 1.
Separate review and an actual run confirmed all 19 fail on FeatureNotImplemented;
see [the baseline handoff](evidence/time-credit-baseline-handoff.md).
Acceptance remains intentionally failing until the later implementation task.

## Qt / Windows

Use `qt-probe-debug` and a CMakeUserPresets.json derived from the example when Qt is
outside standard prefixes. Product minimum remains Qt 6.12; the scaffold has no GUI yet.
On Windows use MSVC 2022/x86-64 in a CLion toolchain with Ninja and Python; terminal
commands need the MSVC environment (CLion terminal/toolchain or Developer PowerShell).
`python` may replace `python3`. The local Qt Core/Test environment probe passed, as recorded in
[environment evidence](evidence/coordinator-environment.md). Windows, product QML,
OBS and live-provider validation remain unperformed. Windows app distribution remains an installation-free ZIP.

## Documentation sources checked during preparation

- https://www.jetbrains.com/help/clion/opening-reopening-and-closing-projects.html
- https://www.jetbrains.com/help/clion/cmake-presets.html
- https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
- https://google.github.io/googletest/quickstart-cmake.html

IDE, compiler and dependency versions should be recorded by BOOT-002 and pinned for
release only after the first relevant builds. No host-specific .idea files are bundled.

After opening the package, manually open PROGRESS.html in your normal browser and
check filters, expanded notes and Light/Dark. The preparation environment checked its
controller in a DOM simulation, not a real browser. If CMake is temporarily unavailable,
`python3 tools/verify_scaffold.py` performs a limited direct GCC/Clang smoke check; this
does not replace BOOT-002 or a real GoogleTest acceptance run.


## Optional Drogon/Qt feasibility experiment

SPIKE-001 uses an explicit [local dependency setup](../tools/setup_drogon.py) and
standalone experiment CMake, separate from the root scaffold and product acceptance.
Follow [the reproduction commands](../experiments/spike-001/README.md).
No system package installation or automatic configure-time download is required on
the validated host. Use a personal CMake user preset or CMAKE_PREFIX_PATH for a local
prefix; the normal project profiles remain unchanged. Passing the bounded Linux
loopback cases does not validate the production server or Windows/Ubuntu deployment.
