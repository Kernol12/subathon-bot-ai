# Independent review — STORAGECONTRACT-002

Reviewer `/root/session_storage_contract_review`, different from author
`/root/session_storage_contract_author` and integrator `/root`, 7 November 2026.
**APPROVED for the physical/internal API contract document and author evidence.**
No header, migration, fixture, test baseline, observed red, source, current producer,
accepted login, provider/platform capability or future CI/merge is approved here.

Actual reviewed tree `b01cc18e3e06bc6767576b91f8d5ebf839f5e383`, author packet
`5c3acde0`, coordinator claim `db09a6c942e6c039128bd3a711554fd9723126b2`,
base main `e4d46d335e52850acc58a73f2500725cffb7c534`. Isolated branch
`review/STORAGECONTRACT-002-account-sessions`, worktree
`/tmp/subathon-storagecontract002-review`. This NEW review record is the only
reviewer-owned change; shared state/HTML/handoff/schema/build/CI/pins, imported
contracts, tests and all authored documents remain untouched.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| [Physical/internal API contract](../contracts/sqlite-account-sessions-v1.md) | `8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1` |
| [Author evidence](storagecontract-002-author.md) | `3b521c50fecd742e966dedad85ffa4061ab1c323` |
| Supplier / lifecycle | `c5f5a6e71664042e18475068d3caf577b0c8d009` / `48e3375bda594a39cca126ba14153d47664f0cdd` |
| Transport | `2c33c2bbe9052455107c3056783d6bb605a5fef2` |
| Password logical / API | `bc9ccbf66a41c9ae69abde4bd905401900e17461` / `7ad1e8d3f3311673dd368f2ef4d297de31dbafcd` |
| Writer work / SQLite writer | `4ba20506db9a858450099b82888a22c8f6dbd761` / `4d280461966f3fd9fcb13ab6d054c88228ca05b8` |
| Migration catalog | `cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61` |

Review inputs: root AGENTS/FIRST_AGENT_PROMPT, architecture/workflow/test strategy,
server AGENTS, specification v0.45 §§10/16 and AUTH-01/05/11/12, DB-01,
SERVER-01/OPS-02, task brief/state/handoff, ADR-0010, complete imported supplier,
lifecycle/password/writer/migration contracts, actual current public headers,
current manifest source and frozen initial-capture fixture. All fourteen imports
listed by the author match both this reviewed tree and its declared base. The
subsequently merged SESSION-008 supplier implementation changes no imported API.

## Findings and verdict

No blocking document defect, requirement conflict or newly unowned architectural
prerequisite was found. No author correction is requested. The proposed schema,
internal operations, finite-source obligations and literal examples are concrete
enough for STORAGETEST-020's later API/harness/test stage. This approval does not
certify future code merely because that code uses the proposed names or records.

The four empty additive relations use exact BLOB references/digest and INTEGER
scalars, preserve NULL/type distinctions and full signed times, and introduce no
raw password/token, encrypted bearer, redundant verifier parameters or public
credential diagnostic. Nonempty opaque references retain NUL/non-UTF-8 bytes.
One globally unique current session identity and installation+digest uniqueness
are explicit. Identity links have default NO ACTION; captured session epochs have
no FK to mutable current epochs. Recovery/restore can change a current fence
without rewriting old sessions. Their atomic policy, distinct replacement epoch
and restored-access gates remain the actual owning workflows; unchanged startup
preserves the current epoch.

Source inspection of the five one-line SQL texts gives physical column counts
4/5/5/13 and the complete session/account/installation closure of22 columns.
The proposed inventory is four tables, five generated indexes and one explicit
page index, ten additive objects;92+10=102. INTEGER PRIMARY KEY aliases the
installation rowid and does not add a second generated PK index. The local and
account composite keys, session PK and installation+digest index provide the stated
exact lookups; the page index orders installation/account/session bytes. Actual
selected-driver CREATE projections and generated names remain mandatory future
fixture evidence, not an executed result of this review.

The real manifest currently ends at immutable version7. Its independently framed
existing fixture reproduces71601 bytes and SHA256
`efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218`.
All imported old source/fixture/contracts are unchanged. Version8, its ID/order,
full102-object projection, actual compiled manifest and selected-driver observations
remain integrator-owned. No constructor/open/read performs automatic migration;
old executables must refuse the newer head. Explicit update, backup, maintenance,
compatibility/readiness and matching-database rollback obligations remain intact.

## Current sources, identity and conditional writes

Reads bind to the caller's actual active revocable writer session. Fixed exact
root lookup precedes linked validation, so an INNER JOIN cannot turn an orphan
into missing. All hidden format/retention/account/install revisions are selected
and checked, including revoked/expired rows and page probes. Disabled access,
revoked state and mismatching valid epochs remain complete facts for the supplier;
missing is only proven absent selected root. Unknown formats, broken links, resource
refusal and unavailability retain honest internal distinctions. Current clock,
permission, ingress and acceptance are actual owning sources, never snapshot flags.

The44-byte snapshot is `SAS1` + host-owned32-byte read scope + BE64 ordinal. A
successful snapshot-producing read consumes its ordinal, including representable
MAX; a later read refuses before wrap. Assembly follows validation of actual current
facts, and every load queries again. Scope/facade lifetime is tied to the originating
work; retained tickets cannot extend it. The selected random primitive has no
invented error return, timestamp/constant fallback or deterministic production mode.
Fresh host nonces do not promise global/historical uniqueness, future authority or
restart/restore safety without the later real source audit.

Identity/revision promises are scoped correctly: current PK/unique indexes and
full expected-record/fence comparisons are enforced decisions; random candidates
are not historical uniqueness evidence. Revisions cannot be deliberately rebound
to different bytes, current/known retained collisions refuse without redrawing,
and no deterministic historical ABA guarantee is claimed. Full source bytes and
current fences still have to match even if revision text matches. The imported
retention-free session DTO is addressed explicitly: stored retention is validated,
token-only rotation must preserve it, and legitimate retention changes also change
revision. Revocation preserves the actual selected retention. No prior snapshot,
revision, session identity or copied DTO becomes later authority.

Mutation preflight precedes SQL; selected corruption precedes CAS mismatch. Full
credential bytes and revision, immutable session identities/captured epochs/digest/
state/times, current account/install fences and host-owned acceptance/current
permission conditions compose in one nonyielding BEGIN IMMEDIATE unit. Fixed
conditional statements plus immediate same-session changes() distinguish exactly
one staged row from a known stale decision. Candidate identity/digest collisions,
including the row's own old digest, refuse without adoption or overwrite. A surprise
SQL/constraint failure retains the actual sticky writer error. Rotation preserves
all times/epochs/retention; reauth requires independently accepted fresh same-account
sign-in; authorized revoke can still target disabled/epoch-invalid rows.

The store owns neither begin/commit nor implicit cleanup. Its ticket destruction
releases no transaction authority; caller RAII and real writer finalization remain.
Staged data and owned reads are provisional. Confirmed rollback differs from
unknown COMMIT/cleanup, which fail-closes without replay; post-commit work exceptions
retain committed truth and no normal publication. No store enum conceals those
outcomes as stale/collision/missing or restores an old digest.

## Password and retained-attempt composition

The existing password port is unchanged: present/missing/unavailable only, with
256-byte account/revision/attempt and127-byte verifier ceilings. General account/
session references keep their wider configurable profile; local credentials import
the narrower existing API explicitly. A general longer account remains representable
and is not silently truncated into a local credential. Host-generated32-byte
identities fit both profiles. Raw fitting stored PHC bytes, including empty or
unsupported records, remain unchanged for the helper's own record preflight;
new credential writes instead require cheap complete supported-record preflight.

Typed internal corrupt/resource results map to the existing public unavailable
shape with neither credential nor attempt. No invented public enum, malformed DTO
or exception protocol manufactures source_invalid. The actual bounded source/bridge
returns current complete verifier bytes and real bound attempt; no writer snapshot
is held across off-writer crypto. Both helper checks take fresh queue reads and
check source-object membership, exact full credential/revision, accepted local-login
lineage, request/account association, access and current account/install bindings.

Conditional rehash still requires that actual owner in the mutation transaction,
then exact full original credential plus fences. Known commit advances only the
same acceptance's lineage to the exact committed verifier/revision and preserves
its original authentication instant. Known rollback keeps old lineage; unknown
poisons it. A reset invalidates acceptance. A mint-consumed attempt may still refer
to its accepted login for rehash, but cannot mint another session.

Accepted-attempt reservation is explicitly real host-owned retained state, through
matching terminal completion: committed consumes; unknown poisons; known rollback
releases only a still-current/fresh original acceptance for a new explicit request.
This safely composes with no durable attempt table because no in-memory acceptance
survives process restart or can be reconstructed from an external attempt string.
The real ACCESSCONTRACT-001 producer must enforce that premise, request membership,
settlement and bounded lifetimes; any durable survival needs its own reviewed
contract. No silently necessary persistence mechanism is claimed or left as a
fabricated supported feature here. Provider tags remain unavailable until actual
verified provider evidence exists.

## Resource and literal-oracle audit

Complete borrowed inputs are projected before binding/capture copies. Selected
metadata inspects class/presence/length and bounded scalars before driver-owned
variable payloads. Guarded payload queries repeat admitted properties and validate
actual QVariant class/length/key equality. Hidden metadata and probes have explicit
projections; they are not free payload. No SELECT *, load-all/sort/count, history
scan, partial fitting prefix or consumer-before-preflight is permitted. Source,
driver payload and constructed output are each bounded, and the checked semantic
peak is3*selected_bytes+mutation_bytes (4194304 at defaults), separately from fixed
scratch, framework/SQL overhead and host retained values. All queries, binding/result
occurrences and changes() also consume unchanged cumulative writer budgets.

Pages use the explicit installation/account index range, optional BLOB > boundary,
ordered session identity and page_size+1 LIMIT. Every probe is validated; each page
rechecks current authorization and facts. Cursors need not exist and grant no lease.
No account/session-count quota, multi-request snapshot or total-memory/CPU claim is
introduced. The lifecycle emits no internal digest and projects the complete reply,
including repeated cursor, before ownership.

Manually checked all24 examples against the schema and imported contracts:

- AS01–06: exact root/different digest, captured/current recovery/restore mismatch,
  unchanged reopen, disabled/revoked/expired priority, hidden format/link/time and
  typed corrupt-row refusal, with no scan beyond the selected key.
- AS07–12: both uniqueness scopes/own-digest collision, full-row stale rotation,
  authorized revocation, rollback/reopen, physical unknown versus post-commit
  exception, and same-unit visibility versus independent snapshots.
- AS13–18: exact full-verifier CAS/reset races, commit-bound lineage, accepted-attempt
  reservation/replay, length-before-payload resource priority, raw PHC versus container
  failure, and absent credential versus orphan distinction.
- AS19–24: NUL/prefix/high-byte keyset/probe/insertion semantics, complete page budget,
  full signed-time/U64 arithmetic, unknown/error truth and pure-input refusal versus
  valid foreign/expired ticket failures.

Independent Python standard-library audit used no project codec/store or SQL/native
execution. It checked the five SQL-text structures/inventory, old manifest framing,
all14 exact import identities, ordered unique AS01–24 IDs, H0 length96, ordinal
boundary encoding, signed arithmetic/full-row literals and all byte-page sequences.
The independently derived DTO results are:

| Value | Exact semantic bytes |
| --- | --- |
| Snapshot | 4+32+8 =44 |
| Supplier complete string payload / resolved strings | 11+44 =55 / 4+44 =48 |
| CompleteSessionFacts / success variant | 55+32+1+24+1 =113 /114 |
| CompleteLocalCredentialFacts / success variant | 96+10+44+1 =151 /152 |
| PasswordCredentialRead with one-byte account/revision/attempt | 96+3+1+1 =101 |
| Page source success / three-record probe page | 51+63N /240 |

MIN+U64_MAX is exactly INT64_MAX; MAX+1 is unrepresentable. Freshness300000/300001,
normal expiry86401000 and both page continuations match literal expectations.
These are proposed oracles, not executed storage, crypto or lifecycle behavior.

## Actual checks and remaining gates

Environment: Linux7.2.14-arch1-1 x86-64, Python3.14.9, Git2.56.0. Commands actually
run in the isolated tree above:

| Command/check | Observed outcome |
| --- | --- |
| git status/branch/rev-parse and source/diff reads | Clean isolated b01cc18 before this record; exact packet/import scope confirmed. |
| Inline python3 standard-library audit described above | Exit0; all structural/framing/identity/arithmetic/order/DTO assertions PASS. |
| python3 tools/check_project.py | Exit0; PASS443 task records/122 decisions, links/presets/current HTML; no draft feature execution/approval. |
| python3 -m unittest discover -s tests/tooling -v | Exit0;19 tooling tests PASS in0.370s. |
| python3 tools/progress.py render --check | Exit0; Progress snapshot is current; shared files unchanged. |
| git diff e4d46d3 HEAD --check | Exit0; supplied packet has no whitespace errors. |

No CMake/configure/build/CTest, SQL/migration execution, C++ feature, native crypto/
identity generation, real account/session/provider/network or live operation ran.
Author-reported checks remain author evidence; they are not reviewer executions.
Documentation/tooling checks fit this document-only scope; no new executable CI
suite is implied. Selected-driver projection, reviewed concrete API/stubs/fixtures/
harness/tests and FIRST intended missing behavior red remain STORAGETEST-020 gates,
then separate source/resource review and passing acceptance/selected CI for STORAGE-020.

Root still owns latest-main/prefix allocation, completion metadata, actual latest-head
CI/discussions/rules and normal PR publication/merge. Actual current/identity/password/
accepted-attempt producers and bridge, permission/clock/lifecycle/transport/publication/
reconciliation composition, setup/invitation/roles/ownership/recovery/provider/restore/
operations and supported Linux/Windows/native/browser/platform/live checks remain
owning gates. All seven integrations, three FX providers and full release scope
remain required.
