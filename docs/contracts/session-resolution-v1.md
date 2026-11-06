# Current account-session supplier — document proposal v1

SESSIONCONTRACT-001; specification v0.45 §10/§17/§18, AUTH-05/09/10/12,
API-01/02/03. Document identity `subathon.account-session-resolution.v1`.
Base: integrated task `40c33ad`; separate review is required before acceptance.
This document proposes an internal supplier API and literal examples. It supplies
no C++ header, schema, migration, test baseline, login service or advertised feature.

Import [wire primitives](wire-primitives-v1.md), the
[writer ownership boundary](writer-transactions-v1.md),
[password component](password-hashing-v1.md) and
[reference consumers](reference-application-adapters-v1.md) unchanged. Password
verification/proposed rehash is not an accepted login. Permission evaluation is
not authentication. Actual current-account, credential, installation, clock and
session producers remain runtime gates. Browser cookies, desktop headers, Origin,
CSRF, transport parsing, HTTP statuses and disclosure envelopes belong to
[SESSIONCONTRACT-002](../tasks/EPIC-007/SESSIONCONTRACT-002.md).

## 1. Token meaning, encoding and selected primitives

An account-session credential is exactly 32 unpredictable bytes, encoded as exactly
64 lowercase ASCII hexadecimal characters. Every byte pattern is representable,
including zero; a synthetic zero vector proves no production randomness. No token
contains account, roles, timestamps or a signature. It is separate from viewer,
overlay, invitation, recovery, provider and ownership-transfer credentials.

The proposed generator uses the already selected libsodium 1.0.22
`randombytes_buf(raw,32)` after successful initialization, without caller-selected
seed, deterministic production mode or fallback. Libsodium documents this API as
producing unpredictable bytes; RNG failure handling and VM/restore behavior need
actual source/runtime review. It has no ordinary success/error return, so the
application must not invent a recoverable return code or successful fallback.
[Official random-data documentation](https://doc.libsodium.org/generating_random_data).

The v1 lookup digest is unkeyed BLAKE2b with output length 32 bytes, computed using
libsodium `crypto_generichash` over these EXACT 60 bytes:

```text
ASCII "subathon.account-session.v1", one NUL byte, raw_token[32]
prefix hexadecimal: 7375626174686f6e2e6163636f756e742d73657373696f6e2e763100
```

The prefix is 28 bytes including NUL; there is no second terminator, token hex,
installation string, salt or native struct padding in that message. This is a
local domain-separation/lookup choice for random credentials, not password hashing
or an upstream prescribed session protocol. Persist only digest, format identity
and nonsecret session metadata. Store the digest as 32 exact bytes; a future wire
diagnostic must not expose it. No raw-token column, encrypted server token copy,
token-bearing audit, URL, log, exception text, metric label or browser local storage.
The installation binding below is mandatory even though not in the digest.

Libsodium supports unkeyed fixed-output generic hashing; its documented algorithm
is BLAKE2b. The pinned header admits 32-byte output and a fixed 384-byte incremental
state. The application selects the single-part form for this 60-byte message;
its complete native scratch still needs source/build review in the later baseline.
[Official generic hashing](https://doc.libsodium.org/hashing/generic_hashing),
[pinned 1.0.22 header](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_generichash_blake2b.h).

Use canonical whole-input hex validation BEFORE decoding. Reject uppercase, NUL,
whitespace, prefix, separators, odd/short/long strings and suffixes, never trim,
case-fold or accept a fitting prefix. The native generic hex decoder accepts more
spellings than this profile; calling it does not replace lexical validation.
[Official helper behavior](https://doc.libsodium.org/helpers).
Native initialization failure or a nonzero hash return yields unavailable, with no
digest success. Ready initialization returns 0 or 1.
[Official initialization](https://doc.libsodium.org/usage).

## 2. Proposed current-resolution API and finite ownership

Future namespace `subathon::server`, header `session_resolution.hpp`; these are
document declarations only. Enum values use uint8_t, consecutive from zero as
listed. An invalid caller-input enum is invalid_input; an invalid state enum in
selected source facts is invalid_stored_data, following section3's priority.
References are byte-exact nonempty
opaque strings, never tokens or authority certificates. Signed times are UTC
milliseconds in the full int64 domain; no float, local timezone or guessed zero.

```cpp
using AccountSessionDigest = std::array<std::uint8_t, 32>;
enum class SessionFailure : std::uint8_t {
    invalid_limits, resource_limit, invalid_input, unavailable,
    invalid_stored_data, missing, revoked, expired, fresh_authentication_required
};
enum class SessionRecordState : std::uint8_t { active, revoked };
struct SessionLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t owned_bytes = 65536;
};
struct SessionLookupRequest {
    std::string installation_ref;
    AccountSessionDigest digest;
};
struct SessionRecordFacts {
    std::string installation_ref, account_ref, session_ref;
    std::string installation_epoch_ref, account_epoch_ref, record_revision_ref;
    AccountSessionDigest digest;
    SessionRecordState state;
    std::int64_t lifetime_started_utc_ms, authenticated_utc_ms, expires_utc_ms;
};
struct CurrentAccountFacts {
    std::string installation_ref, account_ref, session_epoch_ref;
    bool application_access_enabled;
};
struct CurrentInstallationFacts {
    std::string installation_ref, session_epoch_ref;
};
struct CompleteSessionFacts {
    SessionRecordFacts session;
    CurrentAccountFacts account;
    CurrentInstallationFacts installation;
    std::string snapshot_ref;
};
struct SessionMissing {};
struct SessionSourceUnavailable {};
struct SessionSourceInvalidData {};
struct SessionSourceResourceLimit {};
using SessionLookup = std::variant<CompleteSessionFacts, SessionMissing,
    SessionSourceUnavailable, SessionSourceInvalidData, SessionSourceResourceLimit>;
class CurrentSessionSource {
  public:
    virtual ~CurrentSessionSource() = default;
    virtual SessionLookup load_current(const SessionLookupRequest &,
                                      const SessionLimits &) = 0;
};
struct SessionClockSample { std::int64_t now_utc_ms; };
using SessionClockResolution = std::variant<SessionClockSample,
                                            SessionSourceUnavailable>;
class CurrentSessionClock {
  public:
    virtual ~CurrentSessionClock() = default;
    virtual SessionClockResolution sample_current() = 0;
};
struct ResolvedAccountSession {
    std::string installation_ref, account_ref, session_ref;
    std::string record_revision_ref, snapshot_ref;
    std::int64_t authenticated_utc_ms, expires_utc_ms;
    bool fresh;
};
using PreparedAccountSession = std::variant<AccountSessionDigest, SessionFailure>;
using AccountSessionResolution = std::variant<ResolvedAccountSession, SessionFailure>;
PreparedAccountSession prepare_account_session(std::string_view token_hex);
AccountSessionResolution resolve_current_account_session(
    const SessionLookupRequest &, bool require_fresh,
    CurrentSessionSource &, CurrentSessionClock &, const SessionLimits & = {});
```

Missing means the bounded exact installation+digest index has no session record.
It never means unavailable storage, partial projection, an absent linked account,
missing epochs, unsupported stored format or a swallowed error. A selected record
with broken links or malformed metadata is invalid_stored_data. Source failures
cannot return incomplete facts or a fabricated missing row.

The source owns a single consistent current session/account/installation snapshot
on the authoritative Qt execution queue, under the actual caller's transaction or
equivalent serialized state scope. The source's lifetime cannot extend that scope.
It performs bounded exact indexed lookup; no session-history/account-table scan,
network callback, independent writer, nested transaction or token cache grants
access. Physical schema/typed SQL APIs and snapshot-revision production require
their own reviewed test-first work. This interface does not authorize arbitrary SQL
or extend existing finite reference read views implicitly.

All twelve complete-facts string occurrences count separately toward owned_bytes;
there are six record, three account, two installation and one snapshot references.
The five resolved-result occurrences count separately, including copied values.
The input installation is an additional separately owned request occurrence.
Each value must fit both its own limit and its complete value's aggregate limit
BEFORE its allocation/copy; a source must inspect stored lengths/types first.
owned_bytes is a PER-VALUE ceiling for each of request, complete facts and resolved
result, rather than a shared call-wide ceiling. All three may coexist; their
simultaneous string payload is at most3×owned_bytes. A source/result handoff moves
owned values rather than making additional full owning copies; validation borrows
views. Validate owned_bytes<=min(UINT64_MAX,SIZE_MAX)/3 before work so the peak bound
and cumulative sums are representable, then use checked sums for actual payloads.
Thus the literal request1+facts12+result5 owns18 simultaneous bytes even with
owned_bytes=12, within its36-byte peak ceiling. Application fixed objects and native
scratch remain additional fixed storage under their separate obligations below.
No caller-owned allocation is retroactively bounded. Native object/control-block
overhead and fixed scalar/array storage are additional fixed storage, not hidden
string payload; no collections or recursively nested values exist in this profile.
Limits are trusted configuration, positive, and must leave room for each container's
terminator/size arithmetic in size_t and max_size; otherwise invalid_limits.

prepare borrows the supplied view synchronously, checks length before reading
characters, and needs no variable allocation. It reads at most64 token characters,
uses one32-byte raw buffer, one60-byte framed buffer and one32-byte digest output
(124 bytes of application byte storage), plus fixed counters/return discriminant.
Wipe raw/framed scratch on every exit. No raw secret is queued. The caller owning
the original transport secret must erase its copies after handoff. A later native
review must additionally bound crypto stack/work space; 124 is not a process or
libsodium-total memory claim. Resolution uses at most12 borrowed string views,
fixed enum/scalar comparisons and one owned result; it projects the complete result
before growth. Digest-only queue admission is still finite and belongs to the host.

Zeroization uses the selected libsodium facility; ordinary memset alone is not an
assured secret-erasure primitive. Neither zeroization nor guarded allocation proves
all registers, framework copies, swap or crash dumps are protected. Locking/adapter
handling remains an actual security/platform gate.
[Official secure-memory documentation](https://doc.libsodium.org/memory_management).

## 3. Resolution, exact priority and execution validity

prepare priority: token length>64 → resource_limit; length<64 or any non-lowerhex
byte → invalid_input; runtime/crypto failure → unavailable; otherwise digest.
Complete lexical validation precedes crypto. Failures contain only the enum, no
token, digest, source text, account reference or library diagnostic.

resolve priority, stopping at the first applicable step:

1. Validate limits; preflight complete borrowed request reference/aggregate sizes;
   oversized → resource_limit, empty installation → invalid_input. These failures
   call neither producer. Digest is exactly32 typed bytes, never parsed as text.
2. Call source once in the active execution scope. Unavailable → unavailable;
   missing → missing; selected invalid data → invalid_stored_data; source preflight
   resource refusal → resource_limit. No success, clock call or partial resolved
   DTO follows any of these variants. Unknown/unclassified native or storage
   failure is unavailable, never a guessed resource/corruption diagnosis.
3. Validate ALL supplied fact reference/aggregate sizes before copying, then shapes,
   state enum and time order. Oversized → resource_limit; malformed/dangling facts,
   digest mismatch, a different installation/account binding, empty reference or
   lifetime_started>authenticated or authenticated>=expires → invalid_stored_data.
   Session/account/installation installation refs must equal the request;
   record.account_ref must equal account.account_ref. Epoch mismatches are handled
   at step4, not repaired. A source internal selected-format error is also stored
   corruption; no legacy fallback or partial acceptance.
4. Revoked row, disabled application account, session/account epoch mismatch or
   session/installation epoch mismatch → revoked. Epoch equality is byte equality.
5. Sample clock once in the SAME execution scope. Unavailable/untrusted clock →
   unavailable. A trustworthy producer must stop authentication on unresolved
   discontinuity/rollback or recovery uncertainty; it must not certify an old
   wall-clock value as current. now<authenticated or now<lifetime_started →
   unavailable, without unsigned wrap or silently resetting timestamps.
6. now>=expires → expired (expiry is exclusive). Otherwise fresh is EXACTLY
   0<=now-authenticated<=300000ms. Use checked full-domain arithmetic: subtracting
   signed extremes must not overflow. If require_fresh and false →
   fresh_authentication_required. Otherwise project/check all five result references
   and return the owned resolved value. A result budget refusal is resource_limit.

The clock source is an obligation for actual clock/recovery ownership, not a caller
boolean that manufactures trust. A queue-time sample precedes each protected effect.
Compare selected digest bindings with the fixed32-byte sodium_memcmp operation;
index lookup itself has no claimed whole-request constant-time guarantee. A digest
is internal lookup data and is never accepted from an external transport as a
replacement bearer credential. [Official equality helper](https://doc.libsodium.org/helpers).
No frontend admission result, snapshot_ref, session_ref or previously resolved DTO
authorizes later execution. Re-resolve and apply current permissions at execution,
including queued/review/deduplicated-result disclosure. A request ordered before a
revocation may complete within its serialized unit; a request ordered after its
acknowledged commit must fail. Concurrent split read/check/write is insufficient.
If an operation releases the execution scope or waits, it must re-resolve on return.

Public transport collapses missing/revoked/expired and malformed credentials into
the separately reviewed non-enumerating authentication envelope. unavailable stays
an honest service inability; it is not wrong password or nonexistent account.
HTTP codes, any fresh-auth challenge and safe inspection projections remain002/013.
Internal enum distinctions are not permission to expose sensitive existence facts.

## 4. Issuance, replacement, revocation and inspection obligations

The future SESSION-002 service accepts issuance only from the actual account login
owner's accepted authentication, rechecked against the current credential/account/
installation revisions on this same queue. A client flag, password helper success,
Twitch name or manufactured principal reference is insufficient. Local credentials
and optional verified Twitch sign-in map to the same account; actual provider
authentication/reauthentication evidence remains unimplemented. No route acquires
owner access through session issuance.

After accepted fresh sign-in choose normal or remembered policy from trusted owner
configuration. Defaults are86400000ms (24h) and2592000000ms (30days), respectively.
Configured positive lifetimes are represented exactly, and issued expiry must be
representable in int64; refuse invalid configuration/overflow without saturation.
Lifetime configuration uses positive Unsigned64 milliseconds; compute the signed
start plus unsigned duration without a narrowing cast or intermediate overflow.
Set lifetime_started=authenticated=trusted now and expires=now+lifetime. Persist
immutable installation/account bindings and current epochs. Configuration changes
affect new sign-ins; existing sessions keep their recorded maximum unless explicitly
revoked. Existing access never extends a recorded maximum or fresh-auth age.

Generate a new token and stage digest-only row in the actual writer transaction.
Unique installation+digest collision or competing session-ref collision is a
non-success, never adoption of an existing session. Wipe the candidate and return
unavailable; a new accepted attempt may generate again after known rollback. Never
publish a raw token before acknowledged commit and normal work completion.
Commit/cleanup unknown retains unknown transaction truth and suppresses credential
publication; it must not be reported as confirmed rollback or retried blindly with
a new token. Reconciliation and orphan-session cleanup require later service tests.

Token-only replacement/rotation keeps session identity, account/installation,
lifetime_started, authenticated and expires EXACTLY; it changes digest and
record_revision atomically, invalidating the old digest without an overlap grace.
It requires a currently resolved nonexpired session and normal commit before
publishing the replacement. A stale revision loses without overwriting another
rotation/revocation. There is no sliding renewal and no automatic timed rotation
in v1. A renewal that starts another lifetime is a new accepted fresh sign-in;
fresh reauthentication uses new randomness and atomically replaces/revokes the old
credential, updating authentication/lifetime times from that accepted sign-in.
Concurrent queued uses must re-resolve by their original digest, not by session_ref.
Unknown replacement outcome must never revive the old digest or disclose a guessed
new credential. Failed delivery may require fresh sign-in, not raw-token recovery
from server storage. Multiple intentionally distinct logins remain individually
inspectable/revocable; logging in does not silently revoke unrelated sessions.

Logout/revoke atomically marks the selected session revoked under current
authorization. Self inspection/revocation and authorized administration of others
remain required; selection by a nonsecret session_ref alone grants neither right.
Account recovery changes the account session epoch in the same recovery commit,
revoking every existing session while preserving permissions/owner identity.
Restoration replaces the installation session epoch before any restored access;
normal startup of an unchanged installation does not silently rotate this epoch.
Actual reset/restore protocols and storage atomicity remain owning task gates.

Inspection outputs contain only current authorized nonsecret session metadata and
explicit state, never token/digest. Revocation changes application access; expiry
or logout does not terminate the subathon process, timer or integrations. The
configured client-disconnect policy remains independently applicable. Freshness
does not replace sensitive-operation permissions, two-party transfer acceptance,
local-credential requirements or single-use/expiry rules of recovery/setup offers.
OWASP recommends strict session acceptance, replacement on authentication changes
and server invalidation; these inform the local choices above rather than certify
the proposed implementation. [OWASP session guidance](https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html).

## 5. Independent literal examples and remaining gates

Synthetic crypto vectors, NOT production credentials or prescribed random output.
Each digest is BLAKE2b-256 of the exact prefix+decoded bytes above, independently
derived using Python hashlib rather than an implementation under test:

| Token hex | Expected digest hex |
| --- | --- |
| `0000000000000000000000000000000000000000000000000000000000000000` | `a226d544ff92f3247dc784e949c846999fde801e4581c6bb219fde070591d3ea` |
| `000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f` | `e29be8cd64a1fdea5c6fb35c3570a322a65e793d85198c87a566490be0ab9b66` |
| `ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff` | `cef051cbdd51571ac90ebeed6f594ed4bd56a4dded259cd9dccb4b593b2ecbe9` |

Literal state fixture: all required refs are the SINGLE ASCII byte `i` for each
installation occurrence, `a` for account, `s` for session, `e` for both current
epochs and matching row epochs, `r` for record revision and `p` for snapshot.
Active/enabled, digest=the first vector, lifetime_started=authenticated=1000,
expires=86401000, request installation=`i`. Complete-facts payload is12 bytes;
resolved payload is5 bytes. The following results are independent integer/byte
derivations; no resolver executes here:

| Variation | Exact outcome |
| --- | --- |
| now=301000, require_fresh=true | success/fresh=true (age300000) |
| now=301001, require_fresh=true | fresh_authentication_required |
| now=301001, require_fresh=false | success/fresh=false |
| now=86400999 | success, expires still future by1ms |
| now=86401000 | expired |
| now=999 | unavailable, never fresh from wrapped arithmetic |
| missing lookup plus unavailable clock | missing, clock not called |
| revoked row plus unavailable clock | revoked, clock not called |
| row account_epoch=`f`, current=`e` | revoked |
| wrong stored account installation=`j` plus revoked row | invalid_stored_data |
| complete-facts owned_bytes=11 (all refs otherwise fit) | resource_limit before ownership |
| complete-facts owned_bytes=12, reference_bytes=1 | accepted bounds; resolved5 also fits |
| empty request installation | invalid_input, no producer called |
| token=64 `F` bytes | invalid_input, no crypto or source call |
| token=65 `f` bytes | resource_limit before character scan |
| token=63 `f` bytes | invalid_input |

Full-domain fixture: lifetime_started=authenticated=INT64_MIN,
expires=INT64_MAX, trustworthy now=INT64_MAX-1. It is unexpired but not fresh;
age is18446744073709551614, an unsigned64 value. A normal issued lifetime at
now=INT64_MAX-86400000 gives expires=INT64_MAX exactly; increasing now by1
must refuse issuance overflow. Defaults use ordinary day=86400000ms, not calendar
months/DST. A normal sign-in at1000 expires86401000; remembered expires2592001000.
Rotation at301001 must preserve those timestamps and stale freshness.

Finite profiles are engineering limits, not product account/session-count limits.
No pagination, bulk-list truncation or administrative inspection API is smuggled
through this single lookup. Actual listing/schema, auth/account producers, trusted
clock/recovery, native scratch, admission, current permission/disclosure, conditional
writer atomicity, transport002/HTTP013, protected desktop persistence and all later
reviewed tests/intended reds/implementations/platform/live checks remain mandatory.
All seven integrations, three FX providers and the full release scope remain intact.
