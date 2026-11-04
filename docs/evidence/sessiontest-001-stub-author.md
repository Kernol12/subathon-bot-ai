# SESSIONTEST-001 inert password source author evidence

Author: service_author, 2026-11-04. Dedicated
`task/SESSIONTEST-001-stub-author`, worktree
`local-evidence/worktrees/sessiontest001-stub-author`, initially clean at minimal
source release `9171d49c6df277bdf49191ad53e57484173e45ba`.
Owns only the new public header, source and this note. No push/PR/main merge,
shared build/CI/pin/state/HTML, schema, contract, fixture or test edit is performed.

## Exact candidate and preserved authority

| Artifact | Git blob |
| --- | --- |
| [Public header](../../server/include/subathon/server/password_hashing.hpp) | `ec3c5cf87afe406db8501053321598599325a239` |
| [Inert source](../../server/src/password_hashing.cpp) | `eaf56f41bafd502b8e4cb38c0de3082180bffcd5` |
| [Approved API](../contracts/password-hashing-api-v1.md), unchanged | `7ad1e8d3f3311673dd368f2ef4d297de31dbafcd` |
| [Approved logical contract](../contracts/password-hashing-v1.md), unchanged | `bc9ccbf66a41c9ae69abde4bd905401900e17461` |
| [Different API review](sessiontest-001-api-review.md), unchanged | `8107778d5875497ba11f6694a13638faf41796b9` |

API approval `18e1a109` / root import `503a7d3` releases this minimal stage only.
Read root/server AGENTS, architecture/workflow/test strategy, FIRST prompt, task
claim/checkpoint and actual approved API/logical/review. The frozen declarations,
aggregate field order/defaults and enum order are preserved; forward declarations
resolve references without adding public constructors or feature entrypoints.

The standard-only public header exposes raw PasswordBytes, options, exact result/
source DTOs and enums, fixed exceptions, the noncopyable/nonmovable abstract login
port, private final source borrow and privately constructed owned result. The borrow
has only its specified credential pointer, attempt view and source pointer; only
propose_password_upgrade is its friend. Its accessors return the existing borrows
or source-object identity. The result has exactly one owned data member, const
accessor and the approved public defaulted copy/move/destructor operations.

Constructors only initialize the specified fields or fixed std exception text.
No constructor/accessor validates input, initializes sodium, creates a verified
source on behalf of a caller, accesses a port or performs credential/login policy.
No sodium/Qt/SQL/UI type or header appears. GPL-3.0-or-later notices are present.

All THREE feature definitions have exactly this entire body:

```cpp
throw PasswordHashingNotImplemented{};
```

They perform no validation, password pointer preparation, port call, parser,
comparison, protected allocation, native initialization, RNG, hashing, verification
or successful result construction. Inert borrow mechanics do not implement an
accepted-login/current-source check or authorize persistence. No plaintext/source
data was supplied to an executed feature.

## Actual compile-only checks

Host Linux x86-64; `/usr/bin/c++` reports GCC 16.2.1 20260810;
`/usr/bin/clang-format` reports 23.1.4. Python infrastructure checks use 3.14.8.
Only the host standard C++20 library is needed. No selected sodium SDK, library
link or runtime metadata probe is used for this source stage.

Working directory for the following commands is this isolated repository tree.
The shell variable below merely names the actual temporary audit directory:

```sh
audit_dir=/tmp/subathon-sessiontest001-stub-author.AUCPyN
clang-format -i server/include/subathon/server/password_hashing.hpp server/src/password_hashing.cpp
clang-format --dry-run --Werror server/include/subathon/server/password_hashing.hpp server/src/password_hashing.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror -Iserver/include -fsyntax-only server/src/password_hashing.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror -Iserver/include -c server/src/password_hashing.cpp -o "$audit_dir/password_hashing.o"
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror -Iserver/include -fsyntax-only "$audit_dir/declarations.cpp"
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror -Iserver/include -c "$audit_dir/declarations.cpp" -o "$audit_dir/declarations.o"
python3 "$audit_dir/inspect_source.py" "$audit_dir"
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All four compiler invocations passed with zero warnings; no executable was linked
or run. The temporary declaration unit contains no main or dynamic initializer.
Its 61 compile-only static assertions cover exact raw/function/member types and
default arguments, scalar defaults, aggregate shapes, exception bases/distinction,
inaccessible private construction, final/private-borrow restrictions, port abstract/
copy/move properties, owned-result copy/move and const/noexcept accessors. Function
expressions appear only inside unevaluated decltype; no feature/marker executes.
Declaration audit SHA256:
`c735beff5b953da4e12ae8961fec7e27fd20d975c4a34d51f52bb901acd2ff37`.

The Python source inspection checks exactly three whole-body immediate throws,
the standard-only include inventory and exact safe exception text. It uses
`nm -u` to read the compiled object's undefined symbols; none names sodium,
Argon2/crypto_pwhash, Qt or SQLite. This is static inspection, not a runtime/link
probe or selected-library identity proof. Inspection script SHA256:
`017944a01fba89e29e7fd697f87356855b419492efdef7067acd854096cd8154`.
An initial inline audit had an overescaped regular expression in its final
declaration-file check and raised re.PatternError. Corrected the audit only and
reran successfully from the recorded script; source/expectations did not change.
That tooling error is not project behavior failure or an intended red baseline.

Formatting verification passed. check_project passed 401 task records/122 decisions
and local links/presets/current HTML; progress render --check passed without editing
HTML. Exact frozen blob checks and the sole-owned-path staged whitespace/inventory
check accompany the commit. No feature, marker, constructor runtime probe, native
password/initialization/RNG/guarded-memory function, foundation, project test, account
login or live/provider operation was invoked.

## Remaining gates

Different actual minimal-source/harness review is next; this author provides no
self-approval. Root owns shared harness/build/CI/pins and stage releases. Reviewed
independent fixtures/tests and a standalone selected-library foundation must follow;
the actual selected foundation must be green before FIRST intended feature-marker
red. Dependency/compile/setup/lock failure or skips never establish that baseline.
Production SESSION-001, actual account-source/conditional-save integration, admission/
cleanup, platform packaging and supported-hardware cost measurements remain required.
This source intentionally implements none of those behaviors or security claims.
