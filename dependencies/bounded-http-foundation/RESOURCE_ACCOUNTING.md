<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Source allocation inventory (API-016)

This authored inventory is part of the tier A review packet, not a review receipt.
It describes the selected Linux x86-64 Release/shared/C++20/OpenSSL source. The
sealed compiler, standard library and complete DSO/consumer closure identify the
build to which its compile-time guards apply. Allocator overhead, thread stacks,
kernel resources, TLS and whole-process RSS are outside these request bounds.

## Physical storage and reservations

`project_storage` preserves the reviewed evaluation order and all literal plans.
One anonymous mapping supplies P*102424 + I*B + O*(E+128), rounded to 4096. Each
peer's payload offsets are receive/prefix 0/65544, arena 65544/32768, method
98312/16, target 98328/2048, records 100376/1024 and control 101400/1024.
No response-body or whole-head copy is added. Input/output payloads follow those
P peer slices. The mapping's page tail is never lent as capacity.

| Reservation | Source-owned allocation requests and guards |
| --- | --- |
| Peer metadata: 16384 | Physical typed backing is **10240**, comprising a 2048 first-party slice and 8192 SDK slice. Native-only unavoidable standard-object metadata is bounded by **5120** separately, and the fixed poller record by **128**: subtotal **15488**. Another **320** covers five rounded 64-byte callback closures (including the native weak dispatch), yielding **15808 <=16384**. Remaining allowance is unused; it is not a fallback heap. The entire 16384 reservation remains charged through retained handles and both SDK control-block fences. |
| First-party slice: 2048 | `PeerRecord`, four aligned 128-byte handle States, counters and pending/executing mailbox storage. `sizeof(PeerRecord)<=2048` is compiled before allocation. State allocators accept only their issued descriptor/size, never general allocation. |
| SDK typed slice: 8192 | Before construction, `Slot::require` rounds to 64 and checks the remaining slice. Compile-time caps: Slot 2048 (including two 512-byte control-block stores), TCP 2048, Channel 256, Socket 128, parser 2048, WebSocket 512: sum **7040 <=8192**. The actual shared control-block allocator additionally compiles `sizeof(T)<=512`, `alignof(T)<=64`. No placement/general-heap fallback. |
| Native standard metadata: 5120 | The parser compiles the sum of actual selected libstdc++ request/response/binder shared-control object sizes, eight <=128-byte map nodes, <=64 bucket pointers and two 64-byte IO-cache/render-callback allocations. Before allocation, install queries the selected prime policy for at most 32 buckets per map. The selected libstdc++ unordered-map policy receives `reserve(5)` for request and `reserve(3)` for response before insertion; no second insertion phase exists. The selected node type also has a compiled <=128-byte guard. The eight nodes plus bucket allowance fit the contract's 2048 map-metadata allowance. Character storage is accounted separately below. |
| Native characters: 8192 | Only the literal path and five sanitized fields are copied. The contract's per-empty-string bound `round_up_pow2(max(32,n+1))` covers path 256, Host name/value 32/512 and four name/value pairs 256: 1056. Remaining 7136 covers simultaneous host/key copies, appended 60-byte key/GUID, SHA/base64 result, three response names/values and the bounded response content-type string. No Cookie/auth/query field is projected. This uses the selected libstdc++ once-assigned string growth policy, not a post-allocation capacity measurement. |
| Pool metadata | One 128-byte physical slot per I/O entry; `PoolSlot` size is checked. Acquisition precedes read/render; an unqueued reply reservation is returned directly on cancellation. A committed wire slot is returned only by its actual SDK node release. |

The typed metadata allocation uses P*10240+(I+O)*128, not P*16384 followed by
additional unaccounted native allocations. Other metadata requests above consume
the remainder of the same per-peer reservation. `Observation` reports reservation
charges; it does not grant capacity or substitute for these construction guards.

## Shared global G and fixed pollers

R4 correction 9 partitions the SAME **65536** global allowance into **32768 SDK +
32768 first-party** bytes. These are constants, not additional arrays or writable
first-party quota facts. There is no extra mapped G. Before `initFirst_` constructs
the existing main framework/loop, its compile-time inventory checks rounded
framework, listener/plugin/DB/Redis managers, controller/static routers, advice,
two HttpServers, two <=4096 loop/bootstrap groups, 24*128 infrastructure records,
two <=1024 acceptors, 2048 thread-pool/future bookkeeping and 4096 bounded startup
strings/vector/callback bookkeeping against the SDK share.

The loop group includes the EventLoop (<=2048), EpollPoller (<=512), TimerQueue
(<=256), two Channels (<=256 each), their one shared control block, two MPSC
sentinels and two small member-function callback allocations. Empty timer
containers do not allocate payload. Bounded lifecycle/scan/mailbox operations do
not enqueue per-request MPSC closures. Native timer/lease/queue costs remain under
the imported native profile; caller fixture barriers/serialization are separate.

First-party Core (including the inline physical backing for its two
eventfd/timerfd Channels), Foundation/State allocations and
the native registration's <=256-byte global allowance have their own compiled
rounded sum <=32768. The native build copy checks registration object/control
storage before installation. Its retained original native Context/pools remain
separately charged under API-012, not erased from either source inventory.

The private SDK bootstrap binding fixes the main loop at **16 records before its
constructor**. `Runtime::install` reserves IO0's **P+8** event, active-pointer and
(debug builds) FD/channel arrays before returning; the future IO0 constructor
moves those arrays and allocates no replacements. A record allows a <=16-byte
kernel event, 8-byte active pointer and fixed debug pair, with per-array rounding
covered by its 128-byte charge. Main 16 + IO infrastructure 8 are in G; P peer
records are in peer metadata. Epoll never doubles these selected arrays on a full
return. Every bounded active batch fits the preallocated pointer capacity. Debug
registries use fixed cells rather than map insertion; legacy unselected IO loops
retain their existing dynamic behavior.

`install_failure` (correction 10) distinguishes actual pre-run array allocation
failure from invalid configuration/state; first-party create returns the matching
existing Error. No query can reset installation or manufacture a permit.

## Lifetime and exclusions

Acceptor T is stack RAII and claimed before accept; a shared role/peer quota is
checked before socket/channel/name/parser construction. Slot removal waits past
the event batch. Both real shared objects have separate deferred-destruction and
control-return records; weak references pin the peer too. Native retirement is a
separate actual Context fence. IO0 consumes all of them before removed/drain.

Wire spans borrow exactly one reserved application/control owner. Every bounded
stock send entry refuses before copying; the ordinary pipeline/map/vector path is
not instantiated. Native 101 drains its fixed control span before native output.
`before_write` (correction 11) guards each actual borrowed-node send, including
writable-channel callbacks. `discard` (correction 12) clears only the actual cold
or joined installation on its creating thread; the one-attempt latch remains.
Normal HTTP uses no TimingWheel. Fixed timerfd/eventfd scans do no per-turn
registration/allocation. No replay, credentials, provider traffic or secure-account
capture capability is supplied by this generic foundation.

All 34/44 loopback methods and selected compile/layout guards must be read together
with source review. Passing counters alone cannot prove this inventory. Tier A
review must check its stdlib assumptions, every simultaneous temporary and the
pre-main/static initialization order on the sealed selected toolchain.

## Fix round 1 witness and lifetime accounting

The two wake Channels are placement-constructed on IO0 in aligned storage already
allocated with Core before run; their destructors run only after finish's cleanup
fence. No first-turn Channel heap allocation remains. The existing conservative G
compile guard still also allows two rounded Channel sizes, so this change cannot
increase the admitted global bound. SDK Facts and its atomic append counter and
completion publications fit the existing startup-bookkeeping allowance; they add
no heap or payload queue. Physical peer/control/request pool sizes are unchanged.

Runtime::observe (correction 15) returns actual map-carrier/batch/stock-output
operations, actual completed TCP constructors/retained SDK records through their
final fence, and actual projected headers/cookies map sizes. The private Trantor
counter is incremented at the stock list-insert/append sites; selected ordinary
paths are forbidden to reach them. ReadSample.iovcnt comes from readv's actual
argument, and read_min_writable/peaks are updated from actual guards/acquisitions.
These are event witnesses, not structural constants. BoundedIoInternal.h remains
SDK-to-SDK source plumbing, is not installed, and is not included by public SDK
headers or first-party sources.

Socket removal revokes Transport before abandoned node returns. Such returns only
release ownership; they cannot manufacture drained success. Reserved controls
without nodes are reclaimed at the same socket-removal service turn. Receive
teardown wipes once per peer incarnation before reuse; owner_teardown is emitted
and pending successor storage is wiped even after a request was delivered.
Framework-run completion and every SDK-owned IO join are acquire/release-published;
discard and finish require that evidence. Caller-owned framework-thread join is
a caller precondition, not a report inferred from !isRunning or server absence.
