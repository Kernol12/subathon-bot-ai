# STORAGETEST-016 bounded input-category review

Read-only precision audit of accepted [concrete contract](../contracts/sqlite-application-holds-v1.md)
blob `701c710f83b0361a075c0d0bca7cf6ec07fb351e` (§cause/plan/stage matrix,
§bounds/preflight), [API](../contracts/sqlite-application-holds-api-v1.md) blob
`421acf29012f31345cc41cbee239bcf95000ad62` (§finite ownership, §branch groups,
§pure input order), and public header blob
`fe50fb10e6f5a7198df27358666734ad02fb47d1`. This reviews proposed isolated
oracles, not unfinished feature tests or a future API revision.

Actual root working diff inspected: API `421acf29012f31345cc41cbee239bcf95000ad62`
→ `46c70297bacddbdd1649abfe58a76b33ce9773c4`, exactly18 inserted lines after pure
input order, with every earlier byte retained. **Approve that actual clarification.**
It pins the following isolated oracles without changing the public enum or stub:

| Isolated malformed input; all earlier stages valid | Category |
| --- | --- |
| `DetectionData.audit.document` is null | `invalid_detection` |
| Numeric-overflow cause lacks its required `FailureEvidence` | `invalid_failure` |
| Illegal cause/PlanEvidence kind, extent or stopped-for combination | `invalid_plan_evidence` |
| Otherwise legal cause/plan branch has wrong numeric failure stage, or known failing ordinal is nonpositive/not a retained occurrence | `invalid_failure` |
| Malformed `Observation` annotation or observation-local scalar, whether `Cause.first_observation` or `DetectionData.observation` | `invalid_observation` |

An enclosing Detection does not absorb its nested Observation/PlanEvidence/Failure
category. Required detection audit belongs to DetectionData; observation annotation
belongs to Observation. A non-null empty document is valid DATA. An empty required
format/reference key instead belongs to the earlier `invalid_key` tier; it is not
an isolated null-document case. Optional DATA fields permitting present-empty are
not thereby malformed keys. CapturedRevision's required known state-token presence
remains a plan-evidence matrix condition; FailedCapture does not acquire an invented
unconditional token requirement. Resolving a supplied token is post-preflight work.

Precedence remains the accepted stage order: all options; all applicable keys and
typed subjects; all per-vector/combined counts (including replacements); required
scalar/envelope/optional-group/enum shape; branch matrix and Exact bounds; local
ordering/link/replacement shape; aggregate owned input bytes. Thus malformed keys
precede collection overage, and collection overage precedes malformed element
bodies. An otherwise valid oversized Exact component uses `exact_component_limit`
before aggregate `owned_byte_limit`. Both Exact component sizes precede canonical
grammar/arithmetic; the selected-SQL class/nonempty checks are a separate preceding
requirement, not an invented check on public `core::Exact` internals.

No total order among unrelated defects within one stage is established here.
In particular, do not infer a universal plan-versus-failure-versus-Exact precedence
from this table: isolate the tested defect or use only an explicitly ordered pair.
Nested source/plan/state existence, selected revision equality, current token or
permission truth require actual capability/SQL and are not pure shape checks.
Their missing/source-mismatch/stored-corruption distinctions remain unchanged.

Checks were source/document inspection and `git diff --check` only. No new hold
method, factory, marker, feature body or native test was invoked; five existing
configuration verification copies remain uncommitted. Root owns any API wording
clarification; approval above covers only the inspected18-line diff. Full authored test review,
compiler/discovery, own selected CI, intended first red and production gates remain
separate and unapproved by this bounded audit.
