# ACCTTEST-004 minimal interface — build and CI gate

2026-11-03, root integrator; separate from interface author bundle_implementer.
Both actual DOCUMENT approvals accept contract8fa0c76/author608acc1. This phase
adds types and explicit missing-feature stubs only; no service behavior or red.

| Actual artifact | Git blob |
| --- | --- |
| application_service.hpp |03a747a70ab7decd49227396466ff12a32c31864 |
| application_service.cpp |ebbea2388b7f4fb6474940b53cc14b95853969cb |
| header author note |a863a1d9fd2088cb40f8df5e76499ee87086c98b |
| root CMake |166cb1f80cac29d576261a28a59b49476452482e |
| core CMake |1f40148a08033f7dad09c382a08d7f367677c273 |
| server CMake |f07cd0d031e074a6ae1f0939cdd0bba6b68bd04a |
| presets |aeeca7a703346e403f63bcffeca1e4b54e3437f1 |
| selected Qt workflow |e507c083196fa0e97ff49a5c1102a4d63ba6774a |

Five entry definitions are unconditional named throws: command encode/decode,
retained-reference read, service make_work/enqueue. Constructor stores inert
borrowed dependencies/limits. No validator, codec, calculation, callback/queue,
SQL, commit, cleanup or publication behavior is implemented or invoked.

Actual root commands:

```sh
cmake --preset application-service-debug
cmake --build --preset application-service-debug --parallel 2
ctest --preset application-service-debug --show-only=json-v1
ctest --preset application-service-debug --verbose
```

Fresh118-step build PASS/ZEROwarnings on local GNU16.2.1/Boost1.92/Qt6.12. Only
the unchanged264 cases are registered; no service feature executable/case exists.
All264 old checks PASS20.47s;261Qtprocesses1060PASS/0FAIL/skip/blacklist. That is
regression/environment evidence, neither service acceptance nor a feature red.
No new entry/stub/factory/marker/view/codec was called. Original hold red was not
replayed. No service tests or production code exist beyond the explicit stubs.

Ignored raw SHA256:

- configure `local-evidence/accttest004-api-configure.txt`:
  e47bb0f0fb5086de99d7c39a026931537c803e6fa51d21e91a60651bc16a1a64.
- build `local-evidence/accttest004-api-build.txt`:
  5b40c0faba17190d622b59e590623f765b4a9cc46352133e9c59751605496eb9.
- discovery `local-evidence/accttest004-api-discovery.json`:
  ab795fb1a7d46cc8ee48aa9cebb2f5b47fe91260250f38db853b5688c759541f.
- old264 `local-evidence/accttest004-api-old264.txt`:
  d35b2f9fc9737a8c74d0418b0f86ec89cf9fede547502d6f04080f07561b42b9.

Profile21 inherits sqlite-hold-debug and enables the new service flag. Existing
exact/debt/timer libraries become available without enabling unrelated GoogleTest
targets; no new dependency. The selected ordinary Qt job changes ONLY49 preset
references to the inherited profile, preserving every old guard/test command,
required fixture probe, hold35 acceptance, archive/action/Boost pin, permissions
and status name. It compiles the new static interface; no service suite is run.

Actual separate header/stub/config review and the complete API candidate's OWN
official selected environment remain pending. Both must pass before independent
fixture/test authoring. Future fixtures/compiled service cases/first reviewed red
and ACCT004 implementation remain their own gates, with all release scope intact.

## Actual API approval and own selected environment

[Different header/stub/build approval](accttest-004-header-review.md)beb928cf
(importeda5af686) accepts exact interface/source/config/author pins, with its own
fresh34-step archive/dependency build0warnings/exact264 OLD discovery/no invocation.
Five verification config copies remain preserved in that isolated reviewer tree.

Complete API candidate **8d742239616aec1b8e9be65e972a42b4fcf2ae7a** passed OWN
[selected Qt41673443963](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41673443963)
16:21:58–16:24:36Z: fresh118/ZEROwarnings/GNU13.3/pinnedBoost108300/verifiedQt6.12/
SQLite3.53.4. The log explicitly compiles and archives the new interface target,
while running only old stable suites; requiredhold3guard5PASS0skip179ms/exact35/
UNCONDITIONALhold35PASS26.73s/all11sameheadSUCCESS. No service test or marker ran.

Raw local-evidence/accttest004-api-official-ci.txt SHA256
8cd6edea68ab01f2409d11c9e9621be1d66f4f9be18fb068ed13b260e96ac4e8. Actual synthetic
checkout568fc520d6af4956f21cc1848ba6c4df961b256e parents efe+8d742; APIverified
tree df709b23f3a20be0a60407026b2335de585165bb equals8d742tree. Exact run/checkout
records are accttest004-api-ci-run.json and accttest004-api-ci-checkout.json under
ignored local-evidence. Earlier docs-only green is not substituted for this proof.

Minimal API review and own environment gates are satisfied; root may now assign
independent synthetic fixtures. Their actual separate review/mandatory own selected
foundation green precede independent feature tests/full review/first service red.
No production service/codec/validation/queue/transaction behavior is implemented.
