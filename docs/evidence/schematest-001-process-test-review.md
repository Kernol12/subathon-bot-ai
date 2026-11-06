# Independent private process/fault-test document review — SCHEMATEST-001

Reviewer `/root/session_plan_review`, different from author/integrator `/root`,
6 November2026. **APPROVED DOCUMENT ONLY**, actual candidate
`e6fe6bd238426f34a53ca4f67e21c35733f552ad`; private process document blob
`7f30d99d2977a38105ce8618f827da804565c8f1`; clarified public API blob
`a526dbc1114de7f79268dd37c51b97988e627c80`.

Full actual document and parent diff read on NEW isolated branch
`task/SCHEMATEST-001-process-test-review`, worktree
`/tmp/subathon-schematest001-process-test-review`, from that candidate. Only this
review note changes. No source, test, process, candidate import or feature marker
was executed or authored; no dependency/environment/shared/GitHub write occurred.

## Contract consistency and scope

Authority: specificationv0.45 §§17/18, API-01/02/03, TEST-04; root instructions,
workflow, tools/tests instructions and paired [task brief](../tasks/SCHEMATOOL-001.md).
Accepted logical blob `abb426a91173726a703cba70d6a68225de483bdd` and final context
blob `32ec87a25dca2d39227a7fe994df3a2d908a26e6` remain EXACT. The API change from
accepted691 is confined to its bounded control-input paragraph: explicitly name
activation AND outcome descriptors. The same accepted API already required separate
activation/outcome pipes, bounded result+EOF/status checking and cleanup. These are
owned worker output channels, not new package inputs or caller-provided results.
No enum, signature, public argv/help/exit, success condition, priority, payload limit
or selected dependency changes. No new product/runtime capability is asserted.

The new document freezes an internal spawn choice for tests: real module-attribute
subprocess.Popen; absolute trusted interpreter/worker/SDK paths; -I -S; exactly four
private option/value pairs; distinct canonical decimal child-write descriptors>=3;
parent-read ownership; shell=False/close_fds/DEVNULL and exactly those pass_fds.
Resource prologue, trusted installation/ABI checks, no-site activation and minimal
environment obligations remain. No preexec callback or public injectable factory
supplies validation. Both raw inert feature bodies still immediately raise the exact
named marker; this document permits no behavior behind those stubs before red.

## Fault oracle and ownership assessment

The seam interposes only on the parent's spawn (and, for the explicit deadline
case, its module-attribute monotonic clock). It validates the proposed normal spawn
before substituting a tiny REAL child, returns a REAL Popen and retains its real PID.
Fault-channel descriptors preserve the parent's owned endpoints. The test-only
readiness/release pipes are synchronization for the synthetic child, not additional
inputs to the normal worker. Domain/package/scope/schema/instance cases remain
unpatched and must reach the actual SDK path later. No fake successful validator,
accepted-result injection or forged ok in a successful case is admitted.

Fixed malformed/missing/extra activation, overlong257-byte/unknown/truncated/extra
outcome, failure-line/status mismatch and unexpected death exercise exact refusal
and closed diagnostics. A forged ok is allowed only in an explicitly malformed
refusal case. Test source must make each asserted malformed protocol complete enough
to reach that refusal; a deliberately blocked child instead exercises deadline.
This document approval supplies no observed result for either path.

The reap assertion has the correct order: after API return, waitpid(saved PID,
WNOHANG) must raise ChildProcessError BEFORE test poll/wait/context-manager cleanup
can reap. A running child returns no status, while an exited unreaped child returns
status; neither can satisfy the assertion. Finally cleanup occurs only afterward,
so a test-owned cleanup cannot manufacture parent cleanup success. Actual future
tests must retain the Popen/PID, avoid another waiter/SIGCHLD handler and audit every
failure path, pipe end and watchdog; no generic mock of waitpid is allowed.

Start is recorded immediately before Popen. A bounded ready acknowledgement then
blocks the real fault child, and the test advances the parent's observed time beyond
that already recorded deadline. The saved original clock/kernel readiness watchdog
keeps the test disposable and cannot supply its expected resource_limit. This is a
deterministic parent startup/deadline/kill/reap oracle, not memory stress or general
scheduling proof. No repeated sleep or catastrophic regex is necessary.

Read-only primary checks: Python3.10 [Popen descriptors/environment](https://docs.python.org/3.10/library/subprocess.html#subprocess.Popen)
supports explicit pass_fds on POSIX and close_fds/DEVNULL/binary operation;
[waitpid/WNOHANG](https://docs.python.org/3.10/library/os.html#os.waitpid) and
[ChildProcessError](https://docs.python.org/3.10/library/exceptions.html#ChildProcessError)
support the real-child reap distinction; [monotonic](https://docs.python.org/3.10/library/time.html#time.monotonic)
is the elapsed-time source. These are source/documented feasibility facts, not a
new process experiment or Windows support claim. The admitted developer tool is
Linux/POSIX; all Windows/Linux product-platform requirements remain separate.

## Release and remaining gates

No blocking document finding remains. The test author may now depend on these exact
private spawn labels and author bounded REAL fault-child cases. Their actual source,
literal protocols, timers, cleanup/exception paths and failure priority need different
review BEFORE any feature/test execution. Parent-side patches do not prove child
secure opening/read races, trusted dependency/ABI admission, RLIMIT enforcement,
no-fetch semantics or general process correctness. Those need genuine integration
cases and implementation/source audit; unclassified parent mocks supply none.

Different full test/oracle and minimal harness/config review, actual selected green
foundations and intended missing-wrapper red still precede SCHEMATOOL001 production.
This approval is not declaration/hosted-receipt approval, a baseline red, implementation
or release acceptance. Ordinary stable CI must remain passing while intentional red
is registered separately. All7 integrations,3 FX providers and CONTRACT013/runtime/
platform/live/full-release gates remain.

Actual commands in this review tree: `python3 tools/check_project.py` PASS426tasks/
122decisions/links/presets/current HTML; `python3 tools/progress.py render --check`
PASS; `git diff e6fe6bd^..e6fe6bd --check` PASS; exact candidate diff names ONLY the
new private process document and public paragraph clarification. No candidate source
or test import, execution, installation, native child or feature call was performed.
