# Independent review — SESSIONCONTRACT-004

Reviewer `/root/current_session_clock_contract_review`, different from author
`/root/current_session_clock_contract_author` and coordinator `/root`, 7 November
2026. **APPROVED for the clock contract document and its author evidence only.**
No executable baseline, intended red, header, native implementation, authentication
capability, supported deployment or future CI/merge is approved by this verdict.

Actual reviewed tree `78e78f31c5c53ed2f5a9c28c8a773e81068d0f35`, importing author
`da9d7413820ecd18d93c395ce64acf1fb9b216bf`, coordinator claim `0fb5eed`, base main
`7897e3e25d1b87c40569d7943e0bac38d75aef4d`. Isolated worktree
`/tmp/subathon-sessioncontract004-review`, branch
`review/SESSIONCONTRACT-004-current-clock`. This NEW review record is the only
reviewer-owned change. Shared progress/HTML/handoff, authored documents, source,
schemas, dependencies, build configuration and CI remain coordinator/owner paths.

| Reviewed or unchanged artifact | Exact Git blob |
| --- | --- |
| [Clock contract](../contracts/session-clock-v1.md), 401 lines | `946bd12b94d3a83dcd4c86c787bb257d9472e022` |
| [Author evidence](sessioncontract-004-author.md), 132 lines | `0bc14b54cefb34fb1f8acadf437a83e682570f22` |
| Supplier / lifecycle contracts | `c5f5a6e71664042e18475068d3caf577b0c8d009` / `48e3375bda594a39cca126ba14153d47664f0cdd` |
| Transport / physical account-session contracts | `2c33c2bbe9052455107c3056783d6bb605a5fef2` / `8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1` |
| Supplier / transport independent reviews | `41be21f52c7c9dcdede02547cdcd7c88c08732bd` / `288aa0aaa34b30ffd76289beb61d9a2bcdfc8517` |
| Lifecycle / physical-store independent reviews | `61154d59200351da8744ff08e2731e2d8563a933` / `6dd54440b6123c226dbfef7a316e93eea62dd3fe` |

Inputs: root AGENTS/FIRST_AGENT_PROMPT, architecture/workflow/test strategy/current
state, scoped server AGENTS, task brief/handoff, ADR-0010, specification v0.45
§§4/10/16 and AUTH-05/09/12, DB-01, SERVER-01, TEST-04/05, and the accepted
supplier/lifecycle/transport/physical-store contracts and independent reviews.
Both complete authored files were examined. The eight imported blobs match the
declared base and reviewed tree; the two authored blobs also match the actual
author commit. No separate ingress revision branch was imported or reviewed here.

## Findings and trust decision

No blocking document defect, requirement conflict or unowned substantial
architectural prerequisite was found. No author correction is requested. The
proposal supplies a concrete positive operating model and explicit refusals for
SESSIONTEST-009; it does not substitute sources or prose examples for runtime proof.

The accepted specification/supplier obligations require actual current trusted
time and refusal of unresolved discontinuity/rollback/recovery uncertainty. They
do not require an independent absolute-UTC authority or a durable clock floor
surviving every process loss. This proposal explicitly trusts administrator-managed
host UTC/discipline, kernel/libc/hardware and a stable time namespace. Within that
premise it additionally requires current kernel synchronization observations,
current host/install readiness and a live continuity monitor. A caller flag,
checkpoint, old DTO, monotonic counter or plausible wall value cannot establish it.
The limitations for privileged status falsification and coherent unknown VM/process
rollback are stated honestly rather than claimed as detected recovery.

Ordinary healthy startup/restart obtains a new monitor and full current native
frame without rotating installation/account epochs or discarding valid sessions.
Unavailable live evidence/readiness refuses. Orderly restart, crash, kill and reboot
all lose the in-memory floor and must establish fresh live evidence under the same
trusted-host premise; historical continuity is not inferred. This is consistent
with unchanged-startup preservation in the imports, and does not make every normal
clock unavailable through an impossible historical proof requirement.

In-process cancellation, regression or disjoint offset evidence refuses its
detecting call and latches the hold. A later authoritative invocation can make only
one complete re-establishment attempt: new monitor/resolutions, two actual TIME_OK
observations, current namespace/generation/readiness, and R no lower than the
retained accepted process maximum. This is an explicit resolution criterion under
the host premise. Reopening alone, status failure, old samples or acknowledgment
alone cannot clear the hold. A correction below the floor remains unavailable;
neither clamping nor returning the floor is allowed. Transient incomplete frames
retain the same anchor and monitor rather than sliding to a convenient interval.

Known application-managed restore stays fenced through STORAGE-007's actual epoch
replacement/acknowledgment before restored access, accepted login, issuance or
publication. Unknown fence commit remains held. A generation change retires the old
floor only after old credentials/acceptances are fenced. Account recovery remains
ACCESS-005's atomic account-epoch operation and cannot clear the installation-wide
clock hold. No clock journal/table, durable manual hold, new authority service or
hidden schema addition is needed by the proposed model. Stronger cross-process
assurance would require the explicitly stated prerequisite split before code.

## Scope, arithmetic and finite-work audit

The exact `CurrentSessionClock::sample_current()` result/API is unchanged. Its
host-owned binding belongs to the current authoritative execution/publication
scope and installation generation, cannot be prolonged by retaining a facade, and
runs without event pumping, nested work or another writer. Missing/revoked supplier
priority still precedes sampling. Sampling grants no account access or accepted
login and performs no SQL mutation. SESSION-009/API-001 and the existing recovery/
restore owners retain their allocated work.

Publication takes an independent actual sample immediately before the synchronous
emitter, only after confirmed commit, normal exact completion and preserved queue
scope. Unavailable time, regression relative to execution/recorded authentication,
or now at/after expiry suppresses the bearer without undoing committed truth,
reviving the old digest or extending expiry. Clock refusal retains unavailable/503;
it is not a password failure, manufactured reauthentication challenge or subathon
stop. Timer/downtime/disconnect policies remain separate.

Normalized seconds/nanoseconds convert by mathematical floor to milliseconds and
check the final signed64 result, including cancellation at the negative boundary.
Offset/bracket arithmetic cannot force total nanoseconds through int64. Ordered
full-domain millisecond differences fit U64; positive lifetime addition checks the
final signed range. Inclusive 300000ms freshness, exclusive expiry, unchanged
normal/remembered maxima and quotient/remainder cookie rounding all match the
imports. Native monitor availability limits do not narrow stored/simulated signed64
values or owner lifetime configuration.

The stated read order is exactly Bs/D0/A0/B0/R/B1/A1/D1/Be: four BOOTTIME reads,
one REALTIME, two modes-zero adjtimex and two nonblocking eight-byte guard reads.
The frame admits at most 50ms, with no retry or grace; the limit measures an
observation, not a kernel scheduling deadline. Initialization adds five calls for
14 total, re-establishment one old close for 15, and failed setup one cleanup close
for maxima 15/16. One exclusive live descriptor, fixed frame/native scratch and
retained interval/previous values are explicit; kernel/framework allocations and
latency are not falsely included in a process-wide bound. No unbounded history,
account scan, polling thread or application one-second callback is introduced.

## Independently checked primary sources

These were actually opened/read with the web tool on 7 November 2026. Linux
6.12 is identified by commit
[`adc218676eef25575469234709c2d87185ca223a`](https://github.com/torvalds/linux/commit/adc218676eef25575469234709c2d87185ca223a).
The four man-page colophons independently match edition 6.19 fetched 2026-09-09.
This is source evidence, not execution on that kernel or the development kernel.

| Primary source | Evidence and limit checked |
| --- | --- |
| [Linux clock interface](https://man7.org/linux/man-pages/man2/clock_gettime.2.html), [ktime documentation](https://raw.githubusercontent.com/torvalds/linux/v6.12/Documentation/core-api/timekeeping.rst) | REALTIME Epoch/step/slew behavior, BOOTTIME suspend inclusion, RAW distinction, clock resolution and time_t failure limits. Precision admission is not UTC accuracy. |
| [adjtimex interface](https://man7.org/linux/man-pages/man2/adjtimex.2.html), [v6.12 NTP implementation](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/time/ntp.c) | Modes-zero reads need no clock-setting privilege; initial STA_UNSYNC, mutable status/error estimates, TIME_OK/error/leap-state return handling. These establish no authenticated upstream UTC proof. |
| [v6.12 timekeeper layout](https://raw.githubusercontent.com/torvalds/linux/v6.12/include/linux/timekeeper_internal.h), [timekeeping implementation](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/time/timekeeping.c) | Shared adjusted base with distinct real/boot offsets; wall setting changes its offset, while sleep injection advances real and boot offsets together. Constant-offset interval comparison is a reasonable explicit inference under the stated assumptions, not an upstream global continuity guarantee. |
| [timerfd interface](https://man7.org/linux/man-pages/man2/timerfd_create.2.html), [v6.12 timerfd implementation](https://raw.githubusercontent.com/torvalds/linux/v6.12/fs/timerfd.c) | Absolute REALTIME cancellation flags, nonblocking EAGAIN/8-byte counts, cancellation consumption, offset comparison, deferred resume notification, read-driven periodic forwarding, resource cleanup, and settime returning ECANCELED after changing timer state. The document correctly treats the guard as a witness rather than an exhaustive event log. |
| [v6.12 hrtimer implementation](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/time/hrtimer.c) | Clock-change notification and periodic overrun forwarding corroborate the guard model; they do not establish application latency bounds. |
| [Time namespaces](https://man7.org/linux/man-pages/man7/time_namespaces.7.html) | BOOTTIME can be namespace-offset while REALTIME is unvirtualized; a retained anchor therefore requires the same bound namespace. Migration/checkpoint assumptions and rebind holds are explicit. |
| [v6.12 ktime conversion](https://raw.githubusercontent.com/torvalds/linux/v6.12/include/linux/ktime.h), [time64 limits](https://raw.githubusercontent.com/torvalds/linux/adc218676eef25575469234709c2d87185ca223a/include/linux/time64.h) | Signed-nanosecond native ceiling and conversion saturation require the exact native-representation preflight discussed below; public signed64 milliseconds remain a separate domain. |

Concrete next-baseline source guard, **not a document blocker**: §3 already requires
preflighting the native timer representation and rejecting saturation. Checking
only initial-expiry<=9223372036854775807ns would not satisfy that obligation.
The pinned `timespec64_to_ktime` uses `ktime_set`, which saturates when seconds are
at least 9223372036. For example, expiry9223372036854775806ns fits the mathematical
ceiling but converts to9223372036854775807ns. SESSIONTEST-009/source review must
cover that extra exact-representation refusal rather than treating the ceiling as
sufficient. No future implementation is approved by this arithmetic observation.

The chosen interfaces are established Linux facilities; this document introduces
no new minimum-kernel/ABI/library selection. Selected native/container kernel,
x86-64 libc representation, seccomp, discipline behavior and monitor lifecycle
still need actual evidence. Source reading on 6.12 does not validate those targets
or the development kernel7.2.14. Unsupported observations must remain unavailable.

## Literal and check evidence

All SC01–30 were manually checked against their applicable producer, supplier,
lifecycle or publication stage. These include valid/equal-ms time, nanosecond
regression, both offset steps, consumed cancellation, short/error/status failures,
duration admission, transient recovery, retained-floor re-establishment, healthy
startup and unavailable restart, restore/old-scope/account recovery fences, signed
conversion extremes, freshness/expiry/defaults, delayed/failed publication,
post-commit exceptions, full-U64 cookie ceiling and issuance overflow, and the
honest unknown-host-rollback limitation.

Independent inline `python3 - <<'PY'` assertions used only Python integer/byte/text
operations, pathlib/regex and read-only Git queries. They verified ordered unique
SC01–30, all four interval literals and a same-ms overlapping-interval regression,
50ms admission, process-floor comparisons, five normalized conversion vectors,
full i64/U64 differences/addition, default lifetimes/freshness/exclusive expiry,
all five cookie rounding vectors, 9/14/15/16 calls, native ceiling and conversion
cutoff arithmetic, both authored blobs, eight exact base/import blobs, two-path
author scope, seven local links and trailing whitespace. All passed; no project
clock/helper/resolver/native body generated expected results.

Environment actually observed: Linux7.2.14-arch1-1/x86-64, Python3.14.9,
Git2.56.0. Commands in the isolated review tree:

| Actual command/check | Observed result |
| --- | --- |
| `git status --short --branch`, `git rev-parse HEAD`, source/diff/blob reads | Clean dedicated reviewed branch initially; exact revisions above; author packet has 533 total added lines across two files. |
| Independent inline Python audit described above | Exit0; all assertions PASS. |
| `python3 tools/check_project.py` | Exit0; PASS443 task records/122 decisions, local links/presets/current HTML; no feature execution or draft approval. |
| `python3 -m unittest discover -s tests/tooling -v` | Exit0; 19 tooling tests PASS in0.367s. |
| `python3 tools/progress.py render --check` | Exit0; snapshot current, no shared state/HTML changes. |
| `git diff 7897e3e HEAD --check` | Exit0; supplied task delta has no whitespace errors. |
| Final inline pathlib/regex/Git review-record audit | Exit0; two local links resolve, no trailing whitespace/control bytes, only this new review record changes and exact reviewed HEAD is preserved. |
| `git diff --cached --check` after review-only staging | Exit0; new record has no whitespace errors. |

Initial ordinary staging could not create the shared-worktree Git `index.lock`
because that metadata directory is read-only in the sandbox. Authorized Git
metadata access then successfully staged only this review record; its staged
whitespace check passed. No source or shared ownership boundary changed.

No CMake/configure/build/CTest, native clock/helper/feature body, clock-setting
experiment, real credential/session, network-time/provider or live operation ran.
Author-reported checks remain author evidence. Existing documentation/tooling CI
fits this document-only delta; no executable workflow/check name changes are needed.

Root retains completion metadata/current HTML, actual latest-head CI, current
rules/approvals/discussions and normal PR publication/merge gates. SESSIONTEST-009
still needs reviewed concrete minimal API/harness, actual producer-observation and
deterministic policy tests, separate test approval and intended behavioral red
after working foundations. SESSION-009 then requires actual native/scope wiring,
source/resource/trust review, passing acceptance and selected CI. Real readiness,
ACCESS-005/STORAGE-007 workflows, login/lifecycle/transport publication composition,
Linux/native/container/package/platform and live gates remain their owners' work.
All seven integrations, three FX providers and full release scope remain required.
