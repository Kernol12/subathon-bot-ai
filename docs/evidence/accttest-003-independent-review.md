# ACCTTEST-003 independent baseline review

Author: `/root/application_plan_test_author` (separate session).
Reviewer: `/root/application_plan_reviewer`, 2026-10-31.
Reviewed candidate: `06ee499`, including authored candidate `e864ff4` and harness.
Review branch: `task/ACCTTEST-003-plan-review`; reviewer owns this note only.

Verdict: **approved for test baseline**. No blocking findings. This approval permits
coordinator observation of intended stub-red; it does not approve implementation
or claim a red run has occurred.

## Exact reviewed artifacts

| Artifact | Git blob |
| --- | --- |
| Application-plan revision1 contract | `5bb9a914f8b80a7ad43cabcccf3905294a9f7a63` |
| Core interface | `501112d64959d20f682420d33a260e7745f5b378` |
| Three unconditional named stubs | `ac9dcd0a73ec12fffaf3bdb1ff45f9f1f80fc23f` |
| All22 test cases | `f93287fa2c7c3ef4753e6bbbedd1d2a54f4529b9` |

Read root/core/tests AGENTS, FIRST_AGENT_PROMPT, CLION_SETUP, architecture,
workflow, test strategy, progress state, ACCT-003 brief/author evidence,
specification v0.45 §§3/6/7 and NUM-01/02/04/MOD-04, event-state revision1,
receipt-intake revision1, contribution-eligibility revision1, exact-decimal
revision1 and time-credit revision1. Inspected root/core/tests CMake and presets
against existing main. Existing assertions/fixtures/interfaces are preserved.

## Independent oracle examination

Each numbered row corresponds to the actual CTest discovery order and independently
examined test body, not a generated expected result.

| Case | Oracle and specification basis |
| --- | --- |
|1 Original envelope/quote/holds | One revision1, absent parent/override, exact supplied projection and decimal spellings, unknown actor/source times/source metadata, independent pending/unknown/unresolved/not-applied conditions; §§3.2–4, NUM-01/04. |
|2 Rules/modifiers | Both sources receive exactly the captured event once; ordered literal selected/excluded explanations and two-level operator/version provenance survive; §§6/7. Doubles only copy/record. |
|3 Ownership | Later caller context/money/rate/request/review edits and clearing source selections cannot alter the existing captured plan; §§3.4/7. |
|4 Exact/signed/zero | Exact3/2 with resolved2, exact−5/2 with resolved−2, zero and action/reversal provenance are copied. Hand-derived half-even examples are consistent with NUM-01/04; this API does not calculate them. |
|5 Request versus actual |120000ms stays requested after state-token change; application stays not-applied. No cap/actual output exists; B03/B04 and §7 example remain intact. |
|6 Missing history | Explicit missing quote, no requests, unchanged independent holds; §§3.3–4. |
|7 Independent request | A request explicitly marked independent of valuation survives missing historical quote; it does not resolve valuation or authorize application. |
|8 Dependent request | Missing historical quote rejects a valuation-dependent request; no zero or current-rate substitution. |
|9 Nonmonetary | Explicit not-applicable with absent money stays distinct from zero money/confirmed evidence. |
|10 Valuation shape | Resolved without quote, not-applicable with quote and different original spelling fail; lossless source spelling is intentional, including20.0000 versus20.00. |
|11 Local identity | Empty plan/event/root and zero receipt fail; no guessed provider identity. |
|12 First override | Explicit authorized audit appends revision2/parent1 with replacement selections/request/review, retaining original revision and caller history; §§3.4/7. |
|13 Rate resolution | Recorded historical/manual resolution appends without fabricating confirmation or acceptance. |
|14 Authority | Unknown/denied result fails despite audit strings, before sources; enum records an upstream precondition, not authenticated permission. |
|15 Audit completeness | Actor, reason and decision reference each required; original unchanged. |
|16 Expected head | Stale expected0 against head1 fails before either source; original unchanged. Atomic persistence remains downstream. |
|17 Immutable event | Changed event reference, money, arrival configuration, contribution or ancestry fails; original unchanged. Whole event equality is the contract, cases sample meaningful components. |
|18 Repeated override | Append revision3/parent2 preserves both prior revisions and invalidates revision2 preview. |
|19 Current preview | Matching tokens report current without changing contribution/plan; freshness never grants eligibility/permission. |
|20 Stale precedence | Stale plan wins over stale state, then matching plan with old state reports stale_state. |
|21 Empty tokens | Either empty authoritative state token fails explicitly. |
|22 Malformed history | Empty history rejected by both operations; initial parent/decision and noncontiguous revision rejected by preview. |

Expected projections use literal synthetic fixtures and ordinary equality. Exact
fractions use Boost rational constructors solely as value representation; no
production parser/evaluator supplies expected outcomes. The source doubles contain
no rule matching, exclusions, modifier combination or calculation algorithm.

## Boundary and harness assessment

Owned pure transforms are an appropriate bounded refinement of ACCT-003; no server
repository fake is needed. Original event facts/arrival remain immutable while
replacement rule/modifier selections are explicit override calculation context.
Historical valuation, contribution holds and authorization are deliberately separate.
Actual caps, atomic effects/jobs, evaluator behavior and durable audit remain assigned
to later tasks; the contract does not silently remove them from release scope.

All three production functions remain unconditional ApplicationPlanNotImplemented
stubs. The optional application-plan flag defaults OFF, adds only an explicit core
library/test executable and intentional-red labels, and does not add known failures
to stable passing CI. Boost::headers reuses the selected Boost>=1.83 dependency;
sharing its discovery conditional leaves the existing exact-numeric branch intact.
No additional binary link to decimal implementation is necessary for owned Exact
values and existing contribution/receipt projections.

Coverage limits:22 cases are meaningful bounded examples, not exhaustive validation.
They do not individually mutate every immutable field, every malformed later audit
or every valuation-presence combination. Revision exhaustion is contractual but no
runtime overflow case is authored: a valid vector-backed contiguous1..head chain
would require UINT64_MAX entries before head+1 exhaustion, beyond realizable local
resources. A malformed max-head fixture would test invalid history instead. Keep
checked revision increment in later code review; do not claim runtime overflow
coverage here. Upstream rate syntax/numeric consistency/reference resolution are
explicit source preconditions and have separate baselines.

## Commands actually run

Reviewer worktree, Linux x86-64, GNU16.2.1, Python3.14.8, installed Boost1.92.0:

| Command | Observed result |
| --- | --- |
| `cmake --preset application-plan-debug` | exit0; configured installed dependencies |
| `cmake --build --preset application-plan-debug` | exit0;10 build steps, no diagnostics |
| `ctest --preset application-plan-debug -N -L application-plan` | exit0;22 cases listed only |
| `git ls-files -s` for four baseline artifacts | exact blobs above confirmed |

No test bodies ran in this review session. Compilation/listing is not observed red.
Coordinator must run and retain the actual22 intended failures after this committed
approval before implementation. ACCT-003 still needs passing relevant acceptance,
separate code review and stable automatic CI. Qt writer ownership, SQL durability,
concurrency, permission engine, full evaluator, provider capabilities, live services,
Windows/other platform and release checks remain unverified. All seven integrations
and three rate providers remain required.
