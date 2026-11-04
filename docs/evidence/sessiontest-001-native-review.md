# SESSIONTEST-001 — Independent SDK foundation and input generator review

Reviewer: `/root/service_reviewer`, different from program author
`/root/service_author` and harness author `/root`. Date: 2026-11-04.
Dedicated `task/SESSIONTEST-001-native-review`, isolated worktree
`local-evidence/worktrees/sessiontest001-native-review`, initially clean.
Only this new review note is owned/edited. This is a new bounded native-program
review, not a replay or expansion of minimal-source approval `93d9a576`.

## Exact candidate and narrow verdict

Reviewed root candidate `3bb5d0b58e06d2403a52c071a09a12b6c3b58742`, following author
`fc62e6375828a36d98b4e63703367e83326e1088` / import `ad9c27b`:

| Artifact | Git blob |
| --- | --- |
| [SDK foundation](../../tests/acceptance/password_hashing_environment.cpp) | `9f688e3d9353311913af9dac8bab4043e1f5b51e` |
| [Synthetic input generator](../../tests/scaffolding/password_hashing_fixture_generator.cpp) | `f705227eb4460c551fdabd14cf6460b2b6849299` |
| [Author evidence](sessiontest-001-test-author.md) | `46b531f54101fa02a9d5d152bd93935220fdcad0` |
| Tests CMakeLists.txt | `7e72e5c5f95364349125dcc2cb33ff7829d5433f` |
| [Native harness evidence](sessiontest-001-native-harness.md) | `5464065885431189e682f83bd907820bcf155130` |

**APPROVED for these exact two SDK-only programs and native harness only.** No
blocking defect found. Root may perform FIRST foundation execution, then FIRST
generator execution only after that selected foundation is actually green. No
native result or generated credential exists merely because this source review
passes. Full feature tests/private wrappers have not been supplied and are not
approved; FIRST project feature/marker red remains unreleased.

Read complete 174-line foundation and 108-line generator, actual CMake diff/author/
harness evidence, root/tests/server instructions and frozen logical/API/source.
Logical `bc9ccbf66a41c9ae69abde4bd905401900e17461`, API
`7ad1e8d3f3311673dd368f2ef4d297de31dbafcd`, header
`ec3c5cf87afe406db8501053321598599325a239` and three-immediate-throw source
`eaf56f41bafd502b8e4cb38c0de3082180bffcd5` remain unchanged. Root subsequently
reported metadata-only main integration `513b0f9` with these bytes unchanged; this
review remains pinned to the exact artifacts above, not future code or metadata.

## Independent vectors and SDK semantics

H0 matches [libsodium 1.0.22 tv3[2]](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.c)
and its [published expected output](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.exp).
The expected success for the empty password is published upstream, not generated
by this project. Separately decoded original-unpadded Base64 reproduces every
literal H0 salt/digest byte in the foundation. The raw explicit-algorithm call uses
that salt, t=2 and 4882 KiB=4999168 bytes; its expected 32 bytes come from the published
record, not a second project algorithm.

H1 matches the exact ST_PASS/ST_HASH pair in
[hashcat v7.0.0 module34000](https://raw.githubusercontent.com/hashcat/hashcat/v7.0.0/src/modules/module_34000.c):
public password hashcat, Argon2id/v19/t3/m65536/p1 with 16-byte salt/32-byte digest.
The primary module identifies its MIT provenance; the source comment and author
evidence retain attribution. No hashcat code/dependency is linked. These are two
independent published verifier inputs, not observations from an executed foundation.

Header identity/constants precede initialization; only readiness 0/1 allows guarded
allocation and subsequent native calls. Repeat initialization and runtime version/
ABI checks occur inside main. They match the [SDK declarations](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h)
and [initialization contract](https://doc.libsodium.org/usage). Native −1 observations
are labeled unsuccessful, not definitive wrong-password diagnoses.

Empty input uses a real guarded nonnull one-byte buffer with explicit successful
lock and native length 0; the same buffer with length 1 contains one NUL. Both raw
byte and H1 variants have explicit lengths. The five-byte `61 00 62 c3 a9` round trip
checks NUL/non-ASCII preservation and unsuccessful truncation, with exact custom
t3/64MiB output framing. This generated round trip is honestly a capability/profile
check, not an independent expected digest oracle or project password behavior test.

ProtectedBytes owns both guarded allocations through every early return, clears
the entire allocation, unlocks when required and frees. Successful raw-buffer clear
is read back before checked unlock. Native readiness precedes all protected-memory
calls, and lock failure fails the foundation rather than skipping it. This follows
[official secure-memory semantics](https://doc.libsodium.org/memory_management).
Actual locking/free/cleanup execution is still unobserved. Public stack digest/
record scratch is cleared on the normal path; this is not a proof of all future
module scratch/error cleanup, ingress erasure, stack/register protection or secure
OS configuration. No real secret or external input is accepted by either program.

## Generator validity and publication boundary

The generator is raw SDK input preparation, never a SUT setup factory/oracle. It
uses only the two fixed public 28-byte synthetic passwords and exact profiles:
PH12 t6/m32768 KiB=33554432 bytes and PH13 t6/m131072 KiB=134217728 bytes. PH12's
32 MiB is an intentionally valid stored-input profile below the production new-hash
desired floor, not a call to production create_password_hash or a policy change.

Each record is bounded to 128 bytes, must have a terminator and exact algorithm/
version/m/t/p prefix, 22/43-character canonical fields, 16/32 decoded bytes, complete
Base64 consumption/re-encoding and actual SDK self-verification. No H0 cost-edit
is relabeled a valid successful credential. The creation/verification calls use the
same real selected SDK; no simulated successful compiler/crypto/provider is injected.

Both records pass all checks before any stdout record is published. Failed prepare
clears both buffers, reports bounded failure and publishes no credential. Fixed
IDs/passwords and admitted PHC alphabet contain no JSON quote/backslash/control
escaping problem. Records are cleared after output; flush/ferror failure exits
nonzero and explicitly admits partial stdout. There is no cross-file/stream atomicity
claim: root must retain output only after successful exit and complete parse.

The outputs are INPUT credentials only. Expected PH12 joined cost 6/65536 and PH13
unchanged 6/131072/no replacement remain independent logical literals. No expected
upgrade outcome, decision, test assertion or module result is generated. Actual
successful generator output/provenance must be recorded before literal fixtures
and full feature tests are finalized and independently reviewed.

## Actual compile, link and discovery checks — no program execution

Linux x86-64/GCC 16.2.1/CMake 4.4.5/Python 3.14.8; explicit existing static SDK prefix
`/tmp/subathon-sodium-1.0.22-sdk/prefix`. Commands in the isolated worktree:

```sh
clang-format --dry-run --Werror tests/acceptance/password_hashing_environment.cpp tests/scaffolding/password_hashing_fixture_generator.cpp
cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug --target password_hashing_environment password_hashing_fixture_generator
cmake --build --preset password-hashing-debug
ctest --preset password-hashing-debug --show-only=json-v1
nm -C build/password-hashing-debug/tests/password_hashing_environment
nm -C build/password-hashing-debug/tests/password_hashing_fixture_generator
readelf -d build/password-hashing-debug/tests/password_hashing_environment
readelf -d build/password-hashing-debug/tests/password_hashing_fixture_generator
objdump -s -j .init_array build/password-hashing-debug/tests/password_hashing_environment
objdump -s -j .init_array build/password-hashing-debug/tests/password_hashing_fixture_generator
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed. Native targets compiled/linked 4 steps/0 warnings; the additional default
build compiled/linked 8 existing scaffold/module steps/0 warnings. No binary was run.
Both link commands use only the explicit SDK archive plus normal toolchain/Threads
requirements, without any project/GTest target. nm shows native definitions and no
subathon/GTest/C++ dynamic initializer. readelf shows normal C/C++/math/unwind runtime
dependencies, no dynamic sodium/Qt/SQL/GTest. Each init_array contains only the CRT
frame_dummy address, no native-library initialization callback.

Discovery has exactly three old infrastructure cases plus
scaffold.password_hashing_environment, enabled/uninverted, labels scaffold and
password-hashing-environment, timeout 30 and its exact binary command. Generator
is absent from CTest and tests/all; EXCLUDE_FROM_ALL plus no custom execution
prevents running it at configure/build/discovery. This review ran no CTest bodies,
including old infrastructure; project consistency/current HTML were checked directly.
Consistency reports 401 tasks/122 decisions. Staged sole-path/whitespace checks
accompany this note's commit.

Stdlib-only literal/static audit passed 32 facts: two published records, 96/97 ASCII
lengths, framing, four decoded/canonical fields, literal H0 byte arrays, four memory
conversions, independent policy joins, 28-byte synthetic inputs, stage-before-output,
frozen blobs and include/SUT independence. Initial audit expected 27 input bytes;
corrected the audit's arithmetic to 28 and reran all 32, with no artifact change.
That review-tool mistake is not an SDK/project behavior failure or valid red.

Linked but UNRUN reviewer binary SHA256 values:
foundation `822edfebb7d39d614ec1faef766bd65f9f1c75fe685c9d12edc68c21b7790ef8`;
generator `cf784e80437b7b9c90cc601e7a30934334d4863b07eb0ed7d31e680194e8446e`.
No SDK API, runtime-version/init/hash/verify/RNG/protected-memory function, SUT
constructor/body/marker, foundation, generator, fixture preparation or login ran.

## Next gates

Root owns FIRST native execution with child core limits, clean revision/raw results
and exact selected SDK identity. Failure is a real environment/foundation blocker,
never intended product red. Only observed green releases generator execution;
record both successful complete input records and their provenance afterward.
Selected Ubuntu CI, supported-hardware cost, platform/packaging and full module
cleanup remain separate. No current native result proves those capabilities.

Then require actual independently authored feature fixtures/tests/private error
harness, different full test/config review, current selected foundation/CI green,
FIRST plain named-marker red and attribution before production implementation.
No future wrapper may fabricate successful readiness/locking/crypto. Full account/
port/conditional-store/admission/benchmark requirements remain. SESSIONTEST-001 is
incomplete and SESSION-001 unimplemented. Root retains shared state/build/CI/pins/
publication ownership. No push, PR or main merge was performed by this reviewer.
