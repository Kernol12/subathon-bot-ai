# CONTRACTPLAN-004 — independent document and dependency review

Author: coordinator `/root`. Reviewer: `/root/capture_codec_tests`, separate agent
and isolated `task/CONTRACTPLAN-004-independent-review` worktree. The reviewer
previously investigated the missing prerequisite read-only but authored none of
this plan, graph, brief, ADR or handoff. Actual artifacts were reviewed afresh.

**Approved for integration of the DOCUMENT/GRAPH plan only.** Final reviewed
candidate `ddadc2077cc8b469cee329c2e6746ce55ef3e74b` includes original
`eab2d8ff93bd343db279e0d4d724a373617f1b14` plus the bounded summary correction.
Base is integrated main41c4842. This does NOT approve future SESSIONCONTRACT-001
content, any authentication choice/API/schema, session baseline/red/source or
runtime capability. That future document remains planned and needs its own
actual author/different review before any consumer uses it.

## Actual artifacts and specification

Read root instructions, actual [plan brief](../tasks/CONTRACTPLAN-004.md),
[ADR-0007](../decisions/0007-session-contract-prerequisite.md),
[new document-task brief](../tasks/EPIC-007/SESSIONCONTRACT-001.md), actual affected
SESSION-002/003 and CONTRACT-013 briefs/navigation/state/HTML/handoff, accepted
wire-primitives-v1§6/reference-path-v1§2, effective-permission boundary and
specification v0.45§10/AUTH-05/AUTH-12/API-01/02/03. Actual server header and HTTP
schema inventory confirms no missing authentication producer is supplied by
existing password hashing, pure permissions or wire primitives.

ADR blob89ce1797 and new document briefd5681b77 preserve the settled256-bit opaque/
hash-only session policy,24h/30day defaults, five-minute freshness, immediate
revocation, Secure/HttpOnly/SameSite=Lax browser cookies/CSRF and desktop headers,
no account token URLs/browser local storage. They choose NO hash algorithm,
token encoding, cookie/header name/syntax, Origin/CSRF method, authentication
status or authority grant. Those are explicit future contract decisions and
validation obligations, not capabilities manufactured by this planning change.

The supplier/API/transport document is bounded to session resolution and credential
transport imports needed by SESSION002/003 and HTTP013. Its required inventory
includes actual producer ownership/currentness, complete/missing/expired/revoked/
unavailable facts, clock/account/storage questions, issuance/rotation/revocation,
secret redaction, duplicate/mixed credentials, trusted origins/proxies, cross-origin
reads/writes, finite borrowed/scratch/result limits, error priority and independent
literal success/boundary/refusal examples. Negotiation cannot finalize auth.
The explicit instruction to split the document if it exceeds a focused review
preserves the necessary imports rather than permitting a large implementation or
an incomplete accepted contract. No login/account/recovery/provider behavior is
silently declared implemented.

## Independent graph and preservation audit

Compared actual final JSON with `git show 41c4842:progress/state.json` using sets of
IDs/edges and direct field assertions. Old420 IDs and1608 edges all remain;
new422 IDs/1619 edges are exactly2 added records and11 added edges, no removal:

- CONTRACTPLAN-004 → CONTRACT-005, SESSION-001, STORAGE-002 (3).
- SESSIONCONTRACT-001 → CONTRACTPLAN-004, CONTRACT-005, SESSION-001, STORAGE-002 (4).
- CONTRACT-013, SESSIONTEST-002, SESSIONTEST-003 and EPIC-007 each additionally
  require SESSIONCONTRACT-001 (4).

Every old status, owner, verification, review and evidence field is unchanged.
At this reviewed phase plan004 is in_progress and document001 planned/unclaimed;
neither document acceptance nor runtime completion is invented. Original
SESSION002/003 implementation→baseline/prerequisite lists remain byte-equal,
as do APITEST010/API010 runtime lists including actual SESSION003/API001/
ACCESS002/STORAGE002. New document001 depends on no downstream session runtime,
so no document→runtime→baseline cycle is introduced. Tool graph validation agrees.
All existing release tasks/provider/FX/platform/renderers/localization/recovery
scope remain through the preserved IDs/edges and explicit ADR boundaries.

Concurrent STORAGE018 claimd73b292 remains separately in_progress under its actual
owner on task/STORAGE-018-store-author; its runtime prerequisites are intact.
This plan inherited main's planned row rather than writing over the independent
claim. The actual worktrees/refs remain present. Root's integration must retain
that active row/activity when merging planning changes into its task branch;
copying this whole progress snapshot over the claim would be incorrect.

The diff is documentation/navigation, progress and generated HTML only. No source,
public interface, SQL/schema/migration, assertion/fixture, root build, dependency
pin or workflow changed. Existing automatic project/graph/link/render/tooling and
stable-component CI adequately cover the plan; no new executable suite or required
check configuration is justified here. Schema/native/authentication/provider
execution is neither needed nor claimed for this planning review.

## Finding resolved and actual checks

Initial actualeab review found EPIC007's JSON summary still said7 work packages,
while new document child and navigation correctly showed8 packages/15 children.
Author correctionddadc20 changes ONLY that summary7→8 plus regenerated HTML.
Reviewed the exact diff and reran final consistency; no auth content, edge, status,
owner or evidence changed in that correction. No unresolved review finding remains.

Actual review commands on isolated checkout, all exit0:

```sh
python3 tools/progress.py validate
python3 tools/progress.py render --check
python3 tools/check_project.py
python3 -m unittest discover -s tests/tooling -v
git diff --check
```

Final checks report422 tasks/122 decisions, current generated page and valid local
links/presets; all19 tooling tests pass. Independent Python graph assertions above
also pass on finalddadc20. No new feature bodies, live/provider calls or credentials
were used. Only this review note is authored/committed by the reviewer.

Remaining integration gates: root's completion metadata must preserve these exact
semantics and document001 planned status; latest-head CI, current GitHub rules/
review discussions and eligible normal PR merge remain separately required.
Future document/API/baseline/source and platform/live gates remain unapproved.
