# Capability and verification status

Specification version: 0.45

GitHub owns planning; pull requests own review receipts. The old tracker is retained
under the owner-authorized historical disposition, without re-verification.

There is no runnable server yet: the only executable is the scaffold. The current
priority is a vertical slice that starts the server, runs one timer and accepts one
fake contribution (see the [checkpoint](../progress/coordinator-checkpoint.md)).

## Historical source-bound summary

Source: [old tracker](https://github.com/Kernol12/subathon-bot-ai/blob/355ffdfe/progress/state.json). This is a historical snapshot, not current execution or CI.

Bounded core/accounting, SQLite, account/session and corrected native WebSocket components have synthetic/component evidence. Full v0.45 product and release readiness remain unverified, including production-host/session-clock integration, live providers, UI/platform and operations gates. Task completion describes bounded artifacts; publication and current hosted CI are separate PR observations. Use the checkpoint and latest owner instruction for authorization, and task/PR evidence for reviews and next actions.

## Historical verification observations

**C++ contract/scaffold — passed**

IMPL-001 numeric credit helper implemented; independent code review approved. Local scaffold configure/build and 3/3 infrastructure checks pass. No timer/service transaction behavior claimed.

**Progress tooling — passed**

As recorded in the old tracker: 71 Python tooling methods, project consistency and the retired progress page passed. The tooling suite has grown since; run `python3 -m unittest discover -s tests/tooling` for the current count.

**CMake / CLion / Qt — passed**

Selected CLion GCC/C++20 scaffold target ran; IDE-terminal scaffold configure/build/CTest 3/3 and separate Qt 6.12.0 probe 4/4 passed. Product UI/platform support remains unverified.

**GoogleTest acceptance — passed**

19 unchanged independently reviewed revision-1 acceptance cases and four additive reviewed regressions pass; independent reviewer rebuilt and replayed 26/26 full-preset cases. Historical intended red retained. Numeric helper only.

**Live provider checks — not_run**

No live credentials, OAuth or payment tests used.

**Windows / Wayland / OBS — not_run**

Requires the actual reference platforms and capture environment.

**Drogon / Qt feasibility — partial**

Pinned local libraries and six reviewed real Linux loopback experiments pass; independent rebuild/replay passes. Production bridge, persistent WebSocket sessions, deployment/TLS and platform support remain unverified.

**Native WebSocket profile — passed**

44 real Linux loopback methods pass locally and in selected Ubuntu CI with reviewed source/resource, SDK/ABI, fixture, CI recipe and loaded-map evidence. Actual host, TLS/proxy routing, clock, auth/sync and full platform/release gates remain separate; final publication gates are recorded in PR110.

**Production host lifecycle baseline — partial**

Reviewed stubs/tests compile. First run: 75 isolated Qt failures and all 18 native methods fail at explicit NI (24 primary, 24 secondary exit-42 cleanup assertions), no skips/setup/timeouts. Independent results approved; API-001 behavior and actual session clock remain unimplemented.

## Current limits and separate closure duties

Full v0.45 remains required: all seven integrations and three exchange-rate providers,
exact accounting/reversal provenance, English/German and Light/Dark/System, separate
native/browser renderers, desktop editor, Linux server and Windows/Linux x86-64 clients.
Component/synthetic/loopback/real SQLite observations do not establish a production
host, real session clock/authority, live provider, UI/browser/OBS/platform, deployment,
restore, security, performance or release capability. See the specification and test strategy.

The retired tracker checked a few things automatically that the coordinator now
checks by hand before closing a task:
- Passing acceptance results are read from the PR's latest-head CI, not from a
  completion flag.
- The brief's specification references are reviewed at dispatch and closure. Spec ID
  uniqueness and acceptance-map completeness remain automated.
- Intended red is observed on the reviewed tests before implementation starts.
- Closure needs the actual merged PR; a board status or a proof record is not a merge.

Review depth follows the three tiers in the [workflow](AGENT_WORKFLOW.md).
WORKFLOW-006/007 stay stopped. This page records capabilities and limitations, not a
backlog.

Historical disposition: historical, as recorded, not re-verified. Known limitations
are the d717 clean-FIRST identity conflict, six missing dirty-run sources and the
unrecovered seal. They do not block migration and do not supply new execution proof.
