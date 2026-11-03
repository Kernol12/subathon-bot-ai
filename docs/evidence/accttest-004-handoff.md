# ACCTTEST-004 reviewed missing-feature baseline handoff

Root coordinator/integrator, 2026-11-03, task/ACCTTEST-004-service-baseline,
[PR49](https://github.com/Kernol12/subathon-bot-ai/pull/49). Owner requests this PR
be finished cleanly and then all coordination STOP. No production task is started.

## What changed and why

The original application service now has reviewed logical/concrete contracts,
minimal owned C++ interfaces with five unconditional named stubs, independent
literal SQLite fixtures, mandatory old-only foundation diagnostics and35 public
Qt feature cases with224 scenarios. Full test source/harness approvalfa63ccda
precedes the candidate's OWN selected CI and the FIRST intended red below.
This finishes a test baseline only; ACCT004 service behavior remains unimplemented.

Authority: specification v0.45 §§3–5/NUM-01–04/B08, logical6d0cd67f,
concretebbd6cd834b8a959ccff16d63a638e0b77d9c6e4c. Separate SAH2 clarification
retains the actual full signed-correction policy list and failed balance without
rewriting captured target main, old wires, storage schema or public ABI.

## Exact reviewed revision and environment gate

FIRST-red head **4e4c6ebc279edf1453cd87150fc6fb7b6066a1ad** was CLEAN before and
after invocation, with reviewed source/harness/author hashes unchanged:

- CPP36f669edd1b42cdcd78c36145ac61b8b6124c454;
  author205f0cfaa078a8897d395ba00c432b655b733e58.
- Qt75aab5f1d132560ea5579c61042488aa0bef0e35 /
  workflowa4ae72c664bac073c44d7f7c7577c5b4a9fa1fce.
- Header03a747a7/sourceebbea238 are the unchanged immediate stubs.
- Complete fixture JSON6598ad48/helper1909eaac/env26ed3153/author5cc7bfba,
  separately reviewedd19c8cf7 and OWN-MIN selected41695399236 passed before
  affected test authoring. All old33 case objects and all prior assertions/oracles
  remain exact. Final MIN/full-command additions are independently reviewed.

OWN complete approved head's selected41697865377 passed18:52:03–18:53:58Z,
fresh126 steps/zero warnings/GNU13.3/pinnedBoost108300/verified officialQt6.12/
SQLite3.53.4/old stable suites/exact6 foundation41PASS0skip1115ms/exact35 feature
compile-discovery ONLY/all11 same-head checks SUCCESS. Raw SHA256
c2fda6dc40ae721f39c246775226c9532dc6a4a3589a3f9c230603914021be4f.
Actual synthetic2d4453e790ffa15e60618052deac9e303a3726d0 parents efe1c7e+4e4c6eb,
tree5c352925a0ac07e92b4aac3a3cdb46869428f2d4 API-verified equal to the candidate.
Earlier fixture checkpoints were not substituted for this complete-head gate.

## FIRST actual intended red

Only AFTER full actual review and the completed own selected gate, ROOT ran ONCE:

~~~sh
ctest --preset application-service-debug -L '^application-service$' --verbose
~~~

Observed CTest exit8. All35 test processes fail ONLY from the explicit new stubs:

| Exact marker family | Unique failed scenarios |
| --- | ---: |
| OriginalApplicationCommandEncodeNotImplemented |49 |
| OriginalApplicationCommandDecodeNotImplemented |48 |
| OriginalApplicationServiceNotImplemented |125 |
| ApplicationReferenceReadNotImplemented |2 |
| Total |224 |

All70 init/cleanup invocations PASS; zero skipped/blacklisted/fatal/timeout/setup
failures. The [unaltered per-invocation LastTest log](accttest-004-red-ctest.txt)
SHA256 is **e05dafc8c71e1ed07ca38d93e95a33e48edc7d6e791bfc81fc0ad96b2f49996e**.
Each scenario reaches one exact named marker after valid setup. Counts use that
single LastTest capture, not duplicated verbose/output-on-failure stdout. Original
stdout, exit8, preflight, JSON audit and CI/API/check records remain under ignored
local-evidence. No test, fixture, stub or assertion changed to obtain this red.

Root native final compile/link passes3steps/zero warnings/exact35 registration;
different reviewer own fresh4step/exact35/static224 passes. Native stable check
`ctest --preset application-service-debug -LE '^draft-red$' --verbose` passes all
265 existing+foundation checks21.47s:262Qt invocations/1101PASS/0FAIL/0skip. It is
separate from the newly executed intentional red above. Stable raw SHA256
37da9c002e9e833d549565382d19a242605872f9214e8d35bceacb0ae7b07ba2.

## Independent reviews and remaining gates

[Final source/harness review](accttest-004-test-review.md),
[bounded test review](accttest-004-test-codec-review.md),
[fixture review](accttest-004-fixture-review.md),
[fixture CI](accttest-004-fixture-ci.md),
[complete-head CI](accttest-004-baseline-ci.md) retain exact artifacts, prior
findings/corrections and reviewer identity. Author bundle_implementer, independent
fixture author bundle_validation, full reviewer bundle_reviewer, shared harness/
integration root. Historical prebaseline findings were corrected and reviewed
before first red; no observed red was replayed or weakened.

Actual independent attributioncc9909ad629b52e0ec455c7762de9a1fbd6d5dd5,
imported9ca4036, APPROVES this first-red gate after independently checking actual
raw/CI/API/preflight/source identity and temporal order. No replay. ACCTTEST004
is DONE as a reviewed observed-red baseline. Assertions beyond each immediate
stub remain unexecuted until separate production implementation acceptance.
Completion-metadata latest CI, current rules/discussion checks and normal merge
remain the separate closing gates at this commit.
After eligible PR49 merge, STOP; ACCT004 stays planned. All actual adapters,
provider/FX/financial/refund/review/UI/platform/security/recovery/packaging/live
release work remains required. These synthetic component/baseline checks supply
no passing service implementation or full product verification.
