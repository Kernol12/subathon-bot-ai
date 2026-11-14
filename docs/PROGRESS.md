# Task navigation and proof

> GitHub owns task planning. Pull requests hold review receipts; proof records exist
> only for full-cycle (tier A) tasks and for history before the cutover.

Use [GitHub Issues](https://github.com/Kernol12/subathon-bot-ai/issues) and Projects
for current status, assignments, native parent/sub-issues and prerequisites. Board
Kind/Epic selects are display-only. Read versioned [task briefs](tasks/README.md),
the [specification](specification.md), contracts and [workflow](AGENT_WORKFLOW.md).
One feature is one issue with a tests stage and an implementation stage.

```sh
python3 tools/proofs.py validate
python3 tools/proofs.py context TASK-ID --stage tests --compact
python3 tools/check_project.py
```

Proof records preserve review and source facts, not a second backlog. Neither a
board status nor a proof record establishes an actual PR merge or acceptance.
See [STATUS](STATUS.md) for capability limits.
