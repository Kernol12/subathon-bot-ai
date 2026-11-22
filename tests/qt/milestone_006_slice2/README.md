# MILESTONE-006 Stage 1 slice two

[#238](https://github.com/Kernol12/subathon-bot-ai/issues/238) uses frozen r2
`89925f45567d1b0dcc8eec58a800aef9d99c99fd5fcf8592476f69aa501c6104` and selector
matrix `84998d0f2fbbc26e767815f447c3f79a61be8d0d0f3be2004feef92fee24152c`.
Fix round one builds on initial slice-two commit
`4f0dd7ace5067ba0a1a8de4feaff9be0b1f4325b` and its independent review
`b353d0fd7a312e6589d995ba46d09572d49336735d080b05fad19387ea3a762d`.
The credited slice-one ancestor is
`495c8be26df212d497523322a77e77719a3bb24a`, receipt
`c6ab446d23f22c6ce27d441dd2092f22f73ffa76bf6abbaa9e5f024fd6f88d7c`.
The producer and existing fixture paths match protected
`e451293a4bc9a6faf8b11bfaa74714f15e85d9c4`. Owner decision 162 A uses existing
v21 history. The setup advances only disposable fixtures through that shipped
catalog. #318 owns the sole migration.

Enable `SUBATHON_BUILD_MILESTONE_006_SLICE2=ON` for the three new Qt runners.
Use the existing `SUBATHON_BUILD_STORAGE_027_STAGE1=ON` flag for the protected
runners named below. The following outcomes are source predictions. Setup,
catalog or library failures invalidate a behavioral first-stop prediction.
The author has not invoked either set of runners or collected runtime credit.

## New selectors and predicted first stops

| Runner and selector | Data rows | Independent oracle |
| --- | ---: | --- |
| Commands `r1PendingReplacementRetainsOriginalAndBonus` | 1 | Pass. Preview leaves the history and journal untouched. Confirmation changes only slot 1. Readback gives exact bytes 00 81 and S at queued r1. The original job remains readable; the successor can claim r2 exactly once. |
| Commands `s1StopRetainsStartedHistory` | 1 | Pass. Slots 1, 3 and 4 have begun. The supplied bonus effect fake stages +30,000 through the real application store. One receipt and the balance prove one application of that effect, while the product amount policy remains a separate boundary. Omitted choice leaves only cancellations 5/6, one Hold and one intent for M. The in-flight slot 4 can still append its r3 finish. |
| Commands `s5DurableSubsetGatesRelease` | 2 | Pass. One or two independent Holds cover the typed application/edit origins. Continue preserves H1, and resolving H2 alone keeps C blocked. Exact last resolution permits only C to release. |
| Commands `s7FreshStopRefusalsHaveNoWrites` | 2 | Pass. Absent, invalid choice, denied and already-stopped fresh keys leave the command, cause, fence, cancellation and release rows unchanged. A real same-target Stop and exact replay distinguish refusal from fixture failure. |
| Commands `t1T2StopAcknowledgementAndReadback` | 6 | Pass. Both origins reach the real writer after release with independently supplied exact presentation evidence. Rolled back, unknown outcome and abnormal committed return publish no result. Only normal readback exposes the retained single Stop and Hold. Exact retry, changed choice and current denial leave history unchanged. |
| Commands `s9ResumeAtomicRefusalAndDisclosure` | 1 | Pass. Empty, duplicate, H0 resolved, missing, D, R, known denial, revoked grant and per-cause nondisclosure cases preserve the pretrial snapshot. Positive H1+H2 writes two resolutions in one result and leaves D/R active. |
| Settlement `r1ApplicationAutomationSelectionFencesOriginal` | 1 | Pass. Exact pending application confirmation records bytes 00 81 and S at r1 and preserves release 0. Normal release subsequently succeeds, but only history lookup can select the fenced predecessor. |
| Settlement `r3ApplicationAutomationRequeueRemainsStarted` | 1 | Pass. Same application replacement is previewable at r1. It refuses after r2 claim, r3 known failure and r4 retry. Stop fences the next dispatch before authority, retaining four events and zero unstarted cancellations. |
| Settlement `s6StoppedRefundRetainsExactCauses` | 6 | Pass. Both origins × Continue, Review and Skip. 1,020−25=995 against 1,000 retains deficit 5. 995+15−20=990 gives deficit 10 with no new A/B policy observation. Only exact Continue D then Continue R lets B release. A remains at release 0. Authorized replay before and after resolution cannot debit again. |
| Settlement `s7OnlyExplicitSkipEndsReview` | 2 | Pass. Stop alone retains D/R. Explicit Defer Skip stores its own decision and ends the earlier Review with Defer provenance. Only B can release. |
| Settlement `s6OwnerOrderedReconciliation` | 4 | Red at exact warm/cold adoption, after real v21 readback on both origins. Cached or restored A remains ahead of B under D/R or R alone. Once R also resolves, the next owner advance starts B at elapsed zero with no release or gap for A. |
| Owner `r1CachedReleaseSelectsEffectiveCapture` | 1 | Red at exact effective selection. Normal readback selects one distinct successor and original bonus while the owner cache still has the predecessor. Expected selected bytes 00 81 and S. |
| Owner `r2AcknowledgedSuccessorDispatch` | 3 | Red at dispatch publication. Normal r2 commit publishes one exact body, and its duplicate never reaches dispatch authority. Lost acknowledgement and known rollback both retain the selected provisional body at the seam, yet publish nothing. Reopen finds r2 sending only in the lost-ack lane. |
| Owner `knownReplacementAttemptCanSettleAfterReopen` | 1 | Pass. Known exact r2 sending survives reopen and appends r3 outcome-unknown with the supplied evidence. This selects by an already-known attempt. |
| Owner `r7AbsentAuthorityIsUnavailable` | 2 | Red at owner admission on a real writer session. A positive preview and protected known-denial control precede the seam. The actual authority returns absent facts or known denial. Expect unavailable and denied respectively, with unchanged journal, versions and attempts. |
| Owner `p1ExactPlaybackHandoff` | 2 | Red at owner handoff for current/stale generation. Exact X gets one call only when current, with its marker already visible to the fake adapter on an independent connection. Unknown Y and the sending-only attempt get none. Stale X instead acquires obsolete evidence with no call. |
| Owner `p2ExactLateActivationIsScheduledOnce` | 1 | Red at owner late registration with a known started attempt and earlier Stop. Duplicate and wrong-attempt submissions add no second intent. Exact attempt 2 can hand off once on a later turn with the marker independently visible at the Stop call. |
| Owner `p3ColdHandoffNeedsPositiveProof` | 4 | Red on cold restoration after exact Stop readback. Cold restoration alone calls no media. Only an unmarked positively proven instance permits a later one-way handoff. Without proof the unmarked intent stays pending, with no marker or outcome evidence. Separate confirmed evidence leaves the Stop result immutable. |
| Owner `t2ColdRestoreSelectionHoldAndMarkedPlayback` | 2 | Red at combined cold restore with the writer, exact commands and authority. Both stores retain C's unstarted successor and B's exact Hold. The independently marked and unmarked stores require uncertain and pending respectively, with neither a new window nor a media call. |

The table nominates 24 passing controls and 19 intended first stops, excluding
Qt init and cleanup. The passing controls are 13 commands, 10 settlement,
and 1 owner. The predicted first stops are 4 settlement and 15 owner. The [exact selector inventory](expected-selectors.tsv)
pins source call and throw locations for a separate admitted diagnostic.

## Protected normal-entry selectors retained for this slice

The protected suites remain selectable by their existing names. For multiple
rows, select the whole function by its exact Qt selector when admitting it.
These are supplementary protected controls with anticipated passing results,
rather than new #238 runtime credit.

| Frozen row | Runner and selector |
| --- | --- |
| R1, R4, R8, T3 | Commands `replaceRetainsOriginalAndFencesStart`, `staleReplacementCannotAppendFirstSuccessor`, `replaceAcknowledgementAfterReopen` |
| R2, R7 | Commands `replaceExactMultiSlotSelection`, `invalidReplacementShapeAndTypedOrigin`, `freshCommandRefundEligibility` |
| R3 | Commands `attemptedRequeueCannotBeReplaced` with all four original events retained |
| R4, R5 | Commands `replacementRebindAfterFactsChange`. Readers `stoppedOccurrenceExcludesNewRefundPolicy:application-replace-defer-with-real-refund-review` and `:edit-replace-defer-with-real-refund-review` |
| R6, R8 | Commands `bonusSelectionRace`, both origins and committed zero-actual control. Actual Main delta is +30,000 or +20,000, with the losing version adding zero. Rollback leaves the original eligible. |
| S7 | Commands `stopRejectsRealSkipAndFullyHistorical`, `freshCommandRefundEligibility`. Existing #236 `terminalSkipEndsAllRemainingHolds:deferral-resolution` remains intact. |
| S9, T3 | Commands `foreignResumeSet`, `resumeSetReplay` |
| S1, P1–P3 | Commands `playbackScopesAndUnknownCapability`, `latePlaybackIsSeparateFromImmutableStop`, `playbackHandoffAndOutcome`. Readers `applicationPlaybackRetainedAllowance` exposes the application origin. |
| T1 | Commands `stopAcknowledgement`, `presentationOnlyStopStorageGate` and `releasedStopPresentationKnowledge` |
| T4 | Commands `boundedReferenceProjectionCountsFixedCells`, `stopLargeClosure`, `commandBudgetEdges`, `bonusReplacementBudgetEdges`, `playbackBudgetEdges` |
| U1 | Catalog `compiledManifest`, `literalUpgrade`, `manifestLimits`, `writerLimits`, `corruptSuccessor`. Readers `existingSelectors`, `stoppedPartitionedOriginRemainsProved`, `partitionedOriginDistinctEntry`, `sar2HeadCannotBeBypassed`, `freshSar2OwningStageAndReplay`, `denseLegacyAndSuccessorTerminalGate`, `independentV11DeliveryAcrossReopen`, `financialAndOrdinaryHoldClosureAfterAdvance`, `legacyAndLiveFinancialEntries`, `milestoneApplicationDelivery`, `editAttemptAcrossReopen`, `modifierSelectedRevisions`, `rateSavedSelection`, `realSavedRateHeadAndNonHead`, `earlierCredentialSelectedScope`, `initialCaptureAfterUpgrade`, `freshReviewPreviewExecuteFinalRead`, `reviewJournalReopen`, `sixteenEntryReviewKeepsTheProtectedProfile`, `reviewCatalogCorruption`, `protectedIntakePageAndAttemptReopen`, `protectedIntakeLongExactAttempt`, `protectedIntakeAdoptionReopen` |

For T4, 256 × (4,096 + 8) is 1,050,624, which exceeds 1,048,576 by 2,048
before other cells. At 255, the reference subtotal is 1,046,520, leaving
2,056 bytes. The protected two-read fixture charges 128 fixed bytes plus
8 bytes per ordinal, yielding 1,048,688 at 255 and 1,044,576 at 254.
The actual Stop includes its additional fixed and per-member transaction
charges. Its five-axis independent exact and one-below cases remain in
the protected budget oracle. U1 expects the protected 253-object v20
projection and 276 objects at v21, with the same old prefix, and a cumulative
manifest charge of 1,748,014 UTF-8 bytes under the explicit fixture profile.

## Owner entry handoff

The Stage 1 throw stubs may only become single calls to the named product
entries below. Keep all fixture inputs and assertions. A forwarding body may
use a mechanical argument projection. It must never classify, publish, decode,
schedule, call the fake, or store evidence itself. Closing review must check
each replacement against that rule and verify the bodies in the owning module.

| Stub | Proposed owning server entry and remaining work |
| --- | --- |
| `select_effective_release` | Milestone interruption owner `select_effective_milestone_release`. The existing storage read returns version IDs. The owner must select the body and preserve the captured bonus through release. |
| `publish_dispatch` | Milestone delivery owner `publish_acknowledged_replacement_claim`. The existing claim returns a disposition; capture ownership and acknowledged dispatch remain open. |
| `preview_at_owner` | Milestone interruption command owner `preview_milestone_replacement`. It must preserve the distinction between absent facts and denied grants. |
| `process_playback` | Milestone playback owner `process_milestone_playback_turn` for late registration, cold reconciliation and separate handoff. The only new declaration is `PlaybackStopAdapter::request_stop` in `server/include/subathon/server/milestone_playback_stop.hpp`. The renderer or connector adapter supplies the implementation. Currentness and evidence stay on the existing `PlaybackAuthority` boundary. The fixture fake implements the declared Stop port and observes the durable marker at the call. No #804 or #294 API is selected by this declaration. |
| `restore_committed_queue` | Milestone interruption owner `restore_interrupted_milestone_queue` using the current writer and authorized exact selectors. Read the retained one-way marker and independently selected version and Hold through v21 history. |
| `apply_ordered_selection` | Milestone timeline owner `adopt_ordered_milestone_selection` for warm reconciliation and cold adoption of the exact ordered Stop and independent causes. The S6 fake projects D/R at the intermediate turns from the separately asserted store readbacks. Those intermediate scheduling assertions alone do not prove adoption. The final advance with cached A and cleared causes forces exact adoption, since a no-op would release A. |

## Remaining producer and selector details

- **R1/R7 capture and disclosure.** Ordinary configuration publication for a
  later group is still a distinct owner selector. The protected
  `AuthorizedCapture`, preview and `subathon_milestone_slot_versions` have
  action, payload, route and qualified sender fields, with no standalone
  automation format or sender-selection mode. The opaque preview exposes
  no ordered old/new disclosure view. Identify the owning representation
  and source before treating these fields as proved. The cached selection
  fixture gives the owner exact v21 identities and expects the body from
  the actual owner entry. It supplies no decoder. The strict absent-versus-
  denied owner oracle remains required. Direct protected preview collapses
  both cases to denied and cannot serve that oracle.
- **Original application automation and S1 finish matrix.** R1–R5
  selected automation, R7 full typed mixed-patch behavior and full S1
  started-history mix primarily use the real edit lane. R6/R8 cover both
  bonus origins; S5/S6/S7/S8/T1 cover both Stop origins. Protected application
  delivery and playback selections are complementary witnesses. Application
  automation still needs released mixed-slot selection, stale preview details
  and the full S1 history mix. Other remaining selectors include S5 with a
  later application release under exact track membership and the edit lanes
  for S7 initially skipped or fully historical. The full S1 owner publication
  case still needs a single fixture spanning all of its started history
  and post-commit playback, beyond the existing separate store and P1 witnesses.
- **R2/P2/P3 recovery.** Original stranded queries page application/edit
  original jobs only. Exact replacement lookup and finish can settle a
  known attempt. They cannot discover it. A bounded owner discovery entry
  must select replacement sending attempts from the existing v21 version
  and attempt history without inventing data or falling back to originals.
  The protected `stage_playback_handoff` refuses stale proof and
  `stage_playback_outcome` currently requires a marked attempt. P1's
  obsolete-with-zero-call case still needs an owner path that uses
  existing v21 history consistently with the one-way handoff rule.
- **T1/T2 acknowledged integration.** Slice one covers exact timing
  first stops and this slice covers real writer and cold facts. The warm
  dispatch from actual writer acknowledgement into the timeline, and
  explicit replay with no second timing or handoff on that same live
  timeline, remain composition selectors. #294 owns live host wiring.
- **Authority scope.** Synthetic grants and playback facts support component
  evidence. Real current configuration, actor/disclosure authority, renderer
  or provider capability and host composition require their owning entries.
  No new table or column is proposed for this slice. If any unresolved
  v21 representation needs new storage, 162 A requires a numbered owner
  decision before further work on that representation.
