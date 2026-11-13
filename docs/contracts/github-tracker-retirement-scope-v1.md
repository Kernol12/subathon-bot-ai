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
No test was authored/executed for this document and no red/review is claimed.

## Baseline and interpretation

The tables pin complete current Git blobs and exact class.method/line groups; line
numbers refer only to that revision. They are source inventories, not passing-result
claims. R = retire only the named obsolete ledger/UI assertion group; A = adapt all
meaningful assertions to the named retained contract, removing only explicit obsolete
subgroups; K = keep the entire assertion group, with mechanical fixture/import changes
if needed. No unlisted assertion is removable. An A row is not permission to weaken a
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
results enforcement. Per-task spec_refs validation also retires to immutable archive
evidence and active-brief review; repository specification/acceptance-map checks remain.

Exact line ranges below include every assertion in a method unless the disposition
explicitly narrows it. All expected error return/no-stdout/no-write properties of adapted
read-only methods remain; diagnostics change to the approved Proof/Publication namespace.
Per-method scope must be re-reviewed if its source changes before cleanup.

## `tests/tooling/test_progress.py`

Baseline blob `a5d18135d9614da02d3db14be6f4d18a57370d58` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `ProgressTests.test_baseline_is_valid` (49–50) | R: current HEAD state validity. Archive source admission/report and all-record proof validity replace its distinct responsibilities. |
| `ProgressTests.test_combined_notes_at_500_characters_are_valid_for_every_status` (52–61) | R: per-status state note cap. Historical notes remain exact limitations without an aggregate cap; concise completion note remains coordinator policy. |
| `ProgressTests.test_combined_notes_over_500_characters_are_rejected_for_every_status` (63–73) | R: per-status state note cap; never truncate archived limitations to satisfy it. |
| `ProgressTests.test_set_over_limit_refuses_without_changing_state_or_html` (75–98) | R: removed set command and two-output refusal; proof/converter read-only and owned-output failure guarantees remain independently tested. |
| `ProgressTests.test_unknown_dependency_is_rejected` (100–103) | R: current state graph; reconciliation rejects unknown endpoints and native dispatch checks prerequisites. |
| `ProgressTests.test_dependency_cycle_is_rejected` (105–112) | R: current state graph; one-time reconciliation rejects cycles. |
| `ProgressTests.test_cannot_start_implementation_before_red_baseline` (114–122) | A: remove depends_on/status mechanism only; retain refusal to claim implementation readiness without reviewed intended red in proof/dispatch procedure. |
| `ProgressTests.test_can_start_implementation_after_reviewed_red_baseline` (124–136) | A: preserve positive reviewed-red prerequisite example as procedural dispatch evidence; board readiness alone never authorizes implementation. |
| `ProgressTests.test_completed_implementation_requires_passing_acceptance` (138–150) | R (mechanical): retire the state done/acceptance-value assertion, explicitly losing this automated check. Actual passing acceptance remains a coordinator closure obligation with source-bound receipts and005 audit; proof complete does not enforce result values. Separate cleanup approval must accept this precise loss. |
| `ProgressTests.test_completed_implementation_accepts_passing_acceptance_and_separate_review` (152–161) | A/R: retain independent review and proof-completion positives; retire only the state done/acceptance-value routing. Passing actual acceptance is mandatory procedural closure evidence, not an equivalent proof-validator result field. |
| `ProgressTests.test_https_pull_request_evidence_is_valid_and_rendered_as_a_link` (163–173) | A: keep HTTPS evidence admission; retire only rendered HTML hyperlink assertion. Loader/reference and context evidence oracle. |
| `ProgressTests.test_done_cleanup_requires_a_separate_approved_review` (175–183) | A: keep independent cleanup approval requirement via workflow completeness; retire state done field plumbing. |
| `ProgressTests.test_non_https_url_is_not_accepted_as_evidence` (185–193) | A: preserve insecure/malformed URL rejection through proof reference validation. |
| `ProgressTests.test_self_review_is_rejected` (195–200) | A: preserve author/reviewer conflict rejection through canonical stage review engine. |
| `ProgressTests.test_text_and_attribute_injection_are_escaped` (202–208) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_template_tokens_in_notes_stay_literal` (210–212) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_paths_cannot_escape_or_use_url_schemes` (214–217) | A: preserve all unsafe lexical path variants through shared proof/repository safe-path primitive. |
| `ProgressTests.test_duplicate_task_ids_are_rejected` (219–222) | A: preserve duplicate identity refusal in policy/record discovery and archive mapping. |
| `ProgressTests.test_invalid_verification_claim_is_rejected` (224–227) | A: preserve invalid historical verification enum rejection in real proof/schema cases. |
| `ProgressTests.test_render_is_deterministic` (229–230) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_execution_view_orders_shuffled_diamond_and_epic_without_mutating_state` (232–249) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_availability_requires_all_done_dependencies_and_an_unclaimed_task` (251–268) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_full_backlog_execution_view_places_every_prerequisite_first` (270–277) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_no_available_tasks_does_not_offer_a_blocked_or_completed_handoff` (279–285) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ProgressTests.test_state_and_module_filters_offer_all_choices_as_checked_checkboxes` (287–300) | R: entire HTML renderer/controller-only method; no product renderer behavior is covered. Proof serialization/nonmutation remains a distinct retained oracle. |
| `ContextCliTests.test_literal_context_preserves_selected_record_and_excludes_history` (372–396) | A: retire specification_version/state_updated_at/task/status/owner/dependencies/dependents/shared_brief_tasks/available projection; keep selected proof exact content, bounded unrelated-history exclusion and honest limitations. |
| `ContextCliTests.test_relationships_are_direct_and_preserve_declared_and_source_order` (398–417) | R: local context graph/shared-path projection; archive report retains source order and every shared-brief consumer, native board owns current graph. |
| `ContextCliTests.test_availability_matches_literal_contract_variants` (419–435) | R: local availability controller; coordinator checks actual native blockers/merges and owner stops. |
| `ContextCliTests.test_unknown_id_fails_on_stderr_without_stdout_or_writes` (437–441) | A: proof context unknown-ID refusal, exit1, empty stdout and no writes. |
| `ContextCliTests.test_invalid_state_uses_existing_validation_before_lookup_without_writes` (443–451) | A: replace unrelated unknown dependency with unrelated malformed proof; all-record validation must precede selection, no output/writes. |

## `tests/tooling/test_compact_context.py`

Baseline blob `645eafd57cf137c010abb7871a477377d6c228fe` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `CompactContextTests.test_compact_cli_literal_shape_order_serialization_and_no_stage` (142–149) | A: keep exact JSON order/indent/UTF-8/newline, nonmutation and no-stage projection; replace ledger envelope with approved proof compact envelope and explicit omissions warning. |
| `CompactContextTests.test_selected_stage_keeps_all_refs_historical_blockers_and_exact_metrics` (151–163) | A: preserve literal packet/review_refs, B2/B1/B3 blocker order, count5/time3.75/timed3 and all exclusion sentinels; change proof envelope only. |
| `CompactContextTests.test_unstarted_and_untimed_stages_and_existing_next_gates` (165–185) | A: preserve every authoring/initial/verification/closing case, null packet/time, count, blocker and next_gate exact values. |
| `CompactContextTests.test_optional_publication_empty_required_stages_and_availability` (187–215) | A: keep optional publication including null merge fields and no-workflow required_stages=[] (187–197,203–205 adapted); retire only available/status variants, dependencies projection and dependency-availability assertions (198–202,206–215). |
| `CompactContextTests.test_default_context_bytes_and_full_selected_history_are_unchanged` (217–239) | A: retain exact full/compact=False equivalence, complete selected review history, closing-only findings, metrics and nonmutation; replace old task/dependency envelope with proof envelope. |
| `CompactContextTests.test_lookup_validation_and_argparse_errors_precede_compact_ni` (241–258) | A: retain unknown ID/stage, usage exit2, no stdout and no writes; unrelated malformed proof replaces unknown-dependency case. No NI exemption. |

## `tests/tooling/test_workflow.py`

Baseline blob `e85eaba0ffb95758ada30a8a2ba4807464e1fc52` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `WorkflowRecordTests.test_policy_requires_new_claim_records_without_fabricating_legacy_reviews` (103–160) | A: keep frozen exemption/version/fingerprint/duplicate checks, no fabricated legacy workflow and invalid supplied workflow; retire legacy no-policy acceptance and status/HTML/availability branches (145–160), with no-policy fixture144–147 removed as well. Proof enrollment and archived claim admission replace them. |
| `WorkflowRecordTests.test_nonexempt_contract_done_requires_separate_approved_review` (162–173) | A: keep independent approval refusal variants; remove optional no-policy/state-done routing, require canonical contract completion. |
| `WorkflowRecordTests.test_kind_minimum_stages_and_packet_shape_are_enforced` (175–216) | A: retain all kind minima, nonempty/unique authors/revisions/paths, unsafe paths, stage sets, extra/unstarted stages. Replace strict current missing-file check with selected Git binding refusal; historical tree-free load must remain valid. |
| `WorkflowRecordTests.test_review_order_requires_initial_verification_and_fresh_closing_cycles` (218–244) | K: full independent review-order/provider/author/findings/timing grammar and rejected/fresh-closing variants; adapt fixture entry to workflow/proof, no assertion removal. |
| `WorkflowRecordTests.test_receipt_independence_evidence_findings_and_provider_are_validated` (246–269) | K: full independent review-order/provider/author/findings/timing grammar and rejected/fresh-closing variants; adapt fixture entry to workflow/proof, no assertion removal. |
| `WorkflowRecordTests.test_review_times_require_observed_ordered_timezone_pairs` (271–285) | K: full independent review-order/provider/author/findings/timing grammar and rejected/fresh-closing variants; adapt fixture entry to workflow/proof, no assertion removal. |
| `WorkflowRecordTests.test_done_requires_complete_current_stages_and_publication` (287–309) | A: retain absent/pending/stale/multiple-stage refusal, open observation not a merge; use require_complete plus current publication binding and actual-merge closure procedure, not state done. |
| `WorkflowRecordTests.test_publication_shape_and_dependency_readiness_match_context_and_html` (311–348) | A: retain publication malformed state/URL/head/merge/time variants, equal timestamp positive and open null fields (329–348); retire only local dependency readiness/HTML assertions (311–328). |
| `WorkflowCliTests.test_stage_context_derives_packet_next_gate_and_observed_metrics` (403–444) | A: keep every history, next gate, completeness, timing, null packet and read-only assertion through proof context. |
| `WorkflowCliTests.test_default_context_keeps_existing_shape_and_unknown_stage_refuses_read_only` (446–459) | A: replace ledger envelope exact keys only; keep exact selected record and unknown-stage refusal/no writes. |
| `WorkflowCliTests.test_reconcile_records_only_publication_without_claiming_checks_or_authority` (461–493) | A: retire state/HTML writer assertions; retain publication open/merged/null/observed semantics and unchanged historical not_run/checkpoint/content as proof forward-record prefix constraints. No replacement reconcile command. |
| `WorkflowCliTests.test_reconcile_refusals_and_render_failure_preserve_every_file` (495–527) | A: keep malformed publication and immutable merged identity/time/head refusal through publication guard; retire renderer/serialization two-output writer failure injection only. Retain converter owned-output failure cleanup elsewhere. |

## `tests/tooling/test_check_project.py`

Baseline blob `c7d69e317baf73b684e44338a09bd60c58966436` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `ProjectArtifactTests.test_accepts_local_and_https_evidence_without_fetching_it` (54–58) | K: independent repository artifact/evidence/local-link/readable-summary assertions, no state dependency or weakening. |
| `ProjectArtifactTests.test_rejects_missing_or_remote_test_artifacts_and_invalid_evidence` (60–71) | K: independent repository artifact/evidence/local-link/readable-summary assertions, no state dependency or weakening. |
| `ProjectArtifactTests.test_requires_the_readable_summary` (73–77) | K: independent repository artifact/evidence/local-link/readable-summary assertions, no state dependency or weakening. |
| `ProjectArtifactTests.test_rejects_extra_evidence_receipts` (79–82) | K: independent repository artifact/evidence/local-link/readable-summary assertions, no state dependency or weakening. |

## `tests/tooling/test_publication.py`

Baseline blob `3429cbb5155aac9f26534537a1cc2cb5b760ef30` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `PublicationTests.test_clean_range_cli_success_has_exact_public_summary` (92–109) | A: keep exact public summary/limitations, change only retired child/check keys to cutover contract proof defaults. |
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
| `RepositorySchemas.test_progress_schema_and_actual_state` (252–254) | R: entire252–254 current state editor-schema test; real proof/policy schema validation and actual canonical bundle driver replace it. |
| `RepositorySchemas.test_independent_progress_examples` (256–264) | R: entire256–264 current state examples; retain their historical archive, replace with independent policy/proof positives/negatives, not schema bypass. |

## `tests/schema_validation/test_publication_integration.py`

Baseline blob `789502cdba4a5c2bf5a743bfc3e13d0441b792fb` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`.

| Exact method / source lines | Disposition and replacement oracle |
| --- | --- |
| `PublicationSchemaIntegrationTests.test_valid_current_working_tree_copy_passes_real_admission_validator_and_publication` (57–72) | A: preserve actual admitted SDK/publication integration, semantic-valid/schema-invalid refusal, unavailable interpreter and empty-success witness refusal; switch only state fixture/target/witness-name expectations to canonical proof/policy aggregate. Consolidate with existing proof integration only after assertion-by-assertion comparison. |
| `PublicationSchemaIntegrationTests.test_schema_invalid_semantic_valid_state_is_rejected_by_actual_target_and_checker` (74–101) | A: preserve actual admitted SDK/publication integration, semantic-valid/schema-invalid refusal, unavailable interpreter and empty-success witness refusal; switch only state fixture/target/witness-name expectations to canonical proof/policy aggregate. Consolidate with existing proof integration only after assertion-by-assertion comparison. |
| `PublicationSchemaIntegrationTests.test_empty_success_and_unavailable_schema_executables_never_pass` (103–111) | A: preserve actual admitted SDK/publication integration, semantic-valid/schema-invalid refusal, unavailable interpreter and empty-success witness refusal; switch only state fixture/target/witness-name expectations to canonical proof/policy aggregate. Consolidate with existing proof integration only after assertion-by-assertion comparison. |

## Exact non-method groups and retained regression closure

- `tools/check_progress_ui.mjs` blob `971945cf4b661584af9eb4d122a122124eaa1c88` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. R: every Node assert/DOM simulation group: generated task count/order, dependency positions, checkbox choices/filter combinations/search, All/None, expansion/reorder, empty counter and theme. Entire contract is retired task dashboard; no native/browser product scene assertions. No deleted-button replacement tests.
- `tests/schema_validation/schema_cases.py` blob `94f4c8a7a305af7bb6064ebd1708288fa77035bf` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. R only PROGRESS_CASES and its state-only fixture construction; K every OpenAPI component/example case and independent invalid value. Historical fixture bytes remain pinned.
- `tests/support/publication_fixture.py` blob `2c660db0a1695763431b22b392e3e942b1babee2` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. A fixture only, no removal authority: migrate disposable Git setup from progress state/render to canonical proof/archive inputs; retain real commits/side history/private paths, selected schema SDK and explicit-root isolation. Never require actual project task SHAs in synthetic tests.
- `tools/progress_template.html` blob `667c1efed7c37219f7bf511eaeb000cf36725d5c` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. R dashboard markup/controller only after reviewed scope; no replacement UI fixture.
- `tests/CMakeLists.txt` blob `e9237db5df53aca9f0263f3816e1268ef1d99625` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. K scaffold.python_tools/project_consistency and all product registrations/labels; adapt checks to proof/repository defaults, no failure inversion/skips.
- `tests/schema_validation/CMakeLists.txt` blob `37ac597e29c6d987655d1174d5dfc5478a2672fc` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`. A schema.publication target only as necessary to consolidate real proof publication assertions; K schema.repository/proofs/proof_publication labels, environment, timeouts and all distinct actual SDK/witness oracles. No vacuous target.

Every method in the following full pinned files is K; no deletion authority. These own existing loader/conversion/policy/current-source/new-ID/archive/real-SDK counterexamples. Mechanical default/fixture adaptation must preserve each method's complete assertion set.

`tests/schema_validation/test_proof_publication_integration.py` blob `180133140bf78c8ab566c78e6db1f254cf134f23` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `ProofPublicationSDKTests.test_aggregate_and_every_actual_canonical_record_both_required` (67–79): K.
- `ProofPublicationSDKTests.test_real_aggregate_skip_or_failure_never_counts_as_witness` (81–97): K.
- `ProofPublicationSDKTests.test_missing_empty_and_non_sdk_interpreters_are_setup_refusals` (99–104): K.

`tests/schema_validation/test_proof_schemas.py` blob `1dcd5a37c8a0b875437407253d9a374aafcfece4` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `ProofSchemas.test_proof_and_policy_contract` (45–119): K.

`tests/tooling/test_proof_publication.py` blob `e988a4bbf00e62aeed8746aadbd1b02a85f481fd` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `PreparedPublicationTests.test_bootstrap_summary_exact_and_pending_selected_source` (96–118): K.
- `PreparedPublicationTests.test_complete_all_required_and_prospective_ni_replacement` (120–144): K.
- `PreparedPublicationTests.test_selected_binding_refusals_each_with_passing_control` (146–162): K.
- `PreparedPublicationTests.test_metadata_and_unrelated_source_leave_current_oracles_bound` (164–171): K.
- `PreparedPublicationTests.test_history_single_mutations_against_base_and_archive_origin` (173–208): K.
- `PreparedPublicationTests.test_revision_advance_is_pending_until_fresh_verification_closing` (210–220): K.
- `PreparedPublicationTests.test_historical_retired_packet_and_supplied_exemption_are_tree_free` (222–243): K.
- `PreparedPublicationTests.test_same_pr_head_advance_time_and_merged_immutability` (245–274): K.
- `PreparedPublicationTests.test_base_canonical_bootstrap_once_and_rollback` (276–302): K.
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
- `PreparedPublicationTests.test_prepared_schema_witness_and_each_host_failure` (512–521): K.
- `PreparedPublicationTests.test_prepared_only_flags_require_proofs_mode` (523–532): K.
- `StrictGitJsonTests.test_bytes_parser_matches_strict_filesystem_transport` (536–544): K.

`tests/tooling/test_proofs.py` blob `757d232ccdc5cf0a0a27ba08fff6b2742673fbe8` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `ProofTests.test_literal_frozen_policy_and_explicit_empty_enrollment` (197–216): K.
- `ProofTests.test_nested_closed_shapes_and_schema_positive_forms` (218–224): K.
- `ProofTests.test_record_shapes_and_lexical_references` (226–257): K.
- `ProofTests.test_enrollment_identity_and_exempt_records_remain_strict` (259–270): K.
- `ProofTests.test_read_json_strict_decoding` (272–280): K.
- `ProofTests.test_sorted_discovery_load_and_missing_inputs` (290–314): K.
- `ProofTests.test_discovery_rejects_unexpected_children_filename_and_all_ancestor_symlinks` (316–359): K.
- `ProofTests.test_shared_review_engine_tree_free_and_strict_production_default` (361–390): K.
- `ProofTests.test_review_independence_order_outcomes_times_and_rejected_history` (392–414): K.
- `ProofTests.test_completion_currency_pending_minima_and_extra_stages` (416–447): K.
- `ProofTests.test_full_compact_context_exact_projection_and_nonaliasing` (449–477): K.
- `ProofTests.test_publication_observations_never_replace_current_approvals` (479–491): K.
- `ProofTests.test_frozen_supplemental_shape_and_no_canonical_completion_promotion` (493–514): K.
- `ConversionTests.test_bounded_conversion_enrollment_preservation_report_and_nonaliasing` (520–579): K.
- `ConversionTests.test_conversion_absent_notes_historical_fields_and_claim_refusals` (581–608): K.
- `ConversionTests.test_enrollment_is_evidence_based_and_never_status_age_exemption` (610–629): K.
- `ProofCliTests.test_explicit_inputs_usage_and_read_only_json_serialization` (645–665): K.
- `ProofCliTests.test_one_disposable_full_source_cli_conversion_synthetic_mapping_only` (667–763): K.

`tests/tooling/test_repository_checks.py` blob `65f9ceb461828c42f6040ee20be6e3613c5386fb` at `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`:

- `RepositoryChecks.test_repository_only_never_reads_tracker_or_render` (36–47): K.
- `RepositoryChecks.test_repository_checks_preserve_each_artifact_refusal` (49–67): K.
- `RepositoryChecks.test_tracker_checks_preserve_spec_refs_version_render_assets_anchors_links` (69–100): R for retired tracker spec_refs/render/assets/anchors/HTML-link checks. A for version: replace exact state-version refusal with exact STATUS version declaration/missing/duplicate/mismatch cases. Preserve independent repository Markdown/acceptance-map/spec-ID checks in their existing owners; declare lost per-task spec-ref semantics explicitly. Subject to separate cleanup approval, not this proposal alone.

All other tooling, native, schema, security, accounting, provider and product assertions
are outside removal scope. Preserve acceptance-map completeness/version/decision IDs,
all local evidence/link/containment checks and presets. Preserve actual interpreter,
SDK admission, schema-invalid, witness, OpenAPI components/examples, proof/policy
valid/invalid, mode/blob/path, checkout and merge-side privacy failures. No skip,
WILL_FAIL, blanket continue-on-error or baseline weakening.

## Before/after oracle agreement and review order

Before: current state validity/render/availability, strict current-state paths and
state schema plus shared workflow/publication/repository assertions. After: full
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
removal. The separate scope approval is followed by authored-test independent initial
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
