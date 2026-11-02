# Internal application transaction bundle — revision 1

Task: STORAGECONTRACT-001. Authority: [specification v0.45](../specification.md)
§§3.3–3.5,4,8,16; DB-01, NUM-01–04; [event/state boundaries](event-state-boundaries-v1.md)
CONTRACT-002, especially B03–B10. This is a documentation-only logical contract
candidate. It supplies row/port obligations and literal examples for the later
STORAGETEST-002 baseline, not C++ interfaces, DDL, a migration, executable tests or
implemented application behavior. Independent actual review remains required.

Prerequisites are CONTRACT-002, [captured-plan persistence](sqlite-application-plans-v1.md)
STORAGE-015, [pure plans](application-plans-v1.md) ACCT-003 and
[separate time debt](time-debt-v1.md) CORE-005. ACCT-004 and STORAGE-002 consume this
contract; this contract never depends on their future implementation. Existing
receipt/contribution/plan/numeric/writer contracts remain unchanged.

## One durable application, after durable intake

An application starts with already-DURABLE intake: an actual immutable ReceiptId,
its retained configuration/context, and any contribution/plan records required by
that input already exist from earlier acknowledged commits. This is an honest
CALLER precondition established by its earlier intake/capture protocol, not a fact
inferred from SELECT, row presence or a copied session ticket. Staging intake or
a new capture in this unit cannot satisfy that earlier-commit precondition.
Recording intake is not applying it. A pending receipt here means its application
is outstanding; no new pending flag is invented on the existing immutable receipt
row. The existing
contribution dimension is `ContributionApplication::not_applied` where applicable.

The authoritative Qt queue advances/revalidates current state, permission,
eligibility, operation identity and selected plan revision, resolves current
capacity/debt/lifecycle/milestones, and prepares ONE complete application bundle.
One real SQL transaction commits its effects, resulting authoritative state,
application claim/status, required job intents/groups, retrievable operation result
and audit together. Internal prepared result records and owned read/duplicate
values may exist provisionally INSIDE that transaction. These values do not by
themselves certify durability or publishable normal success. No state/result
publication or external send precedes
acknowledged COMMIT and normal committed work completion.

B08 is an exact unit boundary: a KNOWN uncommitted failure after preparing any
subset removes ALL this application's prepared records/updates, including success
result/audit. Already-durable intake remains unchanged and pending/not-applied.
It does not lose its original receipt, attempts, evidence, context or plan. A later
permitted retry rechecks identity, current state and conditions and applies at most
once. A failed application cannot leave a durable applied status/effect/job or
publish success, or keep only the successful actions from a mixed input. Same-session
observation of staged rows before rollback is not external/durable visibility. No
failure audit is smuggled into the failed unit as if it committed; operational
diagnostics are distinct.

An actual unknown COMMIT/rollback outcome is NOT B08 known rollback. It establishes
neither no effects nor a pending receipt. Preserve the existing `outcome_unknown` /
`uncertain_outcome`, absent normal payload and fail-closed writer; do not replay.
A physical COMMIT with lost acknowledgement may contain the WHOLE application.
Outcome resolution/recovery is a separate gate, not an automatic retry recipe.

## Exact owner and proposed internal port shape

Use the existing [writer work boundary](writer-transactions-v1.md) and
[actual tickets](../../server/include/subathon/server/sqlite_writer.hpp): one
`SqliteWorkContext::begin()` performs real BEGIN IMMEDIATE on the controlled
writer. The enclosing application unit exclusively owns actual COMMIT/rollback.
No nested event-loop wait, separate writer connection, savepoint-as-independent-
commit, deferred commit vote or external callback may split the unit.

Propose a NEW inert session-borrowing application-bundle port. Names below are
logical operations, not supplied code/signatures:

| Primitive | Required meaning |
| --- | --- |
| Look up operation | Read the complete transaction-visible intent/result/links for an operation key, strictly validating the selected record closure; presence does not prove prior COMMIT and absence is not authority to apply |
| Read application inputs | Resolve actual receipt/contribution/selected plan and the relevant current snapshot under the same active unit; return owned transaction-visible values and checked revision information, with earlier intake/capture durability a caller precondition |
| Stage bundle | Atomically check identity/claims/revisions/references and stage the whole bounded supplied bundle or a no-own-write domain refusal; never evaluate rules, grant permission or commit internally |
| Read recorded application | Return owned transaction-visible immutable effect/result/audit/job-intent provenance, which may be own staged data; no durability/publication certification; historical after-state need not equal current live state |

ALL normal returns from this borrowed-session port are transaction-visible,
provisional internal values: input reads, operation lookups, recorded-application
reads, exact duplicates, subject-claim refusals and new staging alike. SQL under
an active session sees its own writes. Validating data/closure/identity does not
distinguish prior committed rows from rows staged earlier in this unit and must
not label either lookup as certified durable. No lookup result, duplicate result
or healthy refusal is a committed/published normal work result by itself.

After a complete stage, lookup/read of the same operation may return its own
provisional result/links. An exact same-intent duplicate must return that same
transaction-visible data with NO additional effect/status/job/result/audit rows.
A second operation key for the same original subject must observe the staged
claim and refuse a second award without certifying that the claim is durable.
Healthy refusals leave all earlier staged caller work active and unchanged; they
neither commit nor clean it. Abandonment/known rollback removes the staged original
and its duplicate/claim evidence together. Actual COMMIT plus normal completion
instead exposes exactly one application. AT21 makes both branches explicit.

Even when the caller knows an existing application was committed earlier, lookup
returns its original data without revaluation under today's state. That prior fact
cannot convert an unknown/error outcome of THIS work into normal success. The
current writer's real completion/outcome controls its normal payload/publication;
unknown remains unknown and grants no replay. Publication/response failure AFTER
normal committed completion cannot erase the durable operation result or
retroactively reclassify the already-produced work payload. In contrast, a body
exception AFTER physical COMMIT but BEFORE normal completion leaves accounting
committed with no normal work payload under the existing writer contract.

The future facade owns no destructor cleanup or transaction transfer. The caller
must perform actual COMMIT and await normal committed completion before publishing
normal success from any path. Known local selected storage failures require actual
rollback of the entire caller unit before a known port exception. Writer/retention errors already establishing rollback must not be
"proved" by another inactive cleanup. Capability/thread/phase/protocol/invalid-
statement and actual unknown failures retain their writer truth. Allocation after
own writes must roll back the whole unit or propagate actual unknown; preparing
all allocating buffers before writes does not excuse post-write failure handling.
Pure input rejection and healthy domain refusals make no own writes/cleanup.

Existing ports are NOT all composable unchanged:

- `SqliteRetainedContextStore` and `SqliteApplicationPlanStore` borrow the session;
  their successful results are staged and their destruction is inert.
- Existing `ReceiptTransaction` and `ContributionTransaction` wrappers OWN real
  commit/cleanup after successful begin/adoption. `ReceiptIntake::ingest` and
  `ContributionEvidenceService::observe` commit their intake/linkage units. Do not
  call these owning services/wrappers inside this application unit, double-adopt a
  copied ticket, or commit contribution status before effects/jobs/results exist.
- A future reviewed bundle adapter needs bounded non-owning primitive resolution
  and status/state writes on that same session. The present owning contribution
  facade is not a shortcut to such a capability. No private writer/raw SQL handle
  or fake ownership tag is supplied by this document.

The caller owns current authorization/eligibility and explicit observation ordering.
A preview token only checks freshness of supplied plan/state tokens. A current
preview, `OverrideAuthority::authorized`, nonempty actor ref, resolved quote or
absence of one blocker is not permission or proof all other blockers disappeared.
Recheck against current state inside the serialized unit. A stale preview produces
an explicitly revalidated current proposal/result or a conflict under the eventual
application API; it never silently commits old capacity. Exact external conflict
payloads and revision granularity remain their owning API/accounting gates.

## Proposed logical records and identities

These are INTERNAL logical records/relations for review. They are not the finalized
conceptual record table from CONTRACT-002, physical SQL names, new global numeric
surrogates or wire DTOs. Preserve byte/value identity; never infer equality from
names/amounts/times, a transport connection or a nonempty opaque ref.

| Logical record | Required retained content / relation |
| --- | --- |
| Operation intent | Stable scoped operation key; complete owned input intent and explicit choices; source receipt, optional contribution, selected plan+revision, supplied freshness constraints, reversal identity/basis when applicable |
| Application claim | The original application's typed subject and operation key; a uniqueness guard observed while staged and made durable on commit against another original award to that subject; original claim remains after a refund |
| Applied effect occurrence | Selected captured request occurrence, requested Exact and signed64 units, actual signed64 components by target/unit, actual-cap/debt/rounding explanation, immutable source/calculation/reversal provenance |
| State transition | Relevant before/after authoritative values and checked revision relation; preserve actual timer transition trace and independent debt/pause/lifecycle/score/count fields that this operation touches |
| Application status change | Actual selected subject's not-applied→applied change on successful original application; preserve unrelated current evidence/valuation/review/reversal dimensions, never replace them with an old capture |
| Required job intent / group relation | Stable scoped identity and owned captured versioned action meaning, source effect/event and required ordering/holds; no remote execution claim; exact envelope fields remain a gate below |
| Recorded operation result | Original operation key, application/effect/job references intended to commit in the SAME unit and exact requested/actual outcome plus checked/resulting state relation; retrievable without applying again, provisionally while the unit is active |
| Application audit | The operation/source/plan/revision, actual actor/decision/reason/options where known, current checks/choices, resulting records and provenance; unknown fields remain absent, not invented |
| Reversal progress / occurrence | Original immutable effect locator, this reversal identity/source receipt, cumulative entitlement, previous cumulative reversed amounts, this incremental request, actual timer/score/count outcome, policy/hold and rounding residuals |

Use proposed owned opaque `(operation_scope_ref, operation_ref)` as the internal
operation key. Scope must be retained and stable across reconnect/retry; its mapping
to actual installation/principal/command namespaces is an explicit future API gate,
not a hardcoded client/session identity or a permission claim. The operation kind
and payload belong to the intent, so changing command kind cannot evade an existing
key's consistency check. Compare complete structured intent by byte/value; a hash
alone or a lossy serialization is not payload consistency proof.

Intent includes selected source/plan, supplied preview/expected constraints and
explicit choices. It is distinct from freshly computed actual capacity, checked
current state, generated job/result and audit outcomes. Comparing duplicate intent
must not require recalculating a previous result under today's state. A key reused
for a different source, plan revision, amount or explicit choice is a conflict.
A legitimately changed intent requires a new operation key and fresh authority;
a new key does NOT bypass an already-applied subject claim.

For an original support application, the subject is tagged `ContributionId` when
present (including0). This prevents original support and linked confirmation or
another plan/operation key from awarding it twice. With no contribution, propose a
tagged actual `ReceiptId` subject for that non-support input's original application.
All actions in one selected plan application share this claim; the claim excludes
plan revision so an override cannot award an already-applied contribution anew.
A receipt subject and numerically equal contribution subject are different domains.
Explicit later corrections/refunds have their own role/identity, not a reset of
this original claim or mutation/deletion of old effects.

An effect locator may reuse `(original operation key, selected request occurrence)`
where occurrence is the existing ordered plan-request position, not a new EffectId
allocator. Ordered duplicate action refs remain distinct occurrences. If later
application modeling permits multiple ledger records for one occurrence, its
component identity must be refined BEFORE baseline/API approval; do not collapse
Main/debt or invent an extra numeric surrogate here. Job/group/audit identifiers
remain supplied opaque owned refs with declared scope/retained meaning, not names
mistaken for foreign keys or a new job allocator policy.

## Idempotency and reversal uniqueness

After actual selected-record integrity checks, the following logical outcomes
remain PROVISIONAL/transaction-visible under the port protocol above:

| Observation | Required behavior |
| --- | --- |
| Same operation key, same complete intent, complete transaction-visible result exists (prior committed OR own staged) | Return the same provisional recorded data/references, no new effects/status/jobs/result/audit; do not recompute under current caps or claim prior durability from SELECT |
| Same key, different intent | Operation conflict, no own writes or retarget/upsert |
| New key, original subject has an existing transaction-visible application claim/status (prior committed OR own staged) | Provisional existing-application/conflict data referencing that claim, no second award/jobs; leave earlier staged work unchanged and do not certify claim durability or bind the new key as a new success |
| Missing/dangling/corrupt selected closure | Explicit stored-data/reference failure with known rollback only when established; never replay to "repair" a missing result or incomplete bundle |
| State/plan relation changed since preparation | No-own-write stale/conflict or explicit whole reproposal before staging; never a partial application |

Uniqueness must ultimately be enforced by transaction/database constraints on the
SAME logical identities as service checks. Queue serialization alone is insufficient.
How durable rejection-only results/audits or intent reservations are recorded is a
separate proposed API decision: a no-own-write refusal here does not invent a
committed rejection row or success payload. Any explicit review/decision command
that persists its own outcome still needs its own honest atomic unit/identity.

A reversal is not a second original award. Propose one stable reversal identity
bound to its actual distinct refund/correction receipt, and retain the existing
`TimeReversalRequest::reversal_ref`/`original_effect_ref` meaning. Per-original-effect
reversal occurrences are unique by `(scoped reversal identity, original effect
locator)`. One original effect may have MANY distinct partial-reversal occurrences;
one refund can address several original effects. Do NOT make original_effect alone
unique, make ContributionId unique across every reversal, or let another operation
key repeat the same refund. Bind duplicate reversal identities/source receipts to
the same intent; conflicting reuse refuses. The exact opaque reversal namespace
and verified financial correlation are upstream gates, not inferred from raw
receipt payment links or equal strings.

Update cumulative reversal progress and stage the new occurrence in the SAME unit
as actual effects/state/result/audit/jobs. Competing partial refunds cannot each
consume the same prior remaining entitlement. Prior cumulative values are compared
under the writer; when stale, repropose/conflict without writing. Never delete or
rewrite the original credited effect, rate, selections or prior reversal history.
An original fully reversed application remains originally applied; it is not a new
pending award. Performed actions/completed milestones remain recorded.

## Numeric values, current state and retained provenance

Use signed64 milliseconds, micro-points (millionths of a point) and whole counts
for resolved requested/actual units; check every final conversion AND resulting
aggregate. Negative corrections/refunds and zero are valid data in their appropriate
operation roles. An overflow produces no partial application/job; preserve input
for review. Never wrap, saturate overflow, route through REAL/double or negate
`INT64_MIN` in signed64. Current caps deliberately limiting actual Main credit are
different from arithmetic overflow. Display precision cannot change accounting.

Preserve existing `Exact` rational VALUE as canonical signed numerator/positive
reduced denominator, zero0/1, using lossless bounded components; retain original
money/rate/factor decimal spellings separately. Keep exact requested units, final
signed64 request and any rounding residual independently. For rounding, the logical
residual is `exact units - rounded units`, in exact arithmetic, possibly negative;
retain the applied half-even mode and calculation explanation. Storage preserves
validated supplied derivations; it does not run matching, a numeric evaluator or
re-round to certify them. Existing parser512/256/finalizer4096-bit limits are NOT
new storage bounds; approved plan storage can retain fitting >4096-bit components.

Original/effective/fetched rates, currency, snapshot, known source/timestamps/manual
provenance remain exactly the actual `HistoricalValuation` fields, directly owned
or resolvably linked to the immutable selected captured revision. Absent metadata
stays absent. Preserve every rule/version/exclusion, modifier definition/instance/
operator/factor and requested action/version/target/reversal ref/position needed
for the effect's meaning. Current edits cannot revalue an original or replace the
captured plan. Source original receipt/ancestry/configuration/context are retained;
current contribution state is a separate selected value, not its historical capture.

Debt/Main composition follows [CORE-005](time-debt-v1.md) exactly:

- Eligible nonnegative additions partition `requested_ms = debt_repaid_ms +
  main_added_ms + cap_limited_ms` with exact checked arithmetic. Preserve all three
  components and their original source. Main and debt remain separate nonnegative
  signed64 balances; both may already be positive. Above-cap Main is retained with
  zero headroom by `plan_time_addition`; the older narrow `calculate_time_credit`
  still rejects above-cap input. This contract does not conflate those scopes.
- Only actual `main_added_ms` enters the eligible Active timer credit helper.
  Debt-only repayment or actual0 cannot rescue Grace/confirmation wait. Advance to
  the command observation first; reached hard end/Finished cannot be reopened by
  ordinary credit. Grace is a separate countdown, never negative time/debt.
- Reversal proposals retain full signed request, original effect, selected explicit
  StopAtZero/CarryTimeDebt/RequireReview policy, actual nonpositive Main adjustment,
  nonnegative debt addition and nonpositive unapplied/held remainder. In exact
  arithmetic `request = main_adjustment - debt_added + unapplied`. RequireReview
  holds the ENTIRE deficit timer adjustment with balances unchanged; it is not an
  automatic global contribution/review/delivery status. After Main exhaustion,
  compose the existing zero-elapsed timer reducer at the same observation and
  retain its ordered transitions; do not invent timestamps or erase pause reasons.

For partial refunds, ACCT-006 owns proportional entitlement from ORIGINAL ACTUAL
credit, original valuation/rules and previous reversals. Retain exact cumulative
fraction/basis, cumulative rounded entitlement, prior credited/reversed totals,
increment and residual; use cumulative entitlement minus already reversed amount,
not independently rounded per-refund awards. Full refund clears EXACT remaining
credit; whole-count credit reverses on full refund. Never reverse more than actual
original credit or count cap-limited-away requests as credit. Timer application
may be limited/held while the full reversal record survives in an explicitly
committed decision outcome; do not lose entitlement/history or falsely mark a held
timer change as performed. The later application/reversal policy must state which
other components/jobs are permitted in that held outcome; none is guessed here.

CORE-005 deliberately does NOT define a single "applied time" amount spanning
Main added and debt repaid, or how a later refund allocates between those earlier
components. This contract retains BOTH actual components and their links. ACCT-006
must settle/test that allocation before enabling that combined application/refund
path. The simple no-debt refund examples below do not approve a missing allocation.

## Actual references, closure and representation limits

| Reference / value | Existing physical meaning and required distinction |
| --- | --- |
| ReceiptId | Actual immutable original, domain1..UINT64_MAX; full uint64 BLOB8;0 invalid independently of pure codec0 |
| Optional ContributionId | Actual contribution identity/reference, domain0..UINT64_MAX; SQLNULL/absence differs from present BLOB0; support receipt need not equal linked confirmation receipt |
| Plan reference/revision | Actual immutable `(plan_ref, revision)`; persisted revision1..UINT64_MAX, complete initial/contiguous authorized history; pure codec includes0, expected token0 does not become a stored revision |
| Configuration/context | Actual typed retained configuration or arrival-context namespace, including actual optional known configuration; same bytes in wrong namespace are not resolution; known cfg may differ from arrival cfg |
| State freshness token | Existing core preview token is opaque string; no numeric StateRevision counter/storage currently follows from it; concrete relevant-version coverage is a future gate |
| Exact / original strings | Canonical rational VALUE and exact byte spellings/presence; signed timestamps remain signed64, source schema remains full uint32; no numeric ID/amount through REAL/TEXT coercion |
| Rule/action/modifier/quote/actor/event/job refs | Existing semantic meaning may be supplied/retained upstream, but no finalized physical table/namespace follows from nonempty strings; no fake configuration FK or permission proof |

The later adapter must resolve selected physical sources and actual typed targets
inside the unit and compare relevant immutable projections, not just accept a
serialized ref. A selected revision DTO/local parent proof from STORAGE-015 does
NOT certify a complete sparse MAX history or authorization to apply it. Declared
history/head validation must reject malformed/gapped histories without making a
full-find read cap a product append/application quota. Define a bounded metadata
resolution path in the later reviewed port; do not promise the current targeted
lookup already supplies it. Current state may advance after an old recorded
application; retained historical outcome is not corrupt merely because it differs.

Propose byte-policy continuity with existing storage: reference4096, owned value
262144, canonical component16384 as configurable positive engineering defaults.
These are proposed INPUT/materialization bounds, not new product history/job quotas.
All ordered effects/jobs/audit/transitions and their totals need explicit bounded,
overflow-safe counts/lookup scopes before C++/physical baseline approval. A full
read refuses over-limit rather than truncating; targeted writes cannot acquire a
quota from an unrelated full-history read. Validate actual SQL storage class,
NULL/group/domain/range and bound before conversion. No DDL, numeric allocator,
exact new vector maxima/query count or unknown job payload layout is frozen here.

## Independent literal examples for downstream tests

These are manually derived contract oracles, NOT test code, runtime observations
or producer-generated expected results. `op-A`, `state-a`, job and semantic names
below are synthetic opaque references with stated test-only retained meanings.
For each example arrange real earlier intake COMMIT before the application unit.
Unless stated otherwise all current permission/eligibility/deadline prerequisites
are independently established; strings/fixtures alone do not prove those facts.

| ID / input and current records | Required observable result |
| --- | --- |
| AT01 — current cap/B04: receipt7,CID0,plan-P/rev1 captured request+600000ms at55min; current state-a is Active/Main3480000ms,debt0,cap3600000 | Revalidate current58min. Actual Main+120000, debt repaid0, cap-limited480000; state-b Main3600000. One original claim/CID0, immutable effect occurrence, status applied, required captured job-J queued, operation result/audit commit together; job-J is an explicitly supplied required intent, not inferred from the number |
| AT02 — AT01 staged effect/state/status/job/result/audit, injected known failure before COMMIT/B08 | Earlier receipt7/CID0/not-applied/capture remain exactly as before. No prepared effect, state-b, claim, job-J, success result/audit is durable or published; retry must recheck and can apply once |
| AT03 — same key+same intent after successful AT01, now Main lower and rules edited | Return original recorded+120000/state-a→state-b result; no new award/jobs/audit and no revaluation. Old historical state-b need not equal current live state |
| AT04 — same key reused for different receipt or plan revision; OR new key with already-applied CID0 | First is operation-intent conflict; second existing-application/conflict. No own writes, retarget or second award; a confirmation/source receipt with the same CID is not another contribution |
| AT05 — before stage, checked state-a or selected plan head no longer matches | No partial rows/status/jobs. Explicit conflict or whole revalidation/proposal against current snapshot, never old capacity. A current preview still supplies no permission |
| AT06 — Main55,debt2,eligible request10ms,cap60 | Debt repaid2, Main added5, cap-limited3, resulting Main60/debt0. Preserve request10 and distinct signed state deltas+5/-2; never invent a single refund base7 or apply+10 to Main |
| AT07 — Active Grace90000 or confirmation-wait Main0; debt5,eligible addition5,cap0 | Debt repaid5/Main added0/capped0. Main0/debt0; existing Grace/wait is not rescued; no pause reason or hard end changes. Addition7 without cap instead has Main+2 and can rescue an independently eligible Active snapshot |
| AT08 — Main100,debt5,addition8,cap60 | Existing Main100 retained, debt repaid5/Main added0/capped3. Do not pass this above-cap input into the older narrow credit helper or shrink Main as a side effect |
| AT09 — Main100/debt7,reversal request-150, explicit policy | StopAtZero: Main0/debt7,Main-100/debt+0/unapplied-50. CarryTimeDebt: Main0/debt57,Main-100/debt+50/unapplied0. RequireReview: balances100/7 unchanged,actual0/full-150 held. Retain entire original link/request/policy; do not infer a global application/review result |
| AT10 — Main1/debt0,reversal INT64_MIN,CarryTimeDebt; contrast Main0/debt0 | First: Main0/debtINT64_MAX,Main-1/debt+INT64_MAX without negating signed minimum. Second overflows; no partial application/effects/jobs; preserve input for review |
| AT11 — original €20 actually credited+300000ms/+40000000 micro-points, no debt; first refund€5 then remaining€15/B05 | Cumulative first1/4 entitlement75000/10000000; next full entitlement300000/40000000 minus prior yields225000/30000000. Two DISTINCT reversal identities/receipts may reference same originals; duplicate of either produces no second increment. Current rules/rates do not recalculate originals |
| AT12 — original actual1 micro-point; cumulative refunded1/3,2/3,full/B06 | Exact cumulative entitlements1/3,2/3,1; half-even rounded0,1,1; residuals1/3,-1/3,0; incremental reversal0,1,0. Keep zero occurrences/provenance. A whole-count original1 stays until full, then reverses1 |
| AT13 — original request10min cap-limited to actual5min, full refund | Basis is actual300000ms, not requested600000. Full reversal clears exact remaining actual minus prior reversals; no fresh current-rule computation. A proposed cumulative amount above original actual cannot be committed as a valid refund |
| AT14 — exact finalizer value3/2 micro-points then5/2 | Half-even resolved requests2 and2; exact residuals-1/2 and+1/2 respectively, original decimals/quote spelling unchanged. Aggregate overflow (e.g currentINT64_MAX plus actual1) holds input with no partial effect/job; cap-limiting to actual0 is a distinct valid numeric outcome |
| AT15 — otherwise eligible original application resolves actual0 because cap is full | Preserve the requested amount and actual0/cap provenance with the single original application claim/status/result/audit. It is not pending credit waiting to grow when capacity returns. Required job set is whatever the independently validated action policy supplied, possibly empty |
| AT16 — selected ReceiptIdMAX/plan revisionMAX or optional CID0; wrong SQL class, ReceiptId0, absent typed cfg, dangling plan/reversal original | Full existing numeric domains/NULL distinctions remain; strict selected type/closure validation required. A sparse1,MAX-1,MAX plan is NOT a valid complete application history or valid exhaustion proof. Refuse bad/dangling sources without effect/status/job/result publication |
| AT17 — physical whole-bundle COMMIT succeeds but acknowledgement is lost | Actual whole bundle may be durable; outcome_unknown/uncertain_outcome, no normal success payload, closed writer/no blind retry. Do not report known rollback or receipt pending; later outcome resolution owns the next decision |
| AT18 — local selected failure or abandonment; actual rollback acknowledgement lost | No fabricated known-uncommitted port error/pending claim from inactive cleanup. Unknown outranks local cause, writer closes, no replay. Separately, confirmed COMMIT then a BODY exception before normal completion leaves accounting committed/no normal work payload. A LATER publication/response failure after normal completion leaves accounting and durable operation result committed; it cannot erase the already-produced payload, and retrieving that result is not reapplication |
| AT19 — timer hard end reached while paused/Grace/wait before ordinary credit; OR deficit exhausts Main | Advance first: reached deadline finishes, ordinary credit cannot reopen. For valid reversal Main exhaustion, preserve zero-elapsed reducer transitions and Grace/pause/debt distinctions in one unit; no invented transition UTC or negative Grace |
| AT20 — a committed queued job fails remotely or send outcome is uncertain/B09 | Internal effects/status/result remain committed. Later delivery transitions return through authoritative writer; no rollback of accounting, no known-undelivered claim or remote exactly-once/replay promise |
| AT21 — two fresh branches start from AT01's same earlier-COMMITTED intake/state-a. In one active unit stage op-A, then lookup/read it, repeat same op-A+intent, then try op-B for the same CID0 with current transaction-visible state-b freshness | Own-session reads/duplicate return the SAME provisional+120000/state-a→state-b data/links; no extra effect/job/result/audit. The second-key subject claim refuses without own writes/cleanup; all earlier staging stays active. Outside the unit, the prior application status remains not-applied and no new rows/state are visible yet. Branch1: abandon or confirm rollback—NONE of op-A's prepared records survive, prior intake stays pending. Branch2: actual COMMIT+normal completion—exactly ONE application/effect/job-J/result/audit becomes durable/publishable. No path infers durability from staged row presence or publishes before that completion |

STORAGETEST-002 must turn bounded examples into independently reviewed real-SQL
oracles: actual prior intake commit, observed intermediate staged rows, separate
reader invisibility/inside-catch real rollback, complete database snapshots,
same-unit reads/duplicates/claim refusals and their commit-versus-abandon branches,
uniqueness/duplicate payload conflicts, actual commit/cleanup uncertainty, reopen
and full-range literal data. An independently supplied expected fixture is required;
never derive expected actual effects or manifest data by calling the producer.
Passing existing suites or a failed compile/setup does not establish this baseline.

## Remaining decisions and owning gates

No requirement conflict currently blocks this logical proposal. The following are
explicit unfinished contracts/validation, not invented defaults or release cuts:

| Gate | Required next concrete definition |
| --- | --- |
| STORAGETEST-002 API/physical review | Exact owned bounded bundle/input/result types, input/error priority, read scopes/vector maxima, non-owning primitive composition, physical typed keys/uniqueness/reference closure/additive migration; preserve earlier schemas/ports |
| ACCTTEST-004/ACCT-004 and API-002 | Operation scope/intent equality binding to actual caller/install/source, current permission/eligibility, state-version coverage across every relevant writer, conflict/revalidation and published projection; no absent service approval |
| ACCT-006/refund baseline | Verified original/refund correlation, exact cumulative entitlement, concurrency/over-refund handling, Main-versus-prior-debt-repayment reversal allocation and held multi-effect outcomes |
| MILESTONE/job contracts | Complete versioned owned job/action-group envelope, captured definitions/recipient/sending identity, required ordering/review/Run-Defer-Skip choices, persistence transition legality/retry eligibility; no invented provider behavior |
| Timer/pause/schedule/recovery integration | Relevant advanced snapshot and independent reasons, clock/config references, generated-event ancestry and observed transitions; portable checkpoints/downtime/restore/outcome resolution remain STORAGE-003/operations gates |
| CONTRACT-003 and provider/platform gates | Public wire/version/representation/capability/authentication contracts, postcommit delivery and verified duplicate-safety, supported runtime/platform/live evidence |

A future opaque job payload/reference must have actual retained versioned meaning
before enabling its consumer. Synthetic job intents can establish bundle storage
atomicity only, not a complete job contract or permission to deliver. Likewise this
logical state relation does not finalize a complete timer checkpoint/score/milestone
schema. Unknown event/actor/rule/action/job namespaces cannot acquire fake FKs just
because configuration retention already exists. Refine/review each actual artifact
before its test baseline; no future implementation or test/red approval is given here.

This documentation changes no executable suite/module/dependency/workflow. Run
project/link/progress/tooling checks; later stable executable slices must extend
and observe automatic CI under [the workflow](../AGENT_WORKFLOW.md). All seven
integrations, all three rate providers, both renderers, UI/locales/themes,
Linux-server/Windows+Linux-client x86-64, packaging/security/backup/recovery and
live/platform release gates remain required. No live operation is authorized.
