# ACCTTEST-004 final feature baseline environment

2026-11-03, root integrator. Test baseline only; production ACCT004 remains planned.
Owner requests completion of PR49 followed by STOP, with no next task.

| Frozen complete artifact | Git blob |
| --- | --- |
| Feature tests |36f669edd1b42cdcd78c36145ac61b8b6124c454 |
| Final test author evidence |205f0cfaa078a8897d395ba00c432b655b733e58 |
| Root Qt registration |75aab5f1d132560ea5579c61042488aa0bef0e35 |
| Selected workflow |a4ae72c664bac073c44d7f7c7577c5b4a9fa1fce |
| Concrete contract |bbd6cd834b8a959ccff16d63a638e0b77d9c6e4c |
| Public header / immediate stub source |03a747a70ab7decd49227396466ff12a32c31864 / ebbea2388b7f4fb6474940b53cc14b95853969cb |
| Independent fixture JSON / helper / environment |6598ad48b12c4d4871a114be404e884cb7deffb1 / 1909eaacfdb9c84fdebe1df394e8f31acf6777d7 / 26ed3153d0fe31effcc194b118db1bd46eeea811 |

Actual authored inventory is35 feature slots/224 scenarios, statically verified
without invoking data methods. Frozen frames include38 command cases in each
codec direction, five numeric-overflow rows,17 historical identity rows and all18
previous full successful numeric records. No old assertion or oracle is weakened.

Root actual native commands:

- `cmake --build --preset application-service-debug --parallel 2` — PASS, final
 3-step compile/link, zero warnings, GNU16.2.1/Qt6.12.0/C++20.
- `build/application-service-debug/tests/qt/server_application_service_acceptance
  -functions` and `ctest --preset application-service-debug -L
  '^application-service$' --show-only=json-v1` — exact35 compiled methods equal35
  enabled registrations, draft-red, no failure inversion. No init/data/test body,
  service/codec/view/stub/marker call occurred.

Ignored native evidence SHA256:

- accttest004-final-feature-build.txt:
  279fce86d3bf77fccdde54c0190b2606c3effd57a891c52f1d5b69f114914f3e.
- accttest004-final-feature-functions.txt:
  3024f67458b756cc7e3aa2adfdcd0e72cee22fe4f209aadfdc94c7703cf69fc7.
- accttest004-final-feature-discovery.json:
  94a8ffc15c3b63ecaa9fc68d114f0ec4fc866d50689613dde4736b9f88e3c381.

The mandatory CI foundation links only existing storage primitives and cannot
invoke the service archive. Its approved MIN candidate's own gate passed540887b /
41695399236 with41PASS/0skip/zero warnings, as recorded in
[fixture CI](accttest-004-fixture-ci.md). That run does not substitute for this
COMPLETE final feature candidate's own selected-green.

Actual full independent final source/note/harness approvalfa63ccda accepts all
frozen artifacts above. Reviewer own fresh4-step build/zero warnings/exact35
compiled-registered/source slots/static224 and complete MIN-CI audit pass.
The complete approved candidate's OWN selected CI is pending. Only after it passes
may ROOT invoke
FIRST named missing-feature red on the SAME CLEAN head. The enabled intentional
red suite is separate from mandatory passing component/foundation checks; CI
compiles/discovers its exact methods without executing feature bodies. Red output
must show exact new markers after valid setup, with per-invocation counts and
zero skips, crashes, timeouts or unrelated setup failures. Production, live
providers, actual adapters and platform/release behavior remain unverified.


## Actual complete approved-head selected gate and first-red order

Head4e4c6ebc279edf1453cd87150fc6fb7b6066a1ad passed OWN selected41697865377
18:52:03–18:53:58Z, fresh126/zero warnings/GNU13.3/Boost108300/verified official
Qt6.12SQLite3.53.4/oldstable/exact6/41PASS0skip1115ms/exact35 compiled discovery,
all11same-head SUCCESS. Rawc2fda6dc40ae721f39c246775226c9532dc6a4a3589a3f9c230603914021be4f.
Actualsynthetic2d4453e parents efe+4e4/tree5c352925a0ac07e92b4aac3a3cdb46869428f2d4
APIverifiedequalcandidate. Only AFTER this completed gate ROOT invoked FIRST
same-CLEAN-head feature red: exit8/35processes/224exactnamedmarkers/70initcleanupPASS/
0skip/setup/crash/timeout. [Handoff](accttest-004-handoff.md) and original
[per-invocation raw](accttest-004-red-ctest.txt) retain actual proof. No prior run
or component green is substituted; no tests/assertions/fixtures changed afterapproval.
