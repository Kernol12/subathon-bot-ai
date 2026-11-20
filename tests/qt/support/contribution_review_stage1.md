# ACCT-005 Stage 1 author packet

Tier A. Revision 2 with Correction 1 (80A) is frozen at
`65a1c80494aed498df141c9844e9a95416ff5f9d`, contract blob `1d001d9b`.
The examples body SHA-256 is
`698d045992f54cd8dc24f0cb19e8c60ff179a2a39c4cc37f56fca3e4d92afc9a`.
The protected prefix ends at `84dd71180cd65b26d31f4cbc493b851d356fec73`.
[Original freeze](https://github.com/Kernol12/subathon-bot-ai/pull/742#issuecomment-7023283524) ·
[Owner 80A and v19 assignment](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7039054377) ·
[80A closing](https://github.com/Kernol12/subathon-bot-ai/pull/742#issuecomment-7039183961)

## Expected first stops after admission

| Cases in the Qt source | First intended stop | Independent observation beyond the stop |
| --- | --- | --- |
| `catalogProducerAdvancesExactlyOneStep` | Explicit producer NI | Protected v18 with a previously committed bundle and milestone, exact v19 delta, full schema and framed history, empty new relations |
| `literalCatalogBudgetAtAndOneBelow` | Passing control | One byte below cumulative UTF-8 fails admission, exact charge advances |
| `exactV19FinancialMarker`, `exactV19MilestoneMarker`, `exactV19ModifierMarker`, `exactV19RateMarker` | The respective shipped selector rejects the new exact marker | Each first reads through its normal entry point on v18 and applies literal v19 |
| `lookalikeV19MarkersStayClosed` | Passing control | Exact v19 setup, then a case-changed latest marker; typed refusal for all four selectors |
| `executeIngressRefusesSeventeenWithoutPreview` | Explicit execute NI | Count precedence at ingress, before opaque basis parsing or owning writer work |
| `lookupWithCurrentScopeDoesNotResumeOrCreate` | Explicit lookup NI | Current key denial and zero row work |
| `rollbackPrefixAndExactRetry`, `firstRowRollbackLeavesNoBinding`, `liveCapAndUnrelatedScoreDriftStaleSuffix` | Explicit preview NI after real retained setup | A/B. Main starts at 95, then +4/+1/+0 at cap 100, so 0/3/4 ms is cap-limited. Score is 90/97/104/111. Only 111 crosses 110. A separate +6 correction leaves 103 and invalidates the two remaining projected bases. |
| `uncertainAndAbnormalCommitStop` | Preview NI | C. Existing writer acknowledgment-loss seams and an after-COMMIT exception distinguish actual media and writer outcome. A recovered exact retry resumes at the verified cursor. |
| `changedIntentOrBrokenCursorNeverStagesSuffix`, `singleActiveAttemptAndSessionLossStopForwarding` | Preview NI | D. A competing command cannot stage in the active attempt. Canonical identity, current scope, corrupt cursor and FK-valid wrong receipt link independently block forwarding. |
| `eligibilityAndDisclosureAreSameRow`, `sameTokenEvidenceChangeAndRejectionKeepBlockers`, `acceptedPendingNeedsCurrentQuoteAndNewApplyKey` | Preview NI | E and preserved controls. Pending/rejected/stale/denied outcomes leave independent dimensions intact. The existing ordinary acceptance survives confirmation and rate arrival, then a new authorized apply appends exactly revision 2. |
| `sixteenRowsAreDistinctAndFifteenRowPrefixReconciles`, `mixedSixteenWithRealApplyAndUnavailableSelectors`, `preflightRefusesSeventeenAndUnqualifiedWriter`, `preflightFanoutAndLiveRowBudgetStopAreDistinct` | Preview NI | F and stop controls. One row per real transaction, one final read where eligible, refusal before staging, and completion of a 15-row immutable prefix. Eight +4/+7 applies leave Main 100, score 146 and 27 capped ms. |
| `budgetSpecimenSingleRealUnit` | Preview NI | One actual row unit after real approval and policy reads, with a review-only and an apply specimen. Captured 5/2 is 2 plus 1/2 residual. Authorized 7/2 in the separate child case is 4 minus 1/2. |
| `silentKeepsEffectsAndOrderedMilestoneChoices` | Preview NI | 90 + 35 = 125 and crosses 100, 110, 120 exactly in that order. Internal jobs survive. Ordinary alerts do not. |
| `wholeOriginalDeficits` | Preview NI | 59A/60A. At 99 Main, ordinal 1 combines −99 Main with −1 unapplied or +1 debt. A subsequent −2 at Main 0 is all unapplied under Stop. At Main 100 the entire −100 fits. Reject/Retain each adds no credit. |
| `finishedPartitionAndRetainedFinancialBasis` | Preview NI | Fresh and held Finished originals. At hard end, the time and count requests have zero credit. The literal suffix `00000003 00000002 01 00 02` leaves +15 score alone at ordinal 2. EUR 4 of EUR 20 is 1/5, so the actual shipped #199 refund then removes 3 from 105. |
| `creditedOriginalReviewPreservesItsFinancialHold` | Preview NI after a real credited original, #199 financial hold, ordinary beside-complete hold and literal v19 advance | 80A. With a real v2 ordinary hold, Accept and Reject are ineligible, an alternate basis is stale, RetainHeld keeps both versions, and RejectHeld closes only the ordinary version. The claim, effects and financial version remain exact. |
| `unknownScopeStopsWithoutReadingThePrefix`, `refusedWriterAdmissionStopsWithoutStartingTheRow` | Preview NI after three-row setup | Unknown scope and actual writer admission refusal after an acknowledged prefix separately stop forwarding without another BEGIN or reconciliation. |
| `shippedLinkedRefundBoundaryPreservesFinancialHold` | Shipped financial marker path, after actual v15 financial/ordinary hold setup and v18/v19 advance | An absent/stale financial locator, incomplete ordinary closure or replacement leaves the mixed set unchanged. The valid linked closure preserves the one exact generated financial version and once-only score. |

The 40 invocations predict 30 preview NI stops, one producer NI, one execute
NI, one lookup NI, five marker refusals and two passing controls, excluding
Qt lifecycle hooks. These counts are source-derived.

B1 uses a genuine #199 hold before adding an ordinary beside-complete hold.
Its service stale probes use genuine #199 refunds with the same final
contribution, committed state token and timer, balances, original claim and
plan, and ordinary hold. One refund pays the full timer debit from Main and
leaves no financial hold. The other creates a different genuine locator.
The shipped financial reader exposes the generated active version `v1`.
The linked control separately submits a wrong version for the same hold
reference at the owning boundary. There is no unclaimed financial-hold fixture.

B2 retains the SAR2 and positive sparse score controls. It submits actual
#199 candidates retaining the valid score link and adding one zero-credit
terminal ordinal at a time. Each refusal checks that committed domain rows
and effect-port counters are unchanged. The ordinary Silent
application also checks the v1 result format and SAR1. B3 allows zero or one
final read for first-row rollback and retains the later-prefix checks.

Calls through `execute` also have their own linkable named NI marker. Cases that
need a freshly bound server preview intentionally hit preview first. Fixture
writes, migration reports, raw anchors and reached callbacks have their own
assertions. No first stop above was observed by execution in the author phase.

## Physical catalog proposal

Six statements, in the order in [the literal fixture](contribution_review_catalog_fixture.hpp).
The order is work table, entries table, CID index, acceptance uniqueness index,
terminal rejection uniqueness index, terminal-request table. Eleven new
projected objects comprise those six SQL-bearing rows and five SQL-NULL
autoindexes. The work key is installation/scope/ref; its immutable manifest
and mutable checked cursor share a key. The entries carry nullable verified
anchor FKs and an optional unique application FK. The adapter must compare the
CID and ReceiptId against the *same* existing original, the expected ordinal
and the typed outcome; separate foreign keys deliberately leave that verified
pair check observable. Reserved terminal rows hold only disposition and the
operation and retained plan request foreign keys. Terminal zero-credit is
verified in selected readers. Denied and unavailable journal rows have NULL
protected cells. Existing rows acquire no inferred disposition.

| Charge | Bytes |
| --- | ---: |
| v1–v18 cumulative SQL and repeated projection | 1,311,210 |
| v19 ordered SQL statements | 3,677 |
| Repeated protected objects in the v19 projection | 122,859 |
| Eleven v19 projection rows, including all text fields and present SQL | 4,389 |
| Full v19 projection, 225 objects | 127,248 |
| Full selected v1–v19 cumulative charge | 1,442,135 |
| Explicit normal v19 fixture limit | 1,442,136 |

Ordered SQL UTF-8 sizes are 696, 1977, 84, 121, 121, and 678.
The exact-admission control uses 1,442,135 and its one-below control uses
1,442,134. All are explicit fixture request limits within 2 MiB. Each
SQL-bearing projection charges type, name, table and present SQL. Each
autoindex charges type, name and table. The independent catalog-v1 frame
uses decimal byte lengths followed by a colon and the literal field bytes.
The v19 payload is 133,920 bytes with SHA-256
`407229e831e3f1368f2b5cba415c56fe0cc1b45a30ddfb496c39ec72504e8e87`.

### Existing catalog readers

| Selector | Exact protected marker before upgrade | v19 compatibility control |
| --- | --- | --- |
| `financial_sql::selected` (`sqlite_financial_catalog.hpp`), used by refund and linked hold paths | `18/milestone-target-edits-v1`, tier 2 | `SqliteFinancialRefundStore::active` on v18 and `19/contribution-review-v1`. Preserve tier 2. |
| `milestone_sql::catalog` (`sqlite_milestone_detail.hpp`), used by milestone groups, decisions, delivery, and target edits | `18/milestone-target-edits-v1`, tier 3 | `SqliteMilestoneGroupStore::pending` on both exact pairs. Preserve tier 3. |
| `ModifierRows` (`sqlite_modifier_activations.cpp`) | `18/milestone-target-edits-v1` | `SqliteModifierActivationStore::instance_revision` on both exact pairs. |
| `RateRows` (`sqlite_rate_overrides.cpp`) | `18/milestone-target-edits-v1` | `SqliteRateOverrideUnit::head` returns reserved root E before upgrade and after v19 reopen. |

All four select the latest row and require an integer version and the
catalog singleton's format and identity. Existing branches are:

- Financial returns pre-financial for versions 1–14. It selects tier 1 for
  `15/financial-refunds-v1`, `16/provider-credentials-v1`, and
  `17/rate-overrides-v1`, and tier 2 for the exact v18 pair.
- Milestone returns pre-group for versions 1–10 or `11/delivery-jobs-v1`.
  It selects tier 1 for `12/milestone-groups-v1`, tier 2 for
  `13/milestone-group-decisions-v1` and the listed v14–v17 pairs,
  and tier 3 for the exact v18 pair.
- Modifier accepts `14/modifier-activations-v1` and the listed exact
  v15–v18 pairs. Rate accepts the listed exact v17 and v18 pairs.

The v19 branch will require the exact byte-matched marker. Stage 1 leaves the guards at their protected pairs to expose
the four expected compatibility failures. `lookalikeV19MarkersStayClosed`
expects all four selectors to reject the case-changed v19 marker.
The migration runner's full-history matching is separately covered by
inspection, exact schema, cumulative budget and digest controls.

### Nullable SQL checks

The journal prefix helper normalizes only an SQL NULL receipt ID using
`isNull()`. Schema projections use `isNull()` for absent SQL. Protected
refusal rows use SQL NULL predicates. The new committed images compare
selected actual rows to prior actual rows from the same reader, including
their nullable fields. None compares a typed SQL NULL to an empty QVariant.

## Real writer budget witness design

Use the same configured single writer for actual review work. Setup can
migrate with the existing generous fixture profile then close it before opening
the selected profile on the same disposable store. Required profile for the
witness is 768 statement attempts, 65,536 bytes for one statement, 9 MiB bound
values, 768 result rows and 16 MiB result values. A separate default reopen
has 64 attempts and must refuse without admitting a row. Independent reader
assertions are outside the measured review units.

In the coordinator's admitted measurement build, collect per-unit consumption
where the real writer debits limits. At `SqliteWorkState::execute`, capture every
attempt, maximum serialized statement UTF-8 length, bound cell count and
writer-charged binding bytes. At actual result fetch, capture returned rows,
result cells and charged result bytes using the same value-byte accounting as
the writer. Track document bytes (manifest, basis, result, journal), including
every repeated bind and projection, before materialization. Preserve counters on
failed and rolled-back units. Correlate each snapshot with the actual BEGIN,
control acknowledgment and completion event and with its initial,
qualification, ordinal-write or final-reconciliation purpose. Publish only
counts and synthetic unit labels. The author observer associates work and
ordinal with actual phases. It cannot report SQL consumption.

Take measurements for the two one-row specimens, the 16-row pending and mixed
cases, a 15→16 retry, and the widest admitted silent/milestone, mixed-hold and
SAR2 examples. Include normal and after-stop reconciliation, duplicate
projections, every authority and policy read, guard sentinels, plan append,
selected bundle readback, hold closure, partition rows, milestones and jobs.
Run the shipped authority path separately with synthetic durable account,
session and grants under #183/#294 composition. This author packet's finite
retained-fact reads establish the measurement shape, with its live authority
cost still unverified.

The read-only proof must fit the same finite ceilings before the first row
write. Reserve 128 shared attempts/rows plus at most 384 for a row, or 40
per ordinal for a full 16-row reconciliation. Capture the selected row and
typed link reads on both reconciling units; the row-16 unit compares manifest,
cursor and row absence without re-reading the 15 link closures. Compare the
resulting work trace against at most one initial read, 16 qualification reads,
16 row units and one final read. Reserve 32 binding cells and 104 result cells
per charged attempt/row, 8 overhead + 128 payload bytes per cell plus 4096
payload bytes, and at most 2 MiB per direction for repeated documents. The
arithmetical ceilings are 8,585,216 binding bytes and 16,105,472 result bytes.
These are design ceilings.

For a calibrated supported SQL shape, compare a read-only proof at each exact
measured threshold against an otherwise identical threshold one below (or add
a single valid charged unit). Isolate attempts, peak statement UTF-8, binding
cells, binding bytes, result rows, result cells, result bytes and document
bytes one at a time. Other dimensions must stay below the same configured
limits. Observe identical independent physical state for every rejected
request, with no first journal admission. Repeat the comparison for an
otherwise valid 16-row request at 61,440/61,441 encoded entry bytes and
4,096/4,097 envelope bytes using agreed canonical codec fixtures. The
16-row maximum of 987,136 bytes fits 1 MiB. The authored overlong reason and
synthetic SQL fanout controls cover clearly over-bound shapes; they cannot
calibrate the exact packed-entry and envelope byte edges. Freeze the measuring
seam and exact encodings before treating one-over as witnessed. Separately
introduce a live shape change after a normal prefix, distinguish
RowBudgetExceeded from a known writer rollback, and reconcile that prefix
once if eligible.

Stage 1 owns this baseline, independent review and the later admitted intended
red. Stage 2 owns implementation, measured caps and later implementation
verification. Authored cases, literal arithmetic and successful linking
confer zero runtime, host-latch or capacity credit. Production settings remain
the shipped defaults.

## Decisions for the coordinator

- The DTO uses a redacted outcome without the attempt disposition when a
  historical outcome loses disclosure. Safe interrupted summaries still have
  attempt dispositions. If the coordinator wants the redacted case represented
  as a separate variant rather than an optional field, preserve the same
  ordinal-and-denial disclosure shape.
- Coordinate the exact owned-document framing for the remaining threshold
  controls. If the canonical supported envelope cannot reach 4,096 bytes,
  capture that proof and test the ingress guard separately. Prefer reachable
  codec vectors over arbitrary padding.

- Wrong-store recovery and the shipped host no-reply latch need the coordinator's
  trusted recovery and host fixture. The component packet covers real uncertain
  acknowledgments and matching-store retries. It leaves wrong-store proof
  to the existing recovery authority instead of treating an open SQLite file
  or the generic catalog identity as recovery evidence.
