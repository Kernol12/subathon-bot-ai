# SESSIONTEST-001 — Independent minimal source and SDK import review

Reviewer: `/root/service_reviewer`, different from source author
`/root/service_author` and harness author `/root`. Date: 2026-11-04.
Dedicated `task/SESSIONTEST-001-minimal-review`, isolated worktree
`local-evidence/worktrees/sessiontest001-minimal-review`, initially clean at
`64a924c`. Owns only this new review note; no source, contract, fixture, test,
dependency, build/CI, schema or state/HTML file is edited.

## Actual revision and verdict

Reviewed integrated candidate `64a924c2869eef4b0badba0d9c4e2e7fbd232540` following source author
`d34f2d000ad74c13cd07cd54bd8923bd5b69d047` / import `c7d4c45`.
Actual approved API review `18e1a109` / import `503a7d3` precedes minimal-source
release `9171d49`; this is a new review of actual source/configuration, not approval
inherited from the document stage.

| Actual artifact | Git blob |
| --- | --- |
| [Public header](../../server/include/subathon/server/password_hashing.hpp) | `ec3c5cf87afe406db8501053321598599325a239` |
| [Source](../../server/src/password_hashing.cpp) | `eaf56f41bafd502b8e4cb38c0de3082180bffcd5` |
| [Source author note](sessiontest-001-stub-author.md) | `1465014f0a85bfb880b53b69f037d7bf1cae0a5e` |
| Root CMakeLists.txt | `0cf8d84bcb0417c173602a78522dd2c3c88e620d` |
| CMakePresets.json | `e3759dbceddd4f1424d5ac63da743b4ce285531c` |
| [SDK importer](../../cmake/SodiumDependency.cmake) | `bdbe0d6d7268a7231228ec96a040f59e8686108d` |
| Server CMakeLists.txt | `f59247d65fa5396d19e7873d65d386ea825efd27` |
| Tests CMakeLists.txt | `3a43ca0938347f617c32084f332531b48fefcb05` |
| [Harness evidence](sessiontest-001-interface-harness.md) | `266bdef0e7a5fdc9564fae9d59b5b2b762dfeddd` |
| [SDK receipt](sessiontest-001-sdk-build.json) | `8609f0c8cc44ea5f0a96f300f8ccb1aa65e1c5e7` |

**APPROVED for this exact minimal source and local SDK import harness stage.** No
blocking defect found. Frozen logical `bc9ccbf66a41c9ae69abde4bd905401900e17461`
and API `7ad1e8d3f3311673dd368f2ef4d297de31dbafcd` remain byte-identical.
This approves no feature behavior, selected native runtime, later test/foundation,
new workflow or production implementation. It does not authorize baseline merge
without required current CI and all remaining baseline gates.

## Source and compile-time audit

Read root/server/tests instructions, the complete header/TU, approved API/logical
contracts and their different reviews, actual SESSION-001 brief, harness diff and
dependency evidence. Header includes only its seven declared standard-library
headers. GPL-3.0-or-later notices, namespace and project formatting are preserved.

All options/defaults, five enum lists/order, aggregate fields/order/defaults,
function signatures/default arguments and fixed exception text match the API.
PasswordLoginSource is abstract, noncopyable/nonmovable with protected construction
and virtual destruction. PasswordVerifiedSource is final, cannot be publicly
constructed/copied/moved, has only the three specified private members, and grants
construction only to propose_password_upgrade. Accessors expose the existing
immutable borrow and source pointer identity, not validation or login authority.
PasswordUpgradeResult has only its owned data member, private data constructor,
const accessor and approved public copy/move/destruction operations. No mutable
accessor or public verified/result factory is added.

All three feature bodies consist ENTIRELY of one immediate
`throw PasswordHashingNotImplemented{};`. No parameter validation, source callback,
password copy, allocation/protection, initialization, parsing, crypto or success
precedes it. Other constructors/getters only initialize/read approved fields, move
owned data or initialize fixed standard exception text. No module global object or
initializer executes a constructor, native function or marker.

Independent temporary declaration unit:
`/tmp/sessiontest001-minimal-independent-declarations.cpp`, SHA256
`d181587fe0c3a240f56818de61fbf8b217ab6ea98dd44195296a83bfebf0ace5`.
Its 47 static assertions verify raw/member/function types, aggregates, option
constants, inaccessible constructors, abstract/final/copy/move restrictions,
exception distinction, const/noexcept getters and default arguments. Function
expressions are unevaluated; there is no main or dynamic initializer. Nothing from
the password module was invoked at runtime.

## SDK import and evidence partition

The default-off option enables a framework-free server static target and preset 25
password-hashing-debug. It declares private linkage to the explicit global imported
Sodium::Sodium target. The importer requires Linux x86-64/64-bit, a supplied prefix,
the exact include/static archive paths, and exact 1.0.22/ABI 26/4 header metadata plus
Threads. There is no system/PATH/pkg-config fallback, download or native library
invocation. No machine-specific prefix is committed in presets; all previous
configure/build/test preset entries remain unchanged. GoogleTest setup uses the
existing explicit dependency policy, with fetching OFF in this actual build.

The archive is only declared as a static dependency: archiving this stub module
does not link libsodium into an executed consumer or prove its ABI. The public
source compiles without sodium headers/calls. Header metadata and symbol presence
do not authenticate an arbitrary prefix. Root's authenticated source/build record,
later actual selected consumer link/runtime foundation and CI remain necessary.

Read actual [source provenance](sessiontest-001-dependency-preparation.md), receipt
and `/tmp/subathon-sodium-1.0.22-sdk/SDK_ENVIRONMENT.md` with raw audit records.
Independently rehashed the source archive against recorded authenticated SHA256
`adbdd8f16149e81ac6078a03aca6fc03b592b89ef7b5ed83841c086191be3349`, and all three
SDK receipt artifact sizes/hashes. SDK static archive, 836744 bytes, matches
`959cdbe83cd0c80264e080efdd1e861b20b57ded2a85bd9a924297b8a10f1935`.
Read-only tar/hash comparison confirmed all 678 vendor files match the archive with
no extra source files. No extraction, authentication replay or vendor build ran
in this review.

Vendor evidence is explicitly NOT strictly compile-only: configure ran three native
C conftest programs; its SIGABRT probe exited 134 with a core-dump report and warning.
External systemd retention remains uninspected, not certified absent. The eight
Libtool convenience-library warnings are present in the raw build log, distinct
from this review's zero-warning project build. Those generic configure probes are
not password/library runtime evidence or product red. Future root-owned configure
runs must set child core limits and address the recorded host handling; no vendor
patch/cache-answer/security-disable workaround is approved here.

## Actual independent checks

Linux x86-64, GCC 16.2.1, CMake 4.4.5, Python 3.14.8, clang-format 23.1.4.
Installed GTest selected at `/usr/lib/cmake/GTest`, actual version 1.18.0; 1.17 is
only the existing minimum. Commands from the isolated worktree:

```sh
clang-format --dry-run --Werror server/include/subathon/server/password_hashing.hpp server/src/password_hashing.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror -Iserver/include -fsyntax-only /tmp/sessiontest001-minimal-independent-declarations.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror -Iserver/include -c server/src/password_hashing.cpp -o /tmp/sessiontest001-minimal-independent-password.o
nm -u /tmp/sessiontest001-minimal-independent-password.o
cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug
ctest --preset password-hashing-debug --show-only=json-v1
ctest --preset password-hashing-debug
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed. Strict compile/static assertions emitted no warning; nm/static audit
found no sodium/pwhash/Argon/Qt/SQLite undefined symbol or dynamic initializer.
Independent source inspection confirmed the three complete throw bodies, exact
standard includes/text, frozen blobs and preservation of old presets. Fresh configure
used the explicit prefix with no downloads; build completed 8 steps/0 project warnings.
Discovery contained exactly the three enabled, uninverted scaffold cases and no
password/foundation case. All three old infrastructure tests passed in 0.65s, no
skips. Project consistency reports 401 tasks/122 decisions and current links/presets/
HTML. Final staged sole-path/whitespace checks accompany this note's commit.

No password constructor, feature or marker, selected SDK/library API, runtime-version
call, init/hash/verify/RNG/guarded allocation/lock, foundation, new behavior test,
actual login/account operation or provider call was executed. The three existing
infrastructure tests are not a password baseline or acceptance result.

## Remaining gates

Root must separately release independent fixtures/tests and a standalone foundation
author stage; neither future artifact is approved here. Require different actual
test/foundation/harness review, explicit selected dependency build/link/runtime
identity and real selected foundation green before FIRST reviewed project-marker
red. Compile/setup/protection failures, skips and inversion are not valid red.

New SDK/project/foundation/acceptance CI is still absent and required before baseline
merge; old component checks do not cover this target and workflow access is not
assumed restored. No future CI recipe is certified. Production SESSION-001 and its
different source/config review, actual protected lifetimes/cleanup, trusted port/
conditional-save/account integration, admission, supported hardware measurements,
license packaging and platform behavior retain all owning gates. SESSIONTEST-001
remains incomplete. No push, PR or main merge was performed; root owns shared files
and publication.
