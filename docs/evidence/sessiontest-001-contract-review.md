# SESSIONTEST-001 independent logical password-contract review

Reviewer: service_author, different from document author service_reviewer.
2026-11-04, dedicated `task/SESSIONTEST-001-contract-review`, worktree
`local-evidence/worktrees/sessiontest001-review`. Owns only this new review note.
No source contract, author evidence, state/build/CI/pin, fixture, test or code file
is edited. No push, PR or main merge is performed.

Actual reviewed candidate: author `f730a176`, root integration
`d74fc81` (`Propose bounded Argon2id password storage and upgrade contract`):

- [Logical contract](../contracts/password-hashing-v1.md), Git blob
  `bc9ccbf66a41c9ae69abde4bd905401900e17461`.
- [Author evidence](sessiontest-001-contract-author.md), Git blob
  `4d26e84e00e7d51e7f50074d9b4824ea0333dc75`.

Read root/server instructions, architecture/workflow/test strategy, actual
[SESSION-001](../tasks/EPIC-007/SESSION-001.md), specification v0.45 §10/AUTH-11/S62,
CONTRACT-002, and account/setup/recovery/session-transport task boundaries. Current
server public headers do not expose an implemented account credential/login store;
writer-session capability names are not application login sessions. This proposal
does not pretend those missing account services or durable credentials already exist.

## Verdict: approve this logical document stage only

No blocking requirement conflict or additional owner choice was found. The bounded
profile, raw-byte semantics, preflight, unsuccessful-verification outcome and
successful-login upgrade sequence are coherent with AUTH-11. Actual artifact pins,
API/secret ownership, native foundations/tests, implementation, persistence/login
integration and supported-hardware measurements remain separate required gates.
This verdict approves none of that future work prospectively.

## Primary-source and metadata verification

Browsed official documentation and the actual tagged 1.0.22 source on 2026-11-04.
The [official password-storage documentation](https://doc.libsodium.org/password_hashing/default_phf)
supports explicit password lengths, self-contained encoded salt/parameters and
verification result 0 versus error −1. It does not justify classifying every error
as a wrong password or treating parameter mismatch as authenticated login.
The [initialization documentation](https://doc.libsodium.org/usage) distinguishes
ready return 0, previously initialized 1 and failure −1; no such function was called
by this review.

The [release header](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h)
declares explicit Argon2id13, a zero-byte password floor, 16-byte salt and 128-byte
encoded buffer, with explicit-length inputs. Installed metadata independently
reports version 1.0.22 in `pkg-config --modversion libsodium` and
`/usr/include/sodium/version.h`; read-only header inspection agrees on these
constants. This proves metadata/declarations only, not loaded runtime identity,
initialization, library availability under the future deployment, or measured cost.

The [release Argon2id implementation](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c)
shows internally generated salt, p=1, a 32-byte digest and bytes-to-KiB cost
conversion. Its verify wrapper maps success to 0 and other outcomes to −1. The
proposal correctly preserves an unclassified unsuccessful result and never
substitutes plaintext, a weaker algorithm or guessed costs on failure.

The [release encoder/decoder](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/argon2/argon2-encoding.c)
supports versioned m/t/p field order, original unpadded Base64, canonical decimal
syntax and complete-record termination. The proposed v19/p1/16-byte-salt/32-byte-
digest whitelist is intentionally narrower than generic library possibilities.
Full bounded preflight before verifier/rehash helpers prevents the stored record
from authorizing arbitrary native memory/time. Canonical Base64 round-trip/unused
bits must be checked; alphabet/decoded-length checks alone are insufficient.

The [release dispatch source](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/crypto_pwhash.c)
distinguishes explicit algorithm dispatch from generic entrypoints. The
[upstream parameter tests](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.c)
also expect needs-rehash on lower requested costs. Therefore that helper alone is
not a no-downgrade policy, password proof or account-login authorization. The
proposal's explicit Argon2id selection and componentwise upgrade rule address this.

## Reviewed behavior and scope

Raw 0..1024 password bytes include empty/NUL/non-ASCII/space data without trimming,
normalization, transcoding or truncation. That crypto-input domain does not permit
empty account passwords or decide a product password-strength minimum. Existing
account/invitation/local-owner/recovery requirements are preserved. The record is
bounded separately, owns all encoded bytes and carries its own salt/cost/version;
there is no conflicting editable second parameter set or newly invented SQL schema.

Desired t=3/m=64MiB, new floor t=2/64MiB and accepted legacy t=1/m=8KiB, with
t=10/m=256MiB caps, are explicitly LOCAL PROPOSALS. They are not native maxima,
measured security/latency, deployed compatibility or supported-hardware evidence.
Rejecting desired invalid units/nonintegral KiB before work and checking stored
numeric ceilings before conversion/allocation avoid silent rounding and overflow.
One active expensive call and bounded admission are responsibilities of the owning
login worker, not a hidden unbounded queue or a crypto call on the Qt writer.

Strict record admission accepts only the stated algorithm/version/parallelism and
fixed canonical salt/digest sizes. Other recognized profiles fail as unsupported;
malformed and over-budget records do not reach native verification. Zero-valued
salt/digest bytes are structurally possible data, not evidence of randomness or
password success. PH06–11 correctly separate format/profile/options/resource
refusals from password outcomes. The later API must pin exact combined-error order;
this logical review does not claim that parser exists.

Verification alone never grants a session, resolves account state or authenticates
caller-created booleans/references. The caller first establishes actual password
success, completes other login checks and rechecks the actual credential revision.
Only accepted login may initiate this upgrade sequence. Joined costs never lower
either stored dimension; a record already at/above desired stays byte-identical.
Creation failure retains the old credential with explicit upgrade_unavailable.
Saving the replacement requires the owning store's conditional check against the
actual verified source/current revision; a concurrent reset wins. No stale source,
unsuccessful verification or denied login authorizes replacement. Account/session
issuance/revocation and writer transactions remain their required owning boundaries.

The [official secure-memory guidance](https://doc.libsodium.org/memory_management)
supports the proposal's distinction between guarded allocation and successful
locking. Checked lock failure must not be disguised as protected storage; clearing
needs the non-elidable primitive and actual exception-safe ownership. The proposal
does not promise to erase prior UI/network/stack copies or eliminate crash/swap
exposure. Returned values and errors retain no password/digest shortcut, while
encoded account verifiers stay out of public views and routine diagnostics.

Two implementation details remain explicit next-API audit obligations, not changes
to this admitted logical profile: zero-length logical input must use a valid native
nonnull pointer when required by the tagged header; p=0/malformed p must reject
before crypto, while positive p other than 1 is unsupported. Secret-buffer lifetime
across off-writer verification/login/upgrade and actual source-revision binding
must be designed/reviewed concretely. No API/certificate/source design is approved
merely because these logical obligations are stated.

## Independent H0 and literal audit

Read the complete [upstream tv3 array](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.c)
and [published expected output](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.exp).
Candidate H0 matches tv3[2] byte-for-byte with an empty password. The program emits
failure rows for indexes 0/1/3 and the expected output omits failure 2. This establishes
published upstream expectation, not a local successful crypto verification. Other
upstream profiles are not silently imported into this narrower application profile.

A read-only inline Python stdlib command (base64/re/Path, no project import)
performed 13 assertions: equality with the separately copied upstream H0 literal;
96 ASCII bytes; six delimited fields; canonical 22/43-character Base64; decoded
16/32-byte fields; exact re-encoding; three KiB conversions; two literal joined-cost
facts; and exactly 18 PH examples. All assertions passed. In particular,
4882*1024=4999168, 65536*1024=67108864 and 262144*1024=268435456. These are grammar/
size/arithmetic facts only. No sodium symbol, hash, verify, RNG, memory protection,
native foundation, test or marker ran.

PH01–18 preserve published-empty verification versus separate raw-NUL/Unicode
requirements, length/PHC/profile/cost refusal, non-downgrade, denied/stale login,
replacement failure and admission/secret failures. Their actual fixture/source/
expected-result proof and execution still require independent test review. No
random salt inequality is promoted to deterministic mathematical certainty and
no generated verifier is used as its own oracle.

## Benchmark and future gates

The proposed procedure records supported Linux server hardware/container/RAM,
actual artifacts/runtime, concurrency, cost and memory, then measures initialization
separately and creation/verification/replacement over warm-up, 100 sequential samples
and admitted concurrent load. Its p95 one-second goal is explicitly unmeasured.
The [official parameter guidance](https://doc.libsodium.org/password_hashing/default_phf)
likewise calls for measuring cost against available memory/concurrent work.
The proposal makes no client/server performance equivalence, hard cancellation
deadline, supported machine or weaker fallback claim. Benchmarks and actual
admission/platform behavior remain required before those capability claims.

Actual checks in this review worktree: Git/status/exact blob reads and local header/
pkg-config inspection; primary HTTPS source reads; the 13 stdlib assertions above;
`python3 tools/check_project.py` PASS (401 tasks/122 decisions, local links/presets/
current HTML); `python3 tools/progress.py render --check` PASS; `git diff --check`
PASS. Staged sole-owned-path/whitespace checks accompany commit. No native crypto,
new foundation/test/marker or project feature was invoked; no dependency/build/
installation/link operation or actual secret/provider operation occurred.

Next: separately reviewed concrete API/secret lifecycle, minimal inert source and
harness, independent tests and actual review, selected real-library foundation
green, FIRST valid feature red and its attribution, production/source review and
ordinary CI. SESSIONTEST-001 remains incomplete, SESSION-001 planned. Root owns
all publication/state/build/pin/CI decisions. This document-stage approval leaves
those future gates and full account/platform/release requirements intact.
