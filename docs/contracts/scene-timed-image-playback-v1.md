# Timed playback for Image nodes · #825

Tier B contract draft. Applies to desktop and browser consumers. Covers timed
visuals in an Image node. Scene-level MediaAttachmentRef playback stays under
#804. #453 owns renderer execution and scheduling. This contract selects no
concrete schema, decoder, codec expansion or platform support.

## Merged inputs

- SCENE-009, scene-document-v1 §4 Image and §2 common fields. Resolve the exact
  VisualAssetRef and retain authored fit/alignment, visibility, opacity and
  animation. The Image visual and the common media attachment are independent.
- SCENE-011, scene-assets-packages-v1 §§2.2, 2.4, 5. Use exact admitted asset
  revision, payload digest, inspected media profile and package closure. GIF
  delays are 2–65535 centiseconds in the admitted profile. Preserve GIF frame
  order, transparency and disposal. Video has constant presentation cadence at
  exact rational rate, starting at zero, and duration frames/rate. Use
  presentation order. Container and file extension establish no capability.
- SCENE-012, scene-schema-validation-v1 Image row and media prerequisites.
  Validation and bounded asset evidence precede use. Renderer admission for
  the actual profile and platform remains required.
- SCENE-010, scene-bindings-conditions-timeline-v1 selected observations and
  presentation elapsed. Use qualified owner elapsed for the exact context.
  UTC, frame count and receipt time provide no elapsed proof. Visibility and
  authored animation continue to follow their common paths.
- Specification §Scene and overlay designer, release media targets. Browser
  and desktop each require profile validation. #804 provides the sibling
  playback rules, including its defaults and exact-instance replay rules.
  Its direct scope excludes this Image visual.

## Approved readings for the Image-specific boundary

- **825.2 Start and defaults.** Apply #804's authored start model to the exact Image
   node (lines 30–60). Autoplay and loop default off. Autoplay needs an owner-observed activation
   of the scene carrying the node. An explicitly authored trigger needs an
   authorized owner-observed occurrence selected for the node. Loading, visibility,
   decode readiness, a queued action and reconnect never start playback by
   themselves. Repeating the same start identity is idempotent. A distinct
   admitted occurrence replaces only the selected node's instance.
- **825.3 Visibility and replacement.** Specification lines 397 and 421 and SCENE-010 visibility rules keep
   visibility separate from owner time. Visibility gates output, including any
   embedded audio. Hiding leaves logical progress on the owner clock. Showing
   uses the current position if the same instance is still active. It never
   starts, pauses or restarts that instance. Removal or replacement of the
   selected activation, scene revision, node binding or asset ends its
   applicability. A replacement instance needs its own admitted start. An owner
   window that closes fences the old instance and delayed occurrences.
- **825.4 End and loop.** Following #804 lines 62–82 and SCENE-011 lines 146–154,
   authored loop alone permits wrapping. GIF loop metadata is
   inspected source identity and does not start or extend playback. An instance
   without loop is terminal at its duration, with no timed frame or sound
   emitted thereafter. A separately specified editor preview has no authority
   to resume production playback.
- **825.5 Embedded sound.** GIF has no sound in the admitted profile. For video,
   following #804 lines 68–72 and decision 169, sound needs a verified
   admitted audio track, explicit scene sound enablement and
   current output permission. Sound defaults off and local mute suppresses
   it without changing visual position. Unknown output audio capability cannot
   produce sound. An output that
   only admits silent playback may render the visual if it can establish
   audio suppression and admits the complete exact source profile.

## v1.1 corrections (#908)

1. **Rest state (owner decision 825 B, recorded in 825.10).** On each desktop
   and browser output, a visible GIF or video Image with an admitted exact
   asset and profile shows presentation frame zero as a still before start,
   after one-pass completion, or while qualified playback evidence is missing.
   GIF uses the composited canvas of its first frame. This still emits no
   sound and does not start, seek, resume or advance playback. When qualified
   evidence returns for an existing instance, use its current owner position.
   Visibility still gates all output. An unavailable first frame or unproved
   exact profile supplies no Image visual, and an unsupported profile is
   refused under the existing admission rule. The still supplies no decoder,
   poster or substitute asset fallback.
2. **Sound enablement (825.9 and #804 M10).** The scene's explicit sound
   enablement applies to embedded Image video audio. It defaults off, and
   there is no separate Image sound flag. The admitted track, output permission,
   local mute and current timed position still govern sound eligibility.

## Instance and observations

Bind instance identity to installation/scene scope, exact scene content
revision and activation, stable Image node identity within that revision,
exact typed asset revision, digest and inspected profile, plus the selected
activation or trigger occurrence identity. Equal asset bytes on two nodes
do not make their instances interchangeable. Compare all identity components
before using an observation. Exact identity does not itself grant authority.
A presentation-bound trigger also binds the exact selected presentation
identity, version and owner window. A gap, stop or replacement ends that
window even if the scene and asset stay selected and authored looping is on.
A delayed occurrence from a closed window cannot create or revive an
instance. An event-bound start requires the selected #792 observation.
A queued event or missing observation supplies no start.

The renderer consumes only the current admitted instance with a qualified
nonnegative elapsed position attributable to that instance. If asset or
profile evidence is missing, contradictory or unavailable, omit timed output.
For video, the inspected profile must establish no audio track or the
complete admitted audio record. Unknown source track facts suppress the
visual as well as sound. An affirmatively unsupported profile is refused
as unsupported. A decoder default or fallback profile cannot repair either case. A missing, unqualified
or mismatched clock observation omits timed output and sound until a qualified
current observation is available. Never seek to zero on that account. No
implicit substitution by poster, nearest asset revision, stale cached frame,
different node or local playback clock.

Evaluate time exactly against the inspected profile. For admitted elapsed E
and positive duration D, select E for one pass if E < D, otherwise terminate.
With authored loop, select E modulo D. At an exact boundary the following
frame applies, including frame zero at a loop wrap. For GIF use half-open
cumulative delay intervals and the composited canvas of the chosen frame;
each new pass starts from the same logical initial canvas. For video the
zero-based presentation frame is floor(position × rate). No renderer frame
counter qualifies elapsed. The owner-selected instance and current elapsed
also govern late attachment and reconnect. This contract does not specify
decode scheduling or byte-identical renderer output.

For embedded video audio, use the same position as the visual, with the
exact admitted audio extent from SCENE-011 §§2.3–2.4. Samples are eligible
in the half-open audio interval starting at zero. A shorter audio track
leaves the remaining visual tail silent. Wrap applies to the video duration,
so a new pass samples audio from the same wrapped position. Reconnect and
mute changes never start an independent audio timeline.

## Examples

Every row applies independently to desktop and browser with the same synthetic
verified inputs. Audio rows additionally require evidence for the named output.

| Input | Expected logical observation |
| --- | --- |
| Two-frame GIF delays 2 and 3 cs; authored loop off; file says forever | 0 and 19 ms select frame 0; 20 and 49 ms select frame 1; at 50 ms the pass ends and frame 0 is shown as a still. No sound. |
| Same GIF, no source repeat extension, authored loop on | 50 ms selects frame 0 of a fresh pass; 70 ms selects frame 1. |
| Four video frames at exact rate 2 fps, loop off | 499 ms selects frame 0; 500 ms selects frame 1; 1999 ms selects frame 3; at 2000 ms the pass ends and frame 0 is shown as a silent still. |
| Three frames at rate 30000/1001, loop on | At 33 ms select frame 0; 34 ms selects frame 1. Duration is 1001/10000 s; 100 ms selects frame 2 and 101 ms wraps to frame 0. |
| Admitted GIF or video, no admitted start; visibility becomes true | Show frame 0 as a silent still. Explicit autoplay plus owner activation starts once at zero. A duplicate activation leaves the position unchanged. |
| Four-frame video above hidden at 499 ms, visible at 1000 ms | No output while hidden; show frame 2 from the same instance. If first shown again at 2000 ms, show frame 0 as a silent still. |
| Same GIF referenced by Image A and Image B; new admitted trigger for A | A gets a new exact instance at zero. B stays on its prior timeline or remains inactive. Queued-only trigger changes neither. |
| Active video reconnect at qualified 1500 ms | Frame 3. A reconnect without qualified elapsed shows frame 0 as a silent still when admitted. If elapsed becomes qualified at 1500 ms, show frame 3. |
| Replace scene revision, node binding or asset; observation still names old one | No timed output from that observation even if the asset digest is equal. |
| Verified video audio, scene sound enabled and permission present, local mute on | Visual keeps the same frame; silence. Missing permission is also silent. Unsupported video profile refuses the visual. |
| Admitted one-second MD06 video with audio extent 368/375 s, loop on, scene sound enabled and output permission present | At 981 ms audio is eligible. At 982 ms video continues silently. At 1000 ms the frame and audio both return to phase zero. A muted reconnect at 1050 ms selects visual phase 50 ms in silence. |
| Video source track evidence is incomplete; compare complete exact profile with an admitted silent-only output | Incomplete evidence suppresses both visual and audio. The silent-only output with complete evidence may show the visual if it establishes audio suppression. |
| Presentation-bound GIF is looping; owner window ends while the scene stays active | Stop timed output immediately. A late occurrence from the old window and a reconnect during the gap cannot revive it. |
| GIF delay outside 011, or only a declared MP4 container without verified track facts | Refuse timed playback; do not clamp GIF delays or infer a codec. |
| Visible non-playing Image; the exact asset's first frame cannot be decoded or is unavailable | No Image visual or sound. A cached later frame or alternate asset supplies no still. |
| Visible non-playing Image with a known unsupported exact profile, even if a renderer could extract a thumbnail | Refuse the profile. No Image visual or sound. |
| Timed video is admitted with verified audio, default Image settings, output permission and local mute off | Scene sound enabled makes audio eligible during its admitted extent. Omitting scene sound enablement keeps playback silent. |
