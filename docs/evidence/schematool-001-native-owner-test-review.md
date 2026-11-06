# SCHEMATOOL-001 — independent native57 test-source review

Reviewer B (`/root/session_supplier_author`), 6 November 2026. Fresh isolated
`review/SCHEMATOOL-001-native-owner-tests` from actual
`81ebfdd05beb15d65c4972fe50c5d0443bd38fd7`. ONLY this note changes. Reviewed A's
f090+978 source blob`d768333e9268515d1e253a2e0a31907fa14418cd`, author note
`6594d9e208d73a58a3a9d2efdcc0e5d26d7fb028`. B did NOT author these tests.
This review does not approve B's fixture/child/Ops, any native source, runtime or
future owner. User scope remains finish PR77 then stop.

**FINDINGS — source not yet approved for FIRST.** Two narrow oracle corrections
are required before approval. No row's native expectation, method registration
or frozen contract needs weakening. Root and A were notified before this note.

1. At source lines367–377, the selected AFTER marker is only required to be
   nonnegative. W14/W16 therefore allow read0/idle poll0, though the matrix's
   selected-work paragraph explicitly rejects idle AFTER coverage. Refine by
   actual primitive: positive real read/poll progress; literal0 for successful
   pipe2/close/pidfd-kill; fd-returning W5/W6 must link the returned actual FD to
   its matching prior adoption, rather than merely finding one adoption. Birth
   already correctly matches the actual child PID. These are independent raw
   syscall/identity oracles derived from native§7 and the frozen selected matrix,
   not from Ops417 or whatever value a future producer happens to emit.
2. P3/P4 both specify adopts0; the common replay merely validates acquisitions
   already present in the journal. Thus a selected refusal/final event and NO
   earlier acquisition can satisfy the current pre-birth oracle. Matrix P3/P4
   explicitly requires observing earlier acquisitions independently of no birth.
   Freeze known prior acquisition phase/count requirements from the preparation
   contract/order (at least the config and actual activation pair for P3; resolve
   P4's guaranteed preparation minima with root). Require those real earlier
   generations/release history, not only a balanced possibly-empty set. The test
   must not copy an accidental order from B's implementation.

Read root/tools/tests instructions, nativeR2/processr4, the reviewed57 matrix,
74 preservation map, actual test/helpers/literal rows and the written author
reason for the earlier child-PID shadow correction. That correction now keeps
`child_pid` stable and uses a separate `event_pid` in journal unpacking; parent
SIGINT observations cannot overwrite the born identity used by later oracles.

No additional blocking source defect found in the remaining scope. Direct call
uses actual builtin CDLL/NOW/LOCAL, exact eight builtin pointer/length arguments,
fixed c_int and four control signatures, real installation/files and strongly
retained NUL buffers. Fixture/package/native loading and output-capture setup
precede entry. Fallback is registered BEFORE arm and runs after raw oracles, also
on setup/assertion/marker/KeyboardInterrupt paths. Actual31 raises the exact named
missing-owner exception immediately before every unreached ownership/phase
oracle; no idle phase wait or fake accepted integer replaces it. Unknown
installation/arm/poison/disposal cause is fixture failure, not intended red.

Nonconsuming own-thread and group P_ALL waits reject live/zombie children; actual
controller binding metadata plus P_PIDFD WNOWAIT and ready poll distinguish the
pinned guard from a pipe, and fresh same-thread ECHILD confirms consumed status
without the Python test reaping. All parent FD numbers are fstat-checked before
fallback; generation replay allows legitimate numeric reuse, bounds live8/
total12, requires releases before sole final39 decision and forbids parent
activity afterward. Ordinary actual status excludes125; competing reap uses
the controller's distinct observation. No incomplete journal is claimed to prove
unrecorded resources: full instrumentation/source review remains mandatory.

The base-phase ACK interpretation is sound independently of Ops417: native§7
requires actual adoption/release BEFORE AFTER continuation, but does not assign
that earlier resource event the future marker's edge tag. Same operation base
plus earlier SAME-parent sequence expresses the frozen temporal rule; the test
must retain that ordering, not require an invented edge value. Findings above
strengthen actual result/identity and prerequisite coverage within that rule.

Signal rows require actual controller send, child readiness, actual native18
final observation, real KeyboardInterrupt and unchanged handler/mask. Deadline
requires the literal offset/readiness plus real kill/reap and prompt return.
C1 requires a subsequent real successful phase35 kill. Case28 independently
decodes all248 bytes/PID/scan5/set0..5, flags/access/blocking masks, mask0, four
raw default actions and complete EOF before fallback. The already reviewed
fixed-child foundations remain prerequisites for original protocol bytes; this
suite does not invent another diagnostic channel or infer exec from intent alone.

Actual static verification: stdlib ast.parse and literal-only tuple decoding
(no candidate import) PASS:12 undecorated methods, groups3/7/2/10/1/8/18/3/2/1/1/1,
57 unique rows, all29 independent Linux phase/errno/native-result triples, and
independent struct-size arithmetic248. No module/test/helper/audit import,
compile/load/native/SDK/child/signal or execution occurred. Project/render/staged
diff checks PASS426 tasks/122 decisions/current HTML. After separately authored
correction, actual different source rereview, BOTH-host helper/installed-SDK
GREEN and matching build/entry source gates must still precede any FIRST31.

## Actual correction rereview — approved source

Reviewer B, fresh isolated `review/SCHEMATOOL-001-native-owner-test-correction`
from root`3a055ace8e5c4566e5e86c65027b043fb1da6f4f`. Reviewed actual corrected
test blob`35cff4b78418d5b3505932802796c19d0993bd65` and root's written reason.
**APPROVED for the later native57 FIRST source gate.** Both findings above are
resolved in this revision; their historical record remains. This is test-source
approval only, not B's fixture/child/Ops source approval or execution permission.

W14/W16 now require actual positive read/poll progress. Successful closes,
pipe2 and kill require literal0, including the real-SIGINT AFTER-close row.
W5/W6 require the returned FD to equal exactly one prior actual same-base adopted
FD/result; W5 also enforces the normalization floor5. P3 requires prior config1
and activation2; P4 requires config1, activation2, outcome2 and null1. Their
phase identities and earlier same-parent sequence are checked against the
selected marker. Native§3 explicitly prepares pipes, opens null, then normalizes;
these minima implement that contract and matrix requirement, not Ops417's code.
Existing generation replay/release/EBADF/final39 proof still applies to all those
acquisitions. No fake source phase or parent-stdio mutation is introduced.

Actual diff is ONLY20 additional test-source lines, no removed or changed old
assertion/row/outcome. Independent stdlib AST comparison confirms every row table,
all12 test methods and every other class helper unchanged; only assert_evidence
is strengthened. All57 row IDs/native/status expectations, buffers/real C call,
31-before-unreached-oracles behavior, child identity, pinned/P_ALL checks, packet,
no-log and unconditional post-oracle fallback remain intact.

No candidate import, helper/test/audit/native/SDK/thread/child/signal execution.
AST preservation and project/render/staged diff PASS426/122/current HTML. Actual
BOTH-host helper/SDK/kernel GREEN, matching artifact/build/source admission and
root's separately authorized first-red observation/attribution still precede
FIRST. No common owner body exists or is approved by this review; public74 ports,
all preservation/security/source/acceptance/CI/normal-merge gates remain distinct.
