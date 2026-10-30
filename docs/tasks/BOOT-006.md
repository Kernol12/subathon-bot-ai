# BOOT-006 — Show execution order and dependency readiness

Kind: scaffold (developer tooling). Module: Foundation.
Specification: v0.45 §17, DEV-02, DEV-04. Product contracts remain unchanged.
Status/owner/dependencies: progress/state.json.
Branch: task/BOOT-006-execution-order.

Scope/shared ownership: tools/progress.py, tools/progress_template.html,
tools/check_progress_ui.mjs, tests/tooling/, docs/PROGRESS.md, planning/task evidence,
progress/state.json and generated PROGRESS.html. No product behavior or acceptance
assertions change. Confirm the owner's merge of PR #2 and close BOOT-005 truthfully.

Acceptance:

- Default Execution order places every prerequisite before its dependent, including
  cross-module dependencies and epic completion gates. Preserve source order as the
  deterministic tie-breaker for independent tasks; do not rewrite the state ordering.
- A view selector restores the existing epic grouping without duplicating rows or
  losing search/status/module filters, expanded details, anchors or theme behavior.
- Extend status/state and module filters to multiple checkbox selections, initially
  all selected, with All/None controls and visible selection summaries. Match any
  chosen state and any chosen module, combined with search; empty selections show
  no matches. Preserve selections when changing backlog view.
- Highlight non-epic planned/ready tasks whose recorded prerequisites are all done.
  Do not present done, active, in-review or explicitly blocked tasks as free to pick up.
  Use the same computed availability for the summary and next handoff.
- Cover shuffled/diamond/multiple prerequisites, deterministic ordering and status
  exclusions with focused tooling tests. Exercise both views and existing controls
  in the offline controller check; record real-browser limits separately.
- Keep offline output, escaped editable text, schema and product verification gates.
  Update documentation/evidence/progress, run checks and open a PR for review.

This task changes development tools, not production product behavior. New tooling
tests are authored and run red before the ordering/controller change; they do not
constitute independent review or product acceptance. Leave the PR ready for review.
