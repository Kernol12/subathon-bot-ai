# Historical evidence index

The 969 files currently tracked under `docs/evidence/` are historical receipts and are
scheduled for archival under git tag `evidence-archive-2026-11` by EVIDENCE-001. Until
that task is reviewed and merged, do not remove or add files here. Afterward, this page
will be the concise human-readable area summary; detailed records remain available at
the archive tag and PRs.

The area summary is derived from completed task records in `progress/state.json`. As of
the EPIC-028 planning update, 92 done tasks contain 809 evidence links to 774 distinct
paths. A file can be linked by more than one task or area, and some archived receipts
are not directly referenced by a current done-task record. Counts describe organization,
not verification strength or product completion; this inventory is not yet a readable
summary of what each area proves.

| Area / task family | Done tasks | Distinct linked paths in area |
| --- | ---: | ---: |
| Accounting | 9 | 63 |
| Automation | 2 | 27 |
| Core | 16 | 77 |
| Currency | 1 | 19 |
| Foundation | 9 | 12 |
| Identity | 12 | 101 |
| Localization | 2 | 20 |
| Operations | 27 | 206 |
| Presentation | 2 | 9 |
| Server/API | 10 | 48 |
| Tooling | 2 | 197 |

The global distinct-path count is lower than the sum of area counts because shared
receipts may be linked from tasks in multiple areas. The repository history is organized
by task or evidence family; use task IDs and links in `progress/state.json` to locate
original detail. New task work should use one PR URL in its `evidence` field and one
concise outcome/next-step line in `notes`, rather than adding a second parallel report.

Older receipts mention `FIRST_AGENT_PROMPT.md`, `docs/templates/HANDOFF.md` and
`docs/templates/VERIFICATION.md`; these were removed by the EPIC-028 workflow update.
`coordinator-handoff.md` is the previous coordinator checkpoint, now replaced by
`progress/coordinator-checkpoint.md`. `BASELINE_MANIFEST.json` likewise describes the
original packaged snapshot only.
