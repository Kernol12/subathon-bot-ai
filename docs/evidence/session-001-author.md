# SESSION-001 password primitive implementation handoff

2026-11-05, author `/root/access_reviewer` in its newly assigned
`password_implementer` role, independent of the earlier ACCESS-002 review.
Isolated `/tmp/subathon-session001-author`, `task/SESSION-001-resumed-author`,
base520cc6e (preserved original SESSION001 claim plus current main).
Owns only `server/src/password_hashing.cpp` and this note. The public API,
contracts,37 reviewed tests, private wrappers and input fixtures are unchanged.

## Implemented boundary and retained baseline

Read root/server instructions, workflow, SESSION001 brief, complete logical/API
contracts, header,37-case source/private observers/input records and
[reviewed baseline handoff](sessiontest-001-handoff.md). Its PR59 independent
reviews/original37-case intended red remain valid and were not repeated. No input
generator or expected-result generator was run. The previous source-authenticated,
independently runtime-proven libsodium1.0.22 artifact was restored by the root;
this implementation uses that explicit static SDK, not an installed dynamic fallback.

All three entrypoints now implement bounded creation, standalone verification and
source-bound successful-login upgrade proposals. No account login/session issuance,
authority, durable conditional save, credential reset or write-capable port is
supplied. The caller's actual atomic source/attempt/account conditions still govern
saving any returned replacement; an evaluated result is not a reusable save ticket.

Borrowed preflight validates all option fields first, then password/reference or
record byte ceilings, complete PHC framing/ASCII/canonical decimals, fixed16/32
Base64 decode and exact unpadded re-encoding, malformed minima, supported
algorithm/version/parallelism, cost ceilings and actual memory admission. No
unbounded integer conversion, strlen on borrowed input, native parser, sodium call
or protected password copy precedes those checks. Parser fields are borrowed views;
fixed decoded verifier metadata is cleared using volatile byte writes before any
native readiness is required. Admitted numeric conversion is bounded by10/262144.

Readiness checks sodium_init result0/1, exact runtime/header1.0.22 and ABI26/4.
Creation uses the explicit algorithm-specific Argon2id13 encoded API (v19,p1,
internal random16-byte salt/32-byte digest). Successful output is bounded-scanned
within128 bytes, strictly reparsed and checked against exact requested/joined
costs before any output ownership. No default algorithm, weaker fallback or fixed
salt is exposed. Native verification0 observes verified; non-success remains
unclassified, never proof of a wrong password or accepted account login.

One noncopyable RAII owner allocates max(password bytes,1), checks explicit mlock
BEFORE memcpy and keeps that same protected allocation continuously alive through
verification, first accepted-source check, replacement hashing and second source
check. Every exit/exception wipes the entire owned allocation with sodium_memzero
before sodium_free; actual selected utils.c unlocks guarded memory during free.
Lock-failure construction explicitly clears/frees its allocated memory before
throwing. Native128-byte source/output scratch is RAII-cleared too. No helper
claims it erased earlier caller buffers, registers or native Argon internals.

Upgrade reads current source once into an owned immutable bounded snapshot, checks
state/optional/field/membership rules, freshly verifies and creates its private
source-object-bound borrow only after native success. Source exceptions discard
private diagnostics and map allocation separately. Denied/stale/unavailable gates
remain distinct. Both stored dimensions already strong enough return not_needed;
otherwise componentwise maxima prevent downgrade. Insufficient joined admission or
hash/output/materialization failure returns upgrade_unavailable without changing
the accepted login or old credential. Second-source check errors remain typed
source failures, rather than being swallowed by the generation-failure mapping.
Only two accepted checks permit complete source-bound replacement ownership.
Native scratch holds the new record until that second check; no partial ready
result or write is returned on races. Fixed source/output ceilings establish the
complete credential/read/replacement bounds before additional copies.

Ordinary allocation/length failures map to allocation_unavailable, with the
accepted-generation/materialization outcome partition required by the API.
Guarded allocation/lock failures always use secret_memory_unavailable. Fixed
failure text never includes a password, record, account or native diagnostic.
The standard allocation required to construct a runtime_error may itself fail,
as the frozen API explicitly acknowledges; no successful result follows.

## Actual selected checks

Root restored `/tmp/subathon-sodium-1.0.22-sdk/prefix` from the exact previous
authenticated archive. Root reports archive/static-library hashes match preserved
source/native evidence; actual static archive SHA256 is
`959cdbe83cd0c80264e080efdd1e861b20b57ded2a85bd9a924297b8a10f1935`.
Read selected version/Argon2id headers and source `sodium/utils.c` allocator/free
and `crypto_pwhash/argon2/pwhash_argon2id.c` algorithm-specific implementations.
No global install, vendor suite, fixture generator or FIRST foundation replay claim.

```sh
cmake --preset password-hashing-debug \
  -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug
ulimit -c 0
ctest --preset password-hashing-debug --no-tests=error \
  --output-junit /tmp/session001-author-native41.xml
clang-format --dry-run --Werror server/src/password_hashing.cpp
git diff --check
```

All exit0. Initial build13 steps, then final3-step rebuild after removing an inert
borrowed-read no-op; no warnings. Actual final working-candidate full41/41 PASS,
0 skipped/disabled/failures, CTest3.57s: original37 real selected-library cases,
old3 infrastructure and one current SDK environment. This ordinary environment
execution preserves its original FIRST receipt and generated inputs unchanged.
Original labels still say intentional-red in this author branch; root owns the
separate ordinary acceptance/CI promotion. Passing implementation acceptance does
not rewrite historical stub-red evidence. JUnit details/hashes below refer to
this actual run, not a benchmark or selected Ubuntu receipt.

## Remaining gates

Actual DIFFERENT source/secret-lifetime/allocation/error/native-call review remains
mandatory, including any behavior not measured by these public tests. Root owns
shared labels/build/CI, descriptive stale-header comment, state/map/HTML/checkpoint
and publication. Current automatic selected Ubuntu37 acceptance, integration checks,
completion metadata/latest-head CI/rules/discussions and normal PR merge remain.
Current account/session/atomic conditional-store/admission/worker cancellation,
complete plaintext ingress cleanup, supported-hardware100-sample cost benchmark,
platform/package/license/live release verification are separate gates. No real
credential/supporter/provider/chat/payment/deployment side effect was performed.

Actual source blob `e56442511cf51ec4329f7d4a3cf94a3a093f1bb6`. JUnit SHA256 `fe7c27596938874af63fb0a13afe6427b1d573f6632ec5f960595e052d9f6ac0`; tests41/failures0/disabled0/skipped0.
