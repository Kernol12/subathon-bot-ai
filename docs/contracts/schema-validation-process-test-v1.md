# Schema wrapper private process contract and fault tests — proposal v1

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

The fixed fault cases exercise malformed/missing/extra activation;257 outcome bytes;
unknown/truncated/extra result; a listed failure line with mismatched exit; or
unclassified signal/death. They must return validation_unavailable and reveal no
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
