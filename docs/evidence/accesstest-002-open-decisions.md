# ACCESSTEST-002 applicability decision — resolved

D1 is resolved by the owner's explicit acceptance on 2026-11-04 of the
resource-scoped recommendation. The current authority is the
[owner decision](accesstest-002-owner-decision.md) and specification §10/AUTH-06.
Individual resource scope selects the tier before quantity/other limits; a failed
individual limit cannot fall back to roles. Resources outside that scope retain
normal role deny/allow handling. The revised logical contract and examples require
actual independent review before API/tests/code stages.

## Historical question and alternatives

The following was the original unresolved analysis, retained as history. Its
pending-answer statements describe that earlier checkpoint, not current status.

# ACCESSTEST-002 open applicability decision

Coordinator /root,2026-11-04. Actual logical author identified this specification
boundary before freezing any API/test/implementation. Scope authorityv0.45§10:
individual explicit allow/deny precedes role deny/allow; each request must fit one
complete applicable constrainedgrant. The text does not fully define applicability
versus override scope for a narrow individual allow over a broader role grant.

D1: role permits adding10minutes to timerA; individualallow permits5minutes onA.
For a10minute request, possible policies are:

1. Resource-scope override: individual override forA suppresses role grants forA
   before magnitude/other allowed-limit checks, soA10 isdenied; rolesforother timers
   remain available. Scope/constraint separation needs precise owning definitions.
2. Operation override: any individual decision for that operation replaces its
   role grants, even for other timers. Only complete individual allows authorize it.
3. Complete-request matchingfirst: individualA5 doesnot matchA10, so roleA10 may
   allow it. Denies/overrides likewise need explicit complete applicability rules.

These differ in actualauthority and cannot be silently selected as a representation
detail. Root asked the owner asynchronously with these3choices (resource-scope
recommended); NO answer is observed. A preselected option or elapsedtime is not an
answer. Do not freeze dependent examples/API/tests or produce wider fallback while
pending. Continue unambiguous precedence/nonunion/owner/source explanation and
resource-budget work. Constraineddeny implications must follow the actual chosen
scope semantics rather than a guessed operation-wide interpretation.

Existing settled completegrant/no crossgrantconstraintunion/role-deny/defaultdeny/
protectedowner/currentexecution requirements are NOT reopened. Ownership transfer,
freshauth/session/account workflows anddelegationedits retain their owningtasks.
This is a genuine product decision, not routinepermission towork; root continues
independent scene/password tasks. No new auth/API/test/code/runtime exists here.
