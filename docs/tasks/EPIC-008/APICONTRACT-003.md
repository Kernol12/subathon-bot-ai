# APICONTRACT-003 — Guarded enrollment and recovery host contract

Parent: [EPIC-008](../EPIC-008.md). Kind: contract, document only.
Specification v0.45 §10, AUTH-01/02/08/10, SERVER-01, TEST-04/05.
Dependencies/status: progress/state.json. Planned prerequisite exposed by
[host-auth continuations](../../contracts/host-auth-continuations-v1.md) B3 review;
no contract, test or implementation approval is supplied by this brief.

Define the additive setup/reset host surface after APICONTRACT-002, importing the
frozen host/continuation/account-ingress/password/store contracts. API-013 alone
does not enable setup/reset. Specify named guarded forwarding of STORAGE-020
insert_account/replace_account/replace_local_credential without changed semantics;
bounded conditional directory-binding reads/writes through compiled abstract ports;
and the recovery-code/setup/reset selection and mutation ports required for one
actual caller-owned atomic transaction. Audit every needed operation rather than
leaving another hidden host prerequisite. ACCESS-001 owns actual indexed directory
facts and any separately reviewed logical/physical extension; ACCESS-005 owns
recovery policy. Missing producers/consumer registration refuse, not known absence.

Define compiled purpose/stage registrations, current selection/entitlement/source
checks and matching committed completion, same revocable UTC and host elapsed
sources, finite native-call counts (including recovery-code batches) and stage/
response/aggregate secret-copy budgets. Use the SAME installation password permit;
no second executor or out-of-process console password path. Explicitly reconcile
all selected edge deadlines and noncooperative native lifetime. Console setup/reset
codes preserve their specification purpose/lifetime; no password disclosure or
ownership/permission change follows. No request facade crosses a yield.

Use independent examples for initial-owner race/no existing owner/code replacement,
invitation, current credential/account/directory CAS, reset preserving protected
owner/permissions while revoking sessions/codes, missing producer, failed clocks,
queue/permit/resource refusal, disconnect/stop, committed/unknown/rollback outcomes
and secret cleanup. Obtain the complete independent contract review cycle before
APITEST-014. Coordinator owns shared schema/build/CI/state. No reverse dependency
on ACCESS-001/005 or SESSION-002/003; consumers depend on API-014. No code, route,
provider call, physical schema change or runtime capability is part of this task.

## Authored proposal

[Guarded enrollment and recovery host revision1](../../contracts/guarded-enrollment-recovery-v1.md)
defines the additive proposal and ER01–33 examples. Review/status evidence lives
in progress/state.json and the focused PR; authored examples are not executed tests.
