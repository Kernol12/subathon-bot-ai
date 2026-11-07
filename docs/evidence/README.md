# Historical evidence index

The 969 files already tracked under `docs/evidence/` are historical receipts. Keep
them intact; this page summarizes their navigation and does not replace the original
reviews, logs, commands or task records.

The area summary is derived from completed task records in `progress/state.json`. As of
the EPIC-028 planning update, 92 done tasks contain 809 evidence links to 774 distinct
paths. A file can be linked by more than one task or area, and some archived receipts
are not directly referenced by a current done-task record. Counts describe organization,
not verification strength or product completion.

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
