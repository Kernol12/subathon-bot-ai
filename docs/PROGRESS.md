# Static progress page

Open root PROGRESS.html directly in a browser. It is self-contained: no fetch, CDN,
web server or build tool is needed to view it. Reload after the agent regenerates it.
The page is a generated snapshot, not an automatically synchronized live dashboard.

## Task views and availability

**Execution order** is the default. Every recorded prerequisite appears before its
dependent, including cross-module dependencies and the children required to complete
an epic. Independent tasks keep the source ordering as a deterministic tie-breaker;
this is a valid dependency order, not a requirement to finish every row sequentially.
The generated HTML has this order even without JavaScript.

Select **Epic grouping** to restore the original order from progress/state.json.
Switching views moves the same rows and retains filters, expanded details and links.
Search/status/module filters can hide prerequisites in either view.

Open **All states** or **All modules** to select any combination of checkboxes.
All choices start selected; each menu provides **All** and **None** controls and
summarizes the current selection. Tasks match any selected state and any selected
module, together with the search text. Selecting none in either menu shows no
matches. Selections survive view changes; Escape or a click outside closes a menu.

Green highlights and **Dependencies complete** badges mark non-epic tasks whose
status is planned or ready and whose prerequisites are all done. Active, in-review,
explicitly blocked and completed tasks are excluded. **Ready to pick up** and
**Next handoff** use this same computed availability. Status labels are not rewritten;
the status filter still matches the recorded status. Availability does not waive
independent reviews, test-first gates, ownership or live/platform requirements.

This remains an offline snapshot: GitHub merges do not change task status automatically.
Record the verified merge/result in progress/state.json and regenerate the page.

## Updating the snapshot

The authoritative editable file is progress/state.json. Do not edit PROGRESS.html by
hand. Python >=3.10 and the standard library generate and validate it:

```sh
python3 tools/progress.py validate
python3 tools/progress.py set BOOT-002 --status in_progress --owner clion-agent --note "Checking the Arch toolchain"
python3 tools/progress.py render
python3 tools/progress.py render --check
python3 tools/check_project.py
```

`set` updates status/owner, appends the supplied note to activity and task notes, and
renders HTML. It never invents test outcomes. Edit verification evidence, review identity,
new tasks or dependencies in JSON, validate, then render. Keep exact command results in
linked docs/evidence/ notes. Use relative project paths, not absolute machine paths.

Dependency-ready means every `depends_on` task is done. The tool refuses starting or
completing tasks with unfinished dependencies and detects cycles/unknown IDs. The `done`
status requires an evidence note and independent review for implementation/test-review
kinds. This is a record consistency check; it cannot prove that an agent's claims are true.
Do not invent a reviewer or pass result to satisfy validation.

The [backlog index](tasks/README.md) links every epic to its bounded child briefs.
An epic's dependencies aggregate required child completion; children depend on their
own prerequisites rather than their parent epic. Production slices have a separate
`*TEST-NNN` baseline dependency for independent review and an intended red run.
Search the epic ID in PROGRESS.html to find its child records. Planned records are
neither authored tests nor observed verification, and epic completion still requires
the listed live/platform gates where applicable.

For parallel agents, the integrator owns state.json/PROGRESS.html or merges scoped task
updates carefully. Regenerate once after merging. HTML/task counters show task progress,
not percentage of release features; split tasks as design details emerge.

Verification values: not_run, draft, passed, failed, observed_red, partial, blocked.
Record tests vs native builds vs live providers vs Windows/Wayland/OBS independently.
No secrets, supporter names, tokens or private production logs belong in the page.

Optional controller check (Node.js, no packages): `node tools/check_progress_ui.mjs`.
This uses a small DOM simulation to check both orderings and retained controls;
it does not verify real-browser layout or OBS.
The state.schema.json file provides editor hints; tools/progress.py performs the
runtime checks including dependencies, evidence and separate-review records.
