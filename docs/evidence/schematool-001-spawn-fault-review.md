# Independent private spawn-fault document review — SCHEMATOOL-001

Reviewer `/root/session_plan_review`, DIFFERENT from author/integrator `/root`,
6 November2026. **APPROVED DOCUMENT ONLY**, actual candidate
`6224a6ab2bd9d7bbd2555855a7241046363d172c`, process revision3 blob
`f009979cfe64fe155d9dff27449067513c88c215`.

NEW isolated branch `task/SCHEMATOOL-001-spawn-fault-review`, worktree
`/tmp/subathon-schematool001-spawn-fault-review`, from that candidate. ONLY this
review note changes. Actual full text/diff/reason read; no test, helper, target,
SDK, signal, thread/controller, child, install or live operation was executed.
This is no approval of a230265 tests, any thread proposal or future production code.

## Source-backed gap and exact negative instrumentation

The earlier return-REAL-Popen rule covers a constructor that returns. Selected
CPython can create the actual child and execute later Python work before returning
the handle; constructor failure need not kill/reap that already created child.
The caller's failed assignment therefore cannot be treated as proof no process
exists. This is source evidence, not an observation invented from a mock object.
Public API§4 already requires cancellation/known-resource outcomes and ownership.

Previously independently opened/read [CPython3.10.12 subprocess](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/subprocess.py)
and [3.14.9 subprocess](https://raw.githubusercontent.com/python/cpython/v3.14.9/Lib/subprocess.py)
confirm the post-spawn constructor work and limited constructor exception cleanup.
The new document corrects the testability restriction, not those public obligations.

The permitted test must call the SAVED actual selected _execute_child implementation
FIRST, then hold at bounded post-native readiness while retaining the REAL partial
self/PID/child-created evidence. The substituted child is a real isolated BLOCKED
FAULT child through the existing negative Popen seam; it cannot validate a package.
Failed construction need not return a completed handle. Strong test ownership must
prevent destructor polling from manufacturing an already-reaped result.

The two stimuli are actual SIGINT with unchanged default handler/mask, or a specific
post-spawn MemoryError. Real waitpid(WNOHANG)/owned-FD observations precede ALL
fallback poll/wait/kill/close; a kill-only watchdog cannot reap or turn timeout into
success. Controller/readiness/release/join and failed-fixture handling need actual
source review. Setup, handler, readiness or controller defects cannot count as the
intended constructor-ownership red. No successful worker/result factory is admitted.

Independently read [Python signal/thread rules](https://docs.python.org/3.10/library/signal.html#signals-and-threads)
and [current signal documentation](https://docs.python.org/3.14/library/signal.html#signal.pthread_kill):
Python handles SIGINT as KeyboardInterrupt when the caller has not changed that
handler, and Python handlers execute on the main interpreter thread. This supports
the proposed real-signal fixture's reasoning; it is not a live signal experiment
or proof that a helper-thread-only implementation would solve allocation failure.

## Preservation and next gates

The already authorized real-close-FIRST one-shot interruption and initial private
admission MemoryError cases are now explicitly scoped too. They remain negative
instrumentation, no successful validator or actual heap-exhaustion claim. Their
existing ADC source review/reached-red evidence is not rewritten retroactively.

Independent exact text comparison removes ONLY the new revision3 section/reason
and restores the title: the ENTIRE prior revision2 text is identical. Narrow
empty-activation35-byte env3/EOF/reap refusal, malformed-result rejection, outer
deadline/resource priority, fixed argv/two channels/worker count, public options/
outcomes/caps and all prior assertions stay intact. Public API973961c, installation
86d6d4, parent157 and frozen64f remain unchanged. No dependency/profile/release scope
or production seam is added; all7 integrations/3 FX providers remain.

Root may now independently review the ACTUAL constructor tests against this exact
negative-test contract, then observe attributable NEW-only reached-bug red before
any constructor fix. This approval does not approve those authored tests, source
execution, future threading/private API implementation or passing behavior. Source/
allocation/lifetime/worker/security/integration, full acceptance/latest ordinary
CI/normal merge and product/platform/live/full-release gates remain mandatory.

Actual old-text preservation/primary-source/diff checks PASS; repository consistency
426/122/current links/presets/HTML and render--check PASS. No candidate/test/SDK
import, source edit, signal/child/controller run, old-red replay or GitHub write.
