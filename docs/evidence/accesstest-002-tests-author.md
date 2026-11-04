# ACCESSTEST-002 feature-test author handoff

Author `service_author`, 2026-11-04. Isolated
`task/ACCESSTEST-002-tests-author`, initially clean at release
`a047009f50465d46c3d5142c6d9684d95d678938`. Only the new
[test source](../../tests/acceptance/effective_permissions_tests.cpp),
[case inventory/map](../../tests/fixtures/effective-permissions-v1.json) and this
note change. **Authored; compile/link/list only. No test body or marker was run.**
Different actual full test/harness review and root's FIRST red remain pending.

Frozen inputs read/preserved: logical `3d82bfa418bb14099ee130fca7a400adca66500b`,
API `dda2793c71299dbe36a321ce3dd5620fecc8b214`, header
`edd6ffc25379e5036d5e1622346eaa3f272a1c5c`, immediate stub
`4cf7bcca19e5ab879b62c70c1521d9f977abfd3b`. Minimal approval is root's actual
7ddc58b0/import f1acce3; it is not approval of these tests. Root/core/tests
instructions and the full approved model/API apply.

Candidate source blob `fc82d17ede5d2afe66e42572c17762510e5682f4`; inventory
`d0c25108ec233cdc11260aa1263974805b0fba15`. Exactly 30 registered
`EffectivePermissions.*` names; the JSON records those names, all 45 EP + 2 D1
references and API sections, without generated expected values or a fixture reader.
Raw aggregate builders do not call a SUT factory/parser. Each case reaches the
one evaluator directly from raw facts; row helpers catch only typed expected
errors as success. Unexpected markers/exceptions fail the row. Nonfatal tables,
caller traces, size/iterator guards and the ownership test's caller-level presence
gate preserve diagnostics without unsafe access after failure.

Coverage includes accepted resource-first A/B outcomes; whole-grant quantity/resource
nonunion; conditional denies and overlapping/conflicting/disjoint scopes; narrow
owner protection/target guard/current snapshots; exact fractions, signed minimum,
2^64 and long canonical components; complete source rows, failed keys, suppression,
unsigned/NUL identities, deterministic order and owned data; strict fact/catalog/
request/policy/rational shapes, staged priority and all six profile ceilings.
All expected outcomes/rows/budgets are independent literal contract derivations.
No allocation-error injection seam was added. Budget tests do not measure internal
allocation ordering; actual preallocation and allocation-exception mapping retain
production-source review obligations.

Actual environment: Linux x86-64, GCC 16.2.1, GoogleTest 1.18.0 (pkg-config),
clang-format 23.1.4, Python 3.14.8. Standalone compile/link used:

```sh
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -g \
  -Icore/include tests/acceptance/effective_permissions_tests.cpp \
  core/src/effective_permissions.cpp -pthread -DGTEST_HAS_PTHREAD=1 \
  -lgtest_main -lgtest -o /tmp/subathon-access-tests-author-pee_qklz/effective_permissions_tests
/tmp/subathon-access-tests-author-pee_qklz/effective_permissions_tests --gtest_list_tests
clang-format --dry-run --Werror tests/acceptance/effective_permissions_tests.cpp
```

Final strict compile/link PASS with no warnings; list-only PASS with exactly 30
names matching JSON in order. Compile/list children inherited RLIMIT_CORE=0.
Exact absolute argv/raw logs are retained under that scratch directory in
`compile-final.json`, `compile-final.stdout/.stderr`, `list-final.json` and
`list-final.stdout/.stderr`. Initial compile caught two missing braces around GTest
macro guards; fixed in the owned test source. Initial logs remain preserved;
this authoring compilation diagnostic is not behavioral red evidence.

Inline stdlib inventory/source/field-sum audit PASS: unique names and complete
logical map, no skips/disabled/generic-throw assertions, frozen blobs, and independent
nested B/I facts 93/9→51/3 and failed-key output 53/4, alongside approved global 11/1→9/0,
18/2→26/1 and 12/1→26/1. `python3 tools/check_project.py`,
`python3 tools/progress.py render --check`, formatting and staged whitespace/inventory
checks PASS. No test/foundation/SDK/provider/auth/runtime or marker body execution,
expected-value generation, source/header/contract/schema or shared build/CI/state
edit occurred. Root owns actual CMake/CI discovery, independent review, current
selected environment and FIRST intended red; production ACCESS-002 remains later.
