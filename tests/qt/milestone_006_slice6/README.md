# MILESTONE-006 Stage 1 slice six

[#238](https://github.com/Kernol12/subathon-bot-ai/issues/238), Tier A, frozen r2
`89925f45567d1b0dcc8eec58a800aef9d99c99fd5fcf8592476f69aa501c6104`.
Protected producer `71a1c2e5f661f8d144bac6bdf93a6aaec07d5302`.
The credited parent is `af73b2027b9987c1bf528f2f1de52dc4b7f18aa1`,
tree `fd37e3960dac73aa19c3b94feab7787d90d738e6`.
The slice-five credited receipt, 106-entry manifest and closing review are
`ee24853860afca5d1a6e7a11004097d8d53b294d42c9a6ae091374b7b294213d`,
`27eaed6ad4c43e22070a5378d96380ab1fe43ca726e83db057890c17588afff3`,
and `5ba74de399630ad9a48dd202813e5c7063c9d8876dd9fcebd10d1cab97396d7f`.
S5-L1–L7 carry forward with their original claim limits. Under 162 A and
184 A, the disposable writers use the existing v21 catalog and its original
capture format. Incompatible replacement format stays with #816.

## Exact selectors

`SUBATHON_BUILD_MILESTONE_006_SLICE3=ON` adds the opt-in
`milestone-006-slice6-hold.suite`. The [selector ledger](expected-selectors.tsv)
predicts three first stops and the ordinary Qt `initTestCase` and
`cleanupTestCase` passes. Protected setup and storage checks are part of each
row. There are zero separately registered passing controls. Compilation,
MOC and link inspection supply no runtime results.

| Selector | Before the first stop | Downstream assertions |
| --- | --- | --- |
| `s1MixedEditHoldPublishesOnlyExactMedia` | A normal #751 Review provides the preceding application. A normal target edit captures six immutable slots, then v21 releases the exact group. Audio 1, chat 3 and delivery 4 reach r2. Chat reaches delivered r3. Slot 2 commits the supplied +30,000 ms and its own receipt on the real writer. Default Hold cancels only 5 and 6, adds one exact cause and records one unmarked intent for M, identified by its fake owning adapter. The exact original slot 4 r2 can still finish indeterminate at r3. It leaves the selected originals, timer and immutable Stop result intact. First stop is the playback owner handoff. | The fake sees the committed one-way marker before one call for exact M. The call alone proves no confirmed stop. Later exact adapter evidence commits and reads back separately. The original Stop decision, cause, capture, bonus and two delivery outcomes still match their independent oracles. |
| `t1T2WarmHoldAndMediaReplay:resume-during-gap` | One live timeline starts A at 0 with real release and a real acknowledged audio claim. At request sample 1,800 A still presents. Lost Stop COMMIT acknowledgement at 2,000 publishes no result or call. Reopened writer normally reads the same Stop, one Hold and one unmarked intent. The real writer refuses B's release while held. First stop is timeline adoption of the 2,600 readback. | The 2,600 readback ends A and starts its 1,000 ms gap. A separate playback owner turn sends one marked call for M. The 3,100 exact retry and a repeat playback turn preserve the same rows and call count. Normal Resume at 3,200 clears Hold but B is still absent at 3,599. B first releases at 3,600, with elapsed zero. Exact replay 100 ms later leaves B visible and the original Stop, resolved Hold and marker intact. |
| `t1T2WarmHoldAndMediaReplay:resume-after-gap` | Same independent fixture and first stop. | The 3,600 gap boundary passes under Hold. B is still absent at 3,799 and first releases at 3,800 when the exact Resume clears Hold. It may start on the eligible Resume or on the following same-sample advance. It releases once with elapsed zero, and reaches elapsed 100 after a later exact replay and same-sample observation. |

The early-Resume row would catch a gap backdated to 2,000. Both warm rows
would catch a gap reset by the 3,100 replay. The late-Resume row also
requires Hold to keep B gated after 3,600. The timeline is retained across
the writer reopen. The sample for first acceptance is supplied on the
normally acknowledged readback turn. The fixture does not feed a provisional
decision to the owner. Slice three retains the separate uncertainty-fence
and natural-boundary timing rows. Slice two retains changed-choice conflict,
revoked exact retry and cold Hold/marked and unmarked restoration rows.

## Owner mapping and remaining evidence

- `handoff_playback` follows slices two and three. It forwards once to the
  playback owner's `process_milestone_playback_turn` with its handoff turn
  selector and the existing writer, exact handoff, `PlaybackAuthority` and
  `PlaybackStopAdapter`. The current fake owns only the synthetic capability
  and exact adapter evidence. The real writer owns the marker and confirmation.
- `adopt_stop_completion` follows slice three's
  `adopt_milestone_stop_completion` with the same source enum projection,
  writer status, and optional published decision. The test drives a separate
  playback turn against the same durable intent. The exact real Resume result
  goes through slice one's existing `CommittedResume` update seam. Those
  downstream calls and assertions have no new first-stop credit here.
  Stage 2 review must verify that each stub becomes one owner call with only
  mechanical argument projection. The fixture supplies no timeline or
  playback implementation.
- The two named composition gaps now have authored selectors. Reachability,
  the three stop predictions and all downstream outcomes are unobserved
  here. An independently admitted diagnostic precedes test review, then a
  fresh admission and reviewed red run precede implementation. Only the
  designated first stops qualify for red credit.
- The existing replacement pager's no-partial-page obligation still needs
  source review at implementation. The eight-byte control exercises only
  whole-work preflight. A genuine first-versus-second-candidate budget row
  remains blocked until the owning pager projection can be priced. P1
  unmarked obsolete retirement remains prospective for the existing
  storage path, which only records outcome on a marked intent.
- The earlier replacement, timing, recovery, disclosure and authority owner
  assertions remain downstream of their credited stops. In particular
  S5-L1's known actor mismatch classification, real session revocation,
  route request shape and the absent facts split retain their prior
  limits. This slice adds no live authority, renderer or provider evidence.
  #294 owns host routing and acknowledgement wiring.
- Detailed parity limits remain. R1 cached effective-body selection and
  R2 dispatch publication use an edit-origin owner fixture. The protected
  application selection and claim controls do not observe application owner
  publication. The combined edit S1 row uses exact matching finish with
  indeterminate evidence. The separate original-job cold discovery and
  recovery witness remains the application row in slice three.
