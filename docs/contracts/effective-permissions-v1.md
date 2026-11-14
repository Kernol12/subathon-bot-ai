# Effective permissions and complete constrained grants — proposal v1

ACCESSTEST-002 completed logical proposal, resumed from the partial document
`5dfc6a1e` at owner-decision revision `4429cbb8`. Authority: specification v0.45
§10/AUTH-03/AUTH-06 and
[ACCESS-002](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-006/ACCESS-002.md), with the
[common value contract](wire-primitives-v1.md) and
[application ownership boundary](event-state-boundaries-v1.md).
This defines proposed values, invariants and independent examples, not an API,
schema, evaluator, authentication adapter, permission editor or executable test.

The [recorded owner decision](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/accesstest-002-owner-decision.md) resolves
D1: complete resource scope selects individual overrides before quantity/other
limits. This actual revision still requires a different full logical review before
dependent API work. It supplies no runtime authorization or approval of later stages.

## 1. Authority and responsibility

Resolve granular operations on the server identically for desktop and browser.
An account/role name, open page, provider category, client request, revision token
or earlier preview supplies no authority. A pure calculation over supplied facts
does not authenticate the actor, prove those facts current, write policy/state,
grant a session or attest that an action was executed.

This slice owns precedence, complete-grant matching, protected ownership boundaries
and explanations of the actual permission sources used. It consumes a coherent
server-established account/policy snapshot and a fully typed operation intent.
[ACCESS-003](../tasks/EPIC-006/ACCESS-003.md) owns use-versus-delegate authority,
editing/invitation/self-affecting-change bounds and authorized automation. SESSION
and the account/setup/recovery tasks own identity, session validity, fresh sign-in,
owner transfer and credential recovery. Chat eligibility/categories have their
own configured authorization path; a public chat command need not be an application
account request. No Twitch category becomes an application role automatically.

The server must re-evaluate current rights at the actual execution boundary,
including queued/review work, and enforce the owning operation's other guards.
This is a product requirement, consistent with [OWASP's per-request and default-deny
guidance](https://cheatsheetseries.owasp.org/cheatsheets/Authorization_Cheat_Sheet.html#validate-the-permissions-on-every-request),
not a claim that this document implements or certifies a security boundary.

## 2. Logical facts and identity

Names here describe logical owned values; they are not C++ declarations or wire
member names. No public constructor, caller-validity certificate or transport
representation is selected at this stage.

| Value | Meaning and source |
| --- | --- |
| Actor reference | Exact nonempty internal account identity selected by the authenticated server path, not a display name or caller-selected identity |
| Current owner | Known absent during pre-setup, or one exact account reference; supplied from authoritative ownership state separately from editable roles |
| Policy snapshot reference | Exact opaque identity of the coherent evaluated facts; records what was read, never proves currentness or permission |
| Assigned role reference | Exact identity of a role actually assigned to the actor in that snapshot; no hierarchy, label or implied preset powers |
| Permission operation key | Exact identity of one granular operation in the owning server operation catalog; distinct from an invocation/deduplication reference |
| Source reference | Stable identity of the individual policy entry or role policy entry that supplied a decision |
| Grant reference | Stable identity of one complete constrained allow tuple within that source |
| Typed operation intent | Operation key, complete resource bindings and requested quantities, interpreted by its owning operation definition |

All identity fields compare exact bytes, including NUL and non-UTF-8. Empty required
references are invalid, but the one-byte reference 00 is not empty. Ordering, when
needed for reproducible explanations, is unsigned-byte lexicographic with shorter
exact prefixes first. No UUID/provider/ASCII/case-folding registry is inferred.
Display labels are separate localized presentation data and never select a grant.
Any future external mapping uses each owning field's declared common primitive and
limits; the common 256-byte correlation cap is not a domain-reference cap.

The authoritative loader must resolve the actor's actual role memberships and all
relevant individual/role policy entries. A known empty collection differs from a
failed/partial lookup. Unavailable or unknown ownership/policy facts are not empty
roles, no owner, successful verification, or default current values. Fail closed
with a distinct unavailable/invalid-input result rather than fabricate a decision.
The model cannot retroactively bound allocations already made by a caller/loader.

## 3. Complete grants and exact quantities

An allow source contains independently identified complete grants. A grant is one
conjunction of its operation/resource/quantity constraints, never a collection of
pieces that may be mixed with another grant. Alternatives are separate grants.
For a request to be permitted, one complete grant in the selected permission tier
must contain the ENTIRE requested intent. Having different witnesses for different
constraints is insufficient, even when all witnesses are from one role/source.

The owning operation definition declares the resource and quantity dimensions and
their domains/units. Example dimensions are timer reference plus adjustment in
milliseconds, or scene reference for publication. The declaration is trusted server
operation metadata, not a client flag, arbitrary property name or editable role's
claim about what an operation means. Unknown dimensions/units cannot be ignored.

Proposed matching vocabulary:

- A resource constraint is explicit Any or a nonempty finite set of exact allowed
  resource references. Every bound resource in the intent must fit the SAME grant.
  There is no prefix/path/wildcard-string matching. A global operation has its
  declared empty resource shape; an omitted required binding is not global access.
- A quantity constraint is explicit Any, AtMost(bound), ClosedInterval(lower,upper),
  or MagnitudeAtMost(nonnegative_bound), as admitted by the owning operation.
  Bounds are exact and comparisons inclusive; intervals may have signed endpoints
  where that dimension permits them. Each dimension has its declared unit. No
  seconds/milliseconds, points/micro-points or count/fraction conversion is guessed.
  Missing/unknown constraint data is not Any; there is no numeric infinity sentinel.
- The later reviewed concrete representation must make every declared dimension's
  constraint explicit. Incomplete tuples, wrong units, inverted intervals and
  unsupported constraint forms are invalid input, never partially applied grants.
  Resource scope selects the individual tier separately from quantity matching,
  as defined below and in §4.

The logical resource shape is a finite tuple of named slots declared by the
operation. Each request binds exactly one nonempty opaque resource reference per
slot; every grant declares Any or one nonempty allowed-reference set for each
slot. Slot keys are opaque identities, not positional input order. Missing, extra
or duplicate slots and duplicate members of an allowed set are invalid. A global
operation explicitly declares zero resource slots; its complete empty tuple matches
that operation, not an omitted binding for a resource-bearing operation. One atomic
operation can declare several slots, including several of the same resource kind;
they must all fit one grant. This is a finite logical representation choice, not a
new batch, transfer or multi-timer product capability. An owning operation must
explicitly define its complete obligations before using this model.

Similarly, every declared quantity slot has one exact requested value, a declared
unit/domain and one explicit constraint in each complete policy tuple. A quantity
applies with the meaning given by that operation; it is not implicitly multiplied,
split or summed across resource slots. Duplicate/extra/missing quantity slots are
invalid. These quantity constraints are the nonresource limits in this vocabulary;
there are no hidden predicates, cumulative budgets or arbitrary script conditions.
An operation requiring additional limit types needs their reviewed typed contract,
not an ignored condition or a change to the resource-first tier rule.

An allow entry contains one or more complete grant alternatives. A deny entry has
one complete resource/quantity tuple; disjoint denial alternatives are separate
identified entries. A denial may use explicit Any for every quantity, as in an
unconditional deny. Otherwise its full quantity constraints determine whether it
is an explicit matching denial. Missing bounds never mean an unconditional deny.
Both effects use the same resource and exact quantity vocabulary, while individual
tier selection intentionally considers only operation plus resource scope (§4).

Quantity values and bounds use exact reduced rationals with positive denominator,
the common Rational meaning and finite component budgets. Integers are denominator
1; zero is 0/1. The operation owns sign/integer-only restrictions. Do not impose a
new positive-target/origin rule, Signed64 converter, decimal parser cap or universal
4096-bit ceiling. Preserve already-valid long Exact values from their owning domain.
No float, display rounding, truncation, saturation or final-effect rounding occurs
in permission matching. A noncanonical untrusted value is rejected, not repaired.

Where a signed64 adjustment is a valid owning input, its minimum has exact magnitude
9223372036854775808; do not call overflowing signed abs. Comparisons may use selected
Boost exact arithmetic, with checked finite work/temporary growth before arithmetic.
Cross-cancel or compare bounded integer ratios without constructing an avoidably
huge product. The concrete API must pin complete arithmetic/preallocation checks.

Check the requested operation, not a smaller eventual cap-limited actual effect.
For example, requesting +600000ms against a +300000ms permission remains outside
that grant even if timer capacity would limit actual credit to +300000ms. A zero
change still requests the operation and does not acquire a permission exemption.

Atomic operations involving several resources/quantities require one whole witness
covering that operation. This model does not split a request to manufacture authority
or decide delegation/automation/batch semantics. The owning operation must explicitly
identify its permission obligations; separate legitimate operations do not combine
their limits into a larger grant.

## 4. Resource-first individual precedence

The [owner decision](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/accesstest-002-owner-decision.md), now reflected in
§10/AUTH-06, chooses resource-scoped individual overrides. The previous partial
review remains valid for unchanged requirements; its pending D1 table is superseded
by this section and the fixed D1-A/B outcomes in §8.

For each complete policy tuple, **resource match** means the operation key matches
and every requested resource binding fits that tuple's corresponding Any/set
constraint. Check the whole resource tuple without consulting quantity limits.
Different grants cannot contribute different resource slots to this test. **Full
match** additionally requires every quantity constraint of that same tuple to fit.
No implicit specificity, priority by input order, smaller resource set, narrower
interval, role label or larger limit is part of either match.

For one ordinary operation, evaluate the coherent valid input as follows:

1. Collect individual tuples whose whole resource scope matches. If any exists,
   select the individual tier, including when all their quantity limits fail.
2. Valid selected individual tuples have one effect. For deny entries, a full
   match yields explicit individual denial; without a full match, deny for individual
   limits/no complete individual allow. For allow entries, any fully matching whole
   grant permits with one stable witness; otherwise deny for individual limits/no
   complete individual allow. Mixed opposing effects are invalid as described below,
   not resolved by a new same-tier priority. None of these outcomes falls back to roles.
3. If no individual whole resource scope matches, fully matching role deny entries
   defeat role allows. Otherwise any fully matching role allow grant permits with
   one whole stable witness. Otherwise default deny.

A constrained individual deny that resource-matches but fails its quantity limits
still selects the individual tier: it is not a matching explicit denial, but there
is no implicit allowance and no role fallback. The reason is individual limits/no
complete allow. A constrained role deny that fails a quantity limit is not a full
matching denial; a different complete role allow can still permit. Thus an explicit
unconditional individual deny blocks its resource scope, while a conditional deny
never manufactures a positive individual allowance outside its condition.

Same-effect individual tuples may overlap. For allow alternatives, one complete
fitting grant suffices; a smaller/narrower alternative does not subtract authority
from another.
A broad individual allow plus a narrower individual deny would introduce an
unagreed specificity rule. Instead reject contradictory supplied individual input:
for the same operation, an allow grant's resource scope and a deny entry's resource
scope must not overlap, irrespective of their quantity limits. Resource scopes
overlap exactly when, in every declared resource slot, their allowed sets intersect
or Any permits an intersection; the empty global tuple overlaps itself. This check
uses the complete scopes, without materializing their Cartesian product. Opposing
effects in disjoint resource scopes are valid. Any supplied conflicting pair is
invalid even when the current request lies elsewhere or their quantity intervals
are disjoint. ACCESS-003 must prevent/resolve such edited facts; this model neither
chooses a winner nor silently drops an entry. Role allow/deny overlap is valid and
uses the established complete-deny precedence above.

All supplied shapes and bounds must be valid, including losing/off-scope entries.
Do not hide invalid input behind an early permit/denial or treat a resource limit
failure as an absent override. Exact duplicate/conflicting identities also remain
invalid input. Only a fully validated, completely evaluated input yields a decision.
The later API must freeze error priority and bounded validation work, without
changing these outcomes.

For example, individual A/max300000ms plus role any-timer/max600000ms denies
A/+600000, allows A/+300000 through the individual grant, and allows B/+600000
through the role. These are per-request quantities, not a cumulative allowance.
There is no requirement conflict or remaining D1 policy choice in this revision.

## 5. Protected owner is separate from editable roles

Exactly the authoritative current-owner identity carries the non-removable
access-management/recovery capability promised by AUTH-03. Normal role edits,
role labels, invitations or individual overrides cannot create, transfer or remove
ownership, or lock the owner out of those protected capabilities. This is a narrow
protected source, not permission to do every timer/layout/integration operation.
For ordinary operations the same individual/role/default-deny rules still apply.
For a query specifically about a protected capability, trusted current ownership
and operation classification are sufficient to establish that narrow source;
unrelated editable policy need not be loaded to invent a dependency on its grants.
This is not permission to evaluate ordinary operations from a partial policy set.

The server operation catalog must distinguish protected access-management/recovery
capabilities from ordinary operations. This is not a caller-supplied is_owner or
protected-operation flag. The exact product operation catalog/mapping is a required
account/delegation/recovery integration gate, not invented by granting a role named
Administrator or Owner. The explanation identifies protected ownership separately
from any editable grant that would otherwise deny it.

Permission is only one required check. Sensitive ownership/login-method/recovery
changes still require the specification's fresh authentication and workflows.
Owner transfer needs the actual current owner's fresh authentication and recipient
acceptance with fresh authentication/local credentials; no role or evaluated fact
performs that transfer. A server-console recovery path for the existing owner is
separate from an authenticated application request and preserves ownership.

An ordinary helper-password-reset route cannot reset the current owner and thereby
give an administrator control of ownership. Such a target requires the dedicated
owner-recovery workflow; broad editable access-management/reset permissions cannot
remove that intrinsic target guard. The protected owner also cannot turn a helper
reset request into a substitute for the owning recovery workflow. This model
creates no reset token, credential change, console command or fresh-auth proof.

## 6. Explanations, freshness and unavailable outcomes

A successful logical evaluation owns a permit/deny decision, a stable reason and
the exact evaluated actor/operation/snapshot identities. Reasons distinguish
individual allow/explicit deny, individual limits/no complete individual allow,
role deny/allow, no complete role allow/default deny, protected owner capability and protected-owner workflow/target refusal. Resource/invalid/
unavailable evaluation failures are separate from a fully evaluated denial.
No failed/partial evaluation returns a permit with an omitted explanation.

Retain source rows identifying role binding/role entry or individual entry and
complete grant reference, plus whether that source won, lost to precedence,
matched resource scope, matched completely, or failed a named constraint. The
explanation distinguishes operation/resource mismatch from quantity failure, records
which individual scopes selected the tier, and identifies role sources suppressed
by that tier. It preserves all decisive individual/role denials, not only the stable
first one. No localized sentence or human-readable role label is an authority token.
Where several grants fully fit,
retain their identities and choose one stable witness; never choose independent
smallest constraint pieces. Proposed total source order is kind (protected owner,
individual, role), source reference, grant reference, role reference. Optional
references sort absent before present; byte fields use unsigned lexicographic
order. The smallest fitting complete allow tuple in the selected tier is the permit
witness. Multiple decisive denials remain visible in stable source order. Caller input
order cannot change the decision, winning witness or explanation order.

Explanations are private permission facts. UI disclosure, inspection of another
account, audit access and transport projection need their own CURRENT rights.
The model does not expose credentials/session tokens or decide a public error's
permitted detail. Source/revision references are evidence identities, not bearer
capabilities. Original facts may be retained immutably for audit/preview, but their
permit result must not be cached as permission to execute later work.

At execution the authoritative server reloads/rechecks the actor's current session,
owner/membership/override facts and the operation's required workflow/authority.
It checks relevant revisions together with the actual command/state transition.
A revocation between preview and commit cannot be rescued by the earlier snapshot,
open page, captured application plan, deduplication key or client-supplied permit.
The application [boundary](event-state-boundaries-v1.md) already requires this
revalidation. Actual queue/transaction/retry/disclosure integration remains required.

## 7. Proposed finite local resource profile

These are engineering defaults for one evaluation, not universal transport,
account-installation or stored-domain caps. Every selected profile must be positive,
finite, representable and compatible with its owning value domains. A larger valid
stored reference/component needs an explicit sufficiently large selected profile,
not truncation, loss of an override or a default-deny substitution.

| Resource | Proposed default and rationale |
| --- | --- |
| Each decoded domain/source/grant/revision reference | 4096 bytes, configurable; preserves existing STORAGE-015's default, unlike common correlation's unrelated 256 |
| Each canonical numerator/denominator component | 16384 decimal bytes, configurable; matches existing retained Exact component default, no added smaller bit/final-effect bound |
| Total policy/resource/quantity/source container entries | 4096, configurable; all explicit memberships, policy/grant entries and scope/constraint elements count, including sources that lose precedence |
| Complete owning input | 16MiB, configurable; includes all supplied reference/component bytes and scalar/container values, not only winning grants |
| Complete owning result and explanations | 32MiB and 4096 entries, configurable; checked for the actual complete explanation before copying/growing, no successful truncation |

For the default component profile, each exact ratio comparison may preflight two
cross-products with a conservative bound of 131072 magnitude bits each: at most
32768 combined decimal bytes, with four bits per decimal byte as a safe upper
bound. This is derived scratch capacity, not a new 4096-bit/final-value restriction.
Changing the component profile requires deriving a sufficient checked work bound,
not retaining a smaller hardcoded scratch ceiling. At most two interval-endpoint
comparisons per supplied quantity constraint are needed; no Cartesian explanation
trace or unbounded recursive policy evaluation is part of this proposal.

These bound one already-resolved actor/operation query. They do not permit omitting
unloaded denials or part of a full grant to fit a profile. Caller-owned allocations
remain caller responsibility. The concrete API must freeze exact byte/item formulas,
overflow/duplicate/shape validation order, numeric work limits and before-copy/
before-arithmetic projection before any code/test baseline. No recursive policy
language, provider lookup, role hierarchy or unbounded explanation trace is added.
Existing std/selected Boost suffice; no new dependency is proposed.

## 8. Independent literal examples

I denotes individual policy, R a genuinely assigned role, G a complete grant.
Amounts are exact milliseconds unless stated. Every unqualified denial is an
unconditional applicable deny for the whole selected operation/resource shape.
Resource names in multi-slot examples are illustrative declared slot identities;
they do not introduce a new supported operation.
The examples neither generate expected results from an evaluator nor claim execution.

| ID | Supplied facts / request | Required result or boundary |
| --- | --- | --- |
| EP01 | No I; known empty assigned roles; request timer A/+0 | Default deny; zero does not avoid operation authorization |
| EP02 | R1/G1 allows timer A/+0..300000; request A/+300000 | Role allow, witness R1/G1; inclusive exact limit |
| EP03 | Same only G1; request A/+600000; actual timer cap would yield 300000 | Deny: request does not fit, irrespective of actual cap-limited effect |
| EP04 | R1/G1 A/max300000 and R2/G2 B/max600000; request A/+600000 | Deny; never merge A from G1 with G2's magnitude |
| EP05 | Same sources; request B/+600000 | Allow R2/G2 as one complete witness |
| EP06 | R1 allows A/+300000; R2 explicitly denies this operation; no I | Role deny wins, independent of ordering or labels |
| EP07 | I unconditionally allows this operation; roles deny | Individual allow wins; no narrower/mismatching I constraint in this example |
| EP08 | I unconditionally denies this operation; roles allow | Individual deny wins |
| EP09 | I is for scene.publish; R1/G1 permits timer A/+300000; request A/+300000 | I's different operation does not override this timer operation; role allows |
| EP10 | One role source has separate G1 A/max300000 and G2 B/max600000 | EP04/05 still hold; common source identity does not permit tuple union |
| EP11 | A declared illustrative two-quantity intent needs time 600000 and count 10; G1 allows time 300000/count 10 and G2 time 600000/count 1 | Deny: neither complete tuple fits. This is a matching example, not a newly supported product operation |
| EP12 | Only timer-A grant max300000; exact request 600001/2 | Deny: 300000.5 exceeds limit; no permission rounding to 300000 |
| EP13 | A declared signed correction has request −9223372036854775808 and max magnitude 9223372036854775808 | Exact bound fits; magnitude 9223372036854775807 would not. Business/correction permission prerequisites remain separate |
| EP14 | A quantity domain admits Exact 2^64 and the complete bound is 18446744073709551616/1 | Exact equality fits within the selected profile; no Signed64/Unsigned64 narrowing or general promise that every count/time operation accepts it |
| EP15 | Actor equals actual current owner, no editable grants, asks protected access-management capability | Protected-owner source retained; owning sensitive/delegation workflow checks still required |
| EP16 | Same owner with no ordinary timer grant asks timer A/+300000 | Default deny; no unrestricted owner bypass |
| EP17 | Non-owner's role display label is Owner or Administrator, but no applicable entry | Default deny; labels do not confer ownership or operations |
| EP18 | Current owner has editable role/individual deny on protected access-management capability | Protected capability retained outside editable policy; not permission to bypass fresh authentication/workflow |
| EP19 | Ordinary administrator's broad helper-reset permission targets current owner | Protected-owner target/workflow refusal; no credential/ownership takeover via helper recovery |
| EP20 | Preview permits R1/G1 at snapshot P1; role/grant revoked before queued execution P2 | Current decision denies; immutable P1 is history, not authority |
| EP21 | Ownership transfers between preview and execution; former owner has no remaining editable access-management grant | Old protected source no longer applies; evaluate new owner/current policy, not captured owner flag |
| EP22 | Two fitting grant source identities are raw bytes 80 and 4f; caller permutes their order | Same allow; 4f is the stable witness under unsigned-byte order |
| EP23 | Required reference is raw 00 versus empty; a 4096-byte reference under default 4096 versus one byte over | NUL identity valid, empty invalid; exact limit fits, excess is resource failure unless a larger valid profile is selected |
| EP24 | Policy/owner lookup unknown or partial; a constraint has unknown unit/type | Unavailable/invalid or unsupported input, no invented empty/Any/permit |
| EP25 | Missing required resource binding, inverted interval, duplicate conflicting source facts or incomplete allow tuple | Invalid input; do not drop the failing constraint/denial or use first/last input row |
| EP26 | A permit can be calculated but its complete required source explanation exceeds selected result budget | Resource failure before oversized output ownership; no partial permit/explanation |
| EP27 | An explicitly declared Exact dimension has numerator 1 followed by 5000 zeros, denominator 1, and an equal complete bound | 5001 component bytes fit the default 16384; its 16610 magnitude bits are not rejected by a borrowed 4096-bit finalizer limit. No domain is silently widened to accept this value |
| D1-A | I A/max300000, role A/max600000; request A/+600000 | Deny for individual limits/no complete individual allow; A scope selects I before quantity, with no role fallback |
| D1-B | Same I and a role B/max600000; request B/+600000 | Role allow; I's complete resource scope does not match B |
| EP28 | I A/max300000, role denies timer adjustment; request A/+300000 | Individual allow at the inclusive boundary; role denial loses to selected I |
| EP29 | I A/max300000, role any timer/max600000; request A/+300001 | Individual-limit denial; the one-millisecond excess cannot use role fallback |
| EP30 | I denies A only for quantity interval [0,300000]; role allows A/max600000; request A/+300000 | Explicit individual deny; complete denial tuple matches |
| EP31 | Same entries as EP30; request A/+600000 | Individual-limit/no-complete-allow denial; I's A scope still selects the tier although its deny interval fails |
| EP32 | Same individual deny as EP30; role allows B/max600000; request B/+600000 | Role allow; an individual denial on A does not suppress B |
| EP33 | No I; R1 denies A only for [0,300000]; R2 allows A/max600000; request A/+600000 | Role allow; the constrained role denial does not fully match |
| EP34 | Same roles as EP33; request A/+300000 | Role deny at the inclusive endpoint, despite the complete role allow |
| EP35 | I/G1 A/max300000 and I/G2 A/max600000; request A/+600000 | Individual allow through G2; same-effect alternatives overlap without a narrower-grant priority rule |
| EP36 | I/G1 Any/max600000 and I/G2 A/max300000; request A/+600000 | Individual allow through G1; the narrower resource alternative does not replace the broader complete allow |
| EP37 | I allows Any/max600000 and I denies A; request A/+1 | Invalid contradictory individual input; do not invent narrow-deny specificity or same-tier deny precedence |
| EP38 | I allows A/[0,300000] and I denies A/[300001,600000]; request B/+1 | Invalid contradictory individual input despite disjoint quantity intervals and current off-scope request; opposing resource scopes overlap |
| EP39 | I allows A/max300000 and I denies B; roles allow both; requests A/+300000 and B/+1 separately | Valid disjoint individual scopes; first permits through I, second explicitly denies through I |
| EP40 | Illustrative two-resource operation: I/G1 source A, destination B, max300000; I/G2 source C, destination D, max600000; role G3 source A, destination D, max600000; request source A, destination D, +600000 | Role G3 allows: neither individual resource tuple matches; combining I's source A and destination D must not select the individual tier |
| EP41 | Same I/G1 and I/G2 as EP40; role allows source A, destination B, max600000; request source A, destination B, +600000 | Individual-limit denial via G1's whole scope; G2's larger quantity and the role cannot rescue it |
| EP42 | One I grant has source set {A,C}, destination set {B,D}, max300000; role allows source A, destination D, max600000; request source A, destination D, +600000 | Individual-limit denial: this one explicitly complete resource conjunction covers the request; no cross-grant union is involved |
| EP43 | No I; role G1 source A, destination B, max300000 and role G2 source C, destination D, max600000; request source A, destination D, +600000 | Default deny; distinct complete resource tuples cannot supply a combined role witness |
| EP44 | An illustrative global operation declares no resource slots and an exact count quantity; I allows max5 and role allows max10; request has the declared empty resource tuple and count 10 | Individual-limit denial; a global tuple selects its I scope without inventing a resource or bypassing limits |
| EP45 | Request omits one declared resource slot, repeats a slot, or supplies an extra resource/quantity slot; another tuple would otherwise deny | Invalid shape, rather than an ordinary denial or a partially matched tuple |

## 9. Required next gates

D1 is recorded and its dependent applicability/deny/shape examples are complete.
Obtain a different actual full logical review of this revision, preserving the prior
partial review for unchanged EP01–27 and boundaries. No current document-stage
approval or executable test expectation is implied. A separately reviewed concrete
core/server API, inert source/harness, independent fixtures/tests, selected environment foundation
and FIRST intended behavioral red precede production ACCESS-002. Root owns those
stage releases, shared state/HTML/maps/build/CI/schema and publication.

ACCESS-003 delegation/automation/editing, actual account/session/current-owner
loading, protected capability catalog/recovery/transfer, authoritative revision/
execution-time checks, audit/disclosure, all transports/chat and release/platform
verification retain their required owning tasks. No network/provider/authentication,
credential operation, client-only enforcement or successful runtime behavior follows
from these proposed facts and examples.
