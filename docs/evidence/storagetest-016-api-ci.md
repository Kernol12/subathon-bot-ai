# STORAGETEST-016 — minimal API compilation and stable-suite gate

Current phase: actual minimal interface/stubs only. Concrete701c/[reviewc0](storagetest-016-contract-review.md)
and physical83e/[review696](storagetest-016-physical-review.md) precede this phase.
[API author](storagetest-016-api-author.md)ac0610f supplies APIDOC421acf29,
headerfe50fb10 and CPP5b10c0e0: finite owned values, six IMMEDIATE UNCONDITIONAL
unique-marker throws and an inert constructor only. No validation, SQL/DDL/catalog
data, source lookup, mutation, cleanup or hold implementation. No new entrypoint,
catalog factory or marker has been invoked.

## Exact minimal configuration and actual compiler result

Root CMake5d763585 adds HOLDflag→ORIGINAL→BUNDLE before the existing dependency
checks. Server CMakeb55b5912 appends one conditional static hold library linked to
old bundle/QtCore, no extra dependency. Presets053b517a adds20 sqlite-hold-debug
inheriting19 with matching build/test presets. Workflow88f8073e changes ONLY
preset/build-directory tokens to20, preserving every old step/pin/guard/count/
permission/check name and unconditional old18 original-claim execution. No new
test slot or hold invocation is added at this stage.

Actual root commands, host GNU16.2.1/Boost1.92/Qt6.12/SQLite3.54:

```sh
cmake --preset sqlite-hold-debug
cmake --build --preset sqlite-hold-debug --parallel 2
ctest --preset sqlite-hold-debug --show-only=json-v1
clang-format --dry-run --Werror server/include/subathon/server/sqlite_application_holds.hpp server/src/sqlite_application_holds.cpp
```

Configure/fresh102-step build PASS, ZERO warnings; exact228 OLD enabled
registrations, no hold feature case. Formatting/static project/preset/diff PASS.
Raw local-evidence/storagetest016-api-configure.txt SHA256
61fc1e53957281337223bd5d5bcf322d80b2bcc258b8a0f5ce5717955bae48b0; build SHA256
781a125a6c37768adde613d36d0101f642cfc37699848513a64a736943956cbc.

Root freezes the complete actual source/config before old228 stability execution.
Author independently ran old228PASS8.00s/225Qt715PASS0fail0skip; that is the author's
evidence and not this future root run. Actual different API/header/stub/byteformula/
configuration review is active, no verdict yet. Own API-source selected official
Qt6.12/SQLite3.53.4 stable CI remains unobserved; old document/017 CI is not that
gate. No future-head approval or held capability is inferred.

Only actual API/config approval plus own selected-source oldstable CI permits
independent literal step6 foundation authoring. Its actual new selected-Qt green
then precedes feature tests/full separate review/build/discovery/same-approved-head
selected environment/FIRST intended hold-stub red. Production STORAGE016 waits
its completed baseline and later separate source review/acceptance/promoted CI.
Service004 and all policy/producer/provider/platform/full release gates remain.
