# RULES-006 — Single-rule reference evaluator

Parent: EPIC-003. Baseline: RULESTEST-006; implementation: RULES-006.
Authority: specification v0.45 §17,
§6/§10/NUM-04 and [reference-path-v1](../../contracts/reference-path-v1.md).
Concrete prerequisite: independently reviewed CONTRACT-014.

Own a small core evaluator plus explicit server authority binding for exactly one
immutable captured synthetic trigger/time-add action. Derive the requested amount
from the rule, retain source/action/version provenance and original context, and
reject every unsupported configuration. Check full requested quantity under actual
current ACCESS-003 automation authority, never a client allow flag or capped amount.
Changing configuration must not rewrite a previously captured plan. Core must not
depend on Qt/network/UI/provider SDKs. Reuse existing exact numeric boundaries;
no floats, guessed money/rate/evidence or caller-supplied resolved award.

RULESTEST-006 first supplies independent literal positive/zero/max/overflow,
nonmatch/disabled/unsupported config, immutable-version and revoked/limited
capability tests. Review actual tests separately and observe intended behavioral
red after successful build; missing dependencies/compilation/skips are not red.
RULES-006 waits for that gate and preserves all assertions, then needs passing
acceptance plus separate code review/CI. No complete priority/exclusion/condition/
generated-chain/action-category claim; RULES001–005 and API009 remain required.
Use existing GoogleTest/CTest home for pure domain logic; Qt service tests only
for actual authority/capture integration. Root coordinates shared state/build/CI.
Record checks and handoff; no live contribution/provider actions in default tests.
