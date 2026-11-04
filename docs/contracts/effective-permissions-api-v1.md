# Effective permission evaluation — concrete API proposal v1

ACCESSTEST-002 API document stage, claim `101ff83b`. This refines the approved
[logical model](effective-permissions-v1.md), blob
`3d82bfa418bb14099ee130fca7a400adca66500b`, and its
[actual full review](../evidence/accesstest-002-contract-review.md), blob
`ce5950a599c1039d8c5b0c42a04a22c8e0cd7470`. Specification v0.45 §10/AUTH-03/06,
the [owner decision](../evidence/accesstest-002-owner-decision.md),
[common rational values](wire-primitives-v1.md) and
[execution boundary](event-state-boundaries-v1.md) remain authoritative.

This proposes one C++20 core operation. No header, implementation, test, parser,
schema, authenticating adapter or running enforcement is supplied. Different actual
API review precedes any minimal source stage. ACCESS-003 and SESSION keep their
editing/delegation/automation and account/session responsibilities.

## 1. Public declarations

Future paths are `core/include/subathon/core/effective_permissions.hpp` and
`core/src/effective_permissions.cpp`; namespace `subathon::core`. The public header
uses only `<cstdint>`, `<exception>`, `<optional>`, `<stdexcept>`, `<string>`,
`<vector>` and `<utility>` as needed. Private arithmetic uses selected Boost 1.83;
it does not reuse the narrower decimal parser/finalizer or expose Boost types.
The following declarations freeze aggregate member order and defaults. All enums
have underlying type `std::uint8_t`, with consecutive values starting at zero in
the order shown. Out-of-range values made by casts are invalid input.

```cpp
namespace subathon::core {

enum class PermissionFactsState : std::uint8_t { unknown, complete };
enum class PermissionOwnerState : std::uint8_t { unknown, absent, present };
enum class PermissionOperationClass : std::uint8_t {
    ordinary, protected_access_management, protected_recovery, helper_password_reset
};
enum class PermissionNumberKind : std::uint8_t { rational, integer };
enum class PermissionResourceMode : std::uint8_t { any, allowed_set };
enum class PermissionQuantityMode : std::uint8_t {
    any, at_most, closed_interval, magnitude_at_most
};
enum class PermissionOrigin : std::uint8_t { individual, role };
enum class PermissionEffect : std::uint8_t { allow, deny };

struct PermissionRational {
    std::string numerator = "0";
    std::string denominator = "1";
};
struct PermissionQuantityDefinition {
    std::string slot;
    std::string unit;
    PermissionNumberKind kind = PermissionNumberKind::rational;
    std::optional<PermissionRational> minimum;
    std::optional<PermissionRational> maximum;
    std::uint8_t allowed_modes = 15;
};
struct PermissionOperationDefinition {
    std::string operation;
    PermissionOperationClass classification = PermissionOperationClass::ordinary;
    std::vector<std::string> resource_slots;
    std::vector<PermissionQuantityDefinition> quantity_slots;
    std::optional<std::string> helper_target_slot;
};
struct PermissionCatalog {
    PermissionFactsState state = PermissionFactsState::unknown;
    std::string revision;
    std::vector<PermissionOperationDefinition> operations;
};
struct PermissionOwnerFacts {
    PermissionOwnerState state = PermissionOwnerState::unknown;
    std::optional<std::string> account;
};
struct PermissionResourceBinding {
    std::string slot;
    std::string resource;
};
struct PermissionQuantityBinding {
    std::string slot;
    std::string unit;
    PermissionRational value;
};
struct PermissionRequest {
    std::string operation;
    std::vector<PermissionResourceBinding> resources;
    std::vector<PermissionQuantityBinding> quantities;
};
struct PermissionResourceConstraint {
    std::string slot;
    PermissionResourceMode mode = PermissionResourceMode::any;
    std::vector<std::string> allowed;
};
struct PermissionQuantityConstraint {
    std::string slot;
    std::string unit;
    PermissionQuantityMode mode = PermissionQuantityMode::any;
    std::optional<PermissionRational> lower;
    std::optional<PermissionRational> upper;
};
struct PermissionPolicyEntry {
    PermissionOrigin origin = PermissionOrigin::individual;
    std::string source;
    std::optional<std::string> role;
    std::string operation;
    PermissionEffect effect = PermissionEffect::allow;
    std::optional<std::string> grant;
    std::vector<PermissionResourceConstraint> resources;
    std::vector<PermissionQuantityConstraint> quantities;
};
struct PermissionPolicyFacts {
    PermissionFactsState state = PermissionFactsState::unknown;
    std::vector<std::string> assigned_roles;
    std::vector<PermissionPolicyEntry> entries;
};
struct EffectivePermissionInput {
    std::string actor;
    std::string snapshot;
    PermissionCatalog catalog;
    PermissionOwnerFacts owner;
    PermissionPolicyFacts policy;
    PermissionRequest request;
};
struct EffectivePermissionLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t component_bytes = 16384;
    std::uint64_t input_items = 4096;
    std::uint64_t input_owned_bytes = 16777216;
    std::uint64_t result_items = 4096;
    std::uint64_t result_owned_bytes = 33554432;
};

enum class PermissionOutcome : std::uint8_t { deny, permit };
enum class PermissionReason : std::uint8_t {
    default_deny, individual_allow, individual_deny, individual_limit,
    role_allow, role_deny, protected_owner, protected_owner_target
};
enum class PermissionSourceKind : std::uint8_t { protected_owner, individual, role };
enum class PermissionMatch : std::uint8_t {
    other_operation, resource_mismatch, quantity_mismatch, complete
};
enum class PermissionSourceDisposition : std::uint8_t {
    not_applicable, suppressed, quantity_limit,
    permit_witness, fitting_allow, decisive_deny
};
struct PermissionSourceExplanation {
    PermissionSourceKind kind = PermissionSourceKind::individual;
    std::string source;
    std::optional<std::string> grant;
    std::optional<std::string> role;
    std::string operation;
    PermissionEffect effect = PermissionEffect::allow;
    PermissionMatch match = PermissionMatch::other_operation;
    PermissionSourceDisposition disposition = PermissionSourceDisposition::not_applicable;
    std::vector<std::string> failed_resources;
    std::vector<std::string> failed_quantities;
};
struct EffectivePermissionDecisionData {
    std::string actor;
    std::string snapshot;
    std::string catalog_revision;
    PermissionOwnerFacts owner;
    PermissionRequest request;
    PermissionOutcome outcome = PermissionOutcome::deny;
    PermissionReason reason = PermissionReason::default_deny;
    std::optional<std::uint64_t> witness_row;
    std::vector<PermissionSourceExplanation> sources;
};

class EffectivePermissionDecision;
[[nodiscard]] EffectivePermissionDecision evaluate_effective_permission(
    const EffectivePermissionInput &input,
    const EffectivePermissionLimits &limits = {});

class EffectivePermissionDecision final {
  public:
    EffectivePermissionDecision(const EffectivePermissionDecision &) = default;
    EffectivePermissionDecision(EffectivePermissionDecision &&) noexcept = default;
    EffectivePermissionDecision &operator=(const EffectivePermissionDecision &) = default;
    EffectivePermissionDecision &operator=(EffectivePermissionDecision &&) noexcept = default;
    [[nodiscard]] const EffectivePermissionDecisionData &data() const noexcept;
  private:
    explicit EffectivePermissionDecision(EffectivePermissionDecisionData data);
    EffectivePermissionDecisionData data_;
    friend EffectivePermissionDecision evaluate_effective_permission(
        const EffectivePermissionInput &, const EffectivePermissionLimits &);
};

enum class EffectivePermissionError : std::uint8_t {
    invalid_limits, input_limit, invalid_reference, invalid_catalog,
    invalid_facts, invalid_request, invalid_policy, contradictory_individual,
    invalid_number, facts_unavailable, result_limit, allocation_failure
};
class EffectivePermissionFailure final : public std::exception {
  public:
    explicit EffectivePermissionFailure(EffectivePermissionError code) noexcept;
    [[nodiscard]] EffectivePermissionError code() const noexcept;
    [[nodiscard]] const char *what() const noexcept override;
  private:
    EffectivePermissionError code_;
};
class EffectivePermissionsNotImplemented final : public std::logic_error {
  public:
    EffectivePermissionsNotImplemented();
};
} // namespace subathon::core
```

Only the evaluator constructs a decision; aggregate inputs carry no validation or
authority certificate. `data()` borrows immutable storage until that object is
assigned, moved from or destroyed. No input reference survives the call. A result
survives destruction/mutation of input, including all request values and source
strings. Defaulted copy/assignment may allocate and throw `std::bad_alloc`; those
caller-initiated copies are outside the evaluator's budgets/error mapping. A moved
from object is destructible/assignable; its decision data has no promised meaning.

Failure construction/getters are inert and allocation-free. `what()` returns
`"Effective permission evaluation failed"` for every code, never identifiers or
policy text; `code()` is the only diagnostic payload. The marker's exact text is
`"Effective permission behavior is intentionally unimplemented; review tests first"`.
Neither returned value authorizes a later command.

## 2. Raw values, complete shapes and source identities

Every string other than rational components is a byte-exact opaque nonempty
reference when required. This includes slot/unit identifiers. NUL and high bytes
are permitted; unsigned-byte lexicographic order, shorter equal prefix first,
applies everywhere. No Unicode normalization, text parsing, display label, UUID,
provider category, wildcard spelling or unit conversion is inferred. `unit` names
a declared exact unit, such as `ms`, `micro_points` or `count`; only exact identity
with the owning quantity definition is accepted. These examples are not a complete
operation/unit registry. The authoritative operation catalog establishes meaning;
a client cannot invent a unit, protected classification or operation by passing
an internally consistent raw object to a pure calculation.

A complete catalog has nonempty revision and unique operation keys. Its resource
slot keys and quantity slot keys are each unique in their separate namespaces.
They define finite named tuples; declaration/input vector order is irrelevant.
Every request and policy tuple has exactly one corresponding binding/constraint
per declared slot. Other-operation entries require that operation's own definition;
they are not decoded using the current request's shape. Unknown operation keys
are `invalid_request` or `invalid_policy`, respectively. Missing catalog operations
are not supported/Any operations. Empty resource/quantity lists are valid exactly
when the operation declares those empty lists.

`allowed_modes` uses bits 0..3 for the four quantity modes in declaration order.
It must be 1..15. Every policy constraint mode must be admitted by its definition.
A rational domain admits exact values; an integer domain requires denominator 1.
Optional minimum/maximum define inclusive request-domain bounds; absence is truly
unbounded within finite resource limits. Both present require minimum <= maximum.
Definition endpoints must themselves follow the number kind. These bounds express
signed64/count/nonnegative/custom domains without imposing them on all operations.
For example signed64 integers use literal minimum -9223372036854775808/1 and
maximum 9223372036854775807/1. Permission matching never narrows to these types.

Resource `any` requires empty `allowed`; `allowed_set` requires a nonempty set with
unique members. Quantity `any` requires both endpoints absent; `at_most` uses only
`upper`; `closed_interval` requires both; `magnitude_at_most` uses only a nonnegative
`upper`. Intervals require lower <= upper. Policy endpoints obey the quantity's
number kind but need not lie inside its request-domain bounds. This distinction
allows signed64 minimum's magnitude bound 9223372036854775808/1. An endpoint
outside the request domain does not expand that domain or round a request.

A rational numerator is `0` or an optional ASCII minus followed by a nonzero digit
and zero or more digits. A denominator is a nonzero digit followed by digits.
No plus, negative zero, whitespace, leading zero, decimal/exponent, embedded NUL or
other byte is accepted. Require gcd(abs(numerator),denominator)=1; zero requires
0/1. Reject rather than normalize, including in irrelevant catalog/policy entries.
The caller owns its already-constructed strings/containers; this is not a JSON,
wire, decimal or provider parser. Every byte, including numerator minus, counts.

Each allow alternative is one flattened `PermissionPolicyEntry` with present
nonempty grant; alternatives may repeat source identity. Deny entries have absent
grant and exactly one complete tuple. `role` is absent for individual origin and
present for role origin; every role must be in the unique `assigned_roles` set.
A source identifies one origin/role/operation/effect consistently. Repeated source
rows are allowed only for distinct grant identities of that same allow source;
duplicate source+grant or a repeated deny source is invalid, even byte-identical.
Grant identities are unique within their source, not across unrelated sources.
Source identities are unique across individual/role policy origins. All supplied
entries belong to this actor's snapshot; no editable actor/owner boolean is accepted.

Catalog `unknown` requires empty revision/operations; policy `unknown` requires
empty assigned_roles/entries. Unknown is not known-empty or a partial successful
load. Owner unknown/absent both require absent account; present requires one
nonempty account reference. A complete policy may have known-empty lists. Snapshot
and actor are always required. Snapshot covers the coherent supplied owner/catalog/
policy facts; its bytes are provenance, not a currentness check.

For `helper_password_reset`, helper_target_slot is present and names one declared
resource slot whose binding is the target account. All other classifications
require it absent. Classification is trusted server catalog metadata, not a
request flag. Protected access-management/recovery operations keep all declared
request shape/domain checks; current-owner identity alone supplies their narrow
permission source after valid evaluation. Sensitive workflows/fresh authentication
are still required outside this pure API. It neither implements a console recovery
route nor maps the full product operation catalog; that is an integration gate.

## 3. Validation, availability and error priority

A stage finishes for the entire supplied input before a later stage starts. Error
priority is the following stage order, independent of input-vector permutation.
Within a stage containing several error codes, the listed code order wins; the
error holds no caller row index or input-derived diagnostic string. Future tests
supply their own row traces. Do not return a denial to conceal an evaluation failure.

| Stage | Required work and error order |
| --- | --- |
| 0 | Validate all six positive limits and checked derived work sizes (§5): `invalid_limits` |
| 1 | Borrowed full byte/item/individual string-length preflight, including every supplied losing/irrelevant field: `input_limit` |
| 2 | Validate all tag ranges (including definition/constraint modes) and unknown/present-state payloads: `invalid_catalog`, `invalid_facts` (owner), `invalid_policy`; then all required nonempty identities: `invalid_reference` |
| 3 | Unknown catalog or unknown owner: `facts_unavailable`. Do not pretend to validate catalog-dependent shapes without their definitions |
| 4 | Complete catalog keys/slot uniqueness, kind/mask/target-slot shape; request shape/unit/known operation; all policy shape/unit/identity/membership rules: `invalid_catalog`, `invalid_request`, `invalid_policy` |
| 5 | Every opposing individual allow/deny pair on the same operation: reject overlapping whole resource scopes as `contradictory_individual`, before inspecting quantity semantics or using quantity disjointness |
| 6 | All rational spellings and reduction: `invalid_number`; then definition domains/intervals, request domain/integrality, policy endpoints/intervals: `invalid_catalog`, `invalid_request`, `invalid_policy` |
| 7 | Unknown policy is `facts_unavailable`, except current owner querying a protected_access_management/protected_recovery operation may use unknown policy. Unknown policy still has no payload |
| 8 | Resolve protected guard/source or ordinary precedence and compute complete source matches/explanations as below; project complete result B/I: `result_limit` |
| 9 | Materialize and return the preflighted owned result; no partial success |

An empty required numerator/denominator is `invalid_number`, not invalid_reference.
In stage 2, unknown-state payload violations and enum ranges precede reference
emptiness; the three listed structural categories are evaluated before that shared
reference check. Stage 4 checks endpoint presence/constraint mode but not numeric
order or value. Stage 6 checks all rational canonicality before any of its domain
errors. Unknown policy and complete empty policy remain distinct even when a later
default denial appears likely. Conflicting pairs on other operations are still
invalid. A malformed supplied entry cannot be hidden by a protected-owner permit;
use the allowed unknown-policy input when those unrelated editable facts are not
loaded. Omission does not allow an ordinary request to use partial policy.

The evaluator translates allocation failures, including `std::bad_alloc` and
`std::length_error` from its own bounded work/materialization, into allocation-free
`EffectivePermissionFailure{allocation_failure}`. Caller construction, an invalid
borrowed lifetime, concurrent mutation/data race, hardware exhaustion and process
termination are not retroactively made safe by this API. Checked arithmetic for
resource counts must precede allocation; overflow yields the relevant limits error.
No expected failure produces a partially owned permit or changes the input.

## 4. Decision and explanation mapping

Apply frozen logical §4 without quantity-based individual fallback. One ENTIRE
resource tuple selects the individual tier. Opposing individual scopes overlap
when each corresponding slot intersects (Any intersects every nonempty valid set);
zero-slot global tuples overlap. Do not enumerate Cartesian products, combine
slots from different entries, or choose a specificity hierarchy.

In valid individual input, all resource-selected entries have one effect. Complete
individual allow grants produce permit/individual_allow; choose one whole witness.
A full individual deny produces deny/individual_deny. A selected tier with no full
allow and no full explicit deny produces deny/individual_limit. Role matches are
suppressed, even when a role could fully allow. Outside individual scope, any full
role deny yields deny/role_deny; otherwise one full role allow yields permit/
role_allow; otherwise deny/default_deny. Zero and requested quantities preceding
actual cap-limited effects follow the same rules. No rounding, sum or saturation.

For a complete policy, helper_password_reset targeting the known current owner
returns deny/protected_owner_target, including when actor is that owner; its broad
editable grants cannot bypass the dedicated recovery workflow. For a current owner
querying protected_access_management/protected_recovery, return permit/
protected_owner regardless of editable denials. Other operations/actors follow
ordinary precedence. Known-absent owner grants no protected source. The permitted
unknown-policy exception in stage 7 is only for these protected owner permits.

Emit one source row per supplied policy entry, including other operations. Add one
virtual protected-owner row when protected permission or the owner-target refusal
is decisive: source = actual owner account, operation = requested operation, absent
grant/role, complete match, allow/deny effect respectively. No virtual row arises
from a role label. The result owns actor/snapshot/catalog revision, owner facts and
a copy of the request whose resource/quantity bindings are sorted by slot key.

For each policy row, other operation means `other_operation` and no failed slots.
For the requested operation, record every failing resource slot in sorted order;
if any fail, match is resource_mismatch and failed_quantities is empty. Otherwise
compare every quantity constraint, recording all failing quantity slots; match is
quantity_mismatch or complete. Canonical/domain validation has already covered all
rows even where matching does not need quantity comparisons.

Disposition follows these rules in order:

1. Other-operation/resource-mismatch rows are not_applicable.
2. Under a decisive protected source/guard, every remaining policy row is suppressed.
3. Under selected individual scope, every remaining role row is suppressed.
4. A remaining quantity-mismatch row is quantity_limit.
5. Every full deny in the selected tier is decisive_deny. Under role denial, full
   role allows are suppressed.
6. Of fully fitting allows when permitting, the stable first is permit_witness;
   other full allows are fitting_allow. The virtual protected permit is its witness;
   the virtual refusal is decisive_deny.

Sort all rows by `(kind, source, grant, role)`, enums in declaration order, optional
absent before present, references in unsigned-byte order. Unique source/grant rules
make this a total row order; declaration order/input order never breaks ties.
`witness_row` is the zero-based index of the sole permit_witness row, present only
for permits. Denials retain all decisive sources rather than inventing a grant or
choosing component witnesses. Failed-key lists name constraints, not localized text.
Exact bounds remain traceable via source/grant and immutable snapshot. The snapshot
and returned explanations never authorize later execution or public disclosure.

## 5. Exact resource and arithmetic accounting

B is semantic owned bytes and I is semantic container items, not sizeof, allocator
metadata/capacity or a claim about physical heap/RSS. Use checked unsigned sums:

- `B(string)=size`, `I(string)=0`; enum/bool/u8 contributes one B, u64 contributes
  eight B; scalar I=0. No hidden C-string terminator or container header charge.
- `B(optional<T>)=1 + (present ? B(T) : 0)`; I is contained I when present, else 0.
- `B(vector<T>)=sum B(element)`; `I(vector<T>)=size + sum I(element)`.
- B/I of a struct is the sum of its declared fields, in declaration order. Rational
  has two strings and no extra tags. The decision wrapper adds no fields/charge.

Stage 1 applies these formulas to EffectivePermissionInput, not merely the selected
operation or winning tuple. Every nonnumeric reference payload, including a present
optional, is <=reference_bytes. Each numerator and denominator string separately
is <=component_bytes. All losing sources, other operation definitions, allowed-set
members, explicit role memberships and optional domain/constraint endpoints count.
Require input B<=input_owned_bytes and I<=input_items. Limits themselves are not
part of input B/I. The borrowed preflight traverses existing strings/containers;
it must not first clone/sort/materialize oversized DTOs to discover their size.

Before constructing any output row, copied request or string/vector, project the
complete actual EffectivePermissionDecisionData B/I from borrowed inputs and the
computed classifications/failed-slot counts. Include the virtual row, duplicated
source/request identities, every failed-key string, optional witness's 1+8 bytes,
and owner facts. Require result B/I within both selected ceilings and each
destination container size within its representable max_size, including for
denials/protected outcomes. No truncation, omitted losing source or partial permit
is allowed. Scratch indices/counters may be used to project, but output DTO growth
waits for the complete checked projection. Input lifetime remains borrowed until
return. Any extra owning DTO copy has its own prior checked complete projection.

All limits must be positive, <=SIZE_MAX and <=PTRDIFF_MAX on the target. In addition
require checked representability of `8*component_bytes+1`,
`16*component_bytes + 8*input_items`, and input_items+16; otherwise invalid_limits.
These derive finite work bounds, not new value-domain restrictions. No public
4096-bit cutoff or 256-byte reference cap is borrowed from unrelated primitives.
A larger valid selected profile remains possible with checked larger work capacity.

After stage 1, at most input I borrowed-index records are needed, each charged as
one eight-byte index; sort/scan the borrowed data without cloning the whole policy.
Numeric work handles one rational/ratio comparison at a time, not a cache of every
converted input. A decimal magnitude of d digits is <2^(4d). Canonicality can use
bounded integer Euclid; each operand needs <=4D bits for D=component_bytes.
A full unreduced cross-product needs <=8D bits, 131072 at the default. Derive an
operation's smaller bound from its actual component lengths before constructing
its big integers/products. Count a minus in input bytes but never as a magnitude
digit; using the inclusive D bound is conservative.

For each comparison, handle sign/zero first, then cross-cancel common numerator
and common denominator factors before the necessary products. Compare exact
integers, never cpp_rational-to-float, final-effect conversion or rounded display.
Canonicality rejection must not silently repair an untrusted ratio. Require a
bounded strategy with at most 16 simultaneously owned integer work values, each
within the current <=8D-bit bound, including library arithmetic temporaries.
Semantic numeric payload reserve is therefore at most 16D bytes (262144 default),
plus <=8*input I index bytes. Actual backend limb capacity/metadata is additional
bounded implementation overhead to audit, not misreported B. No temporary may
exceed the projected bound before checking. Euclid on operands of at most G bits
has a conservative 2G+1-division ceiling; checked counters must not wrap. Finite
component/item bounds thus also bound the amount of work; no CPU deadline or
hardware benchmark is claimed. Production source/foundation review must verify
this bounded integer strategy against the actual selected Boost implementation.
No arithmetic or selected-library runtime evidence is created by this document.

## 6. Literal setups, size facts and future baseline reach

Every input type is directly aggregate-constructible. There is one feature entry;
no SUT parser, factory, grant certificate or previous successful evaluation is needed
in test setup. Rational spellings are literal input data, not SUT-generated oracles.
Start ordinary global examples with actor `A`, snapshot `S`, complete catalog
revision `C` containing operation `O` (ordinary, no slots, absent target), absent
owner, complete empty policy, and request `O` with no bindings.

| Independent literal | Required fact/result |
| --- | --- |
| That empty global setup | Input B=11, I=1; deny/default_deny result B=9, I=0, absent witness, no sources |
| Add one individual allow source `I`, grant `G`, operation `O`, empty complete tuples | Input B=18, I=2; permit/individual_allow result B=26, I=1, witness_row=0 and one nine-byte source row |
| Instead classify O protected_access_management, present owner `A`, unknown empty policy | Input B=12, I=1; permit/protected_owner result B=26, I=1 with one eight-byte virtual row and witness_row=0 |
| First setup input byte/item ceilings 11/1, result ceilings 9/1 | Exact byte boundaries fit; input byte ceiling 10 fails input_limit, result byte ceiling 8 fails result_limit; zero ceiling is invalid_limits |
| Ordinary allow setup result I ceiling 1 versus a second same-effect source row | One row fits; two rows require I=2, never omit one to retain permit |
| Rational numerator 1 followed by 5000 zeros, denominator 1 | B=5002, 5001 numerator bytes, magnitude 16610 bits; retains EP27 under sufficient complete B/I budgets |
| Maximum positive 16384-digit component (all nines) squared | 32768 decimal digits, 108853 bits, below the derived 131072-bit comparison capacity; no finalizer shortcut |

The following map covers every approved logical row; it groups setup changes, not
runtime cases or an artificial promised test count. All can directly call this one
entrypoint from raw literals, including errors and protected/unknown facts.

| Logical examples | Direct raw setup |
| --- | --- |
| EP01–10, D1-A/B, EP28–39 | Timer operation resource slot `timer`, quantity slot `adjustment`, unit `ms`; complete entries with the literal resources/intervals in each row; explicit role assignments; literal 300000/1, 600000/1, 300001/1; source effects/grant optionals distinguish allow/deny |
| EP11 | Add the declared second quantity slot `count`; each complete grant retains its own time/count constraints |
| EP12–14, EP27 | Literal 600001/2, -9223372036854775808/1, magnitude bound 9223372036854775808/1, 18446744073709551616/1, and the long canonical integer; catalog domains admit exactly the intended rational/integer values |
| EP15–19 | Present/absent owner identity and trusted catalog classification; helper target bound to owner versus another account; editable grant/deny facts remain separate |
| EP20–21 | Independent raw snapshots P1 and P2 with changed membership/grant or current owner; evaluate each anew, never feed a decision back as authority |
| EP22–23 | Raw source bytes 4f/80, NUL reference, empty required reference and literal 4096/4097-byte identities; permute vectors without changing order semantics |
| EP24–26 | Unknown tagged states, mismatched declared unit/domain tags, malformed shapes/identities/intervals, and result B/I one below complete projection |
| EP40–43 | Two declared resource slots `source`/`destination`; complete A/B and C/D versus explicit {A,C}/{B,D} sets, with no scope union |
| EP44–45 | Declared zero-resource count operation versus missing/extra/duplicate bindings in a nonempty shape |

Future meaningful resource cases must cover all six limit fields, exact/one-over
B/I and derived-size overflow, relevant and irrelevant invalid rows, staged error
priority, stable explanations, input destruction/mutation after return and the
protected unknown-policy exception. Independent std/selected-Boost foundation
facts must be reviewed before their first invocation; they do not call this module
or derive expected permission outcomes from it.

## 7. Minimal source and later gates

After actual API approval, a separately released minimal header/source may supply
only these declarations, inert aggregates, private owned-value construction,
getters and fixed exceptions. The feature's entire minimal body must be exactly:

```cpp
throw EffectivePermissionsNotImplemented{};
```

No validation, numeric work, source lookup, successful result or fallback comes
before that throw. The header needs no fake/test-only constructor for results.
Build/discovery may compile/link and list tests; it must never execute a feature or
marker during configuration/build/global initialization. Every literal raw setup
above reaches its own future stub directly. The marker is not a typed invalid-input
or successful-denial result and must not be accepted by tests as the required error.

Separate actual source/harness review, independent fixtures/test review, selected
environment foundation, and FIRST plain intended behavioral red precede production
ACCESS-002. This API performs no session authentication, clock/network/SQL/queue
work, policy editing, delivery or durable state write. Server adapters still load
coherent current facts, classify real operations, recheck sessions/rights/revisions
at execution/commit and apply workflow/CAS guards. Disclosure needs current rights.
No protected classification, owner/catalog/snapshot reference, raw aggregate or
immutable result is a bearer capability. Root retains shared build/CI/state/schema/
PR62 ownership and every subsequent stage release.
