# SPIKE-001 — pinned local Drogon dependency validation

Date: 2026-10-30. Author: `/root/drogon_dependencies`, separate dependency agent.
Branch: `task/SPIKE-001-dependency-validation`, from main `935e0d6`.
Authority: specification v0.45 SERVER-01, DB-01 and §17. Coordinator `/root` owns
shared progress, experiment CMake and dependency decisions. This agent owns only
`tools/setup_drogon.py` and this evidence. No product behavior is implemented.

## Selected source and build

The coordinator approved the following exact pins and options before they were
encoded in the setup script. The official stable release is
[Drogon v1.9.14](https://github.com/drogonframework/drogon/releases/tag/v1.9.14).
`git ls-remote` and a clean release checkout resolved it to
`3aae769cc405cf5abb9d477675c6430924f02c7c`. Its recorded Trantor gitlink is
`78e480f5e138acdc05f76c55a2716718cd846364`, declaring version 1.5.29.
Source options were inspected in the pinned
[Drogon CMakeLists](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/CMakeLists.txt)
and bundled Trantor CMakeLists before configuration.

- C++20, Release, shared libraries, bundled pinned Trantor.
- Trantor TLS explicitly uses installed OpenSSL.
- Disable ORM/database adapters, ctl, examples, upstream tests/docs, Brotli, YAML,
  c-ares and spdlog for this bounded experiment. Shared ORM interface source files
  still compile upstream; no database-driver support is enabled by this selection.
- Existing system dependencies are JsonCpp 1.9.8, UUID 2.42.4, zlib 1.3.2 and
  OpenSSL 3.6.5. No package was installed and no global search path/ldconfig changed.

The explicit setup script defaults to this checkout's ignored `build/deps`:

```sh
python3 tools/setup_drogon.py
```

A temporary task worktree can share the persistent primary-checkout directory:

```sh
python3 tools/setup_drogon.py --root <primary-checkout>/build/deps --jobs 4
```

The three sibling directories are `drogon-1.9.14-source`, `drogon-1.9.14-build`
and installed prefix `drogon-1.9.14`. The actual validation used the primary
checkout's `build/deps` so cleanup of `/tmp` task worktrees cannot remove it.
Use the installed prefix as `CMAKE_PREFIX_PATH` or a personal CMake user preset;
no machine-specific path is committed to shared presets.

The script alone performs explicit clone/submodule operations; ordinary project
configuration does not download anything. It verifies both immutable revisions,
refuses modified/untracked sources and unrecognized existing installs, bounds
parallel compilation, copies both upstream license notices and records options
and revisions in local `dependency-build.json`. The first operation resolving the
release used `git ls-remote`; source acquisition used `git clone --branch v1.9.14
--single-branch --depth 1` followed by `git submodule update --init --depth 1 trantor`.
Later successful setup runs reused those already downloaded sources. A fully fresh
script-managed download was not separately repeated.

## Actual checks

Environment: Arch Linux x86-64, GCC 16.2.1, CMake 4.4.5, Ninja 1.13.2,
Python 3.14.8. Commands below ran from the dependency worktree unless stated.

| Command/check | Exit / observed result |
| --- | --- |
| `git ls-remote https://github.com/drogonframework/drogon.git refs/tags/v1.9.14 refs/tags/v1.9.14^{}` | 0; release pin above |
| Release clone and `git submodule update --init --depth 1 trantor` | 0; exact clean sources |
| `python3 tools/setup_drogon.py --root <primary-checkout>/build/deps --jobs 4` | Final run 0; configured, compiled shared libraries and installed locally |
| Same setup command repeated against recognized install | 0; no rebuild required; no Git fetch/update performed |
| `python3 -m py_compile tools/setup_drogon.py` | 0 |
| `python3 tools/setup_drogon.py --help` | 0; explicit root/jobs options |
| Disposable installed CMake consumer: `find_package(Drogon 1.9.14 EXACT CONFIG REQUIRED)`, link `Drogon::Drogon` | Configure/build 0 |
| `env -u LD_LIBRARY_PATH <consumer-build>/installed_consumer` | 0; runtime output below |
| `ldd <consumer-build>/installed_consumer` | Both Drogon and Trantor resolve to the local prefix |
| One-off disposable untracked-source probe | Setup exit 1 with modified/untracked-source diagnostic; marker removed afterward |
| One-off unrecognized-prefix probe | Setup exit 1; existing owner-data file preserved |
| `git diff --check` | 0 |

Installed consumer runtime output:

```text
Drogon 1.9.14 commit 3aae769cc405cf5abb9d477675c6430924f02c7c
```

The disposable consumer lives outside source in `/tmp/subathon-drogon-consumer`;
it includes `drogon/HttpAppFramework.h`, prints `getVersion()/getGitCommit()` and
returns success only for version 1.9.14. It exercises installed headers, CMake
exports, linkage and runtime library discovery. It does not start a server.

The initial setup attempt compiled and installed successfully but its extra notice
copy used `License` for Drogon; upstream's actual filename is `LICENSE`. That script
attempt exited 1 after installation. The filename was corrected; only the new
incomplete prefix generated by that attempt was removed, then the final script
installed it successfully. No pre-existing owner data or pinned source was removed.
The final repeat/refusal checks use the corrected script.

Upstream emits a CMake compatibility deprecation for Trantor's old minimum version
and two JavaScript MIME enum deprecation warnings while compiling Drogon. These
were retained and no vendor patches or suppression flags were applied. They did
not fail the build. Doxygen is absent and optional docs are disabled.

## Limits and handoff

Both source checkouts are clean at the pins above. The installed prefix contains
`libdrogon.so.1.9.14`, `libtrantor.so.1.5.29`, their CMake package configurations,
`share/licenses/drogon/LICENSE`, `share/licenses/trantor/License` and the local
build manifest. System compiler/Qt/transitive libraries remain system-managed;
pinning this pair does not freeze the entire environment.

This validates local dependency availability only. It does not validate TLS
handshakes, real requests/WebSockets, response callbacks, lifecycle/shutdown,
production persistence, Ubuntu portability or any release/platform/provider gate.
The coordinator's separate reviewed experiments establish those bounded results.
Release library selection, relocation and complete notices remain later packaging
work. The experiment pin is a justified feasibility choice, not a released product
support claim. No central progress or production file was changed by this agent.
