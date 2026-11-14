# Bounded password hashing and successful-login upgrades — proposal v1

SESSIONTEST-001 FIRST logical document stage, from main dd6a141 and claim a7b0d4f.
Authority: specification v0.45 section 10/AUTH-11/S62,
[SESSION-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-007/SESSION-001.md) and the
[record/application ownership boundary](event-state-boundaries-v1.md).
This proposes behavior and independent examples only. It creates no C++ API,
header, schema, fixture, test, hash, executable foundation or implemented login.
Different actual logical/API/source/test reviews and an intended behavioral red
remain required before production SESSION-001.

## 1. Responsibility and selected dependency

Use libsodium Argon2id, with explicit Argon2 version 1.3 and encoded version 19.
The proposed selected dependency is libsodium 1.0.22. Root must separately pin and
verify actual source/package artifacts, build/link/runtime identity and supported
server environments before the native foundation. The author's host pkg-config
and installed headers report 1.0.22; no runtime version, initialization, hashing,
verification, random generation or memory-lock operation has been executed here.
Header presence is not proof that a future binary loads that library.

The password-storage operation uses libsodium's Argon2id encoded-string facility,
not a home-grown digest, raw-key derivation composition, encryption key, session
token hash or client-side prehash protocol. Select the explicit Argon2id13 algorithm
instead of relying on a future ALG_DEFAULT. The algorithm-specific API can also
provide that explicit selection. The installed and official release headers expose
16-byte salts, a 128-byte encoded-output buffer and explicit-length password inputs.
These are library facts; the smaller application limits below are proposals.
[Official 1.0.22 header](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h).

Libsodium's stored string carries the salt and verification parameters; its verifier
uses those stored values. Verification returns 0 on success and -1 on error, so a
failed call alone does not establish the reason. Parameter choice requires actual
hardware measurements. [Official password-storage documentation](https://doc.libsodium.org/password_hashing/default_phf).

This slice handles bounded byte inputs, safe record preflight, password verification
and a replacement-hash proposal after successful login. It authenticates no account,
grants no session/permission and performs no database write. Account state, fresh
authentication, recovery, password changes and account enumeration protection remain
ACCESS/SESSION-003 and the HTTP boundary. One caller owns authoritative credential
revision checks and persistence. No provider, email or external authentication
service is introduced.

## 2. Password bytes and local engineering limits

A password is an explicit byte sequence with its length. Preserve all bytes,
including embedded NUL, leading/trailing spaces and non-ASCII. Do not trim, case
fold, normalize Unicode, transcode, terminate at NUL or truncate to fit. User-input
text encoding belongs to its separately reviewed client/transport boundary.

The primitive accepts 0..1024 password bytes. Empty input is representable crypto
data, not an account password-strength policy or permission to create an empty
account password. No minimum length, complexity rule, expiry rule or silent password
change is invented by this contract. Reject 1025 bytes before copying or native
crypto work; a larger input is not hashed as a 1024-byte prefix.

Proposed v1 limits and policy, fixed for this logical revision:

| Item | Rule |
| --- | --- |
| Password bytes |0..1024 inclusive|
| Encoded verifier bytes, excluding terminator |1..127 inclusive; no embedded NUL|
| Algorithm/version |Argon2id13; explicit encoded v=19|
| Stored profile accepted for verification |t=1..10; m=8..262144 KiB; p=1|
| New-hash desired cost |t=2..10; memory 64..256 MiB in whole KiB|
| Proposed initial desired cost |t=3, memory 67108864 bytes, encoded m=65536|
| Salt/digest decoded sizes |16/32 bytes|
| Caller admission before invoking a hash/verify |Initially one active expensive call; bounded admission owned by login service|

These are local resource/compatibility decisions, not libsodium's maximum values,
measured security strength, measured latency or provider limits. The library
headers admit much larger costs/passwords. Lower-cost accepted stored records
support verification followed by upgrade; new records never use that lower floor.
No previously deployed project credential format is claimed. Argon2i/scrypt/other
versions/parallelism or different salt/digest sizes are outside this proposed v1
profile, even if a general-purpose library can verify them. Such records require
an explicit reviewed compatibility/migration change; never reinterpret them as v1.

Cost inputs are exact integers in their declared units. Reject an invalid desired
cost before work. Memory bytes must be divisible by 1024, so the native conversion
cannot silently round down. Stored m is KiB; multiply by 1024 only after checking
the small admitted range and representability. In particular 262144 KiB means
268435456 bytes, not 262144 bytes. No numeric wrap, saturation or substitution with
an interactive constant is allowed.

The caller admits work before retaining/enqueuing more password buffers. It budgets
concurrent native memory plus library/worker overhead and existing application RAM,
using the actual stored cost for verification and target cost for creation. This
primitive does not create a scheduler, unbounded queue or global concurrency pool.
Do not run expensive hashing on the authoritative Qt writer or block state commits
on a network callback. Actual worker/admission/cancellation integration is later
SESSION-003/API work. Native hashing has no claimed cooperative hard deadline:
caller timeout must not destroy buffers while an in-flight function still uses them.

## 3. One self-contained verifier record and safe preflight

The logical durable value is the complete owned encoded ASCII verifier string.
Salt, algorithm, version and costs are already inside it; do not persist an
independently editable second parameter set that could contradict the record.
Account identity/credential revision and any future SQL envelope remain the
owning account/store contract. This document adds no migration or transport schema.

The accepted canonical record shape is:

```text
$argon2id$v=19$m=<KiB>,t=<passes>,p=1$<salt-base64>$<digest-base64>
```

Require exact case, delimiters and parameter order, canonical decimal fields with
no plus/leading zero, and no trailing text, newline, whitespace or extra field.
Salt and digest use original Base64 alphabet without padding: 22 and 43 characters
respectively, decoding to 16 and 32 bytes. Reject invalid alphabet/unused pad bits;
canonical re-encoding must reproduce each field exactly. Zero-valued salt/digest
bytes are not by themselves malformed; structural admission proves no password
match or salt randomness. The release encoder/decoder uses explicit parameters,
original unpadded Base64 and a whole-record parse.
[Official 1.0.22 encoding source](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/argon2/argon2-encoding.c).

Preflight is mandatory BEFORE invoking any native verifier or needs-rehash helper:

1. Validate desired-cost configuration; check password and record byte ceilings
   before copying, decoding or native work. Overlong inputs are resource_limit.
2. Check nonempty ASCII/no-NUL record and complete field/token structure. Malformed
   delimiters/numbers/Base64, noncanonical encodings, zero t or m<8 are invalid_record.
3. An identifiable other algorithm, non-19 version or positive p other than 1 is
   unsupported_record, not a password mismatch. Do not invoke a fallback verifier.
4. Compare admitted positive cost fields against the t/m ceilings using bounded
   checked arithmetic; excessive values are resource_limit before multiplication,
   allocation or crypto. Huge decimal cost fields never pass through overflowing
   integers. Decode only the fixed-size salt/digest fields and complete canonical
   checks before accepting the record for native verification.
5. Copy an accepted record into a checked 128-byte zero-terminated native buffer.
   No unbounded strlen or native parse is performed on borrowed untrusted input.

The exact C++ error API/order within same-category malformed fields is a later
reviewed boundary. Record/algorithm/cost errors remain distinct logical outcomes;
no truncated record, guessed default or partially parsed credential is accepted.

The explicit creation source generates its salt internally and writes the encoded
record with a 32-byte digest. It passes memory to Argon2 in KiB and uses p=1.
[Official 1.0.22 Argon2id implementation](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c).
Production creation exposes no caller-selected fixed salt. A native success must
also produce a complete record matching the requested admitted profile; otherwise
return failure, never a successful empty/partial verifier. Preserve every stored
encoded byte; do not reconstruct a different verifier when reading it.

## 4. Verification and successful-login upgrade sequence

Initialization readiness belongs to the selected runtime before crypto/secure
allocation. sodium_init returning 0 or 1 is ready; failure means unavailable, with
no hashing fallback. [Official initialization documentation](https://doc.libsodium.org/usage).

Creation has a fresh library-generated random salt on each invocation. Equal
passwords need not have equal verifier bytes. Tests must not obtain expected
random output from the implementation or pretend salt inequality is a deterministic
mathematical guarantee. Future verifier facts use independent published synthetic
vectors; new-record tests verify profile and actual password behavior separately.

Verification of an admitted record uses the complete explicit-length password and
the record's own parameters. A native 0 is verified; a native -1 is
verification_not_successful, covering mismatch or an unclassified native failure.
Do not diagnose every-1 as wrong_password, infer a password match from parsed
metadata, or rely on errno text to distinguish all failure causes. No error path
grants authentication or triggers a replacement. Native creation failure is
hash_unavailable. Allocation/init/protection failures are explicit failures; no
weaker hash, plaintext cache or fake verifier is returned.

This is a logical sequence, not a chosen C++ function/certificate design:

1. The caller reads the actual current credential/revision and verifies it. A
   non-success or preflight failure produces no upgrade/write.
2. The account/session service finishes all other required login checks and
   rechecks that the verified credential revision is still current. Password
   verification alone is not an accepted application login. If login is denied,
   no rehash is initiated or persisted by this sequence.
3. After actual successful login, compare admitted stored costs with the selected
   desired costs. Rehash is needed when either stored dimension is lower. Choose
   replacement t=max(stored t, desired t) and m=max(stored m, desired m), preserving
   p=1/algorithm/version. Thus no existing dimension is silently reduced. If both
   stored dimensions already meet/exceed desired, keep the exact record unchanged.
4. While the original short-lived password buffer is still available, create the
   required replacement with a fresh salt. On failure retain the old credential
   and report upgrade_unavailable to the caller; do not claim an upgrade occurred.
   The already accepted login is not retrospectively a wrong-password event.
5. The caller conditionally persists the replacement only if the actual credential
   revision/record still equals the verified source. A concurrent password reset
   or change wins; never overwrite it. This proposal owns no SQL transaction,
   retries, session issuance/revocation or caller authorization. A stale/missing
   source cannot be rescued by a caller-created boolean or opaque reference.

Componentwise maximum is a conservative no-cost-reduction engineering rule, not
a claimed total security ordering of all algorithms. All admitted source/desired
costs keep the replacement within this revision's ceilings. Libsodium's
needs-rehash facility detects parameter mismatch, including lower requested costs;
it is not authority to downgrade. The pinned dispatch source and upstream tests
show parameter checking separately from successful password verification.
[Official dispatch source](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/crypto_pwhash.c).

## 5. Secret lifecycle and diagnostics

Passwords never enter durable records, backups, application logs, traces, metrics,
URLs, CLI arguments, fixtures containing real user data or diagnostic exports.
The encoded verifier is also sensitive account material, not a viewer field or
routine log value. Never expose its salt/digest or include supplied password/record
bytes in errors. Return fixed bounded categories; remote login responses and timing/
enumeration/rate-limit policy remain SESSION-003/API responsibilities.

Borrow password storage only during the synchronous operation. Minimize additional
copies; any unavoidable owned copy uses bounded guarded memory, with an explicit
checked lock requirement. If required allocation/locking fails, fail the operation
rather than claim protected storage or fall back to a file. Release owned buffers
on every return/exception with libsodium's non-elidable clearing; retain the same
protected short-lived buffer across verification/login/upgrade only as long as
those caller-owned stages need it. Caller-owned ingress/UI/network copies must be
cleared by their owners; a helper cannot erase an unknown earlier copy. A returned
value retains no password, password digest shortcut, borrowed buffer or diagnostic
copy. Future concrete API review must pin ownership/lifetimes and actual cleanup.

Libsodium provides explicit clearing and guarded allocation, but successful guarded
allocation alone does not prove memory locking succeeded; locking can fail. These
mechanisms do not erase stack/register/earlier copies or guarantee absence from
swap/hibernation/crash images. Deployment must separately validate dump/swap policy.
[Official secure-memory documentation](https://doc.libsodium.org/memory_management).

No timing-safe whole-login claim follows from using a crypto verifier: record
preflight and cost profiles have different observable runtimes. No account/session
provider secret, password-strength policy, pepper-key service or HTTP security
configuration is supplied here.

## 6. Independent literal examples

All inputs below are public synthetic data or byte/format cases, never actual
credentials. H0 is the complete upstream tv3[2] encoded vector for an EMPTY byte
password; source and expected output are independent of this project. Its expected
verification is published upstream, not observed in this authoring stage:

```text
$argon2id$v=19$m=4882,t=2,p=1$bA81arsiXysd3WbTRzmEOw$Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE
```

H0 is 96 ASCII bytes, with 16-byte salt/32-byte digest. The upstream expected-output
file records failures for tv3[0]/[1]/[3], while this vector succeeds. These are
published third-party facts, not new project tests or a claimed local crypto run.
[Upstream synthetic vectors](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.c),
[upstream expected output](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.exp).

Unless stated otherwise, desired t=3/m=65536 KiB and full v1 admission apply.

| Case | Literal input / required behavior |
| --- | --- |
| PH01 | H0 + empty password: expected verified; after full login success, replacement target t=3/m=65536 KiB. No record is generated in this document stage |
| PH02 | H0 + one byte 00: a different password from empty; expected verification_not_successful, never truncate at NUL or upgrade |
| PH03 | Raw bytes 61 00 62 and 61 are distinct; preserve all supplied bytes/lengths. No predetermined generated hash is invented |
| PH04 | UTF-8 bytes c3 a9 and 65 cc81 are distinct; no NFC/case/whitespace transformation |
| PH05 | Password 1024 bytes fits input ceiling; 1025 refuses before crypto/copy. No strength claim or successful hash is inferred from length alone |
| PH06 | Append LF or embedded NUL to H0: invalid_record, no native verifier |
| PH07 | H0 with m=04882, missing p, extra field, padded/invalid Base64: invalid_record, no repair |
| PH08 | H0 with v=16 or p=2, or recognized argon2i prefix: unsupported_record, no alternate algorithm |
| PH09 | H0 with t=0 or m=7: invalid_record; t=11 or m=262145: resource_limit BEFORE native work |
| PH10 | 128 record bytes or an enormous decimal cost: bounded refusal, no unbounded parsing/native allocation |
| PH11 | Desired t=1, t=11, memory 67108865 bytes, or memory below 64 MiB: invalid_options BEFORE password work |
| PH12 | Successfully verified source t=6/m=32768 KiB, successful login: target t=6/m=65536, never t=3 |
| PH13 | Verified source t=6/m=131072 KiB with desired 3/65536: no replacement; preserve encoded bytes |
| PH14 | Wrong/unverified password, invalid record or a correctly verified password followed by denied login: no upgrade/write |
| PH15 | Successful login needs replacement but its allocation/hash fails: old record unchanged, explicit upgrade_unavailable; no false upgraded result |
| PH16 | Accepted login's source revision changes before replacement save: do not overwrite the newer credential; caller records stale outcome |
| PH17 | Runtime initialization or required secret-memory protection unavailable: fail visibly/bounded, no plaintext file or weaker-cost fallback |
| PH18 | Caller uses a smaller admission memory budget than this stored record requires: refuse before expensive work; never verify with substituted costs |

Future baselines must independently review the actual vectors, complete parsing/
cost boundaries, raw-byte/NUL behavior, real selected-library verification and
upgrade timing/failure/concurrency. Upstream examples/shape checks are not a project
foundation or observed feature-red substitute. Tests may use public synthetic byte
vectors; real passwords/tokens/supporter data must never enter source or logs.

## 7. Supported-hardware benchmark procedure — not executed

Run only after the actual dependency/native foundation and a separately reviewed
benchmark harness exist. The default CI correctness suite is not a hardware-cost
certification. Use disposable public synthetic input and private temporary records;
never real account passwords, command-line secrets or credential logs.

Record CPU/model/features, available RAM and deployment/container limit, OS/kernel,
compiler/build mode, exact libsodium artifact/header/runtime versions, thread/worker
admission, selected t/m and measured process memory. Cover supported Linux x86-64
server installation profiles; Windows/Linux clients do not establish server cost.

Measure cold initialization separately from hash work; then fresh creation, correct
verification, unsuccessful verification and successful-login replacement at admitted
current/legacy/target costs. Include the componentwise joined upgrade cost. Use
warm-up followed by at least 100 sequential samples per measured profile, then the
configured maximum concurrent login load alongside ordinary application activity.
Report sample counts, median/p95/p99/max wall time, memory peaks, failures and any
incomplete run. No known mismatch or unsupported profile is reported as verified.

Initial engineering budget proposal is p95 at most 1 second for admitted online
work at chosen concurrency, subject to explicit supported-hardware validation.
This is not a measured result or a hard cancellation deadline. Account for at most
one active hash initially and native overhead; do not extrapolate memory solely
from m. If budget/RAM is unsuitable, revise supported hardware/admission or select
an explicitly allowed cost under review and remeasure; never silently weaken an
existing record, raise caps or label an unmeasured machine supported. Operation
shutdown/overload integration remains later API/SESSION-003 work.

## 8. Remaining stages and scope

No new product conflict was found. The byte/cost/profile/upgrade choices above are
explicit engineering proposals for different review. Account minimum/complexity
policy remains separate rather than an unanswered requirement invented here.
Next: actual logical review; concrete owned C++ errors/options/secret lifecycle
review; minimal distinct marker and inert harness; independent fixtures/tests and
their review; real selected-library foundation green; FIRST intended behavioral
red; production implementation and different source/CI review.

Root owns dependency pins/build/CI, task state/HTML/checkpoint and publication.
The native library is agreed already; no replacement dependency is proposed.
SESSION-001 remains planned, SESSIONTEST-001 incomplete. Session issuance/revocation,
credential/account persistence, permission/recovery/reset, platform secure stores,
network/login security and full-release verification retain their owning tasks.
