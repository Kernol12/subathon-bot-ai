# Schema wrapper private process contract and fault tests — v1, revision4

SCHEMATOOL-001. Proposed actual private-mechanism revision; different document
review is required before dependent native interfaces, tests, probes or code.
Authority remains specification v0.45 §§17/18, API-01/02/03, TEST-04 and the
[public API](schema-validation-tool-api-v1.md). Public calls/outcomes, exact limits,
logical/context/SDK semantics and test-first gates remain. No behavior is supplied.

## Current native mechanism, subject to actual review

[ADR-0009](../decisions/0009-schema-native-process-owner.md) records why actual
CPython post-birth result allocation and ownership/deadline gaps require a different
private transport. The [native process contract](schema-validation-native-process-v1.md)
now defines the exact first-party artifact/C ABI, integer mapping, resource interval,
atomic birth, signal/descriptor state, syscall taxonomy and bounded negative seam.
[Installation revision2](schema-validation-installation-v1.md) defines its admission.
Those actual documents and this revision must be independently approved together;
planning approval alone is insufficient.

This revision explicitly REPLACES production module-attribute subprocess.Popen,
os.pipe and Python time.monotonic choices in revisions1–3 below. Their exact
negative Python interposition mechanisms become historical, not latent fallback
requirements or permitted routes to vacuous green. The old Python implementation
and all original assertions/raw/red/pass/review evidence remain preserved. No new
Python recovery patch is authorized by this document.

One synchronous callback-free native call owns the configuration descriptor, both
protocol pipes, null/normalization handles, atomic PID/pidfd, deadline, termination,
reap and closes. Raw pipe2/clone3/native clock operations replace the old Python
resource acquisition/clock calls. All Python conversion/loading/admission precedes
entry; all native resources are gone before integer conversion or Python unwinding.
Use the native contract's precise actual generated-code/resource/signal gates;
C++/ctypes names are not evidence that those gates pass.

The actual worker receives exactly this trusted 12-entry argv plus null:

```
ABS_INTERPRETER -I -S ABS_WORKER --manifest PATH --sdk-dir ABS_SDK_DIRECTORY --activation-fd 3 --outcome-fd 4
```

Environment remains exactly PATH=/usr/bin:/bin, LC_ALL=C.UTF-8 and LANG=C.UTF-8.
Only stdio0/1/2 (read/write null) and the two blocking WRITE channels3/4 survive
exec. The parent retains the two corresponding bounded nonblocking reads, polls
its pinned pidfd, and accepts no logs or exception text as protocol. No shell,
extra option, Python preexec_fn, successful injected worker or public factory exists.

Ordinary outcomes still require exact A+activationEOF, the exact one public JSON
line+outcomeEOF, matching normal exit and actual parent reap. The sole no-A exception
is exact EMPTY activationEOF + the35-byte environment_unavailable line+outcomeEOF
+normal3+actual reap, before any package/candidate work. No malformed/nonempty A,
forged ok, unknown/truncated/extra/whitespace/status-mismatched result or arbitrary
signal can certify success. Known outer resource/deadline rules retain priority.

The sole production clock is native CLOCK_MONOTONIC, with t0 before the first
native resource acquisition. The absolute ten-second work/result-acceptance deadline
includes configuration preparation, atomic birth, imports/IO and child exit. It
never restarts after spawn/readiness. A final ordinary result requires actual EOF,
matching status, reap, all closes and a final time strictly before the deadline.
Expiry yields resource_limit; cleanup must still terminate/reap/close before return.
Non-real-time kernel delays can prolong mandatory cleanup, not late acceptance or
return with a live/unreaped child. This makes the public cleanup/deadline combination
explicit; no timeout option, limit extension or successful skipped worker is added.

## Preserved negative oracles and reviewed migration

The actual [74-method map](../evidence/schematool-001-native-test-mapping.md),
blob96130822 at be753ee, accounts for public64, cleanup2, context3, lexical3 and
spawn2. All literal inputs, classifications, public lines/statuses, byte ceilings,
readiness guards, FD/reap-before-fallback proofs and nonvalidating setup assertions
remain. Every original FIRST artifact remains byte-identical. A mechanism change
requires different review of the ACTUAL port source/oracle and an intended new red;
a missed Python patch becoming unreachable is not proof or an acceptable pass.

Only the native contract's negative-only SAME-core DSO may replace the private
artifact load in fault tests. Its fixed cases preserve all original activation3,
outcome7, status/death, setup10 and readiness/deadline rows, with real child/PID/FD
identities. Its raw journal/report and original outcome assertions are independent.
No Python callback, fake accepted integer, fabricated process or fake semantic SDK
may establish success. Normal/domain/SDK-clone cases use the real production
transport, real worker and installed pinned backends. Private context controls
also gain public integration; they are not renamed public passes.

Port cleanup and constructor oracles to the actual native acquisition/signal/reap
boundaries with real observations before any fallback. Preserve genuine Python
pre-entry and post-return allocation/cancellation controls; require exact exception
identity wherever actually observable. A real native SIGINT propagates after
cleanup without inventing an exception from an integer. Native-known allocation
refusal after genuine birth checks the same resource_limit/no-survivor requirement;
it is not a claim that Python c_int allocation occurred inside native ownership.
The exact arm/phase/report ordering and case28 raw descriptor/signal observations
are defined by the native contract and independently reviewed source.

Real loader/ABI/build/kernel/rollback/signal facilities must be GREEN before any
intended missing-native-behavior run. Missing library/dependency/compiler support,
unsupported clone3/seccomp, failed readiness, fixture crash, skip or timeout is
neither valid red nor passing selected-host acceptance. Minimal inert ABI/harness,
actual tests, intended red, implementation, separate source audit and full ordinary
both-host CI remain distinct gates. This revision approves no future code.

## Historical revisions1–3, retained verbatim for evidence

The following archived revision3 text is the exact previously reviewed contract.
It describes the old Popen implementation/test mechanism only; the current proposed
revision4 above supersedes those mechanism choices after its actual approval.
Its unchanged public protocol/refusal/cleanup assertions are preserved above and
in the migration map. It is not an additional simultaneous production requirement.

# Schema wrapper private process contract and fault tests — v1, revision3

SCHEMATEST-001, root9c8d083. Refines the approved [public API](schema-validation-tool-api-v1.md)
and [context contract](schema-validation-context-v1.md) for testability only.
Authority specificationv0.45 §§17/18, API-01/02/03, TEST-04; all logical limits,
public signatures/outcomes, real-library ownership and test-first gates remain.
No process/validation implementation or feature/test execution is supplied.

## Exact private spawn plan

Production parent uses the standard-library module attribute subprocess.Popen,
not a prebound Popen alias, with this ordered argv:

```
ABS_INTERPRETER -I -S ABS_WORKER --manifest PATH --sdk-dir ABS_SDK_DIRECTORY --activation-fd N --outcome-fd M
```

All three trusted absolute paths and the manifest path retain the public contract's
encoding/length/ownership admission. N and M are DISTINCT inherited child WRITE
pipe descriptor numbers, exact nonnegative decimal spellings of descriptors>=3.
The parent retains the corresponding read ends. The private worker accepts exactly
these four option/value pairs once each; no additional public CLI or injectable
worker/validator factory exists. The normal script/interpreter/SDK choices come
from the admitted trusted installation, never from package input or user options.

Popen keyword requirements: shell=False, close_fds=True, stdin=DEVNULL,
stdout=DEVNULL, stderr=DEVNULL, pass_fds containing EXACTLY N and M. No preexec_fn,
text-mode streams, inherited input or unbounded capture. Use the existing reviewed
minimal environment/absolute executable/no-site startup. This document does not
relax worker resource admission, .pth refusal or installation/ABI checks.

The worker writes activation exactly A to N only after the fixed resource prologue,
then closes that channel. Outcome goes ONLY to M, as the exact constant JSON line
and matching exit code from the public protocol; close it on exit. Candidate logs
remain suppressed and never become protocol. Parent closes its child-write ends
immediately after spawn, bounds reads, enforces the whole deadline and reaps the
child before returning on EVERY outcome. Cleanup failures cannot yield ok.

Parent obtains the deadline and subsequent time observations through the module
attribute time.monotonic. Record the start immediately BEFORE Popen; the ten-second
bound includes startup/imports/IO through exit. A fresh worker owns each invocation.
This fixed private choice permits deterministic tests of startup-time exhaustion.
It does not create a caller-controlled clock or change the public ten-second limit.

## Fault-only test seam

Tests may intercept ONLY the parent's standard-library Popen call to launch a REAL
tiny isolated synthetic fault child in place of the normal executable. Return the
REAL Popen object, preserve the actual pipe endpoints/ownership and save its PID.
Validate the normal proposed argv/keyword shape before substituting it. The test
child has no standards validator and is never used for a successful/domain-validity
case. Normal package, scope, schema and instance tests use the real unpatched path.

### Native constructor failure and parent cleanup faults (revision3)

The return-a-real-Popen rule above applies when the constructor RETURNS. Actual
selected CPython can create a real child and then raise before returning its
handle; its constructor exception path closes IO but does not guarantee kill/reap.
API§4 still requires parent ownership/cleanup while Python can unwind. This narrow
fault-only extension permits independent tests of that already required behavior.
It supplies no alternate successful validator or public injection.

A test may intercept the ACTUAL selected Popen class's native _execute_child method
ONLY to call its SAVED native implementation FIRST, then retain the actual self/
PID/child-created observation and hold constructor return at a bounded readiness
point. The child is the same real isolated BLOCKED FAULT child substituted through
the existing parent module-attribute Popen seam; no activation/result certifies
validity. After native spawn, a real SIGINT with the unchanged default handler may
interrupt the main thread, or a specific MemoryError may be raised at the selected
post-spawn boundary. Such failed construction cannot return a completed Popen.
Retain its real object strongly for the test's observations; never use destructor
polling/test cleanup as proof of parent ownership. This is explicitly a failed
constructor fixture, not permission to fabricate a successful process/result.

SIGINT tests leave caller signal handlers/masks unchanged, use a bounded controller
and readiness/release synchronization, and join their controller after the oracle.
A kill-only watchdog must never reap or turn a timeout into a passing observation.
Both cancellation and allocation tests capture real waitpid(WNOHANG) and all owned
parent FD states BEFORE any fallback poll/wait/kill/close. Require actual parent
reap/descriptor release; allocation returns resource_limit, cancellation propagates
its original interruption when observable. Setup/readiness/controller/handler or
other failures are not an intended red for this ownership requirement.

Separate bounded parent cleanup tests may inject a one-shot BaseException AFTER
a saved REAL os.close completes on an owned descriptor, or a specific MemoryError
at initial private path-admission entry. The latter checks public known-allocation
containment, not actual heap exhaustion; setup/Popen/package IO guards must remain
independently asserted after return. No successful package/schema/instance test may
use these faults or replace a backend. Ordinary original fault protocol tests and
all public real-validation cases retain their existing semantics and assertions.

This revision changes only permitted negative-test instrumentation, not production
Popen module lookup/argv/keywords, worker count, public calls/outcomes, deadlines,
resource ceilings, raw-data/ref semantics or full release scope. It does not approve
a helper-thread solution or any future code. Different actual document review,
then actual test source/oracle review, reached-bug red and separate corrected-code
review remain mandatory. Preserve original64/earlier red artifacts byte-for-byte.

### One nonvalidating setup refusal before activation

If the resource/setup controls cannot be established, the worker MUST NOT forge A.
Before any package read/candidate work it may close activation WITHOUT writing any
byte, send EXACT `{"code":"environment_unavailable"}\n` on outcome, close outcome
and exit normally3. Parent may return environment_unavailable ONLY after EMPTY
activation EOF, that exact complete line plus outcome EOF, matching normal exit3
and actual parent reap. This reports a nonvalidating setup refusal; it certifies
neither active controls nor package/schema/instance validity.

ANY nonempty malformed/extra activation remains validation_unavailable even with
that line. Received ok, domain/dependency/IO/unknown/extra/truncated/whitespace or
status-mismatched results without A remain validation_unavailable. This exception
is only the exact environment refusal; all other received outcomes require A.
Independent outer interruption precedence remains: parent deadline/known resource
interruption is resource_limit even before A; an unknown signal alone does not
prove memory exhaustion. Keep bounded reads, whole deadline, EOF/status, kill/reap
and descriptor cleanup unchanged. No successful validation is accepted without A.

The fixed fault cases exercise malformed/missing/extra activation;257 outcome bytes;
unknown/truncated/extra result; a listed failure line with mismatched exit; or
unclassified signal/death. Apart from the exact setup refusal above, these received malformed cases return
validation_unavailable and reveal no
child payload on public stdout/stderr. A forged ok line is allowed ONLY paired
with explicitly malformed activation/protocol and an expected refusal. It never
establishes successful validation, installed dependencies or library compatibility.

After validate_package returns, assert os.waitpid(saved_pid,WNOHANG) raises
ChildProcessError BEFORE Popen.poll/wait/context-manager/test cleanup could reap it.
This proves that parent cleanup already reaped the real child. In a finally block,
kill/wait any surviving child AFTER the assertion, to keep failed tests disposable.
Bound test-side readiness/IO with a real kernel select/watchdog and a saved original
clock; the watchdog is not the outcome oracle and cannot turn a timeout into a pass.

For the one deadline case, the real fault child acknowledges test-side readiness
and then blocks on a test-owned pipe. The test interposer waits for this bounded
readiness before returning the real process. A test-only time.monotonic patch then
advances beyond the already recorded ten-second deadline. Expect resource_limit,
kill/reap and no public logs. Keep the original real clock for test watchdogs;
no repeated sleeping, catastrophic regex, actual memory exhaustion or stress run.
This verifies parent deadline accounting/cleanup, not every OS scheduling scenario.

Parent-side interception does not prove child secure-open/read-race/descriptor/ABI
behavior. Those remain real integration tests and implementation/source-audit gates.
Do not label a mocked parent OS error as proof of an inaccessible child dependency.
Actual symlink/missing/nonregular package cases use real disposable filesystem state.
No successful SDK stub, fake semantic validator or injected accepted result is allowed.

## Written clarification and sequence

The earlier API text listed only the activation descriptor while requiring a separate
outcome pipe. Its bounded control-input list now explicitly includes BOTH private
output-channel descriptors, matching the existing two-channel protocol. No package
field, public call/option, outcome, success condition, resource ceiling or product
scope changes. The exact private argv/module choices above are new reviewable test
harness constraints, not an assertion that a worker already exists.

A DIFFERENT reviewer must approve this actual document/API clarification BEFORE
fault tests depend on these labels. Actual test source/oracles then require their
own different review before any run. Raw inert validate_package/main still raise
the exact named missing-feature marker BEFORE spawn or argument processing; no
minimal fault harness may implement validation behind them. FIRST intended red and
later reviewed implementation/ordinary CI remain mandatory separate gates.

Revision2 reason: implementation-readiness review found the broad missing-activation
rule conflicted with API§4's required environment_unavailable when controls cannot
be established, while A is allowed only after verified limits. The narrow fail-only
exception above resolves that conflict without faking activation, accepting success,
changing public calls/options/outcomes or removing any existing refusal assertion.
Actual separate document/source review and targeted additive-case red are required
before implementation; original FIRST63/261 and all valid reviews remain preserved.

Revision3 reason: actual selected CPython constructor source audit found a child can
exist before Popen returns; r2 fault wording required returning a completed object
and could not represent this failed-constructor ownership path. The explicit narrow
negative-test extension above allows real native evidence without forging process
success or changing any production/public requirement. Earlier r2 setup-refusal
exception and all original assertions/reviews remain intact. This proposed revision
requires different actual review BEFORE new constructor tests are approved/run.
