# Scene bindings, conditions and timelines — logical contract draft v1

[CONTRACT-010 #162](https://github.com/Kernol12/subathon-bot-ai/issues/162) · Tier B ·
[specification v0.45](../specification.md) §§3–4, 7–8, 10–13 · protected base
`89054a33dc9b184135fe173d45eda5cf006c0de5`. This contract supplies the typed
binding, formatting, layout, condition and animation references of the
[scene document](scene-document-v1.md). The [asset contract](scene-assets-packages-v1.md)
owns media and fonts. CONTRACT-012 owns encoding and validation, CONTRACT-008
owns integration, and downstream owners enforce access, synchronization and
delivery. RENDER-004 owns renderer execution and platform scheduling.

## 1. Owning observations

The revisions below identify protected code or reviewed baseline declarations.
Scene selection consumes only already-selected, authorized inputs from those
boundaries. A scope, capability, reference, selected fact or render result grants
no access, mutation, action release, delivery or clock authority.
Operands used together come from one admitted view of the selected scene and
domain revisions. Missing input cannot be filled from a different snapshot.

| Key | Protected revision and interface |
| --- | --- |
| D | `8fc9edcf7ac58e61bfd5ed09850e44c97984f77a` scene document, `ebb4368ab5af3b0c39314607896a6213fc061bb5` assets and `806f69f62a019417df80c8fbadb888074a5aa371` CORE-001 `exact_decimal.hpp`. |
| T | `0700d5dc4a1c159ed0b5210fb8051895c5b51b17` CORE-002 `TimerState/TimerClockStep/TimerAdvance`, `afcf5283bff3ec65f83f5ffdd99173d30f5e90e4` CORE-003 `PauseState`, `1259cfb3f34d7da0cddd6a7bcf4be50ebde8fd0a` CORE-005 `TimeBalance`, and `6e1ed7189dc9eec741e6c9928ec5f48188aaade6` CORE-006 `downtime_recovery.hpp`. `abb9595c66d5e07f9ed1999227e0b21432508d42` CORE-007 is a Finished-command baseline. |
| S | `7900a614214f72da85c31d44643cf32a078729cc` receipt facts baseline, `5201edda9de5fa7f438e85efa06d6a628df9101e` ACCT-002 `ContributionState`, `685b17f22998419f1691658a829b468eea05e770` ACCESS-002 `effective_permissions.hpp`. Source privacy is specified by VIEW-03. |
| M | `4e8524139918954f2b0ec791438913305a495e08` MILESTONE-001 `MilestoneProgressionData` and `MilestoneCompletion`. `9219560caa6e2525d89cabde88025dfa58bc1b70` MILESTONE-009 `SqliteMilestoneGroupStore` retains ordinary committed captures. `f63a418b9bb9248a9cc9b0e438139ef2c4b47466` ACCT-007 integrates v18 `sqlite_milestone_target_edits.hpp` with immutable definition/prefix history and edit occurrences. Read only committed observations. |
| P | `7ddcfc8266009e4ff0782daaf5ab7db7a00f07ee` #708 is the baseline. `c92e042ac0f095438b3d52e79a07ffc20c9e8020` #712 implements `MilestoneTimeline` r2. `400a5c5105892ff44a66b75bf8476cfcd4243188` #721 adds keyed disposition and committed holds/skip, with v18 edit integration at M. |
| V | `3ae76b958cc9a663c69c72e45a71f7bf7a4aeca0` #728 `ModifierInstance` and `SqliteModifierInstanceRevision`, `d73e25e5a4709c79ffc9f34aacd081512cbecfc5` #759 `ModifierClockValue` and cut/recovery projectors. `e994c81bb8521b8bd81d4b8d613a0ac161b5211c` #789 freezes the reviewed `ModifierBindingProjection` seam and modeled exact-version name in Stage 1, with production names gated by [#777](https://github.com/Kernol12/subathon-bot-ai/issues/777). |
| E | `514171962e94d56a630f8672e24c4f9ff0b2b816` RULES-001 `RuleAnnouncement` declares the exact presentation version of a typed action. Its declaration alone supplies neither a running presentation observation nor delivery evidence. `RuleCountField` distinguishes unsigned gift count and Bits. |

The protected code has no owner for the authoritative current event announcement,
its sequence, elapsed position or reconnect observation. [#792](https://github.com/Kernol12/subathon-bot-ai/issues/792)
tracks that owning-domain gap. This contract consumes the observation once admitted.

## 2. Values, destinations and formatting

A selected value is `Known(value)`, `Absent` (known inapplicable or no value),
`Unknown` (insufficient evidence), or `Undisclosed` (withheld). Explicit source
null is separate from absence and has no typed value. Carry these distinctions
only within the admitted projection, respecting what can be disclosed about even
the existence of a value or row. Zero, empty text, empty list and false can each
be known. In a visible destination, an unavailable value uses its authored inert
placeholder or omits that slot if none is authored. Whole-row hiding upstream
removes the row without an indexed hole, count, identity or existence signal.

| Logical field(s) | Value, availability and owner |
| --- | --- |
| `timer.lifecycle`, `timer.phase`, `timer.awaitingConfirmation` | T lifecycle `Ready/Active/Finished`, phase `Main/Grace` during Active, and Active confirmation flag. Grace means Active and Grace, including when paused. A known inactive lifecycle makes the phase/confirmation slots inapplicable. |
| `timer.main`, `timer.grace` | T selected exact nonnegative signed64 milliseconds from the separate Main and Grace slots. Retained Grace during Main is a budget, presented as such if selected. It is distinct from running Grace. No substitution for absent or unavailable observations. |
| `pause.active`, `pause.reason(reason)`, `pause.streamUnknownNotice` | T Booleans from the effective reason set and notice. Select the six `PauseReason` members by their actual enum. False membership requires complete evidence for the selected reason. A pending delay contributes no active reason. |
| `modifier.mode`, `modifier.active`, `modifier.remaining` | V selected instance and qualified observation. Mode is `ActiveSubathon/RealElapsed`. Remaining is nonnegative signed64 ms, active is Boolean. Expiry is 0/false and a paused live instance can be positive/true. Replaced is inactive when definitively established for that instance, with no current countdown. Missing clock qualification yields unknown remaining and, for a possibly live instance, unknown active. Saved revision remaining and lifetime at read time cannot establish past or current observed remaining. |
| `modifier.factor`, `modifier.name` | V selected captured definition revision and exact target ordinal supply a finite nonnegative dimensionless factor and original spelling. UTF-8 name requires independently supplied exact configuration, definition and revision match under [133A](https://github.com/Kernol12/subathon-bot-ai/issues/223#issuecomment-7067059849). A missing or mismatched name is unavailable even if the factor is known. |
| `track.position`, `track.score` | M `empty/before_first/between_targets/all_completed` and actual signed64 micro-points. |
| `track.previousTarget`, `track.nextTarget`, `target.current` | M exact current signed64 micro-points. Previous/next are optional projection slots. The last slot selects an already-admitted target by typed milestone identity. |
| `achievement.target`, `achievement.score` | M exact signed64 micro-points of the selected immutable completion, qualified by its capture. They are absent if a complete authorized selection has no such capture. |
| `track.deficit`, `target.deficit`, `track.remaining`, `track.progress` | M unsigned64 micro-point differences (track deficit, selected completed target deficit, remaining to next) and exact rational fraction in `[0,1)`. Optional result slots stay absent. |
| `support.rows` | S ordered, bounded, already-selected recent support rows. Kind is the closed presentation classification Subscription/Gift/Bits/Donation, only for a source fact known to belong to it. Name is literal UTF-8 from a source-permitted display name or authorized anonymization. Original money is exact source decimal plus currency. Gift count and Bits quantity are separately typed unsigned64 counts (E) only on their matching row kind and only when supplied. Apply source privacy and configured hiding to every applicable slot. A source-anonymous name or hidden amount cannot be recovered through another field, valuation, format or comparison. |
| `announcement.current` | P optional current milestone presentation with selected milestone identity and elapsed/duration in ms. Event announcements use the exact E presentation version only when an authorized current event presentation is supplied. Presentation content is the bound authored literal for that selected identity/version. A job, trigger, queued group or old snapshot alone cannot select a current announcement. |
| Authored text and layout | D literal labels, including milestone descriptions and announcement text, can be bound explicitly to the selected typed milestone identity or presentation version. A missing label has no ID-derived fallback. Authored strings remain literal. |

The families below admit only the listed content destinations, in addition to
common visibility, animation and the optional common opacity attachment.

| Scene family | Binding and layout |
| --- | --- |
| Timer | One of Main, Grace, or selected modifier remaining, with duration formatting. |
| Text | Literal, or a typed scalar from the registry with its permitted formatter. No row traversal or arbitrary template expansion. |
| Progress bar | Exact track progress supplies fill geometry only when present. Layout chooses interval, no-origin, empty, or final completion, with optional deficit, remaining and target text slots. When the admitted interval fraction is unknown or undisclosed, use the unavailable fill state. |
| Image | Intrinsic 011 asset/fit. The common opacity attachment, visibility and animation are its dynamic destinations. |
| Shape/group | Intrinsic shape or group structure from 009. Common opacity affects the group's composed subtree as specified by 009. |
| Recent support | Already-selected rows in supplied order. Row layout selects kind, display name, original money, gift count and Bits quantity slots. No receipt indexing, local reclassification or inferred quantity. |
| Announcement area | Selected current announcement and authored content layout. Milestone timing follows §5, even when no local label was authored. |

The common opacity attachment selects one disclosed exact progress fraction and
uses it as the node's absolute base opacity. A present fraction is below 1, so
the attachment itself never produces full opacity. The resulting ratio is a
runtime value. It is never stored as a 009 FiniteDecimal. On an unavailable
fraction use the node's static opacity, which can be 1.
Other slots have no effect on frame geometry, style, asset identity or group
ownership. Visibility remains a separate static or conditional choice, including
for media presentation. There are no arbitrary paths, expressions, credentials,
actor IDs, provider payloads, private data or permission-shaped operands.

Permitted text formats are literal text for Text, catalog labels for enum/Boolean,
`H:MM:SS` or `H:MM:SS.mmm` for durations (whole seconds truncate, hours do not
wrap), fixed points with 0–6 decimals for micro-points, percent with 0–6 decimals
for progress, and exact decimal for a factor or original money. Points divide
micro-points by 1,000,000; percent multiplies the exact ratio by 100. Fixed display
precision rounds half-to-even; formatting never feeds a condition or a ledger.
Exact factor may include an authored multiplier marker. Exact money always
includes its selected currency and preserves the source precision, with no
conversion or valuation substitute. Counts use exact whole units of gifts or Bits
with the corresponding catalog label. Scene locale supplies decimal separator and
built-in labels; authored text and exact retained spelling remain intact. Render
strings as text, including strings that resemble markup, paths or scripts.

## 3. Typed conditions (owner decision 126A)

A visibility expression is a finite tree: `all` and `any` take one or more
conditions, `not` takes one, and a comparison takes a registered operand and
one typed literal. Use equality or inequality for lifecycle, phase, the
confirmation flag, pause Booleans, modifier mode/active and milestone position.
Also allow ordered
comparisons for exact durations, factor, point values, and progress fraction.
Ordered operators are `<`, `≤`, `>`, and `≥`, alongside equality and inequality.
The progress literal is an exact rational in `[0,1]`; its endpoints permit
ordinary bounded comparisons even though a present core interval is below 1.
Units must agree with the field, and literals obey its domain and range. A
closed interval test uses lower and upper comparisons under `all`, choosing
inclusive or exclusive endpoints explicitly. Support rows, names, money and
announcement content are display-only. No presence tests or arbitrary fields.

Validate every node even in a branch whose result is predetermined. Malformed
expressions, invalid members/operators/types and unresolved binding definitions
are invalid logical content. A valid operand whose data is absent, unknown,
undisclosed or explicit null yields `U` for every comparison, including
inequality. For known values, compare exactly before formatting.

- `not(T)=F`, `not(F)=T`, `not(U)=U`.
- `all` is F if any child is F, T if every child is T, and U otherwise.
- `any` is T if any child is T, F if every child is F, and U otherwise.
- A conditional element or presentation trigger activates only on T. F and U
  both suppress activation while retaining distinct evaluation results.

## 4. Actual milestones and captures

Render the owner's exact projection. Given actual score S, current previous
target P and current next target N, deficit is `max(P−S,0)` if P exists (track
deficit is otherwise known zero). Each completed deficit uses that milestone's
own current target. Remaining is `max(N−S,0)` when N exists. An available
interval has origin P, span `N−P` and numerator `max(S−P,0)`, with S below N.
Use the exact ratio, widening before subtraction, including distances through
`2^64−1`. Keep an unknown or undisclosed fraction unavailable in the admitted
view. Before the first completion there is no P or numeric fraction.
At final completion there is no next or interval, even with positive deficit.
The authored final layout can show completion without inventing numeric progress.

Crossings record an ordered prefix of achievements immediately on application.
The owner captures the then-current definition, threshold and actual score for
each new completion. A later edit or ordinary reversal leaves those captured
values intact. Progress and deficit use current thresholds and actual score.
Recovery of a deficit supplies no repeat award. Ordinary group records capture
the exact achievement and original action membership. v18 keeps edit-originated
captures with their own immutable occurrence/job membership and definition and
prefix history. A proposed edit, writer-session result, pending capture or
action-release record cannot substitute for a committed achievement projection.

## 5. Presentation and clock observations

The milestone queue consumes committed groups in threshold order. The display
uses the current authorized `ActiveMilestonePresentation` at a server sample.
Defaults are 10,000 ms presentation and a 1,000 ms following gap. Overrides are
independent per field, duration strictly positive and gap nonnegative, including
zero. For a group started at sample S, it is current on `[S,S+duration)`.
The gap has no active group. The next permitted head starts at elapsed zero on
the actual eligible owner turn on or after the gap boundary. Check differences
separately so adding duration and gap does not overflow. Samples are
nondecreasing within one server lifetime.

Queue Pause/Resume gates new starts. The active window and its gap continue.
By default countdown pause has no queue effect. When linked it independently
gates new starts; both gates must clear. Held or unavailable keyed head
disposition blocks the suffix. A committed skip consumes no window or gap.
An uncertain release blocks further starts in that server lifetime. Release and
durable recovery are server responsibilities. Restoring committed pending groups
starts the first eligible head at zero on its first owner turn, with no invented
saved active animation phase. Overlay disconnect cannot gate advancement or
release. Reconnect observes the current window at its current elapsed position,
or no active group, without replay. A progress animation may follow the ordered
presentation steps as visual transitions between authored visual positions.
Conditions and numeric labels keep actual score/progress. Presentation provides neither another award nor an
intermediate fabricated score.

Use distinct observations for countdown/phase, the pause and Recovery Review
basis, ActiveSubathon modifier elapsed, RealElapsed modifier elapsed and server
presentation. Main/Grace progress only on a qualified unpaused Active cut,
including progression through phase boundaries. A pending signal delay or
unknown stream notice is no pause by itself. Clearing Recovery Review preserves
other reasons. A hard end can finish even during grace or pause. Positive actual
credit can return Grace or confirmation wait to Main as the timer owner allows.
Debt, unapplied time and proposed credit are separate from timer remaining.

ActiveSubathon modifiers debit the timer's qualified progressed milliseconds and
freeze while paused/finished, following the selected countdown downtime policy.
RealElapsed modifiers debit qualified elapsed independently, including during
pause and review wait. Across an unqualified gap a possibly live RealElapsed
modifier has unknown remaining and active status. Timer Freeze/resolution alone
cannot restore that missing clock qualification. An owner-marked inactive or
expired instance can be known inactive. Hiding an element changes presentation
only. UTC instants, frame counts and receipt timestamps supply no elapsed proof.

## 6. Animation observations and previews

Built-in tracks set opacity (fade), parent-space x/y (move), local x/y scale
(scale), or clockwise degrees (rotate), using 009 units and transform order.
Each track declares explicit endpoints, easing, nonnegative signed64 start
offset and positive signed64 duration, both in milliseconds. The offset is
relative to the start of its declared context. All tracks for one node/property
use the same context. Validate the half-open intervals `[start,start+duration)`
from authored offsets. The exclusive end must fit signed64. Overlap for the
same node/property is invalid. Adjacent intervals are valid.

Use the effective base before the first track. During a track its absolute
value overrides the base. Between tracks and after the last track, hold the
latest endpoint while the context remains selected. A later track's start value
takes effect at its exact start offset. Evaluate a server-bound track only in
the current server window at its owner-supplied elapsed position. Other tracks
use qualified elapsed since scene start. Without qualified elapsed use the
effective base. Countdown pause alone freezes neither track clock. Rendering
or skipping a frame changes no run or observation. Replacement or end of the
selected context removes its override. No angle wrapping or shortest-arc
inference.

For a track that has started, let E be context elapsed minus its start offset. Use `t=min(E/duration,1)`. For linear, ease-in,
ease-out and ease-in-out use respectively `t`, `t²`, `1−(1−t)²`, and `2t²`
for t < 1/2 or `1−2(1−t)²` otherwise. Interpolate each component as
`from+(to−from)*easing(t)`. Endpoints satisfy 009 domains. Reference scenes
assert bound values, visibility, fill, transform and opacity at declared
observation times including missed frames, across both renderers. Render
tolerances and rasterization remain downstream. Preview uses the same logical
selections and explicit synthetic observations on a candidate revision without
affecting live state, queue, releases or clocks.

## 7. Independent examples

These are literal logical oracles, derived from the cited owners and the rules
above. “Invalid” means reject the content, distinct from valid unavailable data.

| Basis | Admitted example and expected result | Invalid or unavailable control |
| --- | --- | --- |
| T | Active/Grace, Manual present, 1251 ms remaining → paused Grace; `0:00:01.251`. Ready/Finished remain distinct from Active at 0 ms. On a known Active confirmation wait, `timer.awaitingConfirmation = true` is T. | `phase = Ready`, `pause.reason(Grace)`, `main < -1 ms` and a UTC operand for a duration are invalid. No phase for known Finished. |
| T | Offline delay set to 30,000 ms, continuous confirmed offline with no other pause → unpaused at 29,999, paused at 30,000. Unknown cancels a pending offline delay, not an active reason. | Unknown notice alone cannot prove confirmed offline. Clearing Review in {Review, Manual} leaves Manual. |
| T/D | Signed64 max ms is valid and displays as `2562047788015:12:55.807`. At an enabled hard end's exact UTC instant, paused Active/Grace becomes Finished. | Milliseconds one beyond signed64 max are invalid. An elapsed sample of the same integer is no evidence of reaching a UTC deadline. |
| V/T | Starting 1000 ms per mode, paused qualified cut of 999 ms → ActiveSubathon 1000 and RealElapsed 1; next paused 1 ms → 1000/0, active true/false. | Comparing mode with a phase or active with numeric 1 is invalid. Across an unqualified gap `not(active = true)` is U. |
| V | Captured factor spelling `2.00` and matching name “Double time”, qualified remaining 180000 ms → displays `2.00` and compares as exact 2. Remaining/active is positive/true even while paused and hidden. Exact factors 0 and 0.5 are valid. `factor >= 2` is T. | Negative or infinite factor invalid. Wrong revision name unavailable. A saved 180000 on a replaced revision is no current remaining value. |
| M | Captured A=1000, score 1100, B=2000. Reversal of 300 → score 800, A still complete, 0%, deficit 200, remaining 1200. Edit A to 900 → capture still 1000/1100, current deficit 100, remaining 1200. | Captured target cannot replace current A. Money, formatted text or ms as a micro-point operand is invalid. |
| M | First A=1000, score 800 → before_first, remaining 200, no origin/fraction. Last completed A=1000, score 800, no next → final layout plus optional deficit 200. | `progress = 0` before first yields U. A synthetic zero origin or forced 100% final ratio is invalid. |
| M/D | Prior 0 and next 3 µpt with score 1 µpt → 1/3; `all(progress >= 1/3, progress < 1)` is T, displayed 33.33% at precision 2. Prior INT64_MIN, next INT64_MAX and score 0 → span 18446744073709551615 and earned 9223372036854775808 µpt. | `progress > 101/100`, score 9223372036854775808 µpt and negative unsigned deficit literals are invalid. Exact threshold equality completes next on a normal progression cut. |
| M/D | Before first with score INT64_MIN and next INT64_MAX, remaining is 18446744073709551615 µpt. At precision 2, 125000 µpt displays 0.12 points, but is greater than 120000 µpt. | A remaining literal one beyond unsigned64 max is invalid. Display rounding cannot make the comparison equal. |
| S/E/D | Authorized rows: anonymous Gift with count 5, Bits with quantity 0 and no money, Donation with EUR `9007199254740993.25`, and Subscription with withheld display name. Preserve row order, exact counts and donation, and anonymization. EUR `0.00` displays as known zero. | Gift count on Bits, negative counts and comparison of row details are invalid. No EUR zero for an absent, null, withheld or unknown amount. A valuation cannot replace hidden original money. |
| D/126A | With T, F, U as leaves, `all(T,U)=U`, `all(F,U)=F`, `any(F,U)=U`, `any(T,U)=T`, `not(U)=U`. Only the fourth activates. | Empty `all/any`, two operands to `not`, and `any(T, lifecycle > Active)` are invalid. An unknown comparing unequal remains U. |
| P/M | One input at t=0 reaches A/B/C with actual score 3500. A active at 9999, no presentation at 10000 or 10999, with an owner turn at 11000, B starts then. Reconnect at 11500 sees B at 500 ms. All three captures remain immediate with score 3500. | No A replay, no 1000/2000 intermediate score. With an owner turn starting C at 22000, reconnect at 22100 sees C at 100 ms. |
| P | Pause queue at 5000 of A's default window; A ends at 10000 and its gap ends at 11000. A Resume owner turn at 20000 starts B at 0 unless linked countdown pause or a held head remains. Override duration 1 ms and gap 0 enables next start at the 1 ms turn. | Zero/negative duration, negative gap and regressing sample invalid. A skip consumes zero timing. A start at a late turn uses that turn, with a full duration. |
| E/P/D | A selected current event at its presentation version with literal “Goal <3” displays that text. A selected milestone can use its own authored multiline description. | A queued rule job or no current supplied view gives no current event announcement. Mismatched label version supplies no text. Asset-as-text or markup interpretation is invalid. |
| D/P | 1000 ms linear tracks from opacity 0→1, move (0,10)→(100,30), scale (1,1)→(2,3), rotate 350→10. At 250 ms: 0.25, (25,15), (1.25,1.5), 265°. At 1000 ms the exact endpoint holds in context. With ease-in at 500 ms the fraction is 1/4; ease-out is 3/4. | Zero duration, opacity 1.1, and UTC as elapsed are invalid. Skipping from 0 to 750 ms yields the 750 ms values on one frame. |
| D/P | Same-node move tracks at start/duration (0,500) and (500,250) share one context. At context elapsed 500 the second start value applies. A later gap holds its endpoint. | Start 499 for the second track overlaps and is invalid. Start INT64_MAX with duration 1 is invalid. Tracks for the same property cannot use different contexts. |
| D/T/V/M/S | Preview on synthetic observations includes Ready, Main, paused Grace, confirmation wait, Finished, known and unresolved modifiers, deficit 200, the large donation above and literal name/description with 2048 Unicode scalars each. A shape, image or group can use a selected 1/3 opacity without changing content or child order. | No preview-generated authority, rounding of ledger facts or shortening of names in the bound value. A number cannot select a new asset, and static opacity is used when the selected fraction is absent. |
