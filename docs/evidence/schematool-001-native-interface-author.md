# SCHEMATOOL-001 — minimal native interface and inert stub author

Author B (`/root/session_supplier_author`), 6 November 2026. NEW isolated branch
`task/SCHEMATOOL-001-native-interface-author`, worktree
`/tmp/subathon-schematool001-native-interface-author`, from actual root
`075f3567da0893265a1afac828171ae62959ec8b`. Previous parent-author tree/branch is
preserved. Root assigned ONLY the new header, inert stub and this note. Root owns
build/version map/preset/CI/shared policy and state; A separately owns independent
foundation/test source; C/root review actual authored source before execution.

Read root/tools AGENTS (no more-specific instruction exists under
tools/schema_validation), actual [native contract](../contracts/schema-validation-native-process-v1.md)
blob `aec16d6eda116a38eb76a454133890f8d53c6cb3`,
[ADR0009](../decisions/0009-schema-native-process-owner.md), C exact native-contract
review `44adb36cad2cc75a7909692c49785e16731a140e` and policy review
`48c07527ba61d7df033037f64a3b70b3bd9e02a3`, and selected C++/format/build context.
Those DOCUMENT approvals permit only this separately assigned minimal authoring;
they are not actual source/build/test/capability approvals.

## Exact authored interface

[transport_v1.h](../../tools/schema_validation/native/transport_v1.h) declares the
sole `subathon_schema_transport_v1` C ABI with the contract's exact eight ordered
pointer/unsigned-int parameters and int return, C++ noexcept and C11-compatible
declaration/types/include guard. Compile-time assertions require 8-bit bytes,
32-bit int/unsigned-int and 64-bit pointers. The declaration marks only this symbol
default-visible for the root-owned hidden-visibility/version-map build. The local
noexcept macro is undefined after use. No enum, extra function/type, callback,
success factory or external dependency is added.

[transport_stub.cpp](../../tools/schema_validation/native/transport_stub.cpp)
includes only that header, defines that exact extern-C/noexcept symbol and has a
ONE-statement body: literal `return 31;`. Unnamed definition parameters suppress
unused-parameter issues without reading/copying/dereferencing any argument. There
is no runtime argument processing, file/descriptor/child/thread operation,
allocation, callback, static initializer/resource owner, validator or native
transport behavior. GPL-3.0-or-later is preserved. This is a named native missing-
feature baseline, never product ok and never ordinary environment/dependency
failure or a claimed clean validation.

The future root-owned artifact remains exactly
`<repo>/build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so`,
target `subathon_schema_transport_v1`, under the reviewed opt-in profile. No
artifact was created or loaded/built by this assignment. Later independently
reviewed binding/harness must attribute literal 31 to the distinct named native-
unimplemented marker ONLY for the NEW native baseline; it is forbidden in completed
production. This assignment does not edit/fallback/replay the current public APIs,
worker, frozen 64 tests or historical FIRST/raw/review evidence.

## Actual checks and remaining gates

Allowed checks are source/document inspection, `python3 tools/check_project.py`,
`python3 tools/progress.py render --check`, whitespace and changed-path/source-blob
inspection. Project426/122/current HTML PASS, render current PASS, whitespace PASS;
changed-path inspection confirms only the three assigned paths. Parent source
remains `3438a92c9a98a4aa326ed733abffeee6c26c7502`. These repository checks
execute no authored native/Python candidate, helper/test or SDK behavior and prove
no native facility. No compiler/linker/header syntax probe, CMake configure/build,
artifact load, ctypes call, foundation/test, child/thread/signal or installation
was run. There is no Python binding/source/test/build/CI/progress/policy edit.

Next gates: DIFFERENT actual interface/source/build/test/oracle review; actual
GREEN build/sole-export/C11+C++ ABI/loader/ctypes/kernel foundations on BOTH selected
hosts; NEW independently reviewed intended native missing-feature red; only then
separately assigned owner/binding implementation. Native resource/async-safety/
identity/deadline/offline audits, every preserved/ported 74-method oracle, SDK14/
20-package checks, full public64 and unconditional actual hosted CI/latest-head/
normal merge remain. This stub finishes none of those implementation gates.
