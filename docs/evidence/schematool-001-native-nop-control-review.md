# SCHEMATOOL-001 exact NOP-alias control — independent source review

Reviewer C (`session_plan_review`), 6 November 2026, PR77 only. Fresh isolated
branch from actual `1a467f23cb53f2cb3c7c0f1f7ee14d6cbbfe54a9`; root authored
the additive control/reason. Exact test source is
`tests/schema_validation/native_entry_controls.py` blob
`10b86dfa44b5bf511bdca9f21967fb6a07a015ea`; checker remains unchanged
`f79780a61254c35991af6674328fac2f81f29477`.

**Approved: this actual ONE-method/three-observation source. Release only the
FIRST selected static control run against the unchanged gate.** Select
`NativeEntryControls.test_two_byte_nop_alias_requires_exact_ax_identity` through
the script's ordinary unittest CLI, under the reviewed minimal environment and
selected interpreter; preserve actual source/argv/raw/status/time. No existing
native red, test/ELF/kernel probe, helper compile or future checker correction is
released. Expected algorithm refusal must be distinguished from an import,
setup or fixture failure.

The positive literal replaces only the PLT's trailing padding tuple with actual
observed `xchg %ax,%ax`, then requires the exact known GOT metadata. Register
identity leaves the value unchanged and does not access memory or introduce a
callback. The [primary GNU binutils NOP source change](https://sourceware.org/pipermail/binutils-cvs/2018-February/047566.html)
lists66/90 as this two-byte NOP spelling. It also matches my previously inspected
actual local PLT4446. Different-register `%ax,%bx` and memory `%ax,(%rax)` are
independent negative controls; they must continue to refuse, even though they
appear in padding position. This is no blanket XCHG allowance.

All previous seven methods/helper ASTs and21 observations are exact. The audit
implementation is unchanged, so the initial positive is expected to reach its
actual `RuntimeError: unreviewed syscall PLT operation`; a unittest ERROR caused
by that specific checker frame is genuine reached algorithm refusal, not a
missing-module error or feature/kernel red. Negative rows will become reachable
after the later narrow correction; their green is not presumed now. Source
approval of this test is not prospective approval of that correction.

The only target is pure dictionary/string `probe_entry_binding`. Audit definitions
load under a non-main name as already reviewed; no ELF/proc discovery, binutils,
native library entry, child, owner or validator operation occurs. Actual initial
seven-control pass/audit rejection have only been reported by root at this stage;
their raw attribution is separate from this source verdict.

Actually performed: source/diff/primary-reason review; Python3.10 AST/compile
without exec; exact old-method/helper AST preservation and unchanged gate blob;
project426/122, render--check and diff--check PASS. Neither candidate module,
selected control, binutils nor any native program was executed by this reviewer.
All helper/whole-build/generated/runtime/both-host/owner31/74/CI/merge gates remain.

## Actual selected red and exact alias correction

Reviewer C, actual immutable `e04aa62bf1945d192b711cff93c4b249c3fc702e`, fresh
isolated review branch. I read/hashed all preserved initial-entry control/audit
and selected-NOP raw/receipt files without replay. **The reached selected static
refusal is accepted, and actual corrected checker
`77bd72bd2c9af6e6d600328379e2f8cff00ad998` is approved for unchanged static-control
green and static audit of already-reviewed existing artifacts only.** No new
helper compilation, candidate runtime or future source is approved.

The selected receipt records source `918733424dc66fa49c1adf99e34628ef71beaa20`,
after actual C085 source review, exact qualified ONE-method argv and start
18:01:33.296012Z. Native1 test/0.001s, exit1 and outer0.042579s match the full
raw. Stdout is empty; stderr1511 bytes/SHA256
`d6aa9f69a230d9216ce1d290dc8debb4b4445789f42a8760d8fb6b4aefcd71bb`.
The sole ERROR reaches the positive at test line64, actual probe_entry_binding
line230 and require, ending exactly `RuntimeError: unreviewed syscall PLT
operation`. No import/setup/fixture cause appears. Checkerf797 is exact at both
earlier b3b and selected918; test10b86 is unchanged through the correction.
The observer note reports minimal environment; this selected JSON does not itself
list its keys or watchdog, so I do not invent those missing receipt fields.

The earlier seven-method raw is958 bytes/SHAd88466d7..., all7 pass/0.001s/exit0.
The old-local artifact audit raw is987 bytes/SHA97bacba9..., actual fail-closed
at the same PLT gate/exit1. Both receipts record the explicit minimal three-variable
environment and sourceb3b. These are static harness observations, neither kernel
failures nor owner red. Complete raw hashes match all three receipts; the original
FIRST artifacts/cases were not replayed by this reviewer.

The actual implementation diff adds only `mnemonic == "xchg" and operands ==
"%ax,%ax"` as a skipped decoration. Other register widths/register identities,
memory operands, calls, TLS and alternate PLT sequences remain refused. No public
value or unknown-operation allowance changes. This matches the source-reviewed
positive and preserves both independent negative controls. Root may now run all
eight entry methods/24 observations, original eight audit controls and the static
artifact gate; a future green is not presumed by this source verdict.

Actually performed: complete raw/receipt/hash/frame/source chronology and exact
two-line diff review; syntax-only Python3.10 parse/compile; project426/122,
render--check and diff--check PASS. No module import, control replay, checker/
binutils run, configure/build, native executable or probe was performed by me.
