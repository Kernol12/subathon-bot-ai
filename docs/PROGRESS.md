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
Record the verified merge/result in progress/state.json and regenerate the page; the
reconcile command below imports a saved observation without checking GitHub itself.

## Read a bounded task context

```sh
python3 tools/progress.py context API-012
```

This read-only command validates state and prints JSON to stdout: the complete selected
task, direct dependency status/evidence/reviews, direct dependents, other tasks sharing
its brief, specification version, state update time and dashboard availability. It
omits unrelated task records and activity history, writes no generated context file,
and leaves state/HTML untouched. Unknown IDs fail with a diagnostic and nonzero exit.
Output is recorded information, not verified live GitHub/CI, session liveness or
checkout revisions. Availability is dependency readiness, never authority to override
an owner stop instruction. Read the linked brief, required contracts and scoped
instructions; the coordinator separately checks actual branches, sessions and PRs.

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
new tasks or dependencies in JSON, validate, then render. For completed tasks, record
the PR URL in `evidence`, keep all notes together to 500 characters or fewer, and record
independent review in the state `review` field and PR. Update the matching area summary;
add no files under `docs/evidence/`. Follow [`docs/AGENT_WORKFLOW.md`](AGENT_WORKFLOW.md)
for the readable-summary and archive rules. Local evidence uses project-relative paths,
never absolute machine paths.

Dependency-ready means every prerequisite is complete under its recorded policy.
Legacy done tasks keep their original dependency semantics. A newly tracked done
prerequisite also needs recorded merged publication; artifact completion alone does
not authorize dependent work. Validation refuses ready/active/done dependents while
that prerequisite is unfinished, and checks cycles and unknown IDs. Availability
is still not owner authorization.

The forward `workflow_policy` freezes the 120 already-done IDs and their integrity
fingerprint. New non-epic claims require structured stages; do not backfill invented
historical reviews. Contract tasks need an approved separate review except the
explicit historical exemptions. Implementation, baseline and cleanup retain their
existing passing-acceptance, observed-red and separate-review requirements. Tracked
done tasks additionally require every declared stage closed on its packet revision
and a publication observation (which may still be open). The developer JSON schema
provides structural hints; runtime tooling enforces the relational rules.

This is record consistency, not authentication of sessions, commits, review quality
or live CI. Never invent a reviewer or pass result to satisfy validation.

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

## Stage handoffs and observed review outcomes

```sh
python3 tools/progress.py context WORKFLOW-003 --stage tests
```

The optional stage view adds only `stage_context` to the original output. It gives
the selected packet, completeness, next gate and derived metrics. A required stage
not yet authored has no packet and next gate authoring; no invented revision or
assignment is needed. Unknown stages or a task without workflow fail read-only.
The view contains no unrelated activity or new manually maintained context files.
See [workflow records v1](tasks/WORKFLOW-003.md) for the exact record/CLI contract.

Record actual provider/model/session, phase, revision, verdict, report and scoped
finding IDs per receipt. Add observed timing pairs when available; omit estimates.
Counts, distinct blockers, findings first seen at closing and measured durations help
assess review usefulness. They are not quality scores or automatic review waivers.

## Import a publication observation

```sh
gh pr view 120 --json url,state,headRefOid,mergedAt,mergeCommit > /tmp/task-pr.json
python3 tools/progress.py reconcile WORKFLOW-003 --pr-json /tmp/task-pr.json
```

The PR URL must already be recorded in task evidence. This imports OPEN/MERGED
metadata with a UTC import timestamp and regenerates HTML. It changes no artifact
status, task notes, reviews, verification, activity, authorization or checkpoint.
It rejects malformed input, PR reassociation, merged-to-open regression and changed
immutable merged fields. Review the diff, check live gates separately and reconcile
checkpoint prose manually. An open head is an observation, not a future-head promise.

Validation, serialization and rendering complete before either output is replaced.
Sibling temporary files are cleaned up; an OS failure between replacements can still
leave mismatched outputs, so inspect and reconcile after such failure. The command
performs no network, git or merge operation and does not authenticate saved JSON.
Finalize a PR's own merge in the next already-authorized metadata change; avoid a
chain of completion-only PRs. Historical unrun checks are never promoted by inference.
