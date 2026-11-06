# SCHEMATOOL-001 — bounded published-constructor correction review

Reviewer `/root/session_plan_review`, different from author
`/root/session_supplier_author`,6 November2026. Actual candidate
`4cc464a35567bbd8be1022f16221ecd4f5d1515f`, root import4902d82, parent blob
`3438a92c9a98a4aa326ed733abffeee6c26c7502`. **APPROVED ONLY the TWO observed
published-handle correction paths below. Overall parent approval remains WITHHELD
for the explicit ownership/deadline findings.** No uncommitted/later fix is approved.

NEW isolated branch `task/SCHEMATOOL-001-constructor-code-review`, worktree
`/tmp/subathon-schematool001-constructor-code-review`, starts at actual4902d82.
ONLY this new evidence note changes. Accepted API973961c/processf009979 and
installation86d6d48 retain the full public outcome, one-child lifetime, whole
deadline, exact factory/protocol and cancellation requirements.

## Actual test-first and source evidence

The exact2-method source777d9071cba94cb80e4147918722b5358f53946f was independently
reviewed root57a79f1 AFTER private-r3 C93a5e1, before first new-only native run.
The receipt records clean authored a230265 at12:37:06.899997Z, actual parent1579604,
exit1/2tests0.019s/2failures, external0.113702795s and no timeout. Independently
hashed committed raw stderr3138bytes SHA256
`14fe4719a6cdab8638829dd30a830c74e873c2d07f1dec09d6e1890022c70797` and empty stdout.
Both genuine cases reached `running != parent-reaped`, with factory returned=[]
and ALL4 parent descriptors already closed. Readiness/signal or specific allocation
refusal, controller/watchdog and no-log checks precede that assertion. No unrelated
setup/import/SDK/dependency failure supplies the red. Test fallback follows the
captured real waitpid/FD states. An initial read-only attribution query used
paraphrased failure text; corrected literal-text assertions pass on the native raw.
No test was rerun. Red recorda269e17 predates correction4cc464a.

## Two corrected published-handle paths

ONE owned non-daemon thread calls the module attribute subprocess.Popen exactly
ONCE with the frozen argv/keywords/two descriptors/minimal environment. Captured
native type/init-code references are identity metadata; construction does not call
an alias, __new__, alternate factory, signal handler/mask or profiler. Main-thread
SIGINT cannot interrupt Python constructor execution in that helper. Sticky
cancellation is remembered until the actual returned/recovered handle is published.

When the native constructor throws AFTER self.pid is published, the bounded
32-frame traceback walk accepts ONLY the exact native init code, exact real native
type and builtin positive-int PID from its own state. It does not accept a test PID
or caller result. Only the native control frame's locals are materialized; its
arguments are bounded trusted process controls, not package/SDK trees. Native
traceback references are stripped after recovery and owner references cleared
after synchronous cleanup. Recovery allocation failure is explicitly NOT solved.

The explicit finished Event is published after process/error recovery. This avoids
mistaking an interrupted3.10/3.12 Thread.join liveness result for handle publication.
For these actual entered-constructor paths, cancellation or known MemoryError
unwinds through the enclosing owner, drains the four descriptors and kills/waits
the actual process before propagation/resource_limit. The already reviewed
after-real-close cleanup correction remains intact. No enum/profile/assertion or
successful-validation seam changed.

Author reports source-matching new2 PASS1.017s, prior cleanup2 PASS0.010s and selected
parent11 PASS0.207s. These are AUTHOR targeted observations; no independent rerun,
full public64 or hosted compatibility result is inferred. Exact regression777d907,
prioracde40d and frozen64f4911 sources are byte-identical. AST comparison against
1579604 changes only the preexisting _worker_result plus new private owner helpers,
threading import and fixed identity metadata; public/type/help/protocol helpers
remain exact. Python3.10 AST/compile WITHOUT exec and one module-Popen-call check pass.

## Concrete open findings beyond those two paths

1. **Whole constructor deadline is unenforced.** _join_spawn uses finished.wait()
   with NO timeout and then unbounded join(). The recorded pre-Popen deadline is
   consumed only AFTER _start_spawn returns. A native-first constructor that holds
   before return can therefore delay/hang the caller beyond the accepted whole10s
   startup/IO/exit deadline. The two1s successful regression observations cannot
   certify this missing control. Root/author were notified before this verdict.
   Preserve the exact source, author/review a meaningful deterministic native-first
   hold plus clock-advance/readiness/watchdog regression and observe reached-bug red
   before fixing the new behavior. No long sleep/stress or fake validation is needed.

2. **Thread.start interruption can escape helper acknowledgement.** A native thread
   may exist before caller owner.started=True and before target owner.entered=True.
   If cancellation lands there, both _join_spawn calls skip their loop and cleanup
   clears owner.thread without acknowledging/joining the real helper. Sticky stop
   prevents a later child but does not prove helper quiescence before public return.
   Actual selected [CPython3.10.12 Thread.start/bootstrap](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/threading.py)
   and [3.12.3 threading](https://raw.githubusercontent.com/python/cpython/v3.12.3/Lib/threading.py)
   put native creation before the startup wait/target invocation. Existing SIGINT
   coverage starts AFTER entered-constructor readiness. Keep this separate startup
   ownership gate and obtain reviewed genuine-start cancellation coverage before fix.

3. **Earlier unpublished native PID/recovery allocation remain OPEN.** Native
   PyLong_FromPid can fail after OS birth before self.pid/_child_created publication;
   this traceback recovery then has no valid handle. Materializing native locals
   can itself fail. The source-backed assessment a348215 in parent-review.md remains
   authoritative; no /proc/pidfd/helper-thread proposal is approved by these2 passes.

4. **Before-close bookkeeping remains OPEN.** Author separately identified the
   unchanged _close sequence: owned.remove(fd) BEFORE os.close(fd). Main-thread
   interruption at that boundary can leave an open descriptor absent from owned.
   Existing acde regression intentionally interrupts AFTER saved real close and
   proves a different path. Reviewer confirmed the source interval and notified
   root; do not silently patch, broaden its old verdict or add an unreviewed fixture.

These findings preserve the existing contract and valid prior reviews. They are
not a request for new public options or permission to omit required behavior. Root
coordinates actual private-seam/test gates before further production correction.

Actual project426/122, current render and whitespace checks pass. This review used
source/primary-source/raw/stdlib AST/hash analysis only; no candidate/test/helper/
SDK import or execution, child/thread/signal, install, historical baseline replay,
live operation or GitHub write occurred. Full parent/source gates above, coherent
public64 and all focused regressions passing, final ordinary CI delta/fresh hosted
lanes/latest-head/normal merge and product/provider/platform/live gates remain.
