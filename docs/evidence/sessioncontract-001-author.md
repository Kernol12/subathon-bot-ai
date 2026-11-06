# SESSIONCONTRACT-001 supplier document author

Agent `session_supplier_author`, 6 November 2026. Isolated author worktree
`/tmp/subathon-sessioncontract001-resumed-author`, branch
`task/SESSIONCONTRACT-001-resumed-document-author`, based on reconciled root40c33ad.
Preserved original author/support worktrees untouched. Scope is ONLY
[supplier proposal](../contracts/session-resolution-v1.md) and this note.

Specification v0.45 §10/17/18, AUTH05/09/10/12 and API01/02/03; imported common wire,
writer, password and reference consumer contracts remain unchanged. The task's
separate transport002 prerequisite and all runtime/test-first gates are retained.
No header/schema/migration, production code, executable tests/stubs, native red,
session issuance, randomness, real credentials or live operation were performed.

Authored internal token preparation/current-resolution API, complete current
session/account/installation epoch source, trusted clock obligation, exact error
priority, limits/projection, one-snapshot execution recheck, lifetime/freshness,
rotation/unknown outcome, recovery/restore revocation and inspection ownership.
Concrete native scratch, SQL/index/schema/actual ownership implementations and
transport parsing/disclosure remain explicit owning gates. No caller-created
reference, password verification, public digest or cached result grants access.

Engineering choices requiring actual independent document review: 64lowerhex for
32random bytes; unkeyed domain-separated BLAKE2b-256 using selected libsodium1.0.22;
4096reference/65536owned-string defaults; exact exclusive expiry/inclusive300000ms
freshness; no sliding renewal or timed automatic rotation; atomic no-overlap token
replacement preserving lifetime/auth age; recorded lifetimes unaffected by later
default edits; explicit revocation and current epochs remain immediate.

Read primary sources using web tool6 November2026: libsodium random-data, generic
hash, pinned1.0.22 header, helpers, initialization, secure-memory and OWASP session
guidance. Each technical claim links directly in the proposal. The crypto APIs are
library facts; proposed composition, protocol and runtime obligations are not
upstream guarantees. randombytes_buf is void, zeroization has practical limits,
and a checked source/actual platform baseline remains necessary. No dependency or
pin was added or changed.

Literal vector derivation actually run with Python standard library, independently
of project production code (which has no session implementation):

```python
import hashlib
prefix = b'subathon.account-session.v1\x00'
assert len(prefix) == 28
for raw in (bytes(32), bytes(range(32)), bytes([255])*32):
    print(raw.hex(), hashlib.blake2b(prefix+raw, digest_size=32).hexdigest())
```

All three output pairs are copied literally to proposal§5. Other expected values
derive from byte-occurrence counts12/5 and integer arithmetic: day86400000,
30days2592000000, auth1000→fresh301000 inclusive/not301001, expire86401000
exclusive, and opposite int64 endpoints yielding unsigned age18446744073709551614.
These are authored oracles, not observed product behavior or an approved test suite.

Actual checks in author worktree: `python3 tools/check_project.py` PASS423records/
122decisions/local links/presets/currentHTML; `python3 tools/progress.py render --check`
PASS current; `git diff --check` PASS. No executable capability was introduced, so
existing document/scaffold tooling CI remains appropriate; root owns actual latest
PR CI receipt and any shared state/HTML update. Separate reviewer must examine the
actual authored commit before acceptance. Planned implementation/platform/live gates
remain not_run. No review/feature-completion claim is made by this author note.
