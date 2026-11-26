# RENDER-001 shared geometry and timing oracles

Independent fixture derivations against protected main
`1597aaf4dcc3513c78fcf0ae6d4c3fb8289b6b0c`. Source rules are spec v0.45
§11 (UI-04, MEDIA-02), `docs/contracts/scene-document-v1.md` §§3, 5,
and `docs/contracts/scene-bindings-conditions-timeline-v1.md` §6.
These are design-space and output-space assertions for both renderers.
Use exact expected values and distinguish logical outcomes from raster
tolerance. Performance and OBS output require separate evidence.

## Resize and clipping

Use canvas 200 × 100, explicit alignment (1/4, 3/4), enabled canvas clip,
and viewport 150 × 150. All positions are measured from the top left.
Test point A at design (40, 20). A child rectangle spans design x
[-20, 40] and y [10, 30]. Its visible design x range is [0, 40] after
canvas clipping. In crop, viewport clipping further narrows that range.

| Mode | Scale (x, y) | Painted extent | Offset | A output position | Visible rectangle output bounds (x, y) |
| --- | --- | --- | --- | --- | --- |
| Aspect-fit | (3/4, 3/4) | 150 × 75 | (0, 225/4) | (30, 285/4) | ([0, 30], [255/4, 315/4]) |
| Crop | (3/2, 3/2) | 300 × 150 | (-75/2, 0) | (45/2, 30) | ([0, 45/2], [15, 45]) |
| Stretch | (3/4, 3/2) | 150 × 150 | (0, 0) | (30, 30) | ([0, 30], [15, 45]) |

The axis ratios are 150/200 = 3/4 and 150/100 = 3/2. Fit takes their
minimum and leaves 75 units on y, with 75 × 3/4 = 225/4 above the canvas.
Crop takes their maximum and leaves signed slack -150 on x, with
-150 × 1/4 = -75/2. The crop viewport admits design x from 25 to 125,
so the clipped child's visible design x is [25, 40]. Stretch scales
each axis independently. These are continuous bounds and do not select
pixel coverage, color of unused space, or edge rounding.

Resize the same fit scene from 150 × 150 to 300 × 100. Its ratios become
3/2 and 1, so it paints 200 × 100 at offset (25, 0). A reaches (65, 20).
The design point and scene geometry remain the same. Use the default
aspect-fit mode for a new scene where the authoring owner supplies
defaults. A stored fixture declares its mode explicitly.

## Nested geometry

Use parent translation (100, 50) and parent scale (2, 1), with pivot
(0, 0) and rotation 0. Use child translation (10, 20), pivot (2, 1),
scale (2, 1), and rotation 90 degrees clockwise. Local point (3, 1)
becomes (1, 0) after subtracting pivot, then (2, 0) after scaling,
(0, 2) after rotation, and (12, 23) after adding pivot and translation.
The parent maps it to (124, 73). Changing only the parent frame extent
does not move the point. For this control the parent frame does not
clip descendants, and all points of interest lie inside the canvas.
A 90-degree rotation makes the mathematical expected coordinate exact
without setting a raster tolerance.

## Skipped frames and absolute tracks

Use qualified scene elapsed observations in milliseconds. Base move is
(2, 4). Two tracks animate the same node and property in one context.
Track A is linear over [200, 1000) from (16, 8) to (80, 40).
Track B is ease-out over [1000, 1400) from (10, 20) to (26, 52).
The second track's discontinuous start distinguishes interval selection
from holding Track A's endpoint at the shared boundary.

| Elapsed | Expected move | Derivation |
| --- | --- | --- |
| 199 | (2, 4) | Base before first track. |
| 200 | (16, 8) | Track A exact starting value. |
| 750 | (60, 30) | Progress (750 − 200)/800 = 11/16. Add (64, 32) × 11/16 to (16, 8). |
| 1000 | (10, 20) | Track B starts at the exact shared boundary. |
| 1300 | (25, 50) | Track B fraction 3/4, eased to 1 − (1/4)² = 15/16. Add (16, 32) × 15/16 to (10, 20). |
| 1400 and later in this context | (26, 52) | Exact Track B endpoint held. |
| Unqualified elapsed or deselected context | (2, 4) | No qualified override. |

Compare two observation histories for the same elapsed sample at 750:
one begins at 0 and skips straight to 750, the other samples any
positive number of intermediate frames. Both must reach (60, 30)
on that sample. Repeat with a single jump from 750 to 1300, across the
track boundary. Both reach (25, 50) with the same terminal time (1400).
A paused countdown does not freeze a still-qualified scene clock.
A renderer must not derive elapsed from the number of samples or UTC.

## Unsupported requirement

Use a structurally valid scene intended for both outputs. Its trusted
capability evidence allows the fixture's required feature for native and
marks it unsupported for browser. Publication refuses for the scene as
authored, even though native supports the feature. A draft import may
retain bounded original bytes with an issue where that path supports it.
There is no successful compatible publication or partial browser render.
Use an existing contract-owned capability and a synthetic profile pair
when serializing this fixture; the oracle does not assign a new capability
ID or claim either renderer's real platform support.

Source: `scene-document-v1.md` §5 and `scene-schema-validation-v1.md` §2.
