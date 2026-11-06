# SCHEMATOOL-001 — actual native constructor test review

Reviewer `/root`, different from author `/root/session_supplier_author`,
6 November2026. Actual test candidatea23026537a2fab9923fe77f5b71d06e4d35bc7f4,
blob777d9071cba94cb80e4147918722b5358f53946f. APPROVED for first new-only
reached-bug run AFTER actual C93a5e1 private process revision3 approval
(docf009979). This review follows that document gate; neither prospective
implementation nor a spawn-thread solution is approved. No methods/native hooks/
signals/helpers/target/worker/SDK imports or bodies executed in source review.

Two ordinary tests cover actual native post-spawn, pre-constructor-return ownership.
Both first execute the SAVED selected CPython _execute_child, retaining real self/
PID/_child_created strongly, obtaining real readiness then holding the native
constructor. The existing module-attribute Popen substitution verifies frozen argv/
kwargs/two actual write FDs before launching an isolated blocked fault child. The
child supplies no activation/outcome or validation capability. Test pipe recording
excludes the real constructor's internal error pipe. No fake successful constructor,
process, standards library, domain result or implementation-derived oracle exists.

SIGINT is sent to the actual main thread only after native readiness/constructor
hold. Original default handler is asserted and never changed; controller waits a
bounded acknowledgement then releases the hold. The public call must propagate
actual KeyboardInterrupt, same object when captured at the native hook. The other
case injects a specific MemoryError only AFTER saved native spawn/readiness and
requires literal resource_limit. A helper thread that fixes only SIGINT cannot
satisfy the separate allocation case. Readiness/controller/timeout/handler failures
cannot supply an intended ownership red.

Before ANY test poll/wait/kill/FD fallback, real waitpid(WNOHANG) captures whether
parent already reaped (ChildProcessError) and fstat captures all4 parent channels.
Both running and waitable-unreaped child states fail, including if the probe reaps
it. Strong self reference prevents __del__ polling from creating a false pass.
All4 descriptors must already be closed. A real kill-only watchdog must not fire;
controller errors and public logs must be empty. Finally releases/joins controller,
cancels/joins watchdog, closes sync/protocol leftovers and kills/waits any surviving
child only after those observations. Controller's bounded acknowledgement is not
the reap oracle. These are resource ownership tests, not actual heap exhaustion or
proof of every kernel/ABI/worker path.

Tests/modules are stdlib-only/inert until explicit execution with lazy trusted
parent import. Exact parent1579604, previous cleanupacde40d and original64f491
remain unchanged at author candidate. Their assertions/fixtures/reviews/FIRSTs are
preserved. Python3.10 AST/compile and repository/whitespace checks pass without
execution. Existing private revision2 text, public API973961c, installation86d6
and all production limits/argv/calls remain intact under reviewedr3 test extension.

Next B runs ONLY this2-method module on exact candidate, retains native output/
exit/time/source/review identities and attributes real constructor readiness/
interruption/allocation plus pre-fallback child/FD failures. Then solve actual
ownership without test-detection branches, global handler mutation, seam bypass or
fake success; any concrete private-boundary conflict must be written and separately
reviewed. Corrected source/targeted pass/full public64/current CI remain gates.
