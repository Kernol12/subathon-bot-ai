# Independent supplier document review — SESSIONCONTRACT-001

Author: `session_supplier_author`. Different reviewer:
`session_supplier_review`, 6 November 2026. New isolated review worktree
`/tmp/subathon-sessioncontract001-supplier-review`, branch
`task/SESSIONCONTRACT-001-supplier-document-review`; historical trees preserved.

**APPROVED for document integration:** actual author
`c7e6e681b1fa6c5304a65ce931866e1c81c46887`, after examining initial `e03c11f`
and the complete correction. Accepted supplier blob
`c5f5a6e71664042e18475068d3caf577b0c8d009`; author-note blob
`4610af933c1fb3afc48d8d6dafc0724a8e539cd7`.
Authority is specification v0.45 §§10/16/17/18, AUTH-05/09/10/12,
OPS-04 and API-01/02/03. This approves the
[supplier document](../contracts/session-resolution-v1.md), not an implemented
authentication capability, executable baseline or future API/header.

## Contract and boundary audit

The 32-byte unpredictable bearer credential and canonical 64-character lowerhex
representation preserve the agreed 256-bit policy. Whole-input lexical validation
precedes decoding/crypto; the separately typed internal digest is not an external
replacement credential. The exact 28-byte NUL-terminated domain prefix plus raw32
message and BLAKE2b-256 choice are explicit local engineering decisions. No raw or
encrypted server token copy, bearer-bearing exception/audit/URL/local-storage use,
hosted authorization service or provider shortcut is introduced.

The source requires one coherent actual installation/account/session snapshot and
bounded exact indexed access. Missing rows, unavailable storage, selected corrupt
links and resource refusals remain distinct, representable variants. Matching
installation/account/digest bindings, all required references, state/time shapes
and epoch provenance are checked before success. Recovery invalidates the account
epoch, while restoration replaces the installation epoch before restored access;
normal restart does not revoke sessions implicitly. Password verification and
permission evaluation remain separate components, neither supplying accepted login.

Resolution ordering is complete: limit/request preflight, one source read, complete
fact limits/shape, revocation, one trusted clock sample, exclusive expiry/inclusive
five-minute freshness, then result projection. Invalid source enums are stored-data
failures. Signed-extreme age and expiry addition require checked exact arithmetic.
Unresolved clock rollback/recovery uncertainty cannot certify current authentication.
Actual clock trust and storage producers remain downstream runtime requirements.

Current execution re-resolution and permission checks occur in the same serialized
unit as the protected effect/disclosure; cached principals and references grant no
later authority. A unit ordered after committed revocation fails. Releasing the
scope requires another resolution. This is consistent with the single Qt writer
boundary, without claiming a working queue/SQL adapter.

Issuance requires actual accepted current authentication. Defaults retain24h/30day
maximums and owner configuration; existing recorded lifetimes do not slide.
Token-only rotation preserves authentication/lifetime timestamps and atomically
invalidates the old digest, while fresh sign-in can start a new lifetime. Collisions
never adopt another session. Commit/cleanup uncertainty suppresses token publication
and remains unknown; delivery failure does not authorize reconstructing a raw token
from storage. Self/authorized-administrator inspection and revocation remain required,
and account expiry does not stop the subathon. Viewer/overlay access stays separate.

## Findings resolved in the actual correction

Initial `e03c11f` left the simultaneous payload interpretation implicit and described
invalid enums generically as invalid input. Before approval I requested exact
per-value/peak ownership and source-enum provenance. Actual `c7e6e68` resolves both:
request, complete facts and resolved result have separate owned-byte ceilings;
all three may coexist, their checked peak is at most3×owned_bytes, configuration
admits that multiplication, and handoffs move rather than create extra full copies.
Twelve source references, five output references and one request reference count
each occurrence. The literal profile therefore owns1+12+5=18 bytes under a36-byte
peak ceiling at owned_bytes12. Application/native fixed storage remains additional;
the124-byte prepare scratch is not claimed to bound native crypto or process memory.
Source invalid-state enums explicitly map to invalid_stored_data at the stated
priority. No reviewed test assertions exist or were changed in this document task.

## Independent oracle, source and check evidence

I independently recomputed all three literal digests with Python standard-library
`hashlib.blake2b(prefix + bytes.fromhex(token), digest_size=32)` and matched every
literal. The prefix is exactly28 bytes; message60 and application buffers32+60+32
sum124. Independent integer/count assertions checked12=6+3+2+1 source references,
five result references, fresh301000/not301001 from auth1000, normal86401000 and
remembered2592001000 expiry, and opposite-signed-boundary age18446744073709551614.
These derive examples without a project session implementation; they do not execute
or approve product behavior.

I read the cited primary sources separately. The proposed use of unpredictable
bytes and the absence of an ordinary RNG return code match
[libsodium random-data documentation](https://doc.libsodium.org/generating_random_data).
Unkeyed BLAKE2b output and32-byte selection match
[generic hashing](https://doc.libsodium.org/hashing/generic_hashing); the selected
[1.0.22 header](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_generichash_blake2b.h)
declares32-byte default output and384-byte aligned state. Whole-input canonical
validation and fixed-length comparison do not overstate
[helper behavior](https://doc.libsodium.org/helpers). Initialization0/1 success
matches [official usage](https://doc.libsodium.org/usage), while the proposal retains
the practical limits of [secret erasure/memory protection](https://doc.libsodium.org/memory_management).
Strict acceptance/replacement/invalidation are consistent with
[OWASP session guidance](https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html);
the local composition is not presented as an upstream protocol or runtime proof.

Actual commands in the isolated review tree, initial and corrected candidates:
`python3 tools/check_project.py` PASS423records/122decisions/local links/presets/
currentHTML; `python3 tools/progress.py render --check` PASS; author-range
`git diff --check` PASS. The standalone Python hash/count/arithmetic assertions
above all passed. No CMake build, C++ session call, native crypto foundation, live
provider request, credential issuance, executable behavior test or red run occurred.

Documentation/tooling CI is appropriate to this document-only change. Current PR
checks, completion metadata, rules/discussions and normal merge remain coordinator
gates; this approval does not predict later-head results. SESSIONCONTRACT-002 must
still supply reviewed transport/Origin/CSRF/header/status imports before dependent
baselines and CONTRACT-013. Concrete account/session/clock/storage/queue producers,
native scratch, permission/disclosure integration, reviewed tests/intended reds,
desktop secure persistence, platform/recovery/live/release verification remain open.
All seven integrations and three FX providers remain required.

## Independent completion metadata review

**APPROVED:** actual coordinator completion candidate
`c961e607d6e4a3fa00cfd2d24ada2e70a1943349` (component metadata `705ced0`,
checkpoint `c961e60`). Inspected on a NEW isolated branch
`task/SESSIONCONTRACT-001-completion-metadata-review` at
`/tmp/subathon-sessioncontract001-completion-review`. This reviewer remains
different from supplier author and coordinator. Supplier blob `c5f5a6e...` and
author-note blob `4610af9...` exactly match the already accepted corrected candidate.
No later source, header, schema, test, fixture, build, pin or workflow change is
included in the supplier task.

Independent structured comparison with actual integrated main `e2cd5eb` confirms
all422 original task IDs and1619 dependency edges retained; the split adds one ID
and exactly five edges, yielding423/1624. Only the five existing session-scoped
records change (EPIC007, SESSIONTEST002/003, CONTRACT013, SESSIONCONTRACT001),
plus new SESSIONCONTRACT002. Every other current-main task record, including
STORAGE018 and MILESTONE001, remains identical; all old activity and project
verification/summary are preserved. Supplier001's `done` is DOCUMENT completion
supported by actual separate plan/contract review and literal audits. Runtime is
explicitly not_run. Transport002 stays planned, requires integrated001, and every
downstream consumer retains BOTH document gates and later test-first/runtime gates.

Handoff and checkpoint separate component completion from draftPR72 publication,
latest-head CI, current rules/reviews/discussions and normal merge. They accurately
supersede the earlier owner stop boundary with the resumed coordination assignment.
They preserve unfinished work and record the independent active MILESTONE001 claim
on its separate branch instead of overwriting this supplier branch's main snapshot.
I observed the actual separate `/tmp/subathon-milestone001` task worktree and branch;
milestone source/configuration/CI approval is outside this document review.
DB01/NUM04 and full runtime/platform/live/release gates are not promoted by supplier
completion. There is no supported authentication or running-server claim.

Actual independent commands on the completion candidate:
`python3 tools/check_project.py` PASS423/122; `python3 tools/progress.py render --check`
PASS; `python3 -m unittest discover -s tests/tooling -v` PASS19/19 in0.316s;
`node tools/check_progress_ui.mjs` PASS DOM simulation; `git diff e2cd5eb HEAD --check`
PASS. Structured Python assertions confirmed all task/edge/activity/verification
preservation and both-document dependencies above. No product browser/OBS/native
session behavior or new red baseline was executed. Applicable latest-publication
checks and normal GitHub merge gates remain actual coordinator observations,
not preapproved future outcomes.
