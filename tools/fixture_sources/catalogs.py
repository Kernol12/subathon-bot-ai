# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent physical-contract literals; never read generated fixture outputs.

Stage 5: sqlite-application-bundle-schema-v1; stage 6: sqlite-application-holds-schema-v1.
Stage 7: sqlite-initial-captures-schema-v1; stage 8: proposed sqlite-account-sessions-v1.
SQL and NULL-index facts retain the reviewed archived corpus verbatim.
"""
from .common import load_seeds, manifest, schema_key


# Each non-NULL declaration is (type, name, table_name, exact SQL). Statement order
# is execution order; NULL index names below are separately supplied facts.
ADDITIONS = {
    5: (
        'atomic-application-bundles-v1',
        (
            ('table', 'subathon_app_states', 'subathon_app_states',
             "CREATE TABLE subathon_app_states(token BLOB NOT NULL CHECK(typeof(token)='blob' AND "
             "length(token)>0), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND "
             "length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), configuration_ref "
             "BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0), lifecycle"
             " INTEGER NOT NULL CHECK(typeof(lifecycle)='integer' AND lifecycle>=0 AND lifecycle<=2), phase "
             "INTEGER NOT NULL CHECK(typeof(phase)='integer' AND phase>=0 AND phase<=1), main_remaining_ms "
             "INTEGER NOT NULL CHECK(typeof(main_remaining_ms)='integer' AND main_remaining_ms>=0), "
             "grace_remaining_ms INTEGER NOT NULL CHECK(typeof(grace_remaining_ms)='integer' AND "
             'grace_remaining_ms>=0), awaiting_confirmation INTEGER NOT NULL '
             "CHECK(typeof(awaiting_confirmation)='integer' AND awaiting_confirmation>=0 AND "
             "awaiting_confirmation<=1), time_debt_ms INTEGER NOT NULL CHECK(typeof(time_debt_ms)='integer' "
             "AND time_debt_ms>=0), balance_count INTEGER NOT NULL CHECK(typeof(balance_count)='integer' AND "
             'balance_count>=0), PRIMARY KEY(token), FOREIGN KEY(configuration_ref) REFERENCES '
             'subathon_retained_configurations(reference), CHECK(phase<>1 OR (main_remaining_ms=0 AND '
             'awaiting_confirmation=0)), CHECK(awaiting_confirmation=0 OR (lifecycle=1 AND phase=0 AND '
             'main_remaining_ms=0)))'),
            ('table', 'subathon_app_balances', 'subathon_app_balances',
             "CREATE TABLE subathon_app_balances(token BLOB NOT NULL CHECK(typeof(token)='blob' AND "
             "length(token)>0), balance_kind INTEGER NOT NULL CHECK(typeof(balance_kind)='integer' AND "
             "balance_kind>=2 AND balance_kind<=3), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob' "
             "AND length(target_ref)>0), units INTEGER NOT NULL CHECK(typeof(units)='integer'), PRIMARY "
             'KEY(token,balance_kind,target_ref), FOREIGN KEY(token) REFERENCES subathon_app_states(token))'),
            ('table', 'subathon_app_head', 'subathon_app_head',
             'CREATE TABLE subathon_app_head(singleton INTEGER PRIMARY KEY CHECK(singleton=1), token BLOB NOT '
             "NULL CHECK(typeof(token)='blob' AND length(token)>0), FOREIGN KEY(token) REFERENCES "
             'subathon_app_states(token))'),
            ('table', 'subathon_app_operations', 'subathon_app_operations',
             'CREATE TABLE subathon_app_operations(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), operation_kind INTEGER NOT "
             "NULL CHECK(typeof(operation_kind)='integer' AND operation_kind>=0 AND operation_kind<=1), "
             "subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' AND subject_kind>=0 AND "
             "subject_kind<=1), receipt_id BLOB NOT NULL CHECK(typeof(receipt_id)='blob' AND "
             "length(receipt_id)=8 AND receipt_id<>X'0000000000000000'), contribution_id BLOB "
             "CHECK(contribution_id IS NULL OR (typeof(contribution_id)='blob' AND "
             "length(contribution_id)=8)), plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND "
             "length(plan_ref)>0), plan_revision BLOB NOT NULL CHECK(typeof(plan_revision)='blob' AND "
             "length(plan_revision)=8 AND plan_revision<>X'0000000000000000'), head_token BLOB NOT NULL "
             "CHECK(typeof(head_token)='blob' AND length(head_token)>0), before_token BLOB NOT NULL "
             "CHECK(typeof(before_token)='blob' AND length(before_token)>0), after_token BLOB NOT NULL "
             "CHECK(typeof(after_token)='blob' AND length(after_token)>0), advance_explanation BLOB NOT NULL "
             "CHECK(typeof(advance_explanation)='blob'), choices_format_ref BLOB NOT NULL "
             "CHECK(typeof(choices_format_ref)='blob' AND length(choices_format_ref)>0), choices_document BLOB"
             " NOT NULL CHECK(typeof(choices_document)='blob'), reversal_ref BLOB CHECK(reversal_ref IS NULL "
             "OR (typeof(reversal_ref)='blob' AND length(reversal_ref)>0)), PRIMARY "
             'KEY(operation_scope,operation_ref), UNIQUE(after_token), UNIQUE(operation_kind,receipt_id), '
             'UNIQUE(operation_scope,reversal_ref), '
             'UNIQUE(operation_scope,operation_ref,plan_ref,plan_revision), '
             'UNIQUE(operation_scope,operation_ref,before_token), FOREIGN KEY(receipt_id) REFERENCES '
             'subathon_receipts(receipt_id), FOREIGN KEY(contribution_id) REFERENCES '
             'subathon_contributions(contribution_id), FOREIGN KEY(plan_ref,plan_revision) REFERENCES '
             'subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(head_token) REFERENCES '
             'subathon_app_states(token), FOREIGN KEY(before_token) REFERENCES subathon_app_states(token), '
             'FOREIGN KEY(after_token) REFERENCES subathon_app_states(token), CHECK((subject_kind=0 AND '
             'contribution_id IS NOT NULL) OR (subject_kind=1 AND contribution_id IS NULL)), '
             'CHECK((operation_kind=0 AND reversal_ref IS NULL) OR (operation_kind=1 AND reversal_ref IS NOT '
             'NULL)), CHECK(before_token<>after_token AND head_token<>after_token))'),
            ('table', 'subathon_app_contributions', 'subathon_app_contributions',
             'CREATE TABLE subathon_app_contributions(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), contribution_id BLOB NOT NULL "
             "CHECK(typeof(contribution_id)='blob' AND length(contribution_id)=8), payment_evidence INTEGER "
             "NOT NULL CHECK(typeof(payment_evidence)='integer' AND payment_evidence>=0 AND "
             "payment_evidence<=3), monetary INTEGER NOT NULL CHECK(typeof(monetary)='integer' AND monetary>=0"
             ' AND monetary<=1), original_decimal BLOB CHECK(original_decimal IS NULL OR '
             "(typeof(original_decimal)='blob')), currency BLOB CHECK(currency IS NULL OR "
             "(typeof(currency)='blob')), resolution_ref BLOB CHECK(resolution_ref IS NULL OR "
             "(typeof(resolution_ref)='blob')), missing_rate INTEGER NOT NULL "
             "CHECK(typeof(missing_rate)='integer' AND missing_rate>=0 AND missing_rate<=1), review INTEGER "
             "NOT NULL CHECK(typeof(review)='integer' AND review>=0 AND review<=3), evidence_policy INTEGER "
             "NOT NULL CHECK(typeof(evidence_policy)='integer' AND evidence_policy>=0 AND evidence_policy<=2),"
             " policy_ref BLOB NOT NULL CHECK(typeof(policy_ref)='blob'), decision_choice INTEGER "
             "CHECK(decision_choice IS NULL OR (typeof(decision_choice)='integer' AND decision_choice>=0 AND "
             'decision_choice<=1)), decision_ref BLOB CHECK(decision_ref IS NULL OR '
             "(typeof(decision_ref)='blob')), decision_actor_ref BLOB CHECK(decision_actor_ref IS NULL OR "
             "(typeof(decision_actor_ref)='blob')), application INTEGER NOT NULL "
             "CHECK(typeof(application)='integer' AND application>=0 AND application<=1), reversal INTEGER NOT"
             " NULL CHECK(typeof(reversal)='integer' AND reversal>=0 AND reversal<=2), settlement INTEGER NOT "
             "NULL CHECK(typeof(settlement)='integer' AND settlement>=0 AND settlement<=1), application_after "
             "INTEGER NOT NULL CHECK(typeof(application_after)='integer' AND application_after>=0 AND "
             'application_after<=1), PRIMARY KEY(operation_scope,operation_ref), FOREIGN '
             'KEY(operation_scope,operation_ref) REFERENCES '
             'subathon_app_operations(operation_scope,operation_ref), FOREIGN KEY(contribution_id) REFERENCES '
             'subathon_contributions(contribution_id), CHECK((original_decimal IS NULL AND currency IS NULL) '
             "OR (typeof(original_decimal)='blob' AND typeof(currency)='blob')), CHECK((decision_choice IS "
             'NULL AND decision_ref IS NULL AND decision_actor_ref IS NULL) OR '
             "(typeof(decision_choice)='integer' AND decision_choice IN (0,1) AND typeof(decision_ref)='blob' "
             "AND typeof(decision_actor_ref)='blob')))"),
            ('table', 'subathon_app_claims', 'subathon_app_claims',
             'CREATE TABLE subathon_app_claims(subject_kind INTEGER NOT NULL '
             "CHECK(typeof(subject_kind)='integer' AND subject_kind>=0 AND subject_kind<=1), subject_id BLOB "
             "NOT NULL CHECK(typeof(subject_id)='blob' AND length(subject_id)=8), operation_scope BLOB NOT "
             "NULL CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT"
             " NULL CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), PRIMARY "
             'KEY(subject_kind,subject_id), UNIQUE(operation_scope,operation_ref), FOREIGN '
             'KEY(operation_scope,operation_ref) REFERENCES '
             'subathon_app_operations(operation_scope,operation_ref), CHECK(subject_kind=0 OR '
             "subject_id<>X'0000000000000000'))"),
            ('table', 'subathon_app_effects', 'subathon_app_effects',
             'CREATE TABLE subathon_app_effects(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal INTEGER NOT "
             "NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), plan_ref BLOB NOT NULL "
             "CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), plan_revision BLOB NOT NULL "
             "CHECK(typeof(plan_revision)='blob' AND length(plan_revision)=8 AND "
             "plan_revision<>X'0000000000000000'), arithmetic_role INTEGER NOT NULL "
             "CHECK(typeof(arithmetic_role)='integer' AND arithmetic_role>=0 AND arithmetic_role<=2), unit "
             "INTEGER NOT NULL CHECK(typeof(unit)='integer' AND unit>=0 AND unit<=2), exact_numerator BLOB NOT"
             " NULL CHECK(typeof(exact_numerator)='blob' AND length(exact_numerator)>0), exact_denominator "
             "BLOB NOT NULL CHECK(typeof(exact_denominator)='blob' AND length(exact_denominator)>0), "
             "requested_units INTEGER NOT NULL CHECK(typeof(requested_units)='integer'), residual_numerator "
             "BLOB NOT NULL CHECK(typeof(residual_numerator)='blob' AND length(residual_numerator)>0), "
             "residual_denominator BLOB NOT NULL CHECK(typeof(residual_denominator)='blob' AND "
             'length(residual_denominator)>0), maximum_main_ms INTEGER CHECK(maximum_main_ms IS NULL OR '
             "(typeof(maximum_main_ms)='integer' AND maximum_main_ms>=0)), deficit_policy INTEGER "
             "CHECK(deficit_policy IS NULL OR (typeof(deficit_policy)='integer' AND deficit_policy>=0 AND "
             'deficit_policy<=2)), deficit_disposition INTEGER CHECK(deficit_disposition IS NULL OR '
             "(typeof(deficit_disposition)='integer' AND deficit_disposition>=0 AND deficit_disposition<=1)), "
             "application_explanation BLOB NOT NULL CHECK(typeof(application_explanation)='blob'), PRIMARY "
             'KEY(operation_scope,operation_ref,request_ordinal), FOREIGN '
             'KEY(operation_scope,operation_ref,plan_ref,plan_revision) REFERENCES '
             'subathon_app_operations(operation_scope,operation_ref,plan_ref,plan_revision), FOREIGN '
             'KEY(plan_ref,plan_revision,request_ordinal) REFERENCES '
             'subathon_plan_requests(plan_ref,revision,ordinal), CHECK((arithmetic_role=0 AND unit IN (1,2) '
             'AND maximum_main_ms IS NULL AND deficit_policy IS NULL AND deficit_disposition IS NULL) OR '
             '(arithmetic_role=1 AND unit=0 AND deficit_policy IS NULL AND deficit_disposition IS NULL) OR '
             '(arithmetic_role=2 AND unit=0 AND maximum_main_ms IS NULL AND deficit_policy IS NOT NULL AND '
             'deficit_disposition IS NOT NULL)))'),
            ('table', 'subathon_app_components', 'subathon_app_components',
             'CREATE TABLE subathon_app_components(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal INTEGER NOT "
             "NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), component_kind INTEGER NOT"
             " NULL CHECK(typeof(component_kind)='integer' AND component_kind>=0 AND component_kind<=5), units"
             " INTEGER NOT NULL CHECK(typeof(units)='integer'), before_token BLOB NOT NULL "
             "CHECK(typeof(before_token)='blob' AND length(before_token)>0), target_ref BLOB CHECK(target_ref "
             "IS NULL OR (typeof(target_ref)='blob' AND length(target_ref)>0)), PRIMARY "
             'KEY(operation_scope,operation_ref,request_ordinal,component_kind), FOREIGN '
             'KEY(operation_scope,operation_ref,request_ordinal) REFERENCES '
             'subathon_app_effects(operation_scope,operation_ref,request_ordinal), FOREIGN '
             'KEY(operation_scope,operation_ref,before_token) REFERENCES '
             'subathon_app_operations(operation_scope,operation_ref,before_token), FOREIGN '
             'KEY(before_token,component_kind,target_ref) REFERENCES '
             'subathon_app_balances(token,balance_kind,target_ref), CHECK((component_kind IN (2,3) AND '
             'target_ref IS NOT NULL) OR (component_kind NOT IN (2,3) AND target_ref IS NULL)))'),
            ('table', 'subathon_app_transitions', 'subathon_app_transitions',
             'CREATE TABLE subathon_app_transitions(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), ordinal INTEGER NOT NULL "
             "CHECK(typeof(ordinal)='integer' AND ordinal>=1), transition INTEGER NOT NULL "
             "CHECK(typeof(transition)='integer' AND transition>=0 AND transition<=5), PRIMARY "
             'KEY(operation_scope,operation_ref,ordinal), FOREIGN KEY(operation_scope,operation_ref) '
             'REFERENCES subathon_app_operations(operation_scope,operation_ref))'),
            ('table', 'subathon_app_jobs', 'subathon_app_jobs',
             'CREATE TABLE subathon_app_jobs(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), ordinal INTEGER NOT NULL "
             "CHECK(typeof(ordinal)='integer' AND ordinal>=1), job_ref BLOB NOT NULL "
             "CHECK(typeof(job_ref)='blob' AND length(job_ref)>0), source_request_ordinal INTEGER "
             "CHECK(source_request_ordinal IS NULL OR (typeof(source_request_ordinal)='integer' AND "
             "source_request_ordinal>=1)), group_ref BLOB CHECK(group_ref IS NULL OR (typeof(group_ref)='blob'"
             ' AND length(group_ref)>0)), group_ordinal INTEGER CHECK(group_ordinal IS NULL OR '
             "(typeof(group_ordinal)='integer' AND group_ordinal>=1)), format_ref BLOB NOT NULL "
             "CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL "
             "CHECK(typeof(document)='blob'), PRIMARY KEY(operation_scope,operation_ref,ordinal), "
             'UNIQUE(operation_scope,operation_ref,job_ref), '
             'UNIQUE(operation_scope,operation_ref,group_ref,group_ordinal), FOREIGN '
             'KEY(operation_scope,operation_ref) REFERENCES '
             'subathon_app_operations(operation_scope,operation_ref), FOREIGN '
             'KEY(operation_scope,operation_ref,source_request_ordinal) REFERENCES '
             'subathon_app_effects(operation_scope,operation_ref,request_ordinal), CHECK((group_ref IS NULL '
             'AND group_ordinal IS NULL) OR (group_ref IS NOT NULL AND group_ordinal IS NOT NULL)))'),
            ('table', 'subathon_app_results', 'subathon_app_results',
             'CREATE TABLE subathon_app_results(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), format_ref BLOB NOT NULL "
             "CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document BLOB NOT NULL "
             "CHECK(typeof(document)='blob'), effect_count INTEGER NOT NULL "
             "CHECK(typeof(effect_count)='integer' AND effect_count>=0), component_count INTEGER NOT NULL "
             "CHECK(typeof(component_count)='integer' AND component_count>=0), job_count INTEGER NOT NULL "
             "CHECK(typeof(job_count)='integer' AND job_count>=0), transition_count INTEGER NOT NULL "
             "CHECK(typeof(transition_count)='integer' AND transition_count>=0), reversal_count INTEGER NOT "
             "NULL CHECK(typeof(reversal_count)='integer' AND reversal_count>=0), PRIMARY "
             'KEY(operation_scope,operation_ref), FOREIGN KEY(operation_scope,operation_ref) REFERENCES '
             'subathon_app_operations(operation_scope,operation_ref))'),
            ('table', 'subathon_app_audit', 'subathon_app_audit',
             'CREATE TABLE subathon_app_audit(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), actor_ref BLOB CHECK(actor_ref"
             " IS NULL OR (typeof(actor_ref)='blob')), decision_ref BLOB CHECK(decision_ref IS NULL OR "
             "(typeof(decision_ref)='blob')), reason BLOB CHECK(reason IS NULL OR (typeof(reason)='blob')), "
             "format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND length(format_ref)>0), document "
             "BLOB NOT NULL CHECK(typeof(document)='blob'), PRIMARY KEY(operation_scope,operation_ref), "
             'FOREIGN KEY(operation_scope,operation_ref) REFERENCES '
             'subathon_app_operations(operation_scope,operation_ref))'),
            ('table', 'subathon_app_reversals', 'subathon_app_reversals',
             'CREATE TABLE subathon_app_reversals(operation_scope BLOB NOT NULL '
             "CHECK(typeof(operation_scope)='blob' AND length(operation_scope)>0), operation_ref BLOB NOT NULL"
             " CHECK(typeof(operation_ref)='blob' AND length(operation_ref)>0), request_ordinal INTEGER NOT "
             "NULL CHECK(typeof(request_ordinal)='integer' AND request_ordinal>=1), original_scope BLOB NOT "
             "NULL CHECK(typeof(original_scope)='blob' AND length(original_scope)>0), original_operation BLOB "
             "NOT NULL CHECK(typeof(original_operation)='blob' AND length(original_operation)>0), "
             "original_request_ordinal INTEGER NOT NULL CHECK(typeof(original_request_ordinal)='integer' AND "
             'original_request_ordinal>=1), original_component_kind INTEGER NOT NULL '
             "CHECK(typeof(original_component_kind)='integer' AND original_component_kind>=0 AND "
             'original_component_kind<=3), basis_position INTEGER NOT NULL '
             "CHECK(typeof(basis_position)='integer' AND basis_position>=1), fraction_numerator BLOB NOT NULL "
             "CHECK(typeof(fraction_numerator)='blob' AND length(fraction_numerator)>0), fraction_denominator "
             "BLOB NOT NULL CHECK(typeof(fraction_denominator)='blob' AND length(fraction_denominator)>0), "
             "entitlement_numerator BLOB NOT NULL CHECK(typeof(entitlement_numerator)='blob' AND "
             'length(entitlement_numerator)>0), entitlement_denominator BLOB NOT NULL '
             "CHECK(typeof(entitlement_denominator)='blob' AND length(entitlement_denominator)>0), "
             "residual_numerator BLOB NOT NULL CHECK(typeof(residual_numerator)='blob' AND "
             'length(residual_numerator)>0), residual_denominator BLOB NOT NULL '
             "CHECK(typeof(residual_denominator)='blob' AND length(residual_denominator)>0), "
             "previous_cumulative INTEGER NOT NULL CHECK(typeof(previous_cumulative)='integer'), cumulative "
             "INTEGER NOT NULL CHECK(typeof(cumulative)='integer'), requested_increment INTEGER NOT NULL "
             "CHECK(typeof(requested_increment)='integer'), full_refund INTEGER NOT NULL "
             "CHECK(typeof(full_refund)='integer' AND full_refund>=0 AND full_refund<=1), PRIMARY "
             'KEY(operation_scope,operation_ref,request_ordinal,original_scope,original_operation,original_request_ordinal,original_component_kind),'
             ' '
             'UNIQUE(original_scope,original_operation,original_request_ordinal,original_component_kind,basis_position),'
             ' FOREIGN KEY(operation_scope,operation_ref,request_ordinal) REFERENCES '
             'subathon_app_effects(operation_scope,operation_ref,request_ordinal), FOREIGN '
             'KEY(original_scope,original_operation,original_request_ordinal,original_component_kind) '
             'REFERENCES '
             'subathon_app_components(operation_scope,operation_ref,request_ordinal,component_kind), '
             'CHECK(original_component_kind IN (0,2,3)))'),
            ('table', 'subathon_app_reversal_heads', 'subathon_app_reversal_heads',
             'CREATE TABLE subathon_app_reversal_heads(original_scope BLOB NOT NULL '
             "CHECK(typeof(original_scope)='blob' AND length(original_scope)>0), original_operation BLOB NOT "
             "NULL CHECK(typeof(original_operation)='blob' AND length(original_operation)>0), "
             "original_request_ordinal INTEGER NOT NULL CHECK(typeof(original_request_ordinal)='integer' AND "
             'original_request_ordinal>=1), original_component_kind INTEGER NOT NULL '
             "CHECK(typeof(original_component_kind)='integer' AND original_component_kind>=0 AND "
             'original_component_kind<=3), basis_position INTEGER NOT NULL '
             "CHECK(typeof(basis_position)='integer' AND basis_position>=1), cumulative INTEGER NOT NULL "
             "CHECK(typeof(cumulative)='integer'), last_scope BLOB NOT NULL CHECK(typeof(last_scope)='blob' "
             "AND length(last_scope)>0), last_operation BLOB NOT NULL CHECK(typeof(last_operation)='blob' AND "
             'length(last_operation)>0), last_request_ordinal INTEGER NOT NULL '
             "CHECK(typeof(last_request_ordinal)='integer' AND last_request_ordinal>=1), PRIMARY "
             'KEY(original_scope,original_operation,original_request_ordinal,original_component_kind), FOREIGN'
             ' KEY(original_scope,original_operation,original_request_ordinal,original_component_kind) '
             'REFERENCES '
             'subathon_app_components(operation_scope,operation_ref,request_ordinal,component_kind), FOREIGN '
             'KEY(original_scope,original_operation,original_request_ordinal,original_component_kind,basis_position)'
             ' REFERENCES '
             'subathon_app_reversals(original_scope,original_operation,original_request_ordinal,original_component_kind,basis_position),'
             ' FOREIGN '
             'KEY(last_scope,last_operation,last_request_ordinal,original_scope,original_operation,original_request_ordinal,original_component_kind)'
             ' REFERENCES '
             'subathon_app_reversals(operation_scope,operation_ref,request_ordinal,original_scope,original_operation,original_request_ordinal,original_component_kind),'
             ' CHECK(original_component_kind IN (0,2,3)))'),
        ),
        (
            ('sqlite_autoindex_subathon_app_audit_1', 'subathon_app_audit'),
            ('sqlite_autoindex_subathon_app_balances_1', 'subathon_app_balances'),
            ('sqlite_autoindex_subathon_app_claims_1', 'subathon_app_claims'),
            ('sqlite_autoindex_subathon_app_claims_2', 'subathon_app_claims'),
            ('sqlite_autoindex_subathon_app_components_1', 'subathon_app_components'),
            ('sqlite_autoindex_subathon_app_contributions_1', 'subathon_app_contributions'),
            ('sqlite_autoindex_subathon_app_effects_1', 'subathon_app_effects'),
            ('sqlite_autoindex_subathon_app_jobs_1', 'subathon_app_jobs'),
            ('sqlite_autoindex_subathon_app_jobs_2', 'subathon_app_jobs'),
            ('sqlite_autoindex_subathon_app_jobs_3', 'subathon_app_jobs'),
            ('sqlite_autoindex_subathon_app_operations_1', 'subathon_app_operations'),
            ('sqlite_autoindex_subathon_app_operations_2', 'subathon_app_operations'),
            ('sqlite_autoindex_subathon_app_operations_3', 'subathon_app_operations'),
            ('sqlite_autoindex_subathon_app_operations_4', 'subathon_app_operations'),
            ('sqlite_autoindex_subathon_app_operations_5', 'subathon_app_operations'),
            ('sqlite_autoindex_subathon_app_operations_6', 'subathon_app_operations'),
            ('sqlite_autoindex_subathon_app_results_1', 'subathon_app_results'),
            ('sqlite_autoindex_subathon_app_reversal_heads_1', 'subathon_app_reversal_heads'),
            ('sqlite_autoindex_subathon_app_reversals_1', 'subathon_app_reversals'),
            ('sqlite_autoindex_subathon_app_reversals_2', 'subathon_app_reversals'),
            ('sqlite_autoindex_subathon_app_states_1', 'subathon_app_states'),
            ('sqlite_autoindex_subathon_app_transitions_1', 'subathon_app_transitions'),
        ),
    ),
    6: (
        'durable-application-holds-v1',
        (
            ('table', 'subathon_hold_mutations', 'subathon_hold_mutations',
             'CREATE TABLE subathon_hold_mutations(mutation_ref BLOB NOT NULL '
             "CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), mutation_kind INTEGER NOT NULL "
             "CHECK(typeof(mutation_kind)='integer' AND mutation_kind BETWEEN 0 AND 1), subject_kind INTEGER "
             "NOT NULL CHECK(typeof(subject_kind)='integer' AND subject_kind BETWEEN 0 AND 1), subject_id BLOB"
             " NOT NULL CHECK(typeof(subject_id)='blob' AND length(subject_id)=8), expected_count INTEGER NOT "
             "NULL CHECK(typeof(expected_count)='integer' AND expected_count>=0), entry_count INTEGER NOT NULL"
             " CHECK(typeof(entry_count)='integer' AND entry_count>=1), actor_ref BLOB CHECK(actor_ref IS NULL"
             " OR (typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS"
             " NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), reason BLOB CHECK(reason IS "
             "NULL OR (typeof(reason)='blob')), options_format_ref BLOB CHECK(options_format_ref IS NULL OR "
             "(typeof(options_format_ref)='blob' AND length(options_format_ref)>0)), options_document BLOB "
             "CHECK(options_document IS NULL OR (typeof(options_document)='blob')), application_link_kind "
             "INTEGER CHECK(application_link_kind IS NULL OR (typeof(application_link_kind)='integer' AND "
             'application_link_kind BETWEEN 0 AND 1)), application_scope BLOB CHECK(application_scope IS NULL '
             "OR (typeof(application_scope)='blob' AND length(application_scope)>0)), application_ref BLOB "
             "CHECK(application_ref IS NULL OR (typeof(application_ref)='blob' AND "
             'length(application_ref)>0)), application_receipt_id BLOB CHECK(application_receipt_id IS NULL OR'
             " (typeof(application_receipt_id)='blob' AND length(application_receipt_id)=8 AND "
             "application_receipt_id<>X'0000000000000000')), application_plan_ref BLOB "
             "CHECK(application_plan_ref IS NULL OR (typeof(application_plan_ref)='blob' AND "
             'length(application_plan_ref)>0)), application_plan_revision BLOB CHECK(application_plan_revision'
             " IS NULL OR (typeof(application_plan_revision)='blob' AND length(application_plan_revision)=8 "
             "AND application_plan_revision<>X'0000000000000000')), held_hold_ref BLOB CHECK(held_hold_ref IS "
             "NULL OR (typeof(held_hold_ref)='blob' AND length(held_hold_ref)>0)), held_version_ref BLOB "
             "CHECK(held_version_ref IS NULL OR (typeof(held_version_ref)='blob' AND "
             'length(held_version_ref)>0)), held_request_ordinal INTEGER CHECK(held_request_ordinal IS NULL OR'
             " (typeof(held_request_ordinal)='integer' AND held_request_ordinal>=1)), held_component_kind "
             "INTEGER CHECK(held_component_kind IS NULL OR (typeof(held_component_kind)='integer' AND "
             'held_component_kind=4)), remaining_units INTEGER CHECK(remaining_units IS NULL OR '
             "(typeof(remaining_units)='integer')), PRIMARY KEY(mutation_ref), CHECK(subject_kind=0 OR "
             "subject_id<>X'0000000000000000'), CHECK((actor_ref IS NULL AND decision_ref IS NULL AND reason "
             'IS NULL AND options_format_ref IS NULL AND options_document IS NULL) OR (actor_ref IS NOT NULL '
             'AND decision_ref IS NOT NULL AND reason IS NOT NULL AND options_format_ref IS NOT NULL AND '
             'options_document IS NOT NULL)), CHECK((application_link_kind IS NULL AND application_scope IS '
             'NULL AND application_ref IS NULL AND application_receipt_id IS NULL AND application_plan_ref IS '
             'NULL AND application_plan_revision IS NULL) OR (application_link_kind IS NOT NULL AND '
             'application_scope IS NOT NULL AND application_ref IS NOT NULL AND application_receipt_id IS NOT '
             'NULL AND application_plan_ref IS NOT NULL AND application_plan_revision IS NOT NULL)), '
             'CHECK((held_hold_ref IS NULL AND held_version_ref IS NULL AND held_request_ordinal IS NULL AND '
             'held_component_kind IS NULL) OR (held_hold_ref IS NOT NULL AND held_version_ref IS NOT NULL AND '
             'held_request_ordinal IS NOT NULL AND held_component_kind IS NOT NULL)), CHECK((mutation_kind=0 '
             'AND expected_count=0 AND entry_count=1 AND actor_ref IS NULL AND application_link_kind IS NULL '
             'AND held_hold_ref IS NULL AND remaining_units IS NULL) OR (mutation_kind=1 AND actor_ref IS NOT '
             'NULL)), CHECK((application_link_kind IS NULL AND held_hold_ref IS NULL AND remaining_units IS '
             'NULL) OR (application_link_kind IS NOT NULL AND ((application_link_kind=0 AND held_hold_ref IS '
             'NULL AND remaining_units IS NULL) OR (application_link_kind=1 AND held_hold_ref IS NOT NULL)))),'
             ' FOREIGN KEY(application_scope,application_ref) REFERENCES '
             'subathon_app_operations(operation_scope,operation_ref), FOREIGN KEY(application_receipt_id) '
             'REFERENCES subathon_receipts(receipt_id), FOREIGN '
             'KEY(application_plan_ref,application_plan_revision) REFERENCES '
             'subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(held_hold_ref,held_version_ref) '
             'REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN '
             'KEY(held_hold_ref,held_request_ordinal) REFERENCES subathon_hold_occurrences(hold_ref,ordinal))'),
            ('table', 'subathon_hold_causes', 'subathon_hold_causes',
             "CREATE TABLE subathon_hold_causes(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND "
             "length(hold_ref)>0), subject_kind INTEGER NOT NULL CHECK(typeof(subject_kind)='integer' AND "
             "subject_kind BETWEEN 0 AND 1), subject_id BLOB NOT NULL CHECK(typeof(subject_id)='blob' AND "
             'length(subject_id)=8), contribution_id BLOB CHECK(contribution_id IS NULL OR '
             "(typeof(contribution_id)='blob' AND length(contribution_id)=8)), source_receipt_id BLOB NOT NULL"
             " CHECK(typeof(source_receipt_id)='blob' AND length(source_receipt_id)=8 AND "
             "source_receipt_id<>X'0000000000000000'), event_ref BLOB CHECK(event_ref IS NULL OR "
             "(typeof(event_ref)='blob')), parent_event_ref BLOB CHECK(parent_event_ref IS NULL OR "
             "(typeof(parent_event_ref)='blob')), root_event_ref BLOB CHECK(root_event_ref IS NULL OR "
             "(typeof(root_event_ref)='blob')), configuration_ref BLOB NOT NULL "
             "CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0), arrival_context_ref "
             "BLOB NOT NULL CHECK(typeof(arrival_context_ref)='blob' AND length(arrival_context_ref)>0), "
             "command_scope BLOB NOT NULL CHECK(typeof(command_scope)='blob' AND length(command_scope)>0), "
             "command_ref BLOB NOT NULL CHECK(typeof(command_ref)='blob' AND length(command_ref)>0), "
             "command_format_ref BLOB NOT NULL CHECK(typeof(command_format_ref)='blob' AND "
             'length(command_format_ref)>0), command_document BLOB NOT NULL '
             "CHECK(typeof(command_document)='blob'), cause_kind INTEGER NOT NULL "
             "CHECK(typeof(cause_kind)='integer' AND cause_kind BETWEEN 0 AND 2), supersedes_hold_ref BLOB "
             "CHECK(supersedes_hold_ref IS NULL OR (typeof(supersedes_hold_ref)='blob' AND "
             'length(supersedes_hold_ref)>0)), first_detection_ref BLOB NOT NULL '
             "CHECK(typeof(first_detection_ref)='blob' AND length(first_detection_ref)>0), plan_evidence_kind "
             "INTEGER NOT NULL CHECK(typeof(plan_evidence_kind)='integer' AND plan_evidence_kind BETWEEN 0 AND"
             " 1), plan_ref BLOB CHECK(plan_ref IS NULL OR (typeof(plan_ref)='blob' AND length(plan_ref)>0)), "
             "plan_revision BLOB CHECK(plan_revision IS NULL OR (typeof(plan_revision)='blob' AND "
             "length(plan_revision)=8 AND plan_revision<>X'0000000000000000')), extent INTEGER NOT NULL "
             "CHECK(typeof(extent)='integer' AND extent BETWEEN 0 AND 2), stopped_for INTEGER "
             "CHECK(stopped_for IS NULL OR (typeof(stopped_for)='integer' AND stopped_for BETWEEN 0 AND 1)), "
             'failed_capture_format_ref BLOB CHECK(failed_capture_format_ref IS NULL OR '
             "(typeof(failed_capture_format_ref)='blob' AND length(failed_capture_format_ref)>0)), "
             'failed_capture_document BLOB CHECK(failed_capture_document IS NULL OR '
             "(typeof(failed_capture_document)='blob')), occurrence_count INTEGER NOT NULL "
             "CHECK(typeof(occurrence_count)='integer' AND occurrence_count>=0), failure_stage INTEGER "
             "CHECK(failure_stage IS NULL OR (typeof(failure_stage)='integer' AND failure_stage BETWEEN 0 AND "
             "1)), failing_ordinal INTEGER CHECK(failing_ordinal IS NULL OR (typeof(failing_ordinal)='integer'"
             ' AND failing_ordinal>=1)), failing_target_ref BLOB CHECK(failing_target_ref IS NULL OR '
             "(typeof(failing_target_ref)='blob')), failure_numerator BLOB CHECK(failure_numerator IS NULL OR "
             "(typeof(failure_numerator)='blob' AND length(failure_numerator)>0)), failure_denominator BLOB "
             "CHECK(failure_denominator IS NULL OR (typeof(failure_denominator)='blob' AND "
             'length(failure_denominator)>0)), failure_format_ref BLOB CHECK(failure_format_ref IS NULL OR '
             "(typeof(failure_format_ref)='blob' AND length(failure_format_ref)>0)), failure_document BLOB "
             "CHECK(failure_document IS NULL OR (typeof(failure_document)='blob')), timer_request_ordinal "
             "INTEGER CHECK(timer_request_ordinal IS NULL OR (typeof(timer_request_ordinal)='integer' AND "
             'timer_request_ordinal>=1)), timer_component_kind INTEGER CHECK(timer_component_kind IS NULL OR '
             "(typeof(timer_component_kind)='integer' AND timer_component_kind=4)), timer_request_units "
             "INTEGER CHECK(timer_request_units IS NULL OR (typeof(timer_request_units)='integer' AND "
             'timer_request_units<0)), deficit_policy INTEGER CHECK(deficit_policy IS NULL OR '
             "(typeof(deficit_policy)='integer' AND deficit_policy=2)), deficit_disposition INTEGER "
             "CHECK(deficit_disposition IS NULL OR (typeof(deficit_disposition)='integer' AND "
             'deficit_disposition=1)), unapplied_units INTEGER CHECK(unapplied_units IS NULL OR '
             "(typeof(unapplied_units)='integer' AND unapplied_units<0)), composition INTEGER "
             "CHECK(composition IS NULL OR (typeof(composition)='integer' AND composition BETWEEN 0 AND 1)), "
             'composition_format_ref BLOB CHECK(composition_format_ref IS NULL OR '
             "(typeof(composition_format_ref)='blob' AND length(composition_format_ref)>0)), "
             'composition_document BLOB CHECK(composition_document IS NULL OR '
             "(typeof(composition_document)='blob')), missing_policy_reason BLOB CHECK(missing_policy_reason "
             "IS NULL OR (typeof(missing_policy_reason)='blob')), associated_scope BLOB CHECK(associated_scope"
             " IS NULL OR (typeof(associated_scope)='blob' AND length(associated_scope)>0)), associated_ref "
             "BLOB CHECK(associated_ref IS NULL OR (typeof(associated_ref)='blob' AND "
             'length(associated_ref)>0)), associated_request_ordinal INTEGER CHECK(associated_request_ordinal '
             "IS NULL OR (typeof(associated_request_ordinal)='integer' AND associated_request_ordinal>=1)), "
             "PRIMARY KEY(hold_ref), CHECK(subject_kind=0 OR subject_id<>X'0000000000000000'), "
             'CHECK((subject_kind=0 AND contribution_id IS NOT NULL AND contribution_id=subject_id) OR '
             '(subject_kind=1 AND contribution_id IS NULL AND subject_id=source_receipt_id)), CHECK((plan_ref '
             'IS NULL AND plan_revision IS NULL) OR (plan_ref IS NOT NULL AND plan_revision IS NOT NULL)), '
             'CHECK((failed_capture_format_ref IS NULL AND failed_capture_document IS NULL) OR '
             '(failed_capture_format_ref IS NOT NULL AND failed_capture_document IS NOT NULL)), '
             'CHECK((failure_numerator IS NULL AND failure_denominator IS NULL) OR (failure_numerator IS NOT '
             'NULL AND failure_denominator IS NOT NULL)), CHECK((failure_stage IS NULL AND failure_format_ref '
             'IS NULL AND failure_document IS NULL) OR (failure_stage IS NOT NULL AND failure_format_ref IS '
             'NOT NULL AND failure_document IS NOT NULL)), CHECK((timer_request_ordinal IS NULL AND '
             'timer_component_kind IS NULL AND timer_request_units IS NULL AND deficit_policy IS NULL AND '
             'deficit_disposition IS NULL AND unapplied_units IS NULL AND composition IS NULL AND '
             'composition_format_ref IS NULL AND composition_document IS NULL) OR (timer_request_ordinal IS '
             'NOT NULL AND timer_component_kind IS NOT NULL AND timer_request_units IS NOT NULL AND '
             'deficit_policy IS NOT NULL AND deficit_disposition IS NOT NULL AND unapplied_units IS NOT NULL '
             'AND composition IS NOT NULL AND composition_format_ref IS NOT NULL AND composition_document IS '
             'NOT NULL)), CHECK((associated_scope IS NULL AND associated_ref IS NULL AND '
             'associated_request_ordinal IS NULL) OR (associated_scope IS NOT NULL AND associated_ref IS NOT '
             'NULL AND associated_request_ordinal IS NOT NULL)), CHECK(supersedes_hold_ref IS NULL OR '
             'supersedes_hold_ref<>hold_ref), CHECK((plan_evidence_kind=0 AND plan_ref IS NOT NULL AND '
             'extent=0 AND stopped_for IS NULL AND failed_capture_format_ref IS NULL) OR (plan_evidence_kind=1'
             ' AND extent IN (1,2) AND stopped_for IS NOT NULL AND failed_capture_format_ref IS NOT NULL)), '
             'CHECK((cause_kind=0 AND failure_stage IS NOT NULL AND timer_request_ordinal IS NULL AND '
             'missing_policy_reason IS NULL AND associated_scope IS NULL AND ((plan_evidence_kind=0 AND '
             'failure_stage=1) OR (plan_evidence_kind=1 AND stopped_for=0 AND failure_stage=0))) OR '
             '(cause_kind=1 AND plan_evidence_kind=0 AND failure_stage IS NULL AND failing_ordinal IS NULL AND'
             ' failing_target_ref IS NULL AND failure_numerator IS NULL AND timer_request_ordinal IS NOT NULL '
             'AND unapplied_units=timer_request_units AND missing_policy_reason IS NULL AND ((composition=0 '
             'AND associated_scope IS NULL) OR (composition=1 AND associated_scope IS NOT NULL AND '
             'associated_request_ordinal=timer_request_ordinal))) OR (cause_kind=2 AND failure_stage IS NULL '
             'AND failing_ordinal IS NULL AND failing_target_ref IS NULL AND failure_numerator IS NULL AND '
             'timer_request_ordinal IS NULL AND missing_policy_reason IS NOT NULL AND associated_scope IS NULL'
             ' AND (plan_evidence_kind=0 OR stopped_for=1))), FOREIGN KEY(contribution_id) REFERENCES '
             'subathon_contributions(contribution_id), FOREIGN KEY(source_receipt_id) REFERENCES '
             'subathon_receipts(receipt_id), FOREIGN KEY(configuration_ref) REFERENCES '
             'subathon_retained_configurations(reference), FOREIGN KEY(arrival_context_ref) REFERENCES '
             'subathon_retained_contexts(reference), FOREIGN KEY(supersedes_hold_ref) REFERENCES '
             'subathon_hold_causes(hold_ref), FOREIGN KEY(first_detection_ref) REFERENCES '
             'subathon_hold_mutations(mutation_ref), FOREIGN KEY(plan_ref,plan_revision) REFERENCES '
             'subathon_plan_revisions(plan_ref,revision), FOREIGN '
             'KEY(associated_scope,associated_ref,associated_request_ordinal) REFERENCES '
             'subathon_app_effects(operation_scope,operation_ref,request_ordinal))'),
            ('table', 'subathon_hold_cause_observations', 'subathon_hold_cause_observations',
             'CREATE TABLE subathon_hold_cause_observations(hold_ref BLOB NOT NULL '
             "CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), persisted_state_token BLOB "
             "CHECK(persisted_state_token IS NULL OR (typeof(persisted_state_token)='blob' AND "
             "length(persisted_state_token)>0)), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND "
             "length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), "
             'policy_revision_ref BLOB CHECK(policy_revision_ref IS NULL OR '
             "(typeof(policy_revision_ref)='blob' AND length(policy_revision_ref)>0)), permission_revision_ref"
             " BLOB CHECK(permission_revision_ref IS NULL OR (typeof(permission_revision_ref)='blob' AND "
             'length(permission_revision_ref)>0)), actor_ref BLOB CHECK(actor_ref IS NULL OR '
             "(typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS "
             "NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), observed_utc_ms INTEGER "
             "CHECK(observed_utc_ms IS NULL OR (typeof(observed_utc_ms)='integer')), elapsed_ms INTEGER "
             "CHECK(elapsed_ms IS NULL OR (typeof(elapsed_ms)='integer' AND elapsed_ms>=0)), PRIMARY "
             'KEY(hold_ref), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref), FOREIGN '
             'KEY(persisted_state_token) REFERENCES subathon_app_states(token))'),
            ('table', 'subathon_hold_occurrences', 'subathon_hold_occurrences',
             "CREATE TABLE subathon_hold_occurrences(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND "
             "length(hold_ref)>0), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>=1), "
             "action_ref BLOB CHECK(action_ref IS NULL OR (typeof(action_ref)='blob')), action_version_ref "
             "BLOB CHECK(action_version_ref IS NULL OR (typeof(action_version_ref)='blob')), rule_ref BLOB "
             "CHECK(rule_ref IS NULL OR (typeof(rule_ref)='blob')), target_ref BLOB CHECK(target_ref IS NULL "
             "OR (typeof(target_ref)='blob')), unit INTEGER CHECK(unit IS NULL OR (typeof(unit)='integer' AND "
             'unit BETWEEN 0 AND 2)), attempted_numerator BLOB CHECK(attempted_numerator IS NULL OR '
             "(typeof(attempted_numerator)='blob' AND length(attempted_numerator)>0)), attempted_denominator "
             "BLOB CHECK(attempted_denominator IS NULL OR (typeof(attempted_denominator)='blob' AND "
             'length(attempted_denominator)>0)), finalized_units INTEGER CHECK(finalized_units IS NULL OR '
             "(typeof(finalized_units)='integer')), explanation BLOB CHECK(explanation IS NULL OR "
             "(typeof(explanation)='blob')), PRIMARY KEY(hold_ref,ordinal), CHECK((attempted_numerator IS NULL"
             ' AND attempted_denominator IS NULL) OR (attempted_numerator IS NOT NULL AND '
             'attempted_denominator IS NOT NULL)), FOREIGN KEY(hold_ref) REFERENCES '
             'subathon_hold_causes(hold_ref))'),
            ('table', 'subathon_hold_detections', 'subathon_hold_detections',
             'CREATE TABLE subathon_hold_detections(mutation_ref BLOB NOT NULL '
             "CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL "
             "CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), expected_version_ref BLOB "
             "CHECK(expected_version_ref IS NULL OR (typeof(expected_version_ref)='blob' AND "
             'length(expected_version_ref)>0)), proposed_version_ref BLOB NOT NULL '
             "CHECK(typeof(proposed_version_ref)='blob' AND length(proposed_version_ref)>0), audit_format_ref "
             "BLOB NOT NULL CHECK(typeof(audit_format_ref)='blob' AND length(audit_format_ref)>0), "
             "audit_document BLOB NOT NULL CHECK(typeof(audit_document)='blob'), PRIMARY KEY(mutation_ref), "
             'FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN '
             'KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref))'),
            ('table', 'subathon_hold_detection_observations', 'subathon_hold_detection_observations',
             'CREATE TABLE subathon_hold_detection_observations(mutation_ref BLOB NOT NULL '
             "CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), persisted_state_token BLOB "
             "CHECK(persisted_state_token IS NULL OR (typeof(persisted_state_token)='blob' AND "
             "length(persisted_state_token)>0)), format_ref BLOB NOT NULL CHECK(typeof(format_ref)='blob' AND "
             "length(format_ref)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), "
             'policy_revision_ref BLOB CHECK(policy_revision_ref IS NULL OR '
             "(typeof(policy_revision_ref)='blob' AND length(policy_revision_ref)>0)), permission_revision_ref"
             " BLOB CHECK(permission_revision_ref IS NULL OR (typeof(permission_revision_ref)='blob' AND "
             'length(permission_revision_ref)>0)), actor_ref BLOB CHECK(actor_ref IS NULL OR '
             "(typeof(actor_ref)='blob' AND length(actor_ref)>0)), decision_ref BLOB CHECK(decision_ref IS "
             "NULL OR (typeof(decision_ref)='blob' AND length(decision_ref)>0)), observed_utc_ms INTEGER "
             "CHECK(observed_utc_ms IS NULL OR (typeof(observed_utc_ms)='integer')), elapsed_ms INTEGER "
             "CHECK(elapsed_ms IS NULL OR (typeof(elapsed_ms)='integer' AND elapsed_ms>=0)), PRIMARY "
             'KEY(mutation_ref), FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_detections(mutation_ref), '
             'FOREIGN KEY(persisted_state_token) REFERENCES subathon_app_states(token))'),
            ('table', 'subathon_hold_versions', 'subathon_hold_versions',
             "CREATE TABLE subathon_hold_versions(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND "
             "length(hold_ref)>0), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND "
             "length(version_ref)>0), position INTEGER NOT NULL CHECK(typeof(position)='integer' AND "
             'position>=1), parent_version_ref BLOB CHECK(parent_version_ref IS NULL OR '
             "(typeof(parent_version_ref)='blob' AND length(parent_version_ref)>0)), state INTEGER NOT NULL "
             "CHECK(typeof(state)='integer' AND state BETWEEN 0 AND 1), mutation_ref BLOB NOT NULL "
             "CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), replacement_hold_ref BLOB "
             "CHECK(replacement_hold_ref IS NULL OR (typeof(replacement_hold_ref)='blob' AND "
             'length(replacement_hold_ref)>0)), replacement_detection_ref BLOB CHECK(replacement_detection_ref'
             " IS NULL OR (typeof(replacement_detection_ref)='blob' AND length(replacement_detection_ref)>0)),"
             ' PRIMARY KEY(hold_ref,version_ref), UNIQUE(hold_ref,position), CHECK((replacement_hold_ref IS '
             'NULL AND replacement_detection_ref IS NULL) OR (replacement_hold_ref IS NOT NULL AND '
             'replacement_detection_ref IS NOT NULL)), CHECK((position=1 AND parent_version_ref IS NULL AND '
             'state=0) OR (position>1 AND parent_version_ref IS NOT NULL)), CHECK(state=1 OR '
             'replacement_hold_ref IS NULL), FOREIGN KEY(hold_ref) REFERENCES subathon_hold_causes(hold_ref), '
             'FOREIGN KEY(hold_ref,parent_version_ref) REFERENCES '
             'subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(mutation_ref) REFERENCES '
             'subathon_hold_mutations(mutation_ref), FOREIGN KEY(replacement_hold_ref) REFERENCES '
             'subathon_hold_causes(hold_ref), FOREIGN KEY(replacement_detection_ref) REFERENCES '
             'subathon_hold_detections(mutation_ref))'),
            ('table', 'subathon_hold_heads', 'subathon_hold_heads',
             "CREATE TABLE subathon_hold_heads(hold_ref BLOB NOT NULL CHECK(typeof(hold_ref)='blob' AND "
             "length(hold_ref)>0), version_ref BLOB NOT NULL CHECK(typeof(version_ref)='blob' AND "
             "length(version_ref)>0), version_count INTEGER NOT NULL CHECK(typeof(version_count)='integer' AND"
             ' version_count>=1), PRIMARY KEY(hold_ref), FOREIGN KEY(hold_ref) REFERENCES '
             'subathon_hold_causes(hold_ref), FOREIGN KEY(hold_ref,version_ref) REFERENCES '
             'subathon_hold_versions(hold_ref,version_ref))'),
            ('table', 'subathon_hold_resolution_expected', 'subathon_hold_resolution_expected',
             'CREATE TABLE subathon_hold_resolution_expected(mutation_ref BLOB NOT NULL '
             "CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL "
             "CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), version_ref BLOB NOT NULL "
             "CHECK(typeof(version_ref)='blob' AND length(version_ref)>0), PRIMARY KEY(mutation_ref,hold_ref),"
             ' FOREIGN KEY(mutation_ref) REFERENCES subathon_hold_mutations(mutation_ref), FOREIGN '
             'KEY(hold_ref,version_ref) REFERENCES subathon_hold_versions(hold_ref,version_ref))'),
            ('table', 'subathon_hold_resolution_entries', 'subathon_hold_resolution_entries',
             'CREATE TABLE subathon_hold_resolution_entries(mutation_ref BLOB NOT NULL '
             "CHECK(typeof(mutation_ref)='blob' AND length(mutation_ref)>0), hold_ref BLOB NOT NULL "
             "CHECK(typeof(hold_ref)='blob' AND length(hold_ref)>0), expected_version_ref BLOB NOT NULL "
             "CHECK(typeof(expected_version_ref)='blob' AND length(expected_version_ref)>0), "
             "proposed_version_ref BLOB NOT NULL CHECK(typeof(proposed_version_ref)='blob' AND "
             'length(proposed_version_ref)>0), decision_format_ref BLOB NOT NULL '
             "CHECK(typeof(decision_format_ref)='blob' AND length(decision_format_ref)>0), decision_document "
             "BLOB NOT NULL CHECK(typeof(decision_document)='blob'), replacement_hold_ref BLOB "
             "CHECK(replacement_hold_ref IS NULL OR (typeof(replacement_hold_ref)='blob' AND "
             'length(replacement_hold_ref)>0)), replacement_detection_ref BLOB CHECK(replacement_detection_ref'
             " IS NULL OR (typeof(replacement_detection_ref)='blob' AND length(replacement_detection_ref)>0)),"
             ' PRIMARY KEY(mutation_ref,hold_ref), CHECK((replacement_hold_ref IS NULL AND '
             'replacement_detection_ref IS NULL) OR (replacement_hold_ref IS NOT NULL AND '
             'replacement_detection_ref IS NOT NULL)), FOREIGN KEY(mutation_ref) REFERENCES '
             'subathon_hold_mutations(mutation_ref), FOREIGN KEY(hold_ref,expected_version_ref) REFERENCES '
             'subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(hold_ref,proposed_version_ref) '
             'REFERENCES subathon_hold_versions(hold_ref,version_ref), FOREIGN KEY(replacement_hold_ref) '
             'REFERENCES subathon_hold_causes(hold_ref), FOREIGN KEY(replacement_detection_ref) REFERENCES '
             'subathon_hold_detections(mutation_ref))'),
            ('index', 'subathon_hold_subject_idx', 'subathon_hold_causes',
             'CREATE INDEX subathon_hold_subject_idx ON subathon_hold_causes(subject_kind,subject_id,hold_ref)'),
            ('index', 'subathon_hold_producer_idx', 'subathon_hold_versions',
             'CREATE INDEX subathon_hold_producer_idx ON '
             'subathon_hold_versions(mutation_ref,hold_ref,position)'),
        ),
        (
            ('sqlite_autoindex_subathon_hold_cause_observations_1', 'subathon_hold_cause_observations'),
            ('sqlite_autoindex_subathon_hold_causes_1', 'subathon_hold_causes'),
            ('sqlite_autoindex_subathon_hold_detection_observations_1', 'subathon_hold_detection_observations'),
            ('sqlite_autoindex_subathon_hold_detections_1', 'subathon_hold_detections'),
            ('sqlite_autoindex_subathon_hold_heads_1', 'subathon_hold_heads'),
            ('sqlite_autoindex_subathon_hold_mutations_1', 'subathon_hold_mutations'),
            ('sqlite_autoindex_subathon_hold_occurrences_1', 'subathon_hold_occurrences'),
            ('sqlite_autoindex_subathon_hold_resolution_entries_1', 'subathon_hold_resolution_entries'),
            ('sqlite_autoindex_subathon_hold_resolution_expected_1', 'subathon_hold_resolution_expected'),
            ('sqlite_autoindex_subathon_hold_versions_1', 'subathon_hold_versions'),
            ('sqlite_autoindex_subathon_hold_versions_2', 'subathon_hold_versions'),
        ),
    ),
    7: (
        'reference-initial-captures-v1',
        (
            ('table', 'subathon_reference_initial_captures', 'subathon_reference_initial_captures',
             'CREATE TABLE subathon_reference_initial_captures(installation_ref BLOB NOT NULL '
             "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), provider BLOB NOT NULL "
             "CHECK(typeof(provider)='blob' AND length(provider)>0), source_account BLOB NOT NULL "
             "CHECK(typeof(source_account)='blob' AND length(source_account)>0), identifier_namespace BLOB NOT"
             " NULL CHECK(typeof(identifier_namespace)='blob' AND length(identifier_namespace)>0), opaque_id "
             "BLOB NOT NULL CHECK(typeof(opaque_id)='blob' AND length(opaque_id)>0), receipt_id BLOB NOT NULL "
             "CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000'), "
             "contribution_id BLOB NOT NULL CHECK(typeof(contribution_id)='blob' AND "
             'length(contribution_id)=8), configuration_ref BLOB NOT NULL '
             "CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0), arrival_context_ref "
             "BLOB NOT NULL CHECK(typeof(arrival_context_ref)='blob' AND length(arrival_context_ref)>0), "
             "event_ref BLOB NOT NULL CHECK(typeof(event_ref)='blob' AND length(event_ref)>0), plan_ref BLOB "
             "NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), plan_revision BLOB NOT NULL "
             "CHECK(typeof(plan_revision)='blob' AND length(plan_revision)=8 AND "
             "plan_revision=X'0000000000000001'), execution_context_ref BLOB NOT NULL "
             "CHECK(typeof(execution_context_ref)='blob' AND length(execution_context_ref)>0), PRIMARY "
             'KEY(installation_ref,provider,source_account,identifier_namespace,opaque_id), '
             'UNIQUE(installation_ref,receipt_id), FOREIGN KEY(receipt_id) REFERENCES '
             'subathon_receipts(receipt_id), FOREIGN KEY(contribution_id) REFERENCES '
             'subathon_contributions(contribution_id), FOREIGN KEY(configuration_ref) REFERENCES '
             'subathon_retained_configurations(reference), FOREIGN KEY(arrival_context_ref) REFERENCES '
             'subathon_retained_contexts(reference), FOREIGN KEY(plan_ref,plan_revision) REFERENCES '
             'subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(execution_context_ref) REFERENCES '
             'subathon_retained_contexts(reference))'),
        ),
        (
            ('sqlite_autoindex_subathon_reference_initial_captures_1', 'subathon_reference_initial_captures'),
            ('sqlite_autoindex_subathon_reference_initial_captures_2', 'subathon_reference_initial_captures'),
        ),
    ),
    8: (
        'account-sessions-v1',
        (
            ('table', 'subathon_account_installation', 'subathon_account_installation',
             'CREATE TABLE subathon_account_installation(singleton INTEGER NOT NULL PRIMARY KEY '
             "CHECK(typeof(singleton)='integer' AND singleton=1), installation_ref BLOB NOT NULL UNIQUE "
             "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), session_epoch_ref BLOB "
             "NOT NULL CHECK(typeof(session_epoch_ref)='blob' AND length(session_epoch_ref)>0), revision_ref "
             "BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref)>0))"),
            ('table', 'subathon_accounts', 'subathon_accounts',
             'CREATE TABLE subathon_accounts(installation_ref BLOB NOT NULL '
             "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB NOT NULL"
             " CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), application_access_enabled INTEGER"
             " NOT NULL CHECK(typeof(application_access_enabled)='integer' AND application_access_enabled IN "
             "(0,1)), session_epoch_ref BLOB NOT NULL CHECK(typeof(session_epoch_ref)='blob' AND "
             "length(session_epoch_ref)>0), revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND "
             'length(revision_ref)>0), PRIMARY KEY(installation_ref,account_ref), FOREIGN '
             'KEY(installation_ref) REFERENCES subathon_account_installation(installation_ref))'),
            ('table', 'subathon_local_credentials', 'subathon_local_credentials',
             'CREATE TABLE subathon_local_credentials(installation_ref BLOB NOT NULL '
             "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB NOT NULL"
             " CHECK(typeof(account_ref)='blob' AND length(account_ref)>0 AND length(account_ref)<=256), "
             "revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref)>0 AND "
             'length(revision_ref)<=256), verifier_format INTEGER NOT NULL '
             "CHECK(typeof(verifier_format)='integer' AND verifier_format=1), encoded_verifier BLOB NOT NULL "
             "CHECK(typeof(encoded_verifier)='blob' AND length(encoded_verifier)<=127), PRIMARY "
             'KEY(installation_ref,account_ref), FOREIGN KEY(installation_ref,account_ref) REFERENCES '
             'subathon_accounts(installation_ref,account_ref))'),
            ('table', 'subathon_account_sessions', 'subathon_account_sessions',
             'CREATE TABLE subathon_account_sessions(installation_ref BLOB NOT NULL '
             "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB NOT NULL"
             " CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), session_ref BLOB NOT NULL PRIMARY "
             "KEY CHECK(typeof(session_ref)='blob' AND length(session_ref)>0), installation_epoch_ref BLOB NOT"
             " NULL CHECK(typeof(installation_epoch_ref)='blob' AND length(installation_epoch_ref)>0), "
             "account_epoch_ref BLOB NOT NULL CHECK(typeof(account_epoch_ref)='blob' AND "
             'length(account_epoch_ref)>0), record_revision_ref BLOB NOT NULL '
             "CHECK(typeof(record_revision_ref)='blob' AND length(record_revision_ref)>0), digest_format "
             "INTEGER NOT NULL CHECK(typeof(digest_format)='integer' AND digest_format=1), digest BLOB NOT "
             "NULL CHECK(typeof(digest)='blob' AND length(digest)=32), state INTEGER NOT NULL "
             "CHECK(typeof(state)='integer' AND state IN (0,1)), retention INTEGER NOT NULL "
             "CHECK(typeof(retention)='integer' AND retention IN (0,1)), lifetime_started_utc_ms INTEGER NOT "
             "NULL CHECK(typeof(lifetime_started_utc_ms)='integer'), authenticated_utc_ms INTEGER NOT NULL "
             "CHECK(typeof(authenticated_utc_ms)='integer'), expires_utc_ms INTEGER NOT NULL "
             "CHECK(typeof(expires_utc_ms)='integer'), UNIQUE(installation_ref,digest), "
             'CHECK(lifetime_started_utc_ms<=authenticated_utc_ms AND authenticated_utc_ms<expires_utc_ms), '
             'FOREIGN KEY(installation_ref,account_ref) REFERENCES '
             'subathon_accounts(installation_ref,account_ref))'),
            ('index', 'subathon_account_sessions_page_idx', 'subathon_account_sessions',
             'CREATE INDEX subathon_account_sessions_page_idx ON '
             'subathon_account_sessions(installation_ref,account_ref,session_ref)'),
        ),
        (
            ('sqlite_autoindex_subathon_account_installation_1', 'subathon_account_installation'),
            ('sqlite_autoindex_subathon_account_sessions_1', 'subathon_account_sessions'),
            ('sqlite_autoindex_subathon_account_sessions_2', 'subathon_account_sessions'),
            ('sqlite_autoindex_subathon_accounts_1', 'subathon_accounts'),
            ('sqlite_autoindex_subathon_local_credentials_1', 'subathon_local_credentials'),
        ),
    ),
}


def build(seed_root):
    """Return independently framed catalog steps; proposed step 8 remains DATA only."""
    steps = [manifest(step) for step in load_seeds(seed_root)]
    for version, (identity, declarations, null_indexes) in ADDITIONS.items():
        objects = [{"type": kind, "name": name, "table_name": table, "sql": sql}
                   for kind, name, table, sql in declarations]
        objects += [{"type": "index", "name": name, "table_name": table, "sql": None}
                    for name, table in null_indexes]
        schema = sorted(steps[-1]["expected_schema"] + objects, key=schema_key)
        steps.append(manifest({"version": version, "id": identity,
                               "sql": [row[3] for row in declarations],
                               "expected_schema": schema}))
    return steps
