# ACCESS-002 — pure evaluator implementation handoff

2026-11-05, author /root/access_implementer, isolated
`task/ACCESS-002-implementation`, `/tmp/subathon-access002-author`.
Base54d45fd from actual integrated maind2392e2. Owned only
`core/src/effective_permissions.cpp` and this note. Header, specification,
contracts, original30 tests and literal fixture are unchanged.

## Scope and decisions

Implements the reviewed logical/API boundary in
[effective permissions](../contracts/effective-permissions-api-v1.md). The
[actual independently reviewed baseline/red](accesstest-002-handoff.md) already
exists and was not rerun against the unchanged stub. The new implementation uses
resource-scoped individual precedence, one complete grant, exact requested
quantities, owner protection/owner-target refusal, and owned complete deterministic
explanations. No account/session/current-authority/catalog loader, policy editor,
delegation, queue/transport, authentication or protected workflow is supplied.

The evaluator performs the nine approved stages in order. Tag/category checks and
shape/domain passes complete their entire category before any lower-priority
category starts. Borrowed input B/I/string preflight precedes numeric allocation.
Result B/I and each destination container's max_size are projected before the
first output DTO, row, string or vector is materialized. Repeated borrowed scans
use no index scratch or owning policy copies. Failures carry only the fixed
message/typed code. Allocation/length failures from evaluator work map to the
allocation-free allocation_failure exception. Returned data owns all input fields;
copy/assignment initiated later by callers remain outside this boundary.

## Numeric work bound for independent review

Private arithmetic uses selected Boost1.83 `cpp_int` with expression templates off.
Canonical spelling is checked without allocating; denominator1 requires no big
integer. Other canonicality uses binary gcd. Comparison handles sign/zero first,
then computes its conservative G=4*maximum actual component bytes, proven
representable by stage0/1 BEFORE constructing integers. Four magnitude operands
are parsed using scalar multiply10/add digit. Common numerator and denominator
factors are cancelled with binary gcd and explicit exact unsigned long division;
products use shift/add. No general Big multiplication, division, rational-to-float
or unrelated numeric parser/finalizer is invoked.

For positive odd-normalized gcd operands, every subtraction followed by mandatory
right shifting decreases summed bitlength; the checked 2G+1 iteration ceiling is
conservative. Zero is checked before bit operations. Common-factor restoration
never exceeds original G bits. Long division owns three work values and uses at
most G+1-bit shifted divisor, within the already projected2G comparison bound.
Shift/add products remain below2^(2G), with at most G iterations. Every counter is
checked before decrement/growth. At most nine Big values coexist (four operands,
retained first product, and result/shifted/bits of the second product; plus return
materialization conservatively); gcd/exact division use fewer. This is below16,
each within the actual2G<=8D-bit capacity. No vector of parsed rationals is cached.

Actual selected Boost sources `cpp_int/multiply.hpp` scalar limb overload,
`cpp_int/add.hpp` and `cpp_int/bitwise.hpp` in-place scalar/add/subtract/shift paths
were inspected. These primitive paths use limb/carry scalars and bounded backend
resize rather than recursive Big product/division scratch. Dynamic backend limb
capacity/rounding, allocator metadata and growth are additional implementation
overhead, not falsely charged as semantic B or claimed RSS. Independent source
review must verify the stated bound and the actual selected-library paths.

## Observed checks

Host Linux x86-64, GNU16.2.1, CMake4.4.5, Ninja1.13.2, GTest1.18.0.
The first configure selected installed Boost1.92; no feature test ran at that
configuration. After the coordinator restored selected headers, these commands
configured/built/ran the candidate using Boost1.83:

```sh
cmake --preset effective-permissions-debug \
  -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 \
  -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0 \
  -DCMAKE_CXX_FLAGS=-I/tmp/subathon-boost-1.83-sdk/prefix/usr/include
cmake --build --preset effective-permissions-debug
ctest --preset effective-permissions-debug --output-on-failure
```

All commands exit0. Build18 steps, no warnings. Full67/67 pass,0 skipped,
0.62s: unchanged30 permission cases, old33 numeric+3 infrastructure and independent
selected-Boost21-fact environment. Original harness still labels30 intentional-red;
coordinator owns the separate ordinary-CI/label promotion and explicit private
Boost target linkage, after which this local include override should not be needed.
`clang-format -i core/src/effective_permissions.cpp` and `git diff --check` pass.
No original test/fixture/header was modified or generated from implementation.

## Remaining gates

Actual independent source/allocation/arithmetic/config review is pending. The
original suite covers unequal small fractions and equal large values; a focused
independent unequal long-rational/cross-cancellation regression is useful for the
new bounded arithmetic paths. Current selected Ubuntu automatic feature execution,
coordinator integration checks, task completion metadata, latest-head PR checks,
rules/reviews/discussions and normal merge remain. No production authority,
platform/live verification or full security/release completion follows from these
component passes. No provider/chat/payment/deployment side effect occurred.
