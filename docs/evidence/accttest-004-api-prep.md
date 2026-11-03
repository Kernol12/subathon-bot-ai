# ACCTTEST-004 resumption — read-only concrete API preparation

2026-11-03, root integrator. Preparation reports from bundle_implementer,
bundle_reviewer and bundle_validation concern actual integrated main
efe1c7e87ae2a7e8a92c3f5e7ede88e68e72b089 plus task claim5236d88.
No service API/codec/source, fixture, test, behavior or red was executed. This is
neither contract approval nor a prospective source review.

Authority remains reviewed logical contract6d0cd67f/ADR0003e971a47e. Actual
STORAGE016/017 prerequisites are integrated; all10 postmain workflows passed.

Concrete refinements passed to the author:

- Complete command equality must include every stable logical member and preserve
  absence versus empty DATA, distinct from physical expected-head intent. Freeze
  independent literal bytes. A healthy unknown old envelope format is unsupported,
  not fabricated corruption; known malformed current data has a different outcome.
- Current execution access and actual recorded-source disclosure precede historic
  result publication. Early original claims and ALL active subject holds precede
  a new proposal; compare all15 current contribution fields and actual plan head.
- CORE005's actual Main addition composes with CORE002 on the ORIGINAL
  clock-advanced TimerState. Do not preassign Main_after and then add again. Debt
  updates separately; zero addition cannot rescue confirmation wait.
- read_inputs(stale_plan) has no actual-head payload. Missing counter keys cannot
  be created by key-preserving clock/bundle storage. A whole-input overflow hold
  references the persisted head, never an unretained clock-only before token.
- Trusted policy/action/reference ports cannot commit hidden SQL changes inside a
  required read-only normal refusal. Current binding/authority comes from the
  trusted typed port contract, not nonempty serialized references or booleans.

## Existing real writer boundary and AS15

bundle_validation inspected `server/include/subathon/server/sqlite_writer.hpp`
and `server/src/sqlite_writer.cpp`: Context.begin and Session.commit enforce at
most one BEGIN attempt and terminal session per actual work. A real completion
value exists ONLY after acknowledged COMMIT, normal body return and error none.
Observing that completion therefore establishes at least one real BEGIN/COMMIT;
the protocol establishes at most one. Count the actual enclosing work/body/
completion once; no raw service SQL or synthetic success WorkResult is needed.
Read-only denial/conflict/duplicate units can acknowledge a real COMMIT too.

AS15 additionally needs an exception AFTER acknowledged COMMIT but BEFORE normal
work-body return. If the service exposes only opaque submit, current public/test
seams cannot inject that body tail or inspect its internal admitted work count.
A bounded trusted body entry/callable inside one real outer enqueue_work permits
the independent test to invoke it then throw before outer return. The synchronous
body value remains provisional; only actual normal WorkCompletion may publish it.
Alternatively a separate minimal fault seam would require actual review. The
concrete document must resolve this choice before its minimal header is authored.

`server/include/subathon/server/testing/sqlite_writer_work_faults.hpp` supplies
lose_next_commit_ack and lose_next_rollback_ack while owner-idle. Those real SQL
operations occur before acknowledgement loss and yield UNKNOWN/fail-closed, not
AS15's committed+work_exception. Ack loss cannot substitute for that required case.
Existing writer work tests cover both cases independently, revoked/foreign tickets,
throwing completion and owned lifetime; hold/original-claim tests supply real full
snapshot, late failure and inside-catch lock-release patterns for later fixtures.

No new storage decomposition is presently required by these inspections. Real
authority/policy/action/clock producers, mixed/finished/failed-capture/refund/batch/
continuation semantics and full release remain their explicit mandatory gates.
The author is writing only the concrete API document and own note; the different
reviewer awaits an actual revision. Minimal API/stubs and later fixtures/tests/
their independent review/selected foundation CI/FIRST intended red still follow.

Typed outcome details: bundle stage supplies existing full data only for historical
already_present/subject_claimed/source_claimed; staged may need typed lookup for
full payload. Other healthy conflict/stale outcomes have none. Early original
claim Match grants no disclosure/durable authority. Hold mutation results carry
historical produced versions, not current active certificates; current gate uses
complete active(subject). Known storage rollback cannot become normal Held success;
original uncertainty/capability/protocol remains distinct. Publication checks actual
transaction/error/value independently; committed+work_exception has durable rows
but no payload. Completion runs after capability revocation/release and contains
publisher exceptions. Concrete document still needs owned typed QVariant carriage
and checked extraction plus body/completion responsibility.
