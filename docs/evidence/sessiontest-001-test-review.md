# SESSIONTEST-001 independent full fixture, test and harness review

Reviewer: `/root/codec_author`, different from fixture/test/wrapper author
`/root/service_author` and CMake/CI author `/root`. Date: 2026-11-04.
Dedicated `task/SESSIONTEST-001-full-review`, isolated worktree
`local-evidence/worktrees/sessiontest001-full-review`, initially clean.
Owns only this new note. Earlier dependency-environment work by this reviewer did
not author these tests, wrappers or harness and supplies no independent approval
of them. No author's source, fixture, assertion, contract or shared file is edited.

## Actual candidate and verdict

Reviewed root `2c0c9ab705022c3778e0d97591c4376e39452c68`, following author
`8e6d1dd170f59710ebc989a0e5f85b16567446e6` / import
`82d624de5798f4505a9407250039daa0520cfd00`.

**CHANGES REQUIRED before full test approval or FIRST feature red.** R1 needs a
caller-level bounded-output gate before every real SDK oracle. R2 needs an actual
password-allocation/lifetime/lock observation at the source gates, rather than a
total-allocation count and a lock flag that survives unlock. These are bounded
test/observer defects against the frozen API, not requests to change product
behavior. Preserve the 37 test names, literal inputs and unaffected assertions.
Actual revised artifacts require this different reviewer's re-review. No selected
CI result, red baseline, production implementation or supported platform is approved.

| Actual reviewed artifact | Git blob |
| --- | --- |
| [Logical contract](../contracts/password-hashing-v1.md) | `bc9ccbf66a41c9ae69abde4bd905401900e17461` |
| [API contract](../contracts/password-hashing-api-v1.md) | `7ad1e8d3f3311673dd368f2ef4d297de31dbafcd` |
| [Literal fixtures](../../tests/fixtures/password_hashing_vectors.hpp) | `025d4de904c4c07a08cc192fc6cf94318bd5c0c8` |
| [Feature tests](../../tests/acceptance/password_hashing_tests.cpp) | `52f0e911cc0dbd17b7f6e847ac778c449598c8b7` |
| [Private control declarations](../../tests/support/password_hashing_failures.hpp) | `831a8bb0a4536e52c677a00a70caed5841236394` |
| [Private wrappers](../../tests/support/password_hashing_failures.cpp) | `2782be4171cddafd048efdab130a896a3616adb4` |
| Tests CMakeLists.txt | `9b10f68ce44ea5bcc0f279e5576e5c579613ae96` |
| Password workflow | `cb61b3ec028eddee805f7b6c9928d834789b54c7` |
| [Explicit SDK importer](../../cmake/SodiumDependency.cmake) | `bdbe0d6d7268a7231228ec96a040f59e8686108d` |
| [Test author evidence](sessiontest-001-test-author.md) | `43d1db9df915963689dac0d52ea20d29b22dd985` |
| [Root harness evidence](sessiontest-001-test-harness.md) | `c359bd4df351844012eba24969939ff8d85c466a` |

Read root/tests/server instructions, specification §10/AUTH-11, SESSION-001,
complete frozen logical/API documents, actual minimal-source/native reviews and
root's first-native/input receipts. Read the full 1076-line feature TU, fixture,
both private-control files and actual CMake/preset/import/CI changes. Header
`ec3c5cf87afe406db8501053321598599325a239` and source
`eaf56f41bafd502b8e4cb38c0de3082180bffcd5` remain frozen: exactly three immediate
module-marker throws, with no production behavior preceding them.

## R1 — A failed helper assertion does not guard its caller's SDK oracle

In `password_hashing_tests.cpp`, `real_record` at lines 216–229 checks output length,
expected bounded cost prefix, NUL/framing and salt/digest lengths using ASSERT_*.
The creation test calls it at line 277, then lines 283–285 separately call real
`crypto_pwhash_argon2id_str_verify` on the same returned string with the different
password. A fatal GoogleTest assertion returns from the current helper only. It
does not return from the enclosing test/loop, so this second oracle still executes
after failed validation.

For example, a defective successful SUT return containing an otherwise parseable
PHC record with excessive memory can fail the expected-prefix assertion, then still
request that excessive real KDF at the second call. No such scenario was executed.
API §§3/4 require bounded cost admission before native work, and the author evidence
expressly claims the returned-output guard prevents arbitrary native cost. That
claim is not true for this caller path. This is a test safety/control-flow defect,
not a reason to weaken the output assertions or accept the wrong profile.

Required correction: make successful bounded validation a per-invocation caller
precondition for EVERY real SDK oracle that consumes SUT output, including this
different-password check. Preserve all existing assertions, profile ceilings,
real-library verification and independent input rows. A failed row must remain a
failure, without passing malformed output to native code or allowing previous
rows' failure state to masquerade as this row's validation result. Keep the
distinct missing marker a failure; do not catch it as success or execute a feature
to validate this correction before actual re-review.

## R2 — Observe the actual checked password allocation through each gate

`LoginSource::check_current_successful_login`, line 167, records
`guarded_during_checks &= observed.live_allocations == 1`. This counts all tracked
module guarded allocations. API §4 requires the same checked protected password
copy alive through native verification, both source checks and replacement hashing;
it does not forbid separately guarded bounded record/output scratch. The exact
total of one can reject a conforming implementation that protects such scratch.
Conversely, one unrelated live allocation can satisfy the count after the actual
password allocation was freed.

The wrapper's State stores only a first native password address, and Allocation
stores pointer/size/explicitly_locked, without a lifetime identity. Freeing and
reallocating at the same address can therefore pass the existing same-address
observation. `same_buffer_during_checks` merely copies that historical boolean;
it does not establish that the original password allocation is still live/locked
at either callback.

There is also a concrete lock-state transition gap: `__wrap_sodium_munlock` at
lines 247–249 only delegates. A real successful unlock never clears the tracked
`explicitly_locked` flag, which was latched by successful explicit mlock. Existing
`observe_password` can then report protected-and-locked after early real unlock.
This conflicts with the API's checked protected lifetime; it is not cured by an
unchanged pointer or a positive total count.

Required correction: bounded private observations must identify the actual first
native-password allocation and its continuing lifetime, distinguish address reuse,
and observe actual lock/unlock/free transitions at both source checks and later
native use. Successful real unlock cannot leave that allocation certified locked.
Do not infer protection from a fabricated success, unrelated scratch, old pointer
or caller flag. Preserve real SDK delegation, the single protected password-copy
requirement and whole-buffer cleanup checks; remove the unsupported restriction
on total guarded scratch allocations. No operator-new interception, global SDK
setup, new successful provider/crypto/readiness/lock fake or public API change is
needed or authorized. The author should choose the smallest bounded observation
correction and return actual source for re-review, without executing it first.

## Other inspected portions and literal independence

No additional blocker was found in the following actual portions; this does not
approve a future correction or claim every allocation/OS failure was exercised:

- All three raw entrypoints have direct literal setups. There is no prior SUT
  factory/hash/verification call or public proof/result constructor in a provider
  or global initializer. Marker catches add nonfatal failures and retain row
  reachability; no catch turns missing behavior into a pass.
- PH01–11/17/18 cover empty/NUL/non-ASCII/space/1024 versus 1025 bytes, options
  before input/source work, exact desired/stored endpoints, canonical framing,
  Base64 unused bits, malformed versus unsupported versus resource priority,
  127/128 bytes, huge positive digits, zero admission and actual stored cost.
  Modified H0 cost fields are used only with unsuccessful error injection, never
  as successful credentials.
- PH12/13 use the exact recorded input credentials; expected joined 6/65536 and
  unchanged 6/131072 policies remain independent literals. The additional cases
  preserve stronger time while increasing memory and stronger memory while
  increasing passes. No parameter mismatch helper supplies downgrade authority.
- PH14–16 source tests separate actual simulated credential, accepted credential,
  attempt, current revision/record and login state. Private proof source identity,
  full snapshot/attempt, verify-before-gate and hash-before-second-gate are checked.
  Denied/stale/unavailable/protocol/exception states produce no replacement/write.
  Returned proposals own full CAS-source fields, survive source destruction and
  preserve a newer reset; no actual account transaction or conditional save is
  claimed by the read-only fake.
- Error text is fixed and typed; source diagnostics are not rethrown as secrets.
  Source bad_alloc and guarded/native failures have separate expectations.
  Tracked allocations are inspected before free, with zero/live/overflow checks.
  Ordinary postaccepted replacement-materialization allocation injection is
  explicitly absent, as the author records; production audit still must verify
  that mapping and every cleanup path. No global allocation hook is approved.
- Wrappers default to real SDK calls and inject only −1/null or invalid output
  AFTER an actual real hash succeeded. The invalid-output cases do not manufacture
  a valid credential. Trivial constinit TLS, 16 bounded allocation slots and
  borrowed synthetic views do no global SDK work. Nested real calls are excluded
  from module observations. Shorter valid C strings stop safely on NUL/difference.
  R2 remains the specific lifetime/lock-observation defect above.

Independently compared H0 with the authenticated 1.0.22 upstream source and its
[published expected output](https://raw.githubusercontent.com/jedisct1/libsodium/77e1ce5d6dee871c49ef211222ba18ef0c486bda/test/default/pwhash_argon2id.exp).
H1 equals the exact ST_PASS/ST_HASH pair in the primary
[hashcat v7.0.0 module](https://raw.githubusercontent.com/hashcat/hashcat/v7.0.0/src/modules/module_34000.c),
which identifies its MIT provenance. No hashcat code is linked. Stdlib Base64
checks confirm all four records' canonical 16/32-byte fields and ASCII lengths
96/97/97/98. The exact PH12/13 JSON bytes hash to
`b351cf464de2c5d55932126e4dd079472736358a48bf4d14747f9619afa5b9cc` and match
the literal header byte-for-byte, including 28-byte public passwords. Checked
127-byte boundary construction, memory units and independent componentwise cost
literals without any KDF/SUT call. No expected random verifier or salt-inequality
guarantee is generated or asserted.

The differently approved native-program blobs remain exactly
`9f688e3d9353311913af9dac8bab4043e1f5b51e` and
`f705227eb4460c551fdabd14cf6460b2b6849299`. Read actual narrow approval
`9b92c671592528ede2c67ca7f0b62b94e912c541` / import `c0646b4` and root's
[FIRST native/input evidence](sessiontest-001-native-results.md), commit `095081d`.
Root's foundation green and generator output remain their own valid evidence;
this reviewer did not replay either program or independently execute their crypto.

## Harness, inventory and CI audit

Independent source/discovery comparison found exactly 37 unique ordinary TESTs:
Creation 2, Preflight 4, Verification 4, RecordPreflight 4, Source 4, Upgrade 16 and
SecretFailures 3. All exact names match the frozen source. Discovery has 41 unique
names: those 37 plus scaffold.contract_boundary, scaffold.python_tools,
scaffold.project_consistency and scaffold.password_hashing_environment. Features
are enabled, uninverted and labelled password-hashing/acceptance/intentional-red/
server, timeout 60 s; foundation is separate scaffold/password-hashing-environment,
timeout 30 s, with no intentional-red label. Generator is absent from CTest/default all.

The 12 GNU --wrap options apply only to the feature executable. Actual generated
link rules give foundation/generator only the explicit SDK and no SUT/GTest/wraps;
the feature target alone links the frozen module, real static SDK and GTest main.
PRE_TEST discovery invokes only LIST, with no TestBody/native/global setup. The
selected SDK archive still hashes to
`959cdbe83cd0c80264e080efdd1e861b20b57ded2a85bd9a924297b8a10f1935`.
readelf shows no dynamic sodium dependency. Header metadata is not runtime proof.

The source workflow uses the exact authenticated 2008529-byte archive/SHA pin
before extraction, a disposable static prefix, portable GCC/-O2 and two make jobs,
with core limits before ordinary Autotools native probes. No vendor make check,
native-CPU/LTO/sanitizer/security-disable shortcut or host sodium fallback is added.
GoogleTest retains the existing opt-in pinned source policy. Actual YAML/7 shell
syntax blocks and the extracted source/discovery guard passed local review checks.
The workflow compiles but never automatically executes the input generator, requires
a real separate green foundation, and runs old infrastructure separately. Feature
cases remain registered and visible; FIRST plain marker red is a separate required
operation, without WILL_FAIL, disabled cases or blanket failure suppression.

This is source/configuration review, not selected Ubuntu execution. Workflow access
remains a recorded external publication gate; old PR59 scaffold evidence is not the
new job. Require actual current selected CI after corrected-artifact review before
FIRST feature red or baseline merge, as root's harness evidence already requires.

## Actual permitted checks

Linux x86-64/GCC 16.2.1/CMake 4.4.5/Python 3.14.8/clang-format 23.1.4; installed selected
GTest 1.18.0. From this isolated worktree:

```sh
clang-format --dry-run --Werror tests/fixtures/password_hashing_vectors.hpp tests/acceptance/password_hashing_tests.cpp tests/support/password_hashing_failures.hpp tests/support/password_hashing_failures.cpp
cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug
ctest --preset password-hashing-debug --show-only=json-v1
ctest --preset password-hashing-debug -R '^(scaffold.contract_boundary|scaffold.python_tools|scaffold.project_consistency)$' --no-tests=error --output-on-failure
readelf -d build/password-hashing-debug/tests/server_password_hashing_acceptance
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed: 13 build steps/zero project warnings, 41-case discovery, three old tests
PASS/0.65s/zero failures or skips, 406 task records/122 decisions/local links/presets/
current HTML. Inline read-only Python audited frozen blobs/literals/actual Ninja
links and parsed the workflow; `bash -n` checked all seven run blocks without
executing them. The actual extracted discovery guard executed only its source
checks and CTest show-only. It ran no workflow download/build/foundation command.
Raw audit files are retained in `/tmp/subathon-sessiontest001-full-review-audit`.

One review-tool correction: an initial metadata assertion incorrectly forbade
CMake GoogleTest's standard SKIP_REGULAR_EXPRESSION. Inspected installed
GoogleTestAddTests.cmake line 101 and actual properties, then corrected that
read-only audit. No candidate file changed. There is no GTEST_SKIP, DISABLED case,
custom skip-return or failure inversion in this candidate. That audit-tool failure
is not feature red or a reason to change the baseline.

No new feature body/marker, native/password API, foundation, generator or wrapper
injection was executed. Build and LIST are not behavioral acceptance. Same actual
reviewer must assess separately authored R1/R2 corrections, preserving all valid
inputs/assertions and existing native reviews. Selected CI/FIRST red/attribution,
production and different source review, complete allocation/secret cleanup, real
account/current-login/conditional-save/admission integration, hardware cost and
platform/package/license gates remain. Root owns state/HTML/build/CI/pins and
publication. No push, PR or main merge was performed by this reviewer.

## Actual corrected candidate re-review — approved

2026-11-04, same different reviewer `/root/codec_author`. The entire original
`173d90a` review above remains unchanged historical evidence. Reviewed author
`f1b70a2c43af82795fb0da72eb620b6d5ecc4af6`, applied here as
`e720dd3207281c1cb6d9cee2c5ed5cd8eab4d2c3`. All four corrected blobs match root's
actual integration `6f039616a7900ca177f7860196fc85646dfbe707` exactly:

| Corrected artifact | Actual reviewed Git blob |
| --- | --- |
| Feature tests | `1d0d37753cd54cad189bdf9d08481838cac9880e` |
| Private observer header | `7e87bbe9c328a8791244902c7e9446ceef000996` |
| Private observer source | `4e1f049a2dcd0ff018dc3c728daad39f0773a82f` |
| Author evidence | `b6789645069ede7fdf97c255c6b5cc6e700fdff7` |

**APPROVED for this actual corrected full test/private observer/harness/CI-source
candidate. R1 and R2 are closed.** No remaining blocker was found. This is test
review, not executed behavior, selected CI, FIRST red attribution or production
approval. The valid unaffected full audit and native reviews above are preserved.

R1: `bounded_record` is SDK-free, resets its invocation's caller flag false and
sets it true only after the same six original assertions succeed. Each of the six
callers checks its own flag before every real oracle using that exact immutable
record. The creation row's correct/different-password oracles both follow this
guard. Failed rows remain failures and continue independently; no accumulated
HasFatalFailure status substitutes for current-output validation.

R2: real successful guarded allocations receive nonzero monotonic lifetime IDs;
slot/counter exhaustion records overflow without certifying a lifetime. The first
native password is bound to its pointer AND lifetime. Free retires the entry, so
address/slot reuse cannot revive it. Snapshot computes current original-allocation
liveness and checked lock state at each source gate, without a total-allocation
restriction. Successful real overlapping nonempty munlock clears lock state and
latches interruption; relocking cannot erase that interruption. Later native uses
check original lifetime and protection too. Range subtraction avoids end-address
overflow. The 16-slot constinit TLS, real delegation/error-only failures and all
whole-buffer cleanup assertions remain; no successful protection/crypto fake or
new global SDK call was added. These are source conclusions, not executed wrapper
transition tests or whole-OS memory guarantees.

Independent lexical comparison preserved all 162 ASSERT/EXPECT invocations and
their arguments, every test string literal and all 37 ordered method names. The
fixture, two native programs, logical/API/production files, CMake/preset/import/
workflow and original review bytes are unchanged. The source diff is limited to
the three assigned test/observer files and author history. SDK receipt artifacts
still match their recorded sizes/hashes. No input/oracle was regenerated.

Actual permitted checks on this corrected source:

```sh
clang-format --dry-run --Werror tests/acceptance/password_hashing_tests.cpp tests/support/password_hashing_failures.hpp tests/support/password_hashing_failures.cpp
cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug
ctest --preset password-hashing-debug --show-only=json-v1
ctest --preset password-hashing-debug -R '^(scaffold.contract_boundary|scaffold.python_tools|scaffold.project_consistency)$' --no-tests=error --output-on-failure
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed: three rebuild/link steps/zero warnings, exact 37 enabled uninverted
feature names plus old four (41 total), unchanged labels/timeouts, no generator
CTest, old three infrastructure cases PASS/0.67s/zero failures or skips, and
406 tasks/122 decisions/current HTML. Read-only preservation/discovery artifacts
are in `/tmp/subathon-sessiontest001-correction-review.ydiif6tb`. No new feature/
marker, SDK API, foundation, generator or wrapper injection body was executed.

Root actually adopted the independently approved conditional local sequencing in
`e83c704`, after environment review `d0a3819b` / import `fb3a5e2`. That supersedes
ONLY the historical remote-CI-before-FIRST-LOCAL-red sentence above. The author's
earlier not-yet-adopted statement describes its prior snapshot. Root may next
integrate this approval and apply the adopted clean-head/matching-SDK/discovery
conditions for the explicit local FIRST red, then obtain actual separate marker
attribution. No red result exists from this review. Remote CI/access/publication/
merge, production prerequisites and all account/secret/allocation/benchmark/
platform gates remain OPEN and mandatory; this note completes none of them.
