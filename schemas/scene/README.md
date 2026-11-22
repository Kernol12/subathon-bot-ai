# Scene schema package v1

The five resources under `v1/` encode the authored document, bindings, asset
revisions, portable package manifest and lifecycle records. All share the exact
[wire primitives](../common/v1/wire-primitives.schema.json) and use Draft 2020-12.
The registry key is the absolute schema ID. Repository validation preloads only
these resources and resolves references offline.

The [approved proposal](../../docs/contracts/scene-schema-validation-v1.md)
describes the owning contracts and the structural, lexical, domain and publication
checks. The package manifest selects exactly one immutable scene revision under
owner correction 159. Archive framing has its own review.

The [independent cases](../../tests/schema_validation/fixtures/scene/v1/cases.json)
cover complete instances and named components. The repository checker executes
their structural expectations. Later-phase expectations remain source oracles
for the corresponding domain, profile and publication checks.

## Consumer gates

- R8 requires a shared layout compositing rule and conformance review before
  renderer implementation. The proposed order for progress is track, fill or
  unavailable paint, state text, then slots in array order. Draw support rows
  in supplied order and their slots in array order. Apply each text record's
  own opacity and local clipping to its text, then apply the node opacity and
  clip to the layout composite. Renderer conformance still needs independent
  examples including overlapping text, overlapping rows and node opacity.
- R9's authority and initial `en` and `de` rules are specified by the
  [Scene formatter locale contract](../../docs/contracts/scene-formatter-locale-v1.md)
  under owner decision 160. The existing JSON translation catalog supplies
  built-in label text. Semantic validation and exact catalog support on both
  renderers still need their owning checks. The selected-closure fixture
  retains its synthetic locale premises.
- The owning validators still determine exact reference closure, bounds, media
  inspection, rights and publication eligibility. Platform and deployment
  compatibility have separate evidence gates.
