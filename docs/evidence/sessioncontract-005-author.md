# SESSIONCONTRACT-005 — partial author checkpoint

Author `/root/account_ingress_contract_author`, 7 November2026.
Branch `task/SESSIONCONTRACT-005-author`, worktree
`/tmp/subathon-sessioncontract005-author`, starts at coordinator claim
`20ac405bcee1db44120abcdf3a0b90f570d4fad7` from actual main
`e440c7f050fdee04c39eb5b74b01c9f079124ed6`.
Own ONLY [account-ingress-v1.md](../contracts/account-ingress-v1.md) and this note.
The coordinator owns state/HTML/shared files/pins/build/CI/deploy/publication.
No storage-contract file or runtime source was edited.

This is an honest **partial checkpoint**, not completion of SESSIONCONTRACT-005.
The coordinator requested a bounded different read-only audit of original-app-
listener versus original-external-edge semantics before deciding any explicit
contract revision or prerequisite split. No new proxy subsystem is selected here.
The direct-listener proposal cannot complete the parent as direct-TLS-only.

## Authority and imported facts

Read root AGENTS.md, FIRST_AGENT_PROMPT.md, architecture/workflow/test strategy,
progress/state.json, server/AGENTS.md, task005/010 and coordinator005handoff,
ADR0010, selected specification §§2/10/16/17/decision references, existing
SPIKE001 dependency/bridge/feasibility evidence and accepted contracts. The first
assignment's old environment/test-first work is historical; this document assignment
authorizes no repeated native baseline or implementation.

Imported exact Git blobs at claim20ac405:

| Contract | Blob |
| --- | --- |
| session-transport-v1.md §§1–5 | `2c33c2bbe9052455107c3056783d6bb605a5fef2` |
| session-resolution-v1.md | `c5f5a6e71664042e18475068d3caf577b0c8d009` |
| reference-http-v1.md | `54f1b65a3000a8945513fa5158d0a22e50712d99` |
| wire-primitives-v1.md | `11eeb5f422a174e042143df6b476ae1d717eeacf` |

No import was revised. Supplier ownership/crypto/current-resolution and transport
failure priority, closed CORS, fresh-request policy mismatch and credential rules
remain unchanged. Reference HTTP explicitly has separate external and semantic
4096-byte request limits; treating proxy-shortened input as original external
bytes would change that contract and is not authorized implicitly.

## Actual source authentication and observations

Read the primary checkout's ignored `build/deps/drogon-1.9.14-source` ONLY.
`git rev-parse HEAD` = `3aae769cc405cf5abb9d477675c6430924f02c7c`;
`git status --short` empty. `git submodule status trantor` and
`git -C trantor rev-parse HEAD` = `78e480f5e138acdc05f76c55a2716718cd846364`;
Trantor status empty. These match tools/setup_drogon.py and existing dependency
evidence. Read parser/implementation/server/access logger/public framework headers,
MsgBuffer/TcpConnection/OpenSSLProvider/TLSProvider and request reset/pooling code.
No `cmake`, compiler, CTest, native executable, framework process, package manager,
setup helper or installed framework was invoked.

Do not trust a clean cache alone: I downloaded the following bounded public files
from their exact-commit official raw HTTPS URLs and compared bytes to that cache.
All eight comparisons were TRUE. Output files live only under disposable
`/tmp/sessioncontract005-official`; they are not repository artifacts.

| Exact source relative to Drogon or Trantor root | SHA256, official and cache equal |
| --- | --- |
| Drogon lib/src/HttpRequestParser.cc | `da05d22d153ab7493e1f9109ebfe1f16a067f972d4b3439d03946393b5f678e6` |
| Drogon lib/src/HttpRequestImpl.cc | `54cd93d9fd5b7ace63abf9a59e5a4060a0d923b248bfee0643d672e4acc1f658` |
| Drogon lib/src/HttpServer.cc | `e78d9cfc1800862928316c2eb6131c28337043ec4a42c5241ed9b76c9dbb0d9a` |
| Drogon lib/src/AccessLogger.cc | `bf2a901e54533edfd15a09599fd609267d6e0fd42dd3092e5cafc30bf8add82e` |
| Drogon lib/src/HttpRequestImpl.h | `b43e3c289b15668c67fa056c19d5be666ba6d559d0c5ae45b0b52a4d93016de3` |
| Trantor trantor/utils/MsgBuffer.cc | `c0879566a68298719b1e309c6896918759e18c0356f6bdea9f9207c417371737` |
| Trantor trantor/net/inner/TcpConnectionImpl.cc | `a86acffff34a26c6f7d2dd04344546a859c7de070235a23f2e70f66dada59243` |
| Trantor trantor/net/inner/tlsprovider/OpenSSLProvider.cc | `1a9be775dc4abb74d0b0230cfe80fa627eb1359c290a633db037bc5bceaf6e4f` |

Reproduction: use Python urllib HTTPS reads capped at524289 bytes, reject if
len>524288, SHA256, then exact `data == Path(cache/name).read_bytes()`;
Drogon URL prefix
`https://raw.githubusercontent.com/drogonframework/drogon/3aae769cc405cf5abb9d477675c6430924f02c7c/`;
Trantor prefix
`https://raw.githubusercontent.com/an-tao/trantor/78e480f5e138acdc05f76c55a2716718cd846364/`.
Two successful read-only download commands each exited0 (five Drogon files, then
three Trantor files). Initial sandbox urllib attempt exited1 at DNS resolution,
before downloading anything; the allowed read-only network retry succeeded.
The web-rendered HttpRequestImpl representation initially differed from actual
bytes. Official bounded downloads resolved that discrepancy; the rendered text
was not treated as canonical. This authenticates inspected bytes, not any installed
binary, build provenance, runtime exploit or TLS behavior.

Source findings: selected parser calls addHeader before application advice;
duplicate headers/cookies lose information and raw Cookie TRACE precedes an app
filter. Also found raw Expect WARN/path TRACE, direct parser-error output outside
pre-sending advice, default OPTIONS reflection and JSON parser error logging.
Request pooling/reset and MsgBuffer retrieval do not establish secret erasure.
MsgBuffer has geometric copying/spill-buffer reads; OpenSSL provider grows plaintext
before dispatch. Existing semantic preflight or logging severity cannot prove
preallocation limits, duplicate recovery or erasure of previous copies.
The contract links exact primary source paths and distinguishes facts from patches
proposed for future review. No vulnerability/exploit was executed.

## Official documents actually consulted

Read official RFC9112 parsing/framing and RFC9113 field/Cookie guidance via web,
Caddy reverse_proxy/server options/access-log documentation, and Caddy v2.10.2
release/go.mod pages. Caddy/Go were not installed, selected, pinned or executed.
The current docs' controls/redaction describe ingredients; no arbitrary stock
Caddy/alternative path is asserted lossless, external-bound-preserving or log-safe.
Libsodium secure-memory semantics remain imported from the accepted supplier;
no global process/heap/register/swap/dump erasure guarantee is added.

## Proposed scope and open decisions

The draft names pre-read/pre-growth Trantor/TLS, pre-setPath/addHeader/body-reserve
Drogon parser, and HttpServer dispatch points; proposes fixed arenas/offsets,
owned secret wiping, bounded HTTP1.1 framing, TLS/authority policy facts, immutable
policy revision production/change and closed CORS. These are reviewable proposals,
not claims the existing source already supports them. It keeps TLS internal/native
bounds and host queue/connection budgets separately owned and open.

The strict original-external-edge interpretation could require substantial proxy
work. The draft does not select a Go parser fork or custom proxy protocol. It leaves
three possible audit outcomes explicit: prove actual selected proxy compatibility;
make a minimal **explicit**, independently reviewed specification-justified contract
revision; or split a necessary reusable runtime prerequisite into focused tasks/PRs
before implementation. None may silently drop Caddy/alternative proxy scope,
external4096 bytes, ambiguity rejection or secret-safe first-parser/log handling.
The receive/TLS allocation/erasure correction is also a substantial prerequisite
candidate. Coordinator decides exact records/dependencies/blocked status.

Proposed direct framing is an explicit review choice: HTTP1.1, strict CRLF/Host,
one digits-only Content-Length, no transfer coding/Expect/trailers/compression/
upgrade. It does not claim a selected proxy already enforces this or redefine
webhook/WebSocket transport. The full agreed client/proxy/platform scope remains.
API001 owns actual lifecycle/queue/backpressure and policy serialization;
SESSION003 owns complete parsing/egress/routes/CORS/client integration;
SESSIONTEST010/SESSION010 require their own final contract, independent tests,
reached intended real-stack red, implementation, source/native/config review and CI.

## Limited author checks

Environment observed: Linux x86-64, Python3.14.9; UTC at inspection
`2026-11-07T11:11:03Z`. No native/test environment validation claim.

| Actual command/check | Observed result |
| --- | --- |
| Worktree `git status --short --branch`, `git rev-parse HEAD` | Correct task branch20ac405; initially clean |
| Selected source Git identity/status/submodule plus bounded official byte comparison | Pins above; clean; all eight exact comparisons true |
| `git ls-tree HEAD` on four imported contracts | Exact blobs above |
| Independent inline Python byte arithmetic, no project parser/session implementation | A237/B241/C199/D277; desktop149; X-Pad3938→4096 complete wire; name+value3943; all assertions pass |
| Same independent arithmetic | 16384+34816+65536=116736; plus TLS plaintext16384=133120; head+body100352; U64 epoch/generation serialization maximum44 |
| Independent Python local Markdown link check | All7 relative targets in the two new files exist |
| `git diff --check` / `git diff --cached --check` | 0 on tracked starting diff / staged new documents before commit |

No product tests, test baseline, intended red, runtime traffic, credential, build,
live provider, deployment or supported-platform verification occurred. The proposed
A–D/I01–21 raw-wire oracles need independent review; expected results are literal
contract-derived values, not generated by the implementation. Existing spike and
supplier receipts remain historical; no old red was repeated.

The coordinator must complete document/link/progress/tooling checks and independent
source/contract audit, resolve proxy/native/config decisions and record the exact
prerequisite split before final005 acceptance. The first staging attempt could not create the linked worktree index lock under
read-only shared Git metadata; a scoped Git-only escalation was used for the
authorized two-document checkpoint commit. No source-cache mutation was attempted.
Publication/PR/CI gates remain with
the coordinator. This author cannot approve its own review or mark the parent done.
