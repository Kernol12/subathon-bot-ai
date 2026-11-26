## Task

Task ID and issue:
Review tier (A full cycle / B single review / C checks only) and why:
Stage (tests / implementation / contract; a tier B task names both stages):
Behavior changed and specification or contract sections:

## Verification

Reviewed test baseline and intended-red evidence (implementation stage, or both stages of a tier B task):
Commands actually run, environment and results:
Proof level (authored / observed red / implemented / implemented, local run only / simulated / live-verified):
Only for a suite that no hosted runner can run (`docs/AGENT_WORKFLOW.md`): the commit, command and result of its one plain local run on the reviewed head, and why CI cannot run it:
Remaining gates (live, provider or platform; the missing CI run of a local-run-only suite):

## Review

Reviews are PR comments in the format of `docs/templates/REVIEW.md`.
Tier A only: `progress/proofs/TASK-ID.json` is added in one commit after the closing
approval, immediately before merge.

A draft PR still runs CI. A recorded review is not an authenticated identity, and an
open PR is not a merged change.
