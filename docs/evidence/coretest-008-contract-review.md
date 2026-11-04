# CORETEST-008 independent document review

Reviewer: service_reviewer, different from author codec_author, 2026-11-04.
Isolated task/CORETEST-008-contract-review from coordinator claimc311f0b and
integrated main0b631dc. Actual author5446a91 was independently inspected after
import into review7976c1c; no prospective verdict was issued.

**APPROVED — bounded document stage with explicit unanswered decisions.**

| Actual artifact | Reviewed Git blob |
| --- | --- |
| [Timer editorial commands](../contracts/timer-editorial-commands-v1.md) | 0ed9c69322beac2d5cce1ebba88b4a23891d7ad3 |
| [Author handoff](coretest-008-contract-author.md) | b38bc1ad0b40accd99fec85683acde59bc0a4e05 |

Authority inspected: specification v0.45 sections3.5/4/TIMER-01/NUM-02–04,
[CORE-008 brief](../tasks/EPIC-001/CORE-008.md),
[CORE-002 lifecycle](../contracts/timer-lifecycle-v1.md),
[CORE-005 debt](../contracts/time-debt-v1.md),
[time-credit revision1](../contracts/time-credit-v0.md),
[event/state boundaries](../contracts/event-state-boundaries-v1.md), and the
CORE-007/API-007 adjacent lifecycle/reopen/edit obligations.

## Actual reviewed scope

Read the complete proposal and all16 literal rows. Signed Main-time delta, cap
replacement and deadline replacement are distinct commands. Absolute Main during
start/reopen and separate debt correction cannot be silently encoded by one of
those deltas. The proposal preserves debt, retained Grace, independent pauses/
recovery, full requested/actual/capped/unapplied provenance and immutable history.
Current caller access DATA and expected/current revision comparison supply no
persistence authority, authentication proof, idempotency or successful durable CAS.

Existing CORE-002 observation runs under the actual earlier configuration before
an already-started timer's edit. Thus disabling a deadline at its reached instant
cannot portray uninterrupted Active time or implicitly reopen. Future configuration
is not applied retroactively to elapsed segments. No UTC subtraction or invented
restart interval supplies monotonic progression. A refused pure proposal does not
claim an independently committed clock mutation.

Positive signed deltas reuse CORE-005 debt-first/cap arithmetic and pass only actual
Main credit to CORE-002's current timer. Debt-only actual0 cannot rescue Grace or
confirmation wait. Negative deltas retain the explicit policy and complete signed
request, including INT64_MIN, and compose exhaustion with the already reviewed
zero-elapsed ending reducer. Manual identity remains distinct from ACCT-006
financial entitlement/original-effect allocation. RequiresReview remains timer-only
DATA; the document chooses neither durable/global status nor mixed deferral versus
a complete operation containing a held timer. No numeric algorithm is duplicated.

Examples ED01–04 preserve debt/cap/Grace/wait arithmetic; ED05–10 preserve signed
component identities and exact representability boundaries; ED11 separates elapsed
countdown from editorial credit; ED12/13 show settled settings preservation;
ED15/16 preserve revision/current-access refusal. ED05/06/08 deliberately still
require the selected existing ending policy; they are numeric constraints rather
than complete literal lifecycle-command results. ED09 also requires a valid
still-Active observation under the row preconditions, not an invented Ready/start
or Immediate-zero admission. Concrete executable cases must supply those actual
state/configuration inputs. ED14 explicitly fixes only existing hard-end observation,
not subsequent Finished settings-edit admission or a partial server commit policy.
No row claims an executed test or a finalized C++/wire/error contract.

## Unanswered decisions remain required

The actual document correctly retains all three genuine gates without a default:

- OPEN-CAP: acceptance/rejection/audited reduction when a cap edit is below Main.
  CORE-005's above-cap ADDITION retention does not decide the cap EDIT.
- OPEN-READY: first-start and Ready behavior with an enabled passed deadline.
  CORE-002's narrow Ready rejection does not decide this product behavior.
- OPEN-DEADLINE: accepting versus rejecting a new/moved deadline at or before now.
  Existing enabled-deadline observation priority does not choose edit validation.

No owner answer had arrived at this review. Elapsed waiting time, default selections
or adjacent successful component tests cannot supply one. Affected examples must
remain without executable acceptance oracles until recorded answers are incorporated
and separately reviewed. Finished/reopen, absolute Main/debt corrections and server
command/clock commit semantics retain their explicit owning contracts and full
release scope; this document does not silently remove them.

## Verification and limits

Independent Python integer audit (no product imports or helper invocation) checked
literal partitions10=2+5+3,8=5+0+3,7=5+2; signed identities
-150=-100-0-50 and -150=-100-50+0; INT64_MIN=-1-INT64_MAX;
overflow boundary9223372036854775808=INT64_MAX+1; and100-10+5=95.
It also checked exactly16 distinct ED01–16 rows and one entry for each OPEN anchor.
All passed; this is document arithmetic verification, not product acceptance.

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0. Project check reported396 task records/122 decisions/local links/
presets/current HTML; render snapshot current. No native configuration/build,
GoogleTest/Qt invocation, header/stub/test/fixture, implementation or missing-feature
red was authored or executed for this document task.

This approval permits integration of these exact two documents as a partial contract
checkpoint. It does NOT finish CORETEST-008, approve a concrete API/test baseline,
resolve owner questions, authorize production CORE-008 implementation or approve
future revised examples. Concrete finite bounds/error order/API, independent tests,
actual separate test review and intended behavioral red remain required. No real
server authority, provider, recovery/platform/UI/live/full-release verification is
implied. All seven integrations, three FX providers and remaining release scope stay
required.


## Actual full logical continuation and specification review — 2026-11-04

**APPROVED — exact completed logical documents and root specification transcription.**

Reviewer `service_reviewer`, different from continuation author `service_author`
and specification author/coordinator `/root`. Independently inspected actual author
`c03b4506` as imported at `0e56f39124913fb9653633fd8cc8d77e335229f7`, on isolated
`task/CORETEST-008-full-logical-review`. The entire earlier partial review above is
preserved verbatim; its then-unanswered decisions describe that historical revision.
They are resolved by the actual owner record reviewed here.

| Actual reviewed artifact | Git blob |
| --- | --- |
| [Completed timer editorial contract](../contracts/timer-editorial-commands-v1.md) | f4a4c3ceea45a58bc8116f73169d7bac081e9919 |
| [Continuation author evidence](coretest-008-contract-author.md) | 1dba6bb220eca905ffdcece3956153e905ea279e |
| [Specification](../specification.md), root change 5601a06b | 81406806c02df0f015a8c71c40b149ae1ff0ed19 |
| [Recorded owner answers](timer-editorial-owner-decisions.md) | b5df2afebbd826f13be65aae057d1a22029cf8b7 |

Read the complete revised contract/completions/ED17–44, the actual specification
delta and applicable §3.5/4/18/TIMER-01, existing lifecycle/debt/credit and state
boundaries, and CORE-008/API-007/CORE-007 owning briefs. No remaining requirement
conflict or new owner question was found.

Root's §4/§18/TIMER-01 transcription faithfully retains all three answers: each
below-Main cap attempt needs an explained, explicit Keep/Reduce/Abort choice;
first Start rejects an enabled deadline <= the authoritative observation and stays
Ready; an authorized Active deadline edit to now/past finishes immediately.
No saved global choice, UI acknowledgement as authority, warning-induced countdown
freeze or inferred grant is introduced.

The completed model revalidates revision, whole-operation access and observed Main.
Keep permits legitimate above-cap Main; subsequent additions repay debt before zero
Main headroom, and countdown may later restore headroom. Reduce derives the actual
current excess, preserves debt, and admits cap plus audited reduction together;
it never applies a stale preview amount or adds time after Main falls below cap.
Abort accepts neither editorial change and does not undo elapsed time or a reached
hard end. Durable clock progression versus refused/aborted-command commit remains
an explicit server transaction gate, without a hidden core write.

Old-configuration observation precedes edits. The new reached deadline uses the
existing zero-elapsed lifecycle reducer, including paused Main, Grace and waiting,
ordered HardEndReached/Finished, retained pre-call fields and cleared wait. Main
exhaustion retains Immediate/Confirmation/Fresh/Shared behavior, separate Grace and
debt, independent pause reasons and exact transitions. Credit rescues only through
actual Main addition. INT64_MIN, M-boundary reduction/overflow and full requested/
actual/capped/unapplied provenance remain intact. Manual operations acquire no
financial entitlement or original-effect identity through numeric helper reuse.

Ready/Finished settings edits preserve lifecycle/history. Their ordinary signed
deltas explicitly require API-007 first-start/absolute-Main or CORE-007 post-finish/
reopen/correction work, rather than claiming an Active-only product release.
Separate debt permission, post-finish inputs/actions, pending groups, review holds,
mixed/global RequireReview policy and server CAS remain mandatory owning gates.
No unknown policy is accepted as zero or decided by this document. Neither the
existing legacy credit contract nor another reducer is silently broadened/copied.

### Actual verification and approval limits

Stdlib-only inspection verified all original 16 ED rows byte-identical, exactly 44
unique sequential ED IDs, and the original author-note prefix unchanged. The
original review prefix is 6223 bytes, SHA256
`847f3bd1b4fbeb834abe27472a08e769db946e946982562e731f056faf0e87b3`.
Independent Python integer/bound checks passed 36 facts covering signed partitions,
INT64_MIN/M, current-versus-preview reductions, old hard-end field preservation,
post-Keep debt/headroom sequences and finite projection `4*(4096+1)=16388<=32768`.
These checks inspect literal document claims; they do not run a timer reducer or
establish executable acceptance. ED28/29's first-Start gate is explicitly distinct
from a complete Start operation.

`progress/state.json` is byte-unchanged from root 5601a06b: 406 IDs / 1497 dependency
edges. CORETEST-008 remains in_progress; CORE-008/API-007/CORE-007 remain planned.
`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` passed; project check reported 406 tasks / 122 decisions/current
links/presets/HTML. Only this review note is edited.

Approval permits the exact logical/specification revision to proceed to a concrete
API review. It does not complete the baseline, approve future APIs, finite B/I/error
priority, source/stubs, fixtures/tests, CI or implementation, nor authorize an
unreviewed first red. Independent API/source/test reviews, valid environment,
intended behavioral red and later production/integration/clock/CAS acceptance
remain required. No schema, native/runtime, provider, UI/platform or merge outcome
is certified. Full release scope and all seven integrations/three FX providers
remain unchanged.
