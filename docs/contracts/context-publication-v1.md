# Compact context and publication checks — v1

WORKFLOW-005; specification v0.45 §§17–18; bounded-coordination-v2 §§2,4–5.
Developer tooling only. Preserve existing context output, historical records and
all baseline assertions. No new manifest digest, workflow schema or approval grammar.

## Compact context

Add `--compact` to `tools/progress.py context TASK-ID`, optionally with `--stage NAME`.
The existing default command and `context(data, task_id)` API remain unchanged.
Add keyword-only `compact=False`; it must not mutate the input data.
The CLI still validates all state before selection, uses the existing availability
predicate and fails unknown task/stage requests read-only with exit1/stderr/no stdout.
Existing argparse usage errors remain exit2. Success is one deterministic JSON object.
Both compact JSON and publication-summary JSON use the existing serialization convention:
`json.dumps(value, indent=2, ensure_ascii=False) + "\n"`. Object fields follow the
declaration order below; selected stages follow CLI order, references recorded order,
and other arrays preserve existing state/derived order. Tests may compare parsed values
for behavior but must also verify this CLI serialization on a representative literal.

Compact output retains the existing top-level keys, except that `stage_context` is
added only when requested as today. `specification_version`, `state_updated_at`,
`dependents`, `shared_brief_tasks`, `available` and `limitations` retain their meaning.
The `task` object projects exactly id/title/module/kind/status/owner/brief/summary,
depends_on/spec_refs/notes/verification, plus publication when present. It adds
`required_stages` from workflow (empty list if absent), not the full workflow.
Each dependency projects id/status/owner/brief and publication (null when absent).
Omit task evidence/review prose, rich supplemental evidence, activity and other stages.
Notes remain the existing bounded handoff text, not newly inferred authorization.
An absent task publication is omitted, while each dependency has publication=null
when absent. Full-mode limitations stay unchanged. Compact limitations append exactly:
" Compact view omits non-blocking finding details and closing-only finding metrics;
use full mode or review links." The newline in this paragraph is editorial, not an
output newline; join it with one space. Closing-origin blocking IDs remain included.

For a requested stage, derive completeness/next gate with the existing workflow module.
Compact `stage_context` has exactly:

- name, complete, next_gate: existing meanings;
- packet: null for an unstarted stage, otherwise authors/revision/paths only;
- review_refs: selected-stage receipts in recorded order, each containing
  phase/provider/model/reviewer/revision/outcome/evidence;
- historical_blocking_findings: distinct recorded blocker IDs in first appearance
  order; no claim that they remain unresolved or have been resolved;
- metrics: review_count, elapsed_seconds and timed_reviews from the existing metrics.

All selected-stage review references and blocker IDs remain visible; there is no
silent byte/count truncation. An unstarted stage has empty references/blockers and
the existing empty metrics. No selected packet is repeated inside `task`. Full mode
and report links remain the way to inspect complete history. Neither view verifies
live GitHub/session/source state or grants authority to resume stopped work.

## Publication CLI and scope

Add the standard-library tool `tools/check_publication.py`:

```sh
python3 tools/check_publication.py --base origin/main --head HEAD \
  --task WORKFLOW-005 --stage contract --schema-python build/schema-venv/bin/python
```

`--base`, `--head`, `--task`, at least one `--stage` and `--schema-python` are required;
`--stage` may repeat for distinct stages. Refuse duplicates or unknown/unstarted stages.
There is no public root override, skip-schema flag, arbitrary command option, implicit
ref, install, fetch, network operation, staging, commit, push or merge. The tool runs
from its own repository root, independent of caller cwd. Argparse misuse returns2;
every failed check/setup returns1 with stderr/no stdout. Success returns0 and one JSON
summary. Diagnostic stderr may contain local details and must not be published raw.
Resolve a relative schema-Python argument against the caller's original cwd with
os.path.abspath, before using repository-root child cwd; do not resolve symlinks.

Private test seams (not new public CLI options): module ROOT is its own repository
root, and `main(argv: list[str] | None = None) -> int` accepts an explicit argument
list or normal sys.argv. Tests may patch ROOT to a disposable fixture.
Read state/output under that ROOT and call progress.validate(data, root=ROOT), rather
than rely on another module's import-time root defaults when the test seam is patched.
`run_step(argv: list[str], *, git: bool = False) -> subprocess.CompletedProcess[bytes]`
is the one patchable child-command boundary. It always uses cwd=ROOT, the fixed
environment below, binary stdout/stderr capture, the deadline and output checks;
it raises ValueError for unavailable command/timeout/overflow and returns the actual
completed returncode/bytes otherwise. Main treats unexpected nonzero status as failure.
Unit mocks may replace run_step for host/schema orchestration, while history/binding
cases use real disposable Git commands. Separate direct runner tests patch subprocess.run
to inspect the environment/cwd/flags, not a fake validator success claim.

Resolve both refs to commits without shell interpretation or option injection; require
base to be an ancestor of head and head to equal actual checkout HEAD. Refuse unresolved
merges, tracked changes and non-ignored untracked files. Ignored local evidence/build
state is permitted. Use read-only Git commands with optional index locking disabled.
All Git lookups of recorded paths are literal: Git children set GIT_LITERAL_PATHSPECS=1;
pathspec magic cannot widen the selected path set. An empty base..head range is valid and explicitly reports zero outgoing commits; it
does not establish that a PR is pending or authorize a merge. Reject invalid Git state.

Load/validate the current state with existing progress/workflow invariants. For each
selected stage, read ONLY task.workflow.stages[name].revision/paths as its recorded
binding. Resolve that commit and compare the Git mode/object for every recorded path
at the recorded revision and candidate head; missing paths, changed content/type/mode,
or escaping paths refuse. Selected paths must be regular Git files (100644/100755),
not symlinks or submodules whose target content would escape that comparison.
Do not use the full task commit as the candidate identity:
receipt-only changes after the recorded packet must remain valid. The clean checkout
condition binds tracked paths to the candidate, but filesystem checks can still observe
ignored local files; it is not an immutable tree sandbox. This is binding consistency,
not approval: a draft contract stage may be incomplete, and the summary says so. This
tool never permits publishing unreviewed tests or waives independent stage/merge gates.

Inspect every newly reachable commit in base..head, including merges, not just the final
diff. Walk each commit's entire recursive tracked tree with NUL-delimited Git output;
this catches a private path added then deleted or renamed during outgoing history.
Reject root local-evidence/ or local-secrets/ components, any path component `.venv`
or `node_modules`, and `.env`/`.env.*` basenames except `.env.example`. Match components,
not substrings: docs/local-evidence-guide.md is allowed. These are the fixed v1 forbidden
path rules, not content scanning. A clean path result never proves absence of all secrets.
Also reject any file below docs/evidence/ except docs/evidence/README.md, matching the
existing repository rule and catching an earlier private receipt deleted before head.
All forbidden-path matching is case-sensitive for this Linux developer tool: `.ENV`
and `Node_Modules` do not match these exact v1 rules. This is not Windows/client proof.
Do not read/dump blob contents for this check. Reject malformed/non-UTF-8 path names
rather than silently skip them. Base commits are not rescanned as newly published work.

## Actual local checks

After Git/history/binding checks, run these existing checks in order and stop at the
first failure, retaining bounded diagnostic output:

1. progress.py validate;
2. progress.py render --check;
3. check_project.py;
4. actual progress-schema validation through the existing admitted standards suite.

The first three use the host Python with `-E -s -B` and repository script paths.
Do not use `-I` there: check_project.py requires its sibling progress/workflow imports.
For step4, `--schema-python` explicitly selects the already prepared isolated environment;
preserve its venv invocation path rather than resolving a symlink to the system Python.
The checker never installs it or adds schema dependencies to ordinary tooling.

Invoke that interpreter with `-I -S -B` and a fixed stdlib-only driver. The driver loads
the existing tests/schema_validation/test_project_schemas.py as a non-main module and
runs ONLY RepositorySchemas.test_progress_schema_and_actual_state. Its existing
setUpClass metadata admission must precede all candidate standards-library imports;
the actual Draft202012Validator checks the shipped progress schema and current state.
The driver requires exactly one test, zero failures/errors/skips and emits a fixed JSON
witness with exactly these bytes (including one final LF):
`{"tests":1,"failures":0,"errors":0,"skipped":0}`.
The parent requires both exit0 and that exact witness;
unittest output goes to stderr, and driver stdout must equal the witness exactly.
an absent interpreter, missing SDK, `/bin/true`, missing test, zero tests or skipped test
cannot pass. It does not substitute a fake validator or add a new validation engine.
The caller still owns host/interpreter trust; a JSON witness does not authenticate
malicious executables or the complete host. Existing full schema CI remains unchanged.

EVERY child uses argv without a shell, cwd=ROOT and exactly PATH=/usr/bin:/bin,
LANG=C.UTF-8, LC_ALL=C.UTF-8. Git adds GIT_OPTIONAL_LOCKS=0 and GIT_LITERAL_PATHSPECS=1.
Inherit no GIT_*, PYTHON*, loader or other environment variables from the caller.
Apply a 120-second deadline to every child. Limit each captured stdout and stderr to
1MiB, including Git data (tree listings/revision lists), not just diagnostic text.
Tree growth beyond this explicit v1 limit needs a reviewed limit/streaming change,
never silent truncation. Exceeding the bound or a timeout
fails the check, never passes or retries. A total memory/process-tree guarantee is not
claimed; shared process-supervision improvements belong to WORKFLOW-007. No new native
consumer/kernel admission boundary is introduced for this developer tool.

Successful stdout contains exactly task/base/head/commit_count/stages/checks/limitations.
Refs are resolved full SHAs. `stages` holds each selected name/revision/path_count/complete.
`checks` has recorded_paths/outgoing_paths/progress/render/project/schema = "passed".
`limitations` is exactly this ordered list:

1. "Recorded bindings are not authenticated approvals."
2. "Forbidden-path checks are not complete secret scanning."
3. "Current-head hosted, repository and owner gates are separate."
4. "Filesystem checks can observe ignored local files."

Do not emit absolute workstation paths, environment dumps or raw child output in that
public summary. The tool creates no receipt or generated context file and must not
rewrite source/state/HTML/Git refs; existing Git/host interpreter observations are trusted
within their stated developer scope, not a protection bypass or execution authorization.

## Independent examples, tests and ownership

- Rich APITEST-016 compact tests view excludes the complete supplemental workflow and
  other stages, yet retains all selected review references/blockers/next gate/limitations.
  Old default output is byte/shape-compatible; inputs and state/HTML are unchanged.
- Valid clean fixture range with matching source paths passes after all actual check
  stages; unknown stage, dirty/untracked tree, nonancestor refs or a stale path refuses.
- A private path added then deleted, renamed away, or present in an outgoing merge
  tree is refused; a doc basename containing the words local-evidence is allowed.
- A receipt-only commit after the source packet passes binding checks; a source file
  content or mode change does not. Zero outgoing commits is reported honestly.
- Schema-invalid data that semantic validation accepts is rejected by the actual
  admitted validator. Empty-success/failed/skipped schema drivers are not schema proof.
  Unit mocks establish orchestration only; one real selected-environment integration
  check must demonstrate actual valid/invalid state without changing the real repo.

Coordinator owns contract/docs/navigation, shared progress/build/CI and publication.
Test author owns additive tests/tooling/test_compact_context.py and test_publication.py
and tests/schema_validation/test_publication_integration.py for actual SDK-backed cases,
plus an explicit loadable NI entry point tools/check_publication.py only as needed for
meaningful red. Its NI provides ROOT/main/run_step seams and returns the named
PUBLICATION_CHECK_NOT_IMPLEMENTED failure, never success. Also authorize a narrow
tools/progress.py test-author NI: accept compact=False keyword and --compact flag;
after existing validation/task/stage lookup, compact=True raises ValueError with
COMPACT_CONTEXT_NOT_IMPLEMENTED, caught by main as exit1/stderr/no stdout. CLI unknown
task/stage lookup still happens before that NI; the API has no stage argument. Default
mode stays byte/behavior-compatible. The implementer replaces only these explicit NI
branches. Observed CLI positive assertions expecting success and instead seeing the
named exit1 NI are meaningful red; argparse2/TypeError/missing file/import/SDK are not.
Tests must distinguish those causes, and no incidental API ERROR is counted as red.
Fresh implementer owns tools/progress.py
and tools/check_publication.py, with small support only if reviewed within this scope.
The coordinator owns schema-suite registration/guidance integration; no schema or
workflow record shape changes and no existing assertion deletion are authorized.
Keep actual SDK integration out of ordinary tooling discovery. Register it only under
the existing opt-in schema-validation CTest environment, with exactly the labels
schema-validation;acceptance;tooling, and a 120-second timeout. Run real valid/invalid
state cases there on both selected hosted Python versions. Ordinary unit mocks never
count as proof that the standards validator ran.

Fixtures do not clone HEAD (reviewed tests/NI may still be uncommitted). Copy the current
working-tree source files needed by the checks, including authored tests/stubs, into a
fresh temporary root; omit .git, ignored build/venv/private evidence and other generated
state. Initialize/commit that fixture independently, then create its own truthful synthetic
stage revision/path records and candidate history. Never edit the real repository for a
test or invent historical approvals. SDK-backed cases invoke the fixture's checker with
the actual selected interpreter/metadata admission and actual copied validator source.

Contract review → authored tests/NI stub → full independent test review → observed
intended assertion red through loadable entry points → fresh implementation → full
independent code review/relevant tooling/schema/scaffold checks → protected publication.
Keep unreviewed tests local; a metadata/docs-only draft can host review reports.
