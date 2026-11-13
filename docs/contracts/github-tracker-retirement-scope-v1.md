# Exact tracker assertion retirement scope v1 — MIGRATE-004 / ADR-0012

Authored proposal; author `codex:migrate004-contract-author-20261113:/root`
(Codex gpt-6.1-sol), specification v0.45 §§17–18.
Baseline full revision: `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.
This is the separate exact cleanup scope required by
[ADR-0012](../decisions/0012-test-suite-simplification.md) and
[migration v1 §6](github-task-migration-v1.md). It is not approval to delete.
[Cutover contract](github-authority-cutover-v1.md) defines replacement behavior.
Independent approval of this scope and the subsequent changed-test stage must precede
any assertion removal; cleanup source then needs its own full independent code cycle.
Correction author: `codex:coordinator-59095155`. No new test source or execution is
claimed by this proposal; actual independent review receipts are external evidence.

## Baseline and interpretation

The tables pin complete current Git blobs and exact class.method/line groups; line
numbers refer only to that revision. They are source inventories, not passing-result
claims. R = retire only the named obsolete ledger/UI assertion group; A = adapt all
meaningful assertions to the named retained contract, removing only explicit obsolete
subgroups; K = keep the entire assertion group, with mechanical fixture/import changes
if needed. Every A row adapts in its same pinned file/class/method unless an explicit destination
is given. “Here” means that same method; otherwise the
row names its destination. Mixed A rows authorize only their explicit R subgroups;
there is no undefined A/R category. No unlisted assertion is removable. An A row is not permission to weaken a
mixed method. Retained positive/negative variants and subTests remain, independently
derived from approved contracts rather than implementation output.

Old contracts retired here are state-as-current-backlog/status/availability controller,
its current editor schema, and generated HTML/DOM renderer. Historical conversion
preserves original grammar/fields at pinned archive revision; no current backlog
validator replaces them. Native relationships are audited by full one-time set mapping,
not a permanent dependency engine. New proof closure retains review/source gates;
actual baseline/acceptance execution receipts remain required and reviewed independently
of board status. Retiring old state status transitions never waives test-first or
passing acceptance requirements. The mechanical state acceptance-value check does retire;
the coordinator/independent review/CI receipt gate replaces it procedurally, with the
explicit capability limitation in the cutover contract. This is not equivalent automated
results enforcement. The implementation-before-observed-red dependency check and the completed-note
character cap also retire mechanically; coordinator duties remain, with explicit
limitations9/10 in STATUS/workflow. Per-task spec_refs validation also retires to immutable archive
evidence and active-brief review; repository specification/acceptance-map checks remain.

Exact line ranges below include every assertion in a method unless the disposition
explicitly narrows it. All expected error return/no-stdout/no-write properties of adapted
read-only methods remain; diagnostics change to the approved Proof/Publication namespace.
Per-method scope must be re-reviewed if its source changes before cleanup.

## `tests/tooling/test_progress.py`

Baseline blob `a5d18135d9614da02d3db14be6f4d18a57370d58` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `ProgressTests.test_baseline_is_valid` (49–50) | R: current HEAD state validity. Replace its CI duty with `test_proofs.py::ProofCliTests.test_actual_canonical_bundle_is_valid` (new; default CLI, actual canonical all-record admission), and actual SDK validation in `test_proof_schemas.py::ProofSchemas.test_proof_and_policy_contract` under schema.proofs. Archive admission is separately checked in the one-time audit; not an ordinary CI fallback. |
| `ProgressTests.test_combined_notes_at_500_characters_are_valid_for_every_status` (52–61) | R (mechanical): per-status state note cap (limitation10). Historical notes remain exact limitations without an aggregate cap; concise completion note remains coordinator policy. |
| `ProgressTests.test_combined_notes_over_500_characters_are_rejected_for_every_status` (63–73) | R (mechanical): per-status state note cap (limitation10); never truncate archived limitations to satisfy it. |
| `ProgressTests.test_set_over_limit_refuses_without_changing_state_or_html` (75–98) | R: removed set command and two-output refusal; proof/converter read-only and owned-output failure guarantees remain independently tested. |
| `ProgressTests.test_unknown_dependency_is_rejected` (100–103) | R: current state graph; reconciliation rejects unknown endpoints and native dispatch checks prerequisites. |
| `ProgressTests.test_dependency_cycle_is_rejected` (105–112) | R: current state graph; one-time reconciliation rejects cycles. |
| `ProgressTests.test_cannot_start_implementation_before_red_baseline` (114–122) | R (mechanical): retire the status/dependency observed-red dispatch check, with no equivalent proof function. Reviewed intended-red execution receipts remain mandatory coordinator dispatch and independent review obligations; declare limitation9 in STATUS/workflow. A complete tests packet alone is insufficient. |
| `ProgressTests.test_can_start_implementation_after_reviewed_red_baseline` (124–136) | R (mechanical): same status/dependency dispatch capability loss as114–122, not an adapted assertion. Actual reviewed-red receipts remain required procedurally; limitation9. |
| `ProgressTests.test_completed_implementation_requires_passing_acceptance` (138–150) | R (mechanical): retire the state done/acceptance-value assertion, explicitly losing this automated check. Actual passing acceptance remains a coordinator closure obligation with source-bound receipts and005 audit; proof complete does not enforce result values. Separate cleanup approval must accept this precise loss. |
| `ProgressTests.test_completed_implementation_accepts_passing_acceptance_and_separate_review` (152–161) | A: move independent review/completion positive to `test_workflow.py::WorkflowRecordTests.test_done_requires_complete_current_stages_and_publication`; retain passing approval variants. R only state done/acceptance-value routing; actual acceptance remains procedural (limitation7), not a new validator result field. |
| `ProgressTests.test_https_pull_request_evidence_is_valid_and_rendered_as_a_link` (163–173) | A: move HTTPS PR evidence positive into `test_proofs.py::ProofTests.test_record_shapes_and_lexical_references`; retain exact URL. R only HTML hyperlink rendering163–173; no product renderer assertion. |
| `ProgressTests.test_done_cleanup_requires_a_separate_approved_review` (175–183) | A: move distinct author/reviewer cleanup kind positive and rejection to `test_workflow.py::WorkflowRecordTests.test_nonexempt_contract_done_requires_separate_approved_review`, expanded for cleanup kind; R state done plumbing only. |
| `ProgressTests.test_non_https_url_is_not_accepted_as_evidence` (185–193) | A: move every insecure/malformed reference negative into `test_proofs.py::ProofTests.test_record_shapes_and_lexical_references`; keep passing HTTPS control. |
| `ProgressTests.test_self_review_is_rejected` (195–200) | A: move its legacy author/reviewer conflict into canonical stage author/reviewer refusal in `test_workflow.py::WorkflowRecordTests.test_receipt_independence_evidence_findings_and_provider_are_validated`; preserve both positive and conflict control. Legacy receipt format itself is not retroactively strengthened. |
| `ProgressTests.test_text_and_attribute_injection_are_escaped` (202–208) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_template_tokens_in_notes_stay_literal` (210–212) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_paths_cannot_escape_or_use_url_schemes` (214–217) | A: move every unsafe lexical path variant to `test_proofs.py::ProofTests.test_record_shapes_and_lexical_references`; retain independent valid path control. |
| `ProgressTests.test_duplicate_task_ids_are_rejected` (219–222) | A: move duplicate-ID refusal to `test_proofs.py::ProofTests.test_literal_frozen_policy_and_explicit_empty_enrollment` (duplicate required_records), plus existing `test_sorted_discovery_load_and_missing_inputs` for filename/task-ID mismatch. No second graph validator. |
| `ProgressTests.test_invalid_verification_claim_is_rejected` (224–227) | A: move the exact invalid verification value into shared `nested_shape_refusals` consumed by `test_proofs.py::ProofTests.test_nested_closed_shapes_and_schema_positive_forms` and `test_proof_schemas.py::ProofSchemas.test_proof_and_policy_contract`; both reject after valid control. |
| `ProgressTests.test_render_is_deterministic` (229–230) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_execution_view_orders_shuffled_diamond_and_epic_without_mutating_state` (232–249) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_availability_requires_all_done_dependencies_and_an_unclaimed_task` (251–268) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_full_backlog_execution_view_places_every_prerequisite_first` (270–277) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_no_available_tasks_does_not_offer_a_blocked_or_completed_handoff` (279–285) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_state_and_module_filters_offer_all_choices_as_checked_checkboxes` (287–300) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ContextCliTests.test_literal_context_preserves_selected_record_and_excludes_history` (372–396) | A: move bounded selected-record/unrelated-history and warning assertions to `test_compact_context.py::CompactContextTests.test_default_context_bytes_and_full_selected_history_are_unchanged`. R ledger envelope fields specification_version/state_updated_at/task/status/owner/dependencies/dependents/shared_brief_tasks/available. R only session-liveness and checkout-revision wording379–380: approved WARNING does not claim those checks. Keep recorded/GitHub/CI/not-verified concepts and literal proof WARNING/OMISSIONS; do not invent authorization verification. |
| `ContextCliTests.test_relationships_are_direct_and_preserve_declared_and_source_order` (398–417) | R: local context graph/shared-path projection; archive report retains source order and every shared-brief consumer, native board owns current graph. |
| `ContextCliTests.test_availability_matches_literal_contract_variants` (419–435) | R: local availability controller; coordinator checks actual native blockers/merges and owner stops. |
| `ContextCliTests.test_unknown_id_fails_on_stderr_without_stdout_or_writes` (437–441) | A: move unknown-ID exit1/empty-stdout/stderr/no-write assertions to `test_compact_context.py::CompactContextTests.test_lookup_validation_and_argparse_errors_precede_compact_ni`. |
| `ContextCliTests.test_invalid_state_uses_existing_validation_before_lookup_without_writes` (443–451) | A: move validation-before-selection and no-write assertions to `test_compact_context.py::CompactContextTests.test_lookup_validation_and_argparse_errors_precede_compact_ni`; unrelated malformed proof replaces unknown dependency, with passing bundle control. |

## `tests/tooling/test_compact_context.py`

Baseline blob `645eafd57cf137c010abb7871a477377d6c228fe` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `CompactContextTests.test_compact_cli_literal_shape_order_serialization_and_no_stage` (142–149) | A: keep exact JSON order/indent/UTF-8/newline, nonmutation and no-stage projection; replace ledger envelope with approved proof compact envelope and explicit omissions warning. |
| `CompactContextTests.test_selected_stage_keeps_all_refs_historical_blockers_and_exact_metrics` (151–163) | A: preserve literal packet/review_refs, B2/B1/B3 blocker order, count5/time3.75/timed3 and all exclusion sentinels; change proof envelope only. |
| `CompactContextTests.test_unstarted_and_untimed_stages_and_existing_next_gates` (165–185) | A: preserve every authoring/initial/verification/closing case, null packet/time, count, blocker and next_gate exact values. |
| `CompactContextTests.test_optional_publication_empty_required_stages_and_availability` (187–215) | A: adapt here. Keep optional publication/null merge fields187–197 and no-workflow leaf required_stages=[]/optional publication204–206. R availability status loop198–203 and dependency/availability207–215. No publication assertion206 is removed. |
| `CompactContextTests.test_default_context_bytes_and_full_selected_history_are_unchanged` (217–239) | A: retain exact full/compact=False equivalence, complete selected review history, closing-only findings, metrics and nonmutation; replace old task/dependency envelope with proof envelope. |
| `CompactContextTests.test_lookup_validation_and_argparse_errors_precede_compact_ni` (241–258) | A: retain unknown ID/stage, usage exit2, no stdout and no writes; unrelated malformed proof replaces unknown-dependency case. No NI exemption. |

## `tests/tooling/test_workflow.py`

Baseline blob `e85eaba0ffb95758ada30a8a2ba4807464e1fc52` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `WorkflowRecordTests.test_policy_requires_new_claim_records_without_fabricating_legacy_reviews` (103–160) | A: adapt here using canonical policy/proof. Keep exemption version/fingerprint/duplicate checks, historical exempt missing workflow and invalid supplied workflow. Keep epic minimum positive142–144 through `workflow.kind_minimum` (epics are not proof records). Keep done/missing-workflow refusal111–119 as `proofs.require_complete`; R in_progress/in_review status routing there. Keep planned/ready/blocked invalid supplied workflow134–141 as one status-free proof variant, R only those three statuses. R no-policy145–148 and availability/HTML150–160; no implicit absence exemption. |
| `WorkflowRecordTests.test_nonexempt_contract_done_requires_separate_approved_review` (162–173) | A: keep independent approval refusal variants; remove optional no-policy/state-done routing, require canonical contract completion. |
| `WorkflowRecordTests.test_kind_minimum_stages_and_packet_shape_are_enforced` (175–216) | A: retain all kind minima, nonempty/unique authors/revisions/paths, unsafe paths, stage sets, extra/unstarted stages. Replace strict current missing-file check with selected Git binding refusal; historical tree-free load must remain valid. |
| `WorkflowRecordTests.test_review_order_requires_initial_verification_and_fresh_closing_cycles` (218–244) | K: full independent review-order/provider/author/findings/timing grammar and rejected/fresh-closing variants; adapt fixture entry to workflow/proof, no assertion removal. |
| `WorkflowRecordTests.test_receipt_independence_evidence_findings_and_provider_are_validated` (246–269) | K: full independent review-order/provider/author/findings/timing grammar and rejected/fresh-closing variants; adapt fixture entry to workflow/proof, no assertion removal. |
| `WorkflowRecordTests.test_review_times_require_observed_ordered_timezone_pairs` (271–285) | K: full independent review-order/provider/author/findings/timing grammar and rejected/fresh-closing variants; adapt fixture entry to workflow/proof, no assertion removal. |
| `WorkflowRecordTests.test_done_requires_complete_current_stages_and_publication` (287–309) | A: retain absent/pending/stale/multiple-stage refusal, open observation not a merge; use require_complete plus current publication binding and actual-merge closure procedure, not state done. |
| `WorkflowRecordTests.test_publication_shape_and_dependency_readiness_match_context_and_html` (311–348) | A: adapt here. Keep ALL five malformed-publication variants at327–328 and every shape/equal-time/open-null assertion327–348. R only dependency readiness/HTML311–326. Publication validation stays shared with proof records. |
| `WorkflowCliTests.test_stage_context_derives_packet_next_gate_and_observed_metrics` (403–444) | A: keep every history, next gate, completeness, timing, null packet and read-only assertion through proof context. |
| `WorkflowCliTests.test_default_context_keeps_existing_shape_and_unknown_stage_refuses_read_only` (446–459) | A: replace ledger envelope exact keys only; keep exact selected record and unknown-stage refusal/no writes. |
| `WorkflowCliTests.test_reconcile_records_only_publication_without_claiming_checks_or_authority` (461–493) | R: entire461–493 reconcile snapshot/write procedure retires; no replacement reconcile or claim of equivalence. Surviving open/merged/monotonic-observed/immutable history requirements are already owned by `test_proof_publication.py::PreparedPublicationTests.test_same_pr_head_advance_time_and_merged_immutability`245–274 and `test_history_single_mutations_against_base_and_archive_origin`173–208. Snapshot parsing and two-output writes cease to be supported. |
| `WorkflowCliTests.test_reconcile_refusals_and_render_failure_preserve_every_file` (495–527) | R: entire495–527 retired snapshot parser/renderer writer failure procedure. Name the loss: invalid gh snapshot shapes and render rollback are no longer machine-checked because reconcile is removed. Existing forward-proof identity/time immutability stays in `PreparedPublicationTests.test_same_pr_head_advance_time_and_merged_immutability`; converter output ownership stays in `ConversionTests.test_conversion_absent_notes_historical_fields_and_claim_refusals`. No assertion is called mechanically equivalent. |

## `tests/tooling/test_check_project.py`

Baseline blob `c7d69e317baf73b684e44338a09bd60c58966436` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `ProjectArtifactTests.test_accepts_local_and_https_evidence_without_fetching_it` (54–58) | K: preserve every independent artifact/evidence/local-link/readable-summary assertion here. Adapt shared harness as explicitly mapped below; it currently patches tracker inputs and needs a valid STATUS positive control before testing negatives. |
| `ProjectArtifactTests.test_rejects_missing_or_remote_test_artifacts_and_invalid_evidence` (60–71) | K: preserve every independent artifact/evidence/local-link/readable-summary assertion here. Adapt shared harness as explicitly mapped below; it currently patches tracker inputs and needs a valid STATUS positive control before testing negatives. |
| `ProjectArtifactTests.test_requires_the_readable_summary` (73–77) | K: preserve every independent artifact/evidence/local-link/readable-summary assertion here. Adapt shared harness as explicitly mapped below; it currently patches tracker inputs and needs a valid STATUS positive control before testing negatives. |
| `ProjectArtifactTests.test_rejects_extra_evidence_receipts` (79–82) | K: preserve every independent artifact/evidence/local-link/readable-summary assertion here. Adapt shared harness as explicitly mapped below; it currently patches tracker inputs and needs a valid STATUS positive control before testing negatives. |

## `tests/tooling/test_publication.py`

Baseline blob `3429cbb5155aac9f26534537a1cc2cb5b760ef30` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `PublicationTests.test_clean_range_cli_success_has_exact_public_summary` (92–109) | A: adapt here to exact cutover summary: checks recorded_paths/outgoing_paths/proofs/policy_history/project/proof_schema, guard, three children in proofs/project/proof_schema order, exact ten ordered limitations; preserve task/base/head/stages/commit count and no extra fields. No legacy success expectations remain. |
| `PublicationTests.test_receipt_only_change_and_empty_range_keep_source_binding` (111–122) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_completed_synthetic_stage_reports_true_without_authentication` (124–148) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_repeated_distinct_stages_preserve_cli_order_and_exclude_unselected_binding` (150–162) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_stale_content_mode_missing_path_and_nonregular_git_files_refuse` (164–209) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_literal_unusual_recorded_paths_and_escaping_or_unresolvable_bindings` (211–235) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_forbidden_components_added_then_deleted_or_renamed_are_rejected` (237–251) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_allowed_component_boundaries_case_and_base_only_private_history` (253–268) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_merge_side_history_is_scanned_even_when_final_merge_tree_is_clean` (270–291) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_forbidden_path_in_merge_commit_itself_and_non_utf8_names_refuse` (293–313) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_dirty_untracked_conflicted_and_invalid_repository_refuse` (315–353) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_refs_require_commits_ancestry_checkout_head_and_no_option_injection` (355–380) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationTests.test_required_arguments_unknown_duplicate_unstarted_stages_and_invalid_state` (382–402) | A: keep all usage/selection/pending refusals; invalid canonical proof replaces invalid state. |
| `PublicationTests.test_host_then_inline_schema_order_and_relative_interpreter_preserve_venv_path` (404–432) | A: keep literal venv interpreter path and actual witness/order; replace four legacy children with proofs/project/proof-schema children. |
| `PublicationTests.test_each_host_failure_stops_later_checks_and_schema_witness_is_exact` (434–449) | A: preserve each retained/new child failure short circuit and exact witness/empty-success refusal; remove only retired progress/render children. |
| `PublicationTests.test_patched_root_is_passed_to_semantic_validation` (451–458) | A: proof loading and repository checks use explicit patched root; retain isolation from ambient checkout. |
| `PublicationRunnerTests.test_runner_uses_exact_scrubbed_env_root_binary_capture_and_deadline` (462–489) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |
| `PublicationRunnerTests.test_runner_bounds_both_binary_streams_including_git_and_refuses_setup_failures` (491–512) | K: preserve full source-binding, Git mode/path, checkout/ancestry, outgoing/private/merge-side history, runner bounds/environment and completion assertions; adapt fixture input to proof default only. |

## `tests/schema_validation/test_project_schemas.py`

Baseline blob `1c5c3724d1515bf7abd832c968cef8330c8efe8a` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `RepositorySchemas.test_inventory_includes_every_actual_schema_target_and_example` (184–208) | A: remove progress from schema node traversal and its Draft URI assertion only; retain every OpenAPI target/component/example inventory and add/retain real policy/proof target inventory in owning proof suite. |
| `RepositorySchemas.test_whole_openapi_and_each_schema_definition` (210–214) | K: entire real OpenAPI/component/positive-example/negative-example assertion group and actual SDK admission. |
| `RepositorySchemas.test_invalid_openapi_document_is_rejected` (216–220) | K: entire real OpenAPI/component/positive-example/negative-example assertion group and actual SDK admission. |
| `RepositorySchemas.test_all_current_references_resolve_without_retrieval` (222–234) | A: retire progress-reference loop (231–234) only; retain all OpenAPI/component/example offline reference resolution; proof/policy references stay tested in owning suite. |
| `RepositorySchemas.test_all_declared_positive_examples` (236–240) | K: entire real OpenAPI/component/positive-example/negative-example assertion group and actual SDK admission. |
| `RepositorySchemas.test_independent_component_examples` (242–250) | K: entire real OpenAPI/component/positive-example/negative-example assertion group and actual SDK admission. |
| `RepositorySchemas.test_progress_schema_and_actual_state` (252–254) | R:252–254 current state schema test. Its real-HEAD CI obligation moves to `ProofSchemas.test_proof_and_policy_contract` in existing schema.proofs: actual canonical policy AND every record using real admitted SDK; malformed/missing input fails. Python all-record CI oracle `ProofCliTests.test_actual_canonical_bundle_is_valid` also required. |
| `RepositorySchemas.test_independent_progress_examples` (256–264) | R: entire256–264 current state examples; retain their historical archive, replace with independent policy/proof positives/negatives, not schema bypass. |

## `tests/schema_validation/test_publication_integration.py`

Baseline blob `789502cdba4a5c2bf5a743bfc3e13d0441b792fb` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `PublicationSchemaIntegrationTests.test_valid_current_working_tree_copy_passes_real_admission_validator_and_publication` (57–72) | A: consolidate57–72 into `test_proof_publication_integration.py::ProofPublicationSDKTests.test_aggregate_and_every_actual_canonical_record_both_required`: retain SDK admission, exact one-test witness, no-write snapshot, actual checker success, proof_schema key, exact head and commit_count=1. R only progress-target method name and old schema key; no separate duplicate suite after mapping. |
| `PublicationSchemaIntegrationTests.test_schema_invalid_semantic_valid_state_is_rejected_by_actual_target_and_checker` (74–101) | R: old semantic-valid/schema-invalid state premise74–101 (missing review field) has no honest proof equivalent. Do not invent one. Retain its distinct actual-SDK enforcement duty in `ProofPublicationSDKTests.test_aggregate_and_every_actual_canonical_record_both_required`67–79: schema mutation with passing Python/bundle/project controls must cause the actual canonical schema driver and checker to refuse with ValidationError/empty stdout/non-NI diagnostic. The fixture stays Python-valid because only its schema is mutated. Declare this precise changed premise in the test packet. |
| `PublicationSchemaIntegrationTests.test_empty_success_and_unavailable_schema_executables_never_pass` (103–111) | A: consolidate103–111 /bin/true and missing interpreter refusals into `ProofPublicationSDKTests.test_missing_empty_and_non_sdk_interpreters_are_setup_refusals`; retain exit1/empty stdout/nonempty stderr/non-NI and positive actual SDK control. No lost distinct interpreter case. |

## Exact non-method groups and retained regression closure

- `tools/check_progress_ui.mjs` blob `971945cf4b661584af9eb4d122a122124eaa1c88` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. R: every Node assert/DOM simulation group: generated task count/order, dependency positions, checkbox choices/filter combinations/search, All/None, expansion/reorder, empty counter and theme. Entire contract is retired task dashboard; no native/browser product scene assertions. No deleted-button replacement tests.
- `tests/schema_validation/schema_cases.py` blob `94f4c8a7a305af7bb6064ebd1708288fa77035bf` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. R only PROGRESS_CASES and its state-only fixture construction; K every OpenAPI component/example case and independent invalid value. Historical fixture bytes remain pinned.
- `tests/support/publication_fixture.py` blob `2c660db0a1695763431b22b392e3e942b1babee2` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. A fixture only, no removal authority: migrate disposable Git setup from progress state/render to canonical proof/archive inputs; retain real commits/side history/private paths, selected schema SDK and explicit-root isolation. Never require actual project task SHAs in synthetic tests.
- `tools/progress_template.html` blob `667c1efed7c37219f7bf511eaeb000cf36725d5c` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. R dashboard markup/controller only after reviewed scope; no replacement UI fixture.
- `tests/CMakeLists.txt` blob `e9237db5df53aca9f0263f3816e1268ef1d99625` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. K scaffold.python_tools/project_consistency and all product registrations/labels; adapt checks to proof/repository defaults, no failure inversion/skips.
- `tests/schema_validation/CMakeLists.txt` blob `37ac597e29c6d987655d1174d5dfc5478a2672fc` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. R schema.publication target after all three methods above are mapped to schema.proof_publication; K schema.repository/proofs/proof_publication labels, environment, timeouts and all distinct actual SDK/witness oracles. No vacuous target.

### Pinned helper, fixture and import groups

All pins are at baseline `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.
For files already pinned above, the same complete blob covers these groups.

| Exact carrier/group | Disposition and destination |
| --- | --- |
| test_progress.py TaskRows20–32, task35–41, ProgressTests.setUp45–47 | R dashboard parser, real-state setup and state-status builder after mapped methods move. No real-state read remains in ordinary tests. Actual HEAD proof CI validation has its explicit owner below. |
| test_progress.py ContextCliTests.setUp306–336 | A: move selected/unrelated Unicode/history sentinels to test_compact_context.py::literal_state; preserve bounded selected-history assertions. R dependency/status/shared-path values only as enumerated method rows. |
| test_progress.py invoke_context338–364/context366–370 | A: move exit/output and created/deleted/changed-byte guards361–363 to CompactContextTests.invoke; adapt target to canonical proofs. R renderer/state patching only. |
| test_compact_context.py constants20–49/task50–59/literal_state62–92/expected_compact95–109, invoke113–130/success132–140 | A in place: preserve independently literal packet/review/blocker/timing/order/omission and Unicode values; canonical proof envelope only. Keep no-write130 and NI-versus-setup diagnostics, updating only actual seam marker. R explicitly retired availability/graph inputs, not surviving publication values. |
| test_workflow.py receipt28–37/stage40–46/publication49–55/task58–69/state72–80/exempt83–87 | Move these archive-format builders unchanged to new tests/support/archive_state_fixture.py; same parameter defaults and return values, no production algorithm. Frozen archive consumers in test_proofs and schema examples import there. Adapt current WorkflowRecordTests to separate canonical proof builders; never silently alter archive conversion inputs. R current-state validator calls, not original builder bytes/field expectations. |
| test_workflow.py Articles90–99 | R HTML parser after its specific availability/HTML assertions retire. |
| test_workflow.py WorkflowCliTests.invoke352–391/context393–401 | A in place: use canonical proof CLI. Keep disposable snapshot no writes and real-checkout no writes (state/HTML targets become canonical policy, every proof, checkpoint; preserve entries/bytes even for missing files), subprocess.run/Popen/socket/urlopen refusal guards371–374 and exact stage context values. R snapshot/render/serialization writer-injection branches only for retired reconcile methods. |
| test_publication.py LIMITATIONS26–31, fixture37–39/invoke41–71/success73–82/refusal84–90 | A in place: ten exact contract limitations, three children0 proofs/1 project/2 proof_schema, schema WITNESS only index2; preserve real Git calls and snapshot bytes/index/refs, explicit ROOT, scrubbed env, timeout and non-NI refusal assertions. No silent count changes in K methods. |
| test_publication.py tracker path literals203–204,233,357,400 | A: use canonical proof/bookkeeping counterparts where testing source exclusion/invalid bundle; line357 noncommit blob uses HEAD:tools/proofs.py. Preserve actual assertion intent. Mocked argv470 can stay literal historical command; it is not executed. |
| test_check_project.py setUp18–40/run_check42–52 | A in place: remove progress imports/STATE/OUTPUT/validate/render patches; provide exact valid STATUS version plus specification/evidence/map/preset inputs. Keep each original positive/negative artifact assertion and valid default positive before every mutation so missing STATUS cannot satisfy a negative. |
| test_project_schemas.py PROGRESS_URI17/setUpClass98–161/schema traversal184–208/reference loop222–234 | R PROGRESS_URI/state-schema read131/progress registration137/progress examples148 and only their traversal/reference branch. K SDK admission before imports, offline deny registry, OpenAPI/schema/example loading and structural validator. All eight baseline method rows above remain the complete method removal authority. |
| tests/support/publication_fixture.py LOCAL_SOURCES19–29 and PublicationFixture33–122 | A: canonical disposable Git/repository fixture, no progress render import/child. Remove retired paths from LOCAL_SOURCES; keep current owned source copy support, exact fake commits/side-history, clean environment, real-Git timeout, schema SDK path and snapshot filesystem/index/ref guards. Exclude real canonical policy/records from tracked-copy input and supply tiny independently authored synthetic bundle. Keep referenced repository artifacts and valid STATUS so project checks pass nonvacuously. |
| tests/support/proof_publication_fixture.py complete blob e7ca7b24e16dd096c71d96387548862946186413 | A: shared tiny synthetic archive/current proof builder. Retain receipts24–28, packet31–33, observation36–40, real history/bootstrap/append-only modes in43–168. Replace prepare_archive's render/save129 with direct independently authored historical bytes/report; arguments159–168 drops --proofs, preserving stage order/complete/bootstrap flags. Keep archive semantics and no production issue/commit assumptions. |
| tests/support/frozen_proof_policy.py complete blob6dd5ffb1ffda6f2582ba51418de56db469e4e106 | K literal120 IDs/order/fingerprint unchanged, imported independently of retired state and workflow test modules. No new exemption. |
| test_proofs.py imports20–24 and module helpers29–195 | A import archive builders from frozen support, removing progress import/current validation only as method rows specify. K every independent literal, shared shape mutation and APITEST016 fixture value. No helper-generated expected output from candidate implementation. |
| test_proof_publication.py invoke47–66/success68–71/refusal73–76 and LIMITATIONS31–38 | A: three children/witness index2 and exact ten limitations. Keep Git subprocess path, passing controls, no-write snapshot, NI refusal distinction and independent hand-authored guards. |
| test_repository_checks.py positive26–29/refusal31–34 | A: remove --repository-only and use default; retain full ROOT isolation, passing repository control and exact diagnostics; valid STATUS required. |
| test_proof_publication_integration.py probe40–43/checker45–51/positive53–60/refuse62–65 and test_publication_integration.py run_checker41–49/probe_schema51–55 | A: actual proof_schema aggregate, three children and literal admitted SDK; preserve clean environment, actual SDK probe, no-write guards and exact witness. Consolidate old integration harness only after all three mapped methods' distinct assertions are incorporated. |

`tests/tooling/test_publication.py` remains the primary suite for Git path/mode/checkout/
privacy/runner/environment assertions; `test_proof_publication.py` owns archive/policy/
record evolution and bootstrap. Retain both without copying their oracles into a third
suite. SDK integration has one primary owner: test_proof_publication_integration.py.
The old test_publication_integration.py can be removed only after its exact three
method mappings above appear in the reviewed test diff. Baseline schema nodes4 become3
(schema.repository/proofs/proof_publication), with ALL real SDK duties retained.
Scaffold retains its existing three nodes; canonical real-HEAD validation runs in
scaffold.python_tools through ProofCliTests.test_actual_canonical_bundle_is_valid.
Root CMake add_custom_target(progress) is an implementation retirement, not a test
removal. BASELINE_MANIFEST.json stays unchanged historical evidence and is not a consumer.

The following full pinned files own loader/conversion/policy/current-source/new-ID/archive/real-SDK counterexamples. K rows keep complete oracles; A rows enumerate interface changes and any specifically retired subgroup. No blanket K classification overrides these per-method dispositions.

`tests/schema_validation/test_proof_publication_integration.py` blob `180133140bf78c8ab566c78e6db1f254cf134f23` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `ProofPublicationSDKTests.test_aggregate_and_every_actual_canonical_record_both_required` (67–79): A: adapt here to cutover defaults, three children and `checks["proof_schema"]`. Keep pre-mutation actual SDK success/exact one-test witness70, BOTH selected and unselected record mutations68–74, and checker refusal/ValidationError/offending identity77–79. R specifically the post-mutation aggregate-success assertion76: the aggregate now validates actual records too, so replace it with nonzero probe exit, empty stdout/no success witness, real ValidationError and offending identity. After each schema-only mutation require passing Python default-bundle and project controls; snapshot guards cover all probes/checks. Preserve every independent synthetic schema case in the aggregate; no second duplicate suite or mock SDK. This precise before/after oracle change must appear in the authored test packet and PR.
- `ProofPublicationSDKTests.test_real_aggregate_skip_or_failure_never_counts_as_witness` (81–97): A: adapt here through positive helper to cutover defaults, three-child proof/project/proof_schema order and `checks["proof_schema"]` (retire only old schema key). Preserve actual SDK aggregate AND every-record invalid refusal, skip/failure/witness/interpreter properties in full; no mock SDK replacement.
- `ProofPublicationSDKTests.test_missing_empty_and_non_sdk_interpreters_are_setup_refusals` (99–104): A: adapt here through positive helper to cutover defaults, three-child proof/project/proof_schema order and `checks["proof_schema"]` (retire only old schema key). Preserve actual SDK aggregate AND every-record invalid refusal, skip/failure/witness/interpreter properties in full; no mock SDK replacement.

`tests/schema_validation/test_proof_schemas.py` blob `1dcd5a37c8a0b875437407253d9a374aafcfece4` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `ProofSchemas.test_proof_and_policy_contract` (45–119): A: retain ALL independent schema positive/negative/SDK/offline-reference assertions45–119 here; add actual HEAD policy/every-record validation in this same aggregate and CI node schema.proofs. Exact one-test witness retained; missing inputs fail clearly. Shared examples use frozen archive builders, not retired test_workflow imports.

`tests/tooling/test_proof_publication.py` blob `e988a4bbf00e62aeed8746aadbd1b02a85f481fd` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `PreparedPublicationTests.test_bootstrap_summary_exact_and_pending_selected_source` (96–118): A: adapt here to canonical default publication: exact six check keys recorded_paths/outgoing_paths/proofs/policy_history/project/proof_schema, three children in proofs/project/proof_schema order and ten literal limitations. R old progress/render/schema keys and default tracker success assumption only; keep all bootstrap/source/history/guard cases unchanged.
- `PreparedPublicationTests.test_complete_all_required_and_prospective_ni_replacement` (120–144): K.
- `PreparedPublicationTests.test_selected_binding_refusals_each_with_passing_control` (146–162): K.
- `PreparedPublicationTests.test_metadata_and_unrelated_source_leave_current_oracles_bound` (164–171): K.
- `PreparedPublicationTests.test_history_single_mutations_against_base_and_archive_origin` (173–208): K.
- `PreparedPublicationTests.test_revision_advance_is_pending_until_fresh_verification_closing` (210–220): K.
- `PreparedPublicationTests.test_historical_retired_packet_and_supplied_exemption_are_tree_free` (222–243): K.
- `PreparedPublicationTests.test_same_pr_head_advance_time_and_merged_immutability` (245–274): K.
- `PreparedPublicationTests.test_base_canonical_bootstrap_once_and_rollback` (276–302): A: adapt here to canonical default publication: exact six check keys recorded_paths/outgoing_paths/proofs/policy_history/project/proof_schema, three children in proofs/project/proof_schema order and ten literal limitations. R old progress/render/schema keys and default tracker success assumption only; keep all bootstrap/source/history/guard cases unchanged.
- `PreparedPublicationTests.test_one_sided_base_and_unavailable_archive_never_bootstrap` (304–321): K.
- `PreparedPublicationTests.test_canonical_discovery_and_strict_git_bytes` (323–347): K.
- `PreparedPublicationTests.test_archive_report_and_full_extraction_not_just_hashes` (349–375): K.
- `PreparedPublicationTests.test_brief_relocation_requires_exact_archive_path_mode_and_bytes` (377–391): K.
- `PreparedPublicationTests.test_new_id_exact_stanzas_and_selected_complete_packet_in_both_modes` (393–425): K.
- `PreparedPublicationTests.test_archive_origin_comparison_survives_corrupt_existing_base` (427–431): K.
- `PreparedPublicationTests.test_archive_pin_must_precede_target_and_ancestor_proof_location_blocks_reset` (433–439): K.
- `PreparedPublicationTests.test_exempt_historical_workflow_can_gain_pending_and_reviewed_packets` (441–458): K.
- `PreparedPublicationTests.test_supplemental_historical_map_is_neither_upgraded_nor_removed` (460–469): K.
- `PreparedPublicationTests.test_additive_archived_identity_uses_full_reconciled_kind_and_mapping` (471–481): K.
- `PreparedPublicationTests.test_selected_bundle_is_revalidated_not_trusted` (483–490): K.
- `PreparedPublicationTests.test_prepared_checkout_and_outgoing_private_history_keep_existing_guards` (492–510): K.
- `PreparedPublicationTests.test_prepared_schema_witness_and_each_host_failure` (512–521): A: adapt here to three children0=proofs,1=project,2=proof_schema; exact schema witness only at index2, each failure short circuits later calls and no stdout. R five-child count and legacy witness positions3/4; keep every failure kind and real schema witness refusal.
- `PreparedPublicationTests.test_prepared_only_flags_require_proofs_mode` (523–532): A: adapt here to final CLI. R --complete/--bootstrap require --proofs assertion523–532; replace with positive default --complete/--bootstrap controls and old --proofs argparse exit2. Keep unsupported flags/missing arguments checks. No compatibility path.
- `StrictGitJsonTests.test_bytes_parser_matches_strict_filesystem_transport` (536–544): K.

`tests/tooling/test_proofs.py` blob `757d232ccdc5cf0a0a27ba08fff6b2742673fbe8` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `ProofTests.test_literal_frozen_policy_and_explicit_empty_enrollment` (197–216): K.
- `ProofTests.test_nested_closed_shapes_and_schema_positive_forms` (218–224): K.
- `ProofTests.test_record_shapes_and_lexical_references` (226–257): K.
- `ProofTests.test_enrollment_identity_and_exempt_records_remain_strict` (259–270): K.
- `ProofTests.test_read_json_strict_decoding` (272–280): K.
- `ProofTests.test_sorted_discovery_load_and_missing_inputs` (290–314): K.
- `ProofTests.test_discovery_rejects_unexpected_children_filename_and_all_ancestor_symlinks` (316–359): K.
- `ProofTests.test_shared_review_engine_tree_free_and_strict_production_default` (361–390): A: adapt here to one historical tree-free grammar plus current selected Git bindings. R only strict progress.local_path/evidence_path/validate current-tree existence calls363–371,378–379,386. Keep bookkeeping exclusion, lexical path safety, historical missing-file positives, and review grammar. Current regular-mode/blob/missing-file failures remain `PublicationTests.test_stale_content_mode_missing_path_and_nonregular_git_files_refuse`164–209; no weakened current binding.
- `ProofTests.test_review_independence_order_outcomes_times_and_rejected_history` (392–414): K.
- `ProofTests.test_completion_currency_pending_minima_and_extra_stages` (416–447): K.
- `ProofTests.test_full_compact_context_exact_projection_and_nonaliasing` (449–477): K.
- `ProofTests.test_publication_observations_never_replace_current_approvals` (479–491): K.
- `ProofTests.test_frozen_supplemental_shape_and_no_canonical_completion_promotion` (493–514): K.
- `ConversionTests.test_bounded_conversion_enrollment_preservation_report_and_nonaliasing` (520–579): K.
- `ConversionTests.test_conversion_absent_notes_historical_fields_and_claim_refusals` (581–608): K.
- `ConversionTests.test_enrollment_is_evidence_based_and_never_status_age_exemption` (610–629): K.
- `ProofCliTests.test_explicit_inputs_usage_and_read_only_json_serialization` (645–665): A: adapt here. R usage-exit2 assertions for default validate/context (646–647), replace with successful canonical defaults over disposable valid ROOT while preserving explicit inputs, unknown flags/missing command exit2, unknown ID exit1, exact serialization/UTF-8/no writes. Do not use real production data for synthetic expected output.
- `ProofCliTests.test_one_disposable_full_source_cli_conversion_synthetic_mapping_only` (667–763): A: adapt here from temporary real-state coupling to pinned synthetic Git archive adapter. R production ROOT/state.json inventory and --source/--source-root/--identities syntax and strict progress.validate admission only. Retain exact full extraction/every-row report uniqueness/order/types/identity preservation and output reservation/cleanup assertions667–763 against independently authored tiny archive. New command --archive-commit FULL --output-dir NEW. No legacy validator or large copied dataset.

`tests/tooling/test_repository_checks.py` blob `65f9ceb461828c42f6040ee20be6e3613c5386fb` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `RepositoryChecks.test_repository_only_never_reads_tracker_or_render` (36–47): A: adapt here to default repository-only behavior. R inverted default-tracker refusal44 and direct retired check_tracker calls40,45–47. Replace with default success over broken/unreadable tracker sentinel while preserving no read/render calls. Old --repository-only is argparse exit2, separately asserted; positive uses no flag and valid STATUS.
- `RepositoryChecks.test_repository_checks_preserve_each_artifact_refusal` (49–67): A: adapt here removing --repository-only spelling only; retain every exact repository artifact failure and a default success control with valid STATUS. Failure exit1 must identify its intended artifact, never missing STATUS or argparse.
- `RepositoryChecks.test_tracker_checks_preserve_spec_refs_version_render_assets_anchors_links` (69–100): R: spec_refs/render/assets/anchors/HTML-link tracker groups69–100; per-task spec_refs loss is limitation8. A: move only version mismatch to new `RepositoryChecks.test_status_version_declaration` in this file: exact STATUS declaration, missing/duplicate/mismatch refusals and passing control. Keep independent repository link/spec/acceptance checks in their existing owners. Approval of this separate scope accepts exactly these retirements, not new test source.

All other tooling, native, schema, security, accounting, provider and product assertions
are outside removal scope. Preserve acceptance-map completeness/version/decision IDs,
all local evidence/link/containment checks and presets. Preserve actual interpreter,
SDK admission, schema-invalid, witness, OpenAPI components/examples, proof/policy
valid/invalid, mode/blob/path, checkout and merge-side privacy failures. No skip,
WILL_FAIL, blanket continue-on-error or baseline weakening.

## Before/after oracle agreement and review order

Before: current state validity/render/availability, strict current-state paths and
state schema plus shared workflow/publication/repository assertions. The intended-red dependency/status check and completed-note character cap retire mechanically, with explicit limitations9/10 and mandatory procedural duties; never imply automatic equivalents. After: full
one-time archive semantic/identity/relationship comparison, canonical all-record
proof/policy grammar, selected current Git binding, independent repository/spec/status
checks and coordinator actual merge/dispatch procedure. Historical context never
becomes a new result, and procedural duties are not misrepresented as machine gates.

Write a small named changed-behavior set after contract approval:

1. Canonical validate/context default invocation succeeds on one independently authored
   synthetic bundle; full/compact serialization, packet/null/next gate, ordered review
   references, rejected blocker history, timing, explicit omissions and no mutation
   stay literal. An unrelated malformed/missing required record fails before selection.
2. Default publication reads proof only, runs proof/project/real proof-schema children
   in order and fails for each child or wrong/empty witness. Existing stale source,
   checkout/private-history/schema/interpreter failures remain nonvacuous. No live board.
3. A newly added genuinely new **unselected** ID without its own complete selected
   reviewed brief packet refuses, even when the selected old task is complete. A
   legitimate new ID's exact stanzas and own reviewed binding pass; unrelated selected
   approvals cannot enroll it. Cover normal and bootstrap admission branches.
4. Bootstrap archive brief A versus newer target-base brief B cannot relocate B to A's
   URL. Equal baseline bytes may relocate. An already published correct archive URL
   continues passing normal publication after local brief deletion. Do not require the
   local source to exist for an already immutable historical URL.
5. Repository default needs no state/HTML, retains every acceptance-map/spec/link/preset
   rejection, and rejects STATUS/spec version mismatch. Archive converter reads only
   pinned regular Git blobs, rejects mismatch/unavailable history and preserves owned
   private output reservation/cleanup without importing old tracker.

Reuse existing focused fixtures, synthetic tiny commits and independent literal expected
values. No hundreds-record fixtures, deleted-file/button absence tests or fake historical
SHA exceptions. Record exact old/revised oracle mapping in the test packet/PR before
removal. The authored packet must give exact before/after tooling method counts (baseline181),
schema nodes (4→3) and methods, and scaffold nodes (3 retained), with every delta tied
to these rows; counts are audited inventories, not success targets. The separate scope approval is followed by authored-test independent initial
Claude/Codex verification/Claude closing **before** intended assertion red and deletions;
new behavior implementation follows in a fresh session and receives the complete code
review cycle too. Read-only/doc audit checks do not manufacture red. Scope approval alone
cannot approve new tests/source or erase earlier rejected reports. If any method/group
cannot be preserved as promised, stop for a revised exact independently reviewed scope.

## Historical source pins, outside current oracle paths

- `tools/progress.py`: `3610c486213392714e8486d0fabc7eb408ec1ac6` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`; pin before factoring/retirement.
- `tools/workflow.py`: `fa2764124932f6e4b2be438b71af896cd6e1319c` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`; pin before factoring/retirement.
- `tools/proofs.py`: `eb42b21c339cc97bc9a35ec9e67e860b192baf11` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`; pin before factoring/retirement.
- `tools/check_publication.py`: `7d8d353c838548defa81fab3e5d806a9a7dd45cf` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`; pin before factoring/retirement.
- `tools/check_project.py`: `07412fe88967cfd78b61aedc89e2148a0b0fc7cd` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`; pin before factoring/retirement.
- `progress/state.schema.json`: `712a5ee437ad37fb86e96ff80d5c9947950d6236` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`; pin before factoring/retirement.

003 original tests revision `5c89947c8cf4056f4463cf496fe099bb777c6967` retains its
original NI source objects and review/red receipts; runtime correction revision
`fcf205a86398004b5c2863eee4d8c85e2af2d023` is separate implemented source. Neither is
an allowlist bypass.004 tests receipts must separately enumerate each removed oracle
and any new NI seam's exact mode/blob at their actual reviewed tests revision. Their
current paths bind only retained/new oracles; no fictitious current files, altered
historical packet or source ownership field is permitted.
