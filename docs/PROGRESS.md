# Task navigation and proof

> GitHub owns task planning since the final protected MIGRATE-004 cutover.
> Canonical proof records preserve review and source evidence. The historical
> intermediate archive-preparation PR did not switch authority or close MIGRATE-004.

Use [GitHub Issues](https://github.com/Kernol12/subathon-bot-ai/issues) and Projects
for current status, assignments, native parent/sub-issues and prerequisites. Board
Kind/Epic selects are display-only. Read versioned [task briefs](tasks/README.md),
the [specification](specification.md), contracts and [workflow](AGENT_WORKFLOW.md).
All brief paths remain accessible through MIGRATE-004; bulk compaction belongs to006.

```sh
python3 tools/proofs.py validate
python3 tools/proofs.py context TASK-ID --stage tests --compact
python3 tools/check_project.py
```

Proof records preserve review/source facts and limitations, not a second backlog.
Missing required proof refuses. Compact context explicitly lists its omissions.
Neither board Done nor proof completion establishes actual PR merge or acceptance.
See [STATUS](STATUS.md) for capability limits and retired automation duties.
