# Native persistent WebSocket profile — proposal v1

APICONTRACT-001; identity `subathon.native-websocket-profile.v1`. Document only;
no patched SDK, test, runtime or RFC conformance certification is supplied.
Base main `1dcf2b61f65a4d87a29a0ac52e68d7c8b0d20c90`; claim `806dba7`.
Import [host lifecycle v1](api-host-lifecycle-v1.md) and
[wire primitives v1](wire-primitives-v1.md). This server-side native frame profile
is below JSON/command/auth/snapshot protocols. It does not change SESSION-011's
account H1 ingress profile, which REFUSES Upgrade, or grant account endpoints,
authenticated Upgrade, API-003 synchronization or provider capabilities.

## 1. Exact source basis and admission boundary

The inspected clean selection is Drogon **1.9.14**, commit
[`3aae769cc405cf5abb9d477675c6430924f02c7c`](https://github.com/drogonframework/drogon/tree/3aae769cc405cf5abb9d477675c6430924f02c7c),
with Trantor commit
[`78e480f5e138acdc05f76c55a2716718cd846364`](https://github.com/an-tao/trantor/tree/78e480f5e138acdc05f76c55a2716718cd846364).
The existing stock SPIKE-001 SDK is an input, not an implementation of this
profile. The following are source observations, not executed native results:

| Selected source | Observed limitation that correction must address |
| --- | --- |
| Drogon `lib/src/WebSocketConnectionImpl.cc`, parser and `onNewMessage` | One `message_`/`type_` is shared by fragmented data and controls. Control can consume/contaminate the data accumulator. Explicit valid sequence, RSV, complete Text/Close UTF-8 and Close-body validation cannot be assumed from a successful echo. |
| Same file, Close dispatch | Raw received Close payload DOES reach the callback, AFTER `connPtr->shutdown()`. A one-line raw echo does not establish correct closure. |
| Trantor `trantor/net/inner/TcpConnectionImpl.cc`, `shutdown`/`sendInLoop` | With no queued output, shutdown enters Disconnecting and later send is rejected. With pending output, `closeOnEmpty_` defers closure; that distinct case does not prove the idle path correct. |
| Trantor `trantor/utils/MsgBuffer.cc`, `readFd`; TCP read callback | Readv can fill current writable space plus an 8192-byte stack buffer, then grow before WebSocket parsing. Logical message size alone is not a preallocation/native buffer bound. |
| Trantor `trantor/net/inner/Socket.cc`, bind/listen | Failure terminates the process; the host's one-lifetime/process-terminal startup profile applies. |

Root owns admission of a DISTINCT corrected source/build/install prefix. Before
native tests, record exact upstream/submodule commits, complete ordered patch
bytes/digests, clean-versus-modified status, compiler/C++/Qt/OS/ABI, CMake options,
transitive library identities, build/install outputs and license/notice obligations.
Use the selected Release/shared/C++20/OpenSSL recipe unless separately reviewed;
no silent source upgrade or feature toggle is authorized here. Preserve the clean
stock SDK and never describe patched code as pristine upstream. Root must admit
every touched layer, including Trantor read/buffer correction if needed; the profile
does not approve a particular patch merely by stating desired behavior.

## 2. Server framing and separate message/control ownership

The normative source is [RFC 6455 §§5/7/8](https://www.rfc-editor.org/rfc/rfc6455.html).
This profile negotiates NO extensions; all RSV bits are zero. Client frames must
be masked, server frames unmasked. Require minimal length encoding, a clear high
bit in 64-bit lengths, defined opcodes, unfragmented controls and control payloads
at most 125 bytes. Reject invalid headers before payload ownership. Fixed masking
keys below are synthetic fixture values, never a real-client randomness policy.

The data state is `idle` or `fragmented_text/binary`. A Text/Binary frame can
start only in idle; continuation only during that same unfinished message.
Finish on its FIN continuation. Controls never change the data opcode, consume
its bytes or reset its UTF-8 state. Permit legal control insertion between data
fragments and preserve data order; no interleaved second data message. A completed
Text message must be Unicode scalar UTF-8, including code points split across
fragments; no replacement, normalization or locale repair. Binary remains exact
bytes. Deliver one owned message only after its final validated frame; never a
prefix, control payload or partially decoded Text. Empty messages are valid.

Control parsing uses its own at-most-125-byte owned body. Ping produces an
unmasked Pong with identical payload while no peer Close has been received.
Unsolicited Pong produces no reply and no data callback. Pending Pong responses
may coalesce to the most recently processed Ping under the RFC rule; at most one
pending Pong is retained. Ping during local closing never reopens application
data or extends the Close deadline. After peer Close, send no new Pong or data.
Control callbacks, if exposed internally, have separate bounded ownership and
cannot masquerade as a Text/Binary application message.

On framing/sequence failure, stop application admission, discard unfinished data,
and attempt one protocol-error Close 1002 if the transport/output reserve permits;
then bounded force-close. Invalid Text or Close-reason UTF-8 uses 1007; admitted
resource/message exhaustion uses 1009. These are new local failure statuses,
not invented received statuses. Invalid input never reaches application callbacks.
No network callback executes SQL or authoritative state work.

## 3. Close body, status and state

Maintain separate `close_sent` and `close_received`, initially false, plus one
absolute deadline established by the first local Close or valid peer Close.
Application-data admission ends immediately when either becomes true. Local
Close is serialized on the connection's IO loop after already admitted output,
using the reserved control capacity; no subsequent data frame is enqueued. Drop
unstarted application-output leases rather than append them after Close. Bytes
already handed to the kernel cannot be withdrawn. Do not send a second Close.

Preserve received closure as a tagged observation: `no_close_frame`,
`close_without_status`, or `close_with_status{code, reason_utf8_bytes}`. A valid
empty body is `close_without_status`; a two-byte 1000 body is a PRESENT status
with an empty reason. Neither is a code 0/default 1000, and abrupt TCP EOF is
`no_close_frame`. Invalid/malformed Close is a protocol failure, not a valid
received observation. Diagnostics may use semantic abnormal/no-status labels,
but 1005/1006/1015 are never placed on the wire or substituted as received codes.

A body of one byte is invalid. A body of two or more bytes has one network-order
16-bit status and a complete valid UTF-8 reason of at most 123 bytes. Reason
validation is independent of any unfinished Text state. Freeze this profile's
accepted wire set to 1000–1003, 1007–1014 and 3000–4999, based on the
[IANA registry](https://www.iana.org/assignments/websocket) snapshot updated
2026-10-05. Reject other codes, including reserved 1004/1005/1006/1015 and
unassigned 1016–2999, with 1002. Local server Close requests use that set EXCEPT
1010, whose missing-extension meaning is client-directed; receiving 1010 is valid.
Application/private
codes remain exact integers without inferred authentication, retry or provider
meaning. Future registry assignments need explicit profile review/versioning.

On a valid peer Close, validate and isolate the body BEFORE shutting down TCP.
If no Close has been sent, reply once with EXACTLY that valid body, including an
empty body, EXCEPT peer status 1010: preserve that peer observation, but reply with
an empty Close body (`88 00`) so the server does not transmit the client-directed
status. This explicit response policy never relabels the received status as absent
or normal. Exact-body echo for the remaining set is this profile's choice, not a
claim that the RFC requires identical response codes.
Discard any unfinished data without an application callback. Mark peer closure,
stop parsing new data, and close TCP once the Close reply has actually drained
from the local output queue. This local drain is not confirmed peer receipt.
Never rely on a callback sending after an earlier transport shutdown.

If Close was already sent, record the valid peer body without sending another
Close. Simultaneous endpoints can legitimately retain different peer statuses;
do not rewrite either to match. When both sides have sent/received Close, the
server initiates TCP teardown after its already queued Close has drained. Any
transport error/output failure or expiry of the original 1000ms Close deadline
force-closes on the IO loop and records incomplete handshake honestly. Further
traffic, Ping, repeated stop or partial peer Close cannot refresh the deadline.

## 4. Native allocation, callback and time budgets

The concrete reference profile uses these finite startup limits. Alternate
deployment values must pass checked configuration/admission and compatible
domain/endpoint review; no client can increase them. Limits are engineering
resource policy, not narrower common wire/accounting/reference types.

| Limit | Reference value |
| --- | ---: |
| Established WebSocket connections / incomplete HTTP upgrades | 32 / 16 |
| One data frame payload / one assembled data message | 1048576 / 1048576 bytes |
| Frames in one data message, including empty data fragments | 4096 |
| Retained application callback messages per connection | 2 |
| Actual callback-payload copies per connection / globally | 2097152 / 16777216 bytes |
| Raw receive-buffer CAPACITY per connection | 1048590 bytes (message bound plus 14-byte maximum client header) |
| Assembled data-payload CAPACITY per connection | 1048576 bytes |
| Pending application-output wire bytes per connection | 1048590 bytes |
| Separate control output reserve per connection | 254 bytes (two maximum 127-byte server control frames) |
| Control body / pending Pong | 125 bytes / 1 |
| Frames processed in one IO turn | 64 |
| Incomplete upgrade / incomplete frame absolute deadline | 5000 / 10000ms |
| Open fragmented-message absolute deadline / Close deadline | 30000 / 1000ms |

An established idle connection with no partial frame/message may remain open;
idle is not a leaked command lease and cannot block host stop. Absolute partial
frame/message deadlines start on their first byte/fragment and never reset on
trickle traffic or control insertion. Check expiry before accepting more bytes.
At a turn quota, schedule at most one bounded continuation per connection; retain
only within the same capacities. Callback backpressure refuses/closes 1009 rather
than grow pending message/command lists. Persistent valid multi-message service
must resume when leases are released; one-message-and-Close is not this profile.

Enforce capacities BEFORE native read/copy/growth, not only after constructing a
Drogon message. A read may consume only the remaining admitted raw capacity;
reserve/account any fixed read scratch, overread slack and allocator-capacity
rounding, and prove the actual selected implementation's maximum. Do not use an
unbounded readv fallback followed by a size check. Native assembly, callback,
host-command and output copies are separately charged while simultaneously live;
moving a lease transfers ownership, copying consumes another reservation. Drain
or reject a complete permitted frame so pipelined peers need no unbounded raw
backlog. Checked headers reject oversized advertised lengths without allocating
that body or overflowing `size_t`/length sums.

Before sending, reserve full encoded wire bytes INCLUDING each header and mask
policy; fragment deliberately within available output capacity or refuse. The
control reserve is unavailable to application output. Its two slots cover one
Close plus the one allowed pending Pong; exhaustion/transport failure follows
bounded teardown, never an unchecked send queue. No unbounded timer, callback,
message or close-observation history is retained. Exactly one close timer and
one partial-frame/message deadline state per connection suffice. Connection
teardown invalidates all scheduled callbacks and releases reservations once.

These capacities bound application-owned payloads and audited native buffers,
not total RSS, allocator metadata, framework/kernel socket memory, CPU scheduling
or attacker-wide network costs. Actual buffer strategy/capacity rounding, native
read scratch, TLS/Caddy/HTTP ingress and connection admission require separate
source/resource evidence. SESSION-011's H1 request limits do not automatically
apply to this separate Upgrade/native WS path. The later authenticated Upgrade
owner must supply its full pre-map/pre-log request budget and current authority;
this document does not turn unknown ingress behavior into support.

## 5. Independent literal wire examples

Hex rows are COMPLETE frames separated by ` / `; spaces are readability only.
Client frames have MASK=1 and literal four-byte fixture keys; server frames have
MASK=0. These independently derived vectors do not come from a proposed parser.
Unless stated, start open/idle with capacity and no prior Close. No Text echo is
implied: application callbacks and mandatory control output are separate oracles.

| ID | Client input or event | Required local observation / server wire output |
| --- | --- | --- |
| WS01 | `81 81 01 02 03 04 40 / 82 81 05 06 07 08 47` | Two callbacks: Text `41`, then Binary `42`; persistent connection remains open. |
| WS02 | `01 81 01 02 03 04 e3 / 89 81 05 06 07 08 75 / 80 82 09 0a 0b 0c 8b a6` | FIN=0 Text `e2`, Ping `70`, FIN=1 continuation `82 ac`; one Text callback `e2 82 ac` and Pong `8a 01 70`. No partial Text or Ping bytes in data. |
| WS03 | `01 81 01 02 03 04 40 / 88 82 05 06 07 08 06 ee` | Unfinished Text `41` discarded; valid peer 1000/empty reason; reply `88 02 03 e8`, then drain/teardown. No Text callback or `41` in Close. |
| WS04 | `88 80 01 02 03 04` | `close_without_status`; reply `88 00`. No received 0/1000/1005. |
| WS05 | `88 85 01 02 03 04 02 eb 61 7d 64` | Peer 1001/reason `bye`; exact reply `88 05 03 e9 62 79 65`. |
| WS06 | `88 81 01 02 03 04 02` | One-byte body invalid; local 1002 output `88 02 03 ea`, no valid peer status observation. |
| WS07 | `88 84 01 02 03 04 02 ea c3 ab` | Code 1000 followed by invalid UTF-8 `c0 af`; local 1007 `88 02 03 ef`, never raw-echo invalid bytes. |
| WS08 | `88 82 01 02 03 04 02 ef` versus `88 82 01 02 03 04 02 f6` | First decoded 1005 rejects with 1002; second decoded 1012 accepts/replies `88 02 03 f4`. |
| WS09 | `88 82 01 02 03 04 0e a3` | Decoded 4001 retained as raw valid application/private status; reply `88 02 0f a1`; no permission/semantic fact inferred. |
| WS10 | Server already sent `88 02 03 e8`; peer sends WS05 | Record peer 1001/`bye`, no second Close; complete local drain then TCP teardown. |
| WS11 | Server sent Close, peer has not; Ping `89 81 01 02 03 04 79` | Pong `8a 01 78` if transport/reserve permits, no data admission or deadline reset. Same Ping after valid peer Close produces no Pong. |
| WS12 | Unsolicited Pong `8a 80 01 02 03 04` | No reply and no data callback. |
| WS13 | Each independently: unmasked `81 01 41`; RSV1 `c1 80 01 02 03 04`; fragmented Ping `09 80 01 02 03 04`; orphan continuation `80 80 01 02 03 04` | Local 1002; no application callback. |
| WS14 | `01 81 01 02 03 04 40 / 82 80 05 06 07 08` | New Binary while Text unfinished rejects with 1002; no prefix callback. |
| WS15 | `81 82 01 02 03 04 c1 ad` | Text bytes `c0 af` reject with 1007; whole-message validation cannot insert replacements. |
| WS16 | `89 fe 00 00 01 02 03 04` or `81 fe 00 01 01 02 03 04 40` | First control uses extended length; second nonminimal data length; both reject with 1002 before data callback. |
| WS17 | Header advertises frame 1048577; reduced-test profile bound 4 advertises 5 | Resource refusal 1009 before advertised-body allocation. Exactly bound remains eligible subject to aggregate/callback budgets. |
| WS18 | TCP EOF without any Close, versus WS04 followed by EOF | First `no_close_frame`/incomplete handshake; second retains actual empty Close observation. Neither invents normal 1000. |
| WS19 | Peer never answers local Close; Ping arrives at 999ms | Original 1000ms force-close still applies; no extended handshake or confirmed peer receipt. |
| WS20 | `88 82 01 02 03 04 02 f0` | Valid peer 1010/empty reason remains recorded; reply `88 00` under the explicit server-direction exception, not received-status replacement. |

APITEST-012 must independently author/review tests for these oracles, valid and
invalid boundary families, actual native buffers and real peer/server/simultaneous
Close, then observe intended behavior red on a working selected foundation.
API-012 must preserve assertions, correct the admitted source profile, obtain
independent source/resource/ABI/build review and pass actual selected CI. Fake
frames alone, source reading, a blind Close echo or the old cooperative one-command
spike cannot close those gates. APITEST-001/API-001 then retain real lifecycle,
partial/idle/uncooperative peer, signal/startup failure, queue/drain and owner
teardown acceptance; API-003 retains full persistent authorized synchronization.
TLS/proxy/HTTP Upgrade security, containers, supported platforms, packages and
full release verification remain open owners' gates. All seven integrations and
all three FX providers remain required.
