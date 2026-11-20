# ACCT-005 Stage 1 author packet

Tier A. Frozen revision 2 at `8841829008a55cfcb3e850abd785c798cbb5cc32` and contract blob `f05a2419a23301a3a2d81cdceb4461128a986333`, integrated onto the protected v16 base at `a5854627dfdc76654169d4b56f35bb6f4bd7bece`. The example body SHA-256 is `47751fb964fdc80ce7258058bc8a4a1639ce271934c1c598b2bb0e673b96cc5b`. [Freeze](https://github.com/Kernol12/subathon-bot-ai/pull/742#issuecomment-7023283524) · [Activation](https://github.com/Kernol12/subathon-bot-ai/issues/197#issuecomment-7025688441)

## Expected first stops after admission

| Cases in the Qt source | First intended stop | Independent observation beyond the stop |
| --- | --- | --- |
| `catalogProducerAdvancesExactlyOneStep` | Explicit producer NI | Genuine v16 with a previously committed bundle and milestone, exact v17 delta, full schema and framed history, empty new relations |
| `literalCatalogBudgetAtAndOneBelow` | Passing control | One byte below cumulative UTF-8 fails admission, exact charge advances |
| `exactV17FinancialMarker`, `exactV17MilestoneMarker`, `exactV17ModifierMarker` | The respective shipped selector rejects the new exact marker | Each first verifies its v16 control and independently advances full literal v17 |
| `lookalikeV17MarkersStayClosed` | Passing control | Exact v17 setup and then a case-changed history marker; typed rejection for each selector |
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
| `shippedLinkedRefundBoundaryPreservesFinancialHold` | Shipped financial marker path, after actual v15 financial/ordinary hold setup and literal v16/v17 advance | An absent/stale financial locator, incomplete ordinary closure or replacement leaves the mixed set unchanged. The valid linked closure preserves the one exact generated financial version and once-only score. |

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
| v1–v16 cumulative SQL and repeated projection | 1,065,558 |
| v17 ordered SQL statements | 3,677 |
| Repeated protected objects in the v17 projection | 96,456 |
| Eleven v17 projection rows, including all text fields and present SQL | 4,389 |
| Full v17 projection, 172 objects | 100,845 |
| Full selected v1–v17 cumulative charge | 1,170,080 |
| Explicit normal v17 fixture limit | 1,170,081 |

Ordered SQL UTF-8 sizes are 696, 1977, 84, 121, 121, and 678.
The exact-admission control uses 1,170,080 and its one-below control uses
1,170,079. All are explicit fixture request limits, with a fixture ceiling
of 2 MiB. Every repeated SQL-bearing projection charges type, name, table
and present SQL separately. Each autoindex charges type, name and table.
The independent catalog-v1 frame uses decimal byte lengths followed by a
colon and the literal field bytes. Its v17 SHA-256 is
`544c88a07e8555fb2a912b6c558e01e0174d2ccb1b1d55a60e602bc455003338`.

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

- A genuine shipped #199 financial hold is born against an already credited
  original and exposes a generated active version, rather than arbitrary
  version labels from the examples. The real linked-closure case uses a known
  completed original and observes the exact generated version. To exercise an
  unclaimed whole original in the presence of a financial hold, either establish
  a separately supported reachable provenance or use a synthetic comparison
  control with no application authority. Prefer the shipped reachable control
  and keep whole-input apply through a financial hold conditional on owning
  policy.
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
