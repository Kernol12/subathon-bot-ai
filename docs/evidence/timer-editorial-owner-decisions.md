# Timer editorial commands — owner decisions

Recorded by coordinator `/root` on 2026-11-04 for
[PR52](https://github.com/Kernol12/subathon-bot-ai/pull/52), CORETEST-008/CORE-008,
specification v0.45 §4/TIMER-01. This records the owner's answers; it is not an
implementation, reviewed executable baseline or passing acceptance result.

## Lowering the cap below current Main

The owner requested an additional warning with an explicit **Keep**, **Reduce**
or **Abort** choice. This is a choice for each affected cap change, rather than
one globally selected policy that silently keeps or reduces time.

For an example with Main 100 minutes, old cap 120 minutes and proposed cap
60 minutes:

| Choice | Result |
| --- | --- |
| Keep | Save cap 60; retain Main 100. The lower cap limits future additions. |
| Reduce | Save cap 60 and reduce Main to 60 through an explicit audited time reduction. |
| Abort | Cancel the change; retain cap 120 and Main 100. |

The warning must explain these consequences. Authorization, current-state
validation, exact requested/actual effects and audit requirements still apply to
the chosen operation. Other timer state follows its existing owning rules.
Concrete UI, command and transaction contracts remain to be authored and reviewed.

## First Start with an already-passed deadline

The owner accepted the recommendation: reject Start and remain Ready when the
enabled hard deadline is at or before the current authoritative observation.
Change or disable that deadline before starting.

## Editing an active event's deadline to now or earlier

The owner accepted the recommendation: allow an authorized deadline change to
the current observation or an earlier instant, and immediately finish the event.
Existing hard-end observation precedence and explicit reopening rules remain.

## Remaining work and coordination

All three PR52 product questions now have owner answers. Incorporate them into
the specification and affected timer contract/examples, obtain actual separate
review, then complete the API/test-first/CI gates. PR52 remains unfinished and is
resumed after PR62's normal merge on 2026-11-04. This logical/specification update
still needs actual different review before the API/test-first stages.

The owner separately accepted PR62's resource-scoped permission recommendation.
[That decision](accesstest-002-owner-decision.md) is resolved and its baseline is
merged. No further owner answer is needed for any of these four choices.
