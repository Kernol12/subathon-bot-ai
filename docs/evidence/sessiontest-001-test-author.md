# SESSIONTEST-001 password test author evidence

The first sections preserve preliminary commit
`fc62e6375828a36d98b4e63703367e83326e1088`; the final section records the subsequent
feature-test candidate after root's reviewed native foundation/input preparation.
Historical UNRUN/pending statements below describe that preliminary revision,
not a claim that root's later recorded native work did not happen.
The final correction appendix records the actual different R1/R2 review findings
and the revised source candidate; it claims no approval or behavioral red.

## Preliminary SDK-only candidate fc62e637

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

## Full feature-test candidate after root's native gates

Root separately integrated native candidates and harness, obtained different
approval `9b92c671` / import `c0646b406b5df0d66699433d99e6808466131d88`, then observed
FIRST selected foundation PASS 1/1 (0.21s) at 2026-11-04T14:42:52Z. Root next
executed the SDK-only generator at 14:43:01Z, exit 0, and retained both fully
self-verified INPUT records. These are root observations, not this author's
executions. Root's durable native/fixture receipt is commit
`095081d4913cd27b53200715ce471a1a1566dded`. Exact raw input artifact:
`local-evidence/worktrees/sessiontest001-integrator/local-evidence/sessiontest001-native/generator-first.stdout`,
SHA256 `b351cf464de2c5d55932126e4dd079472736358a48bf4d14747f9619afa5b9cc`.
This author read it, checked that checksum, and copied both literal records/passwords
byte-for-byte. It is input preparation only: expected PH12 target6/65536 and PH13
no replacement still come from the independently reviewed logical contract.

Final feature artifacts, all new except this updated author note:

| Artifact | Git blob |
| --- | --- |
| [Literal public input vectors](../../tests/fixtures/password_hashing_vectors.hpp) | `025d4de904c4c07a08cc192fc6cf94318bd5c0c8` |
| [Feature tests](../../tests/acceptance/password_hashing_tests.cpp) | `52f0e911cc0dbd17b7f6e847ac778c449598c8b7` |
| [Private error controls](../../tests/support/password_hashing_failures.hpp) | `831a8bb0a4536e52c677a00a70caed5841236394` |
| [Private GNU wrappers](../../tests/support/password_hashing_failures.cpp) | `2782be4171cddafd048efdab130a896a3616adb4` |

Both approved native-program blobs and all four frozen logical/API/header/source
blobs in the preliminary table remain unchanged. No baseline assertion/fixture was
weakened, skipped or replaced; these feature tests are newly authored and awaiting
their first different actual review. Parent/root retains CMake/flags/CI/pin/state/
HTML/receipt ownership. No such shared file is changed in this author tree.

### Literal map and actual discovery inventory

There are 37 unique ordinary TEST declarations, independently matched to LIST ONLY
discovery. This is a declaration/discovery count, not passing tests, 37 executed
behaviors or a fabricated assertion/scenario total. Table loops retain caller-row
SCOPED_TRACE and nonfatal expectations; helpers explicitly fail on the distinct
missing marker or wrong exception category instead of treating any throw as success.

| Suite | Tests | Logical coverage |
| --- | --- | --- |
| PasswordCreation | 2 | PH01/03/04/05/11: empty/NUL/composed/decomposed/space/1024 byte input, selected real verification of returned records; exact desired endpoints via error-only native refusal |
| PasswordPreflight | 4 | PH05/11/18: all three direct raw entrypoints, invalid options before oversized input/source work, zero admission, actual stored memory ceilings before native initialization |
| PasswordVerification | 4 | PH01/02/03/05: published raw H0, one-NUL distinction, nonterminated borrowed record view, 1024-byte admission at verify and upgrade |
| PasswordRecordPreflight | 4 | PH06–10: complete field/decimal/canonical Base64, unused bits, p0 versus positive unsupported p, unsupported before supported-cost failure, huge digits without expensive verification, exact127/128 record ceiling, supported stored endpoints with explicitly unsuccessful native injection |
| PasswordSource | 4 | State/optional/attempt consistency and fixed priority; 256/257 byte fields, source membership, missing/unavailable, source bad_alloc/diagnostic privacy and actual retained record preflight |
| PasswordUpgrade | 16 | PH01/12–16/18: raw H0 accepted-source proposal; no unverified/denied/stale upgrade; actual port identity/full source/attempt, both gate phases, joined admission/costs, exact PH12/13, both dimensions of non-downgrade, owned lifetime, full CAS-source binding and later reset preservation |
| PasswordSecretFailures | 3 | PH15/17: initialization, guarded allocation, explicit lock, native creation and invalid-output failures; fixed safe text and bounded cleanup/no native work after refusal |

All setup is raw literal data plus a simulated read-only authoritative account/login
port. No earlier project hash, verify, factory or certificate is called to make an
entrypoint reachable. The private proof is created only inside future implemented
upgrade, never by fixtures. The fake port tracks actual simulated current credential,
accepted login source and actual attempt separately; it cannot manufacture crypto
success and returns stale/denied from those states. Tests check private proof source
identity, exact full retained credential and attempt, native verification before
the first gate, fresh hash before the second, same live protected buffer, and no
replacement after either gate refuses. They do not claim an implemented real account
service, permission decision, SQLite conditional save or persistence proof.

Returned verifier checks use only the real selected SDK AFTER that test's SUT call
returns successfully, with observations/injections suspended. Output ceiling/exact
bounded cost prefix/NUL/framing/salt/digest lengths are asserted before invoking that
oracle, preventing a malformed SUT output from requesting arbitrary native cost.
No project verification helper supplies its own creation oracle, random output is
not an expected literal, and fresh salt inequality is not asserted as certainty.
Published H0/H1 and generated PH12/13 are setup INPUTS, not expected SUT hash bytes.
Editing H0 costs is used only to exercise structural admission with an explicitly
unsuccessful native error; it is never claimed a valid success fixture.

### Private error-only seam and observation bounds

The Linux-only linker seam defaults to the real selected SDK. Its thread-local
state is constinit, fixed-size and contains only scalar counters, bounded allocation
slots and borrowed views of public synthetic test inputs. No SDK call or dynamic
SDK/SUT setup occurs globally, during GTest registration or listing. Scope controls
are instantiated only inside TestBody; no test framework/global SetUp calls a
feature. Nested native dispatch/guard calls bypass observation, so SDK internals
are not counted as separate module-owned password copies or extra operations.

Failures return only −1/null. The two output-corruption modes run only AFTER an
actual real native hash succeeded and produce an invalid version or unterminated
output for rejection. They never synthesize a valid verifier, success/readiness/
locking, fixed salt, successful password comparison or alternate crypto provider.
The raw observer reads password bytes only for an exactly tracked sufficiently
large guarded allocation. Record comparison stops at the first differing byte/NUL
and checks the terminator only after matching the bounded expected prefix; it does
not memcmp an expected long size beyond a shorter valid native string. Root's
pre-freeze feedback prompted that source hardening; no expected behavior changed.

Observations cover live/locked input, exact raw bytes/record terminator, input copy
before explicit lock, same protected pointer, and complete tracked bytes being
zero before free. They never inspect freed memory, retain password copies in
diagnostics, claim to wipe caller/stack/register/native Argon buffers, or count
allocator/native internal ownership as application DTO ownership. A scope exposes
outstanding/wrong cleanup as failure; it does not quietly free leaks to pass.

Exact required wrap symbols (feature target only, NEVER foundation/generator):

```text
sodium_init sodium_malloc sodium_allocarray sodium_mlock sodium_memzero
sodium_munlock sodium_free crypto_pwhash_str crypto_pwhash_str_alg
crypto_pwhash_argon2id_str crypto_pwhash_str_verify crypto_pwhash_argon2id_str_verify
```

There is deliberately NO operator-new interposition. A valid implementation can
preallocate its complete replacement before native hashing, so arming a next-new
failure after hash success would require an implementation-specific allocation order
or capture unrelated diagnostics. Root agreed to omit that optional nondeterministic
seam. Guard allocation/native hash failures and port bad_alloc are covered; runtime
injection of an ordinary postaccepted replacement-materialization bad_alloc is an
explicit remaining test limit. Independent production source audit must still verify
all documented allocation-failure mappings and old-record preservation; the limit
does not remove that behavior requirement or justify fabricated measured coverage.

### Final author checks, compile/link and LIST ONLY

The final two feature translation units compile in syntax-only mode with the same
C++20 warning set as below. Actual object builds and manual link use a private
temporary directory, frozen stub source and exact selected SDK archive; no shared
build/profile file is edited. Installed GTest metadata is 1.18.0. Commands:

```sh
feature_audit=/tmp/subathon-sessiontest001-feature-author.zTpNW1
sdk_prefix=/tmp/subathon-sodium-1.0.22-sdk/prefix
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -Iserver/include -I"$sdk_prefix/include" -DGTEST_HAS_PTHREAD=1 -c tests/acceptance/password_hashing_tests.cpp -o "$feature_audit/password_hashing_tests.o"
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -Iserver/include -I"$sdk_prefix/include" -c tests/support/password_hashing_failures.cpp -o "$feature_audit/password_hashing_failures.o"
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -Iserver/include -I"$sdk_prefix/include" -c server/src/password_hashing.cpp -o "$feature_audit/password_hashing.o"
c++ "$feature_audit/password_hashing_tests.o" "$feature_audit/password_hashing_failures.o" "$feature_audit/password_hashing.o" "$sdk_prefix/lib/libsodium.a" -lgtest_main -lgtest -pthread -Wl,--wrap=sodium_init -Wl,--wrap=sodium_malloc -Wl,--wrap=sodium_allocarray -Wl,--wrap=sodium_mlock -Wl,--wrap=sodium_memzero -Wl,--wrap=sodium_munlock -Wl,--wrap=sodium_free -Wl,--wrap=crypto_pwhash_str -Wl,--wrap=crypto_pwhash_str_alg -Wl,--wrap=crypto_pwhash_argon2id_str -Wl,--wrap=crypto_pwhash_str_verify -Wl,--wrap=crypto_pwhash_argon2id_str_verify -o "$feature_audit/password_hashing_tests"
"$feature_audit/password_hashing_tests" --gtest_list_tests > "$feature_audit/gtest-list.txt"
```

Object/link checks passed with zero final warnings. Static source/nm inspection
before listing found only inert GTest registration, no global SDK/SUT constructor
or feature setup. Listing returned all 37 names, no TestBody or marker. Binary
SHA256 `b87029aa281cc3fdc6c0ae55f2921038d26d2cf133340f8e84decc100b0b1b7e`;
listing SHA256 `e870eab75ca0518a4bf1e039d54ac90e45083b381ca84e40478647ef5bc56f4d`.
An initial compile caught three GTest-macro dangling-else warnings; explicit braces
fixed those unreviewed source statements, followed by clean final builds. An initial
inventory audit compared source order to GTest's grouped-suite listing order; changed
only that audit to compare unique name sets/counts, then all 37 matched. Neither is
a project behavioral failure/red or a change to expected fixtures/assertions.

Formatting verification, project consistency/current HTML, frozen exact blob checks,
generated-input checksum/literal equality and staged owned-path whitespace checks
passed. No feature/test body, module marker, sodium init/hash/verify/RNG/memory API,
foundation/generator, real login or provider operation ran in this author stage.
The only executed feature binary mode was GTest LIST ONLY. Native successful input
preparation above is explicitly attributed to root's reviewed separate programs.

Different actual full fixture/test/wrapper/harness/CI review is now required. Root
owns actual CMake/CI integration, current selected CI foundation green and FIRST
plain intended feature-marker red; no future approval or red result is supplied by
this note. Production implementation/source audit, all allocation handling and actual
account/store/admission/conditional-save/platform/benchmark gates remain required.
This full candidate does not finish SESSIONTEST-001 or implement SESSION-001.

## Correction after actual different review R1/R2

Actual review `173d90a53d74e5e29a647084cc0675cb861957f6`, root import
`cbdb1332c4d4b04bab7122f11155948a8c0dcf9d`, is CHANGES REQUIRED. Its frozen
[review note](sessiontest-001-test-review.md) has Git blob
`56ad3b1c1589b2d601389acb96c36689b2c7710e`. This correction started clean at that
root import on dedicated `task/SESSIONTEST-001-test-correction`, isolated worktree
`local-evidence/worktrees/sessiontest001-test-correction`. The same source author
owns only the test TU, private observer header/source and this author note.
Fixtures, approved native programs, contracts, production header/source, review
note, CMake/workflow/pin/state/HTML and other agents' work remain frozen.

### Written specification/API basis and exact revised source

My earlier blanket output-oracle safety claim was incorrect: a fatal assertion in
a helper returns from that helper, so the creation caller's subsequent different-
password native oracle was not guarded. R1 is justified by API §§3/4's mandatory
bounded cost admission before native work, not a changed product requirement.

The SDK-free bounded_record helper retains the SAME six original assertions and
literal bounds/prefix expectations. A caller-owned bool is reset false for each
invocation and becomes true only after all six succeed. All six oracle caller
sites inspect their own flag and continue that row/return the test on failure
BEFORE calling any real SDK oracle on that record. The creation row's two native
oracles share that successfully checked immutable current-row string. No cumulative
HasFatalFailure/previous-row state substitutes for this invocation's result. A
malformed row still fails and cannot request an oversized native KDF; following
table rows remain reachable. These paths have been inspected, not executed.

R2 is justified by API §4's SAME checked protected password allocation surviving
verification, both source checks and replacement creation. That does not restrict
the total count of separately guarded record/output scratch. The old live-count
of one could reject valid scratch ownership and did not identify the password
allocation. A first address alone also survived free/address reuse; a latched
lock flag incorrectly survived successful real unlock.

The bounded observer now assigns a monotonically increasing nonzero lifetime ID
only after an actual real guarded allocation succeeds. It retains the first native
password's pointer AND lifetime ID. Free retires the tracked entry; reusing its
slot/address gets a different ID and cannot recreate the original lifetime.
Snapshot reports the CURRENT original password allocation's live and checked-lock
state plus uninterrupted protection. The source gates check these semantic facts
instead of total guarded allocation count, while later native uses check original
allocation identity and protection. Unrelated live scratch does not certify a
missing original password copy, nor does legitimate extra scratch make it fail.

Successful real explicit mlock records full-allocation locking; repeated locking
of an already checked locked allocation is not mistaken for copy-before-first-lock.
Successful real nonempty overlapping munlock clears current lock state and records
an interruption that later relocking cannot erase. Checked unsigned address
subtraction avoids range-end overflow. Cleanup free removes the entry; subsequent
current observations cannot certify its stale address. Slot/lifetime-counter
exhaustion remains an explicit bounded tracking-overflow diagnostic, never a false
positive protection result. The fixed16-slot tracker, real SDK delegation,
error-only injections and whole-buffer zero-before-free checks are preserved.
There is no operator-new/public API/successful-crypto/readiness/locking fake.

| Corrected artifact | Previously reviewed blob | Revised blob |
| --- | --- | --- |
| [Feature tests](../../tests/acceptance/password_hashing_tests.cpp) | `52f0e911cc0dbd17b7f6e847ac778c449598c8b7` | `1d0d37753cd54cad189bdf9d08481838cac9880e` |
| [Private observer header](../../tests/support/password_hashing_failures.hpp) | `831a8bb0a4536e52c677a00a70caed5841236394` | `7e87bbe9c328a8791244902c7e9446ceef000996` |
| [Private observer source](../../tests/support/password_hashing_failures.cpp) | `2782be4171cddafd048efdab130a896a3616adb4` | `4e1f049a2dcd0ff018dc3c728daad39f0773a82f` |

### Actual allowed checks and preserved baseline inputs

Linux x86-64/GCC16.2.1/Python3.14.8/CMake4.4.5/clang-format23.1.4, installed
GTest1.18.0, explicit root-prepared static SDK prefix. Actual commands:

```sh
correction_audit=/tmp/subathon-sessiontest001-test-correction.LIe2GS
clang-format -i tests/acceptance/password_hashing_tests.cpp tests/support/password_hashing_failures.hpp tests/support/password_hashing_failures.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -Iserver/include -I/tmp/subathon-sodium-1.0.22-sdk/prefix/include -fsyntax-only tests/acceptance/password_hashing_tests.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -I/tmp/subathon-sodium-1.0.22-sdk/prefix/include -fsyntax-only tests/support/password_hashing_failures.cpp
cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug
ctest --preset password-hashing-debug --show-only=json-v1 > "$correction_audit/discovery.json"
build/password-hashing-debug/tests/server_password_hashing_acceptance --gtest_list_tests > "$correction_audit/gtest-list.txt"
clang-format --dry-run --Werror tests/acceptance/password_hashing_tests.cpp tests/support/password_hashing_failures.hpp tests/support/password_hashing_failures.cpp
python3 "$correction_audit/audit.py"
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Both strict syntax checks PASS. Configure/build PASS, 13 steps/zero project
warnings. LIST ONLY and CTest show-only contain all unchanged37 feature names and
41 total registered tests; no feature was disabled, inverted or skipped. The
stdlib source audit tokenizes balanced ASSERT/EXPECT invocations and proves ALL
existing invocation arguments/string literals unchanged; it separately compares
all source string literals and the37 exact method names against cbdb133's frozen
input. It checks six independent caller gates, the SDK-free validator, bounded
lifetime/unlock structure and10 unchanged fixture/native/contract/production/build/
workflow/review blobs. It invokes no C++ helper, wrapper, injection or feature.
Audit SHA256 `eb287e73efc973db10479a57a9f3419a76006a5be8aae6a16232ba973b8294ea`;
discovery SHA256 `809c7b4560dc35ab8abc6a819204ab8c328f153e1d91ec4a8c5a068d0db72d33`;
listing SHA256 `e870eab75ca0518a4bf1e039d54ac90e45083b381ca84e40478647ef5bc56f4d`.

Initial show-only redirection targeted an absent local-evidence directory and
failed before CTest ran. Corrected only the ignored artifact destination to the
temporary directory above, then show-only passed. This setup error is not a
feature failure, native run or valid intended red. Project consistency/current
HTML passed (406 tasks/122 decisions); staged four-owned-path/whitespace checks
accompany the commit. No permission/D1 artifact or password fixture/native program
was changed. No selected SDK API, test/marker/wrapper/injection body, foundation,
generator or behavior probe ran; only compile/link and discovery/listing executed.

The original different reviewer must re-review this actual corrected revision.
No approved baseline, red attribution, production release or self/future approval
is claimed. Root's separate sequencing proposal `25506e` is under different review,
NOT adopted by this correction and does not authorize a first-red execution.
Current selected CI/FIRST red and all actual implementation/allocation/secret/
account/conditional-save/admission/platform/benchmark gates remain as recorded by
the coordinator. Permission D1 stays pending in its separately preserved worktree.
