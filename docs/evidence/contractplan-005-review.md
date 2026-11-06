# Independent plan review — CONTRACTPLAN-005

Plan author: coordinator `/root`. Different reviewer: `session_supplier_review`,
6 November2026. My earlier contribution was read-only readiness investigation;
I did not author the ADR, briefs, graph or generated progress. Actual candidate
`47879e9de23904195a56d249a9a81fea5bed1f2f` examined in NEW isolated branch
`task/CONTRACTPLAN-005-independent-review`, worktree
`/tmp/subathon-schema-plan-review`. Only this review note is owned/changed.

**APPROVED for planning integration.** Specification v0.45 §§17/18,
API-01/02/03, DEV-04/TEST-04 remain authoritative. Reviewed actual
[ADR-0008](../decisions/0008-schema-validation-foundation.md),
[plan brief](../tasks/CONTRACTPLAN-005.md),
[paired baseline/implementation brief](../tasks/SCHEMATOOL-001.md),
CONTRACT013 prerequisite addition, both navigation updates and state/generatedHTML.
This is a sequencing repair, not a standards-library selection or authored tool.

Independent structured comparison with actual base main `4e85241` confirms:
all423 original task IDs and1624 edges retained; exactly three new records and
five new edges yield426/1629. The added edges are plan005→CONTRACT005,
SCHEMATEST001→plan005/CONTRACT005, SCHEMATOOL001→SCHEMATEST001 and
CONTRACT013→SCHEMATOOL001. No cycles or reverse dependency on the actual HTTP
package exist. Only old CONTRACT013's dependency list/notes change. Every old task
status/owner/review/evidence and every other task field, all old activity, project
verification and summary remain unchanged. New foundation records are planned with
no review, evidence or verification; plan005 is in_progress pending this actual
review. Standalone foundations leave epic membership/work-package counts intact.

CONTRACT013 retains its two-route package and all accepted supplier/transport,
common-wire, runtime and release requirements. It cannot claim actual structure/
reference/example acceptance from links or JSON syntax. Missing runtime session,
clock, ingress and permission producers remain their own runtime gates rather than
new schema-authoring prerequisites. Separately active transportPR74 has its own
task branch; integration must retain its actual completed component/evidence when
later main is adopted, rather than replay this branch's older planned snapshot.

The new developer baseline explicitly requires bounded contract/API/limits/error/
format policy and independently authored examples, existing standards libraries
with actual version/license/pin review, independent test review, real selected
dependency foundation success and then observed intended missing-wrapper red.
Missing/import/unsupported-dialect/network/skip failures do not qualify as red.
Implementation waits for that integrated baseline and requires preserved assertions,
different source review, passing acceptance and unconditional ordinary offline CI.
This ordering does not insert an unreviewed working wrapper before its tests.

The brief confines references/instances/paths to an admitted offline registry,
requires bounded size/depth/count and truthful failures, and distinguishes format
annotations from actual exact-domain constraints. It forbids a new standards engine,
implicit remote fetch, escaping paths, secret diagnostics and success-after-skip.
No OpenAPI/JSONSchema version, dependency pin, CLI/API, schema, fixture/test/stub,
wrapper, workflow or support claim is chosen by the plan. Tool validation cannot
certify raw HTTP decoding, CSRF/authentication/current permissions or framework/
WebSocket behavior. All seven integrations, three FX providers and full release
requirements remain. No unresolved product conflict was found.

Actual independent commands in this isolated candidate:
`python3 tools/check_project.py` PASS426records/122decisions/current links/presets/HTML;
`python3 tools/progress.py render --check` PASS;
`python3 -m unittest discover -s tests/tooling -v` PASS19/19 in0.319s;
`node tools/check_progress_ui.mjs` PASS DOM simulation;
`git diff 4e85241 HEAD --check` PASS. Standalone Python assertions verified the
exact ID/edge deltas and all preservation claims above. No schema validator,
wrapper/test/stub, native feature, live endpoint/provider or platform check ran.
Existing consistency/tooling CI is appropriate to this planning-only change;
actual completion metadata/latest publication CI/current rules/discussions and
normal merge remain coordinator gates. Later executable CI and required-check
configuration must be assessed when the new foundation actually exists.
