# SESSION-008 — independent source and configuration review

Author `/root/session_supplier_implementer`; different reviewer
`/root/session_supplier_code_review`, 7 November 2026. Isolated worktree
`/tmp/subathon-session008-review`, branch `review/SESSION-008-source`, initially
clean. This reviewer owns only this note and authored no implementation, test,
fixture, contract or CI change.

## Actual packet and verdict

**APPROVED for integration at `2cf5ba581135c43443a362f28a4cc9b0213e0f14`.**
No blocking source, resource, secret-lifetime or configuration finding remains.
Implementation author `26f275fa`, import `028f83b`; coordinator `2cf5ba5` promotes
ordinary acceptance labels and unconditional selected-SDK CI execution. This is
approval of actual written code/configuration with independent local acceptance,
not an assertion that latest-head hosted CI, publication or merge has occurred.

Read root/server/tests instructions, workflow, architecture/test strategy,
[task](../tasks/EPIC-007/SESSION-008.md), [current handoff](session-008-handoff.md),
[author evidence](session-008-author.md), the accepted supplier and the complete
39-case baseline/fixtures/native program. The independent
[baseline/native review and FIRST-red attribution](sessiontest-008-review.md)
remain valid; their original receipts were not replaced or replayed. Specification
v0.45 §10, AUTH-05/12 and TEST-04/05; accepted supplier contract blob
`c5f5a6e71664042e18475068d3caf577b0c8d009`.

| Reviewed artifact | Git blob |
| --- | --- |
| Implementation | `0525d75537d890a1dc00bb940b5cefd709c8d6c0` |
| Unchanged public header | `0c0f5d30647e525d6c9bd1d5daeb1b5288a3b5b8` |
| Unchanged 39 tests/private native observer | `fbdab51f268d78ea418f3ab145e96e87540dd3aa` |
| Unchanged literal fixture | `84b86c678415ca5ac8306efd4ba12d42ac0906ab` |
| Unchanged SDK foundation | `34f3bbd904d0e28254b631440f8d773072b6a3de` |
| Ordinary labels | `d5df833230b5d10a14de7230da9ce827d0d4b98f` |
| Selected-SDK workflow | `1dfef3aad5b974c6208081e2929ea9781338fef2` |

## Preparation, secret lifetime and native use

Wrong lengths return before character access or scratch construction. The canonical
path reads each of the64 borrowed input characters once. Its first32 text bytes use
raw32 and its remaining32 use frame60; decoding starts only after complete lexical
admission. Forward in-place decoding cannot overwrite an unread pair. The second
half is decoded from frame into raw's remaining16 bytes, then frame is completely
replaced by the exact28-byte prefix including one NUL and raw32. No extra owning
token text, decoder output, dynamic buffer or queue is introduced.

The digest is constructed directly in the one output variant. During hashing,
raw32+frame60+digest32 give124 bytes of active application byte buffers. The nested
scratch scope ends before a possibly non-elided successful variant return, so that
handoff cannot overlap a second digest with raw/frame. Failure returns construct
only the enum alternative, not another digest buffer. Static nonsecret prefix,
fixed scalars/discriminants and native object overhead are separate fixed storage.
The scratch destructor clears both entire arrays using sodium_memzero on partial
lexical failure, initialization failure, hash failure and success, and during any
ordinary C++ unwinding after construction. Native C calls add no C++ exception
producer. The caller still owns erasure of its original transport secret.

Initialization accepts only0/1; hashing is the selected real unkeyed single-part
crypto_generichash with input60/output32, no native success fake or fallback.
Resolution compares selected digests with sodium_memcmp over the fixed32-byte
array, without claiming whole-request constant time. The unchanged selected static
archive SHA256 independently matches
`f8e295ad9a6d8c6d3c657e4d2f33d590da161f6ed5a81dfae09d1067fab3d017`.
The earlier separate selected-native hash work/stack audit applies to this same
archive/profile;124 is not a total stack/process/initialization/runtime bound.

## Resolution, allocation and execution boundaries

Positive limits, individual string max_size/terminator arithmetic and the
representable min(UINT64_MAX,SIZE_MAX)/3 aggregate ceiling are checked before either
producer. Aggregate owned_bytes is not incorrectly treated as one string's
max_size. Remaining-budget subtraction proves each cumulative sum safe before
addition. Request size precedes its empty-shape check. All12 complete-fact
occurrences, including duplicate values, are preflighted before shapes, bindings,
valid state and time order; all five result occurrences are projected/preflighted
before any result string grows. Reference view arrays have disjoint scopes and
iteration borrows by reference, preserving the at-most12-view bound.

The source's prvalue initializes one owned local variant without a complete-facts
copy. Validation borrows it; the output strings are copied once directly into
their result variant and the returned variant is elided or moved. There is no
additional complete owning DTO. Request, facts and result therefore retain their
separate per-value payload ceilings and3× simultaneous payload bound. Partial
result construction unwinds through owned strings; allocation and all other
unclassified exceptions return unavailable without exposing diagnostic text.
The actual source must still preflight storage before producing its snapshot;
consumer validation does not retroactively establish that producer obligation.

Source is called exactly once after request admission. Explicit missing, invalid,
resource and unavailable variants stop before clock. Every malformed shape/link,
digest mismatch or invalid stored time order precedes revocation. Revoked state,
disabled access and either epoch mismatch precede the sole clock call. Clock
refusal/rollback precedes exclusive expiry, then freshness and optional challenge.
After signed ordering proves now>=authenticated, unsigned64 subtraction is the
exact full-domain nonnegative age; it performs no signed subtraction or narrowing.
The five-minute boundary includes300000ms, and stored timestamps are preserved.

Each execution reloads current source facts and clock. No caller identity flag,
snapshot/reference, previous DTO or source/clock result becomes authority for a
later scope. No new API, dependency, SQL, real clock, login, lifecycle or transport
behavior is introduced. Real queue/snapshot/clock trust remains with the host.

## Configuration and independently observed checks

The CI delta removes intentional-red labels only from these passing39 cases and
requires exactly {'session-resolution','acceptance','server'}. Its actual Python
guard passes against reviewer discovery: exact39 unique enabled/uninverted methods,
one separate SDK foundation and43 total registrations. Existing pins/check name/
old suites remain. The ordinary workflow requires SDK1, then unconditionally runs
all39 using the same preset/label; no manual body gate, failure inversion or blanket
continuation is added. Hosted execution and required-check enforcement remain
coordinator gates.

Linux7.2.14-arch1-1 x86-64; GNU16.2.1 (20260810), CMake4.4.5,
clang-format23.1.4, Python3.14.9. Commands actually run in this worktree:

```sh
cmake --preset session-resolution-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-session-sdk/prefix
cmake --build --preset session-resolution-debug
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -Iserver/include -I/tmp/subathon-session-sdk/prefix/include -fsyntax-only server/src/session_resolution.cpp
clang-format --dry-run --Werror server/src/session_resolution.cpp
ctest --preset session-resolution-debug --show-only=json-v1 > /tmp/subathon-session008-review-discovery-sequential-clean.json
ctest --preset session-resolution-debug -L '^session-resolution$' --no-tests=error --output-junit /tmp/subathon-session008-review-feature-sequential.xml
ctest --preset session-resolution-debug --no-tests=error --output-junit /tmp/subathon-session008-review-full.xml
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All return0: independent12-step build/strict syntax/format without warnings,
exact43 registrations, feature39 PASS0skip in0.05s, full43 PASS0skip in0.73s.
Independent JSON/JUnit checks verify unique counts and zero failures/errors/skips;
the workflow's actual session-discovery Python block also executes successfully.
Project check passes443 records/122 decisions and current HTML. No historical red
or live provider operation was run.

One reviewer execution error is excluded from acceptance evidence: initial CTest
discovery and feature execution ran concurrently against their shared PRE_TEST
generated file. That race duplicated39 registrations and ran78 passing bodies.
I removed only the disposable generated discovery file, regenerated sequentially,
then obtained the exact43 inventory and39/full43 results above. The initial local
JSON/XML remain distinguishable; no tracked source or original receipt changed.

## Remaining gates

Latest-head hosted CI, actual required reviews/rules/discussion resolution and
normal PR publication/merge remain. Real Qt/SQL bounded snapshot production,
trusted clock/recovery, accepted login/permissions, issuance/RNG/lifecycle,
transport, protected client storage, platform/security and full release remain
their owning tasks. External libc/loader/first-init/global process limits and
register/swap/crash-dump protection are not certified by this component review.
All seven integrations and three exchange-rate providers remain in release scope.
