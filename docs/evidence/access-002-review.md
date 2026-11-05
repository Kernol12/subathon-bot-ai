# ACCESS-002 independent source and integration review

2026-11-05, reviewer `/root/access_reviewer`, different from source author
`/root/access_implementer` and shared harness author `/root`. Actual clean review
revision `2524b5a` contains root candidate `399fd58`, author source `891e851` and
comment-only correction `fbe0f005`. Only this review note is authored here.

**APPROVED for the actual pure evaluator and shared build/CI promotion below.**
No blocking defect found. This is source/config approval, not future regression
approval, unobserved Ubuntu CI success, merge approval or running-server authority.
Original independently reviewed tests and first-red evidence remain intact.

## Actual artifacts and boundary

| Artifact | Git blob |
| --- | --- |
| `core/src/effective_permissions.cpp` | `b7b81e9327ef8840effecb8319425b1a397bf8af` |
| Frozen public header | `edd6ffc25379e5036d5e1622346eaa3f272a1c5c` |
| Original thirty-test source | `fc82d17ede5d2afe66e42572c17762510e5682f4` |
| Original literal inventory | `d0c25108ec233cdc11260aa1263974805b0fba15` |

Read the complete logical model/API, header, original1385 test lines/inventory,
owner decision and baseline handoff against specification v0.45 §10/AUTH-03/06.
The implementation changes only the pure evaluator; no request authenticates
itself, trusts a cached permit, writes policy/state or invokes transport/SQL.
Current session/owner/catalog/policy loaders, execution/disclosure rechecks,
delegation, protected workflows and platform/security integration remain required.

The complete-input passes implement stage0–9 in order. Category-wide tag/shape/
number/domain passes retain priority across input permutations and irrelevant
entries. Unknown facts differ from known empty collections; only the actual
owner's narrow protected query admits unknown policy. Every number is canonical
before any domain error can win. Invalid opposing individual scopes are detected
before quantities and without Cartesian expansion. Source/grant consistency,
complete named tuples and exact units prevent fragments or quantity fallback.

Requested quantities select complete grants. One whole resource tuple selects
individual precedence even when its quantity test fails; role denies beat role
allows only outside that scope. Protected owner and helper-reset target guards
produce explicit virtual sources. All rows, failed keys, suppression, decisive
denials and the stable sole witness follow the frozen unsigned-byte total order.
Owned request/explanations retain input lifetime independence.

## Allocation and arithmetic audit beyond the baseline

Borrowed `Count` visits every input field using checked remaining-byte/item sums,
including optional tags, masks, losing rows and nested sets. Empty identities are
remembered until their designated error stage. No input DTO clone, sorted owning
policy, scratch index vector or numeric conversion precedes this preflight.
Stage0 checks every positive/representable profile field and all three derived
work-size formulas before visiting input. Container/string max_size checks are
explicit. Allocation-free typed failures carry fixed text only.

Resolution and complete output B/I projection use borrowed scans. Before the first
owned output DTO/copy/vector/row, projection includes duplicated context/request,
all source rows, virtual source, failed-key strings, optional witness bytes and
all destination vector max_size checks. Materialization starts afterwards;
sorting moves owned values, and row insertion moves rather than clones the row.
No partial permit or truncated explanation is returned. Evaluator-local
`std::bad_alloc` and `std::length_error` map to `allocation_failure`; caller copies
remain outside that boundary. The public baseline cannot alone prove ordering or
inject every allocation site; these conclusions include actual source inspection.

Numeric work has expression templates disabled. Each operation derives
G=4*maximum actual component bytes before constructing its integers; stage0/1
prove G/2G/2G+1 representability and 2G<=8D. Canonicality uses borrowed spelling
checks and at most two parsed magnitudes. Binary gcd strips powers of two and
normalizes positive operands: each subtract/mandatory shift decreases summed
bitlength, so the checked2G+1 counter is conservative. Common-factor restoration
cannot exceed the original magnitude. Exact long division owns moved remainder,
shifted divisor and quotient, with <=G+1 divisor bits inside preflighted2G.
Shift/add multiplication's partial sums and shifted operand are bounded by2G;
its bit counter is checked. Sign/zero precede comparisons and common numerator/
denominator factors are cancelled before products. No floating conversion,
rounding, signed abs, generic rational normalization or unrelated finalizer limit
appears.

Inspected the actual selected Boost1.83 `number.hpp`, `cpp_int.hpp`, scalar
`cpp_int/multiply.hpp`, `cpp_int/add.hpp`/`add_unsigned.hpp` and
`cpp_int/bitwise.hpp`. The used scalar multiply10/add, positive subtract and
in-place one-bit shifts use limb/carry operations and bounded backend growth;
they do not invoke generic Big multiplication/division or Karatsuba scratch.
The cpp_int precision traits avoid variable-precision expression temporaries.
At most nine magnitude values coexist conservatively, including return
materialization, below the approved sixteen; each stays inside its projected
2G magnitude bound. Backend limb rounding/growth, transient old/new limb buffers
and allocator metadata are additional bounded overhead, not semantic B or an RSS
claim. No cache holds every converted policy value. The author's product comment
was corrected to count result plus two scratch values, without behavior change.

## Shared harness and actual checks

Reviewed actual `ed30e98` changes: explicit private `Boost::headers` dependency and
standalone-option discovery, acceptance descriptions, removal only of
`intentional-red`, exact enabled thirty-name/metadata guard retained, and
unconditional ordinary selected-Ubuntu permission execution. The pinned package,
compiler macro check, separate21-fact SDK environment and old36 remain mandatory.
No assertion, fixture, source identity or timeout was weakened. Stable job name,
permissions/read scope, checkout pin and trigger coverage remain. YAML parses;
all nine shell bodies pass `bash -n`; all three embedded Python blocks compile.
An initial reviewer syntax-check script indentation error was corrected; it was
not a project/build/behavior failure or red-baseline result.

Independently ran from this isolated reviewer tree:

```sh
cmake --preset effective-permissions-debug \
  -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 \
  -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset effective-permissions-debug
ctest --preset effective-permissions-debug --no-tests=error
```

All exit0: fresh18-step build/no warnings,67/67 pass/0 skipped/0.65s, consisting
of original30 permission cases,33 existing numeric,3 infrastructure and the
independent selected environment. No manual CXX include override is used. Ninja
dependency inspection finds521 Boost headers for the actual evaluator object,
all under the selected1.83 prefix. Exact30 enabled inventory, labels/timeouts,
`clang-format --dry-run --Werror`, project consistency, current progress rendering
and whitespace checks pass. Host Linux x86-64/GNU16.2.1/CMake4.4.5/GTest1.18.0;
this does not substitute for automatic Ubuntu validation.

Additive unequal-long-rational and allocation-injection tests are being considered
by the coordinator; their unwritten source is not approved by this note. Actual
separate test review before first invocation, relevant integration acceptance,
latest-head Ubuntu/all applicable CI, task metadata/PR rules/reviews/discussions
and normal merge remain. Full server/account/delegation/disclosure, provider/live,
Windows/OBS and release verification remain outside this pure component approval.
