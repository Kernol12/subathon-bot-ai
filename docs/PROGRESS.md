# Static progress page

Open root PROGRESS.html directly in a browser. It is self-contained: no fetch, CDN,
web server or build tool is needed to view it. Reload after the agent regenerates it.
The page is a generated snapshot, not an automatically synchronized live dashboard.

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

For parallel agents, the integrator owns state.json/PROGRESS.html or merges scoped task
updates carefully. Regenerate once after merging. HTML/task counters show task progress,
not percentage of release features; split tasks as design details emerge.

Verification values: not_run, draft, passed, failed, observed_red, partial, blocked.
Record tests vs native builds vs live providers vs Windows/Wayland/OBS independently.
No secrets, supporter names, tokens or private production logs belong in the page.

Optional controller check (Node.js, no packages): `node tools/check_progress_ui.mjs`.
This uses a small DOM simulation; it does not verify real-browser layout or OBS.
The state.schema.json file provides editor hints; tools/progress.py performs the
runtime checks including dependencies, evidence and separate-review records.
