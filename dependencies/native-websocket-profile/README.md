# API-012 implementation candidate

This directory is an **unadmitted source candidate**, not acceptance evidence.
Base/claim: `642c9a0e7e7a5f7e97b2f27da3a0508edb85ec83` /
`54df5b30149e35c26e27dc2b62a7ad428a37efa9`. The implementation checkout was
`t3/native-websocket-profile`, initially clean at the claim. The consumer header,
original 38-method baseline, fixtures, status, CMake, CI and stock SDK were unchanged
at that author handoff. Subsequent coordinator revisions add six independently
reviewed-or-pending listener/lifecycle methods (44 total) and explicit fixture wiring;
see the task record for actual gates. The original 38 method assertions stay intact.

Read `manifest.json` for ordered patch bytes and exact source identities. Apply
patch 1 at the Trantor repository root and patch 2 at the Drogon repository root.
The patches were authored in memory against read-only stock; no corrected vendor
checkout, configure, compiler invocation, install, load or native body was used.
`git apply --check` is a read-only applicability check, not a patched build.

The private arena, frame state, native source dispatch and lease accounting are in
`server/src/native_websocket_profile.cpp`. There is no tooling behavior change and
no additional dependency or feature switch. The stock facade is replaced; the
candidate requires its exact patched SDK, with no stock-header fallback.

## Storage and preallocation argument for review

All WebSocket payload storage is allocated from one private byte arena. Its only
operations are exact-size, byte-aligned extents and release. Extent metadata is
outside payload storage, fixed in count at startup, and never grows. The allocator
searches existing live extents for a disjoint interval before exposing that interval;
no pointer, logical length or string supplied by a consumer selects an allocation.
Fragmentation may refuse with resource_limit; it cannot grow the arena or borrow a
payload budget. This is a private profile strategy, not a generic allocator API.

For C=connections, U=incomplete_upgrades, L=callback_leases_per_connection,
R=min(raw_capacity_bytes,4096), M=message_bytes, O=application_output_bytes and
G=callback_bytes_global, startup checks and bounds:

- Owner records: C+U fixed registry entries; at most C established/retiring owners
  and U incomplete owners. One transient refused Owner can exist on each accepting
  IO owner before registration; it has no payload extents or timer on refusal.
- Extent metadata: C*(L+4)+U fixed entries. The four native extents are raw,
  assembly, application wire and the two-slot control reserve. Each live lease owns
  exactly one extent, including a zero-length logical message.
- Arena usable bytes: A=C*(R+M+O+254)+G. Mapping bytes are ceil(A/page)*page,
  checked before mmap. The unassignable page tail is less than one page. No extent
  can overlap that tail. At page=4096, reference A=84025728 and mapping=84029440;
  SMALL A=1404 and mapping=4096. These are source-derived allocation bounds, not
  measured RSS. The whole mapping remains reserved for the one framework lifetime.
- Each IO owner has at most one recurring timer record, one pending quota callback
  (plus its executing predecessor) and one detach cleanup record. A consumed callback
  releases its dependency only when its queue callable is destroyed. Timers removed
  from Trantor's ID set can still retain a record until their heap entry is destroyed;
  the owner slot remains charged through that interval.
- Live lease metadata is bounded by C*L. The fixed Profile/Context, vectors, shared
  pointer control blocks, std::function and native framework objects remain separately
  bounded metadata. They are not payload copies or total-RSS claims. Exact compiler,
  standard-library/allocator ABI and these object/control-block sizes remain an
  independent admission concern; no invented libc allocation bound is used for
  payload copies.

Raw extent capacity is R, allocated before native reads. recv is passed exactly
remaining raw space, with no readv scratch or fallback. Parsing drains header and
payload incrementally, so a full maximum-sized frame needs neither frame+header
read-ahead nor an unbounded pipeline. Masking is in place into the separate
assembly extent; the only fixed payload scratch is the 125-byte control body and
four-byte mask. One data assembly extent has capacity M from its first admitted
data header until release or transfer. Oversized advertised lengths, bad sequence,
nonminimal lengths and frame-count exhaustion refuse before that body's growth.

Raw includes retained Upgrade read-buffer capacity during handoff. Empty HTTP
storage is released before promotion. Nonempty read-ahead must fit *together* with
R before native promotion allocates/registers an established owner. During handoff
inspect/peak includes both source and destination capacities; the source vector is
released afterward. The reduced profile can refuse an Upgrade whose existing HTTP
read-ahead allocation cannot fit. Existing HTTP parsing, TLS ingress, accept queues,
logs and request/header storage are separate ingress work, not a newly claimed
SESSION-011 budget. The selected synchronous WebsocketControllerBinder handshake
and callback registration order are essential source-review inputs.

Assembly transfers its exact extent to the first lease without a payload copy.
The arena uses no per-message operator new, SSO, capacity rounding or terminator.
A clone's proven payload bound is exactly the original extent capacity M, even for
an empty logical message. Reserve that bound and a slot under the Context lock
before obtaining another extent or copying. Capacity stays exactly M, so no downward
adjustment is necessary. Both original and clone remain charged while live. A
message of 65 bytes therefore needs at least 65 more bytes for its clone; a pair
cannot fit a 128-byte payload budget. Empty messages still consume slots and M
bytes under this deliberately conservative private strategy.

A send owns one exact n+header extent, reserved before formatting/copy; it does not
pass a string or MemBufferNode copy to Trantor. Another send while that extent is
occupied refuses. The two control slots occupy 254 distinct bytes, allocated before
use and unavailable to application output. One pending Pong can be replaced before
its first byte is written; an already started Pong is not mutated. A Close is
immutable once latched. Output begins on the owner immediately when ordinary
Upgrade output has drained, then resumes on writable callbacks. A successful local
admission never proves peer receipt. Already started application output drains
before controls, and neither control consumes its capacity.

`fixed_native_storage` includes the inline masking/control/peer-Close storage even
for a still-owned terminal object; it is not fabricated as zero after socket close.
The SDK data-dispatch empty-string scratch is included during dispatch and in its
observed peak. Local Close is encoded directly into the reserved slot rather than
through a reason string/125-byte temporary. Snapshot/result DTOs and consumer-owned
call arguments are fixed bounded observations, not retained native payload copies.
Review MUST distinguish live extent capacities (snapshot fields) from the explicitly
bounded arena backing reservation, its unused extents/page tail and allocator/native
metadata above; counters alone cannot admit that distinction.

## Native dispatch, elapsed guards and retirement

The Trantor seam is private to TcpConnectionImpl. The Drogon Runtime and Session
operations are private to the selected framework owners and first-party factory.
They cannot be called by the fixture to issue a delivery or supply substitute
payloads. Runtime startup is latched at actual HttpAppFrameworkImpl::run entry.
The factory is installed once before startup; it owns immutable Limits/source and
cannot be reset by facade destruction.

The registry compares object pointer AND shared-ownership identity without calling
unknown virtual functions. Null, no matching binding, and wrong recorded IO thread
refuse in that order, before time or allocations. A terminal owner is inspectable
until genuine retirement; mutators refuse. Inspect and global inspect do not read
time, schedule work, consume bytes or repair state.

Only complete, validated Text/Binary establishes the private thread-local current
scope. Controls temporarily clear any outer data scope; nested data delivery
refuses. The source keeps its exact assembly/type/length until one successful claim
moves its extent; a second claim sees delivery_claimed even after that move. Scope
RAII clears current state on return/exception. The SDK invokes the application with
an empty raw string while bytes stay private; the consumer can observe payload only
through accept_callback. Ping callbacks carry metadata only; the genuine control
body is handled by the native owner. There is no fixture parser, private-hook setter
or fixture resource enforcement.

All native entry guards use the owned source's uint64 nanoseconds and per-owner
previous observation. Milliseconds-to-nanoseconds, start+duration, counts and byte
arithmetic are checked. Equality expires. Native read, write, timer, explicit recheck
and quota continuation all enter the same guard; there is no source replacement,
UTC fallback or deadline reset. Header/trickle/control traffic cannot extend an
existing frame/message/Close anchor. The timer is just a 5ms wake hint. Source
unavailability, regression and anchor overflow close the affected peer on its IO
owner. Factory performs no elapsed observation. Fake sources remain simulation.

At quota, the next buffered byte establishes a partial-frame anchor *before* the
single continuation is queued. A Ping observer queued synchronously in its actual
control callback precedes that continuation. Selected EventLoop::doRunInLoopFuncs
consumes the FIFO without another poll/timer dispatch between those two records;
the continuation rechecks the original anchor before parsing. This is the WS22
source causality argument, still requiring independent exact-source review.

Close state and first valid peer body are separate and immutable. Valid empty,
status/reason, peer 1010, invalid status/UTF-8, local Close and protocol-error Close
follow the frozen distinctions. The first anchor is never refreshed. After local
Close, application sends refuse while TCP can still be live; after peer Close no
new Pong is admitted. Both-sides Close or protocol failure tears down after admitted
wire drains; an uncooperative peer is forcibly closed at the original deadline.

Trantor retirement removes the poller entry before closing/releasing the fd, clears
its native buffers, invokes closed before the public Close callback, and detaches
after that callback. Later read/write/error callbacks in the same channel event
are inert, and connectDestroyed does not remove the entry twice. A retained public
WebSocket object thus cannot keep that fd or growable native buffer alive. This is
a material vendor lifecycle change requiring focused independent review.

The profile's timer and queued records retain Context/Owner and count as work.
closed releases native extents and invalidates the timer; detached queues one cleanup
record. A slot retires only at terminal+detached+zero leases+zero work. Off-owner
lease copy/release uses the same lock as global/per-connection reservation; reset
releases payload and counters in one critical section. Empty retained leases keep
the slot occupied. No tombstone/history exists. Release does not queue resumption:
the parser never retains a consumer-wait list, and capacity exhaustion closes 1009;
otherwise later input continues normally with newly freed lease capacity.

## Coordinator integration recipe — deferred, not executed

1. Obtain independent Claude source/resource/protocol and build/ABI/license admission
   of this exact commit, patch order/hashes and private arena strategy. Specifically
   review arena backing versus live-capacity accounting, pre-promotion read-ahead,
   Socket/Channel early retirement/reentrancy, timer-record destruction/FIFO causality,
   caller check order and private source attribution. No source approval is claimed
   by this author packet. Resolve concerns before releasing preparation/build.
2. Only after admission create distinct ignored paths under primary build/deps:
   `drogon-1.9.14-api012-candidate1-source`, `...-build`, `...-sdk`. Preserve pristine
   `drogon-1.9.14-source`, its Trantor submodule and `drogon-1.9.14` byte-for-byte.
   Clone the pinned local repositories without hard links/alternates, detach at the
   manifest's commits, initialize the Trantor path at its exact pin, then apply
   patch 1 there with `git apply --check` followed by `git apply`; apply patch 2 at
   Drogon root likewise. Do not label either corrected tree pristine or rerun the
   existing pristine setup script on it. Capture both before/after file hashes,
   git status/diff and all ordered patch digests.
3. Configure only the corrected source/build/sdk with Ninja and the unchanged
   16 options in tools/setup_drogon.py OPTIONS. Pass each `-Dkey=value` literally,
   plus the distinct `CMAKE_INSTALL_PREFIX`. Retain Release/shared, Drogon C++20,
   effective Trantor C++14 and OpenSSL. Keep compile commands/cache/link logs and
   actual compiler/libstdc++/glibc/OS identity. No upgrade or feature toggle is needed.
   Existing configure probes that execute native stdlib code remain an admission gate.
4. After that stage's release, `cmake --build <corrected-build> --parallel 4`, then
   `cmake --install <corrected-build>`. Record true outputs rather than predicting
   ABI or build success. Preserve Drogon LICENSE, Trantor License, orm_lib/COPYING
   even with BUILD_ORM=OFF, applicable bundled notices and all actual direct/transitive
   notices. Preserve GPL-3.0-or-later for the first-party code and these modifications;
   copy project LICENSE and the exact patch set/manifest as correction provenance.
   A full notice/legal inventory belongs to coordinator admission, not these examples.
5. Create corrected dependency-build.json from this manifest's profile, upstreams,
   ordered_patches and build_options plus actual build/artifact identities. Never copy
   the three-field pristine manifest to label this SDK. Coordinator updates
   cmake/DrogonDependency.cmake to require this corrected identity, full patch hashes,
   distinct SDK path, correct imported target/header/library locations and notices.
   No new first-party source is needed in server/CMakeLists.txt: its existing static
   target already owns native_websocket_profile.cpp and links Drogon::Drogon.
6. Independently inspect actual compiled SDK/probe direct/transitive ELF, SONAME,
   symbols/vtable/RTTI, RPATH/version requirements, licenses and controlled loader
   map. Pure-virtual TCP additions are intentionally ABI-incompatible with stock;
   never mix patched and stock headers/DSOs. Admit the actual artifact before load
   and native feature bodies. The current consumer's admitted_corrected result
   identifies this compiled implementation, not a runtime configuration switch.
7. Run the full current 44-method native baseline only after those gates. Preserve
   original stock red/reviews; do not replay stock. Then independent code/native
   acceptance review, relevant regression/current CI, ordinary selected-suite CI
   promotion, final source/license packet and protected merge remain coordinator work.
   Do not erase intentional-red classification before actual corrected green.

This profile refuses direct native TLS sockets; TLS/HTTP ingress requires its own
admission and remains in release scope. No whole host/readiness, authentication,
synchronization, SESSION-011 Upgrade admission or session-clock binding is supplied.
No preparation script was added: new automation behavior would need its own focused
test-first path. The recipe is declarative so the coordinator can perform or separately
author that preparation after the required reviews.

## Author's verification and remaining gates

Read-only patch applicability checks passed at both exact upstream pins, and both
stock git worktrees remained clean afterward. `clang-format --dry-run --Werror`
and `git diff --check` passed. Stock ELF digests remained Drogon
`a478e5a1e933f3e7b44feec5e7492e26da2d12231c566ae3d6a8684499046fcd` and Trantor
`71329f317d50b73c24d043bf0755a742eaaf24f9536afd0eef5affb06a545d33`, matching the
accepted baseline receipts. Frozen test/header equality was checked against the
claim. No test, compiler, SDK configure/build/install, shared-library load, listener,
feature body, stock replay, PR, push or merge was performed. Syntax/link/native
results are deliberately unknown until their review stages release those checks.

Legacy public native send/shutdown calls are routed into the IO-owned guards with
no off-owner copied-payload queue. The native Session handle itself is exchanged/
observed atomically, so off-owner lookup cannot race detach. Those void legacy
methods cannot return the consumer boundary's typed refusals. Automatic Ping
configuration and SDK sendJson formatting are refused before retaining/formatting
payload in this profile: callers must prepare a separately budgeted envelope and
use the frozen Profile API. This adds no JSON/auth/sync capability and does not
remove those release requirements. These restrictions need explicit source/API
review along with the admitted consumer routing.
