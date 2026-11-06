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
