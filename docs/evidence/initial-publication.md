# Initial repository review and publication handoff

Date: 2026-10-29. Agent: Codex. Branch: `main` (initial commit).
Scope: static foundation review, local scaffold/toolchain validation, Git setup.
Authority: specification v0.45; time-credit contract revision 0 remains DRAFT.
Shared files owned for this work: README.md, progress/state.json, PROGRESS.html,
and this evidence note. No production behavior or test assertions changed.

## Review findings

- This is a development foundation, not a working bot. Server, desktop, browser,
  persistence, integrations and renderers remain planned. Existing module boundaries
  and all release requirements remain in scope.
- The core deliberately throws FeatureNotImplemented. Six draft acceptance cases
  cover valid cap-credit examples only. Negative inputs, lowered caps, overflow,
  lifecycle/debt/hard-end interactions still require contract decisions and reviewed
  tests before implementation. Scaffold success does not satisfy those gates.
- Scaffold CI runs infrastructure checks. Acceptance CI is manual and retains real
  failures. Neither workflow was executed on GitHub during this local review.
- Existing ignore rules exclude build trees, IDE settings, local presets, environment
  files, databases and local secrets/evidence. A source scan for common private-key,
  GitHub/AWS/OpenAI token signatures found no matches; this is a limited pattern scan.
- BASELINE_MANIFEST.json describes the original packaged snapshot. Its hashes are
  historical and are not a checksum manifest for the modified repository.

## Environment and checks

Arch Linux rolling, x86-64. GCC 16.2.1, CMake 4.4.4, Ninja 1.13.2,
Python 3.14.8, Node.js 24.19.0, Git 2.56.0.
`pkg-config --modversion Qt6Core gtest` reports Qt 6.12.0 and GoogleTest 1.18.0.
GoogleTest availability was checked through pkg-config only; acceptance compilation
has not been established.

Commands below exited 0 unless specified:

| Command | Observed result |
| --- | --- |
| `python3 tools/check_project.py` | 41 tasks, 122 decisions, links/presets/current HTML validated |
| `python3 -m unittest discover -s tests/tooling -v` | 12 tests passed |
| `python3 tools/progress.py render --check` | Snapshot current |
| `node tools/check_progress_ui.mjs` | DOM simulation passed; no actual browser validation |
| `cmake --preset scaffold-debug` | Configured |
| `cmake --build --preset scaffold-debug` | Built |
| `ctest --preset scaffold-debug` | 3/3 passed |
| `cmake --preset qt-probe-debug` | Configured with Qt >=6.12 |
| `cmake --build --preset qt-probe-debug` | Built |
| `ctest --preset qt-probe-debug` | 4/4 passed, including Qt queued event-loop probe |

## Repository setup and next gates

The requested SSH remote is `git@github.com:Kernol12/subathon-bot-ai.git`.
Initial sandboxed `git ls-remote` exited 128 on a system SSH configuration permission
error. The same command outside the sandbox exited 0 with no advertised refs:
authentication worked and the remote was empty. Initialized local `main` and added
`origin`. Publication uses a normal push without force.

BOOT-002 remains in progress: command-line native and Qt probes pass, but the CLion
selected toolchain/profile and interactive browser checks have not been verified.
Next: finish that environment handoff, then CONTRACT-001 and TEST-001; arrange a
separate reviewer for REVIEW-001, then demonstrate the intended red TEST-002 baseline.
No independent review is claimed by this note. No acceptance, live-provider,
Windows/Wayland/OBS, packaging or product verification was performed.

README repository setup and progress evidence were updated; PROGRESS.html is
regenerated from progress/state.json. The final Git commit identifies this handoff.
