# SESSIONTEST-001 password API document author evidence

Author: service_author, 2026-11-04, dedicated
`task/SESSIONTEST-001-api-author`, worktree
`local-evidence/worktrees/sessiontest001-api-author`. Started clean at release
claim `f94f5ae83dfb244c1c2eaa492ab8e82a790171e9`, after logical approval
`98f049400d7a0148da1aa623cc71e9e7b4253aa5` / root import `febe5c2`.
Owns only the new API proposal and this note. No publication, shared state/HTML,
dependency pin, build/CI, schema, source, fixture or test edit is performed.

## Actual proposed artifact and frozen inputs

[Password hashing API](../contracts/password-hashing-api-v1.md), Git blob
`7ad1e8d3f3311673dd368f2ef4d297de31dbafcd`, refines the approved logical slice.
The following actual input blobs remain unchanged:

- [Logical contract](../contracts/password-hashing-v1.md):
  `bc9ccbf66a41c9ae69abde4bd905401900e17461`.
- [Logical author evidence](sessiontest-001-contract-author.md):
  `4d26e84e00e7d51e7f50074d9b4824ea0333dc75`.
- [Different logical review](sessiontest-001-contract-review.md):
  `6226c59d21a01602ce73cd3c776408200a15e0f5`.

Read root/server instructions, architecture/workflow/test strategy, actual
[SESSION-001](../tasks/EPIC-007/SESSION-001.md), specification AUTH-11/section 10,
and approved logical/ownership boundaries. No existing account credential/login
service is claimed implemented. This author previously independently reviewed the
logical document, but cannot approve this newly authored API. A different actual
API reviewer is required.

## Concrete choices

Three raw entrypoints cover creation, standalone verification and post-login
replacement proposal. All are directly reachable using raw password bytes/literal
H0 and simulated account state, without an earlier SUT factory or public verified
certificate. Minimal future bodies must immediately throw the distinct exact
PasswordHashingNotImplemented marker. No actual declaration/source is added here.

The upgrade call independently verifies the actual current credential read through
a trusted read-only login source. Only native success permits an ephemeral private
borrow to be supplied to that source's real accepted-login/current-revision gate.
The borrow binds the actual source object, full immutable credential and exact
actual login-attempt reference; no caller bool or opaque reference proves success.
It cannot be publicly constructed, copied, retained or queued. The same protected
password copy spans verification, first gate, new hash and second gate.

The returned replacement owns account/attempt/source revision/full original record
and new record. It contains no plaintext or public reusable proof. Source checks
before and after generation do not close the post-return race: the owning store
must still atomically compare actual login/account conditions and the entire source
revision/record before conditional save. This module deliberately has no write port,
login issuance or hidden persistence. A concurrent reset wins and is never restored
from the old snapshot. This preserves the logical caller-owned persistence scope.
Reverification and both checks add real work; the benchmark must measure the actual
flow, including any earlier standalone verification, rather than promise one-hash
latency. There is no new scheduler, hard cancellation or writer-thread crypto.

Exact option/defaults, raw 0..1024 bytes, empty nonnull native pointer handling,
strict canonical PHC/Base64 and zero-p-before-unsupported-p classification,
fixed error text and priority, admitted memory versus actual worker authority,
owned output limits and cleanup/failure outcomes are specified. Native −1 remains
an unsuccessful observation, never a reason-specific wrong-password proof.
Componentwise joined costs never downgrade; unavailable generation yields no
replacement and never changes a previously accepted login or stored credential.

The t=3/64MiB initial profile/caps and libsodium 1.0.22 pin remain proposals pending
selected artifact/runtime and supported-server measurement gates. The API does not
silently label this custom profile INTERACTIVE or MODERATE, claim measured latency,
or treat installed headers as runtime proof. Rigorously checking lock success and
bounded RAII cleanup does not claim to erase unknown earlier/stack/register copies.

## Primary facts and actual checks

Read only official documentation/release declarations for native facts:
[initialization](https://doc.libsodium.org/usage),
[guarded allocation, locking and clearing](https://doc.libsodium.org/memory_management)
and the [tagged Argon2id header](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h).
These distinguish readiness 0/1, explicit lock checks, nonnull explicit-length
input, Argon2id13/128-byte output and the custom default versus bundled profiles.
They do not prove runtime identity or hardware cost. Previously approved logical
review retains upstream H0 provenance and encoder/verification-source attribution.

Actual checks in this worktree, Python 3.14.8:

- `python3 tools/check_project.py`: PASS, 401 task records/122 decisions, local
  links/presets/current HTML. This infrastructure check invokes no feature.
- `python3 tools/progress.py render --check`: PASS; no generated HTML was edited.
- A read-only inline Python stdlib command using base64/re/Path, no project import:
  16 literal size/Base64/cost assertions PASS, approved H0 literal equality PASS,
  API local-link existence and balanced fenced-block checks PASS. Facts include
  96 H0 ASCII bytes, 16/32 decoded bytes with canonical re-encoding, the exact
  4882/65536/262144 KiB conversions, 8MiB admission, 639-byte credential,
  895-byte present read, 1022-byte replacement and componentwise cost join.
- Git exact-blob/status reads and `git diff --check`: PASS. Sole-owned-path staged
  whitespace/inventory checks accompany the commit.

No actual header/source/stub, C++ compilation/link, dependency installation,
native library/runtime call, sodium_init, hash, verify, RNG, guarded allocation,
memory lock, test, foundation, marker, live login or account operation ran.
The inline assertions only inspect public literal bytes and ordinary arithmetic.
No expected fixture was produced by a project feature or crypto implementation.

## Remaining gates

Different review of this exact API is next; this note gives no self/future approval.
Root owns stage release, selected immutable dependency artifacts/identity and
build/CI. Then require separately reviewed inert source/harness, independent tests
and native foundation, observed selected real-library foundation green before
FIRST intended project marker red, production implementation/source review and
ordinary acceptance/CI. No setup failure or skipped test can replace that baseline.
Actual account/login/store conditional persistence and queue lifetime/admission,
supported-server measurements and platform/package behavior remain required owning
integration/release gates. SESSIONTEST-001 remains incomplete and SESSION-001
planned; no push, PR or main merge was performed by this author.
