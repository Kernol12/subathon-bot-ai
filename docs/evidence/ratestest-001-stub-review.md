# RATESTEST-001 independent minimal source and harness review

**Verdict: APPROVED for the actual minimal source/harness stage at f5afb60.**
Reviewer service_reviewer, different from stub author codec_author, API/model author
service_author and shared harness integrator root, 2026-11-04. Ownership is this
new review note only. Earlier logical/API review notes remain unchanged.

Actual candidate: author f75d7cc, source import3d01e9a, shared harness a9c3b61 and
preset encoding correction f5afb60e0fc65acafa5aa1a05f509a9a10c6b242. Merged the
actual candidate cleanly into the existing isolated rate review branch, preserving
its earlier7003473/0b9a449 review history. Exact reviewed artifacts:

| Artifact | Git blob |
| --- | --- |
| [Header](../../core/include/subathon/core/rate_snapshot.hpp) | 3a06ff18d29c3bd2c5953e6747130aa84cbee8f5 |
| [Stub source](../../core/src/rate_snapshot.cpp) | 4cf1c1ed6b54efe3625baf97903de19d3ebacf8d |
| [Author note](ratestest-001-stub-author.md) | 3a033591cd52afdb3f6f888bd0817ca0f837bc05 |
| Root CMake | 07b8a50dd66d0c8dc7d250763e355c026b9466d2 |
| Core CMake | d87b834e72c3131aba34ab43718b8e79df65c795 |
| Presets | dcdb2e4d40977a96c511ac8459c2b6f260005465 |
| Rate workflow | cff7435623f2f1d1b714be5a98fdca6ea9b5f515 |
| Existing GoogleTest dependency selection | ecedcf7a9f7084d9af7d5f12b9cca2612283fb39 |
| Approved model | 76d2d9b3e542c56b7475e99defd717d9902885a5 |
| Approved API | 556840c2827b9c1e9a29c3ec9c378a86a31d4118 |

## Source findings

Read root/core AGENTS, the complete approved model/API and their review notes,
existing Exact/ExactDecimal and ReceiptMoney interfaces, full new header/source,
author evidence and actual shared harness diff. Authority remains specification
v0.45 sections3.5/9/15, NUM-04/FX-06 and those approved contracts. No substantive
source or harness correction is required for this bounded stage.

Independently compared all19 aggregate field lists to the API, plus all13
RateSnapshotLimits fields/defaults and all three enum lists/order. A read-only
balanced-brace/whitespace-normalized declaration audit confirms exact matches;
source inspection checks signatures, argument order, default limits and variant
alternative order. Types use owned std::string/vector/optional, the existing
Exact/ExactDecimal and ReceiptMoney, with no added numeric or framework dependency.
The optional raw/instant time fields remain independent data.

All four result classes are final, own exactly their specified private data
aggregate, expose only const noexcept data access and have the approved public
defaulted copy/move operations. They have no default or public data-taking
constructor and no mutable accessor. Each private constructor has only its matching
entrypoint as friend. Constructing ordinary input/data aggregates validates nothing.

RateSnapshotFailure retains its enum and fixes what() to Invalid rate snapshot
input. The distinct RateSnapshotNotImplemented derives from logic_error and has
the exact approved message. Every complete feature body contains only
throw RateSnapshotNotImplemented{}; as its unconditional first/only statement.
There is no validation, parser/helper invocation, arithmetic, graph work, output
projection, successful value construction or call to another feature. This was
checked by full source inspection and exact complete-body matching, not by
invoking the marker. Inline getters/copy/move/private data ownership are interface
definitions, not an implementation of the model. No static initialization calls
these entrypoints. Qt, SQL, provider, network, clock, UI and filesystem code are
absent from this new core boundary.

## Harness and independent checks

The new option enables the existing exact-numeric dependency target; the new
subathon_rate_snapshots static library compiles the actual TU and publicly links
subathon_exact_numeric. It uses existing include paths/C++20/warning options.
Profile23 rate-snapshot-debug inherits exact-numeric-debug. The final diff and a
JSON comparison prove all previous preset entries retain their original values
and encoding, including the22 existing visible profiles and hidden base; only
the new profile is appended.

Configuration/build/discovery executes no new rate function or marker. The actual
CTest inventory has33 existing numeric cases and three infrastructure checks, with
no new rate command, disabled case or WILL_FAIL inversion. Existing numeric test
executables link the existing numeric library, not a successful rate substitute.
The workflow uses the existing immutable checkout pin, exact Ubuntu Boost1.83 apt
version/header/config checks and existing GoogleTest selection/fetch pin. It builds
the stub target and runs only that existing36-case inventory. No hidden generation
step, blanket continue-on-error or pretend new feature acceptance exists.

Actual independent commands in this isolated review worktree:

```sh
cmake --preset rate-snapshot-debug
cmake --build --preset rate-snapshot-debug
ctest --preset rate-snapshot-debug --show-only=json-v1
ctest --preset rate-snapshot-debug --output-on-failure
clang-format --dry-run --Werror core/include/subathon/core/rate_snapshot.hpp core/src/rate_snapshot.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All pass. Fresh configure selects GNU16.2.1, installed Boost1.92.0, GoogleTest1.18.0
and Python3.14.8. Build completes14 steps with zero compiler warnings. Discovery
assertions verify exactly33+3, enabled/uninverted, with no rate entrypoint command.
CTest36/36 passes in0.69 seconds. Project consistency reports398 task records,
122 decisions, links/presets/current HTML; render check is current. Static declaration,
body and prior-preset preservation checks also pass. These are source/dependency
foundation checks; no new feature, exception marker, factory or query is invoked.
Installed local Boost/GTest differ from the workflow's selected environment; a new
selected CI result is not claimed by this review.

## Remaining gates

This exact minimal source/harness may integrate. Root retains current selected CI,
shared state/checkpoint/publication and subsequent stage release ownership. The
next independent literal fixtures/tests need their own actual review and selected
green dependency foundation before FIRST ordinary named-marker red. Header presence,
successful compilation, existing numeric passes, a skipped test or expected marker
catch cannot substitute for that behavioral baseline. No future test or production
source is approved here.

RATESTEST-001 remains incomplete and RATES-001 unimplemented. The required
RATESTEST-009/RATES-009 recurring-rational valuation/capture/codec/durable bridge,
actual provider behavior, cache/update/pin authorization, historical/UI and
Linux/Windows release verification retain their separate gates. No usable credit
adapter, supported provider capability or finite-decimal fallback is introduced.
