# Current account-session clock — document proposal v1

SESSIONCONTRACT-004; identity `subathon.account-session-clock.v1`.
Specification v0.45 §§10/16, AUTH-05/09/12, DB-01, SERVER-01, TEST-04/05.
Base main `7897e3e25d1b87c40569d7943e0bac38d75aef4d`, claim `0fb5eed`.
DOCUMENT ONLY. The [author record](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/sessioncontract-004-author.md)
records actual sources, literal audits and checks; a different reviewer must
evaluate this actual proposal before acceptance. No native clock call, header,
test, stub, implementation, deployment or platform capability is supplied.

Import [supplier v1](session-resolution-v1.md) §§2–4, blob
`c5f5a6e71664042e18475068d3caf577b0c8d009`,
[lifecycle v1](session-lifecycle-v1.md), blob
`48e3375bda594a39cca126ba14153d47664f0cdd`,
[transport v1](session-transport-v1.md), blob
`2c33c2bbe9052455107c3056783d6bb605a5fef2`, and
[physical account/session v1](sqlite-account-sessions-v1.md), blob
`8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1`, unchanged, including their
independent reviews. Their current-scope, error priority, commit/publication,
epoch, absolute lifetime and full-domain arithmetic obligations remain binding.
[ADR-0010](../decisions/0010-session-executable-prerequisites.md) allocated the
clock work but chose no mechanism; this document makes the concrete proposal.

## 1. Meaning of trust and its explicit host assumption

Use the selected Linux host's reported POSIX realtime as UTC milliseconds under
the ordinary trusted-host model: the administrator maintains a correct UTC clock
and functioning clock discipline, the kernel/libc and hardware or hypervisor are
trusted, and the application stays in its original time namespace. The application
does not set the clock or synchronization status, choose an Internet time server,
perform NTP, or operate a network time authority. Existing host/deployment owners
must document and validate clock discipline for native Linux, Docker and Podman;
no particular daemon, network protocol or new package is selected here.

Trust is established from actual local kernel synchronization observations PLUS
the current continuity monitor and authoritative installation readiness below.
An HTTP date, client timestamp, accepted-login time, checkpoint, fake clock,
`std::chrono::system_clock::now()` alone, monotonic counter or caller `trusted`
flag cannot satisfy that predicate. Kernel synchronization status is evidence
inside this host trust model, not proof of absolute physical UTC: privileged code
can change the status and error estimates. Reading `TIME_OK` does not authenticate
an upstream server. The kernel initializes unsynchronized status and implements
the returned state and writable discipline parameters in
[Linux v6.12 NTP code](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/time/ntp.c).

Consequently this contract promises exact decisions against current OS-reported
time under those assumptions. It cannot promise correctness against a compromised
host, coherent VM/process snapshot rollback, or an administrator falsely marking
an incorrect clock synchronized. Those observations may be indistinguishable from
ordinary startup. No boot ID, saved wall/elapsed pair or timerfd proves otherwise.
Known restore or clock uncertainty is fenced; an unknown host restore is an
explicit platform/trust limitation, never certified as detected. If a deployment
cannot meet the premise, authentication is unavailable. If review interprets the
accepted supplier as requiring absolute UTC independent of this premise, that is
an explicit requirement conflict: fail closed and obtain a revised source/trust
contract before implementation, rather than claim these Linux APIs prove it.

Linux realtime uses an Epoch representation with leap-second behavior and can be
stepped or slewed. BOOTTIME includes suspend; RAW is not a UTC source. No TAI/local
timezone substitution or leap-smear correction is added. See the
[Linux clock interface](https://man7.org/linux/man-pages/man2/clock_gettime.2.html)
and [v6.12 timekeeping documentation](https://raw.githubusercontent.com/torvalds/linux/v6.12/Documentation/core-api/timekeeping.rst).
Time namespaces can offset BOOTTIME while leaving REALTIME unvirtualized; the host
must bind the monitor to the same namespace throughout its lifetime, including
containers. Namespace migration/checkpoint restore requires a hold/rebind, never
reuse of the former anchor. [Linux time namespaces](https://man7.org/linux/man-pages/man7/time_namespaces.7.html).

## 2. Actual owner and unchanged consumer API

The ONE server installation host owns ONE clock service on the authoritative Qt
queue. The service owns the native monitor descriptor, in-memory continuity state
and nonsecret reason for refusal. Drogon/request threads, clients and integrations
cannot supply samples or change readiness. State transitions and native reads run
on that queue without event pumping, await, network I/O, nested writer work or a
second state writer. A foreign/reentrant/expired-scope call returns unavailable
without using a previously observed time.

The existing API remains exactly `CurrentSessionClock::sample_current()` returning
`SessionClockSample{int64 now_utc_ms}` or `SessionSourceUnavailable`. There is no
public `set_time`, `set_trusted`, reset-anchor flag or diagnostic-bearing failure.
An internal host-owned facade binds it to ONE actual execution or publication
scope and current installation generation; retention of the facade cannot prolong
that scope. Its host binding uses actual current installation facts and operational
readiness, not a request's installation reference. Bound identity facts borrow
existing bounded host/source storage only while its lifetime is guaranteed; no
clock-owned history, account/session table scan or copied secret is needed.

The actual writer/session source still establishes coherent installation/account/
record facts and validates shapes/epochs. Sampling does not acquire account access
or override revoked/disabled state. Supplier priority remains: missing and revoked
stop before calling the clock. Lifecycle issue still needs actual accepted fresh
sign-in; timestamp availability does not establish it. The physical store owns no
clock policy or hidden clock table. SESSION-009 owns the real bounded Linux adapter,
monitor state and host scope binding; API-001 owns the authoritative host/queue
lifecycle. ACCESS-005 and STORAGE-007 own their actual recovery/restore protocols.

## 3. Proposed concrete Linux observation profile

This is a source-backed candidate for later test/implementation review, not a new
minimum kernel/package selection. Source reference is Linux **6.12**, commit
[`adc218676eef25575469234709c2d87185ca223a`](https://github.com/torvalds/linux/commit/adc218676eef25575469234709c2d87185ca223a).
The linked Linux man-pages were rendered from **6.19**, fetched 9 September 2026,
as identified in their colophons when read on 7 November 2026. Actual selected
Ubuntu/native/container kernel, libc ABI, seccomp and clock behavior still require
SESSIONTEST-009/BOOT/deployment evidence. Reading source is no runtime verification.

At cold initialization or permitted re-establishment, query REALTIME and BOOTTIME
resolution once each with `clock_getres`. Require successful normalized positive
resolutions no greater than 1000000ns; otherwise unavailable, without replacing
the clock by a coarse source. Retain `q=max(resolution_real,resolution_boot)`.
The 1ms ceiling is this adapter's precision admission limit, not a session-domain
timestamp/lifetime ceiling or a measured accuracy bound.

Create exactly ONE `timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK|TFD_CLOEXEC)`.
Arm it once with `TFD_TIMER_ABSTIME|TFD_TIMER_CANCEL_ON_SET`, initial expiry one
second after a newly read REALTIME, interval exactly one second. Preflight that
native time addition and its timer representation; reject inability without
saturation. For the audited kernel's signed-nanosecond timer representation,
require the initial expiry to be positive and at most 9223372036854775807ns.
This native monitor range does not narrow the supplier's full signed64 UTC type.
The adapter can honestly be unavailable for an OS value outside its native range.
No alarm clock/capability, wakeup requirement, polling thread or periodic Qt work
is added: reads below consume ordinary expiration counts. The periodic timer is
only a discontinuity witness; expiration counts never become UTC or elapsed
authentication age. Cancellation is distinct from normal expiration.
[Linux timerfd interface](https://man7.org/linux/man-pages/man2/timerfd_create.2.html).

Do not rearm an existing monitor while ready or discard cancellation by reopening
it. After reading cancellation, latch the hold BEFORE any subsequent action. A
failed rearm can nevertheless have changed timer state; it never means readiness.
The later source audit must check descriptor exclusivity and close/rearm cleanup.
[v6.12 timerfd setup/read/cancellation](https://raw.githubusercontent.com/torvalds/linux/v6.12/fs/timerfd.c).

One `sample_current()` makes ONE acquisition attempt, in this exact order:

1. Read BOOTTIME `Bs`; nonblocking read of the guard `D0`; read `adjtimex` `A0`
   into a freshly zeroed `timex` with `modes=0`.
2. Read BOOTTIME `B0`, REALTIME `R`, then BOOTTIME `B1`.
3. Read another zeroed, modes-zero `adjtimex` `A1`; read guard `D1`; read
   BOOTTIME `Be`. Stop immediately at any failing observation.

Both synchronization reads must return exactly `TIME_OK`, with neither
`STA_UNSYNC`, `STA_CLOCKERR`, `STA_INS` nor `STA_DEL`. Every other return, including
pending/in-progress leap states and native errors, is unavailable. Modes zero is
read-only; no CAP_SYS_TIME operation or adjustment is attempted. maxerror/esterror
are kernel-reported estimates, not independent UTC evidence or additions to the
five-minute freshness/expiry comparisons.
[adjtimex interface and state meanings](https://man7.org/linux/man-pages/man2/adjtimex.2.html).

Each guard read supplies exactly one 8-byte buffer. `EAGAIN` or a complete 8-byte
positive expiration count is a healthy observation; `ECANCELED` is a latched
discontinuity hold. Zero/short read, invalid count, EINTR or any other error is
unavailable. No retry loop, sleep, busy wait, fallback or reuse of the last sample.
The guard is not a complete event log: pinned cancellation code compares offsets,
and resume notification may be deferred. A negative read result must never be
treated as a healthy count. The monitor supplements the bracket check below; it
does not defeat the explicit host/restore limitations in §1.

All timespecs must be normalized (`0<=nsec<1000000000`), and BOOTTIME reads must be
nonnegative and ordered `Bs<=B0<=B1<=Be`. Require `Be-Bs<=50000000ns` exactly.
Also recheck that R plus one guard period remains in the native timer range
admitted above; leaving that range is unavailable, not a wrapped monitor success.
This 50ms bound refuses a delayed acquisition; it adds no authentication grace.
Successful R is the actual observation inside that interval, not an extrapolation
to return/commit time. Nonblocking/fixed calls bound application work, not kernel
scheduling latency or a guarantee that a stalled OS returns within 50ms.

Using exact mathematical nanoseconds, form the feasible realtime-minus-boottime
offset interval `I=[R-B1-q, R-B0+q]`. The quantization allowance q belongs ONLY to
this observation comparison. On the first complete accepted frame set `K=I`.
Thereafter require `K intersect I` nonempty and replace K by that intersection;
never slide, widen or reset K to accept a disjoint frame. Require `B0` no earlier than
the prior accepted frame's `B1` and `R` no earlier than its prior accepted R.
Equal observed times are legal. A mismatch, decreasing time or cancellation latches
the discontinuity hold, even if milliseconds would hide a nanosecond rollback.
Do not convert to milliseconds before these checks.

This interval check is an explicit inference from the shared adjusted timekeeping
base and distinct offsets, rather than an upstream UTC certification. Ordinary
frequency slewing advances the shared base; changing a wall offset breaks the
relation. Suspend compensation changes wall and boot offsets together, although
the guard can still request reevaluation. See
[v6.12 timekeeper bases/offsets](https://raw.githubusercontent.com/torvalds/linux/v6.12/include/linux/timekeeper_internal.h)
and [timekeeping reads/sets/resume](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/time/timekeeping.c).
Small changes inside finite observation/quantization uncertainty, or changes
cancelled before a witness is observable, are not proven absent. Deployments must
retain the trusted discipline premise; no claimed step-detection percentage or
invented zero-error guarantee follows from this candidate.

Only after the complete frame, current scope/readiness and continuity checks
succeed, convert that R exactly under §5, retain its observation state, and return
the sample. Any missing/unclassified source, conversion/resource failure or
exception returns only unavailable. There is no partial success or guessed zero.

## 4. Startup, holds, recovery and serialized fences

Cold state exposes no sample. The authoritative host first establishes actual
installation identity/epoch and startup/restore readiness through its current
sources. A successful fresh monitor initialization and full frame can then enter
ready immediately under §1's host premise. No arbitrary settling delay, recovered
timer checkpoint or previous-process monotonic value establishes trust.

An ordinary unchanged startup/restart preserves the installation and account
epochs and every recorded session time. It initializes a NEW monitor and observes
the current kernel; a healthy current source permits resolution of still-valid
sessions. A past process's anchor/high-water mark is not persisted or reconstructed
from session rows. This is explicit trust in the currently disciplined host after
restart, not a proof of historical continuity across the process boundary. Kernel
reboot needs fresh live synchronized evidence just as process restart does. No
live evidence, source binding or readiness means unavailable, even if a saved
checkpoint and the apparent wall time look plausible.

After a failed status/read/oversized-duration frame, return unavailable and retain
the old K/R/B state and monitor. A later complete healthy frame may recover only
if it satisfies that SAME anchor, nonregression and current host readiness. A
lost/closed/replaced guard, cancellation, disjoint interval or regression is a
latched observation hold: the detecting invocation MUST refuse. Repeated wall
timestamps alone cannot clear it. Known namespace migration or unresolved actual
restoration instead requires its owning host/workflow rebind/readiness first.

A LATER actual authoritative invocation may attempt re-establishment once: require
current host/installation readiness in the same namespace/generation, close the
old guard, initialize a fresh guard/resolutions, and obtain a new complete frame
with BOTH actual synchronization reads TIME_OK and R at least the greatest prior
accepted R retained by this process. Only then replace the anchor and enter ready.
This explicitly treats fresh kernel-reported synchronization as resolution under
the trusted-host premise, without an arbitrary manual acknowledgment or permanent
hold. Merely reopening the descriptor, receiving a caller flag, or observing two
plausible wall values does not satisfy it. An unresolved native/synchronization/
readiness failure remains unavailable. If corrected time is below that retained
floor, remain held until actual time reaches it or the owning workflow resolves
its installation generation; do not return the floor, clamp time, invent a fresh
sign-in, or extend a stored maximum. No bypass/reset endpoint is added.

Orderly restart, abrupt crash/process kill and host reboot all lose the in-memory
anchor/floor. Each cold start takes the same fresh-source path above, never a saved
anchor or success flag. A known incomplete application restore/recovery remains
fenced through its actual owning readiness protocol across restart; clock RAM is
not that protocol. This design does NOT promise historical nonregression or hold
durability across process loss. It trusts current host UTC/synchronization, just
as ordinary startup does. A clock-owned journal/floor or restart-proof rollback
detector is not implicitly supplied. If stronger cross-restart evidence is required,
root must allocate that explicit durable/source prerequisite before code and
review the changed trust contract; the accepted schema cannot be changed silently.

Restoration is already a mandatory stopped/staged STORAGE-007 operation. Before
ANY restored credential resolution, local-login acceptance, issuance or secret
publication, host readiness stays fenced until the actual installation epoch has
been replaced and acknowledged under that workflow. A good clock cannot clear
that fence. Epoch e→f invalidates every old session and accepted tuple; an unknown
fence commit remains held. Rebind clock scopes to the new actual generation after
the fence, then establish fresh observations. Old facades/queued work cannot use
the new readiness. A new installation generation may retire its old observation
floor only after its old credentials/acceptances are fenced, never as clock-only
rotation. Normal startup does not take this path.

ACCESS-005 recovery changes the selected account epoch in its own atomic recovery
commit, preserving permissions/owner identity. It invalidates old accepted tuples
and sessions for that account. It cannot clear an installation-wide clock hold,
reset global K/R or validate another account. These transition protocols are
downstream owners' reviewed baselines, not implemented by a clock or epoch field.
The actual host must order enter-hold/fence/rebind with state commands on its queue;
clock sampling performs no SQL, durable mutation or nested transaction of its own.

## 5. Exact timestamps, execution and publication

Convert normalized `(sec,nsec)` to `sec*1000 + floor(nsec/1000000)` as a
mathematical integer and range-check the FINAL result against signed64. Do not
first multiply in signed64, narrow time_t, pass through floating point or reject
a representable final value because an unchecked intermediate overflows.
Negative times use floor to the containing millisecond; `(-1,999999999)` is -1ms,
not 0. Full signed64 remains the consumer/data domain, even where the actual Linux
clock/monitor cannot produce those physical times. Boundary examples below are
arithmetic/simulated observations, not a claim the host can set/return them.

Offset/bracket arithmetic must likewise be exact, using checked normalized-pair
operations or an already admitted exact integer facility; total nanoseconds and
signed differences must not be forced through int64. No new compiler extension,
library or native-time representation is admitted by this document.

Supplier/lifecycle continue to compare now against lifetime_started/authenticated
before age subtraction; earlier time is unavailable. Ordered differences of any
two i64 millisecond times fit U64, including 18446744073709551615. Freshness remains
inclusive age<=300000; expiry exclusive now<expires. Positive U64 lifetime addition
is mathematical start+duration with signed-range rejection, never saturation.
Existing maximums, authentication instants, token-only rotation and owner-policy
changes retain the exact unchanged supplier/lifecycle behavior.

Sample at authoritative EXECUTION, after current source/epoch priority permits it,
once per supplier resolution/issue stage as already specified. The fixed sample
belongs only to that indivisible nonyielding work. Queue admission, a previous
successful command or an open page supplies no sample for later work. A released
scope/wait requires current re-resolution, rather than extrapolating elapsed time.

PUBLICATION samples independently, immediately before the synchronous emitter,
after acknowledged commit AND normal exact completion, with the unchanged queue
scope preserved through the emitter call. The producer does not bypass these
checks. Unavailable/discontinuity, publication before execution/authenticated/
lifetime-start time, or publication at/after expiry suppresses the bearer, retains
committed truth and cannot revive the old digest. A naturally later healthy
publication is not itself a wall-clock discontinuity. Remembered cookie seconds
remain `remaining/1000 + (remaining%1000 != 0)` for positive U64 remaining, avoiding
`remaining+999` overflow. No execution-time cached sample determines Max-Age.

Clock refusal maps through the existing unavailable envelope (transport 503), never
wrong password, expired/missing account or a manufactured reauthentication
challenge. It does not freeze session age, renew access, log out users as if a
revocation committed, or stop the subathon. Main/Grace Freeze/Count elapsed/Review,
hard-end authority, real-elapsed/active modifiers and independent disconnect policy
remain their own timer/recovery contracts. An elapsed counter or frozen checkpoint
used there does not supply session UTC.

## 6. Finite work and remaining ownership

A normal complete acquisition is at most NINE native reads: four BOOTTIME, one
REALTIME, two modes-zero adjtimex and two nonblocking guard reads. No retries or
input-dependent scan. Initialization adds two resolution reads, one creation, one
arming-time read and one settime, FOURTEEN calls including its first healthy frame.
Re-establishment adds up to one old-descriptor close, at most FIFTEEN on success. Failure
adds at most one cleanup close: at most FIFTEEN for cold initialization or SIXTEEN
for re-establishment. Failed initialization closes its newly created descriptor
by RAII. Transient frame failure retains the
existing monitor under §4. At most one live descriptor exists and it is never
shared with readers that could consume cancellation. Its periodic work uses the
kernel's timer/read mechanism; application code
registers no one-second callback. Shutdown first revokes facades, then closes it.

Scratch has at most five frame timespecs, two timex outputs, one 8-byte read buffer,
the initialization timer/resolution outputs and fixed checked arithmetic values.
Retained observation state is one offset interval, previous R/B1, q, finite state/
reason and descriptor; there is no growing sample/history/attempt collection or
clock-owned variable payload. Host/source identity facts and scope lifetimes keep
their owning budgets; framework/kernel allocations, ABI sizes and scheduling are
additional and require actual source/resource review. The call counts and fixed
objects are not a measured process-memory, CPU or syscall-latency guarantee.

No new durable fact, SQL schema, public API, dependency or global time service is
selected. The host/readiness integration and Linux adapter fit SESSION-009's
already allocated producer scope. Actual application restore/recovery readiness
still requires its owning protocols. Any proposed new durable recovery record,
independent clock-assurance source or extra API must be split by root into explicit
contract/test/implementation prerequisites with separate review before code;
it cannot silently enlarge STORAGE-020 or
ACCESS-005/STORAGE-007. Such a discovery does not reduce release scope.

## 7. Independent literal examples and acceptance gates

All references/times are synthetic. These are independently derived proposed
oracles, not tests or native calls. For observation rows use q=1ns, valid original
namespace, current installation e, no recovery hold, healthy synchronization/guard
and total acquisition10ns unless varied. Observation values in the first rows
are NANOSECONDS; source/lifecycle rows explicitly use MILLISECONDS.

| ID | Input/order | Required oracle |
| --- | --- | --- |
| SC01 | Cold healthy frame B0=100, R=1100, B1=102ns | I=K=[997,1001]ns; return0ms. Real deployment still needs §1's host premise. |
| SC02 | After SC01, B0=110, R=1110, B1=112ns | I=[997,1001], K unchanged, R/B ordered; return0ms is legal equality. |
| SC03 | After SC01, B0=110, R=1010, B1=112ns | I=[897,901] disjoint; unavailable/latched rollback, no clamped R. |
| SC04 | After SC01, B0=110, R=1210, B1=112ns | I=[1097,1101] disjoint; unavailable/latched forward discontinuity. |
| SC05 | R nanoseconds decreases by1 inside otherwise overlapping brackets | unavailable/latched; flooring both to same ms cannot hide regression. |
| SC06 | Guard read ECANCELED; later R/interval appears unchanged after backward then forward correction | unavailable/latched before returning a sample; a net matching pair cannot clear consumed cancellation. |
| SC07 | A0 TIME_ERROR/STA_UNSYNC; A1 would have been healthy | unavailable; stop at A0, no R/read-on success. |
| SC08 | REALTIME read fails, guard read EINTR, or resolution/create/settime cannot be established | unavailable, no zero/cached sample; failed initialization releases its resource. |
| SC09 | Ordered Bs/Be difference50000000 versus50000001ns | First duration fits; second unavailable. Neither adds50ms to freshness or expiry. |
| SC10 | Guard returns EAGAIN versus exactly8 bytes encoding count3 | Both healthy guard observations; count3 never supplies3000ms of UTC. A short7-byte read is unavailable. |
| SC11 | One transient status failure after SC01, then healthy SC02 with same monitor/anchor/readiness | First unavailable; second may recover. Replace/loss of guard instead requires held re-establishment. |
| SC12 | Greatest accepted R=2000000ns; corrected current R=1999999ns and fresh healthy native evidence | Remain unavailable; no floor substitution. R=2000000ns may re-establish only with current readiness and a new complete healthy monitor/frame. |
| SC13 | Ordinary restart, unchanged actual installation/account epochs e; fresh current kernel/frame healthy, now2000ms | New monitor/anchor; old session started/authenticated1000, expiry86401000 resolves unexpired/fresh. Epoch e is preserved; no blanket logout. |
| SC14 | Same restart, live status unavailable, old checkpoint2000 and caller trusted=true | unavailable; checkpoint/flag cannot establish time or freshness. |
| SC15 | After SC04's refusal, later actual invocation initializes new guard, both native status reads TIME_OK, stable frame R=2000ns with same current namespace/generation/readiness | May re-establish K from the fresh frame because R>=prior1100ns. Old wall values or a caller acknowledgment alone remain insufficient. |
| SC16 | Restored old epoch e and valid-looking clock before STORAGE-007 e→f acknowledgment | All restored access/acceptance/publication remains fenced; no old credential is accepted before replacement. Unknown epoch commit stays held. |
| SC17 | After actual restore fence f, queued old e tuple/clock facade | Old acceptance is stale_authentication; old session is supplier revoked/lifecycle authentication_required; old clock facade unavailable. No rebinding old work by reference. |
| SC18 | Account a recovery e→f, other account b unchanged | a's old sessions/acceptances invalid; b keeps its own facts. Account recovery cannot reset the global clock hold/floor. |
| SC19 | Normalized sec=-1,nsec=999999999 | Mathematical UTC=-1ms, never guessed0. |
| SC20 | sec=-9223372036854776,nsec=192000000 versus191000000 | First final value=-9223372036854775808ms fits; second=-9223372036854775809 refuses conversion. Native ability is not asserted. |
| SC21 | sec=9223372036854775,nsec=807999999 versus808000000 | First final value=9223372036854775807ms fits; second=9223372036854775808 refuses. |
| SC22 | authenticated=started=-9223372036854775808ms, expiry9223372036854775807, simulated trusted now9223372036854775806 | Unexpired, age18446744073709551614, not fresh. now=expiry is expired; ordered full difference is18446744073709551615. |
| SC23 | started/authenticated1000ms, expiry86401000; execution301000 versus301001ms, require_fresh | First age300000 fresh; second requires fresh authentication. Trusted now999 is unavailable under supplier; no unsigned wrap. |
| SC24 | Issue at1000ms normal/remembered | Expiry86401000/2592001000; clock outage/access/rotation never changes these stored maxima. |
| SC25 | Execution1000ms commits normal exact completion; continuous healthy publication86400999 versus86401000ms | First can publish with remaining1ms; second suppresses committed bearer, no expiry extension. A long elapsed interval alone is not a step. |
| SC26 | Healthy execution1000ms, publication unavailable or discontinuity | State stays committed, bearer/reply suppressed, unavailable. Execution sample cannot be reused. |
| SC27 | Publication before execution/authenticated, or work committed but completion throws | Suppress; no old digest revival. The latter fails before any publication sample. |
| SC28 | Positive remaining1/999/1000/1001/18446744073709551615ms | Remembered Max-Age1/1/1/2/18446744073709552 seconds by quotient/remainder, without adding999. |
| SC29 | start=-9223372036854775808ms plus duration18446744073709551615; or normal start9223372036768375808 | First expiry9223372036854775807 fits; second would exceed i64 by1 and refuses issuance before RNG/write. |
| SC30 | Kernel reports TIME_OK after a coherent privileged VM restore invisible to the process | No detection guarantee. Known restore must hold; an unknown coherent host rollback is outside §1's trust model, not simulated supported recovery. |

Different actual document review must assess the source/trust premise, interval
inference, guard/recovery/resource semantics and compatibility with all four exact
imports. SESSIONTEST-009 still needs the concrete minimal adapter/scope API, finite
harness and independent specification-linked policy/native-observation tests,
different test approval and intended missing-behavior red after working foundations.
Future source must show actual Linux producer wiring as well as fake-time policy
cases, then separate source/resource review, passing acceptance and selected CI.
Native dependency errors, source reading, a fake TIME_OK or old supplier tests are
not that evidence. No clock-set experiment belongs to this document task.

The actual host/readiness binding, process-loss trust limits, native kernel/libc/
container observations, lifecycle commit/emitter composition, accepted login,
ACCESS-005/STORAGE-007 fences and full Linux/package/platform checks remain their
own gates. This document creates no executable CI behavior. All seven integrations,
three FX providers and the full release scope remain required.
