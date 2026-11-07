# SESSIONCONTRACT-004 — author evidence

Author `/root/current_session_clock_contract_author`, 7 November 2026, separate
from the later document reviewer and coordinator `/root`. Isolated worktree
`/tmp/subathon-sessioncontract004-author`, branch
`task/SESSIONCONTRACT-004-author`; current main base
`7897e3e25d1b87c40569d7943e0bac38d75aef4d`, coordinator claim
`0fb5eed0521dd39a0f4411f813909fa275b88f00`.

Own ONLY the new [clock proposal](../contracts/session-clock-v1.md) and this NEW
evidence note. Root owns shared state/HTML/handoff, graph, schemas, builds, pins,
CI, integration and publication. No header/test/stub/source/native call, schema,
dependency, clock experiment or production behavior was added. This is authoring
evidence, not an independent review or accepted clock capability.

## Authority and unchanged imports

Read root AGENTS/FIRST_AGENT_PROMPT, architecture/workflow/test strategy/current
state, server AGENTS, task brief/handoff, ADR-0010, accepted supplier/transport/
lifecycle/physical storage contracts and reviews, and specification v0.45 §4
clock-policy separation, §§10/16 and AUTH-05/09/12, DB-01/SERVER-01. The baseline
and production stages remain separate; this document supplies no intended red.

| Unchanged imported artifact | Exact Git blob |
| --- | --- |
| Supplier v1 | `c5f5a6e71664042e18475068d3caf577b0c8d009` |
| Lifecycle v1 | `48e3375bda594a39cca126ba14153d47664f0cdd` |
| Transport v1 | `2c33c2bbe9052455107c3056783d6bb605a5fef2` |
| Physical account/session v1 | `8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1` |
| Supplier independent review | `41be21f52c7c9dcdede02547cdcd7c88c08732bd` |
| Transport independent review | `288aa0aaa34b30ffd76289beb61d9a2bcdfc8517` |
| Lifecycle independent review | `61154d59200351da8744ff08e2731e2d8563a933` |
| Physical account/session independent review | `6dd54440b6123c226dbfef7a316e93eea62dd3fe` |

The proposal retains sample_current's exact result/API, missing/revoked-before-
clock priority, full i64/U64 semantics, independent publication sampling, actual
commit/normal-completion gates, current queue scope, existing absolute maximums,
freshness, account recovery and restore epochs. No imported blob was edited.

## Concrete proposal and source audit

Choose ordinary trusted-host OS-reported UTC with live kernel synchronization
evidence, bounded realtime/BOOTTIME brackets, a single cancellation guard and
in-process nonregression. The native profile and finite transition rules are
concrete proposals here; ADR-0010 did not preapprove them. Ordinary healthy
startup/restart can establish current time without rotating epochs. An observed
discontinuity refuses its detecting call; fresh actual synchronized observations
can later re-establish against the retained process high-water and current host
readiness. No arbitrary manual approval or new durable clock hold is required.

Current kernel synchronization is a trust premise about the managed host. It
cannot prove absolute UTC, authenticated recent upstream time, or detect a coherent
privileged VM/process rollback. Process loss discards the observation floor.
Known application-managed restore/recovery fences still belong to STORAGE-007/
ACCESS-005 and cannot be cleared by a good sample. If stronger historical/source
assurance is required, the document explicitly refuses that unimplemented promise
and requires root to split its actual new durable/source/API prerequisite before
code. No global network service or modification of the accepted schema is hidden.

Primary sources actually browsed/read on 7 November 2026, without executing their
code or invoking the proposed native APIs:

| Source/version | Actual observation used; limit |
| --- | --- |
| [Linux 6.12 commit](https://github.com/torvalds/linux/commit/adc218676eef25575469234709c2d87185ca223a), `adc218676eef25575469234709c2d87185ca223a` | Commit identifies Linux6.12; this is an audited source reference, not a newly selected release kernel. |
| [v6.12 NTP implementation](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/time/ntp.c), `time_status`, `process_adjtimex_modes`, `__do_adjtimex` | Initial unsynchronized state, parameter mutability, modes-zero read/state return. No authenticated upstream/time-correctness proof. |
| [v6.12 timekeeping](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/time/timekeeping.c), `ktime_get_with_offset`, `do_settimeofday64`, `__timekeeping_inject_sleeptime`, `timekeeping_resume` | Adjusted base/offsets, clock-set notifications, suspend compensation and resume notification. Offset-interval stability is our explicit inference, not an upstream API guarantee of UTC. |
| [v6.12 timekeeper layout](https://raw.githubusercontent.com/torvalds/linux/v6.12/include/linux/timekeeper_internal.h) | Distinct realtime/boot offsets and shared adjusted read base. No user-space access to internal sequence counters is proposed. |
| [v6.12 ktime documentation](https://raw.githubusercontent.com/torvalds/linux/v6.12/Documentation/core-api/timekeeping.rst) | Realtime, suspend-aware BOOTTIME and unadjusted RAW serve different purposes; none establishes time authority alone. |
| [v6.12 timerfd](https://raw.githubusercontent.com/torvalds/linux/v6.12/fs/timerfd.c), `timerfd_clock_was_set`, `timerfd_resume`, `timerfd_canceled`, `timerfd_read_iter`, setup | Cancellation/consumption, offset comparison, deferred resume work and periodic rearming on reads; guard is not an exhaustive event journal. |
| [v6.12 hrtimer](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/time/hrtimer.c), `clock_was_set`, `clock_was_set_delayed`, `hrtimer_forward` | Source corroborates notification and finite-period overrun advancement; no native measurement or event-loop callback is supplied here. |
| [clock_gettime](https://man7.org/linux/man-pages/man2/clock_gettime.2.html), [adjtimex](https://man7.org/linux/man-pages/man2/adjtimex.2.html), [timerfd](https://man7.org/linux/man-pages/man2/timerfd_create.2.html), [time namespaces](https://man7.org/linux/man-pages/man7/time_namespaces.7.html), man-pages6.19 | Each read colophon states tarball6.19 fetched2026-09-09. Read clock/status/error/flags/read/cancellation/rearm/namespace semantics. These live HTML URLs are identified by the observed edition; their future content is not pinned. |

The 1ms resolution ceiling, 50ms acquisition bound, one-second guard period,
interval intersection/quantization allowance and recovery transition are local
engineering proposals for independent review. They do not change any session
freshness/expiry threshold or establish a measured accuracy/step-detection bound.
No runtime kernel/libc ABI, actual synchronization state, timer behavior, container
seccomp or deployment clock discipline was probed. Linux6.12 source inspection
does not verify the development kernel7.2.14 or selected release environments.

## Independent literal and scope audit

The independent standard-library integer/byte audit is described with actual
results below. It uses no clock implementation, native API, project arithmetic
helper or resolver to generate expected results. It checks SC01–30 identifier
closure, offset intervals/intersections, regression/forward/quantization cases,
normalized negative and full signed-edge conversion, exact U64 differences and
addition, freshness/exclusive expiry, cookie quotient/remainder, native-call
counts and byte-exact import identity. These are document-oracle checks only.

The manual policy audit covers each row against its relevant stage: successful
and same-ms observations, rollback/forward and net-cancel witness, status/native/
short-read/resolution/descriptor failure, acquisition bound, transient recovery,
retained-floor re-establishment, normal startup/uncertain restart, restore before
the installation fence, stale tuples/facades, independent account recovery,
signed extremes, execution versus delayed/failed publication, committed exception,
max-age overflow avoidance, issuance overflow and unknown host-rollback limits.

## Actual checks and remaining gates

Environment observed with read-only commands: Linux7.2.14-arch1-1/x86-64,
Python3.14.9, Git2.56.0. No machine-specific configuration was changed.
The literal audit and repository/tooling/render/whitespace commands were run in
one `set -euo pipefail` sequence, stopping on any failure. All completed with exit0.

| Actual command/check | Outcome |
| --- | --- |
| Git branch/status/rev-parse and git ls-files -s imports | Clean dedicated claimed branch initially; exact four contracts/four reviews tabulated above. |
| Independent inline `python3 - <<'PY'` audit | PASS: ordered unique SC01–30; five interval literals/intersection and same-ms regression; five signed conversion vectors; full i64/U64 boundaries; defaults/freshness/expiry; five cookie ceiling vectors; 9/14/15/16 call counts; eight exact unchanged import blobs; two-path-only scope. No product/native code used. |
| `python3 tools/check_project.py` | PASS443 task records/122 decisions, local links/presets/current HTML. Explicitly does not run/approve draft product tests. |
| `python3 -m unittest discover -s tests/tooling -v` | PASS19 tests in0.356s; tooling only. |
| `python3 tools/progress.py render --check` | Progress snapshot is current; no shared HTML/state mutation. |
| Inline pathlib local-link/trailing-whitespace audit, `git diff --check` | PASS: seven new local links resolve inside this tree; both new files have no trailing whitespace; tracked delta clean. Final staged whitespace check additionally covers the new files. |
| Final native-range/scope/link audit and project recheck | PASS: R+1000000000ns at9223372035854775807 fits KTIME_MAX; increasing R by1 exceeds it. SC01–30 closure, two-path scope, local links/whitespace and project443/122 PASS after that range clarification. |

Initial sandboxed staging stopped before staging because the shared worktree Git
index directory was read-only. Authorized Git metadata access is needed to stage/
commit these two files; it changes no document/runtime capability or check result.

No CMake/build/CTest, feature/native clock test, real credential/account, provider,
network time request or clock-setting experiment was run. Existing documentation/
tooling CI is appropriate; this delta adds no executable suite or check name.
Root still owns actual independent document review, integration/progress/current
HTML, latest-head CI/rules/discussions and normal PR publication/merge. This author
does not certify those gates. SESSIONTEST-009 needs its actual reviewed minimal
API/harness/tests and observed intended behavioral red; SESSION-009 then needs
actual native source binding, passing acceptance, different code/resource/trust
review and selected CI. Current host readiness, full recovery/restore workflows,
login/lifecycle/transport composition and actual Linux/package/platform support
remain owning gates. All seven integrations, three FX providers and full release
scope remain intact.
