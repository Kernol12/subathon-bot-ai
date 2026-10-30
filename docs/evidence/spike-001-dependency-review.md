# SPIKE-001 — independent local dependency setup review

Date: 2026-10-30. Author: `/root/drogon_dependencies` (coordinator preserved the
artifacts as `0f5c57b`). Reviewer: `/root/bridge_reviewer`, separate from author
and coordinator. Review-worktree equivalent prerequisite: `e9dde64`.
Scope: tools/setup_drogon.py and spike-001-dependencies.md against specification
v0.45 SERVER-01, section 17 and the selected local feasibility dependency decision.

**Verdict: approved for integration of the bounded explicit local setup.** This
validates neither production support nor the real network bridge. No dependency
or global path is installed/modified by this review.

Exact reviewed setup-script blob: `d42867d72b35565e0b1b6d6c70b2bf1463ec9921`.
Drogon revision `3aae769cc405cf5abb9d477675c6430924f02c7c` declares 1.9.14 and
its gitlink pins Trantor `78e480f5e138acdc05f76c55a2716718cd846364`. The installed
local manifest agrees with both clean source checkouts. The reviewer inspected
actual pinned upstream CMake options and actual generated cache: shared libraries
ON, ORM/ctl/examples/tests/docs/Brotli/YAML/c-ares/spdlog OFF, bundled Trantor and
OpenSSL TLS selected. These are experiment build choices, not release pins.

The script is explicitly invoked; normal configure does not fetch sources. It
uses subprocess argument arrays, verifies immutable revisions/clean source trees,
rejects an unrecognized nonempty prefix, bounds compile parallelism, copies both
upstream license notices and writes a local identity/options manifest. It does
not invoke sudo, ldconfig, system package installation or edit global CMake paths.
Failure after a partially completed install can require manual inspection/cleanup
of that newly created incomplete prefix; conservative refusal is preferable to
silently overwriting an unrecognized install. The evidence records its actual
initial license-filename failure and corrected successful run.

The evidence correctly distinguishes a tested reuse path from an un-repeated
fully fresh script-managed download, and installed-linkage validation from server
startup/TLS/runtime portability. Using system compiler/Qt/transitive libraries
means these two source pins do not freeze the whole machine environment.

## Reviewer checks actually performed

| Command/check | Exit / observation |
| --- | --- |
| `python3 -m py_compile tools/setup_drogon.py` | 0 |
| `python3 tools/setup_drogon.py --help` | 0; explicit root/jobs options |
| Disposable unknown-prefix invocation | Exit 1, expected diagnostic; pre-existing marker content preserved |
| `python3 tools/setup_drogon.py --jobs 0` | Exit 2; positive job bound enforced before setup |
| One-off manifest/source `git rev-parse` and `git status --porcelain` checks | Both immutable pins match; both sources clean |
| `env -u LD_LIBRARY_PATH /tmp/subathon-drogon-consumer/build/installed_consumer` | 0; Drogon 1.9.14 and exact commit printed |
| `ldd` on that installed consumer | Drogon and Trantor resolve to the local prefix |
| Upstream source/CMakeCache inspection | Actual options and Trantor gitlink agree with setup script |
| `git hash-object tools/setup_drogon.py` | Reviewed blob above |
| `git diff --check` | 0 |

The consumer was built earlier by the dependency investigator; the reviewer
executed it and inspected linkage but did not independently rebuild it, rerun the
full dependency build/install, or perform a fresh download. The refusal check used
only a disposable temporary directory and preserved its marker. No shared state,
root/experiment CMake, production source or existing test was edited.

Next: coordinator integrates this review with separate test-baseline review,
records actual missing-operation red evidence, then obtains independent review
of the real experiment implementation and actual passing local checks. Release
packaging, full notices/manifests, Ubuntu compatibility, real TLS, providers and
other platform gates remain outside this setup review.
