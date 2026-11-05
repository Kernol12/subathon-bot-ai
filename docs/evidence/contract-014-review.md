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
