# RENDER-001 fixture derivations

The logical fixtures target the scene binding contract at main 1597aaf4.
They are reference inputs and expectations for both renderer owners, with
no dependency on either renderer implementation. Field names in this
fixture file describe the logical oracle and do not define a new wire
or scene format.

For the interval example the earned score is 130−90=40 and the span is
210−90=120. The exact fraction is 1/3; the English precision-two label is
33.33%. After reversal the completed threshold stays complete. Deficit is
90−72=18, remaining is 210−72=138 and the numerator clamps to zero. The
immutable capture stays 90/103. Before the first milestone there is no
numeric fill. After the last target, the 10-point deficit remains
observable with no remaining-to-next or numeric interval.

For the running example each clock debits 250 ms from 900 ms. In paused
Grace, the ActiveSubathon clock retains 900 ms while the qualified
RealElapsed clock reaches zero. A saved balance does not prove a current
RealElapsed observation. The timer phase is independent of pause state
and becomes inapplicable when the lifecycle is Finished.

Sources: spec v0.45 §§5 and 11; scene-bindings-conditions-timeline-v1
§§3, 5 and 7; scene-formatter-locale-v1 exact duration and percent rules.
Fixture review precedes any reliance by native or browser baselines.
These examples establish no renderer pass or OBS compatibility.

## Consume the shared fixtures

Run `python3 -I -B tests/fixtures/render/v1/check_references.py` from
the repository. To export concrete scene and bindings documents for each
case, supply `--output` with an existing empty directory. Each output
contains the versioned document and bindings, an observation where
applicable, and the node-level oracle. The same authored input serves
both renderers. The materializer applies explicit fixture overrides and
does not calculate the expected renderer results.

The checked stock inputs `sd15-all-families`, `bt07-authored-bindings`,
`sd01-shape` and `sd18-unknown-feature` are referenced by exact case ID.
The unsupported case uses a synthetic output-capability result to test
publication refusal only. No real platform claim follows. These are
structural document/bindings fixtures, with resource closure and raster
execution owned by the renderer baselines.
