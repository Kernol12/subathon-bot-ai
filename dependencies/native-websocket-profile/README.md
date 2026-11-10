# API-012 source correction candidate 4

**Unadmitted source candidate. No corrected SDK preparation, compilation, load or
native acceptance is claimed or released by this handoff.** Candidate 1 was
b3f04db0a10c1ced77bdb0488566e1155dd8da71 (author 1915bae4009d1fff79b14a8bd41589079305b61e).
This correction session started clean at fdb073ed3072ae06e05045fff669e4eae30a1ed9,
branch `t3/api-012-native-profile-fixes`, checkout
`/home/archnoob/.t3/worktrees/subathon-bot-ai/t3-0014ea98`.
The coordinator owns status, contracts, build/CI, SDK admission and protected merge.
The candidate2 author changed only the cpp, two patches, manifest and this README.
Coordinator candidate3 corrected source-review B5/B6/L1 on the same owned paths;
candidate4 additionally fixes B7 by completing any started native frame before
selecting another frame. No extra state, storage or queue is introduced.
source/resource re-review still precedes corrected SDK preparation.
Public first-party header and all tests are frozen. The baseline is the independently
reviewed 44 methods (38 original assertions preserved, six additions at d9ac991).
Original stock WS02 red and supplemental missing-profile red remain distinct;
neither supplies corrected native capacity or deadline evidence. No stock replay.

`manifest.json` binds the first-party source, exact ordered patch hashes and profile
identity. Patch 1 applies at the pinned Trantor root; patch 2 at the pinned Drogon
root. Patches were generated as in-memory strings from read-only stock. No corrected
vendor tree or build/install prefix was prepared. No upgrade, dependency or feature
switch is introduced. The selected Release/shared recipe remains Drogon C++20,
Trantor C++14 and OpenSSL with the same 16 options.

## Nonfragmenting storage proof (B2)

Let C=connections, U=incomplete_upgrades, L=callback_leases_per_connection,
R=min(raw_capacity_bytes,4096), M=message_bytes, O=application_output_bytes,
G=callback_bytes_global and Q=floor(G/M). Startup rejects zero/incoherent limits
and checks all sums, products, vector sizes, offsets and page rounding before use.
The mapping consists of:

- C disjoint owner regions, each `[R raw][254 control][O application output]`.
  The regions have fixed index-derived offsets. A promoted owner exclusively holds
  its region index through actual terminal+detach+zero leases+zero queued work.
  Releasing individual buffers never releases that owner region to another peer.
- C+Q disjoint M-byte slots, shared ONLY by assembly and callback leases. Offsets
  and sizes are fixed at startup. Allocation scans a fixed live-bit table for a
  free slot; it never searches variable-sized holes or moves a live payload.
- Metadata: C+U fixed owner registry entries, C region flags and 3C+(C+Q) fixed
  block records. These vectors never grow after startup. Incomplete owners have
  separate U accounting and no payload region/message slot before promotion.

At most C owners can each hold one assembly slot. Every live lease (including an
empty logical message and a clone) reserves exactly M global/per-owner bytes and
one lease handle, so there can be at most Q live lease slots. A new assembly has
at most C-1 other assemblies; a successfully reserved clone has at most Q leases
including itself. Therefore a free M slot exists for either operation whenever
its declared counters admit it. Claiming a current assembly transfers that very
slot into a lease without copying. Reservation and slot acquisition/release share
one mutex. Native regions cannot contend with message slots or another owner's
regions. No allocation history, small output hole, or other peer can cause a
contiguity refusal. Genuine system allocation failure of metadata is separately
allocation_failure; counters/snapshots alone do not establish allocator behavior.

Clone's proven payload bound is B=A=M, reserved BEFORE metadata allocation, slot
acquisition or copying. No libc payload allocation, terminator, SSO, geometric
rounding or second native payload copy occurs. Both original and clone stay
charged simultaneously. Move transfers ownership; reset frees the slot and
counters once under the same lock. Handle count is additionally bounded by C*L,
checked at startup. At reference, Q=16, so conservative M-byte charges allow only
16 globally retained leases even though C*L=64. This consequence is intentional
and explicit; there is no payload-length-based overcommit.

Usable backing A=C*(R+254+O)+(C+Q)*M. Mapping reservation is
ceil(A/page)*page, with an unassignable tail less than one page. If G is not a
multiple of M, its remainder cannot fund another lease and is not mapped as a
message slot. At page=4096:

| Profile | Native region bytes | M slots | Usable A | Mapping | Page tail |
| --- | ---: | ---: | ---: | ---: | ---: |
| Reference | 33,694,080 | 48 x 1,048,576 | 84,025,728 | 84,029,440 | 3,712 |
| SMALL | 1,020 | 6 x 64 | 1,404 | 4,096 | 2,692 |

The ENTIRE backing reservation, including unused slots/regions and page tail,
lasts for the single framework lifetime. Touched pages need not be returned;
this is not RSS or allocator-resident-byte proof. Snapshots report LIVE assigned
buffer capacities, not that whole reservation. An active output buffer charges
its full O region capacity even when encoded wire size is only n+header; only
n+header bytes are formatted/sent. Raw/controls charge R/254 when active, assembly
and each lease charge M. Unused backing and page tail are accounted here explicitly,
not hidden inside `fixed_native_storage` or fabricated as observed live buffers.

Raw is acquired before recv, which receives only into its remaining space, with
no readv scratch/fallback. Parsing drains headers/payload incrementally. Advertised
length, sequence, aggregate M, frame count and minimal length checks precede body
storage/copy. Controls use an inline 125-byte body and a separate two-slot 254-byte
output region. Whole-message Text UTF-8 and Close-reason UTF-8 remain independent.
Streaming Text scalar state additionally preserves validation after payload discard.
Fixed scratch reports the inline mask, 125-byte parsed control body, separate
125-byte latest-Ping payload, first-Close and three UTF-8 words;
the SDK empty dispatch string is included during dispatch and in its peak.

Upgrade read-ahead remains a separate stock HTTP allocation until owner-thread
handoff. Empty storage is released before promotion. Nonempty existing vector
CAPACITY plus R must fit raw_capacity_bytes BEFORE promotion; snapshots/peaks count
both while simultaneously retained, then release the source vector. A reduced
profile may refuse read-ahead that cannot fit. This is not full pre-parser HTTP,
request/header/log, TLS, accept-queue or kernel-memory admission. Those ingress
budgets remain downstream, not inferred from SESSION-011's H1 profile.

Profile/Context objects, shared_ptr control blocks, fixed vectors, lease State,
std::function and framework objects are separately bounded metadata, not payload
copies or a total-RSS guarantee. At most C+U registered owners plus one transient
refused owner per accepting IO loop exist. Lease metadata is bounded by C*L.
Per owner there is one timer callable, one pending continuation (plus an executing
predecessor), and one detach-cleanup callable. Each record counts as work until
its last queue/timer capture is destroyed, including invalidated timer heap entries.
Exact compiler/stdlib/allocator object sizes remain independent admission work.

## Listener, idle and thread boundary (B1/B4/M2/H1)

The selected private SDK ABI exports `DROGON_SUBATHON_NATIVE_PROFILE_V1` from
HttpAppFramework.h. `addListener` has a final `bool nativeProfile=false` argument.
HttpAppFrameworkImpl rejects marked+TLS OR marked+missing installed profile by
throwing invalid_argument BEFORE ListenerInfo is stored, HttpServer is constructed,
or any bind/listen socket is created. ListenerInfo carries the explicit flag to
every actual HttpServer (both ListenerManager construction paths). Only its marked
onConnection invokes the native factory and attaches a Session. There is no
path/address/peer/order heuristic or global-policy-based listener marking.

Ordinary listeners retain stock HTTP/TLS/idle/connection behavior and consume zero
native slots/timers. While any profile is installed, onRequests refuses EVERY
ordinary-listener WebSocket Upgrade before advices/WebSocket controller dispatch;
there is no stock parser fallback. A completed ordinary HTTP request on the marked
listener stops its parser and sends one fixed HTTP 400 literal with Content-Length: 0
and Connection: close before ordinary endpoint dispatch. Stock transport drains
that bounded pre-Upgrade response before shutdown; no request-derived body is allocated. The ordinary Upgrade guard covers pipeline batches too. A marked
listener is dedicated internal ingress within the SAME Drogon process; API-001
must later configure both listeners and Caddy routing without reducing HTTP,
webhook, WebSocket, TLS or deployment scope.

Marked connections consume U incomplete owners with original upgrade deadline and
native socket retirement. Marked HttpServer instances do not enable stock HTTP
idle kickoff: incomplete owners use the native Upgrade deadline from acceptance.
At explicit successful native establishment, the Trantor seam also resets any kickoff
entry's connection BEFORE dropping its weak reference and zeros idleTimeout and
its backup. The wheel may retain an inert entry until expiry. Native reads/writes
need no stock extendLife: established idle is permitted indefinitely, and all
native partial/message/Close deadlines remain authoritative. This is permanent,
unlike sendAsyncStream(disableKickoff)'s temporary backup/restoration. The selected
startEncryption path is TLS handshake machinery, not a safe native idle policy;
marked TLS is refused before listening. Ordinary listener kickoff is unchanged.

beginUpgrade/bind/initial check BOTH recorded IO thread identity and the actual
EventLoop owner before native buffer operations or time guards. The SDK checks
owner before acquiring/releasing any handoff buffer or processing a native 101;
asynchronous filters/advices returning off-owner are force-closed through the
stock thread-safe forceClose path, without copying input or enqueuing handoff work.
Ordinary HTTP asynchronous response handling is unchanged. A native beginUpgrade
latch suppresses further stock reads during handshake processing; onMessage stops
after the parsed native request so WS read-ahead stays intact until initial().
On-owner synchronous bind, 101, callback registration and initial handoff cannot
interleave another IO read. Original absolute Upgrade deadline remains while pending.

MsgBuffer release swaps out the vector and sets size/capacity/head/tail/initCap to
the empty state (M3). retrieveAll (and retrieve through it) preserves zero indices
for an empty vector instead of restoring offset 8. Empty begin/peek uses one shared
nonpayload char sentinel; nonempty begin uses vector.data(). Positive subsequent
growth reconstructs an ordinary MsgBuffer via its existing checked-owner paths;
size >= tail >= head and writableBytes=size-tail cannot underflow after release.
No per-peer replacement allocation is retained after release. That sentinel is
one fixed SDK byte, not a claimed payload allocation or new receive scratch.

## Close, dispatch and retirement (B3/M1)

Local Close discards the assembly SLOT and payload length but preserves opcode,
active frame, aggregate message byte/frame counts, streaming UTF-8 state and the
original partial-frame/message anchors. Legal continuation/control frames are
validated/discarded without delivery; Ping can still obtain Pong. FIN still checks
complete scalar UTF-8. Close/body/header/frame/message limits apply while closing.
The FIRST valid peer Close body is retained without a second local Close or any
refresh of the original absolute Close deadline. Invalid sequence/status/UTF-8
still causes bounded failure. Immediate/buffered output and peer1010 distinctions
remain intact, as does the immutable first-Close observation.

No native fail/force or consumer/close callback is invoked under Context::mutex.
Factory time sampling/guard/anchor and timer arming run outside its registry
critical section. Bind's registry/region updates end before native establishment
or catch teardown. accept_callback unlocks before resource/allocation-error fail.
Buffer/lease/counter updates and retirement remain synchronized; those operations
invoke no application callback or native force. An allocation failure while arming
an unattached owner removes its registration; it does not publish a partial Session.
The small TimerQueue addTimerInLoop correction rolls back its ID-set insertion
if heap insertion throws, so failed native timer arming cannot accumulate orphan
IDs outside the owner/queued-record bounds. No timer API or ordinary successful
timer semantics change; its exact exception path also needs source review.

The source-bound delivery scope remains private: complete validated Text/Binary
holds its exact assembly/type/length; one accept_callback reserves and moves it.
Second claim is delivery_claimed, expired scope is outside_delivery, control scopes
clear outer delivery, nested data refuses. SDK dispatch uses an empty mutable string,
not a payload surrogate. Null -> not_owned (pointer AND ownership identity, no
unknown virtual call) -> wrong_owner -> closed ordering is unchanged. Registry
matching uses the bound pointer and weak ownership identity without creating a
strong temporary whose last destruction could run SDK shutdown under the lock. Inspection
allows terminal-owned state, samples no time and performs no repair/scheduling.

Trantor disableAll -> Channel remove -> fd close ordering, same-event inertness,
closed before public close callback and detached afterward remain unchanged.
The owner/region slot retires only after terminal+detached+zero leases+zero work;
TCP close or invalidating a timer alone cannot free it. No tombstones or history.
Off-owner lease release/copy shares the budget mutex and cannot revive a socket.
The private Session handle is atomically observed/exchanged across detach.

All elapsed guards use immutable owned uint64 monotonic observations. Arithmetic,
regression, unavailability and equality expiry retain bounded teardown semantics.
Read/write/timer/API/quota continuations recheck original anchors. The 5ms recurring
timer is only a wake hint. FIFO quota causal ordering remains source-derived and
requires native confirmation; no UTC or actual host-clock binding is supplied.

## Coordinator wiring and remaining gates

1. Independent Claude exact source/resource/protocol/build/license review of this
   revision and manifest must RELEASE preparation first. Review all B1-B4/M1-M3,
   H1, fixed-slot backing/live accounting, marked-without-profile/TLS registration
   refusal, handoff ownership, and unchanged socket/queued-record retirement.
2. Only then create DISTINCT ignored primary paths, for example
   `build/deps/drogon-1.9.14-api012-candidate4-{source,build,sdk}`. Preserve stock
   `drogon-1.9.14-source`, its Trantor submodule and `drogon-1.9.14` unchanged.
   Clone pinned local sources without hardlinks/alternates; initialize exact
   submodule pin, capture provenance, apply patch1 at Trantor then patch2 at Drogon.
   Never run pristine setup automation on a corrected tree or call it pristine.
3. Configure/build/install only after the corresponding releases, using Ninja,
   manifest's unchanged 16 options and the distinct CMAKE_INSTALL_PREFIX. Capture
   actual compiler, C++ standards, libc/stdlib/OS/Qt/ABI, cache, logs and outputs.
   No compiler probes/build/install/load were performed by this author session.
4. Preserve Drogon LICENSE, Trantor License and orm_lib/COPYING even with
   BUILD_ORM=OFF (orm core still compiles), bundled crypto headers, actual direct/
   transitive notices and project GPL-3.0-or-later LICENSE/correction provenance.
   Full license inventory remains coordinator admission.
5. Coordinator updates cmake/DrogonDependency.cmake and CI to admit this exact
   profile/source/ordered hashes, distinct prefix and actual artifact identities
   in dependency-build.json. Never reuse the pristine three-field manifest.
   The existing server target already owns this cpp and links Drogon::Drogon;
   no first-party header or shared CMake edit is needed from this worker. The
   frozen fixture already selects the final true listener argument under the
   header marker. Keep that selection and the unmodified stock path.
6. Independently admit actual ELF NEEDED, resolved direct/transitive libraries,
   SONAME, vtable/symbol/RTTI (including cross-DSO Session casts), RPATH/version
   requirements and loader map BEFORE load/identity/passive/native execution.
   Stock and corrected SDK headers/DSOs MUST NOT mix across profile identities: TcpConnection's
   virtual ABI, Session, HttpAppFramework::addListener and private layouts differ.
7. Only after these gates run actual current44, then independent code/native
   review, regressions/current CI, ordinary CI promotion and protected merge.
   Preserve red evidence; no new stock run or assertion changes. Stop after API-012.

Remaining limitations: one pending application frame and no writable notification
need downstream backpressure design; M-byte lease charging is conservative. Dead
half-open idle peers can hold slots until host liveness/stop policy acts. Periodic
timers and parsing/discard after peer Close remain bounded, with no performance
claim. A partially sent Pong is immutable. Exactly one additional 125-byte fixed
payload buffer retains the latest pending Ping, replacing older unsent pending
responses. After the started Pong drains, that payload is encoded into the SAME
127-byte Pong slot. A started application/Pong/Close frame always finishes first;
unstarted frames then use application, Pong, Close order. Thus a Pong received
after Close starts follows that complete Close, never splits it. Empty Ping is
represented distinctly;
no new wire slot, history, timer or deadline is introduced. The control wire reserve
stays 254 bytes, and the extra payload capacity is charged in fixed native storage.
Legacy Ping/Pong sends are suppressed so they cannot replace an owed response.
Legacy void send/shutdown off-owner refuse without a copied payload queue;
sendJson and automatic Ping configuration are suppressed. Use the
frozen Profile API for typed refusals/owned envelopes. These restrictions add no
JSON/auth/sync/host capability. Full HTTP ingress, native TLS, Caddy, platform,
API-001 ready host/clock/auth and API-003 sync remain separate unimplemented gates.
All integrations/providers and the complete release scope remain required.

## Author static checks

Both ordered patches passed `git apply --check` against the exact pinned pristine
repositories. `clang-format --dry-run --Werror` for the first-party cpp and
`git diff --check` passed. Manifest hashes, unchanged 16-option recipe, and frozen
header/test bytes were checked statically. Stock source repositories remain clean;
installed stock Drogon/Trantor DSO hashes match the earlier admitted receipts.
These checks establish source identity/applicability only. No configure, compiler,
SDK preparation/build/install, load, listener, test body, stock replay, push, PR or
merge ran in this session. Syntax/link/ABI/native results remain unknown pending
the independent review releases and coordinator's subsequent stages.
