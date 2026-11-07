# Independent review — SESSIONCONTRACT-003 document

Reviewer `/root/session_lifecycle_contract_review`, different from author
`/root/session_lifecycle_contract_author` and coordinator `/root`, 7 November 2026.
**APPROVED for this lifecycle contract document and its author evidence only.**
No header, schema, test baseline, intended red, implementation, authentication
capability, producer, provider support, platform behavior or future CI/merge is
approved by this verdict.

Actual reviewed tree `e0973fb6343c2701b47b00a9172f3d49df7105ce`, containing author
packet `72a80198065587abf481e02f8d2f05983396f115` and coordinator claim `db6fbee`
from main `ebb4368ab5af3b0c39314607896a6213fc061bb5`. Reviewed on isolated branch
`review/SESSIONCONTRACT-003-lifecycle`, worktree
`/tmp/subathon-sessioncontract003-review`. Only this new review record is owned
or changed here. Shared progress/HTML/handoff, authored documents, builds, pins,
tests and supplier contracts are outside this reviewer's write scope.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| [Lifecycle document](../contracts/session-lifecycle-v1.md) | 2f30ac72c1b2f8582c451808d455acbde36f1e3b |
| [Author evidence](sessioncontract-003-author.md) | 01000940f140188e05119024a7afde8723cf8e3a |
| Unchanged supplier | c5f5a6e71664042e18475068d3caf577b0c8d009 |
| Unchanged transport | 2c33c2bbe9052455107c3056783d6bb605a5fef2 |
| Unchanged password logical/API | bc9ccbf66a41c9ae69abde4bd905401900e17461 / 7ad1e8d3f3311673dd368f2ef4d297de31dbafcd |
| Unchanged writer work | 4ba20506db9a858450099b82888a22c8f6dbd761 |
| Unchanged permission logical/API | 3d82bfa418bb14099ee130fca7a400adca66500b / dda2793c71299dbe36a321ce3dd5620fecc8b214 |

Authority and review inputs: root AGENTS/FIRST_AGENT_PROMPT, architecture,
workflow/test strategy, actual task/state and [task handoff](sessioncontract-003-handoff.md),
specification v0.45 §§10/16 and AUTH-01/05/09/10/11/12/15, DB-01/SERVER-01,
ADR-0010, full SESSION-002/ACCESS-004 briefs, accepted supplier §3/4, transport
§3/4, and the password/writer/effective-permission boundaries above. Read the
complete new contract and author evidence. The current writer header was read
only to confirm the actual work/session/result vocabulary; no feature code ran.

## Findings and verdict

No blocking document defect or requirement conflict was found in the actual
packet. No author correction is required by this review. In particular, this
verdict does not substitute a prose assertion for any missing producer design:
the document explicitly assigns those designs to their owning tasks and refuses
unknown/unsupported sources. All imported blobs match both the reviewed tree and
base main; the two reviewed authored blobs also match the actual author commit.

The six command shapes, matching prepared work/completion, inert construction,
owner-thread/admission failures and bounded nonsecret results are concrete enough
for the later minimal API/test stage. No client selects an internal command
variant to evade a real route, supplies a digest as an external bearer, or grants
authority by presenting a nonsecret session/attempt/revision reference.

Accepted authentication is explicitly owned by the actual login source and bound
to this request/work, complete verified credential lineage, current account and
authenticator revisions, access state, installation and both epochs. Local
acceptance works without a preexisting session or Twitch dependency. Conditional
post-login rehash must preserve actual acceptance lineage; arbitrary revision
mismatch is never ignored. Password-helper results, provider names, client flags
and fabricated principals cannot satisfy this boundary. Verified Twitch is a
reserved method requiring ACCESS-004's own real verified evidence and baseline;
an unimplemented path is unavailable. Issuance cannot change permissions/ownership.

An accepted attempt is reserved through the real terminal outcome. Committed or
unknown attempts cannot mint again; a known rollback permits reuse only while the
same original acceptance remains current/fresh. The deferred producer contract
must make that guarantee concrete. This document neither selects a durable attempt
table nor advertises process-crash or provider behavior from an opaque reference.

## State, authority and publication audit

All reads, current authority checks, conditional mutations and real synchronous
commit compose on one actual writer work/session. The unit cannot create another
writer, nested transaction, password job or network callback. CAS fences cover
the original record/revision/digest/state, immutable identities, epochs and current
acceptance/account/install authority. A source write decision cannot hide a sticky
SQL/cleanup/commit failure as a benign stale/collision result.

Rotation changes only digest/revision, preserves recorded times/retention/identity,
rejects its own old digest and every conflicting candidate, and invalidates the old
digest at the replacement commit without overlap. Reauthentication requires both
a currently valid original session and separately accepted fresh same-account
sign-in, then atomically replaces the credential and starts the explicit new
lifetime. Expired/revoked originals use ordinary sign-in instead. Neither operation
refreshes queued original-digest requests by session identity. Multiple independent
sign-ins and unrelated sessions remain intact. Recovery/restore epoch invalidation
and unchanged-startup preservation stay in their owning workflows.

Positive U64 lifetime configuration has no smaller invented lifetime ceiling.
Signed expiry uses mathematical addition followed by an i64 range check; duration
is never narrowed first. Defaults remain 86400000/2592000000ms. Freshness is
inclusive at 300000ms and expiry exclusive; full-domain differences fit U64 without
signed overflow. Configuration affects new sign-ins, with no sliding renewal or
rotation-based authentication upgrade.

Self selection is the exact resolved actor account; inspecting/revoking another
account requires the real current operation/target permission and workflow guards
before any selected-row/page lookup. Role labels and previous permits are
insufficient. Protected-owner classification comes from the trusted operation
catalog. Shape/corruption/resource/unavailability failures are not disguised as
missing rows or evaluated denials. Inspection exposes neither bearer nor digest.

Inspection uses one bounded indexed keyset read, at most page_size+1 rows including
a validated probe, unsigned-byte order, and a cursor equal to the last emitted row
only when another row exists. No load-all scan, total-count query, partial page,
multi-request snapshot lease or product session-count quota is introduced. Every
page rechecks disclosure; current account/install facts and the execution clock
classify active/revoked/expired rows without guessing future authentication times.

The actual writer outcome remains distinct from logical failure and publication.
Known rollback differs from uncertain COMMIT/cleanup, which fail-closes without
replay. Post-commit exceptions/protocol failures retain committed truth and expose
no normal success payload. A wrong/missing private envelope cannot be replaced by
captured intent. Only actual committed, error-free, normally returned matching
envelopes permit bearer publication.

One nonreentrant authority turn extends through matching completion and the
synchronous emitter; it permits no event pumping or later state command between
the final fences and emission. A new trusted publication-time sample must still
be current and strictly before expiry. Failure suppresses/erases the bearer rather
than extending lifetime. Emitter handoff is at most once and does not prove remote
receipt. Delivery uncertainty, committed response loss and orphan reconciliation
cannot revive an old digest, recover a stored raw token, blindly retry an uncertain
replacement or revoke unrelated sessions. Cookie/native handling imports the
accepted transport without changing its public error or secret-publication rules.

## Finite ownership and oracle audit

The complete request, every source projection and complete reply have explicit
DTO accounting before ownership/growth. Required reference occurrences, enum/
presence tags, digests and scalars count; page probes and repeated cursor/metadata
references are included. Five positive checked lifecycle limits, page_rows+1,
destination sizes and the peak sum are validated, alongside the independent
supplier and permission profiles. At most four source success values plus one
projected replacement, a fixed resolved actor and one reply are retained; bounded
handoffs move and validation borrows. The semantic peak is checked as
request_bytes + 5*source_bytes + 2*result_bytes, 7405568 bytes at defaults.
This is explicitly separate from SQL/native/framework/permission-producer budgets
and fixed object overhead, not a process memory or hard CPU bound.

One token candidate has fixed 32+60+32+64=188 application byte slots. Raw/frame
scratch is erased after last use; the bearer is erased on destruction or emitter
return. A copyable Qt envelope shares one secret owner rather than cloning the
buffer. Actual native scratch, control-block/framework/egress copies and platform
erasure limitations remain later source/security gates.

Manually checked all LC01–32 against the stated stages and accepted contracts:
defaults/configuration, signed extremes, rotation/freshness/reauthentication,
credential/epoch races, queued rotation/revocation order, collision, rollback/
unknown/post-commit/response-loss truth, publication clock, target non-disclosure,
current permissions, recovery/startup, revoked-row behavior, bounded keyset
changes/probes, corrupt links, acceptance age/reservation and complete-page refusal.
These remain proposed literal oracles, not executed lifecycle behavior tests.

Independent Python standard-library audit was run with `python3 - <<'PY'` using
integer arithmetic, exact byte sorting, document-only regex and read-only Git
blob queries. It checked all 32 unique ordered LC identifiers; the four signed
expiry boundary calculations; normal/remembered/custom/reauthentication expiry;
freshness 300000/300001; full U64 difference and overflow-safe ceil at
1/999/1000/1001/U64_MAX; scratch/default-peak sums; unsigned NUL/prefix/high-byte
ordering; both literal page sequences and insertion behavior; and the exact
base/current/author blob identities above. All assertions passed (exit 0).
Independent DTO derivation from declared shapes produced:

| Complete value | Derived literal bytes |
| --- | --- |
| Issue request, one-byte required references | 1+2+1+1 = 5 |
| Rotation request | 1+2+32 = 35 |
| Credential reply | 1+4+1+3*8 = 30 |
| Revocation reply | 1+4 = 5 |
| Inspection row | 2+1+1+3*8 = 28 |
| One-row page, absent cursor | 1+3+8+28+1 = 41 |
| Two-row page, one-byte present cursor | 1+3+8+56+2 = 70 |
| Page-source success, 65 one-byte-reference records | 1+(3+1)+2+1+65*(6+32+1+24) = 4103 |

## Actual checks and remaining gates

Environment: Linux 7.2.14-arch1-1 x86-64, Python 3.14.9, Git 2.56.0.
Commands actually run in the isolated reviewed tree:

| Command | Observed outcome |
| --- | --- |
| `python3 tools/check_project.py` | Exit 0; PASS 443 task records/122 decisions, local links/presets/current HTML. Explicitly does not run/approve draft product tests. |
| `python3 tools/progress.py render --check` | Exit 0; snapshot current; no shared files changed. |
| `git diff ebb4368 HEAD --check` | Exit 0; no whitespace errors in the supplied task delta. |
| `git ls-files -s` for the nine reviewed/imported paths | Confirmed exact blobs tabulated above. |
| `python3 - <<'PY'` independent literal/source-identity audit described above | Exit 0; all arithmetic/DTO/order/identity assertions PASS. |
| `git diff --cached --check` after adding only this record | Exit 0; review record has no whitespace errors. |

No CMake/build/CTest, C++ feature/native/crypto code, real credential/session,
network/provider operation or product behavior test was executed. The author's
reported tooling checks remain the author's evidence, not a separate reviewer
execution claim. Existing documentation/tooling CI is appropriate to this delta;
new executable CI does not follow from accepting a contract document.

Current completion metadata, latest-head selected CI, discussions/rules and normal
PR merge remain coordinator gates. The minimal API/harness and specification-linked
test packet still require review and observed intended red before lifecycle
implementation, then separate source/configuration review and passing acceptance.
Actual store/current readers/conditional persistence, local-login/credential/
permission/clock producers, lossless log-safe ingress, egress, reconciliation,
recovery/restore, verified Twitch, DPAPI/QtKeychain and supported browser/OS/package
evidence remain owning tasks. All seven integrations, three FX providers and the
full release scope remain required. This review supplies no future implementation
approval or self-certification.
