# ACCTTEST-004 concrete service API/codec document author evidence

Author bundle_implementer,2026-11-03. DOCUMENT ONLY, not independently approved,
not an API/header/stub/fixture/test/red or service implementation. Root's actual
claim5236d88519284bc5e79e7d9b9074bbbbf5beb42f preceded these edits in fresh isolated
task/ACCTTEST-004-service-api-author, from normally integrated main
efe1c7e87ae2a7e8a92c3f5e7ede88e68e72b089 after PR48. Old STORAGE-016 source tree
ce194761 and all older work/reviews/raw evidence are preserved.

Actual concrete candidate d421471b927636cd159794adea223d5b5d49b3c2:
[application-service-api-v1.md](../contracts/application-service-api-v1.md),
blob6ee28717e8aa78896180c1ac0cc22e31adf3bf2a. Only that document and this note are
owned. Root retains progress/HTML/build/presets/CI/schema/integration/publication.
Different reviewer receives the actual committed candidate; no prospective verdict
or self approval is claimed.

## Source and contract pins

The previously reviewed logical contract6d0cd67f487dae92a17d41eb07dda5b407c78a56
and ADR-0003 e971a47e64b7cabffa19e325cee698378a2d0a8a remain unchanged. This is their
next concrete boundary, not a repeated logical task or a claim based on old264 tests.
Read root/server instructions, ACCT-004 brief, architecture/workflow/test strategy,
logical/ADR and actual integrated ports. There is no scoped docs/AGENTS.md.

| Actual source header | Git blob |
| --- | --- |
| sqlite_writer.hpp | afc9f483da7d8e61a5e73715ca33819216e9e668 |
| sqlite_application_bundle.hpp | 8614bcef00ecbcbe2f95d24751ece82b6db537ce |
| sqlite_application_holds.hpp | b9043716536063d7dc47ca97b2874ba1ccc80586 |
| sqlite_retained_context.hpp | 55c82815986052b996a46c408ab616e68815a4f2 |
| core application_plan.hpp | 501112d64959d20f682420d33a260e7745f5b378 |
| core contribution.hpp | 65ad81ac0dc3fc808421258ab89b30ff610be085 |
| core timer_lifecycle.hpp | 69cad7023b9928371877d45955b800aed9996cd5 |
| core time_debt.hpp | f3cc819c16f3df907c0b68b0738e3aaef4f51950 |

## Concrete choices and preserved boundaries

- One bounded big-endian length-framed grammar defines complete command equality,
  every optional/present-empty choice and exact bytes. The physical intent's current
  CAS token is separate. Known malformed semantics is not conflated with healthy
  unsupported old format.
- Typed synchronous access/disclosure/current-policy/segmented-clock/actions and
  purpose-bound fresh-reference ports receive owned input and a finite revocable
  retained-reference read view, not a full session/SQL/writable store. All adapters
  remain explicitly unavailable until their real semantics are reviewed.
- Real queued BEGIN/COMMIT and matching normal completion own publication. The
  prepared Work/matching WorkCompletion pair is also the path used by enqueue;
  a test can wrap the actual committed body then throw before outer body return
  to prove committed/work_exception/no value, without a fake commit-failure enum.
  Exact metatype/invariant extraction prevents generic QVariant coercion or captured
  side-channel data from becoming a normal response.
- Early exact key/complete command, global ORIGINAL claims and ALL active holds
  precede current revaluation. Current plan head and all15 contribution columns
  remain independent of token coverage. Stale plan has no invented head payload.
- CORE005 numeric Main_after is never double-added: CORE002 receives the original
  per-occurrence clock-advanced timer plus only actual_main_added. Current caps,
  above-cap retention, debt-only zero rescue, signedMIN and whole-input overflow
  preserve the accepted numerical contracts.
- Whole-input holds name the retained checked head, not an unstored advanced token,
  and stage no success effects/jobs/claims/head/contribution mutation. Missing
  counter definitions are not silently initialized. Collision is a single explicit
  refusal rather than a token-generation loop.
- State/result/audit/hold observation and non-deliverable synthetic job meanings,
  output dispositions, exact storage mappings, bounds and owning-byte/item rules
  are specified for later independent fixtures. Framing bytes and logical owned
  DTO bytes are deliberately separate specified measures.

No new requirement conflict or additional storage decomposition is proposed.
Unknown current authority/configuration/clock lineage/action production meaning,
mixed timer/global outcome, post-finish/paused/recovery acceptance and signed
manual derivation/continuation remain explicit required owning-task enablement
gates. Existing ACCT-005/006, CORE-008/API-007, milestones/delivery/recovery and
full integration/platform/release obligations remain mandatory.

## Actual checks

Linux local document environment, Python3.14.8. No CMake/native/service/codec call
or old component test was run for this document phase.

- Independent scratch stdlib grammar/Fraction audit:
  python3 /tmp/acct004-doc-literals.py PASS. Script SHA256
  d5c2ec58d592b1adf873b878c95a3577b5ee031d605683c05d21f779156a86f4.
  It concatenates the document's independently frozen field bytes; it does not
  import any repository helper/UUT, write a fixture or invoke a feature.
- Command C0=58 framed bytes; CID0 variant66; present-empty reason62; one x/BOM/
  NUL/FF/A choice73. Logical owned C0=25/0items, CID0=33, empty reason remains25,
  one choice=32/1item. These are separate hand-derived formula oracles.
- Compound grammar audit: Clock39, effect88, SAS1=90, SAR1=182, SAA1=178 bytes.
  Literal constituent bytes/formulas are retained in the contract; these examples
  claim no healthy database/source closure.
- Independent exact additions:58/0/+10/cap60→60/0, components+2/0/+8;
  55/2/+10/cap60→60/0,+5/-2/+3;100/5/+8/cap60→100/0,0/-5/+3.
  Fraction equality confirms debt repayment+Main credit+cap remainder=request.
- python3 tools/check_project.py PASS396 tasks/122 decisions/local links/presets/
  current HTML. This compiles or approves no service baseline.
- python3 tools/progress.py render --check PASS; no shared output regenerated.
- git diff --check PASS. Before this note, changed-path comparison against claim
  listed only the concrete contract; no header/source/test/fixture/config edit.

Next actual gates: different DOCUMENT review and corrections, minimal API/unique
stubs/harness actual review, independent literal real-SQL fixture/test authoring,
full separate test review, selected foundation green/registration check and FIRST
same-clean intended missing-service/codec red. ACCT-004 production remains gated.
No live provider/network/user-state effects or permission request occurred.

## Actual first review corrections

Actual reviewer of candidate6ee found a material historical-detection mapping gap.
Root explicitly authorized this DOCUMENT correction. A new held result now requires
record_detection.recorded ONLY and the exact new creation outcome. Already_recorded
is a typed stage_conflict, because an old historical active version may now belong
to a resolved hold. Every disposition0–9 is mapped; impossible detection outputs
fail the unit rather than inventing a hold. Independent CS16 records h/d/v1 then
resolution v2: today's active set is empty; exact old detection retry returns old
v1 DATA but cannot justify held. No application effects/claims or ref regeneration.

The same correction explicitly extends host lifetime obligations to ALL outstanding
prepared work pairs, including unsubmitted pairs. Draining admitted work is
insufficient while an unsubmitted callable still references service/ports/writer.
No safe use-after-destruction guarantee or dependency ownership was invented.
Only these two owned MD files changed. Different actual review of revised bytes
is still required; no prior approval is carried forward.

The bounded codec/numeric reviewer independently reproduced all literal counts and
arithmetic, then identified two concrete precision gaps, released by root for this
same document-only correction. Per-method accounting now pins encode Command25 to
Envelope98 and decode Envelope98 to Command25, including distinct input/output
failure priority, actual decoded result items and opaque-record non-items.
Independent intermediate ceilings bound port/proposal/semantic temporary ownership
without changing old record quotas or pretending embedded vectors are returned.
Decoder scan/materialization/error stages and context mappings are explicit.
Per-occurrence CORE002 credit now explicitly consumes the CURRENT running timer,
including prior effects, before installing THIS numeric Main_after. CS18 freezes
58→60→60 for two requests+10 under cap60; it never resets the second timer to58.
No codec/service implementation or test invocation accompanies these corrections.

The full reviewer additionally required concrete echoed ReferenceResolution binding,
finite token-free ApplicationCalculatedProposal members and an implementable order:
calculate→prepare jobs→issue refs→complete token-dependent codecs/bundle. The revised
text supplies those exact fields; it never demands complete SAR1 before tokens exist.
Read-view invalid-key pure rejection, callback-local expiry/foreign error and actual
underlying WorkFailure are now distinct. Pure/local failures require the service's
one active cleanup when propagated; already-rolled-back retained/query/store errors
are mapped without another rollback. Actual unknown still dominates. Prepared-pair
lifetime and all preceding revised mappings remain subject to different review.

## Final narrow error-surface pins

After actual6160/3e49 inspection resolved the material findings, root released
two remaining documentation pins. Escaped callback-local view expired_callback/
wrong_thread maps exactly to outer port_contract3/invalid_output1 after ONE active
cleanup; the direct local exception tag remains distinct, no stale_session or other
writer error is fabricated, and actual cleanup unknown dominates. Missing Completion
on the owner thread returns enqueue Admission{invalid_work}, while make_work throws
PreparationFailure(invalid_work); both occur before actual enqueue/unit/callback.
No additional within-stage total ordering or new runtime behavior is claimed.
Different actual review of these final bytes remains required.
