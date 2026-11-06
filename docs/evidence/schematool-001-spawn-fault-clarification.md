# SCHEMATOOL-001 — proposed private constructor-fault clarification

Author/coordinator `/root`,6 November2026. Actual C/B native CPython source audit
confirmed a post-spawn, pre-return constructor window. Public API§4 ownership and
resource/cancellation outcomes already apply; process-test revision2's test-only
return-real-Popen rule cannot represent a constructor that raises after spawning.
Root therefore proposes revision3 negative instrumentation before approving/running
authored constructor candidatea230265. This is a written private testability change,
not a public behavior/profile/release reduction or an implementation approval.

Actual addendum permits ONLY saved actual native _execute_child first, then bounded
readiness/hold and real SIGINT/default unchanged or specific post-spawn MemoryError.
It retains actual partial object/PID and requires waitpid/FD oracles before fallback;
no destructor poll, watchdog reap, fake success or validating backend is allowed.
Existing parent os.close-after-real-close and initial allocation negative seams are
also made explicit; actual earlier tests/reviews/reds remain preserved. Production
module Popen argv/kwargs/twoFD/oneworker/wholedeadline/caps are unchanged.

C must independently review actual revision3 text; only then root final-reviews the
actual spawn tests against it before new-only native red. Author B keeps production
1579604 and a230265 inert meanwhile. Ordinary original64 tests/contracts/pins and
all source/CI/security/platform/live/full-product gates stay intact. No constructor
helper thread or other solution is approved prospectively.

Checks: repository links/state/current HTML and whitespace only; no target/test/
SDK import, signal, child, native probe, install or live operation. This note is
proposal/evidence of scope, not the different document review verdict.
