# MILESTONE-006 Stage 1, first slice

[#238](https://github.com/Kernol12/subathon-bot-ai/issues/238), Tier A, frozen r2
`89925f45567d1b0dcc8eec58a800aef9d99c99fd5fcf8592476f69aa501c6104`.
Protected producer `c905cc840e11a33224d09fb5bc94ae4d6c57724d`. Frozen selector
matrix `84998d0f2fbbc26e767815f447c3f79a61be8d0d0f3be2004feef92fee24152c`.

Opt in with `SUBATHON_BUILD_MILESTONE_006_STAGE1=ON`. The two runners have
static CTest registration, with each Qt slot and data row retained by moc.
The timing runner uses the protected timeline for presentation, owner turns
and release. Its recording port supplies only independent #236 facts. The
test-only typed update entry is the missing application checkpoint. It
receives acknowledged Stop, unresolved Stop or acknowledged Resume facts.
It adopts a fact without walking the queue. A separate advance can occur at the
same sample. In Stage 2 this test adapter will forward to the server-owned
entry, with the forwarding diff reviewed and no owner logic in test code.
Storage witnesses use disposable writers, normal owning entries and shipped
catalogs. Side effects such as playback handoff are outside the timing fixture.

## Predicted first stops on the protected producer

| Selector | Reachability and expected result |
| --- | --- |
| `s2AcknowledgedStop` | Real A release at 0, still active at request time 1,800. First stop at application of acknowledged Stop at 2,000. Expected B starts with full duration at 3,000 or the later 3,100 turn. Output at 2,400 and separate adapter observation at 4,000 have no owner update. |
| `s3IndependentGates` | Real A presentation and two supplied #236 causes on B. Stop rows first reach the 2,000 application. At the derived 3,000 boundary R still gates B after D resolves. After R resolves at 3,100, an independent manual pause holds until 3,200. Natural-boundary controls are predicted passes at 11,100 and 11,200. |
| `s4ExactTargetAndNaturalBoundary` | Real exact A release and protected window or natural gap. First stop at applying Stop to same-ref pending B, B during the gap, or A exactly at expiry with an unstarted queued slot. Natural boundary stays 11,000. B is passed without release when B was stopped. The supplied #236 Skip disposition control is predicted to pass. |
| `s5ZeroGapAndIndependentHolds` | Real A release; first stop at acknowledged A Stop or at the earlier acknowledged C Stop supplying H1. Zero gap is eligible on a separate advance at 2,000. Exact Resume of H2 leaves H1 active. The 1,000 gap elapses before the single Hold resolves at 3,500. |
| `s8ReleaseRaceTiming` | Restore a ready, unreleased exact head without presenting, or advance to a real A release. First stop at the acknowledged update. Stop-first allows B on the same-sample advance with no A release. Active release-first preserves A's one release and uses boundary 3,000. |
| `t1KnownRollbackKeepsNaturalBoundary` | Predicted pass. No owner update from the rolled-back turn. A's original window and 11,000 boundary survive. |
| `t1UncertaintyGatesSchedulingWithoutFreezingElapsedTime` | First stop at applying the scheduling fence at 2,000. A remains active through 9,999, time keeps running, and B is fenced at 11,000 until normally acknowledged readback. |
| `t1UncertainStopReadbackWhileActive` | First stop at the 2,000 uncertainty fence. The integrated sequence keeps A active at 2,300 and applies readback at 2,600. B stays unreleased at 3,000 and 3,599, then starts at 3,600. This distinguishes a backdated gap and a retained uncertainty fence. |
| `t1FirstReadbackUsesReconciliationSample` | First stop at applying the acknowledged exact readback. Separate readback entry witnesses expect 3,600 for a read at 2,600, the original 11,000 after a read in the natural gap, and immediate eligibility on the first advance after an already elapsed gap. |
| `s6aUnstoppedCrossing_v20`, `explicitDeferredSkip_v20` | Predicted passes on both origins. The live refund owner and the v20 normal decision readers prove exact 1,020→995 and 1,010→990 crossings, 2/3/0 active causes under Continue/Review/Skip, and distinct configured versus explicit Skip provenance. Preserve protected `terminalSkipEndsAllRemainingHolds` and the existing v21 unstopped controls. |
| `s8ReleaseRaceStore` | Predicted passes through the real v21 application and edit release overloads and acknowledged v21 Stop. Release bits follow the winner and Stop cancels one unstarted slot in either ordering. |

A failure in setup, catalog inspection, protected release, compilation or
linking is outside these predicted behavioral first stops. A runtime run
and a Tier A review have their own gates.

## Second slice checklist

Follow the frozen selector matrix for full inputs, expected results and
normal-entry names.

- **R1–R2** — bind replacement preview to the exact original plus untouched
  slot and confirm bytes/sender. Add the cached owner release and reselect
  path, the mixed-started refusal and eligible subset, and exact successor
  claim with a one-shot dispatch witness. The protected pending list returns
  original captures. Version IDs and claim disposition alone cannot supply
  the selected dispatch snapshot.
- **R3–R5** — retain the four-event attempted requeue and fence it. Exercise
  preview invalidation by sending revision, nonreused equal-value config
  revision, Stop and changed D/R/release gates, plus replacement-first.
- **R6–R8** — cover both bonus race orders and the zero-actual receipt, both
  typed origins, every invalid authority/intent/kind case, and confirmed
  rollback of each unit. Direct storage conflates absent authoritative
  facts with denial. The owner witness must distinguish unavailable.
  Locate automation format/sender-mode facts and an ordered old/new
  disclosure view before claiming the full R1/R7 preview witness.
- **S1** — use actual started/sent/applied history and two unstarted slots,
  omit choice, and separately prove committed Hold and exact playback
  publication. Seed supported activation from its owning fake.
- **S5/S6** — add durable exact subset Resume and compose the held-stop
  warm/cold head on both origins. In each new-refund policy lane verify the
  physical balance, no new stopped-A decision, saved R replay before and
  after independent Continue resolutions of D and R, and eventual B-only
  release. The existing successor Skip lane uses a subsequent explicit
  Skip and cannot stand in for Continue D then Continue R. Seed real
  application track membership and exact current edit authority for
  positive release.
- **S7/S9** — run real explicit Defer Skip after Stop and exact fresh Stop
  refusals. Exercise empty, duplicate, stale, unrelated, foreign and
  unauthorized Resume sets with the authorized subset/atomic control.
  Preserve independent D/R provenance and scheduling.
- **P1–P3** — supply exact positive activation/current generation and
  capability from a fake media owner. Exercise obsolete generation, late
  same-attempt versus wrong-attempt registration, and durable one-way handoff
  and recovery with current proof. The protected delivery adapter has only
  `start`. The old stranded pager covers original origins only. Retain
  known replacement-attempt finish as a distinct witness and report the
  replacement discovery gap.
- **T1/T2** — retain real `stopAcknowledgement` fault/readback controls.
  Exercise exact retry, changed choice and revoked retry plus cold effective
  selection, independent Hold and marked media intent. Existing exact
  selection returns version IDs. Establish the owning capture and
  scheduling boundary before claiming a cold effective-start witness.
- **T3/T4/U1** — cover stable Replace and set-order-insensitive Resume
  retries and their conflicts. Use independent five-axis charge oracles
  with exact-fit and one-below refusal. Keep the protected populated/fresh
  successor catalog, SAR2 and normal reader controls with wrong-marker and
  missing-required-shape refusals. Production catalog admission and host
  composition remain with their owners.
