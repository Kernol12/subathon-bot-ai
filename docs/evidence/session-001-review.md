# SESSION-001 independent implementation and CI review

2026-11-05, reviewer `/root/selected_boost`, different from password source author
`/root/access_reviewer` (`password_implementer`) and shared-harness author `/root`.
Isolated `task/SESSION-001-code-review`, `/tmp/subathon-session001-review`, actual
candidate920cfaa containing author6b870c27/source import920cfaa, root CI promotion
faef49f, header descriptive-comment correction0a167d5 and SDK note35156d1.
Owned only this NEW review note. No source/header/test/fixture/build/CI/shared
progress change, source fix, baseline rerun or fixture generation by this reviewer.

**APPROVED for the actual bounded password source and current build/CI candidate.**
No blocking defect found after complete source/API/test/private-wrapper inspection,
fresh selected-SDK acceptance and the independent ownership/allocation/exception/
parser/native-link audit below. This approves neither actual account/session
integration nor an unwritten future revision. Latest automatic selected Ubuntu
results, final metadata/review/discussions/rules/merge gates remain root's work.

## Exact inspected revisions

| Artifact | Git blob |
| --- | --- |
| server/src/password_hashing.cpp | e56442511cf51ec4329f7d4a3cf94a3a093f1bb6 |
| public header, semantic declarations unchanged | 6699670861bbfa8ab49ea0c3c24517665820601b |
| logical contract | bc9ccbf66a41c9ae69abde4bd905401900e17461 |
| concrete API | 7ad1e8d3f3311673dd368f2ef4d297de31dbafcd |
| original37 tests | 1d0d37753cd54cad189bdf9d08481838cac9880e |
| private failure observer implementation | 4e1f049a2dcd0ff018dc3c728daad39f0773a82f |
| private observer header | 7e87bbe9c328a8791244902c7e9446ceef000996 |
| input fixture | 025d4de904c4c07a08cc192fc6cf94318bd5c0c8 |
| server CMake | f59247d65fa5396d19e7873d65d386ea825efd27 |
| test CMake | b9feb4f448f6d041d3f72c364c0ab79e6fb3a7e3 |
| ordinary acceptance labels | 6819bdfd329d12054074babf81b36c6a8a881784 |
| selected workflow | f97e0e1ae29ec91305a828d13e4278a10ac17680 |
| explicit SDK import | bdbe0d6d7268a7231228ec96a040f59e8686108d |

Read root/server AGENTS, workflow, SESSION001 task, complete logical/API contracts,
header/source, all37 test bodies, private observation/error controls, all literal
fixture inputs, author handoff and SDK restoration note. Independently compared
original37 tests/fixtures/wrappers/contracts against base520cc6e: all blobs exactly
unchanged. Header text differs only by its stale-stub comment; removing line comments
produces identical declaration text. Earlier PR59 test/review/observed-red evidence
remains valid and is not recreated by these implementation checks.

## Source findings beyond the existing assertions

**Borrowed preflight and exact byte handling.** All option fields, including unused
verify policy fields, validate before password/reference/record work. Password1025,
record128 and source-reference257 reject before native calls/additional copies.
Upgrade read protocol precedes source field ceilings, which precede empty/wrong
membership. Source-return allocation is correctly the trusted producer's bounded
read responsibility; this helper cannot retroactively prevent that allocation.
All PHC scanning uses byte-bounded string_views/fixed arrays, with no native parse,
unbounded strlen, owned input normalization or secret copy before admission.

The parser splits exactly six dollar fields, validates ASCII and exact parameter
order and complete canonical decimal spellings, and rejects whitespace, leading
zero, NUL/extra fields. Algorithm grammar is bounded lowercase ASCII. Fixed16/32
Base64 decoding and exact original-unpadded re-encoding reject invalid alphabet,
length and unused pad bits; accumulator payload is masked between byte/sextet
steps. Zero salt/digest are structurally legal, not verification. Invalid minima
(t0,m<8,p0) precede unsupported algorithm/version/parallelism, which precedes supported
cost ceilings and actual memory admission. Huge decimal fields are compared by
length/lexical order only after canonicality; conversion occurs only for t<=10 and
m<=262144. KiB multiplication then fits the exact268435456 ceiling. No substituted
policy cost, truncation, floating point or fallback algorithm appears. Decoded
verifier metadata uses a fixed volatile wipe without calling sodium before readiness.

**Real selected native boundary.** Readiness follows pure preflight, accepts
sodium_init0/1 and requires exact selected runtime/header1.0.22 plus ABI26/4.
The creation call is algorithm-specific Argon2id13, not ALG_DEFAULT, and offers no
fixed-salt API. Native output is scanned within128 bytes, reparsed canonically and
compared against exact requested operations/memory before std::string ownership.
Unterminated, unsupported/malformed or wrong-cost successful output becomes
native_output_invalid. Native hash failure is hash_unavailable. Verification passes
its actual stored record/cost and complete explicit-length password to the selected
SDK; native non-success remains unclassified not_successful, not a wrong-password
claim or login grant. No needs-rehash helper can downgrade a stronger dimension.

**Continuous secret lifetime and failure cleanup.** One noncopyable RAII owner
allocates max(length,1) with sodium_malloc, explicitly checks sodium_mlock BEFORE
memcpy and keeps the SAME allocation alive across verify, both actual source checks
and replacement hashing. Empty input has a nonnull protected pointer and length0.
Plaintext never enters an owning std::string/vector, returned value, exception or
private proof. Constructor lock failure explicitly clears/frees the allocated
buffer before throwing; all constructed-owner exit/exception paths clear the entire
allocation with sodium_memzero before sodium_free. Native source/output record
arrays also clear all128 bytes on exit; malformed preflight metadata has its own
fixed wipe. Source checks or result allocation cannot leak the RAII-owned secret.

Read actual selected source sodium/utils.c and current configure DEFS:
HAVE_MMAP/HAVE_MLOCK/HAVE_MPROTECT enable its aligned/page-protected branch.
Its sodium_free restores writable pages, checks canaries, invokes sodium_munlock
on the full guarded allocation and releases pages; sodium_munlock itself clears
before OS unlock. Thus the production owner can correctly free/unlock through
sodium_free without an explicit early unlock of its password pointer. The private
observer records zero bytes before free, real lock/lifetime identity and interruption
including address reuse; it suppresses SDK-internal recursion, not product calls.
No global operator-new replacement or successful-native fake is introduced.
This is not a claim to wipe caller ingress, all registers/Argon scratch or crash
images, nor to make a fatal SDK fault recoverable.

**Actual source binding and outcome partition.** Upgrade reads once into an owned
immutable current credential snapshot and verifies it anew. Only native success
can construct its private noncopyable borrow, bound to both that exact snapshot/
attempt and source-object address. String equality cannot replace belongs_to.
Native failure makes no gate/hash call. First denied/stale/unavailable/invalid
source result maps distinctly; source exceptions lose private diagnostic text.
Already-strong records require just the first accepted check and remain unchanged.
Otherwise componentwise maxima preserve stronger memory/time costs; insufficient
joined admission returns upgrade_unavailable/resource_limit without hash work.

Accepted-generation hash/output/allocation failures yield upgrade_unavailable;
failed secret allocation/lock remains secret_memory_unavailable. The second actual
source check is outside the generation-error catch and cannot be swallowed as a
hash failure. A second denied/stale/unavailable/protocol/exception result discards
the new record and never returns replacement_ready. Only two accepted checks allow
complete copies of account/attempt/expected revision/FULL original verifier/new
verifier; every field already fits its complete bound. Replacement materialization
bad_alloc/length_error maps upgrade_unavailable/allocation_unavailable. Earlier
ordinary module allocation failures map typed allocation_unavailable, guarded
allocation has its distinct code, and port bad_alloc maps separately from other
port exceptions. Exception construction can itself fail as the frozen API explicitly
allows; it cannot produce a successful result. The module has no persistence or
write-capable source; post-return atomic login/current-source CAS remains mandatory.

## Independent selected checks and CI/link audit

Host GNU16.2.1/C++20, Python3.14.9, CMake4.4.5, Ninja1.13.2, GTest1.18.0,
Linux x86-64; explicit restored static libsodium1.0.22 SDK. Independently rehashed
all three artifacts against original sessiontest-001-sdk-build.json: archive
836744bytes/SHA959cdbe83cd0c80264e080efdd1e861b20b57ded2a85bd9a924297b8a10f1935,
version.h and libsodium.pc ALL match the already reviewed proof exactly. This is
current restoration verification, not a fabricated new source signature/FIRST run.

```sh
cmake --preset password-hashing-debug \
  -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug
cmake --build --preset password-hashing-debug --target password_hashing_fixture_generator
# RLIMIT_CORE=(0,0), output retained before decoding:
ctest --preset password-hashing-debug --no-tests=error \
  --output-junit /tmp/session001-review-checks/native41.xml
clang-format --dry-run --Werror server/src/password_hashing.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0. Fresh configure/build13 steps and explicit generator BUILD2 steps have
no warnings; generator was NEVER invoked. Full41/41 PASS,0 skipped/disabled/failures,
CTest3.51s: unchanged37 password methods, old3 infrastructure and1 current selected
SDK environment. No supported-hardware benchmark or timing guarantee follows.
Raw stdout SHA2568e12410b2d4da0b247be444db903628e0493b63281ee9cdec748f12de8d27426;
JUnit SHA25684f5d161da76552b062bf796e880dfacb6299d0819cac1ee10da5a87664670ae.
Receipt/raw/JUnit/discovery/link commands remain `/tmp/session001-review-checks/`.

Executed the actual workflow's embedded37-case discovery guard against this fresh
build. YAML parsing/all shell bodies `bash -n`/embedded Python compilation pass.
Discovery has41 enabled/uninverted cases, exact37 methods/labels/timeouts, no custom
result rewriting and no generator CTest entry. Workflow retains exact authenticated
source SHA/size, static disposable SDK, pinned checkout/GTest fallback, least contents
read permission and mandatory standalone environment then unconditional37 acceptance
and old infrastructure commands; no intentional-red exclusion/inversion/continue-on-error.
Source/output tests are now ordinary acceptance; historical original red remains.

Read actual Ninja target-specific link commands and nm symbols: only the feature
TEST executable has exactly12 private GNU wraps; production static module, standalone
foundation and generator contain no password_test/__wrap symbols. Both standalone
programs link exact selected static SDK without project/GTest; production module
also uses that explicit imported SDK, not a dynamic/system fallback. An initial
read-only default-Ninja command listing omitted EXCLUDE_FROM_ALL's generator rule;
requesting that actual target's commands resolves it and the isolation audit passes.
No build/discovery operation executed the generator or a feature body. Actual41
execution above is the ordinary fresh implementation check, not FIRST baseline red.

## Remaining integration/release gates

Approval is limited to these exact source/config artifacts and observed local
component acceptance. Root must observe actual automatic current selected Ubuntu
CI and latest-head final gates before merge. Actual account/login source, atomic
conditional store/session authority/recovery, worker/admission/cancellation and
plaintext ingress cleanup remain owning tasks. Supported Linux hardware100-sample
cost measurements, dump/swap policy, platform/package/license/live verification
are unperformed. No provider/chat/payment/deployment side effect occurred; no real
credential was used. All wider product scope remains. This sole review note does
not update task completion, public capability, PR approval count or merge status.
