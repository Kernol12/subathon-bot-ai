# Independent document review — CONTRACT-014

2026-11-05. Author: `/root/selected_boost` (reference_author).
Reviewer: `/root/access_implementer`, a different agent/session from this document's
author. Review-only branch `task/CONTRACT-014-document-review`,
`/tmp/subathon-contract014-review`, actual candidate
66c4547f4d7b3558b181fb47f832ff88facbcf90 containing authorad563175.
Contract blob: `c4128ab0eabb1290635fe918819292a2a69606c8`.
Owned only this NEW review note; no contract/source/tests/shared state changed.

**Verdict: request changes.** Full document inspection is complete. The concrete
consumer proposal has two missing interface obligations and one clock-observation
ambiguity. These are technical contract corrections, not owner product decisions,
permission requests or a reason to change agreed scope. No dependent source/test
stage or final contract acceptance is released by this review.

## Actual scope inspected

Read root AGENTS/workflow/task014, ADR0006 and the reference logical profile;
specification §§3/4/6/10/17/18 including NUM-04/AUTH-03/06/07; the entire571-line
new contract and author note. Compared actual receipt/contribution/plan, writer
adoption, retained-context, bundle, application-service, effective-permission,
timer-lifecycle and debt headers/contracts and relevant implemented orchestration.
This review does not repeat or replace the earlier logical-profile/component
reviews. No native/product program or new behavior was invoked.

The proposed SRC1/SRA1/SRX1/SRF1 byte grammar, integer precision, principal kind,
immutable configuration/context links, borrowed preflight/result budgets, resource
scope composite, owner/delegation boundary and no provider facts by inference
are consistent in principle. Existing ACCT001–004 imports retain their true
single-writer/session/commit semantics. The contributor supplies no amount/grant;
request-sized current authority precedes protected result lookup and is rechecked
for actual historical records/hold subjects. Cap-limited actual effects, exact0,
NUM-04 hold, known rollback versus unknown, no replay after uncertainty and
post-COMMIT body failure are preserved. RP13 authentication/HTTP remains separately
gated. These positive findings do not approve the still-incomplete interfaces below.

## R1 — required durable capture lookup is not a concrete contract

Section3 step3 and its following paragraph explicitly say the source→initial-plan/
execution lookup is not in the existing retained API and leave its mechanism for
API011. That is an honest runtime limitation, but the necessary consuming
find/retain/binding interface itself is also missing. `SqliteRetainedContextStore`
only finds exact `(kind,reference)` and `SqliteApplicationPlanStore` only finds
known plan refs/revisions; neither can discover the source's FIRST plan/SRX1.
Without concrete data/signatures/refusal/error/atomicity rules, two capture units
can choose competing plans after a retry or interruption. Task014 explicitly
requires durable capture signatures/formats/limits before downstream baselines;
"provide/review a narrow lookup later" does not finish that contract acceptance.

Required correction: define a narrow owned initial-source/capture binding and
concrete consumer find/retain interface now. Pin exact identity scope (including
actual receipt/CID/source/install/config/arrival/plan/execution bindings as needed),
unique first-writer meaning, exact already-present versus conflict/missing behavior,
no partial writes, current caller-session ownership and provisional status until
actual commit. Retaining the initial plan/SRX1/index must be ONE real unit, with
failure and competing candidate handling explicit. Pin bounds/preallocation/error
mapping and how referenced immutable records are validated. Explicitly map the
original normalized `ReceiptInput`/`ReceiptArrival` fields, particularly
configuration_ref and modifier_context_ref to SRC1/SRA1, so capture/discovery uses
one concrete meaning rather than an implied new private method.

Assign the actual new storage producer/schema/migration/independent baseline/red/
implementation to explicit owning tasks and add them as REQUIRED prerequisites of
both API011 records/owning epic while preserving every old edge. Root has proposed
unused IDs STORAGETEST-018/STORAGE-018 for this purpose; their actual definitions,
DAG and revised contract must receive the later independent review. No actual
schema/migration/header/source or producer implementation is needed in this
DOCUMENT correction, and no absence is converted into a completed capability.

## R2 — admission/refusal has no typed callable outcome

Section1 says unknown categories return not_ready and known unsupported categories
return unsupported_configuration; `ReferenceUnavailable`/`ReferenceGate` exist.
But the proposed `evaluate_reference_rule` returns only `ReferenceRuleEvaluation`,
the failure enum contains neither gate, and the strict configuration admission
function remains a "future" function without signature/result/failure mapping.
The raw DTO cannot represent nonempty unsupported categories, so a caller could
project those facts away before invoking the only concrete derivation signature.
The contract says derivation must check admission, but provides no consistent
channel to reject it with the stated typed disposition.

Required correction: define the concrete read-only admission consumer/data seam
and its exact success versus unavailable outcome, or an equally complete explicit
availability/failure channel around derivation. Pin which source establishes the
complete unprojected category/trigger/current-state facts, the immutable binding
checked, how a valid admitted configuration reaches rule derivation and what the
existing proposed pure entry does for unknown/unsupported facts. Distinguish
malformed tags/values from complete known unsupported configuration and genuinely
unavailable facts. Do not invent an allow bool or consider the DTO's `complete`
tag an authority certificate. Actual configuration/current-authority producer
implementation may remain a named future gate; the consumer signature and mapping
cannot remain unspecified when this contract task becomes done.

## R3 — zero elapsed with a fresh observation reference

Section5 says a NEW observation ref is assigned before the callback and the callback
uses that reserved observation. It later permits no segments for an identical
anchor/UTC. The unchanged service's `check_clock` requires an empty segment vector
to use BOTH the existing observation_ref and observed UTC. A new reserved ref with
empty segments is therefore a real invalid-binding rollback, even when elapsed0
and UTC unchanged. Clarify the exact branch: reuse the actual anchor ref only with
an unchanged observation/binding and empty vector, OR retain the fresh reserved ref
and include an explicit zero segment even at unchanged UTC. Define disposal of
any unused reservation accordingly. Preserve the changed-UTC zero-segment rule;
never fabricate elapsed from UTC. In the policy table, the actual imported member
name is `inputs.current_state.configuration_ref`, not `inputs.state`.

## Independent literal checks and limits

A new Python standard-library/Fraction script independently checked19 facts, all
PASS: RP01 actual120000/capped480000/after3600000, RP09 actual60000/capped540000,
RP10 actual0/capped600000, r1/r2 amount distinction, RC01 advance3450000 and
actual150000/capped450000, 1+INT64_MAX overflow, exact integer/residual0 and zero,
37/36-byte issued refs,12304-byte4096-component source composite/its overflow of
the default4096 profile, and64×8=512-byte decoder-index payload. These are simple
document oracle checks, not executed product/schema tests or generated SUT values.
The resource formulas distinguish semantic B/I, encoded frame bytes and backend/
allocator overhead; imported permission/service/store ceilings remain independent.
The static bounded grammar fits the proposed maximum64 borrowed records and depth3.

Actual commands in this review tree:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0:417 tasks/122 decisions/local links/presets/current HTML and whitespace.
The script above used only independent literal arithmetic/Fraction and printed19
passed named checks. No CMake/native/feature/SDK-runtime or schema-validation run
is claimed or needed for this Markdown-only review. Current automatic document CI
and later latest-head PR/rules/discussions gates remain the coordinator's work.

## Required next handoff

Route R1–R3 as bounded corrections to the original document author; root owns the
explicit018 prerequisite planning and all shared task state/HTML. Re-review the
ACTUAL corrected document and new task/DAG artifacts before accepting CONTRACT014.
No provider/session/delegation/current-port/runtime capability is certified by an
interface document. Real adapter tests/red/implementation, selected CI, supported
platform/recovery/HTTP/SQLite end-to-end checks and the complete seven-integration/
three-FX/client/renderer/localization/theme/package/license/live scope remain.

## Actual corrected document and prerequisite-plan re-review

2026-11-05, SAME independent reviewer `/root/access_implementer`; original author
`/root/selected_boost`, corrected author revision
7bca3ba5b942fe5edfec51a6340b513b35457f82. Examined combined root candidate
84d087c5f5457eb98ca7fe5fba55c15b2185060c, including required-prerequisite plan
4018afd09c565f487bdc4283d64365f30ace351d. Merged that actual candidate into the
isolated review branch without altering its artifacts, then re-read the complete
corrected document/author evidence, the new018 brief/state/DAG/navigation changes,
and compared affected obligations with the actual unchanged imported interfaces.

Exact examined blobs:

| Artifact | Git blob |
| --- | --- |
| corrected reference application contract | `5a12ebea1ecce7192c57db3462427bc3cfb9bfd6` |
| corrected author evidence | `8235917484be08fd8fd27640b33eb37a7d6fb677` |
| new STORAGE018 brief | `57852bbbc8b37f7fc1683cfccf57726dcfdc9183` |
| combined task state | `e6ef92f574b93c80505c1fb51de86c138176f997` |

**Verdict: approved for integration of this DOCUMENT/PREREQUISITE PLAN only.**
R1/R2/R3 are resolved in this actual candidate. Original findings above are retained
as the superseded candidate's review, not removed or silently reclassified. This
approval supplies no future API/schema/test-red/source/runtime acceptance.

### R1 closure and actual planning

The revised document now defines ReferenceCaptureKey/ReferenceInitialCapture,
ReferenceCaptureLimits, exact find/retain signatures/refusal alternatives and the
caller-session SqliteReferenceCaptureStore consumer. Key scope includes installation
and all four receipt-source components; receipt1..MAX/CID0..MAX/initial revision1,
exact first binding and installation+receipt uniqueness are explicit. The existing
receipt's immutable context/configuration/source and actual original support,
initial plan1 and SRX1 must all close over the exact candidate. `find` checks its
selected immutable closure and retains the distinction between absence and stored
corruption; advancing current plan history or contribution dimensions does not
rewrite the original capture. Missing/mismatched new references are separate
healthy refusals; an existing dangling/contradictory binding is corruption after
actual known cleanup, with uncertainty overriding the local reason.

The host finds first, stages plan/SRX/index in ONE real session, and commits none
of them independently. Refusal after earlier staging requires actual whole-unit
cleanup; no competing orphan plan/context may be committed as partial success.
Adoption/commit of the earlier receipt/contribution units remains unchanged and
separate from this plan unit. All returned storage values stay provisional until
actual acknowledged normal work completion. ReceiptArrival fields now explicitly
bind configuration_ref to SRC1 and modifier_context_ref to SRA1. SRI1 has exact
field order, integer/byte grammar, bounded borrowed scan, complete output projection
and explicit original-reference semantics. No hidden existing lookup, SQL bypass,
auto-BEGIN/COMMIT, process map or hash collision assumption is introduced.

The actual plan adds ONLY STORAGETEST-018/STORAGE-018, both still planned. The
baseline requires CONTRACT014 and accepted STORAGE012/013/014/015; implementation
requires those plus its own baseline. APITEST011 AND API011 require STORAGE018;
EPIC009 requires both new records. Independent comparison of pre-plan417 records
to the actual419-record state confirms ZERO removed old IDs/edges, exactly four
added edges on old records,11 prerequisite edges on the new records, and419 tasks/
1605 edges with no cycle. Every old task's verification value is preserved. All
old implementation→baseline gates remain; the new pair's own independent contract/
physical/API/test/red/source/selected-CI gates are explicit in the brief. No schema,
SQL/migration statement/version, old source, test or build/workflow file changes.
Navigation counts and actual rendered state match the new pair. Producer physical
schema/API validation remains a genuine018 prerequisite, not fabricated runtime
completion or a reason to leave this now-concrete consumer interface unspecified.

### R2 typed admission and R3 actual clock binding

The revised full configuration-facts DTOs retain unprojected enabled rules,
trigger/action/unit lists, conditions/exclusions/execution limits and category
sets, alongside explicit known/unknown state. A source unable to represent a
complete known unsupported owning configuration can return typed unsupported,
without constructing fake narrow fields. Concrete snapshot/load, admission,
runtime-admission and derivation variants define availability/refusal now; missing
runtime producer implementation remains explicitly gated under API011. The intake
host can use queue-owned snapshot without fabricating the private service read view.

All malformed tags/refs/shapes/values/bindings are checked before supported-profile
classification. Expected/actual/projection revision bindings must agree; unknown
with payload is not a partial success and contradictory complete projection is
invalid_binding. Known extra rules/categories/shapes/hard deadline are unsupported;
valid unavailable facts are not_ready, with nonempty fixed reason refs. Wrong gates
in that availability channel are invalid_shape. Derivation calls admission on the
retained arrival's exact version, propagates typed unavailability with no request,
and requires admitted configuration equality before producing one exact integer
request. Historical r1 cannot be replaced by current r2. Current runtime checks
remain separate from current delegated permission and cannot authorize an operation
merely by returning a complete aggregate. All five ACCT004 ports remain unchanged;
actual producer authority/currentness and complete borrowed/result bounds are
explicit requirements rather than a bool or permissive fallback.

The two clock branches now match actual existing `check_clock`: empty segments use
the actual old observation_ref/UTC/unchanged binding and release any unused fresh
reservation; a retained fresh ref emits an explicit segment, even for elapsed0/
unchanged UTC. Changed UTC also uses a zero segment. Nonzero elapsed is genuinely
monotonic, checked, same-lineage and remains inside the admitted Main interval.
Current-head/reservation journal retention, rollback, unknown and post-COMMIT-body
failure rules remain truthful. The policy member name is corrected to
inputs.current_state.configuration_ref. No clock/recovery evidence is manufactured.

### Re-review checks actually run

Ran project consistency, current progress-render check and whitespace check again;
all exit0, now419 tasks/122 decisions. A separate Python standard-library script
asserted the exact old/new task IDs and edges above, required new pair dependencies,
old verification preservation, acyclicity and absence of source/test/schema/build/
workflow changes. It additionally checked23 independent arithmetic/length facts:
original request/cap/clock/overflow/reference oracles plus a manually constructed
SRI1 body with ten distinct one-byte refs, ReceiptId1, CID0 and revision1. That
independent frame is78 bytes, semantic B34/I0, format-ref length37 and complete
envelope B115; key B5, CID0 and revision1 preserve their exact unsigned values at
the independently derived offsets. No SUT serializer/decoder produced these oracles.

No native/feature/foundation/schema-validation program was invoked. This is actual
document/planning review, not a compiler/schema-validator/runtime pass. Existing
consistency CI is appropriate for this documentation-only PR; latest-head CI,
GitHub permissions/rules/threads/merge and final coordinator metadata remain
publication gates. STORAGETEST018/STORAGE018, ACCESS003/current authority,
configuration/runtime/clock/entropy/bootstrap producers, actual API001/session/
HTTP contracts and downstream reviewed baselines/red/implementations/source review
remain required before the reference path can work. Every broader release gate
listed above remains; no task beyond this document/plan is marked completed.

## Final combined-main metadata review

2026-11-05, same independent reviewer. Actual combined candidate
9c01573d5c04281089383e7c32ae2eb82058a8bb, after main integration
a9f5f977b40457707395c44c3ea90433be19ace7 and the one stale-owner-label correction.
Main prerequisite is actual PR64 merge685b17f22998419f1691658a829b468eea05e770.
Examined the complete new handoff, actual state/HTML/checkpoint, navigation and018
brief against the previously approved corrected contract/plan and current main.

**Verdict: approved for integration/publication of final document-task metadata.**
This extends the previous actual document/planning approval to this exact combined
revision. It does not satisfy unseen latest-head GitHub checks, rules/discussions
or normal merge, and supplies no downstream source/schema/runtime acceptance.

Independent exact comparisons establish:

- Concrete contract5a12ebea/author82359174, STORAGE018 brief57852bbb and all three
  approved navigation/task-brief edits are byte-identical to approved84d087c.
  No new interpretation or unwritten producer has been smuggled into completion.
- Against current main, only four old task rows differ: CONTRACT014, EPIC009,
  APITEST011 and API011; the only added task rows are the two018 records. Every
  unrelated old row, including accepted ACCESS002, is exactly main's value.
  Global verification is identical; main's complete activity history remains an
  exact prefix, followed only by the two actual CONTRACT014 entries. All original
  dependency edges remain, with the reviewed mandatory018 additions unchanged.
- CONTRACT014 is done as a contract DOCUMENT only: contract/planning review passed,
  implementation not_run, actual different review identities/revisions and all
  evidence linked. Root corrected the stale pending-reviewer owner label to the
  actual selected_boost/access_implementer author/reviewer identities. Both new018
  records stay planned and require the independent baseline/red/source gates.
- Source/server/tests/schema/CI/build/specification/architecture/acceptance-map
  trees are IDENTICAL to integrated main685b17f. This includes PR64's reviewed
  evaluator/regressions/ordinary selected-Boost CI; this PR adds no executable
  change or concurrent implementation patch. Main's Git merge tree and second
  parent's tree both equal53054827e286d2c2dbfd806c4cc174479d5cad2a, consistent with
  the checkpoint's exact reviewed PR64 tree attribution.
- Handoff correctly distinguishes SRI1 semanticB34, document78, format37 and
  envelope115 and preserves the actual R1/R2/R3 history, current419/1605 DAG,
  explicit producer work and document-versus-runtime distinction. It claims no
  authored executable acceptance, schema-validator or feature result here.
  Publication/merge and latest applicable CI remain open at its authoring.
- Checkpoint preserves parked/unfinished work and earlier reviews, attributes the
  actual PR64 merge/CI to coordinator observations, and records current password
  work as a separate unmerged task with its own different review/official result.
  No old CI result is transferred to an unseen final head. This review does not
  independently approve that separate source task or claim its completion.

Actual checks: `python3 tools/check_project.py`,
`python3 tools/progress.py render --check`, `git diff --check` all exit0 on both
combineda9f and corrected9c01573. `python3 -m unittest discover -s tests/tooling -v`
ran19/19 PASS0.289s on a9f; the subsequent change affects only the task owner string,
updated timestamp, regenerated HTML and checkpoint. An additional exact script
proved that final owner-only task change, the frozen blobs/trees/rows/history and
all mandatory edges above. No native/feature/foundation suite was repeated.

Final examined metadata blobs: handoff
`d44cd850c6f08e6c31fe0178bb25f0870d5f0c11`, state
`ef62b7bd6c6d77993c2056316067ebf488912795`, HTML
`d86a812233abdad307e23c88e32c99d5fa73509b`, checkpoint
`318db40c6d37a68162211af0f2de3a41c0ea7bdc`.
All broader release/current-authority/physical-producer/HTTP/platform gates recorded
in the prior review remain. Root may now publish this focused document PR, observe
its own final applicable CI and current rules/reviews/threads, and merge normally
only when those separate gates permit it.
