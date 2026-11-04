# MILESTONETEST-001 minimal interface/stub author handoff

Author: service_author, 2026-11-04. Dedicated
`task/MILESTONETEST-001-contract-author`, isolated worktree
`local-evidence/worktrees/milestonetest001-contract-author`. The coordinator released
this stage in claim `9edebb9` after actual API approval `b8932cd` / integration
`21ba96d`. Inspected a clean tree, then normally merged actual prerequisite branch
`task/MILESTONETEST-001-progression-baseline` as `7a49b10`, preserving prior work and
independent reviews.

Owns only the new [header](../../core/include/subathon/core/milestone_progression.hpp),
[source](../../core/src/milestone_progression.cpp) and this note. Root owns all
shared build/presets/workflows/pins/state/HTML/maps/schema/checkpoint integration.
No fixture, test, harness, root configuration or approved contract is edited.
Root/core instructions and the actual approved declarations were read first.

## Exact candidate and inert boundary

- Header blob: `9a1678137ba6f85cbfbb545630f08d36a261790b`.
- Source blob: `4e3291f0e832237470a8cd0bc349e0106ad8fd77`.
- Frozen logical model remains `0115a5cc3cdc4fb1ebb8c9739716a27cb3379933`.
- Frozen API remains `58b8e129815c0525176cc6c1b6830a7e6f0a9fdb`.

All eight aggregate field types/order/defaults and both enums match the approved
API. The result privately owns one data aggregate with const access and defaulted
public copy/move operations; it has no default/public data constructor. Failure
construction stores its code with exactly `Invalid milestone progression input`.
The distinct marker constructor has exactly the approved text:

```text
Milestone progression is intentionally unimplemented; review tests first
```

There is one advance_milestone_progression definition. Its ENTIRE body is:

```cpp
throw MilestoneProgressionNotImplemented{};
```

Its definition omits parameter names to avoid unused-parameter warnings without
adding statements. No helper, validation, successful result, arithmetic, source
lookup, permission or state/action work precedes the throw. Getters and ordinary
owned-data/exception construction are inert declarations, not progression behavior.
No feature or marker constructor/body was invoked during this stage.

## Actual compile-only checks

Environment: Linux x86-64; GNU C++ 16.2.1 (20260810), selected host Boost headers
1.92.0 (`BOOST_VERSION=109200`), clang-format 23.1.4 with repository configuration.
This is local compilation, not selected CI or a Python/platform/provider claim.

- `clang-format -i core/include/subathon/core/milestone_progression.hpp core/src/milestone_progression.cpp`
  and corresponding `clang-format --dry-run --Werror ...` — PASS.
- `c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Icore/include -fsyntax-only core/src/milestone_progression.cpp`
  — exit 0, no diagnostics.
- `c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Icore/include -c core/src/milestone_progression.cpp -o /tmp/subathon-milestone-progression-stub.o`
  — exit 0. This emits only a relocatable object; nothing was linked/executed.
- Compile-only stdin inspection with
  `c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Icore/include -x c++ -fsyntax-only -`
  — exit 0. Static assertions inspect the eight aggregates, exact defaults, result
  privacy/copy/move/finality, const/noexcept accessor, exception bases/finality,
  Exact member type and entry signature/default argument. All call expressions
  occur in unevaluated decltype/noexcept/requires contexts; no feature executes.
- A read-only inline standard-Python text comparison checked all ten aggregate/
  enum declarations against the frozen API, ignoring whitespace, and verified the
  source's sole function body is exactly the single throw above. PASS; no project
  module/feature was imported or called and no expected fixture was generated.
- `python3 tools/check_project.py`, `python3 tools/progress.py render --check`,
  and `git diff --check` — PASS: 396 task records, 122 decisions and current
  links/presets/HTML. Staged owned-path/whitespace checks accompany the commit.
  These establish infrastructure/document consistency only.

The next gate is different actual source AND coordinator-owned build/harness
review. Independent feature tests/fixtures, their review, selected green foundation
and FIRST intended behavioral red have not occurred. Production MILESTONE-001
remains planned. No progression behavior, normal feature success, native test run,
action delivery, authoritative write, provider request or credentials operation is
claimed. There is no push, PR creation or main merge by this author stage.
