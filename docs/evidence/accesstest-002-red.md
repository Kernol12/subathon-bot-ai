# ACCESSTEST-002 first observed behavioral red

Root ran the reviewed baseline once on clean
`8acc9b9cf88bb67762c2060ee5fb16a5345a273f`, 2026-11-04T18:24:09.993522Z to
18:24:10.048423Z. [Receipt](accesstest-002-red.json),
[complete output](accesstest-002-red.txt), [JUnit](accesstest-002-red.xml).

```sh
ctest --preset effective-permissions-debug -L '^effective-permissions$' \
  --no-tests=error --output-on-failure \
  --output-junit /tmp/subathon-accesstest002-first-red/red.xml
```

Actual CTest exit8:30/30 failed, zero skipped. Every case's retained output contains
`Effective permission behavior is intentionally unimplemented; review tests first`.
Root's mechanical count is215 marker occurrences and216 failure locations; the
ownership test's safe caller-level presence gate supplies the additional failure.
Different actual attribution review is still pending at this evidence commit.
No compile/dependency failure, disabled/inverted test, fixture defect or skip is
being counted as behavioral red.

The exact reviewed test/inventory/header/stub blobs were checked before invocation.
Tests:fc82d17e; inventory:d0c25108; header:edd6ffc2; immediate stub:4cf7bcca.
Different actual test review1d8e9335/import6805011, root-harness review7d7f651/
importddb6fac, and actual label correction reviewebe17a9e/import8acc9b9 all
preceded this run. Logical/API/minimal source approvals remain linked in their
separate notes. No permission implementation exists; baseline completion will
not finish ACCESS-002 or any account/delegation/session integration.

Linux x86-64, GCC16.2.1, GoogleTest1.18.0, CMake4.4.5/Ninja1.13.2. Selected authentic
Boost1.83 host foundation previously passed21 primitives; old36 also passed.
Corrected actual Ubuntu selected CI at4674822,
[run41886146750](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41886146750),
compiled and exactly discovered30 enabled cases, passed independentSDK1/1 andold36,
and ran no feature body. The first red uses the same reviewed feature/harness
artifacts plus only the actual correction review note. Core dumps disabled.
No production arithmetic/validation/evaluation or live side effect was executed.
Raw receipts retain hashes and full command. Independent attribution and final
status/current CI/normal merge gates remain; do not replay the FIRST run.
