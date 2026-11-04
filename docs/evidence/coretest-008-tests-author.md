# CORETEST-008 test author handoff

Author `service_author`, 2026-11-04. Isolated `task/CORETEST-008-tests-author`,
initially clean at release `4c6c02f55f27cb17bb69c0a314d7d6a92b26f8e4`. Only the new
[test source](../../tests/acceptance/timer_editorial_commands_tests.cpp),
[name/map inventory](../../tests/fixtures/timer-editorial-commands-v1.json) and
this note change. **Authored; strict compile/link/list only. No feature/test/marker
body was run.** Actual different full test/harness review and root FIRST red remain.

Frozen logical f4a4c3ce/API a2ac5a2c/header 92281931/CPP 651c68b3 match their full
recorded Git blobs. Root/core/tests instructions and actual source/harness approval
1648acf1/import5f52125 apply; these do not prospectively approve the new tests.
Source blob `3b0c80411e73736925ed2491e12741cbb03aedeb`; inventory
`1a4fb38d1a28b768e138590dca46e8b3dc5e0abc`. Exactly 27 TimerEditorial.* names;
JSON maps every ED01–44 and API sections, without expected values or a fixture reader.

Raw aggregate builders/expected literals cover complete input audit, values,
adjustment/warning fields, statuses/prerequisites, every per-segment/editorial batch
including unused fixed slots, all owner choices and all 44 logical examples. Added
boundaries cover four-event exhaustion, non-exhausting negatives, repayable-debt
rollback on overflow, future/negative-UTC deadline edits, explicit pause segments,
missing policy, strict field/tag/preview/error priority, opaque IDs, deep lifetime/
copy/move ownership and all four profiles/exact B. No old reducer, numeric helper,
SUT factory or prior success supplies setup/oracles. Every raw case calls the sole
projector; unexpected markers/exceptions fail, with per-row traces/nonfatal checks
and safe optional/size/iterator/caller gates.

Linux x86-64, GCC 16.2.1, GoogleTest 1.18.0 via pkg-config, clang-format 23.1.4,
Python 3.14.8. Strict standalone C++20 compile/link PASS without warnings:

```sh
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -g \
  -Icore/include tests/acceptance/timer_editorial_commands_tests.cpp \
  core/src/timer_editorial_commands.cpp -pthread -DGTEST_HAS_PTHREAD=1 \
  -lgtest_main -lgtest -o /tmp/subathon-coretest008-tests-vrkvjrk_/timer_editorial_commands_tests
/tmp/subathon-coretest008-tests-vrkvjrk_/timer_editorial_commands_tests --gtest_list_tests
clang-format --dry-run --Werror tests/acceptance/timer_editorial_commands_tests.cpp
```

List-only PASS: all 27 names match JSON in order. Exact absolute argv, compile/list
exit0 and raw stdout/stderr are retained in that scratch directory's
`compile.json`, `list.json`, `compile.stdout/.stderr`, `list.stdout/.stderr`.
Capture writes raw bytes; decode only for display, never strict-UTF8 log ownership.
Children inherited RLIMIT_CORE=0. Inline stdlib inventory/frozen-input/literal audit
PASS: all 44 ED, no skips/disabled/generic-throw assertions, no primitive oracle
calls; 13 independent size/bound checks use the approved 82/224, 81/92, 93/284
goldens and verify held 83/186 and
two-step 99/246. Formatting, project 406/122, render check and staged whitespace/scope
checks PASS. No dependency/foundation suite was replayed.

No allocator faults/new dependency or deterministic valueless-variant exception was
introduced; preallocation ordering, valueless handling and allocation-failure mapping
retain actual production-source audit obligations. Budget assertions do not measure
internal allocation order. No source/header/contract/schema/shared build/CI/pin/state,
provider/native SDK/runtime-body work, push or merge. Root owns actual preset/CI,
different review and FIRST intended red; production CORE-008 and UI/current-rights/
queue/clock/CAS/start/reopen/mixed-review integration remain later gates. No self or
future approval, completed baseline or product pass is claimed.
