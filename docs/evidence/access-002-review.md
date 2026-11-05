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

## Additive regression and harness review — before first invocation

Actual root candidate `cf7fe53`, independently integrated as reviewer `9ad1162d`,
contains final author `3a3dab40` and root harness `a1cc306`. **APPROVED for the
actual two additive tests and isolated harness below.** Coordinator may invoke
these tests and perform relevant full69 acceptance. No future passing result is
claimed by this pre-execution approval.

| Actual artifact | Git blob |
| --- | --- |
| New regression source | `636664385aad978de7ffcd6230ecdd32ce3b60c3` |
| Test CMake | `85fcc130357f2486c4d19b15170af18dee2e4873` |
| Labels helper | `2ef3ab510652348c23a4c800a805da3c18d033f4` |
| Automatic workflow | `072f86343a2a456fd1e7187daac3216025bf485b` |

Nine independently literal long-ratio directions exercise unequal cross-products,
common numerator and denominator cancellation, reversal and negative signs.
Python stdlib Fraction/gcd independently confirms all nine canonicality/direction
oracles and `(10^80+1)^2 - 10^80*(10^80+2) = 1`. Expected permission outcomes are
hardcoded mathematical consequences, never generated by project parsing/matching.
The original30 assertions, fixture, contracts and header remain unchanged.

Actual first candidate81725648 had an unchecked `request.quantities[0]` read;
reviewer requested a caller-level shape gate so missing ownership reports test
failure instead of undefined behavior. Final63666438 has that exact ASSERT before
both indexed reads. Root additionally replaced an arbitrary8192 allocation sweep
ceiling with one unfailed instrumented call measuring actual A allocation points,
then injection at each0..A-1. No expected allocation count or conforming scratch
strategy is imposed. This discovers private fault points, not domain expected
results. The intermediate optional-parameter compilation defect is explicitly
retained in the author note and is not behavioral-red evidence.

The allocator replacement is present only in a separate executable. Constant raw
fixture strings do not invoke the feature. A constinit thread-local state is armed
only around the direct evaluator call/inert optional move/error-code capture;
it fires one bad_alloc at one chosen event and disarms before any GTest/logging/
trace or caller result copy. Global ordinary/array/aligned/nothrow new and matching
delete variants use appropriate malloc/aligned_alloc/free behavior; aligned byte
rounding checks overflow. Each injected trial must actually fire, return typed
allocation_failure and retain no decision. An unfailed call must return a complete
permit; copied retained data survives clearing the caller. Invalid limits and
already oversized input must allocate zero times under a first-event fault.
This is controlled fault simulation, not real-OOM/process/hardware assurance.

The exact two-name enabled/metadata/timeout guard is mandatory alongside the old
thirty-name guard. Regression labels include ordinary effective-permissions so
the same unconditional CTest step executes both; no baseline/inventory weakening,
false red, skip or failure inversion is introduced. Private allocator symbols do
not enter the original acceptance, selected environment or production library.

Before this approval, reviewer independently configured/built the final selected
Boost1.83 candidate (incremental two-step regression compile/link, zero warnings)
and listed69 cases WITHOUT executing either new body. Exact two enabled labels/
timeout discovery and `nm -C --defined-only` confirm replacement operator new is
defined only in the regression executable, absent from original acceptance and
SDK environment. YAML/all nine shell bodies/three embedded Python blocks and
regression clang-format checks pass. Actual first new-regression execution,
independent passing69 and automatic latest-head CI remain to be observed.

### Independent post-approval acceptance receipt

After coordinator imported the actual pre-invocation approval and reported its
first full69 pass at `b4a82b7`, this reviewer ran the same unchanged selected
candidate from clean `b2ab5c8`:

```sh
ctest --preset effective-permissions-debug --no-tests=error \
  --output-junit /tmp/access002-review-native69.xml
```

Direct exit0,69/69 pass,0 skipped, CTest reported0.66s. This includes the
unchanged30 permission cases, both approved regressions, separate selected21-fact
environment and old36 checks. The allocation-fault test passed in0.02s and the
nine-direction rational test passed; no error was rewritten as successful red.
JUnit SHA256 `7b277a6928cc1b52436f7ea218ac480669d2db11f539c8a4a3899cbcadac22fb`.
Source/config/test approvals above remain valid; automatic latest-head CI,
publication/metadata/rules/discussions and normal merge are still separate gates.

## Final component metadata and CI attribution review

Actual root candidate `19ad06114248410a4f531fac9a5ba2404abf9743`, integrated as
reviewer `82965fd8285cef178a9bb352cd9461437c204bc8`. **APPROVED for the actual
completion handoff, architecture/state/map/HTML/checkpoint and evidence receipts.**
No blocking finding. Source/test/config approvals remain applicable: thirteen
approved runnable/interface/contract/config blobs are byte-identical to the
pre-invocation reviewed revision. No behavioral change was carried over unseen.

The authoritative state changes only ACCESS-002 to component done with passing
acceptance/separate review, preserving the original baseline and its observed red.
AUTH-03/AUTH-06 remain partial in the acceptance map, append the new evidence and
explicitly retain current identity/fact loading, real transport/execution/disclosure,
delegation/recovery/editor and platform/live security requirements. Architecture
and handoff accurately describe a pure calculation, not authenticated authority or
a running security boundary. Task done is explicitly distinct from unmerged PR64;
latest-head checks, rules/reviews/discussions and normal merge remain required.
Historical checkpoint stages are followed by the current reviewed/pass stage;
parked work and CONTRACT014's requested corrections remain explicit rather than
being promoted to completed capability. No owner decision/access action is invented.

Independently queried actual GitHub run42066180272: event pull_request,
headSha `b1c7b5447ee9f425fd61dde8b274095c750a9e2d`, completed SUCCESS,
job127380752987 completed2026-11-05T16:37:17Z. Logs identify the normal PR merge
checkout `c4554f1` merging that head into main d2392e2. Read setup/configuration,
discovery, command/output evidence and programmatically checked the complete log
phases: pinned Boost1.83/macro108300, GNU13.3, exact original30/new2 enabled guard,
21 std/Boost primitive checks with0 failures, ordinary32/32 permission processes
passing0.12s and old36/36 passing1.60s. None of those phases has a failed/skipped/
timeout process or failure inversion. Six source/test/config blobs named by that
run equal the actual current reviewed candidate. This supports the handoff's
attributed execution receipt; it does not assert that an unobserved final metadata
head has passed all checks.

The actual restored SDK package is10746098 bytes with SHA256
`747b364a7d578babed9efa4c40446be4ba3470ac5b2c9951b0e3bde3d7bb47d6`, matching
the linked authenticated restore receipt. Both actual retained local69 JUnit
files independently report tests69/failures0/disabled0/skipped0; reviewer receipt
hash still matches the recorded7b277a69 value. Host GNU16 and selected Ubuntu
GNU13 evidence are kept distinct; controlled allocator fault simulation is not
physical OOM/platform proof. Existing numeric/global/live verification is not
upgraded by this component completion.

Actual checks at this metadata revision: `python3 tools/check_project.py` passes
417tasks/122decisions/links/presets/current HTML; `python3 tools/progress.py render
--check` passes; `python3 -m unittest discover -s tests/tooling -v` passes19;
`git diff --check` passes. Additional Git/Python blob/CI-log/JUnit/package checks
above pass. No redundant native69 replay or first-red/foundation rerun was made.
Latest final-head automatic CI and GitHub merge eligibility remain root's next
mandatory gates; this approval neither asserts a merge nor replaces required
GitHub approvals/branch protection.
