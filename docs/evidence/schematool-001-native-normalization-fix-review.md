# SCHEMATOOL-001 — active-close fix and completed owner source review

Reviewer C independently reviewed author B's actual
`ece3dad564b0c765631e7129e8945ebc3252d318`, core blob
`e89a093c781136cac5e16e55ba98b97d4cc746c0`, on a fresh isolated note-only
branch. **APPROVED SOURCE.** This resolves the sole blocking finding from my
whole owner75 audit b857. Together with the separately reviewed codegen, R3,
scalar and observer deltas, I identify no remaining blocking owner source
finding. This is not a claim that acceptance, generated proofs or CI have passed
on the corrected implementation, or that the task is done.

The test-first gate is actual, not prospective: I independently read the complete
correctly selected new-regression raw and receipt at clean
`ae97dea38fee530b5f7ba6bc2d7e81c2dcc9ac7f`, UTC21:55:52.494014. It names the
real fault artifact and --artifact option; one method/one FAIL/.180s/exit1,
stdout0, stderr1087 bytes/SHA256
`5f4706f7965452458f2cef31a9c2781d15b5f939707bcfc05131af4fb9870c51`.
Receipt SHA256cc179aebb871f9f32b6f01afa4b8ded96e8dde7ad7451dc7a7a8716839b61e1d.
Only line89 fails: actual0 != literal18. Every earlier unchanged source08e3
assertion for real close0/release ACK before AFTER41, fstat EBADF and fresh
own/group P_ALL physical cleanup has passed. The later full assert_evidence
was not reached in that failing run and is not claimed as a red-run success.
The prior missing-artifact argparse exit2 ran zero tests and is excluded.

The actual fix changes only the three normalization work arguments from false
to true and updates the adjacent stale bug-preservation comment. I restored the
entire old header byte-for-byte by reversing exactly those literals/comment.
No type, operation, phase, cleanup call, test, oracle, return mapping, ownership
transition, priority rule, deadline or resource bound otherwise changes.

Normalization is active work under native section6: a real close EINTR now
records interrupted18; the reviewed negative AFTER41 continuation also records18
while preserving the actual kernel close0. Real close still precedes slot
invalidation and release ACK, and a closed number is never retried. The genuine
duplicate is still moved into its owned role before the existing decision check.
That check stops preparation before birth when interruption was recorded, then
ordinary cleanup releases the remaining identities. close_bare's default false
and every cleanup caller remain exact, so cleanup failures retain their separate
classification. Existing known-resource priority and final post-quiescence
commit path remain intact. No result, signal or successful validator is forged.

Old57, new regression08e3, FaultOps2f772f7b and fixturee669ee35 are unchanged
against the actual admitted baseline. No revised assertion is used to approve
the fix. The completed source audit still includes all bounds/copies, ABI inputs,
whole deadline, config snapshot, atomic PID/pidfd adoption, eight descriptor slots,
raw child setup, protocol/EOF/status, same-thread reap and cleanup review recorded
in b857; generated whole-stack and runtime facts remain separate gates.

Root may integrate and rebuild this exact source, then run BOTH approved combined
static auditors using fresh matching ELF/CI/SU records. Require unchanged strict
helper/child/clone/return/TLS/PLT/indirect/trap/export/eager/no-initializer gates
and inherited stack totals <=32768. A refusal halts dependent execution; no old
artifact hash/proof may certify the new DSOs. Preserve the observed red raw and
its command-error predecessor durably before final integration metadata.

Only after those new matching static gates pass, root may rerun the SAME one-case
normalization regression with correct explicit artifact, selected ordinary
CPython3.14 and env3. Require actual full PASS, including the previously unreached
literal18/no historical birth/final39/full ownership/caller-state/no-log oracles.
Only after that succeeds may root run the reviewed full native69, binding9,
public74 and loader-cleanup1 acceptance against the corrected integrated source
and artifacts, under the same reviewed environment/finite bounds. This is a
conditional execution release, not a predicted pass. Preserve all outcomes;
any new uncovered behavior defect returns to a separately reviewed regression
and reached red before its fix. Reuse only genuinely unchanged prerequisite
proofs; if foundation binding inputs change, establish the exact bootstrap first.
Latest hosted CI, actual outcome attribution, completion metadata and merge gates
remain separate; no publication/ready/merge claim follows from this source note.

Actually performed: actual full delta and surrounding ownership/decision path
inspection, complete header-inverse and four unchanged-blob checks, raw/receipt
hash/cause/source analysis; clang-format dry-run/Werror, project426/122,
render/diff PASS. No compiler, candidate import, checker/test/SDK/native call,
ELF load, thread, child, signal, probe or baseline replay ran in this review.
Only existing PR77 is in scope, then stop.
