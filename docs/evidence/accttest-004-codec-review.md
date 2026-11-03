# ACCTTEST-004 — independent bounded codec/arithmetic document review

Reviewer bundle_validation,2026-11-03. **Final bounded DOCUMENT approval:8fa0c76 /
author608acc1**, as recorded below. The initial **changes requested on actual6ee28717**
record is preserved. Scope is ONLY framed grammar/literal bytes, logical B/I
and limits/decode ordering, absence/empty/copy consistency, CS01–03 and CORE005→002
composition. Main reviewer separately owns complete service semantics/ports/order.
This is not API/header, fixture/test/red, runtime, implementation or policy approval.

Actual [document](../contracts/application-service-api-v1.md) authored d421471 and
integrated ba6ae75, blob `6ee28717e8aa78896180c1ac0cc22e31adf3bf2a` (641 lines).
Inspected actual author note bd9b354→roote7e6e1c, path
`docs/evidence/accttest-004-api-author.md`, blob
`91636d764b8cc6dd02766060b7e2136166f60505`. The author's reported calculations
were not accepted as independent evidence; the checks below were separately derived.

Review tree `task/ACCTTEST-004-codec-review` at ab9ca61695bffef161e35505a7d5486dad747ec7
was created by root from integrated efe1c7e+claim5236d88+actual document before any
review evidence edit. Own only this note. No proposal/source/header/test/fixture/
configuration/progress/old-tree edits or native/old acceptance/red replay.

## Independently verified

Authority: specificationv0.45 §3.5/§4 and NUM-01–04; logical service
6d0cd67f487dae92a17d41eb07dda5b407c78a56;
time-debt contract909f16e7ee6f4e9de6dc31acc728236032d51482;
timer-lifecycle contract40de8137fd546a406b4dd195a3ac5d594c07f78d and actual public
core layouts. No repository function, future codec, test comparator or author's
script was imported or executed.

Independent stdlib struct/integer construction agrees byte-for-byte with all FOUR
actual hex blocks (SAC1 C0, Clock, SAS1 and Effect). Independent compound assembly
also agrees with SAR1/SAA1 lengths:

| Value | Framed bytes |
| --- | ---: |
| C0 |58 |
| contribution/CID0 variant |66 |
| present-empty reason variant |62 |
| one x/six-byte choice variant |73 |
| Clock l/t/UTC1000/one zero-elapsed segment |39 |
| SAS1 configc/basisa/policyv/Main58/Grace90/debt0/empty balances |90 |
| CS01 Effect |88 |
| SAR1 |182 |
| SAA1 |178 |

Logical command B/I independently gives C0=25/0, CID0=33/0, present-empty reason=25/0,
one choice=32/1. Optional wire engagement differs from logical ownership: absent
reason is one byte; present-empty is five wire bytes but adds0 logical data bytes.
The choice contains exact EFBBBF00FF41; no text normalization/NUL truncation applies.
U64MAX and signed I64MIN retain their distinct typed big-endian representations.

CS01: debt0,Main58,+10,cap60 partitions2 Main/0 debt/8 cap, after60/0. CS02:
55/2,+10,cap60 partitions5/-2/3→60/0;100/5,+8,cap60 partitions0/-5/3→100/0,
without clamping old Main. CS03:0/5,+5,cap0 repays5 with actual Main0, so Grace/wait
is preserved; +7/no cap repays5 and gives actual2, so CORE002 may rescue while
retaining unused Grace. No requested/debt-repaid/capped amount is used as rescue.
The CS13 signedMIN examples also agree: Main1 leaves debtMAX; Main0 requires2^63
and cannot produce a signed64 final debt. No service or numeric implementation ran.

E's explicit duplicated typed-field checks, absent/empty tags, canonical Exact
sign/zero/gcd grammar and no coercive unsupported-format parsing are sound targets.
Compound byte anchors explicitly do not claim a healthy database/source closure.

## C1 — exact accounting boundary targets need specification

F lines560–600 gives general counting rules but does not identify which ACTUAL
owned value each cap measures for encode/decode, port outputs, temporary semantic
decoding and the final response. This prevents unique independent boundary oracles.

Concrete counterexample: encode C0 takes CommandB25 and returns an Envelope with
format40+document58=98 owned bytes; decode takes98 and returns25. With
input_owned_bytes25/result_owned_bytes97, the document does not determine which
call must fail or which value those caps charge. Similarly, choices_per_command2
versus result_items1 for decoding a two-choice command lacks a specified result.
Whether command_bytes counts its encoded document alone is described; it does not
resolve the independent logical ownership cap target.

Provide a per-boundary table naming the measured value, effective field/count/wire/
owned/item caps and category/priority for encoding input/output, decoding outer
envelope/decoded command, port/intermediate values and actual final Response.
Distinguish wire cursor/length/count validation before allocation from semantic DTO
key/count/body/conditional checks and healthy configured overage from malformed
known documents. Do not silently decide these in the future codec.

In particular, final Response records/hold sets must retain their EXISTING published
B/I formulas. Their encoded command/clock documents are opaque owned bytes: parsed
temporary choices/segments are not extra vector members of the returned record.
Port/intermediate limits may be separately bounded, but may not become phantom
final-response items or a global history quota. Main reviewer independently agrees
this precision is required. No product policy decision is needed to document it.

## C2 — pin the running per-occurrence timer

D lines350–353 says CORE002 receives the ORIGINAL clock-advanced TimerState "for
that occurrence". Pin it unambiguously as the CURRENT running pre-occurrence timer,
including all earlier requests' results, before installing THIS numeric Main_after.
It must not mean the one fixed initial advanced-before snapshot for every request.

Independent two-occurrence example: Main58/debt0,+10 then+10,cap60 gives actual
Main2 then0 and numeric Main60 then60. CORE002 must consume timers58 then60.
Reusing initial58 on the second actual0 would incorrectly retain58 and fail the
document's required numeric/timer equality, blocking a valid complete operation.
This is a wording/composition correction, not a new cap/Grace/pause policy. Main
reviewer independently agrees. Single-occurrence CS01–03 values themselves are correct.

## Checks and remaining gate

Actual scratch command `python3 /tmp/acct004-codec-independent.py` PASS, using ONLY
stdlib framing and exact Python integers, reading the actual document's literal
blocks. Script SHA25665986fbb2d475157058e8eb4293f2dd4290a32f87cd651ece5ca00ffdf9542e6;
output SHA256325f189223ba4cc86a9f2d0f2bdfbeefa4629996d593701b39c324059621ee67.
Source/core/SQL/service/codec functions and old acceptance suites were NOT executed.
Document static/link and diff checks accompany the evidence commit; they do not
approve future implementation or establish a failing baseline.

Both findings were sent to root, author and main reviewer. Await the actual corrected
document and author note, then re-pin/review within this bounded scope. All other
semantics and phase/release gates remain the main review/coordinator's responsibility.

## Final actual corrected-document re-review — APPROVED within bounded scope

Actual author correction `6160e352613b2c1e5e0776099be9c648fd7d6a95`→root073c51e
gave document3e497030/author1318dc51. Then inspected the COMPLETE actual narrow
`ee05694510dbe3892d97a56ce52c65441a3a08b4`→rootde73386 delta. Final approved
document blob is **8fa0c76ad90ad046712463dfc95206e2a99b4e40**; actual final
author-note blob is **608acc129c48f8ee5ed97fd09db1ade541511c56**. Both were
verified from the imported actual authored commits; no proposal bytes were edited
by this reviewer. The final error-surface delta changes no codec, B/I, numeric
example or timer-composition requirement. Its full semantic verdict belongs to the
main reviewer; this approval is complementary and bounded as stated above.

**C1 resolved.** SectionF now names the ACTUAL measured input/output at each public
codec, admission, intermediate and final-response boundary. Public pure output
overage is explicit result_limit7; intermediate/storage semantic overage after
BEGIN is a known result_limit with actual rollback. Intermediate limits are
independent per value, distinct from final Response and existing store profiles.
Actual opaque envelope bytes count, but parsed temporary choices/segments/effects
are NOT phantom vectors in a returned RecordedBundle. Fixed Completion/admission
metadata does not become hidden response data. No global history quota is added.

Decoder order is now explicit: options, outer envelope fields, actual input B,
unsupported outer format, structurally bounded byte-view scan, typed preflight,
output B/I projection, then materialization. Unreadable/truncated/trailing frames
fail without inventing unreachable later fields; complete frames retain key/source/
presence before configured counts and bodies/conditionals. Internal retained decode
uses intermediate/store bounds and separately maps unsupported versus malformed
versus overage, rather than reusing caller-input aggregate policy. This resolves
the fixture-oracle ambiguity without claiming an implemented decoder/allocation audit.

Independent CS17 derivation from the grammar (not author's script/UUT): fixed
command format is40 bytes, so C0 Envelope B=40+58=98. Encode Command25 with
input25/output97 passes input but exceeds output→specified pure7. Decode that98-byte
Envelope with input25 fails input6 BEFORE parse/output. With two EMPTY x choices,
document=58+2*(B(x)5+B(empty)4)=76, Envelope116, decoded Command27 and actual I2.
At choices_per_command2/result_items1, decoded output fails7 while encoded opaque
Envelope I0 fits. A proposal ownership illustration with two TimerState19 copies,
debt8 and ClockObservation30 (five one-byte refs, UTC8, one17-byte segment) is B76/I1,
consistent with the newly explicit actual members and no framing/hidden-copy charge.

**C2 resolved.** D now explicitly feeds CORE002 the CURRENT running timer BEFORE
THIS occurrence, including all preceding actual timer changes. Only the first uses
the initial clock-advanced snapshot. CS18 independently gives58→60→60 for requests
+10,+10/cap60, actual2 then0, capped8 then10. Neither resetting to58 nor adding actual
credit again to numeric Main_after is allowed. CS01–03, above-cap debt repayment,
zero/positive Grace/wait rescue and signedMIN arithmetic remain unchanged/correct.

All original literal hex, lengths58/66/62/73/39/90/88/182/178 and logical25/33/25/32
were re-derived against the final actual document and still match. Optional absence
versus present-empty and raw BOM/NUL/FF bytes are unchanged. Cross-field duplicated
typed values remain required to agree; synthetic examples remain byte anchors,
not healthy source/platform/provider fixtures. No unrelated codec/numeric scope
regression was found in the actual corrections.

Actual command: `python3 /tmp/acct004-codec-independent-final.py` PASS, stdlib
struct/integer arithmetic ONLY, with no repository helper, author's script, native
core/service/codec/API/stub/fixture/test/marker invocation. Script SHA256
100e4402c471b8e1d276acce5fcb99b8c468c194fb0167b247e98fbb1c949b5a;
output SHA2566a91ef93be4043e8174225574b58e9e26902d75576525be7df50a81d22e45b28.
Static project/link and diff checks pass. The earlier changes-requested record and
its independent scratch artifacts above remain preserved.

This APPROVES only the actual final DOCUMENT's bounded codec/accounting/arithmetic
scope. It does not approve a future header/layout/stub/harness, source allocator,
independent fixture/test or intended-red baseline, service implementation, authority
adapter or production policy. Main review/root retain all other actual gates.

## Actual fixture-golden clarification — bounded DOCUMENT approval

Inspected the complete actual `aba965732eb38bfa81aad0d02fcb6a7d60d44d58`
delta, imported unchanged as0ad12d1. Approved bounded document blob is now
**862c419395f4e9daf34f8ae8790a06abbec6f410**, author-note blob
**78f36dd7a3f0d0ad70cefb250f7af2a6b45c791c**. The earlier approval and
changes-requested history above remain preserved. This addendum approves only the
actual clarification's codec/ownership/copy-consistency scope, not any fixture.

The field table resolves the actual ambiguity: successful bundle annotations use
SAC1/SAS1/SAR1/SAA1 success0; all four applicable newly created numeric-hold
annotations are identical complete SAH1 envelopes. Policy holds have three such
copies and no fabricated failure. The embedded Failure is a finite leaf without
FailureEvidence.annotation, so these copies do not create recursion. Each real
owning copy remains separately charged by the existing hold B formula; no vector
or optional-engagement item is invented.

Source request explanations and policy/action port explanations are copied exactly.
SAA1 calculation and SAH1 explanation DATA are non-null empty, while advance text
is empty for exact head reuse or the exact20 ASCII bytes `clock-observation-v1`.
This supplies deterministic golden values without adding generated numeric prose.
Present-empty command actor/decision values remain lossless in SAC1/SAH1 and bundle
audit DATA, while the existing nonempty typed hold-key domain projects those values
to absence. Unknown Policy/Clock and optional UTC/elapsed remain honestly absent;
no timestamp or replacement identity is invented. Cross-field equality requirements
are preserved by these explicit projections rather than accidental normalization.

Independently reran stdlib-only manual grammar/arithmetic derivation:
`python3 /tmp/acct004-codec-independent-final.py` PASS; its new output file
`/tmp/acct004-clarification-anchors.txt` SHA256 remains
6a91ef93be4043e8174225574b58e9e26902d75576525be7df50a81d22e45b28.
All original literal anchors, lengths and CS01–03/17/18 arithmetic are unchanged.
An additional independent stdlib framing calculation constructed a49-byte numeric
Failure leaf and213-byte SAH1 using the documented C0/Policy/Clock grammar anchors:
format49+document213 gives one Envelope B262 and four copies B1048. A separate
unknown-Policy/Clock/missing-policy example is97 wire bytes with no invented times.
These are grammar/accounting illustrations, not healthy source/hold fixtures or
new required literal acceptance vectors. Present-empty wire `0100000000` remains
distinct from absent `00`; typed-key projection does not rewrite that wire input.

The new verification partition changes no wire grammar, B/I formula or arithmetic.
It expressly separates future public runtime observations, old component proof and
actual source audit; it adds no write-capable fake or observed private/race claim.
Complete service-semantic approval belongs to the separate full reviewer.
Actual `git diff HEAD^ HEAD --check` and `python3 tools/check_project.py` PASS
(396 tasks/122 decisions). No service/header/stub/factory/native/fixture/test or
marker was invoked; no proposal/source/configuration bytes were edited.
