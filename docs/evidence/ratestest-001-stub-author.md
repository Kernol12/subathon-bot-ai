# RATESTEST-001 minimal C++ interface author handoff

Author: codec_author, 2026-11-04. Branch `task/RATESTEST-001-stub-author`, isolated
worktree `local-evidence/worktrees/ratestest001-stub-author`, actual starting head
`3496ee5` after main `07dc53e` and the coordinator's approved rate prerequisites.
The coordinator released this source stage after independent API approval
`0b9a449` / integration `6b9a217`. That approval covers the document only; this
author cannot independently approve the new source written here.

Owns only these three new paths:

- [rate_snapshot.hpp](../../core/include/subathon/core/rate_snapshot.hpp).
- [rate_snapshot.cpp](../../core/src/rate_snapshot.cpp).
- This author evidence note.

Root owns all CMake/preset/CI/state/HTML/schema/pin/checkpoint/publication changes.
No existing source, contract, assertion, fixture, other worktree or shared harness
file is edited. Prior API/model reviews and locale work remain preserved.

## Exact interface and deliberate missing behavior

Read root/core instructions, the approved logical model and API, accepted API
review, existing Exact/ExactDecimal and ReceiptMoney public headers, and the
root architecture/workflow/test-first boundaries. The unchanged authorities are:

- Logical model blob `76d2d9b3e542c56b7475e99defd717d9902885a5`.
- Concrete API blob `556840c2827b9c1e9a29c3ec9c378a86a31d4118`.

The header supplies exactly the declared owned aggregates, enums and limits, four
final result wrappers with private data-taking construction, public defaulted
copy/move operations and const noexcept data access. Each wrapper owns one data
aggregate. No result has a public default/data constructor or mutable accessor;
raw input aggregates are not validation or authorization certificates. Only the
corresponding feature entrypoint is a friend of each wrapper.

The error constructor retains its supplied enum and fixes what() to
`Invalid rate snapshot input`. The distinct RateSnapshotNotImplemented constructor
has the exact message:

```text
Rate snapshots are intentionally unimplemented; review tests first
```

Each of the four feature definitions has exactly one statement:

```cpp
throw RateSnapshotNotImplemented{};
```

It is unconditional and first. Definition parameters are unnamed to avoid unused
warnings; public declarations retain the approved names, order and default limits.
There is no argument validation, parser call, positivity/normalization/bit/gcd
calculation, successful result construction or call to another feature. Header
dependencies are the standard library plus existing core ReceiptMoney/Exact;
no Qt, SQL, UI, provider, filesystem, network or new numeric dependency is added.

## Actual checks

These establish source/interface compilation only. No feature, exception marker,
factory, query, test body or native executable was invoked.

Environment actually read: GCC 16.2.1 (20260810), installed Boost 1.92.0 headers
(`BOOST_VERSION=109200`), clang-format 23.1.4. No dependency was installed.

```sh
clang-format -i core/include/subathon/core/rate_snapshot.hpp core/src/rate_snapshot.cpp
clang-format --dry-run --Werror core/include/subathon/core/rate_snapshot.hpp core/src/rate_snapshot.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -Icore/include -c core/src/rate_snapshot.cpp -o /tmp/subathon-rate-snapshot-stub-author.o
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All commands exited 0. The object compilation emitted no warnings. Formatting
uses the repository .clang-format. The project check reports 398 task records,
122 decisions and valid local links/presets/current HTML; its old draft-test
notice does not establish a feature test result. The generated progress snapshot
is current. No CMake/build configuration is edited or claimed exercised here.

A read-only Python source-text comparison, importing no project module, used
balanced braces to compare complete declarations against the API: all 19 aggregate
field lists, three enum lists and 13 limit fields/defaults match exactly. It also
checked that each complete feature body contains only the immediate marker throw.
This is an author consistency check, not independent source review or a test run.
Final staged-path and whitespace checks accompany the commit handoff.

| Artifact | Git blob | SHA256 |
| --- | --- | --- |
| Header | `3a06ff18d29c3bd2c5953e6747130aa84cbee8f5` | `f45ed456793c1589cbdea4b0124f32d94d97e0ee8f0975f845d84a496f60f7c4` |
| Stub source | `4cf1c1ed6b54efe3625baf97903de19d3ebacf8d` | `1faa791e19d66692763d1cd9efb4207ee359723b577ef6ddd16cdfdb13220b88` |

## Remaining gates

Different actual minimal-source/harness review precedes fixture/test authorship.
The selected dependency foundation must be green without invoking these four
entrypoints. Independently authored literal tests require separate review before
the first observed named-marker behavioral red. Compilation, header presence,
missing dependencies, skipped tests or catching a marker as success are not that
red and do not authorize production implementation.

RATESTEST-001 remains incomplete; RATES-001 production remains planned. The required
RATESTEST-009/RATES-009 finite-decimal valuation/capture/codec/durable bridge remains
planned. No recurring-rational rounding fallback, provider capability, usable award
adapter, operational or Linux/Windows release verification is supplied by this
minimal interface. Root records actual integration, review, CI and subsequent
stage releases separately.
