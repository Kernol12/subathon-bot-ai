# SESSIONTEST-001 — Independent concrete password API document review

Reviewer: `/root/service_reviewer`, 2026-11-04. API author: `/root/service_author`.
The reviewer authored the earlier logical contract, but did not author this API.
This review independently checks the new API against that frozen, differently
reviewed logical input; it does not renew or substitute for the logical review.
Dedicated branch `task/SESSIONTEST-001-api-review`, isolated worktree
`local-evidence/worktrees/sessiontest001-api-review`, initially clean.
Only this new review note is owned/edited.

## Exact candidate and verdict

Actual author commit `fca99279355ea31d6249f6e5fe83eaaa2cfde023`, imported at
`775f08409b2134bbfc3579ac8052b4792e1e7c22`:

| Artifact | Git blob |
| --- | --- |
| [Concrete API](../contracts/password-hashing-api-v1.md) | `7ad1e8d3f3311673dd368f2ef4d297de31dbafcd` |
| [API author evidence](sessiontest-001-api-author.md) | `2b9ec1e3024f86e0cea5836fe672fb45cc21f7f1` |
| [Frozen logical contract](../contracts/password-hashing-v1.md) | `bc9ccbf66a41c9ae69abde4bd905401900e17461` |
| [Logical author evidence](sessiontest-001-contract-author.md) | `4d26e84e00e7d51e7f50074d9b4824ea0333dc75` |
| [Different logical review](sessiontest-001-contract-review.md) | `6226c59d21a01602ce73cd3c776408200a15e0f5` |

**APPROVED for this concrete API document stage only.** No blocking requirement
conflict or new owner decision was found. The three raw entrypoints, source-bound
upgrade, ownership, error phases and defaults preserve the admitted logical slice.
This approves no future header/source/harness, dependency pin, fixture/test,
foundation/red, implementation, account service or platform claim.

Read all 447 API lines and all 319 logical lines, the logical/API author and logical
review evidence, actual SESSION-001 brief, specification §10/AUTH-11, root/server/
tests instructions and existing workflow/ownership boundaries. The API remains
standard-library-only at its public boundary; sodium is private implementation.

## Authority, freshness and lifetime audit

Standalone verification is an observation, never a login capability. Upgrade takes
raw password/account data and a trusted actual login-source dependency; it accepts
no caller-created verified flag/certificate. The source must read the actual current
credential and its bound attempt, not reconstruct a record from selected policy.
Wrong account, state/optional inconsistencies and invalid enum values are protocol
errors; missing/unavailable are distinct. Port producers must bound their own reads.

Only actual native verification success may construct PasswordVerifiedSource.
Its private constructor/friend, deleted copy/move, final class, immutable snapshot
reference, exact attempt reference and source-object identity prevent a public
proof factory. The source checks actual accepted login/account conditions and the
same full credential/revision/attempt before generation and again after it. Native
unsuccessful returns before either check. Denied/stale/unavailable/protocol failures
never produce replacement_ready. No-rehash returns only after the first accepted
check, preserving exact stored bytes.

The proof is a synchronous borrow only. Its credential/attempt/source outlive both
callbacks within the call; callbacks cannot retain or queue it. An authoritative
queue bridge copies bounded metadata into an owned command and never queues the
borrow or runs crypto on the writer. The same protected password copy survives
verification, both checks and new hashing; caller input/source lifetime cannot end
while native work is in flight. These are explicit port/lifetime obligations, not
an already demonstrated queue implementation or runtime revocation mechanism.

The result's private data constructor and const accessor expose an owned immutable
proposal, not durable authority. It binds account, attempt, full original verifier
and revision, plus replacement. Both checks still leave a post-return race; the
actual store MUST atomically recheck still-accepted login/account conditions and
the exact source revision AND record before saving. Newer resets win. The helper
has no write port or session issuance and never restores the snapshot over newer
state. Actual store/port behavior and persistence remain separate integration gates.

## Input, cost, errors and secret handling

All option fields validate first, including unused standalone-verification fields.
Password 0..1024 bytes preserves empty/NUL/non-ASCII/whitespace without normalization;
oversize refuses before copying/ports/native work. The valid empty span is handled
with a later nonnull one-byte protected allocation and native length 0. Invalid
caller memory is not a recoverable verification failure.

Record ceilings precede scanning/copies. Complete ASCII/field/decimal/Base64 grammar
and canonical unused-bit checks precede profile/cost decisions. t=0, m<8 and p=0
are malformed; otherwise unsupported algorithm/version/positive p rejects before
native verification. Admitted supported costs are bounded before conversion, KiB
multiplication and allocation; no needs-rehash helper parses untrusted input first.
Source protocol shape precedes present-field ceilings and membership validation,
as explicitly documented. Zero admitted memory is a zero budget, not unlimited.
The scalar never pretends to acquire actual worker/RAM admission.

Stored costs govern verification. Replacement uses componentwise maxima and cannot
downgrade; valid H0 fits 8 MiB admission but its desired 64 MiB replacement does not.
Generation/native-output/materialization failures after accepted login yield the
bounded unavailable outcome and no partial replacement. Port diagnostics are
discarded; typed exceptions use fixed text and allocation exceptions have explicit
limits. Native −1 stays not_successful, not a reason-specific wrong-password proof.

Guarded allocation is followed by an explicit successful lock check before copying
plaintext. RAII owns clearing/freeing on all exits, including callback exceptions
and stale second checks; record/output scratch is also cleared. No plaintext or
digest shortcut is returned/logged/persisted. Earlier ingress, stack/register copies,
swap/dumps and actual secure-memory results are not falsely certified. Exact source
audit and meaningful cleanup/error tests remain required before implementation
approval; this document does not demonstrate cleanup by execution.

## Primary facts and independent checks

Read only official documentation and 1.0.22 release sources:

- [Initialization](https://doc.libsodium.org/usage): readiness 0/1 and initialization
  before native use; no readiness call was performed.
- [Secure memory](https://doc.libsodium.org/memory_management): guarded allocation
  can succeed without locking; explicit lock result and clearing/freeing matter.
- [Argon2id release header](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h):
  nonnull explicit-length native inputs, 16-byte salt/128-byte output and custom
  t3/64MiB distinct from named INTERACTIVE/MODERATE profiles.
- [Release implementation](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c):
  internal salt, 32-byte digest, p1 and verification 0/−1 mapping; no call was made.
- [Upstream tv3](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.c)
  and [expected output](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.exp):
  H0 is the literal empty-password index 2, not a project-generated oracle.
- [Parameter guidance](https://doc.libsodium.org/password_hashing/default_phf):
  actual memory/concurrency/time measurement is required. Reverification plus both
  checks and joined-cost hash must be measured in the actual login sequence; the
  API does not claim one-hash latency, hard cancellation or supported hardware.

A stdlib-only inline Python audit (base64/re/Path/subprocess for Git reads; no
project import) passed 28 literal/declaration/frozen-input assertions: H0 equality,
96 ASCII bytes/six fields; 22/43 Base64 lengths, 16/32 decoded lengths, canonical
round-trips and two unused-bit counterexamples; exact KiB conversions/admission;
639 credential, 895 present-read and 1022 replacement string bytes; both literal
non-downgrade examples; 99-byte maximum supported record fitting 128; 18 preserved
PH rows; three raw declarations; five exact frozen blobs. These are size/grammar/
arithmetic facts, not cryptographic verification. Initial audit selected the first
format-template block instead of H0; corrected the audit selector and reran all 28
successfully. No contract or expected value changed; that audit error was no
project behavior failure or intended red.

Host `pkg-config --modversion libsodium` and header reads report 1.0.22 only.
Inspected root's [source authentication receipt](sessiontest-001-source-authentication.json)
and [preparation note](sessiontest-001-dependency-preparation.md), imported 77a3bb2:
they record authenticated source bytes, not an installed/selected SDK or native
runtime proof. This reviewer did not repeat authentication or execute that library.

Actual commands, Linux/Python 3.14.8/Git 2.56.0:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
pkg-config --modversion libsodium
```

All passed; project check 401 tasks/122 decisions/local links/presets/current HTML.
Git exact-blob/status reads and installed header-symbol inspection also completed.
Final staged inventory/whitespace checks cover only this review note. No compiler,
linker, dependency extraction/build, init/hash/verify/RNG/secure-memory native call,
test/foundation/marker, real login or provider operation was run. No contract,
assertion, fixture, source, schema, pin, state/HTML/build/CI file was edited.

## Required next gates

Root may release the next bounded author stage after integrating this exact verdict.
Future minimal source must preserve all three single immediate distinct marker
throws before validation/source/native work; inert constructors/import/build/
discovery cannot execute a marker. Literal H0/raw simulated source ports directly
reach each entrypoint without a SUT factory or forged proof. Stronger-cost success
fixtures need independent valid verifier/password provenance; editing H0 costs
does not preserve its digest or create a success oracle.

Require different actual minimal-source/harness review, independently authored and
reviewed tests plus standalone selected-library foundation, real selected foundation
green before FIRST intended project marker red and its attribution, then production
and different actual source/config review with ordinary acceptance/CI. Actual
protected-buffer/port/store/admission/conditional-save behavior, account/session
security, hardware measurement and supported packaging remain required. SESSION-001
is not implemented and SESSIONTEST-001 is not completed by this document approval.
No push, PR or main merge was performed; root owns shared files and publication.
