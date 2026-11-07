# SESSIONCONTRACT-005 — independent complete-solution review

Reviewer `/root/account_ingress_solution_review`, 7 November 2026, a different
actual agent/session from resumed author `/root/account_ingress_contract_author`
and coordinator `/root`. Review worktree `/tmp/subathon-sessioncontract005-review`,
branch `review/SESSIONCONTRACT-005-solution`. Only this new record is owned/edited.

**Verdict: approved for integration of the complete revision2 DOCUMENT proposal
at `08b6f01ba789f4d7b43890d284583af446101035`. No blocking finding.** This approves
the source-backed solution and literal contract for the separately reviewed native
and source baselines. It does not admit a patch, ABI/build artifact, proxy/config,
runtime path, supported client/platform, or any dependent implementation.

## Actual packet and review scope

Packet tree `e5bb4bb1a3bc4b967915660d9d4f58b32074fbfd`; actual resumed author
`bbacc5e43241c7d822c3634062d17079eef375ff` imported at08b6f01. Both author-owned
documents are byte-identical to that author commit. Current accepted-main input
is `e712d85`, including accepted006R2/current clock/physical session contracts.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| [Complete ingress revision2](../contracts/account-ingress-v1.md) | `00a4fa46b93af3e71cfeaf8ddf000dc4acf98f42` |
| [Resumed author evidence](sessioncontract-005-author.md) | `2985712aa93cbdccddd73c1b751eaa93ff8aaca3` |
| [Accepted transport R2](../contracts/session-transport-v1.md) | `0d0cbde9450abc5d7f99b7a97b03cfe41f385890` |
| [Accepted reference HTTP R2](../contracts/reference-http-v1.md) | `fe69e3c3b06fbe63d1d2beaea458cbacf905032f` |
| [Accepted006 actual review, including correction](sessioncontract-006-review.md) | `ae0369085712b1d764e9cfdc3d3009b82b4e504d` |

Consulted root AGENTS/FIRST, server scoped instructions, architecture/workflow/test
strategy, specification v0.45 account/deployment/development requirements and
AUTH-12/15, SERVER-01, API-01, DEP-05; actual005 brief/handoff, accepted006
author/source/full83bb1ca3 and corrected0990c608 review, ADR0011, native011,
source010 and bundled007/alternative008 paired briefs. Supplier/common wire/current
clock/physical session imports retain all six exact blobs in the proposal table.

The original partial4b0178cc/importd720282 is not the solution under review. Its
entire original author note remains an exact suffix of the current evidence file,
including the original eight official comparisons and failed/successful receipts.
The old read-only proxy audit and accepted006 review retain their original scopes;
neither substitutes for this separate complete005 review. No old red is replayed.

## Native source and concrete feasibility assessment

Read actual selected Drogon `3aae769cc405cf5abb9d477675c6430924f02c7c` and Trantor
`78e480f5e138acdc05f76c55a2716718cd846364` in the primary ignored source cache.
Both Git statuses are clean. Independently rehashed all eight files against the
preserved official/cache SHA256 table: all match. The installed library was not
loaded, authenticated or run. Additional interface/channel files below were read
and compared byte-for-byte with bounded immutable official HTTPS downloads.

| Additional actual source | SHA256, official/cache equal |
| --- | --- |
| Drogon `lib/src/HttpRequestParser.h` | `d23100e8a081493cbda22fc576298e59fd0cac307d888bc49cd93f8fef42bd34` |
| Trantor `trantor/utils/MsgBuffer.h` | `7732e23d7d49916e4e208577b72a249156d8a415bb59a7efe0b596a566549dfa` |
| Trantor `trantor/net/inner/TcpConnectionImpl.h` | `92c498c4b1b1e904e319fa1802f8babe1c58db04bd7f0b327bb862abc7f6c1f4` |
| Trantor `trantor/net/Channel.h` | `21203ef8c36af9a1fab741c10e063291d9c391b97d35b30ed742bae5481b12c6` |
| Trantor `trantor/net/Channel.cc` | `5f430f63fcfd32725437fd31099469178cb4cc6d68e01ce3da726eb652d3082e` |

Exact source positions below refer to authenticated raw files, not web-rendered
line numbering. They establish where the proposed correction must act; the
replacement behavior still needs the actual011 test-first patch/source review.

- **Receive before parsing.** `MsgBuffer.cc`34–36 initializes payload plus8 prefix;
  39–59 can compact without wiping or allocate/copy;140–146 retrieves without
  erasure. At148–171, writable>=8192 makes iovcnt1. A successful read then cannot
  populate extBuffer or enter the append/growth branch. The8192 stack object still
  exists; this reasoning says it receives no secret, not that it is absent.
  `TcpConnectionImpl.cc`130–192 reads before invoking the application callback;
  therefore the guard belongs before readFd, not in onMessage. Its private channel
  already owns read enable/disable (`Channel.h`191–205). Existing dispatch can have
  a ready read callback (`Channel.cc`71–117), so a pause alone never replaces the
  EACH-read precondition. Constructor/factory profile selection must precede read
  enablement (`TcpConnectionImpl.cc`276–290).
- **Original method/target and every occurrence.** `HttpRequestParser.cc`151–172
  consumes the method and converts it to an enum before processRequestLine.
  Consequently the proposal's original method/whole-line bound also requires
  correction at kExpectMethod; processRequestLine alone cannot recover it. This is
  already demanded by §§2–3's original method view,1..16 limit, complete line bound
  and split-input tests. SESSIONTEST011 must cover partial methods around the old
  seven-byte cutoff as well as target processing before setPath/setQuery. That
  later function currently decodes/copies paths (`HttpRequestImpl.h`160–175) and
  copies query bytes at224–227. Capture/fixed slots and trusted route identity
  avoid these raw copies on the proposed account path.
- **Legacy maps, framing and body are bypassed.** Parser headers at198–224 call
  addHeader before selected-map framing, Expect and body reserve at226–340.
  `HttpRequestImpl.cc`440–550 allocates/normalizes fields and logs Cookie at466;
  duplicate maps cannot reconstruct multiplicity. Body reserve/append at809–871
  can allocate strings or create temp files. The proposal captures all fields in
  the fixed arena, validates occurrence-list framing/projected route size, and
  keeps body bytes in its route-owned bounded owner. It does not allow these raw
  inputs into legacy maps, request body/cache, URL parser, JSON parser or serializer
  merely because a later callback is private. Ordinary routing receives only the
  trusted nonsecret route/framing projection. This is a concrete bypass design,
  not an assertion that current upstream already implements it.
- **Stable source views and revocation.** The private single-use lease and outer
  synchronous visit enclose the entire transport call, so read_current returning
  cannot end view lifetime early. The unchanged span<AccountHeaderView> API needs
  stable fixed storage for up to64 views in the concrete lease/scope declaration;
  offset records cannot themselves be reinterpreted as that span. This belongs to
  the expressly separate fixed records/lease facts, without another owning field
  string/map or variable arena copy.011/010 must review its concrete representation,
  invalidate live source access at scope end and never test a dangling C++ reference
  by invoking undefined behavior. No raw view, lease, request attribute or retained
  HTTP body may enter Qt work; only the owning route's admitted DTO/prepared result
  may move onward.
- **Pipeline, retention, errors and teardown.** `HttpServer.cc`160–250 currently
  parses multiple requests before onRequests, stores them, and resets the parser;
  callback packs/response handling can retain requests through send completion.
  Parser pool deleters at92–116 may queue reset, while `HttpRequestImpl.h`71–105
  only clears strings/maps. The proposal instead permits one active raw scope,
  leaves later bytes solely in the fixed receive owner and requires API001 admission
  before continuing. Raw bytes must never enter those pooled/queued/sent request
  holders; later response retention cannot postpone ingress erasure. Native close
  at292–312 disables the channel but does not wipe the buffer, so011 must add wipe
  on release/error/timeout/cancel/EOF/destruction, including pending pipeline bytes.
  Compaction must preserve admitted unread bytes and erase consumed/obsolete copies
  before release/reuse. Scope erasure on every exit includes exceptions.
- **Logs and response paths.** Cookie TRACE, Expect WARN (`HttpRequestParser.cc`311),
  path TRACE (`HttpServer.cc`421–424), JSON diagnostic (`HttpRequestImpl.cc`33–57),
  request serialization (`HttpRequestImpl.cc`193 onward), AccessLogger formatters,
  direct parser errors (`HttpServer.cc`198–221) and default OPTIONS reflection
  (1371–1386) are all addressed before raw data can enter them. The early private
  dispatch/owned bounded refusal must cover parser failures too; a late log or
  response scrubber does not satisfy the proposal. Only fixed admitted diagnostics
  may cross error/send callbacks. Native output and secret credential-response
  copies still have their separate transport/SESSION003 owners, not a new global
  ingress wipe guarantee.

The concrete plain receive mode is viable as a bounded adjustment to existing
storage/channel/parser ownership. For the reference profile the raw head40960 plus
body<=4096 fits the65536 receive owner; consumed data can be copied into fixed
capture owners and erased before a new read. Coalesced later requests do not create
additional leases. The hard pre-read check safely prevents spill/growth even if
processing cannot free enough space. This is source-based feasibility reasoning,
not a measured guarantee of progress, resource usage or patched lifetime safety.

The independent native sum is104472 configured byte regions plus fixed records/
lease facts. It excludes fixed framework/view objects, native crypto/output/TLS,
allocator/kernel/global/concurrency ownership; it is neither exact heap allocation
nor additional disjoint116992 application ownership. The document explicitly keeps
those actual admission limits separate. Optional direct TLS remains unavailable
without its own native manifest/tests; it does not force a TLS slab/runtime design.
The [official libsodium erasure documentation](https://doc.libsodium.org/memory_management)
supports the selected owned-region erasure primitive and its limits, not global
erasure from ordinary clear/free or a successful request.

## R2 security, edge and policy assessment

Reused accepted006's authenticated26 Caddy/Go source comparisons; independently
hashed all26 local files against its table, all unchanged. Read affected raw Via,
retained TransferEncoding, all-value Join, URL and pre-handler417/505/log-sink sites.
This review does not repeat26 network downloads or claim new proxy execution.

The complete proposal preserves the exact admitted transformation exceptions:
all joined Connection values, retained-TE refusal, ordinary User-Agent/Pragma/hop
metadata, raw URI roundtrip/authority, addedVia16 and all incoming Via, framing and
bounded external cleanup, fixed pre-handler417/505/501 versus application failures.
It never reinstates external4096 or H2 encoded4096. Every actual backend metadata
occurrence consumes field_count/H/complete semantic4096; no hidden Via discount.
Both bundled Caddy and an operator's existing proxy alternative remain required,
with exact loaded artifact/config/client evidence under007/008. Direct TLS remains
optional; no envelope, mTLS, durable boot epoch or new proxy subsystem is mandated.

Canonical configuration, full scheme/host/effective-port comparison, actual native
peer/listener facts and admitted isolation are separate from spoofable forwarding,
Host, Via or SNI. The default proxy's overwrite/single-hop requirements and exact
backend cardinality validation preserve authority. Unknown isolation/loaded config/
TLS facts fail unavailable; an arbitrary localhost address is not sufficient proof.
The root discard/NopCore/stdlog and launch-time stdout/stderr sink requirements
preserve006's pre-handler clone/error/debug/panic reasoning. Missing output alone
does not establish allocation, copy or erasure safety in Go/TLS/native owners.

The bounded immutable snapshot and live-owner unsigned64 generation model is
coherent. The maximum `g`+decimal reference is21 bytes. Generation equality is
meaningful only within the same live serialized owner; no prepared scope/queue is
persisted/restored across owner replacement. Admission stop, validated publication,
edge/native readiness coordination and drain/cancel precede resume. Queue execution
compares the ORIGINAL policy reference with current same-owner policy before
supplier lookup, protected effect or disclosure. A mismatch requires503 and a fresh
actual request. A retained pointer/caller flag/digest cannot create current readiness
or rebind an old request. Actual clock/store/current login/permission/restore remain
their own producers, not properties supplied by ingress.

Credential cardinality, empties, mixed-form priority/no fallback, all Cookie pairs,
exact Origin/marker, browser GET/HEAD/unsafe rules and closed CORS/OPTIONS remain
unchanged. Complete HTTP/WS/webhook/browser/native/publication/platform release
scope and all seven integrations/three FX providers are preserved.

## Independent literal audit and actual commands

Environment: Linux7.2.14-arch1-1 x86-64, glibc2.44, CPython3.14.9; inspection/checks
on7November2026, last pre-record timestamp12:48:50UTC. Inline audit code used only
stdlib text/Git/hash/integer/regex operations; no future parser/transport/supplier
implementation generated expected results.

Independently reconstructed literal octets from A–D:237/241/199/277; desktop
control149. Re-derived the supplierZ digest from its literal framed32 zero bytes
with stdlib BLAKE2b-256: `a226d544ff92f3247dc784e949c846999fde801e4581c6bb219fde070591d3ea`.
Checked I01–24 inventory and each semantic failure priority against the imports.
Duplicate credentials A/D401 and Origin/marker B/C403 occur before crypto/lookup.
Empty/combined/mixed Cookie/Auth/Origin cases, native versus edge syntax/CL,
framing/body/protocol, policy changes, secret canaries and ABI/lifetime failures
retain the specified owning refusal phases and valid positive controls.

Independent byte projections: external4097→OWS-normalized159; forwarding87+Via16
gives262. X-Pad3835/3836 gives app4096/4097. IncomingVia34+addedVia16=50;
complete controls286/296. Native104472, application116992, decoded cleanup528386,
raw cleanup2165866504 and generation21 all recount correctly. Whole-list grammar
checks pass5 valid/8 hostile nomination vectors; this is not Caddy/CEL execution.

| Actual command/audit | Observed result |
| --- | --- |
| `git status --short --branch`, `git rev-parse HEAD`, `git rev-parse HEAD^{tree}`, `git ls-tree HEAD ...` | Correct clean assigned branch/packet/tree and blobs above before this record |
| `git status --short`, `git rev-parse HEAD`, corresponding `git -C trantor` source checks | Both pinned source commits, clean statuses |
| Independent inline SHA256/import/Git/history audits | Original8/source26 match; six imports frozen; both author/import files identical; original4b author evidence exact suffix |
| `sed`, `nl -ba`, `rg` on actual selected/cache sources | Read-only positions/ownership/logging/capture/channel evidence above |
| Bounded inline urllib immutable official comparisons for5 extra source files | First sandbox attempt DNS failure before downloading; approved read-only network retry exit0, all5 exact bytes match, each read capped524289/rejected over524288; no local source writes |
| Web open of immutable MsgBuffer.h/Channel.h and official libsodium page | Two raw-source cache misses, not source proof; libsodium page retrieved; exact raw comparisons above resolved the source gaps |
| Independent inline literal/digest/capacity/grammar/history audit | exit0; all asserted counts/projections/identities above pass |
| Independent inline relative Markdown link audit | exit0; all14 current proposal/author-note relative targets exist |
| `python3 tools/check_project.py` | exit0;450 task records/122 decisions, links/presets/current HTML pass; draft-test informational output is not native execution |
| `python3 -m unittest discover -s tests/tooling -v` | exit0;19/19 pass,0skips,0.379s; tooling only |
| `python3 tools/progress.py render --check` | exit0; snapshot current; no state/HTML regeneration or edit |
| `git diff --check` | exit0 before this record; final record/links/whitespace checked before commit |

## Handoff and remaining gates

The complete proposal is sufficiently concrete for SESSIONTEST011 to define/review
its private declaration, real selected-stack controls and allocation/copy/log/lifetime
oracles. Method capture, stable fixed views, pre-read guards, wipe-aware compaction,
one-scope pipeline continuation and every error/retention/destruction path must be
tested and separately source-reviewed at that stage. A semantic budget or this
source review cannot establish actual patched behavior. Missing API/compile/setup
or universally unavailable controls cannot supply a valid behavior red.

Native011, actual source/current-policy010, loaded proxy007/008, API001 lifecycle/
queue/backpressure and full SESSION003 transport remain distinct test-first owners.
No substantial new prerequisite was found necessary for this design. A genuinely
necessary reusable receiver/TLS/runtime expansion must be split before code as
required by the existing briefs/workflow; none is implicitly approved here.

No subagent, compiler/CMake/CTest/native ELF/helper/feature body, Go/Caddy process,
SDK/package setup, runtime traffic, real credential/provider operation, live check
or platform verification ran. Existing document/project/tooling CI fits this packet;
future real-stack suites/selected CI remain owning implementation gates. Coordinator
must integrate only this review, maintain state/HTML/handoff, then satisfy actual
latest-published PR85 CI, reviews/discussions/current rules and normal merge gates.
The author cannot self-approve and this record does not claim those gates passed.
