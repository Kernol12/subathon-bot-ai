# MILESTONE-006 Stage 1 slice three

[#238](https://github.com/Kernol12/subathon-bot-ai/issues/238) frozen r2
`89925f45567d1b0dcc8eec58a800aef9d99c99fd5fcf8592476f69aa501c6104`, protected
`e451293a4bc9a6faf8b11bfaa74714f15e85d9c4`. The credited slice-two parent is
`1f7d297036df3a7dd891537da60b3db91b91d143`, receipt
`1b4c5e2d8981b28fa3d167bba77e3ec867ab1276279b51ffe824f2f2ec148a94` and closing
`0618b8f499edfa8a2adb4e88bd6de76a854cc12532e5cf68749d044338983e21`.
Fix round 1 follows the review of `e73a9a8e7b31cf49d1029b99999921eb7b568803`
with SHA-256 `13f59cf1dbc3ba8049623a1149550233ef4774ad98f1384f95fdaaa0f2d65689`.
The rebuild uses the retained protected pin above. Remote protected main
has moved to `71a1c2e5f661f8d144bac6bdf93a6aaec07d5302` through #751.
Exact producer and custody comparison against that pin remains an
admission prerequisite. This fix does not update shared refs.

Enable `SUBATHON_BUILD_MILESTONE_006_SLICE3=ON`. Disposable databases use
only shipped migrations, then the exact v21 inspection and explicit fixture
profile already used by slice two. The application lane uses real receipt,
bundle, delivery enrollment, group capture, acceptance and track setup. Qt
test execution and admission remain separate. All outcomes below are source
predictions.

| Runner and selector | Rows | Independent oracle |
| --- | ---: | --- |
| Application `r2R4ApplicationClaimInvalidatesMixedPreview` | 1 | Pass. Claim A3 at r2 after the mixed preview. Both stale confirm and a fresh mixed preview refuse without a version or result. A1 remains unstarted. Replacing A1 prevents any predecessor handoff, and only the first r1 successor claim reaches authority and writes sending r2. |
| Application `r4R5ApplicationPreviewRebindsCurrentGates` | 4 | Pass. Unchanged Defer permits selection without release. Resolving Defer and releasing, Stop Continue while held and unreleased, and C9/C10/C11 with equal capture bytes each stale the old preview. Fresh selection succeeds where the exact slot is still eligible. |
| Application `r7ApplicationKindChangesAreAtomic` | 1 | Pass. Both direction changes paired with an otherwise eligible slot refuse as a unit. A's preview is invalid for independently eligible B even with equal requested bytes. Authorized A confirmation leaves B at its original version. |
| Application `s5ApplicationHeadUsesExactTrackHold` | 1 | Pass. A's durable same-track Hold blocks B's original application release. Exact Resume releases only B, keeping A at release 0. |
| Application `s1ApplicationStopRetainsFullHistory` | 2 | Pass. Three originals start, chat settles, and the supplied +30,000 effect and receipt commit once. Stop cancels slots 5 and 6, adds one Hold and identifies M alone. After reopen, already-started slot 4 can finish with matching evidence or be selected by the original stranded pager and settled unknown. |
| Application `s1FullHistoryAcknowledgesBeforePlayback` | 1 | First stop at acknowledged playback handoff with the full retained history. The existing server Stop port must receive M once, after currentness and a visible durable marker. The second turn calls nothing. |
| Recovery `strandedReplacementPaging` | 2 | First stop at the missing page entry after real application/edit replacement, claim, finish, Stop and cold readback. A bound of one keeps each page to one candidate and advances through all three real version keys, even when a page has no sending result. The original-job pager sees none. The replacement scan must find the exact stopped r2 sending version, excluding cancelled queued and delivered versions. Exact existing finish changes it to r3 outcome-unknown and it disappears from discovery. |
| Recovery `replacementCatalogBudgetRefusesWholeWork` | 1 | Pass. The protected exact lookup starts with two catalog integer cells at eight bytes each. Its eight-byte limit therefore refuses the whole work before any candidate is read. The writer publishes no value and leaves history intact. The same lookup succeeds at the ordinary budget. |
| Recovery `s7EditInitiallySkippedAndFullyHistorical` | 2 | Pass. The actual edit initially Skips C, or releases an empty C with no live presentation. A fresh Stop writes no result, fence, cancellation or Hold. |
| Composition `t1T2WarmWriterCompletion` | 3 | First stop on warm adoption of the real writer result. Rollback preserves 11,000. Lost acknowledgement leaves A visible and can fence B at 11,000 until a late normal readback, which keeps the natural gap elapsed. B may start during adoption or the following advance on the same 11,500 sample. Either path must release B exactly once at elapsed zero and show 100 ms at 11,600. Early readback at 2,600 interrupts A and gives 3,600. Real exact replays at 3,100 and 3,700 neither move that boundary nor interrupt B. |
| Composition `r7PositiveOwnerPreviewIsConfirmable` | 1 | First stop at the authorized owner preview with actual session, current authority and exact application target. A real writable confirm on the protected path must consume the returned opaque preview and select exactly one successor. |

The [exact selector inventory](expected-selectors.tsv) predicts **12 passing
controls and 7 first stops**, excluding Qt init and cleanup.

## Owner entry mapping

Each test-only stub may become a single call to the named owning server
entry using mechanical argument, enum or DTO projection only. Keep every
fixture input and assertion. No forwarding body may classify, discover,
decode, dispatch or write evidence. Closing review must verify each
forwarding body and its owning product entry. This slice adds no production
declaration.

- **Replacement recovery** uses `stranded_replacement_attempts(session, track,
  after_version, limit)` in the milestone delivery owner. The existing v21
  version identity is scoped by installation and track. Its attempt history
  carries revision and state. A scan requires no job body. The component
  receives an already-quiescent writer session, as the original pager does.
  The delivery owner should declare the page and exact attempt descriptor
  corresponding to `ReplacementRecoveryPage` and `StrandedReplacement`.
  The fixture may mechanically project those DTOs to its assertion shapes.
  Review the candidate-scan bound and cumulative result accounting at source,
  including whole-work failure without a partial page when a later candidate
  exhausts the budget. The owning replacement pager and its row projection
  do not yet exist, so this slice cannot independently price the transition
  between the first and second candidates. The eight-byte control exercises
  catalog preflight only. Recovery writes use the existing exact
  `finish_replacement` entry only after selection. Live process or epoch
  fencing remains outside the fixture.
- **S1 and P2/P3** use the milestone playback owner's
  `process_milestone_playback_turn` and the already declared
  `PlaybackStopAdapter::request_stop` interface. Slice three's
  `handoff_playback(writer, handoff, authority, adapter)` forwards as
  `process_milestone_playback_turn(writer, PlaybackTurn::handoff, handoff,
  authority, adapter)`, using the same owner turn selector as slice two.
  Currentness and reliable
  outcome remain on `PlaybackAuthority`. For late activation the owner
  must pass the exact `LatePlayback` identity to `stage_late_playback`,
  then rediscover the retained intent from v21. On cold restore it must
  read the retained marker without assuming a live renderer. Only a later
  normal `stage_playback_handoff` can authorize a call outside SQL. The old
  outcome entry only accepts a marked attempt. Retiring obsolete unmarked
  playback without claiming a handoff still needs an owner read/write path
  over existing v21 evidence. Adapter confirmation and real renderer
  ownership stay outside #238.
- **Warm T1/T2** use `adopt_milestone_stop_completion(timeline, sample,
  command, source, work, published)` in the milestone timeline owner.
  Exact work status and normally published decision come from the real
  writer. The owner interface should name the command-versus-readback
  completion source, with a mechanical mapping from the fixture's
  `StopCompletionSource`. Pass the existing command, work and decision types
  directly. The timeline owner must distinguish the first accepted readback
  from a later replay of the same operation. This is a component boundary.
  #294 remains responsible for live host wiring and acknowledgement routing.
- **R7** uses `preview_milestone_replacement(session, authority, command)`
  in the milestone interruption command owner. It must produce the exact
  protected storage preview on the positive path and preserve the already
  credited absence-versus-denial tests.

## Remaining selectors and owner decision 184 A

Typed ordered old/new disclosure and ordinary configuration publication
still need their own selectors. Under [184 A](https://github.com/Kernol12/subathon-bot-ai/issues/238#issuecomment-7087890719),
a replacement inherits the verified original automation format through its
predecessor chain when the owning profile confirms compatibility.
Current authority supplies sender-selection mode in the authorized preview.
Existing durable history retains the exact qualified sender. Incompatible
or unproved captures stay gated. #816 tracks incompatible replacement
formats. No new v21 BLOB encoding, schema change or disclosure fixture is
part of this fix.

Open component details include application R5's Defer-to-refund-Review
replacement while preview is outstanding (existing positive controls cover
both origins in the protected readers), R7 parity for every cross-origin
invalid shape, the edit-origin full mixed S1 stranded finish through one
combined fixture, and warm Hold/replay with media on the same owner. This
slice uses application Continue without a media port for warm timing and
retains the separate marked/unmarked cold and full S1 media witnesses.
Production capture publication, renderer proof and host wiring stay with
their owning entries.
