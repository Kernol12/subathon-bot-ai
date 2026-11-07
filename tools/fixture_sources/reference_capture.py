# SPDX-License-Identifier: GPL-3.0-or-later
"""Named independent reference rows and public frame literals; no database observations."""
from .catalogs import build as catalogs
from .common import blob, i64, integer, null, sized, text, u64


FORMAT = 'subathon.sqlite-reference-capture.fixture.v1'
PROVENANCE = 'Independent literal specification v0.45 / CONTRACT014 / STORAGETEST018 supplier v1. Author struct/hashlib only; no project implementation or SQL engine executed. Schema projection remains pending selected-driver observation.'

PROVIDER = b"synthetic.reference.v1"
EVENT_TYPE = b"synthetic.reference.support.v1"
RULE_EXPLANATION = b"reference.single-rule.match.v1"
CALCULATION = b"reference.integer-ms.v1"
REVERSAL = b"reference.original-time.v1"

# Independent SQL column order, shared only by these literal INSERT/SELECT recipes.
COLUMNS = {
    'subathon_retained_configurations': (
        'reference,document'
    ),
    'subathon_retained_contexts': (
        'reference,document,configuration_ref'
    ),
    'subathon_receipts': (
        'receipt_id,provider,source_account,identifier_namespace,opaque_id,event_type,schema_version,source_timestamp_utc_ms,actor_id,actor_display_name,money_original_decimal,money_currency,payment_namespace,payment_opaque_id,activity_namespace,activity_opaque_id,server_timestamp_utc_ms,connection_ref,configuration_ref,modifier_context_ref'
    ),
    'subathon_receipt_attempts': (
        'receipt_id,ordinal,event_type,schema_version,source_timestamp_utc_ms,actor_id,actor_display_name,money_original_decimal,money_currency,payment_namespace,payment_opaque_id,activity_namespace,activity_opaque_id,server_timestamp_utc_ms,connection_ref,configuration_ref,modifier_context_ref,disposition'
    ),
    'subathon_contributions': (
        'contribution_id,original_receipt_id,activity_provider,activity_source_account,activity_namespace,activity_id,payment_evidence,monetary,original_decimal,currency,resolution_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_ref,application,reversal,settlement'
    ),
    'subathon_contribution_observations': (
        'receipt_id,contribution_id,kind,evidence,activity_provider,activity_source_account,activity_namespace,activity_id,verification_ref,support_monetary,support_original_decimal,support_currency,support_resolution_ref,support_missing_rate,support_review,support_evidence_policy,support_policy_ref,support_decision_choice,support_decision_ref,support_decision_actor_ref,support_application,support_reversal,support_settlement'
    ),
    'subathon_plan_headers': (
        'plan_ref,event_ref,parent_event_ref,root_event_ref,receipt_id,contribution_id,head_revision'
    ),
    'subathon_plan_revisions': (
        'plan_ref,revision,parent_revision,payment_evidence,monetary,original_decimal,currency,resolution_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_ref,application,reversal,settlement,valuation_state,historical_resolution_ref,historical_original_decimal,historical_currency,comparison_currency,effective_rate_decimal,fetched_rate_decimal,snapshot_ref,rate_source_ref,rate_timestamp_utc_ms,fetch_timestamp_utc_ms,manual_decision_ref,override_authority,override_actor_ref,override_reason,override_decision_ref'
    ),
    'subathon_plan_rules': (
        'plan_ref,revision,ordinal,rule_ref,version_ref,selected,explanation'
    ),
    'subathon_plan_requests': (
        'plan_ref,revision,ordinal,action_ref,action_version_ref,rule_ref,target_ref,unit,requires_historical_valuation,exact_numerator,exact_denominator,requested_units,calculation_explanation,reversal_behavior_ref'
    ),
    'subathon_reference_initial_captures': (
        'installation_ref,provider,source_account,identifier_namespace,opaque_id,receipt_id,contribution_id,configuration_ref,arrival_context_ref,event_ref,plan_ref,plan_revision,execution_context_ref'
    ),
}

def _insert(table, bindings):
    placeholders = ",".join("?" for _ in bindings)
    return {"sql": f"INSERT INTO main.{table}({COLUMNS[table]}) VALUES ({placeholders})",
            "bindings": bindings}


def _query(name, table, where, expected_row):
    # Expected rows are supplied literal DATA; this does not evaluate SQL.
    return {"name": name, "sql": f"SELECT {COLUMNS[table]} FROM main.{table} WHERE {where}",
            "expected": [expected_row]}


def _documents(configuration, arrival, event, execution, plan, opaque, receipt, contribution,
               lengths):
    # CONTRACT014 / sqlite-initial-captures-v1: explicit public byte grammar.
    origin = sized(PROVIDER) + sized(b"A")
    identity = origin + sized(b"N") + sized(opaque)
    rule = (sized(b"R") + sized(b"r") + sized(b"a") + sized(b"b") + sized(b"T") +
            i64(600000) + b"\x01" + sized(b"I") + sized(b"U") + sized(b"K"))
    timer = b"\x00" + i64(120000) + b"\x00\x00"  # end-mode0, reuse0, no hard end
    config = (b"SRC1" + sized(configuration) + sized(b"W") + origin + sized(b"N") +
              rule + b"\x00" + timer + b"\x01" * 5)  # absent cap, five complete facts
    arrival_document = (b"SRA1" + sized(arrival) + sized(event) + i64(0) + sized(config) +
                        sized(b"Q") + sized(opaque) + b"\x00" + sized(b""))
    execution_document = (b"SRX1" + sized(execution) + sized(arrival) + sized(b"I") +
                          identity + u64(receipt) + u64(contribution) + sized(plan) + u64(1) + rule)
    initial = (b"SRI1" + sized(b"I") + identity + u64(receipt) + u64(contribution) +
               sized(configuration) + sized(arrival) + sized(event) + sized(plan) + u64(1) +
               sized(execution))
    values = (
        ("SRC1", "subathon.reference.configuration.v1", config),
        ("SRA1", "subathon.reference.arrival.v1", arrival_document),
        ("SRX1", "subathon.reference.execution.v1", execution_document),
        ("SRI1", "subathon.reference.initial-capture.v1", initial),
    )
    return {name: {"format": format_ref, "document_hex": value.hex(),
                   "document_bytes": document_bytes, "envelope_bytes": envelope_bytes}
            for (name, format_ref, value), (document_bytes, envelope_bytes) in zip(values, lengths)}


def _capture(name, configuration, arrival, event, execution, plan, opaque, receipt, contribution,
             key_bytes, record_bytes, lengths):
    documents = _documents(configuration, arrival, event, execution, plan, opaque, receipt,
                           contribution, lengths)
    config_document = bytes.fromhex(documents["SRC1"]["document_hex"])
    arrival_document = bytes.fromhex(documents["SRA1"]["document_hex"])
    execution_document = bytes.fromhex(documents["SRX1"]["document_hex"])
    rid, cid, revision = blob(u64(receipt)), blob(u64(contribution)), blob(u64(1))
    cfg, arr, evt, ctx, pln = map(blob, (configuration, arrival, event, execution, plan))
    provider, account, namespace, source_id = map(blob, (PROVIDER, b"A", b"N", opaque))

    # Independently supplied typed rows. Reused below for seeding and expected identity
    # queries, never derived from database/project observations or a policy model.
    configuration_row = [cfg, blob(config_document)]
    arrival_row = [arr, blob(arrival_document), cfg]
    receipt_row = [rid, provider, account, namespace, source_id, blob(EVENT_TYPE), integer(1),
                   null(), null(), null(), null(), null(), null(), null(), null(), null(),
                   integer(0), blob(b"Q"), cfg, arr]
    attempt_row = [rid, integer(1), blob(EVENT_TYPE), integer(1), null(), null(), null(),
                   null(), null(), null(), null(), null(), null(), integer(0), blob(b"Q"),
                   cfg, arr, integer(0)]
    contribution_row = [cid, rid, null(), null(), null(), null(), integer(3), integer(0),
                        null(), null(), null(), integer(0), integer(1), integer(0), cfg,
                        null(), null(), null(), integer(0), integer(0), integer(1)]
    observation_row = [rid, cid, integer(0), integer(3), null(), null(), null(), null(), null(),
                       integer(0), null(), null(), null(), integer(0), integer(1), integer(0),
                       cfg, null(), null(), null(), integer(0), integer(0), integer(1)]
    header_row = [pln, evt, null(), evt, rid, cid, revision]
    plan_row = [pln, revision, null(), integer(3), integer(0), null(), null(), null(),
                integer(0), integer(1), integer(0), cfg, null(), null(), null(), integer(0),
                integer(0), integer(1), integer(0), null(), null(), null(), null(), null(),
                null(), null(), null(), null(), null(), null(), null(), null(), null(), null()]
    rule_row = [pln, revision, integer(1), blob(b"R"), blob(b"r"), integer(1),
                blob(RULE_EXPLANATION)]
    request_row = [pln, revision, integer(1), blob(b"a"), blob(b"b"), blob(b"R"), blob(b"T"),
                   integer(0), integer(0), blob(b"600000"), blob(b"1"), integer(600000),
                   blob(CALCULATION), blob(REVERSAL)]
    execution_row = [ctx, blob(execution_document), cfg]
    initial_row = [blob(b"I"), provider, account, namespace, source_id, rid, cid,
                   cfg, arr, evt, pln, revision, ctx]

    intake = [
        _insert("subathon_retained_configurations", configuration_row),
        _insert("subathon_retained_contexts", arrival_row),
        _insert("subathon_receipts", receipt_row),
        _insert("subathon_receipt_attempts", attempt_row),
        {"sql": "UPDATE main.subathon_receipt_allocator SET last_allocated=? WHERE singleton=1",
         "bindings": [rid]},
        _insert("subathon_contributions", contribution_row),
        _insert("subathon_contribution_observations", observation_row),
        {"sql": "UPDATE main.subathon_contribution_allocator SET last_allocated=? WHERE singleton=1",
         "bindings": [cid]},
    ]
    capture = [
        _insert("subathon_plan_headers", header_row),
        _insert("subathon_plan_revisions", plan_row),
        _insert("subathon_plan_rules", rule_row),
        _insert("subathon_plan_requests", request_row),
        _insert("subathon_retained_contexts", execution_row),
    ]
    index = _insert("subathon_reference_initial_captures", initial_row)
    cfg_where = f"reference=X'{configuration.hex()}'"
    arr_where = f"reference=X'{arrival.hex()}'"
    receipt_where = f"receipt_id=X'{u64(receipt).hex()}'"
    contribution_where = f"contribution_id=X'{u64(contribution).hex()}'"
    plan_where = f"plan_ref=X'{plan.hex()}'"
    execution_where = f"reference=X'{execution.hex()}'"
    source_where = (f"installation_ref=X'49' AND provider=X'{PROVIDER.hex()}' AND "
                    f"source_account=X'41' AND identifier_namespace=X'4e' AND opaque_id=X'{opaque.hex()}'")
    query_rows = (
        ("subathon_retained_configurations", 0, cfg_where, configuration_row),
        ("subathon_retained_contexts", 1, arr_where, arrival_row),
        ("subathon_receipts", 2, receipt_where, receipt_row),
        ("subathon_receipt_attempts", 3, receipt_where, attempt_row),
        ("subathon_contributions", 5, contribution_where, contribution_row),
        ("subathon_contribution_observations", 6, receipt_where, observation_row),
        ("subathon_plan_headers", 8, plan_where, header_row),
        ("subathon_plan_revisions", 9, plan_where, plan_row),
        ("subathon_plan_rules", 10, plan_where + " AND ordinal=1", rule_row),
        ("subathon_plan_requests", 11, plan_where + " AND ordinal=1", request_row),
        ("subathon_retained_contexts", 12, execution_where, execution_row),
        ("subathon_reference_initial_captures", 13, source_where, initial_row),
    )
    queries = [_query(f"{name} complete literal {table} {ordinal}", table, where, row)
               for table, ordinal, where, row in query_rows]
    queries.append(_query(f"{name} complete initial binding by installation/receipt",
                          "subathon_reference_initial_captures",
                          "installation_ref=X'49' AND " + receipt_where, initial_row))
    return {"key_bytes": key_bytes, "record_bytes": record_bytes, "documents": documents,
            "intake": intake, "capture": capture, "index": index, "queries": queries}


def _initial_row(*, installation=None, provider=None, account=None, namespace=None, opaque=None,
                 receipt=None, contribution=None, configuration=None, arrival=None, event=None,
                 plan=None, revision=None, execution=None):
    # One concrete 13-column input builder, not an external mutation/patch language.
    return [
        blob(b"I") if installation is None else installation,
        blob(PROVIDER) if provider is None else provider,
        blob(b"A") if account is None else account,
        blob(b"N") if namespace is None else namespace,
        blob(b"E") if opaque is None else opaque,
        blob(u64(7)) if receipt is None else receipt,
        blob(u64(0)) if contribution is None else contribution,
        blob(b"C") if configuration is None else configuration,
        blob(b"X") if arrival is None else arrival,
        blob(b"V") if event is None else event,
        blob(b"P") if plan is None else plan,
        blob(u64(1)) if revision is None else revision,
        blob(b"Y") if execution is None else execution,
    ]


def _refusals():
    # Every named finite refusal and its exact old order is supplied explicitly.
    rows = [
        ("TEXT installation", _initial_row(installation=text("I"))),
        ("INTEGER receipt", _initial_row(receipt=integer(7))),
        ("TEXT receipt", _initial_row(receipt=text("7"))),
        ("zero receipt", _initial_row(receipt=blob(u64(0)))),
        ("INTEGER CID", _initial_row(contribution=integer(0))),
        ("TEXT CID", _initial_row(contribution=text("0"))),
        ("revision2", _initial_row(revision=blob(u64(2)))),
        ("empty reference 0", _initial_row(installation=blob(b""))),
        ("empty reference 1", _initial_row(provider=blob(b""))),
        ("empty reference 2", _initial_row(account=blob(b""))),
        ("empty reference 3", _initial_row(namespace=blob(b""))),
        ("empty reference 4", _initial_row(opaque=blob(b""))),
        ("empty reference 7", _initial_row(configuration=blob(b""))),
        ("empty reference 8", _initial_row(arrival=blob(b""))),
        ("empty reference 9", _initial_row(event=blob(b""))),
        ("empty reference 10", _initial_row(plan=blob(b""))),
        ("empty reference 12", _initial_row(execution=blob(b""))),
        ("receipt FK", _initial_row(receipt=blob(u64(8)))),
        ("contribution FK", _initial_row(contribution=blob(u64(1)))),
        ("configuration FK", _initial_row(configuration=blob(b"z"))),
        ("arrival FK", _initial_row(arrival=blob(b"z"))),
        ("plan revision FK", _initial_row(plan=blob(b"z"))),
        ("execution FK", _initial_row(execution=blob(b"z"))),
    ]
    return [{"name": name, "statement": _insert("subathon_reference_initial_captures", row)}
            for name, row in rows]


def build(seed_root):
    steps = catalogs(seed_root)
    return {
        "format": FORMAT,
        "provenance": PROVENANCE,
        "manifest": steps[6],
        "prefixes": [
            {"version": step["version"], "id": step["id"],
             "payload_bytes": step["payload_bytes"], "sha256_hex": step["sha256_hex"]}
            for step in steps[:6]
        ],
        "captures": {
            "A": _capture("A", b"C", b"X", b"V", b"Y", b"P", b"E", 7, 0, 26, 55,
                          ((116, 151), (157, 186), (138, 169), (99, 136))),
            "B": _capture("B", b"C2", b"X2", b"V2", b"Y2", b"P2", b"E\0",
                          18446744073709551615, 18446744073709551615, 27, 61,
                          ((117, 152), (161, 190), (142, 173), (105, 142))),
        },
        "constraint_refusals": _refusals(),
    }
