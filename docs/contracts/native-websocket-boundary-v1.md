# Native WebSocket consumer boundary — v1

APITEST-012/API-012; internal consumer boundary for the reviewed
[native profile](native-websocket-profile-v1.md) and
[host lifecycle](api-host-lifecycle-v1.md). Exact proposal revision4 SHA256
`4f9f587ee396b49ce0e99ad228680ac13d06aef1b221ca442c12ddc5f09b77ac`
received independent boundary review before header/stub/test authoring. This
specifies required behavior; the stock facade remains explicitly unimplemented.
It supplies no native profile, actual host/readiness or clock-binding capability.

## Scope and declarations

The [consumer header](../../server/include/subathon/server/native_websocket_profile.hpp)
and [implementation](../../server/src/native_websocket_profile.cpp) have no Qt SQL,
domain or session authority. Drogon owns framing/connections/IO; the selected
corrected source must enforce its native obligations. One pimpl facade binds
startup policy, callback ownership, local commands and bounded diagnostics.
No vendor patch, new framework, general allocator or process subsystem is chosen
by this boundary. Exact private SDK ABI remains a separate source/admission gate.

### Explicit listener admission

Policy installation is process-wide; connection admission is listener-specific.
The admitted SDK must expose an explicit startup listener marker, defaulting to
ordinary HTTP behavior. Only a listener registered with that marker uses this
profile's incomplete-upgrade owner slots, Upgrade deadline, direct-native-TLS
refusal and native socket retirement. No path, peer address, registration order
or process-global installed-policy test implicitly marks an ordinary listener.
Registration of a marked listener requires the installed profile before startup;
ordinary listeners retain their own HTTP/TLS/timeout policy unchanged.

The selected SDK refinement adds a final `bool nativeProfile = false` argument to
`HttpAppFramework::addListener`, carried through `ListenerInfo` to its actual
`HttpServer` instances, and exports the header feature marker
`DROGON_SUBATHON_NATIVE_PROFILE_V1`. This is part of the exact corrected SDK ABI,
not a provider capability or general ingress extension. Marked native TLS is
refused before listening. On the marked listener, a completed ordinary HTTP
request is refused with a bounded response rather than entering ordinary endpoint
dispatch. Pre-parser request/header/log byte admission remains the later ingress
owner's obligation; this does not claim full H1 safety.

The fixture's corrected build explicitly marks its loopback listener; its stock
build retains the original public call. This necessary listener-selection wiring
and additive isolation tests need independent review before use. Existing wire,
resource and deadline assertions remain unchanged, and original stock red remains
preserved. API-001 must later configure the ordinary HTTP and dedicated native
listeners in the same process; Caddy routing, authenticated Upgrade, host readiness
and deployment verification remain separate gates. This internal separation does
not reduce the specified HTTP, webhook, WebSocket or TLS release scope.

The stock factory returns Unimplemented, creates no Profile or lease and touches
no native state or elapsed source. Its backend query reports stock upstream.
Other methods have linkable out-of-line stubs returning Unimplemented, with empty
view/reset semantics only for impossible/moved leases. The stock protocol fixture
separately reaches actual public Drogon behavior. Unsupported facade failures
cannot be called native capacity/deadline observations or setup-free protocol red.

Limits are explicit caller values; reference/reduced test literals derive from the
parent profile. Absence is never known zero. The exact C++20 declaration shape is:

```cpp
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>

namespace drogon { class WebSocketConnection; }
namespace subathon::server::native_ws {
using Connection = std::shared_ptr<drogon::WebSocketConnection>;

enum class Backend { stock_upstream, admitted_corrected };
Backend linked_backend() noexcept;
struct Unimplemented {};
enum class Failure {
    null_connection, not_owned, wrong_owner, closed, resource_limit,
    allocation_failure, allocation_bound_unavailable, outside_delivery,
    delivery_mismatch, delivery_claimed, elapsed_unavailable,
    elapsed_regression, deadline_expired, invalid_input
};
struct Unavailable { Failure reason; };
template<class T> using Result = std::variant<T, Unimplemented, Unavailable>;

struct Limits {
    std::uint64_t connections, incomplete_upgrades;
    std::uint64_t frame_bytes, message_bytes, frames_per_message;
    std::uint64_t callback_leases_per_connection;
    std::uint64_t callback_bytes_per_connection, callback_bytes_global;
    std::uint64_t raw_capacity_bytes, assembly_capacity_bytes;
    std::uint64_t application_output_bytes, control_output_bytes;
    std::uint64_t frames_per_turn;
    std::uint64_t upgrade_ms, frame_ms, message_ms, close_ms;
};
struct ElapsedInstant { std::uint64_t nanoseconds; };
struct ElapsedUnavailable {};
class ElapsedSource {
public:
    virtual ~ElapsedSource() = default;
    virtual std::variant<ElapsedInstant, ElapsedUnavailable>
        sample() const noexcept = 0;
};
enum class ConfigurationFailure {
    zero_limit, inconsistent_limits, arithmetic_overflow, unsupported_platform,
    missing_elapsed_source, already_installed, native_started
};
struct InvalidConfiguration { ConfigurationFailure reason; };

enum class MessageKind { text, binary };
struct MessageView { MessageKind kind; std::span<const std::byte> bytes; };
struct LeaseResult;
class MessageLease {
public:
    MessageLease(MessageLease&&) noexcept;
    MessageLease& operator=(MessageLease&&) noexcept;
    MessageLease(const MessageLease&) = delete;
    MessageLease& operator=(const MessageLease&) = delete;
    ~MessageLease();
    std::optional<MessageView> view() const noexcept;
    std::uint64_t reserved_payload_bytes() const noexcept;
    LeaseResult try_copy() const noexcept;
    void reset() noexcept;
private:
    struct State;
    std::unique_ptr<State> state_;
    explicit MessageLease(std::unique_ptr<State>) noexcept;
    friend class Profile;
};
struct LeaseResult {
    std::variant<MessageLease, Unimplemented, Unavailable> value;
};

struct NoCloseFrame {};
struct CloseWithoutStatus {};
struct CloseWithStatus {
    std::uint16_t code;
    std::array<std::byte, 123> reason;
    std::uint8_t reason_size;
};
using PeerClose = std::variant<NoCloseFrame, CloseWithoutStatus, CloseWithStatus>;
using LocalClose = std::variant<CloseWithoutStatus, CloseWithStatus>;
struct ByteCounters {
    std::uint64_t raw_capacity, assembly_capacity, fixed_native_storage;
    std::uint64_t application_output, control_output;
    std::uint64_t callback_payload_connection, callback_payload_global;
};
struct GlobalSnapshot {
    std::uint64_t owners_live_or_retiring, owners_peak;
    std::uint64_t upgrade_owners, upgrade_owners_peak;
    std::uint64_t callback_leases, callback_leases_peak;
    std::uint64_t callback_payload_bytes, callback_payload_bytes_peak;
};
struct Snapshot {
    ByteCounters current, peak;
    std::uint64_t callback_leases_connection, callback_leases_global;
    std::uint64_t owners_live_or_retiring, owners_peak;
    std::uint64_t upgrade_owners, upgrade_owners_peak;
    std::uint64_t callback_leases_connection_peak, callback_leases_global_peak;
    std::uint64_t ws_bytes_read, ws_bytes_consumed, frames_consumed;
    std::uint64_t frames_in_last_turn;
    bool continuation_pending, close_sent, transport_closed;
    PeerClose first_peer_close;
    std::optional<ElapsedInstant> frame_deadline, message_deadline, close_deadline;
};
struct Admitted {};
enum class CloseDisposition { started, already_closing };
struct Rechecked {};
class Profile {
public:
    Profile(const Profile&) = delete;
    Profile& operator=(const Profile&) = delete;
    ~Profile();
    LeaseResult accept_callback(const Connection&) noexcept;
    Result<Admitted> try_send(const Connection&, MessageKind,
                             std::span<const std::byte>) noexcept;
    Result<CloseDisposition> start_close(const Connection&, const LocalClose&) noexcept;
    Result<Rechecked> recheck_deadlines(const Connection&) noexcept;
    Result<Snapshot> inspect(const Connection&) const noexcept;
    Result<GlobalSnapshot> inspect_global() const noexcept;
private:
    struct State;
    std::unique_ptr<State> state_;
    explicit Profile(std::unique_ptr<State>) noexcept;
    friend struct ProfileFactory;
};
using CreateResult = std::variant<std::unique_ptr<Profile>, InvalidConfiguration,
                                  Unimplemented, Unavailable>;
CreateResult create_profile(const Limits&,
                           std::shared_ptr<const ElapsedSource>) noexcept;
} // namespace subathon::server::native_ws
```

`ProfileFactory` is private implementation support, not a caller API. Public callers
cannot construct Profile or lease state. The elapsed input is already checked
unsigned64 nanoseconds; native timespec conversion and session UTC/trust belong
to the later actual host adapters and their separate review/native gates.

## Exact operation semantics

**Factory/configuration.** `create_profile` is startup-only, before Drogon listener/
connection creation; no concurrent call and only one successful installation per
process/framework lifetime. It copies Limits into owned immutable state and retains
shared ownership of the one read-only ElapsedSource. Caller mutation/destruction of
its Limits/shared_ptr variable cannot change installed policy or destroy the source.
The source must itself provide thread-safe read-only observations; replacing its
object/domain is not supported. Native connections, pending bounded callbacks and
leases retain their actual dependent context; no callback captures a dead facade.
The factory does not start Drogon, bind ports, open SQL or grant host readiness.
A second install after a successful installation refuses already_installed. A
first install after ACTUAL native startup refuses native_started, without inventing
any successful prior installation. For the corrected backend, check in this order:
(1) successful existing installation, (2) actual native-started source state,
(3) unsupported platform, (4) absent source, then (5) zero, overflow and consistency
validation, before allocating/installing native policy or sampling the elapsed
source. A previous failed attempt is not a successful installation. If both first
two facts hold, already_installed wins. Actual startup state comes from the selected
framework owner, not a caller bool. Stock's unconditional Unimplemented precedes ALL
these checks and does not read native startup or sample/retain the supplied source.
Successful CreateResult always contains a NONNULL unique_ptr<Profile>; nullptr is
never success and any construction failure has its truthful error alternative.
The admitted backend is selected by reviewed build/SDK identity, never a client/
config string.

For the implemented path, reject zero limits, absent source, unsupported platform,
overflowing byte/count/duration products/additions and inconsistent bounds before
native ownership. In particular frame<=message<=assembly capacity; raw capacity
can hold frame+14; application output can hold frame+10; control reserve>=254;
callback per-connection bytes can hold one message and global>=per-connection;
finite connection*callback-slot and integer/size_t conversions must fit. Duration
ms-to-ns is exact and nonoverflowing, all durations positive; start+duration is
checked separately at each actual start. These are simple compatibility rules for
this concrete reference profile, not reduced domain/accounting types. Allocation
failure is Unavailable/allocation_failure, never success or InvalidConfiguration.
On stock, Unimplemented takes precedence and no validation result is fabricated;
invalid-configuration tests therefore remain explicit missing first-party behavior
red on that stub, separate from native wire defects. No `Profile` exists on failure.

**Native owner/connection and safe check order.** A method taking Connection
first rejects null as Unavailable/null_connection, without dereference, native
virtual calls, source sampling, allocation or reservation. Next it looks up a
CURRENT actual-source-owned binding in the bounded owner slots using object AND
shared-ownership identity, without calling methods on an unknown object. No match
returns Unavailable/not_owned. This single reason includes foreign, stale and
fully retired bindings; the API keeps no tombstone history to guess which it was.
Then compare the current thread/loop with the owner recorded by actual native
registration; mismatch returns wrong_owner before source/time/reservation work.
Hold the actual context while the method runs so concurrent final release cannot
retire it mid-call. Only after these checks may operation-specific state, delivery,
time and capacity checks run. No native pointer from a caller is blindly trusted.

Mutating/send/claim/recheck operations reject a known closed native context as
Unavailable/closed. The deliberate exception is nonmutating inspect: a genuinely
still-owned TERMINAL context may be inspected on its actual still-running owner
loop, preserving transport_closed/first-Close/current-retained-lease facts. It
must not revive state or sample time. When the actual close AND all dependent
leases/queued records/continuations/teardown acknowledgment are gone, the owner
slot/context is retired and removed; a later inspect returns Unavailable/not_owned,
NEVER a zero Snapshot or historical cached peer record. A retained Connection
shared_ptr alone does not promise the profile context is still registered.

`inspect_global` has no Connection parameter and is a synchronized pure fixed-size
observation of this Profile's actual global budget/owner-slot state, callable from
any thread. It samples no clock/native peer and creates no peer history. It is the
bounded way to observe final retirement after the last empty lease is released:
lease reset synchronously releases its reservation; an actual native-owner barrier
lets already queued cleanup run; inspect_global then reports actual remaining
slots/leases/bytes. A barrier does not assert retirement. If cleanup is still pending,
its slot MUST remain counted and the assertion must wait for actual bounded
progress or fail. Once last cleanup is actually gone, known0 follows from the real
budget, even though inspect(old_connection) now returns not_owned. The fixture
keeps at most one such control/snapshot in flight, bounds its observation loop and
retains no terminal snapshot list. Framework owner loops stay alive for this check.

No Profile method sends a Qt command or executes SQL. Native connection admission,
pre-read/pre-growth/Upgrade quotas must already be bound by the startup native
profile; a late controller attach is not an ingress guard. The downstream patched
SDK's private mechanism/export ABI remains an API-012 source/admission gate.

**Actual callback source and once-only ownership claim.** `accept_callback` now
accepts ONLY Connection; caller-supplied kind/string are intentionally removed.
The exact validated kind, native-owned completed payload object/storage and dispatch
lifetime come from a narrow PRIVATE selected-source current-delivery hook. Immediately
before its synchronous internal data-dispatch callback, native code binds the actual
connection, Text/Binary kind and its actual fully validated/reserved payload to the
CURRENT dispatch scope, initially unclaimed. The binding is private, per current
native dispatch/owner and not a public token, reusable permit or independently
constructible issuer. Active outer/reentrant/other-connection scopes cannot be
claimed as the current one. Source-owned RAII invalidates/clears it on dispatch exit,
including exceptions. No binding survives an async queue interval.

After safe connection/owner/closed checks above, absence of a current native data
scope returns outside_delivery; a foreign active scope or changed kind/payload
identity returns delivery_mismatch; a successful earlier claim in this scope returns
delivery_claimed. These refusals precede move, source sampling for this operation,
reservation and allocation. Native binding must still identify the source's exact
validated kind and the source-owned payload object/storage; no caller can substitute
another string, type or old data. Public accept_callback obtains those values from
the private hook and derives the resulting MessageView from them. An actual
completed native producer has ALREADY reserved/proved its own capacity before any
native growth; downstream acceptance cannot retroactively prove that allocation.

The corrected routing invokes this internal ownership transition BEFORE exposing
mutable raw payload to an application consumer. A public controller/app consumer
cannot observe or modify an unreserved raw string first and then call it validated.
The frozen fixture's corrected path enters as the narrow internal dispatch shim,
claims through Profile first, and emits data observations only from the resulting
lease. Any preclaim diagnostic is bounded metadata with no retained raw-payload
copy, and is not application delivery. The stock path has no implemented Profile
and separately reports actual raw stock callbacks without a profile-capacity claim.
Native source/resource review must prove this actual routing and payload identity;
the fixture cannot register or fabricate the private current-delivery hook.

On a genuine current delivery, accept_callback reserves one callback lease and
ACTUAL retained payload capacity before moving ownership out of native assembly.
Success moves the exact bytes/type once into the lease and marks that current
source delivery claimed before any consumer callback/reentry. Failure leaves the
native-owned payload/reservation intact for its normal dispatch cleanup, creates
no lease, and refuses application delivery; resource exhaustion requests native
bounded1009/teardown where appropriate. No fixture guard implements this behavior.
Direct strings can test outside-scope refusal only; they cannot create evidence
of native validation, ownership or capacity. The source proof must also account for
selected SSO/move behavior, any object/inline copy and all simultaneously retained
producer/consumer storage; a C++ move expression is not automatically zero-copy.

Lease view is an immutable borrowed span valid while that lease remains valid;
zero-length valid payload is optional-view present, while moved/reset is absent.
`reserved_payload_bytes()` is actual payload capacity of a valid lease, zero ONLY
for reset/moved or a genuinely zero-capacity allocation. Fixed object/context/
terminator overhead is separately bounded/audited, never hidden payload capacity.
Move transfers the reservation without allocating/copying bytes or increasing the
live count; move assignment first releases its old reservation once.
`try_copy()` uses a PRIVATE selected allocation strategy with a source-proven
finite conservative capacity upper bound B for this exact length/strategy/allocator,
plus separately bounded object/terminator/context overhead. Under the synchronized
budget it reserves B AND one new lease slot atomically BEFORE allocation/copy.
After allocation, its proved actual retained charge A must satisfy A<=B; only a
DOWNWARD adjustment from B to A is permitted. Never allocate, inspect capacity,
and then try to reserve or grow a reservation. If no finite independently reviewed
bound is known, return Unavailable/allocation_bound_unavailable BEFORE allocation,
copy or slot reservation. A failed reservation returns resource_limit before work;
an admitted allocation failure releases that reservation once and returns
allocation_failure. An unexpected A>B is a source/proof defect, not passing
preallocation evidence; fail closed and preserve that failure. No generic allocator
API is exposed. All copies, including empty messages, consume slots. No unaccounted
implicit string copy is permitted by the adapter's contract. A per-conn
lease limit2 therefore also prevents an unlimited number of zero-byte clone handles.
The `connections` budget is an OWNER-SLOT budget: established native WebSockets
are a subset. Keep a slot while the native connection, its first-Close state, ANY
callback lease (even empty), pending continuation/control/task or native teardown
acknowledgment still retains that peer's context. A TCP close does NOT free that
slot. Release it exactly once only after actual native close plus all dependent
leases/queued work are destroyed, cancelled-and-removed or owner-acknowledged.
An expired weak callback still sitting in a queue counts until removed/executed;
calling it weak or invalidated does not erase its retained queue/control block.
New admission refuses when all live-or-retiring owner slots are occupied. The
same lifetime rule holds the separate finite incomplete-upgrade owner slots until
their actual work is gone. There is no unbounded retired-owner/history list.

Consequently total outstanding lease handles are also bounded by the checked
connections*slots product, including outstanding leases from closed connections:
32owner slots and2leases give at most64handles. Zero bytes cannot bypass either
slot budget. This uses the existing two finite configuration counts, with no
additional generic registry. Copy after connection closure refuses; an existing
retained lease's bytes remain valid until release.

Lease move/reset/destruction can occur on another consumer thread; the same object
is not concurrently mutated. Budget release and copy admission are synchronized
and atomic with respect to global/per-connection limits. They retain owned context/
source lifetime, release exactly once, and schedule at most the ONE native weak-
lifetime resumption continuation already permitted per connection. They create no
unbounded cross-thread queue. Final native teardown invalidates that continuation;
old release cannot reopen a closed/new-generation connection. Source/resource review
must verify these operations and the full context/metadata bounds.

**Send/Close/recheck.** `try_send` is local admission only, never delivered-to-peer
proof. For a live owned connection, try_send refuses Unavailable/closed when
close_sent or a valid close_received is latched, before capacity reservation,
encoding, copy or queue. This means application-output admission is closed; it
never sets or infers Snapshot.transport_closed. Actual deadline/source guards
still run as required; expiry or unavailable-time teardown is not bypassed. With
valid time before the original deadline, another valid start_close returns
already_closing, emits no extra Close and preserves the first body/deadline.
It admits the complete encoded wire bytes including headers and every live
owned copy BEFORE encoding/copy/queue; input span need live only for this call.
Refusal changes no application output; invalid local Text/input is invalid_input.
Use the native profile's separate control reserve. `start_close` validates LocalClose
under the frozen set/direction/reason rules, latches its first deadline and native
state on the IO loop, and enqueues at most one Close behind already admitted output.
Repeated calls return already_closing without rewriting peer observation or resetting
time. It does not prematurely half-close away the peer handshake. `recheck_deadlines`
is a narrow native-owner wake: reads the configured source and invokes the SAME
native deadline/continuation entry check used by actual IO/timer callbacks. It accepts
no caller-supplied time, does not advance/fix time and creates no parallel timer.
Unavailable/regressing/overflowing observations and equality expiry follow the
accepted teardown/no-reset semantics. Source failure cannot invent SQL outcomes.

**Snapshot/ordering.** `inspect` (including the terminal-owned exception above) is
read-only, fixed-size and nonallocating on the actual owner, and does not sample time, check deadlines, consume input or mutate
state. Unimplemented/Unavailable is an outer result; no Unknown field is serialized
as0. A successful snapshot comes from ACTUAL native owner hooks in the admitted
source plus actual lease budget state; all fields describe one coherent observation
(the budget lock may be sampled while that IO owner cannot interleave). For a fully
closed but still-owned native object, zero bytes are Known only after actual release.
PeerClose is the first VALID observation: no frame versus empty versus code/reason;
reason_size<=123 and bytes beyond it have no semantic value. No invalid/control raw
payload is decoded in the fixture to manufacture this native observation.

ByteCounters.current/peak report real retained capacity/copies, not RSS or logical
string length. raw/assembly include actual selected storage capacity and allocator
rounding. `fixed_native_storage` separately accounts fixed read scratch/framing/
terminator bytes not in payload counters; no capacity bytes may be counted twice or
silently omitted. Output counters charge all simultaneously retained encoded/native
wire copies; control remains separate. Callback counters include every actual lease
copy and global state. Peaks are lifetime high-water observations, not success claims
about an unobserved allocation; exact source/resource review is still mandatory.
`ws_bytes_read` starts at native WebSocket ownership after Upgrade; consumed bytes
include native-parsed frame headers/payload, distinct from read capacity/read-ahead.
Counters are checked, never wrapped; overflow tears down without fabricated values.
owners_live_or_retiring/upgrade_owners and their peaks come from the actual slot
owner, including closed peers with empty leases or queued weak work. They never
exclude a dead owner merely because its TCP socket closed. frames_in_last_turn and
continuation_pending originate from the native scheduler. Deadlines are the real same-domain absolute anchors, absent only when not active.

An stdin/control acknowledgment only proves its owner task ran. It is NOT a barrier
for earlier network bytes still in the kernel. Tests use actual native callback/
wire/closed events, or inspect checked consumed/read byte progress before asserting
the associated state. Only one bounded command/snapshot response is outstanding;
no polling sleep is an ordering mechanism. Snapshot failure is a failed observation,
not a passing zero-work guard. An owned fake source advanced by a test plus actual
native-owner recheck proves simulated deadline behavior, not OS/session-clock trust.

**Lifetime.** Contexts own immutable configuration/source while actual native work
or leases depend on them. Destroying the facade cannot dangle a native callback;
its destructor neither waits on a network callback nor claims thread/process drain.
The actual host later owns its owner-loop barriers and teardown. Its managed
owner-slot reservation survives facade release while native connections/leases/
continuations still depend on context; no asynchronous callback can free the slot
before its own queue record/dependencies are gone. The fixture keeps
facade/source alive until connections have closed, releases its leases and joins
native framework threads before destroying them; killed cleanup preserves failure.
No retry/start lifetime or authority is obtained by keeping a Profile/snapshot/lease.

## Frozen fixture now versus later

The same two authored fixture files contain distinct, explicit observation modes:

1. At startup report admitted linked backend and result of factory. In stock mode
   the factory is Unimplemented, no Profile object is used, and a minimal actual
   Drogon controller still starts on127.0.0.1:0 using public APIs. Its only logical
   stock setting is explicit maximum message size for compatible raw-wire cases;
   it claims none of the profile's native buffer/time/lease capabilities. It logs
   actual raw native callback type/bytes and forwards explicit public native send/
   shutdown/forceClose commands. It never echoes received Close, reparses bytes,
   synthesizes error Close, repairs control ownership or starts a fixture deadline.
   Public Close/fragment/Ping defects are asserted from actual raw wire/callbacks.
2. In the admitted-corrected build, create_profile must succeed for a valid case
   BEFORE native listener startup, supplying the same literal Limits and owned
   source. The fixture's native controller and raw-wire oracles remain; Text/Binary
   ownership FIRST flows through actual-source-bound accept_callback (no raw payload
   consumer before its lease), and control commands call
   Profile methods/inspect. Invalid config is tested as its exact typed refusal.
   A failed valid factory is setup failure, not behavior red. No source patch or
   missing symbol is introduced to make the stock baseline fail to compile.
3. Resource/lease/monotonic cases require the Profile result and all concrete owner
   observations. On stock they explicitly FAIL `Unimplemented` (no skip), separately
   classified as missing first-party profile behavior. They cannot be reported as
   native capacity/clock observations. The protocol family still reaches the real
   stock implementation regardless of that absent facade. At least those reached
   protocol failures must be independently attributed before implementation starts.
4. Corrected-source implementation must pass these frozen oracles, with the actual
   observations/hooks/callback budgets linked to the real owners. No test path may
   calculate native snapshot values or simulate enforcement for that profile. Test
   controlled clock is clearly tagged simulation; an actual read-only Linux monotonic
   source inside the fixture can additionally drive real loopback timeout tests,
   without becoming the actual API-001/SESSION-009 source or ready-host binding.

## Boundary examples

These are literal semantic oracles, not executed tests or allocator observations.

| ID | Literal condition/action | Exact required distinction |
| --- | --- | --- |
| B01 | Limit2owner slots, one peer TCP-closed while one valid zero-byte lease remains | inspect on its actual running owner succeeds with transport_closed=true and lease count1; global owners remains1. New peer still consumes the other slot. Close alone never freed the first. |
| B02 | Continue B01 with its second peer still live; release the first peer's final empty lease and complete its actual native cleanup/barrier | Global owners becomes1, counting the live second peer; inspect(first_connection) returns Unavailable/not_owned, never known zero peer state. Global owners may become0 only after the second peer closes and ALL of its leases/queued work/teardown dependencies are actually gone too. No tombstone exists. |
| B03 | Private clone strategy for logical3bytes proves B=8; available payload budget7 | resource_limit before allocation/copy/slot. With budget8 and actual A=5, reserve8 first, then charge5 by downward adjustment. An unknown bound returns allocation_bound_unavailable before any allocation even with abundant budget. Values3/8/7/5 are synthetic policy oracles, not a claimed std::string allocation. |
| B04 | Genuine validated Text `41` callback current on connectionA, source owns its payload | accept_callback(A) derives Text/`41` from current source and succeeds once. A second call in that dispatch returns delivery_claimed with no extra reservation/copy. After scope exit a later call returns outside_delivery. |
| B05 | Construct local string `41` and call accept_callback(A) on its IO owner outside a native delivery, or call B while A is current | First returns outside_delivery; local string cannot be supplied to the API. Second returns delivery_mismatch (provided B otherwise has a current owned live binding). No substitute bytes/type or native-validation evidence is produced. |
| B06 | Corrected backend actual native-started=true, no successful prior profile | create_profile returns InvalidConfiguration/native_started; no source sample, policy allocation or fictitious installation. If a successful profile already exists too, already_installed wins. Stock returns Unimplemented before either query. Successful result is always nonnull. |
| B07 | null Connection; then a foreign/non-current binding; then genuine owned binding on wrong thread | Respectively null_connection, not_owned, wrong_owner, all before dereference of unknown object, elapsed sample or reservation. A still-owned terminal binding on its true owner is inspectable; mutation returns closed. |


Strict event/command/log/buffer/connection caps and one native lifetime per child
apply to the authored fixture. Native pending-output observations and owner
barriers establish drain state; a peer merely not reading proves no pending buffer.
Independent exact test/stub/build/dependency review precedes every new body.
Compilation, passive registration and actual probe/link/loaded-library identity
must work before intended native behavior red. Only then may a fresh API-012
implementation correct the separately admitted source profile, preserve assertions,
obtain source/resource/ABI/license review and pass actual acceptance/current CI.
