# ACCTTEST-004 — independent bounded codec/arithmetic document review

Reviewer bundle_validation,2026-11-03. **Changes requested on actual6ee28717**;
no document approval yet. Scope is ONLY framed grammar/literal bytes, logical B/I
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
