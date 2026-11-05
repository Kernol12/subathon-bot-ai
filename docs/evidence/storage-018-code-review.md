# STORAGE-018 independent SQL implementation review

Reviewer: `capture_reviewer`, different from the SQL source author and additive
regression author. Review scope is the actual caller-session SQL implementation,
its selected closure/resource/error behavior, and bounded test registration. This
note does not approve unwritten fixes or infer correctness from passing tests.

## Initial candidate and findings

Reviewed root `c06e6f03fdf4c6301cd41707df1092793bc2b016`, authored source
`ce8d4d7`/import `54967ed`, SQL CPP blob
`46c0e254d140bc5176f74cede141236d6705e533`. The existing 31 SQL methods/51
scenarios passing and the old 337 checks passing did not establish source approval.
The original fixture, literal codecs, API and schema contracts remain the oracles.

The audit follows initial-capture contract sections 2, 4 and 5, consumer section 2,
and the existing receipt/contribution/application-plan contracts. Confirmed defects
in the initial source were:

1. A present unindexed request with noncanonical `0600000/1` reached candidate
   comparison and returned `source_mismatch`; its own malformed structure requires
   `invalid_stored_data` and actual whole-session cleanup first.
2. Intrinsically invalid plan valuation (resolved quote without actual event money,
   or missing-historical valuation with a dependent request) similarly reached
   comparison instead of stored-data failure.
3. Applying nonempty requirements to every imported reference rejected valid
   present-empty rule/parent bytes. Imported opaque fields retain the existing
   contracts; only actual required identities/audit fields require nonempty bytes.
4. A generic healthy two-rule contiguous collection was classified as corruption.
   The agreed bounded interpretation first validates selected count/ordinal/type
   metadata, then refuses unsupported cardinality as unindexed `source_mismatch`
   (indexed mismatch is corruption). It does not own or certify unsupported generic
   child payload collections. Supported 1/0/1 still requires complete validation.
5. Adding physical SQL row sizes imposed a hidden smaller quota, including storage
   keys/ordinals and duplicated fields rather than the complete selected semantic
   DTO. Actual optional groups also require one logical presence byte each.

No correction was authorized on these findings alone. Separate regression
authorship, actual program/configuration review and reached behavior red preceded
the corresponding bounded source corrections.

## Reviewed structural regression and observed red

Separately authored `7a169d0` plus `bf92893`, root imports `27991bd` and `9507c97`,
add four methods/eight scenarios to the original 31 methods/51 scenarios without
changing the prior assertions or fixture. Reviewed actual CPP blob
`ec0f58e11669d08885d3918e1aff426f9e836c4b` and root harness `87a6901` before
execution. All scenarios use old committed intake/plan/SRX primitives and no
index for the candidate. They cover malformed versus healthy different canonical
requests, both intrinsic valuation defects, empty rule/parent references, and
healthy two-child versus corrupt ordinal shape. Semantic failures check whole
cleanup inside the catch and an independent connection's actual `BEGIN`; healthy
refusals check the caller's marker remains active before host rollback.

The CMake registration and compiled-function guard were 35, enabled and still
`acceptance;draft-red`; ordinary CI did not invoke the new SQL bodies. These
program/configuration gates were explicitly approved before root's targeted run.

Independently read root's raw `/tmp/storage018-structure-red.txt`, SHA-256
`a4944334a59e8acb40ccf00b186de833969135367ec58c1bb47e7b5400abef97`.
At clean `87a6901` with held CPP `46c0e254...`, root observed direct exit 8:
six reached wrong-behavior failures, two already-correct controls, eight
init/cleanup passes, zero skipped/setup/stub failures. CTest's repeated failure
transcript is not a second execution. This is a valid new behavior regression red,
separate from the preserved historical FIRST-red baseline.

Structural correction `8f5a7d7`, imported root `98c252e`, actual CPP
`8702843f4a52b8ef6c662cf78c53a117408ef1e1`, was subsequently inspected. It adds
required-reference roles, canonical decoding before comparison, actual
header-receipt valuation checks and bounded child metadata classification. The
author's passing 35-method/59-scenario run is recorded by the author; final source
approval remains pending the resource and scalar preownership issues below.

## Independent semantic resource derivation and observed red

Consumer section 2 counts declared field/copy bytes, enum/bool 1, 64-bit scalar 8,
one optional presence byte plus its contents, and exact canonical component bytes.
Independent fixture-A arithmetic from public core/server DTO declarations is:

| Declared value | B |
| --- | ---: |
| ReceiptIdentity | 25 |
| ReceiptFacts: event type 30 + schema 4 + six absent optional tags | 40 |
| ReceiptArrival: UTC 8 + Q/C/X | 11 |
| PlanEvent: V, absent parent, V, receipt, identity/facts/arrival, present CID0 | 96 |
| ContributionState including complete conditions | 12 |
| PlanValuation with absent quote | 2 |
| One RuleSelection | 33 |
| One RequestedEffect | 70 |
| CapturedPlanRevision: revision 8 + absent parent + capture/rules + absent override | 223 |
| SqlitePlanRevisionSnapshot: plan_ref P once + revision | 224 |

The other selected fixture-A DTOs are at most 186 bytes, including the retained
role-envelope payloads. The old sum of header/receipt/support/revision/rule/request
physical rows was 279; it cannot replace the declared snapshot projection. Budget
240 fits the complete selected DTOs; 222 is below even the mandatory revision 223.
No implementation output was used to derive these expected values.

Reviewed separate author candidate `aea8e7c`, CPP
`6d832b975a4c248acd21b0520f040feec54c6510`, and actual root harness `807e1d3`
before execution. It adds one two-row indexed healthy-A test (36 methods/61
scenarios total), preserves all 35 prior methods, and checks exact find plus
`already_present`/normal commit at 240, versus known `result_limit`/actual whole
cleanup at 222. Configuration only adds its enabled draft-red registration and
changes the compiled-function guard to 36; no automatic SQL feature-body lane.

Independently read root's raw `/tmp/storage018-budget-red.txt`, SHA-256
`3d114490a862719f56610f6af740a8ff5354bb6b7d0afadc938e5791b067f7b2`.
Root's actual clean `807e1d3`/held CPP `46c0e254...` run at 20:43:58Z exited 8:
the healthy 240 row reached a storage refusal, the 222 overage control passed,
plus two init/cleanup passes, zero skipped/setup/stub failures (98 ms). This is
valid behavior red for the hidden quota. Only afterward was field-by-field typed
resource correction authorized. The new formula is not yet approved here.

## Remaining scalar preownership finding

At actual structural candidate `98c252e`, `children()` selects raw
`min(ordinal)`, `max(ordinal)` and, for requests,
`max(requires_historical_valuation)` alongside a malformed-type count. A corrupt
TEXT/BLOB may therefore be copied into Qt before the type count is examined.
The metadata path must return only bounded fixed scalars even for malformed
storage; a writer result-byte refusal cannot substitute for selected known-shape
corruption. The raw ordinal aggregate existed initially; the dependency aggregate
is new in the structural correction. Root and source author were notified.

A separate focused test/review/reached-red gate is required before correcting
these aggregate expressions. No final source/configuration approval is given by
this interim note. The historical reviewed baselines, valid review records and
FIRST-red evidence remain valid and must not be replayed or rewritten.
