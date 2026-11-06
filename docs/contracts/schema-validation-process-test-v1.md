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
