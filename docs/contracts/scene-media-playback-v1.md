# Scene media playback — logical contract v1

[Scene media #804](https://github.com/Kernol12/subathon-bot-ai/issues/804) · Tier B ·
owner decisions 168–171 · [specification v0.45](../specification.md) §11,
MEDIA-01 and MEDIA-02 · protected base `4b522332c6ab0099e0e3c488b162240858306fa0`.

This contract defines admission, sound, replacement and resume for scene audio
and video shared by the native and browser outputs. [RENDER-004 #453](https://github.com/Kernol12/subathon-bot-ai/issues/453)
owns execution and platform scheduling. Concrete schema fields, wire encoding,
output policy configuration and measured codec support remain downstream work.

## Owning inputs

| Input | Owning boundary |
| --- | --- |
| Authored scene and media attachment | The [scene document](scene-document-v1.md) §§1, 4–5 defines exact scene revisions and typed attachment references. Playback consumes the selected revision, the exact attachment within it and that attachment's authored policy. |
| Media bytes and output compatibility | The [asset contract](scene-assets-packages-v1.md) §§1–2 defines immutable asset identity and separates declaration, inspection and compatibility. An output needs compatible evidence for the exact asset and profile it will use. |
| Current activation, presentation and elapsed position | The [bindings and timelines contract](scene-bindings-conditions-timeline-v1.md) §§1, 5 defines the selected view and clocks. Its milestone owner already supplies a current milestone presentation and its qualified elapsed position. [#792](https://github.com/Kernol12/subathon-bot-ai/issues/792) owns the corresponding event announcement selection and sequencing. Event playback that needs that observation remains unavailable until the owner can supply it. |
| Output sound permission and local mute | The admitted output supplies its sound permission for the selected scene and its local mute state. Silence is the effective default when sound permission is missing or denied. |

Use one admitted view of scene, asset, output and presentation. A reference
resolves according to its own kind and exact revision. A path, filename,
display label, equal media bytes or cached payload cannot establish the
selected attachment or instance.

The logical instance identity binds the admitted scene revision and activation,
the exact attachment and immutable asset revision, and the start cause. An
autoplay cause is the scene activation occurrence. A trigger cause is the exact
authorized occurrence selected by the owning domain, including the presentation
identity and version where the trigger belongs to a presentation. A new
activation or occurrence is distinct even when it uses the same asset.
Reconnect and a different output view can observe the same instance. An output
checks its own compatibility and sound policy.

## Admission and playback

1. **Explicit start.** Support an authored trigger matched to an authorized,
   owner-observed occurrence. Support authored autoplay when the scene becomes
   active. Autoplay defaults off. Loading, showing or reconnecting to a scene
   on its own does not start playback. A declaration, compatible asset, queued
   job or known attachment does not supply a cause or permission.
2. **Exact match.** Admit a start only for the selected active scene, the
   matching authored attachment and cause, and an output with compatible
   evidence. If currentness, authorization or identity has not been established,
   the start is unavailable. Known output incompatibility is unsupported.
   An undisclosed input stays within the owning projection's disclosure rules.
3. **Retrigger.** A repeated delivery of the same authorized occurrence for
   that attachment has no effect, including after replacement or end. A
   distinct authorized occurrence selected for the same attachment replaces
   its current playback from the beginning. The renderer follows the owner's
   current selection or ordering and cannot revive old playback from message
   arrival order. #792 owns announcement sequence and expiry.
4. **Background loop.** The authored background loop flag defaults off.
   An explicit loop repeats playback of the admitted background attachment
   within the same instance and active window. File loop metadata alone does
   not enable this policy. The loop does not create a fresh trigger or extend
   the owner window.
5. **Sound.** Audible playback requires explicit scene sound enablement,
   output sound permission and no local mute. An output that permits silence
   only can run an otherwise admitted attachment muted. Mute and unmute
   change audibility without stopping, restarting or shifting the playback
   clock. Permission to play silently does not grant permission for sound.

## Currentness and resume

The elapsed position comes from the qualified clock for the exact instance
within the selected view. A scene autoplay instance follows the scene
activation. A presentation-bound trigger follows the selected owner's active
window. A missed frame, local mute, asset load time or disconnected renderer
does not extend that window. A clip that ends without an admitted loop becomes
terminal. A replaced or stopped instance remains terminal, and deactivation
ends the activation's autoplay instance.

Resume only the same still-active, compatible instance at its current qualified
elapsed position. For a running loop, the total elapsed position remains the
owner-clock position. The renderer uses the admitted loop extent to derive
phase. For an elapsed position of 2,750 ms and a 1,000 ms extent, the phase
is 750 ms. An unqualified elapsed position yields no resume or synthetic
start at zero. Ended, stopped, replaced, unknown, undisclosed or unsupported
instances yield no resume. There is no backlog replay on reconnect.

For a current milestone presentation, use the qualified age and window from
the milestone owner. Its following gap has no active presentation. Exact Hold,
Stop and release rules stay with the milestone timeline owner. Event
presentation uses the same selected-input boundary when #792 supplies it.

## Independent examples for downstream tests

All times are milliseconds. Each nonnegative elapsed position is a qualified
observation for the identified instance.

| Case | Input | Expected observation |
| --- | --- | --- |
| M01 | Scene S revision R, attachment A, compatible output. Load and display S with default autoplay and no trigger. | No start. Compatibility and selection alone do not admit playback. |
| M02 | Autoplay enabled for A. S becomes active as activation P at t=1000. The same nonterminal instance is current at t=1250. | Start A in activation P at elapsed 0. Reconnect at t=1250 resumes that instance at 250. Repeated observation of P is not a new start. |
| M03 | A is explicitly triggered by exact occurrence E1 at t=3000. E1 is delivered again at t=3150. Owner selects authorized E2 for A at t=3200, then E1 is delivered again at t=3300. | E1 starts at 0 and its first duplicate leaves elapsed 150. E2 starts at 0 and replaces E1. The stale E1 delivery leaves E2 current at elapsed 100. |
| M04 | Scene sound enabled and A otherwise eligible. Output O permits sound but locally mutes at elapsed 300 and unmutes at 650. Output Q permits silent playback only. | O becomes silent from 300 and resumes sound at elapsed 650. Q stays silent. Both retain their qualified playback position. |
| M05 | A contains declared loop metadata and the authored loop flag is omitted. The 1,000 ms clip is observed at its end. Compare an authored background loop with the same compatible extent in a still-active window. | Without authored loop the clip ends and cannot resume. With authored loop at total elapsed 2,750, resume the same instance at phase 750. |
| M06 | The exact attachment and a supported profile are available, but only an event job and a declaration exist. A second observation has an authorized cause and known incompatible output. | The first observation has no proved start cause. The second is unsupported for that output. Neither plays. |
| M07 | A reconnect supplies an asset matching an old stopped instance, or supplies a current identity without qualified elapsed. | No resume in either observation. The old instance is terminal and the current position remains unavailable, respectively. |
| M08 | Milestone window starts t=2000, lasts 1,000 ms and is followed by a 200 ms gap. Current media is selected through the presentation. Reconnect at t=2999 and then at t=3000. | Resume the window-bound active instance at 999 on the first observation. No milestone resume at the gap boundary t=3000. The next window needs its own owner selection. |
| M09 | On autoplay's scene deactivation, the old asset stays cached. The same revision activates again with a distinct activation and unchanged attachment bytes. | The old instance cannot resume. The new authorized activation admits its own autoplay instance at 0. |
