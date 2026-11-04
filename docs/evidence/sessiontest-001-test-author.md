# SESSIONTEST-001 preliminary native foundation and input-preparation author evidence

Author: service_author, 2026-11-04. Dedicated `task/SESSIONTEST-001-test-author`,
worktree `local-evidence/worktrees/sessiontest001-test-author`, initially clean
at test-author release `d9762d75d348b911c92f34bd0980db87f64e65a4`. Different minimal
source/harness approval `93d9a576` / root import `30bdd15` is preserved.
Root subsequently split this preliminary SDK-only candidate from the later feature
tests and explicitly assigned the new scaffolding generator below. This commit
contains only these two native-program sources and this bounded author note.

## Actual artifacts; not a completed test candidate

| Artifact | Git blob |
| --- | --- |
| [Standalone SDK foundation](../../tests/acceptance/password_hashing_environment.cpp) | `9f688e3d9353311913af9dac8bab4043e1f5b51e` |
| [SDK-only synthetic input generator](../../tests/scaffolding/password_hashing_fixture_generator.cpp) | `f705227eb4460c551fdabd14cf6460b2b6849299` |
| [Frozen logical contract](../contracts/password-hashing-v1.md), unchanged | `bc9ccbf66a41c9ae69abde4bd905401900e17461` |
| [Frozen API](../contracts/password-hashing-api-v1.md), unchanged | `7ad1e8d3f3311673dd368f2ef4d297de31dbafcd` |
| [Frozen public header](../../server/include/subathon/server/password_hashing.hpp), unchanged | `ec3c5cf87afe406db8501053321598599325a239` |
| [Frozen immediate-throw source](../../server/src/password_hashing.cpp), unchanged | `eaf56f41bafd502b8e4cb38c0de3082180bffcd5` |

Read root/tests instructions, actual contracts/minimal review/harness/SDK evidence
and upstream sources. No production/header/API, CMake/preset/CI/pin, state/HTML,
schema or existing fixture/assertion was edited. This is not the feature-test
inventory, actual selected foundation green, observed red or SESSION-001 behavior.
No feature tests, input fixture header or error wrapper are supplied in this commit.

## Foundation proof partition

The foundation has its own main and includes only standard headers and sodium.h.
It has no project header/helper/fixture/test framework, module constructor, marker
or SUT link/symbol. File-scope data consists of literal constexpr values; no SDK or
SUT call occurs during construction/discovery. Intended native checks, all UNRUN:

- Exact selected header version 1.0.22/ABI 26/4 and Argon2id13/salt16/output128
  constants; initialization readiness 0/1, repeat readiness 1 and actual runtime
  identity. Header/link success alone is not that future runtime observation.
- A guarded one-byte nonnull empty-password buffer with explicit successful lock;
  published H0 verifies at length zero, while the one-NUL input returns native −1.
- The explicit-algorithm raw H0 derivation equals the independently decoded
  published 32-byte digest at t=2/m=4999168 bytes and literal 16-byte salt.
- A second independent published input, hashcat's t=3/m=65536/p1 vector, verifies
  its exact public password; a different public input is unsuccessful. Native −1
  is never given a reason-specific wrong-password interpretation by the module.
- Real SDK creation at the custom t=3/64MiB profile with five literal bytes
  61 00 62 c3 a9; bounded output framing/canonical16/32 Base64, actual explicit-
  length verification and unsuccessful truncation. This portion is honestly an
  SDK round trip/profile capability check, not an independent expected hash oracle.
- Explicit guard lock, whole-buffer clearing observed before release, checked
  unlock, and RAII cleanup across early returns. A lock failure is a failing
  foundation/setup result, never a skip, successful protection claim or feature red.

Public H0 provenance is [libsodium 1.0.22 tv3[2]](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.c)
and its [published expected output](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.exp).
H1 is the exact ST_PASS/ST_HASH pair in [hashcat v7.0.0 module34000](https://raw.githubusercontent.com/hashcat/hashcat/v7.0.0/src/modules/module_34000.c),
whose module identifies MIT provenance. No project feature generated either value.
Ordinary stdlib Base64 decoding, not crypto, produced H0's literal salt/digest byte
spellings from that published record. The [official SDK declarations](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h),
[initialization](https://doc.libsodium.org/usage) and
[guarded-memory guidance](https://doc.libsodium.org/memory_management) support the
selected calls; they do not certify the as-yet-unrun native result.

## Exact PH12/13 input preparation remains gated

The logical examples require actual valid t=6/m=32768 and t=6/m=131072 input
records. H0 cost edits would invalidate its digest. Hashcat's valid independent
t=3/m=65536 vector is additional coverage and does not replace those exact reviewed
examples. No suitable published exact t=6 pair was established by the read-only
source search; no invented/generic-width fixture is silently admitted.

The separate SDK-only generator uses the literal public synthetic passwords
`Subathon public fixture PH12` and `Subathon public fixture PH13`, exact explicit
Argon2id13 encoded creation at the two required profiles, complete bounded profile/
canonical Base64 checks, and actual SDK self-verification. Only after BOTH records
pass does it emit JSON INPUT records to stdout, including their public passwords
and costs. It accepts no external password, file path or provider data. No SUT type,
helper, validation, factory, verification or expected policy calculation is linked.
The expected upgrade targets/no-downgrade/no-replacement results remain independent
logical literals; the generated values would only be input credentials.

Root must first obtain different review of these actual programs, observe the
selected foundation green, then explicitly execute/record the generator and retain
its successful raw output before literal fixtures can be finalized. Neither native
program has run. The generator checks stdout publication errors; an I/O error can
leave partial stdout, which must not be consumed as a successful fixture artifact.
It creates no repository/output file or filesystem-wide atomicity promise.

## Actual compile/link-only checks

Linux x86-64, GCC16.2.1 20260810, clang-format23.1.4, Python3.14.8. Explicit SDK:
`/tmp/subathon-sodium-1.0.22-sdk/prefix`; its authenticated/build provenance remains
root-owned [SDK evidence](sessiontest-001-interface-harness.md). Its earlier vendor
compiler probes/warnings are neither repeated here nor relabeled as feature red.
No implicit host/PATH sodium selection or dynamic libsodium lookup is used.

Actual commands from this worktree (shell variables below only name these exact
local audit paths):

```sh
native_audit=/tmp/subathon-sessiontest001-native-author.L63bXH
sdk_prefix=/tmp/subathon-sodium-1.0.22-sdk/prefix
clang-format -i tests/acceptance/password_hashing_environment.cpp tests/scaffolding/password_hashing_fixture_generator.cpp
clang-format --dry-run --Werror tests/acceptance/password_hashing_environment.cpp tests/scaffolding/password_hashing_fixture_generator.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -I"$sdk_prefix/include" -fsyntax-only tests/acceptance/password_hashing_environment.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -I"$sdk_prefix/include" tests/acceptance/password_hashing_environment.cpp "$sdk_prefix/lib/libsodium.a" -pthread -o "$native_audit/password_hashing_environment"
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -I"$sdk_prefix/include" -fsyntax-only tests/scaffolding/password_hashing_fixture_generator.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -I"$sdk_prefix/include" tests/scaffolding/password_hashing_fixture_generator.cpp "$sdk_prefix/lib/libsodium.a" -pthread -o "$native_audit/password_hashing_fixture_generator"
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All syntax/link invocations passed with zero warnings; both final executables
exist but were NEVER EXECUTED. Source/include and nm/readelf static inspection
confirmed no project symbols or C++ dynamic global initializer and no dynamic
sodium/Qt/SQL/GTest dependency. The linked native SDK symbols are present, not called.
Read-only Python base64/re/Path/subprocess checks passed include/symbol restrictions,
H0 decoded byte spellings and the exact 32768/131072 KiB conversions. No Python
crypto library or fixture generation was used. Final executable SHA256 values:

- Foundation: `1f0f28ab78815eee745f977f56aad7294604b7cb7404051b0d3cf95a25ddddbf`.
- Generator: `ea3f2de41b55f9d0d73d004304e2f8c87d3fb207344e9a8ec8a896705c702eaf`.

Formatting/current project checks passed (401 task records/122 decisions); current
HTML and whitespace passed. Frozen input blob and sole-owned staged path checks
accompany the commit. No init/runtime-version/hash/verify/RNG/memory-protection
library function, module body/marker, foundation, generator, test or login executed.

## Next gates and later feature-test plan

Different actual program/harness review is next, not this author's approval. Root
owns CMake/link flags, selected SDK/CI and both first native executions. After green
foundation and exact input preparation, finalize independent literal fixtures and
all-three-entrypoint PH01–18 feature tests, obtain different test/harness review,
then FIRST plain intended missing-marker red before any production implementation.
No complete feature-test inventory/count is claimed while exact inputs are absent.

The proposed private Linux error-only test harness will default every wrapped call
to the real selected SDK, with only error/null returns, bounded observations and
invalid-output corruption AFTER actual native successful creation. It must never
manufacture valid successful crypto or fake readiness/locking. No production public
seam or crypto-provider interface is requested. Its actual later source/link flags
and cleanup assertions require independent review; none is present in this commit.
Actual store/login/CAS integration, admitted work/cleanup, supported-server benchmark,
platform packaging and release security remain required owning gates. SESSION-001
remains unimplemented and SESSIONTEST-001 incomplete. No push/PR/main merge occurred.
