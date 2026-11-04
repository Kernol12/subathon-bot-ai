# ACCESSTEST-002 minimal inert source — author handoff

2026-11-04. Author: service_reviewer. Dedicated
`task/ACCESSTEST-002-source-author`, isolated
`local-evidence/worktrees/accesstest002-source-author`, initially clean at
`c8cb5f261f8ed85aea240338d9b47ed20a8ceb8a`.

Owns only the new [header](../../core/include/subathon/core/effective_permissions.hpp),
[TU](../../core/src/effective_permissions.cpp) and this note. Root owns the build,
preset/CI, shared state/HTML, pins and PR62. No push/publication/main merge here.
Read root/core/tests instructions and the actual full logical/API documents/reviews.

| Exact approved input | Git blob |
| --- | --- |
| Logical model | `3d82bfa418bb14099ee130fca7a400adca66500b` |
| Concrete API | `dda2793c71299dbe36a321ce3dd5620fecc8b214` |
| Different actual API review | `e007e35b1a0f34ca360dbe18dc77da1c876addaf` |

Actual API approval `5c029d4`/root import `b9e0520` preceded this explicit source
release. This artifact is authored, **NOT independently source-reviewed or RUN**.

| New source | Git blob | SHA-256 |
| --- | --- | --- |
| Header | `edd6ffc25379e5036d5e1622346eaa3f272a1c5c` | `b336b6d20b21eb787eecff72e96c44c3e141d99d9e83af5a8bea1b07ca5f0978` |
| TU | `4cf7bcca19e5ab879b62c70c1521d9f977abfd3b` | `e436c231376b849f3f843ec398a2ecc45b0e251234b301491b20658e62cde9e7` |

The header transcribes the exact approved declaration block: 16 ordered/defaulted
aggregate structs, 14 uint8 enums, one evaluator, private owned decision construction
with defaulted copy/move and borrowed const getter, allocation-free fixed failure,
and a distinct logic_error marker. A whitespace/comment-insensitive comparison of
the complete namespace block matches the actual approved API, not a guessed draft.
The public header and TU use only the permitted standard-library dependencies.

The whole evaluator body is exactly `throw EffectivePermissionsNotImplemented{};`.
Its implementation parameters are unnamed to avoid unused-parameter diagnostics;
the named public declaration is unchanged. There is no validation, allocation
projection, parser, numeric helper, normalizer, permission/source lookup or successful
result. Private move construction, const data/code getters and fixed what() are
inert mechanics only. The failure text and marker text match the API exactly.

Actual compile/link/static commands, with ROOT=this worktree and SCRATCH the literal
`/tmp/subathon-accesstest002-minimal-source-4xrgbu17`:

```sh
clang-format --dry-run --Werror core/include/subathon/core/effective_permissions.hpp core/src/effective_permissions.cpp
/usr/bin/c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -fPIC \
  -I ROOT/core/include -MD -MF SCRATCH/effective_permissions.d \
  -c ROOT/core/src/effective_permissions.cpp -o SCRATCH/effective_permissions.o
/usr/bin/c++ -shared -Wl,--no-undefined SCRATCH/effective_permissions.o \
  -o SCRATCH/libeffective_permissions.so
/usr/bin/nm -C SCRATCH/effective_permissions.o
```

All four exit 0; compile/link stderr are empty. Exact absolute argv, working
directory and raw outputs remain in SCRATCH; commands.json SHA-256
`aba630ba284d19030f66295218732d008d9f1bb4f983a1d8a76d88a6270a206f`.
Child search overrides were removed and RLIMIT_CORE=(0,0). The object defines the
evaluator and inert members, with no main, authored global-initialization function,
Boost/GTest/Qt/SQL symbol or Boost/GTest header dependency. No binary/shared library
was run or loaded. No constructor/getter/feature/marker, SDK arithmetic, foundation
or test body ran. No CMake or additional SDK stage was performed.

Read-only Python source/Git checks confirm exact declarations and sole immediate
throw. Project consistency, unchanged current HTML, staged whitespace and exactly
three new assigned paths are checked before committing. Existing logical/API/
review/fixture/assertion files are unchanged. No test or fixture is authored here.

Next requires codec_author's actual independent review of this source AND root's
actual minimal harness. Later independent fixtures/tests and their actual review,
selected environment evidence, FIRST intended behavioral red and explicit production
release remain required before ACCESS-002 implementation. Root's existing 21-fact
selected SDK foundation was not replayed. No future review, permission enforcement,
baseline completion, current CI or merge approval is claimed.
