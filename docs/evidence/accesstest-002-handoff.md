# ACCESSTEST-002 baseline handoff

Root coordinator, 2026-11-04; focused
[PR62](https://github.com/Kernol12/subathon-bot-ai/pull/62), branch
`task/ACCESSTEST-002-effective-permissions`. The executable baseline is complete, with actual independent red attribution
3f40238f. ACCESS-002 remains unimplemented;
its entire evaluator still throws the explicit marker immediately.

The owner accepted resource-scoped individual overrides: select the operation and
whole resource tuple before quantities, without role fallback inside that scope.
A five-minute individual allowance on A denies A/+10 despite a ten-minute role
allowance; B/+10 still uses the role. The
[decision](accesstest-002-owner-decision.md), specification §10/AUTH-06,
[logical model](../contracts/effective-permissions-v1.md) and
[API](../contracts/effective-permissions-api-v1.md) agree. No owner decision remains.
Separate [timer choices](timer-editorial-owner-decisions.md) are preserved for PR52.

## Exact independent stages

| Actual artifact | Different review |
| --- | --- |
| Logical3d82bfa4 / author064fecab | codec_author0e8bffe9; [review](accesstest-002-contract-review.md) |
| APIdda2793c / author646a11e3 | codec_author5c029d4e; [review](accesstest-002-api-review.md) |
| Headeredd6ffc2 / immediate stub4cf7bcca / author78195bef | codec_author7ddc58b0; [review](accesstest-002-source-review.md) |
| SDK-only environment8d88fd0e / author58b7f581 and root harnessdab476e | codec_author15460f62; [review](accesstest-002-environment-review.md) |
| Thirty testsfc82d17e / inventoryd0c25108 / author25001158 | codec_author1d8e9335; [review](accesstest-002-tests-review.md) |
| Root test build/workflowb519153 | service_reviewer7d7f651; [review](accesstest-002-tests-harness-review.md) |
| Root CTest label compatibility3dc7108 | service_reviewerebe17a9e; [review](accesstest-002-labels-review.md) |

The inventory maps all45 logical EP examples plus D1-A/B, with raw synthetic
setups and independent expected outcomes. It covers whole-grant matching, complete
explanations/failed keys/ordering/ownership, protected owner and target refusal,
exact fractions/signed minimum/long values, strict facts/shapes/canonicality,
validation priority and all six complete input/result budgets. Budget assertions
do not themselves measure internal allocation order or allocator fault handling;
production review must audit those stated API obligations. No test assertion,
expected fixture, header or stub changed after its actual approval.

## Observed checks and limits

[First SDK environment](accesstest-002-environment-first.md): authenticated selected
Boost1.83 host, 21 primitive checks pass1/1; old36 numeric/infrastructure also pass.
Root configure/build/list67 and exact30 enabled inventory/metadata checks pass.
Host is Linux x86-64/GCC16.2.1/GTest1.18.0/CMake4.4.5/Ninja1.13.2.

Selected Ubuntu CI at46748221d4ccca43b5279e3c568ad157e918072e passed all13 checks.
[Actual corrected lane41886146750](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41886146750)
compiled/exactly discovered30 without executing feature bodies, then passed the
independent selected Boost1.83 environment andold36. The preceding strict label
failure41884253112 is retained in the [correction](accesstest-002-labels-correction.md),
not counted as red; guards and expectations were preserved.

[First genuine red](accesstest-002-red.md) at clean8acc9b9cf88bb67762c2060ee5fb16a5345a273f,
2026-11-04T18:24:09.993522Z: plain CTest exit8,30/30 failures,0 skipped. All30 cases
contain explicit missing-behavior markers; full raw output/JUnit/receipt/hashes
are retained. [Independent attribution](accesstest-002-red-review.md)3f40238f confirms all216
failure locations:215 explicit markers plus the safe ownership presence gate.
No setup/dependency/fixture/skip failure was counted. Do not replay the FIRST run. This is reviewed missing behavior, not passing product
acceptance. Default CI compiles/lists intentional red and runs the separate green
SDK/existing suites; no inversion, disabled case or generic error continuation.

Before any merge: review this actual final status/map/HTML/handoff, push normally,
require all current-head applicable CI,
inspect rules/reviews/discussions and merge normally. No branch protection bypass.
Implementation and account/current-session/catalog/queue/transaction/disclosure/
delegation/recovery/editor integration, actual production arithmetic/allocation
strategy, supported platforms/live operation and full release gates remain.

Next priority after PR62's clean merge: go through preserved PR52/54/56/59 with the
owner. Their known decisions are answered; remaining evidence/build/review gates
need execution, not routine owner permission. Do not start ACCESS-002 ahead of that
priority or discard unfinished work/valid reviews. Root owns shared state/map/build/
CI/PR/checkpoint; `progress/state.json` is authoritative and HTML is regenerated.
