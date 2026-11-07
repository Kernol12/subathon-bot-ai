# SESSIONCONTRACT-006 — independent boundary revision review

Reviewer `/root/ingress_boundary_revision_review`, 7 November 2026; a different
actual agent/session from author `/root/ingress_boundary_revision_author` and
coordinator `/root`. Review branch `review/SESSIONCONTRACT-006-boundaries`,
worktree `/tmp/subathon-sessioncontract006-review`, starts at actual packet
`f4b0fb8ea2bd065fa81759e5dfe9157902a92eab`. Author commit
`cf8d736ef81af51a45342617baa2602871fd121b` was imported as that packet.
Only this new review record is owned/edited by the reviewer.

**Verdict at f4b0fb8: request changes.** The explicit R1→R2 engineering split is
consistent with specification v0.45 AUTH-12, SERVER-01, DEP-05 and API-01, and
the additive graph preserves release scope. Two source-backed inaccuracies below
must be corrected before approving this actual revision. This verdict supplies
no runtime, acceptance-baseline, native artifact or deployment approval.

## Reviewed packet and authority

Read root AGENTS, FIRST_AGENT_PROMPT, coordinator instructions/checkpoint,
architecture, workflow, test strategy, specification §§2/10/16/17 and relevant
decision/acceptance rows, task006, parent005, ADR0011 and the task handoff.
Reviewed the actual contract/author packet and five affected briefs; parent005's
preserved partial `4b0178cc89b340dcb6f80e6eb80bab8329ca0198` was read in its
separate worktree. Draft PR85 remains unfinished and unaccepted; draft PR87's
publication/latest CI/normal merge remain coordinator gates, not inferred here.

| Actual artifact at f4b0fb8 | Git blob |
| --- | --- |
| Transport R2 | `ec11f35a9c18fbef0e155525dd125ad9cf215bd4` |
| Reference HTTP R2 | `199ab57a31efbcb9d880952f14d0fcabaabec2e3` |
| Reference OpenAPI | `ee317612c7f890ecccecd650f8853205b87eeff9` |
| Author evidence | `71711da557dd3ab33ff48407fb86b461c699f179` |
| ADR0011 | `4193c504d022d4137102d23d0d79452a665783cc` |
| Progress graph | `c22e9ae3f1ce20b02bd111970a47d5091fbcc87c` |
| Parent005 brief | `6fb8bf13205695ab29b723b45edc6aeeb0c14e67` |
| Task006 brief | `5236913493d56d74690f9dacb779af6ed1a03c1e` |
| Native011 paired brief | `256fe46bda92003926d75cd58b967db47a21a1a7` |
| Bundled007 paired brief | `a176ce9480a89482e0b4785066868b6d9ccfe3ba` |
| Alternative008 paired brief | `04feb938b9a3cc52706eba60eb2af3c753b8c86c` |

Compared contract/schema invariants with author base `9bea6fb`; graph invariants
with actual accepted current-main base `7897e3e`, which already contains accepted
STORAGECONTRACT-002/PR86. The old transport R1 `2c33c2bbe9052455107c3056783d6bb605a5fef2`
and HTTP R1 `54f1b65a3000a8945513fa5158d0a22e50712d99` remain explicit historical
revisions. Their original review/validation receipts are not new R2 evidence.

## Blocking findings

1. **Enumerate and count stock Caddy's Via addition.** Transport §1.1's exhaustive
   transformation profile omits a real stock operation. At the pinned source,
   `prepareRequest` unconditionally appends one Via occurrence at raw-file lines
   735–736. For the H1.1 candidate its value is `1.1 Caddy`; the occurrence costs
   `3+9+4=16` semantic bytes. Existing incoming Via occurrences remain additional
   occurrences. [Pinned Caddy source](https://raw.githubusercontent.com/caddyserver/caddy/551f793700fe1550845c824470b623fd1aa03d36/modules/caddyhttp/reverseproxy/reverseproxy.go).

   Reference §5.1 lines373–382 and author evidence lines172–175 use only the three
   X-Forwarded fields. Their hypothetical three-field arithmetic is correct, but
   it is not the stated stock candidate's complete field set. The normalized
   external4097 example reaches backend262, not246. Its asserted X-Pad3851/3852
   boundaries become4112/4113, so both exceed4096. Independently counted candidate
   boundaries are3835/3836 with the same Host/Authorization/forwarding fields plus
   Via. Explicitly admit this addition, count every original and added occurrence,
   and revise the candidate literal vectors/evidence. Do not silently remove Via
   or introduce a new undocumented header mutation to preserve the old numbers.

2. **Include stock pre-handler417/505 in the edge refusal contract.** Reference
   §4 phase1 lines245–250 enumerates fixed400/431/501 or safe close. Pinned Go
   `conn.serve` handles a nonempty unsupported Expect before invoking Caddy:
   lines2088–2090 call `sendExpectationFailed`, which emits fixed417/close at
   lines2217–2232. Separately `readRequest` returns fixed505 for unsupported
   HTTP versions at1049–1050; `conn.serve` writes that fixed status text.
   These cannot be converted by the proposed later Caddy refusal routes.
   [Pinned Go server source](https://raw.githubusercontent.com/golang/go/6e676ab2b809d46623acb5988248d95d1eb7939c/src/net/http/server.go).

   Add the explicit nonreflecting stock refusals and their negative oracles,
   distinguishing them from application CommonFailure/authentication results.
   `Expect: synthetic-secret` and a syntactically parsed unsupported HTTP version
   must reach zero backend dispatch without echo/CORS/auth/state lookup. Preserve
   the bounded cleanup/log obligations. A parser patch solely to coerce these
   safe fixed statuses into the current narrow list is unnecessary.

## Security, resources and compatibility assessment

The old external4096/H2-encoded4096 oracle is explicitly superseded by finite edge
parsing and complete application semantic4096 admission. External4097 can now fit;
application4097 still fails with every forwarding occurrence counted. This changes
unimplemented engineering admission, and is not represented as renaming backend
bytes or retaining the old H18/RP14 oracle unchanged. The affected unfinished005
I08/I10/I11/I15 grammar/framing/protocol cases and future owning suites are named.
The specification requires secure proxy support but does not prescribe those old
wire constants. No completed supplier behavior or payload baseline is weakened.

Complete transport C++ declarations and §§2–4 are byte-identical. Supplier/common
and reference-path contracts are unchanged. Reference §2 and25 unaffected H rows
are unchanged. Parsed OpenAPI changes are exactly three description strings;
paths, operations, security alternatives, payload assertions and all23 examples
remain identical. This structural result is not a JSON Schema validation claim.

The critical source reasoning is otherwise coherent for a candidate awaiting real
admission: textproto appends all per-name values including empties; Header.Clone
and H1 writing preserve security occurrences; the CEL placeholder joins ALL
Connection values, while header_regexp alone is an any-value matcher. The anchored
close/keep-alive grammar prevents every security/framing nomination before stripping.
Transfer-Encoding matching consults retained Request.TransferEncoding after Go map
removal, so CL+TE cannot bypass that refusal. Equal CL, obs-fold/OWS/line delimiters,
User-Agent, Pragma and listed hop metadata are explicit exceptions rather than
general permission to erase authentication evidence. Via requires correction1.

Exact duplicate/empty/mixed/no-fallback credentials, all Cookie pairs, browser
Origin/marker and closed CORS rules are preserved. Original RequestURI versus
URL.RequestURI equality, origin-form refusal, escaped path/empty-query checks,
canonical Host/effective-port comparison and configured-peer-only overwritten
forwarding metadata preserve authority boundaries; actual configuration still
needs positive and hostile raw-wire controls. Prepared-policy mismatch still
requires a fresh request, not digest re-admission or a new boot-epoch protocol.

The source-backed40960 head ceiling distinguishes Go read allowance/slop from the
application4096 bound. MaxBytesReader is a decoded read limit, not a raw wire cap.
Go post-handler/Close drains, chunk-line/footer/positive-chunk count/trailer bounds
and reader prefetch are explicitly accounted for; the loose2165866504 raw cleanup
formula and116992 application payload sum are arithmetically correct. These are
source/control reasoning, not measured installed limits or a heap bound. Earlier
TLS/native allocations, retained maps/copies, erasure, concurrency/process/OS limits,
supported clients and actual selected configuration remain closed admission gates.

Logging review includes the pre-handler request clone/lazy error logger and default
5xx/debug paths, rather than assuming access-log omission is sufficient. The pinned
discard writer installs NopCore; the separate standard-log sink and launch-time
stdout/stderr suppression cover distinct sinks. No custom cores/exporters or raw
request placeholders are admitted. Environment/third-party/TLS/runtime/crash/export
ownership still needs selected artifact review and canary observation. No global
zeroization or installed log-safety guarantee is accepted from these fragments.

Graph audit retains all443 old IDs/1735 edges and adds exactly7 nodes/45 edges:
450/1780, acyclic. Every old review, evidence, verification and existing note is
retained. Only authorized additive dependencies/notes, epic summaries and unfinished
005's planned→blocked/owner change differ; accepted storage002 remains done.
Native011 and bundled007/alternative008 have explicit paired reviewed/red baseline
and implementation owners; source010 and full003 remain dependent. Five briefs
cover those pairs and006/parent005. No new reusable parser/proxy/envelope/mTLS/TLS
slab/runtime subsystem is mandated or hidden in this documentation change. Bundled
and alternative proxy support remain release requirements; direct TLS is optional.

## Actual checks and limits

Environment: Linux7.2.14-arch1-1, x86-64; Python3.14.9. Read-only source inspection
and existing documentation/tooling audits only; no CMake/compiler/native test,
Go/Caddy process, setup helper, new feature/helper/source execution, credentials,
provider/live operation or subagent was used.

| Actual command/audit | Result |
| --- | --- |
| `python3 tools/check_project.py` | exit0;450 tasks/122 decisions, links/presets/current HTML pass |
| `python3 -m unittest discover -s tests/tooling -v` | exit0;19/19 pass,0 skipped; tooling only |
| `python3 tools/progress.py render --check` | exit0; snapshot current; no regeneration/state edit |
| `node tools/check_progress_ui.mjs` | exit0; offline DOM/controller simulation; actual browser/OBS not run |
| `git diff --check` | exit0 |
| `python3 /tmp/sessioncontract006-independent-audit.py` | exit0 after correcting the review script's initial assumption about005's old status; exact graph/invariance/strict-JSON/23pointer checks,6allow/10deny whole-list grammar checks and independent literal/budget arithmetic pass; Via discrepancy confirmed |
| Independent inline Python relative-link audit | exit0;17 local Markdown targets exist across10 packet documents |
| Independent inline Python SHA256 audit of author source table | exit0; all26 actual local files match recorded hashes |
| Independent inline Python bounded urllib re-fetch from immutable official commit URLs | exit0; all26 official raw files match recorded hashes AND local bytes; reads capped at524289/rejected above524288,4 read-only workers; no source executed |
| Official web source inspection | Caddy reverseproxy/matchers and Go server loaded; chunked raw-page renderer returned cache miss, so no success claimed for that web rendering; the independent bounded immutable raw fetch above succeeded |

Independent audit script SHA256:
`f3eabaced98540150b523192f419552ed5f3ef638a9d702fafb8f51b7c292255`.
Its first exit1 was the review script assuming005 was in_progress at7897e3e;
the actual baseline row is planned. Corrected comparison admits only that explicit
planned→blocked delta. This was an audit-assumption error, not a project failure
or feature red run. The author's actual assigned-tree project-check failure remains
unchanged in its evidence; the integrated f4b0fb8 project check passes separately.

The complete26-file hash inventory is in the exact author evidence blob above;
it was independently verified rather than copied as an unsupported claim.
Finding source SHA256s: Caddy reverseproxy
`a873d0f5cdd7216f0708162504990641ac0c7df59ff9ea2f869437dc1ea3beb1`;
Go server `6fdef8d77c060b890e56d56f04fa1243fbf976a366aa5ef827496387a5493625`.
Source commits are Caddy `551f793700fe1550845c824470b623fd1aa03d36` and
Go `6e676ab2b809d46623acb5988248d95d1eb7939c`, not admitted installed artifacts.

No local admitted schema venv/native artifact was available or invoked. Unchanged
hosted schema CI must validate the actual revised api.json and23 examples through
the public manifest CLI after its admitted foundation gates. Current artifact CI,
required reviews/discussions/normal merge, the two corrections and their actual
different review remain006 integration gates. Native011, complete005/source010,
proxy007/008, full HTTP/session/queue/WS/webhook/client/platform/operational/live
acceptance and all seven integrations/three FX providers remain separate gates.
