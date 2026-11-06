# Independent review — SESSIONCONTRACT-001 pre-content split

Author: coordinator /root and the preserved session-document author assignment.
Reviewer: /root/session_plan_review, separate resumed agent, 6 November 2026.
Verdict: approved for integration of the planning split and current-main reconciliation.
This does not approve a supplier/transport contract, tests, runtime behavior or task completion.

Reviewed actual preserved `eb98c1834a79d9ab341d0531de1effd5887da7d7` against
`ba66804`, then actual reconciliation `40c33ad` against integrated main
`e2cd5ebfd4842bf2d776642d901d679612132060`. Authority is specification v0.45
§§10/17/18, AUTH-05/AUTH-12 and API-01/02/03, with the retained five-minute
freshness and lifecycle obligations. [ADR-0007](../decisions/0007-session-contract-prerequisite.md)
explicitly permits splitting before authoring while preserving consumer prerequisites.

## Exact scope checked

The original supplier/current-session resolution, ownership, lifecycle, opaque random
256-bit tokens, hash-only server storage, issuance/renewal/rotation/revocation,
normal24h/remembered30day owner-configurable defaults, five-minute freshness,
execution-time recheck, secrecy, limits and literal examples stay in supplier001.
Browser cookie/Origin/CSRF/header syntax, parsing/trust/mixed-credential boundaries,
HTTP authentication errors and transport examples move to transport002. Viewer/overlay
credentials stay separate; no public-client shortcut, auth implementation or supported
framework capability follows from the split. Both briefs require actual independent
review and retain later test-first/runtime/platform/release gates.

Independently compared complete task dictionaries and dependency-edge sets:

- 422 IDs/1619 edges become423/1624; zero removed IDs or edges.
- The only added ID is SESSIONCONTRACT-002, depending on SESSIONCONTRACT-001.
- CONTRACT-013, SESSIONTEST-002, SESSIONTEST-003 and EPIC-007 each retain001
  and additionally depend on002. Their existing runtime prerequisites remain.
- SESSIONCONTRACT-002 remains planned. Supplier001 remains in progress, with
  an actual resumed owner and appended reconstruction note, not a completion claim.
- Every task outside the six scoped records equals latest main exactly, including
  STORAGE-018 status, source/configuration reviews and all evidence/notes.
- EPIC-007 correctly reports9 work packages/16 child records; the index reports
 193 packages/353 child records. Both-document descriptions now match the graph.

No blocking planning/scope defect remains. Current-main reconciliation preserves all
old source/tests/contracts and valid reviews. Supplier001 still needs actual authored
content and its own different reviewer; this planning review cannot release consumers.

Reviewed document blobs, identical in eb98c18 and40c33ad:

| Path | Git blob |
| --- | --- |
| docs/tasks/EPIC-007.md | e96925062bab671f69173022ee899356c6d29164 |
| docs/tasks/EPIC-007/SESSIONCONTRACT-001.md | 752ee0de5f7fb7cfc9704592bf79a0eb9667b230 |
| docs/tasks/EPIC-007/SESSIONCONTRACT-002.md | d1e7906f51432e6bece34486e6eabac50ece2240 |
| docs/tasks/EPIC-007/SESSION-002.md | 6688bad3c62c580e46598f4d73b4be87116c4dfd |
| docs/tasks/EPIC-007/SESSION-003.md | 1d2271dc40cc2f455ed3bc5c680aa9bb4acceec7 |
| docs/tasks/CONTRACT-013.md | a3ea2b7c6bb5745ae18beed724c8db61b4ad2b33 |
| docs/tasks/README.md | 30584f292c5c5d76383723e6cdea4962ea42e008 |

Reconciled progress/state.json blob: `e141a71f5e1d14450b6510cd0293a8a91dd1451b`.

## Observed checks and limits

On stable committed40c33ad in the integrator worktree:

```text
python3 tools/check_project.py
PASS:423 task records,122 decisions,local links,presets,current HTML
python3 tools/progress.py validate
Valid progress:423 task records
python3 tools/progress.py render --check
Progress snapshot is current
git diff --check e2cd5eb..40c33ad
exit0
```

Independent Python comparison loaded the three committed JSON objects with
`git show`, asserted every non-scoped latest-main task equal, all old edges retained,
the exact five added edges, the preserved scope records, and seven identical document
blobs. It passed after allowing the reviewed actual supplier-owner reassignment and
single appended resume note. An earlier project-check attempt during the integrator's
active merge encountered transient JSON conflict markers; it was not a candidate
defect or completed verification and was superseded by the stable checks above.

Existing automatic project/progress tooling remains relevant. This planning-only split
introduces no executable suite, dependency pin or new branch-protection requirement.
Latest publication CI, normal merge checks and actual contract review remain separate.
No native, provider, platform, authentication, live-operation or release verification
was performed by this review. All seven integrations, three FX providers and the full
agreed product remain required.
