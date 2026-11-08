# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent named hold scenarios, typed rows and literal query/byte oracles.

Only catalog framing is calculated. These finite records preserve the original
reviewed synthetic DATA; no SQL or application policy is evaluated here.
"""
import json

from .catalogs import build as catalogs
from .bundle import insert, physical_manifest, statement
from .common import blob, integer, migration_payload, null, render as render_standard, text, u64


OPAQUE = b"\xef\xbb\xbfa\x00\xff"

FORMAT = 'sqlite-application-holds-v1-independent-vectors'

PROVENANCE = ('Independently frozen approved physical701/83e/API421 literals, manual catalog inventory and '
 'stdlib decimal-length F/hashlib. No new hold facade/factory/marker, production manifest/codec or '
 'observed new feature output generates expectations. Synthetic DATA only; unknown '
 'permissions/mixed/continuation policy not enabled.')


def _envelope(format_ref, document=OPAQUE.hex()):
    return {"format": format_ref, "document_hex": document}


def _observation(token):
    return {"token": token, "annotation": _envelope("o"), "policy": None,
            "permission": None, "actor": None, "decision": None, "utc": "0", "elapsed": "0"}


def _source(subject_kind, subject_id, receipt, event):
    return {"subject": {"kind": subject_kind, "id": subject_id}, "receipt": receipt,
            "event": event, "parent": None, "root": event,
            "configuration": "cfg", "context": "ctx"}


def _occurrence(ordinal, action, action_version, rule, target, unit, exact, final, explanation):
    return {"ordinal": ordinal, "action": action, "action_version": action_version,
            "rule": rule, "target": target, "unit": unit, "exact": exact,
            "final": final, "explanation": explanation}


def _plan(kind, key, extent, stopped, failed, occurrences):
    return {"kind": kind, "key": key, "extent": extent, "stopped": stopped,
            "failed": failed, "occurrences": occurrences}


def _failure(stage):
    # Both numeric cases retain the supplied MAX+1, never a calculated outcome.
    return {"stage": stage, "ordinal": "2", "target": "score",
            "exact": ["9223372036854775808", "1"], "annotation": _envelope("f")}


def _cause(hold, source, operation, token, plan, kind, failure=None, timer=None,
           missing_policy=None, supersedes=None):
    return {"hold": hold, "source": source,
            "command": {"operation": operation, "intent": _envelope("c")},
            "observation": _observation(token), "plan": plan, "kind": kind,
            "failure": failure, "timer": timer, "missing_policy": missing_policy,
            "supersedes": supersedes}


def _causes():
    retained = "synthetic retained request"
    unresolved = "unresolved synthetic policy"
    return {
        "hOverflow": _cause(
            "hOverflow", _source(0, "0", "7", "event-PO"), ["scope", "hOverflow"], "p",
            _plan(0, ["PO", "1"], 0, None, None, [
                _occurrence("1", "time", "v1", "rule", "main", 0, ["5", "1"], "5", retained),
                _occurrence("2", "score", "v1", "rule", "score", 1, ["1", "1"], "1", retained),
            ]), 0, failure=_failure(1)),
        "hPause": _cause(
            "hPause", _source(0, "0", "7", "event-A"), ["scope", "hPause"], "p",
            _plan(0, ["PA", "1"], 0, None, None, []), 2, missing_policy=unresolved),
        "hFail": _cause(
            "hFail", _source(0, "18446744073709551615", "18446744073709551615", None),
            ["scope", "hFail"], None,
            _plan(1, None, 2, 0, _envelope("fc"), [
                _occurrence("1", None, None, None, "", 1, ["0", "1"], "0", None),
                _occurrence("2", None, None, None, "score", 1,
                            ["9223372036854775808", "1"], None, None),
            ]), 0, failure=_failure(0)),
        "hReceipt": _cause(
            "hReceipt", _source(1, "8", "8", "event-C"), ["scope", "hReceipt"], "p",
            _plan(0, ["PC", "1"], 0, None, None, [
                _occurrence("1", "correction", "v1", "rule-C", "main", 0,
                            ["-3", "1"], "-3", "synthetic correction"),
            ]), 2, missing_policy=unresolved),
        "hOld": _cause(
            "hOld", _source(0, "18446744073709551615", "18446744073709551615", None),
            ["scope", "hOld"], None, _plan(1, None, 2, 1, _envelope("fc"), []),
            2, missing_policy=unresolved),
        "hNew": _cause(
            "hNew", _source(0, "18446744073709551615", "18446744073709551615", "event-B"),
            ["scope", "hNew"], None, _plan(1, ["PB", "1"], 2, 1, _envelope("fc"), []),
            2, missing_policy=unresolved, supersedes="hOld"),
        "hTimer": _cause(
            "hTimer", _source(0, "0", "7", "event-PT"), ["timer", "original"], "t0",
            _plan(0, ["PT", "1"], 0, None, None, [
                _occurrence("1", "timer", "v1", "rule", "main", 0,
                            ["-150", "1"], "-150", retained),
                _occurrence("2", "score", "v1", "rule", "score", 1, ["2", "1"], "2", retained),
            ]), 1, timer={"ordinal": "1", "component": 4, "request": "-150", "policy": 2,
                          "disposition": 1, "unapplied": "-150", "composition": 1,
                          "decision": _envelope("mixed"),
                          "associated": {"operation": ["timer", "original"], "ordinal": "1"}}),
    }


def _detection(hold, mutation, expected, proposed, token):
    return {"cause_id": hold, "mutation": mutation, "expected": expected, "proposed": proposed,
            "observation": _observation(token), "audit": _envelope("u")}


def _version(hold, version, position, parent, state, producer, replacement=None):
    return {"hold": hold, "version": version, "position": position, "parent": parent,
            "state": state, "producer": producer, "replacement": replacement}


def _detections_and_versions():
    # This is the finite reviewed sequence dPause1..65, not a transition simulator.
    detections = {}
    versions = {}
    for hold, mutation, token in (
        ("hOverflow", "dOverflow", "p"), ("hFail", "dFail", None),
        ("hReceipt", "dReceipt", "p"), ("hOld", "dOld", None),
        ("hNew", "dNew", None), ("hTimer", "dTimer", "t0"),
    ):
        detections[mutation] = _detection(hold, mutation, None, "v1", token)
        versions[mutation] = _version(hold, "v1", "1", None, 0, mutation)
    for position in range(1, 66):
        mutation, version = f"dPause{position}", f"v{position}"
        parent = None if position == 1 else f"v{position - 1}"
        detections[mutation] = _detection("hPause", mutation, parent, version, "p")
        versions[mutation] = _version("hPause", version, str(position), parent, 0, mutation)
    return detections, versions


def _resolved_old_version():
    return _version("hOld", "v2", "2", "v1", 1, "rReplace", ["hNew", "dNew"])


def _views(versions):
    views = {}
    for hold, mutation in (("hOverflow", "dOverflow"), ("hFail", "dFail"),
                           ("hReceipt", "dReceipt"), ("hOld", "dOld"),
                           ("hNew", "dNew"), ("hPause", "dPause1"), ("hTimer", "dTimer")):
        current, event = versions[mutation], None
        if hold == "hOld":
            current, event = _resolved_old_version(), {"kind": 1, "resolution": "rReplace"}
        elif hold == "hPause":
            current, event = versions["dPause65"], {"kind": 0, "detection": "dPause65"}
        views[hold] = {"cause_id": hold, "first_detection": mutation,
                       "first_version": versions[mutation], "current_version": current,
                       "current_event": event}
    return views



def _resolutions():
    return {'rReplace': {'request': {'mutation': 'rReplace',
                              'subject': {'kind': 0, 'id': '18446744073709551615'},
                              'expected': [['hFail', 'v1'], ['hOld', 'v1']],
                              'entries': [{'hold': 'hOld',
                                           'expected': 'v1',
                                           'proposed': 'v2',
                                           'decision': {'format': 'decision', 'document_hex': ''},
                                           'replacement': 'dNew'}],
                              'actor': 'reviewer',
                              'decision': 'decision',
                              'reason': '',
                              'options': {'format': 'options', 'document_hex': 'efbbbf6100ff'},
                              'application': None},
                  'produced': [{'hold': 'hOld',
                                'version': 'v2',
                                'position': '2',
                                'parent': 'v1',
                                'state': 1,
                                'producer': 'rReplace',
                                'replacement': ['hNew', 'dNew']},
                               {'hold': 'hNew',
                                'version': 'v1',
                                'position': '1',
                                'parent': None,
                                'state': 0,
                                'producer': 'dNew',
                                'replacement': None}]}}


def _proposals():
    return {'timer_correction': {'mutation': 'rTimer',
                          'subject': {'kind': 0, 'id': '0'},
                          'expected': [['hTimer', 'v1']],
                          'entries': [{'hold': 'hTimer',
                                       'expected': 'v1',
                                       'proposed': 'v2',
                                       'decision': {'format': 'decision',
                                                    'document_hex': 'efbbbf6100ff'},
                                       'replacement': None}],
                          'actor': 'reviewer',
                          'decision': 'decision',
                          'reason': '',
                          'options': {'format': 'options', 'document_hex': ''},
                          'application': {'kind': 1,
                                          'operation': ['timer', 'correction'],
                                          'receipt': '8',
                                          'plan': ['PC', '1'],
                                          'held': {'hold': 'hTimer',
                                                   'version': 'v1',
                                                   'ordinal': '1',
                                                   'component': 4,
                                                   'remaining': '-150'}}},
     'forbidden_old_beside_alias': {'mutation': 'rAlias',
                                    'subject': {'kind': 0, 'id': '0'},
                                    'expected': [['hTimer', 'v1']],
                                    'entries': [{'hold': 'hTimer',
                                                 'expected': 'v1',
                                                 'proposed': 'v2',
                                                 'decision': {'format': 'decision',
                                                              'document_hex': 'efbbbf6100ff'},
                                                 'replacement': None}],
                                    'actor': 'reviewer',
                                    'decision': 'decision',
                                    'reason': '',
                                    'options': {'format': 'options', 'document_hex': ''},
                                    'application': {'kind': 0,
                                                    'operation': ['timer', 'original'],
                                                    'receipt': '7',
                                                    'plan': ['PT', '1'],
                                                    'held': None}}}


def _byte_item_oracles():
    return {'hOverflow.cause': {'bytes': 267, 'items': 2},
     'hPause.cause': {'bytes': 125, 'items': 0},
     'hFail.cause': {'bytes': 174, 'items': 2},
     'hReceipt.cause': {'bytes': 191, 'items': 1},
     'hOld.cause': {'bytes': 105, 'items': 0},
     'hNew.cause': {'bytes': 133, 'items': 0},
     'hTimer.cause': {'bytes': 287, 'items': 2},
     'dOverflow.detection': {'bytes': 309, 'items': 2},
     'dFail.detection': {'bytes': 211, 'items': 2},
     'dPause1.detection': {'bytes': 165, 'items': 0},
     'dPause65.detection': {'bytes': 170, 'items': 0},
     'dNew.detection': {'bytes': 169, 'items': 0},
     'dTimer.detection': {'bytes': 327, 'items': 2},
     'rReplace.batch': {'bytes': 244, 'items': 3},
     'rReplace.recorded': {'bytes': 296, 'items': 5},
     'rTimer.batch': {'bytes': 129, 'items': 2},
     'rAlias.batch': {'bytes': 102, 'items': 2},
     'hOverflow.view': {'bytes': 367, 'items': 2},
     'hFail.view': {'bytes': 253, 'items': 2},
     'hReceipt.view': {'bytes': 286, 'items': 1},
     'hOld.view': {'bytes': 490, 'items': 5},
     'hNew.view': {'bytes': 207, 'items': 0},
     'hPause.view': {'bytes': 264, 'items': 0},
     'hTimer.view': {'bytes': 373, 'items': 2},
     'CID0.active': {'bytes': 640, 'items': 4},
     'rReplace.mutation': {'bytes': 62, 'items': 2},
     'dOverflow.mutation': {'bytes': 40, 'items': 1},
     'hPause.history_first2': {'bytes': 403, 'items': 2},
     'hOld.history_resolution': {'bytes': 338, 'items': 6},
     'hPause.history_query': {'bytes': 25, 'items': 0},
     'hPause.empty_tail': {'bytes': 11, 'items': 0},
     'CIDMAX.active': {'bytes': 469, 'items': 4},
     'empty.active': {'bytes': 9, 'items': 0},
     'missing.history': {'bytes': 1, 'items': 0},
     'conflict.mutation': {'bytes': 1, 'items': 0}}


def _literal_arithmetic():
    return {'hPause.history_first2': '1 + (6 + 3 + (24 + 1 + 165) + (26 + 1 + 167) + 8 + 1) = 403; items2',
     'hOld.history_resolution': '1 + (4 + 2 + (33 + 1 + 296) + 1) = 338; items1+5=6',
     'dOverflow.mutation': '1 + 9 + 1 + 29 = 40; items1',
     'CIDMAX.active': '9+253+207=469; items2+2+0=4',
     'CID0.active': '9+367+264=640; items2+2+0=4',
     'scope': 'Independent reviewed API421 arithmetic, no production/new feature output used'}


def _history_pages():
    return {'pause_first2': {'hold': 'hPause',
                      'checked': 'v65',
                      'entries': [{'version': {'hold': 'hPause',
                                               'version': 'v1',
                                               'position': '1',
                                               'parent': None,
                                               'state': 0,
                                               'producer': 'dPause1',
                                               'replacement': None},
                                   'kind': 0,
                                   'event': 'dPause1'},
                                  {'version': {'hold': 'hPause',
                                               'version': 'v2',
                                               'position': '2',
                                               'parent': 'v1',
                                               'state': 0,
                                               'producer': 'dPause2',
                                               'replacement': None},
                                   'kind': 0,
                                   'event': 'dPause2'}],
                      'next': '2',
                      'has_more': True},
     'old_resolution': {'hold': 'hOld',
                        'checked': 'v2',
                        'entries': [{'version': {'hold': 'hOld',
                                                 'version': 'v2',
                                                 'position': '2',
                                                 'parent': 'v1',
                                                 'state': 1,
                                                 'producer': 'rReplace',
                                                 'replacement': ['hNew', 'dNew']},
                                     'kind': 1,
                                     'event': 'rReplace'}],
                        'next': None,
                        'has_more': False}}


def _synthetic_data_meanings():
    return {'authority': 'All formats are synthetic retained DATA only. They do not prove '
                  'execution/disclosure grants, current production policy, actual producer evaluation, '
                  'current COMMIT or permitted correction/reject/defer/continuation. No provider or '
                  'delivery meaning.',
     'binary_documents': 'efbbbf6100ff is exactly BOM + a + NUL + FF. It is intentionally opaque '
                         'storage payload, never interpreted as JSON/text or empty.',
     'mixed': 'In hTimer only: explicit synthetic recording declaration BesideCompleteApplication; '
              'ordinal1 requested-150/RequireReview is wholly unapplied with Main/debt0 while ordinal2 '
              'score+2 is actually credited. This supplies declared fixture meaning only; no actual '
              'production mixed policy is enabled.',
     'c': 'Synthetic complete original command-intent bytes, opaque equality data; hTimer command key '
          'exactly timer/original. No production canonical command codec is supplied.',
     'o': 'Synthetic observation annotation; actual known token references retain full stored state, '
          'UTC0/elapsed0 are genuinely supplied zeros. Absent pre-capture token is unknown.',
     'fc': 'Explicit pre-finalization failure/provenance annotation; hFail retains known finalized0 '
           'then failedMAX+1 without a finalized amount, known_prefix means remaining sequence '
           'unknown; hNew prior PB is context only, not the failed proposal.',
     'f': 'Numeric failure annotation with actual declared final-conversion or aggregate stage, known '
          'ordinal/target/Exact. No arithmetic/semantic producer proof from bytes.',
     'u': 'Owned synthetic non-success hold audit or actual complete bundle audit; typed '
          'relation/fields determine role, not a permission flag.',
     'options': 'rReplace records closing hOld and atomically creating hNew, no associated '
                'application; future rTimer proposal is typed DATA only.',
     'decision': 'Present-empty decision document remains distinct from absence, and grants no policy.',
     'correction': 'Receipt8/PC request-3 is a distinct complete original correction with Main-3. '
                   'hTimer full-150 remaining is retained origin DATA; no assertion that this -3 '
                   'completes or authorizes the whole held adjustment. Proposed resolution is not '
                   'stored as a completed policy outcome.'}


# Fixed literal INSERT columns, independently retained from the original rows.
COLUMNS = {
    'subathon_retained_contexts': ('subathon_retained_contexts',
        'reference,configuration_ref,document'
    ),
    'subathon_hold_mutations_detection': ('subathon_hold_mutations',
        'mutation_ref,mutation_kind,subject_kind,subject_id,expected_count,entry_count'
    ),
    'subathon_hold_causes_numeric': ('subathon_hold_causes',
        'hold_ref,subject_kind,subject_id,contribution_id,source_receipt_id,event_ref,parent_event_ref,'
        'root_event_ref,configuration_ref,arrival_context_ref,command_scope,command_ref,'
        'command_format_ref,command_document,cause_kind,supersedes_hold_ref,first_detection_ref,'
        'plan_evidence_kind,plan_ref,plan_revision,extent,stopped_for,failed_capture_format_ref,'
        'failed_capture_document,occurrence_count,failure_stage,failing_ordinal,failing_target_ref,'
        'failure_numerator,failure_denominator,failure_format_ref,failure_document'
    ),
    'subathon_hold_cause_observations': ('subathon_hold_cause_observations',
        'hold_ref,persisted_state_token,format_ref,document,policy_revision_ref,permission_revision_ref,'
        'actor_ref,decision_ref,observed_utc_ms,elapsed_ms'
    ),
    'subathon_hold_occurrences': ('subathon_hold_occurrences',
        'hold_ref,ordinal,action_ref,action_version_ref,rule_ref,target_ref,unit,attempted_numerator,'
        'attempted_denominator,finalized_units,explanation'
    ),
    'subathon_hold_detections': ('subathon_hold_detections',
        'mutation_ref,hold_ref,expected_version_ref,proposed_version_ref,audit_format_ref,audit_document'
    ),
    'subathon_hold_detection_observations': ('subathon_hold_detection_observations',
        'mutation_ref,persisted_state_token,format_ref,document,policy_revision_ref,'
        'permission_revision_ref,actor_ref,decision_ref,observed_utc_ms,elapsed_ms'
    ),
    'subathon_hold_versions': ('subathon_hold_versions',
        'hold_ref,version_ref,position,parent_version_ref,state,mutation_ref,replacement_hold_ref,'
        'replacement_detection_ref'
    ),
    'subathon_hold_heads': ('subathon_hold_heads',
        'hold_ref,version_ref,version_count'
    ),
    'subathon_hold_causes_policy': ('subathon_hold_causes',
        'hold_ref,subject_kind,subject_id,contribution_id,source_receipt_id,event_ref,parent_event_ref,'
        'root_event_ref,configuration_ref,arrival_context_ref,command_scope,command_ref,'
        'command_format_ref,command_document,cause_kind,supersedes_hold_ref,first_detection_ref,'
        'plan_evidence_kind,plan_ref,plan_revision,extent,stopped_for,failed_capture_format_ref,'
        'failed_capture_document,occurrence_count,missing_policy_reason'
    ),
    'subathon_hold_mutations_resolution': ('subathon_hold_mutations',
        'mutation_ref,mutation_kind,subject_kind,subject_id,expected_count,entry_count,actor_ref,'
        'decision_ref,reason,options_format_ref,options_document'
    ),
    'subathon_hold_resolution_expected': ('subathon_hold_resolution_expected',
        'mutation_ref,hold_ref,version_ref'
    ),
    'subathon_hold_resolution_entries': ('subathon_hold_resolution_entries',
        'mutation_ref,hold_ref,expected_version_ref,proposed_version_ref,decision_format_ref,'
        'decision_document,replacement_hold_ref,replacement_detection_ref'
    ),
    'subathon_app_contributions': ('subathon_app_contributions',
        'payment_evidence,monetary,original_decimal,currency,resolution_ref,missing_rate,review,'
        'evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_ref,application,reversal,'
        'settlement,operation_scope,operation_ref,contribution_id,application_after'
    ),
    'subathon_hold_causes_timer': ('subathon_hold_causes',
        'hold_ref,subject_kind,subject_id,contribution_id,source_receipt_id,event_ref,parent_event_ref,'
        'root_event_ref,configuration_ref,arrival_context_ref,command_scope,command_ref,'
        'command_format_ref,command_document,cause_kind,supersedes_hold_ref,first_detection_ref,'
        'plan_evidence_kind,plan_ref,plan_revision,extent,stopped_for,failed_capture_format_ref,'
        'failed_capture_document,occurrence_count,timer_request_ordinal,timer_component_kind,'
        'timer_request_units,deficit_policy,deficit_disposition,unapplied_units,composition,'
        'composition_format_ref,composition_document,associated_scope,associated_ref,'
        'associated_request_ordinal'
    ),
}


def _insert(shape, bindings):
    if shape in COLUMNS:
        table, columns = COLUMNS[shape]
        return insert(table, bindings, columns=columns)
    return insert(shape, bindings)


_statement = statement



def _source_foundation():
    # Common original22 statements, with all typed values supplied explicitly.
    return [
        _insert('subathon_retained_configurations', [blob(b'cfg'), blob(b'')]),
        _insert('subathon_retained_contexts', [blob(b'ctx'), blob(b'cfg'), blob(b'')]),
        _statement('UPDATE main.subathon_receipt_allocator SET last_allocated=? WHERE singleton=1', [
            blob(u64(18446744073709551615))]),
        _statement('UPDATE main.subathon_contribution_allocator SET last_allocated=? WHERE singleton=1', [
            blob(u64(18446744073709551615))]),
        _insert('subathon_receipts', [blob(u64(7)), blob(b'synthetic'), blob(b'installation'), blob(b'event'),
            blob(b'receipt-A'), blob(b'synthetic.support'), integer(1), null(), null(), null(), null(), null(),
            null(), null(), null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx')]),
        _insert('subathon_receipt_attempts', [blob(u64(7)), integer(1), blob(b'synthetic.support'), integer(1),
            null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0),
            blob(b'connection'), blob(b'cfg'), blob(b'ctx'), integer(0)]),
        _insert('subathon_contributions', [blob(u64(0)), blob(u64(7)), null(), null(), null(), null(),
            integer(3), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'),
            null(), null(), null(), integer(0), integer(0), integer(1)]),
        _insert('subathon_contribution_observations', [blob(u64(7)), blob(u64(0)), integer(0), integer(3),
            null(), null(), null(), null(), null(), integer(0), null(), null(), null(), integer(0), integer(1),
            integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1)]),
        _insert('subathon_plan_headers', [blob(b'PA'), blob(b'event-A'), null(), blob(b'event-A'),
            blob(u64(7)), blob(u64(0)), blob(u64(1))]),
        _insert('subathon_plan_revisions', [blob(b'PA'), blob(u64(1)), null(), integer(3), integer(0), null(),
            null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0),
            integer(0), integer(1), integer(0), null(), null(), null(), null(), null(), null(), null(), null(),
            null(), null(), null(), null(), null(), null(), null()]),
        _insert('subathon_receipts', [blob(u64(18446744073709551615)), blob(b'synthetic'),
            blob(b'installation'), blob(b'event'), blob(b'receipt-B'), blob(b'synthetic.support'), integer(1),
            null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0),
            blob(b'connection'), blob(b'cfg'), blob(b'ctx')]),
        _insert('subathon_receipt_attempts', [blob(u64(18446744073709551615)), integer(1),
            blob(b'synthetic.support'), integer(1), null(), null(), null(), null(), null(), null(), null(),
            null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx'), integer(0)]),
        _insert('subathon_contributions', [blob(u64(18446744073709551615)), blob(u64(18446744073709551615)),
            null(), null(), null(), null(), integer(3), integer(0), null(), null(), null(), integer(0),
            integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1)]),
        _insert('subathon_contribution_observations', [blob(u64(18446744073709551615)),
            blob(u64(18446744073709551615)), integer(0), integer(3), null(), null(), null(), null(), null(),
            integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(),
            null(), integer(0), integer(0), integer(1)]),
        _insert('subathon_plan_headers', [blob(b'PB'), blob(b'event-B'), null(), blob(b'event-B'),
            blob(u64(18446744073709551615)), blob(u64(18446744073709551615)), blob(u64(1))]),
        _insert('subathon_plan_revisions', [blob(b'PB'), blob(u64(1)), null(), integer(3), integer(0), null(),
            null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0),
            integer(0), integer(1), integer(0), null(), null(), null(), null(), null(), null(), null(), null(),
            null(), null(), null(), null(), null(), null(), null()]),
        _insert('subathon_receipts', [blob(u64(8)), blob(b'synthetic'), blob(b'installation'), blob(b'event'),
            blob(b'receipt-C'), blob(b'synthetic.correction'), integer(1), null(), null(), null(), null(),
            null(), null(), null(), null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx')]),
        _insert('subathon_receipt_attempts', [blob(u64(8)), integer(1), blob(b'synthetic.correction'),
            integer(1), null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0),
            blob(b'connection'), blob(b'cfg'), blob(b'ctx'), integer(0)]),
        _insert('subathon_plan_headers', [blob(b'PC'), blob(b'event-C'), null(), blob(b'event-C'),
            blob(u64(8)), null(), blob(u64(1))]),
        _insert('subathon_plan_revisions', [blob(b'PC'), blob(u64(1)), null(), integer(3), integer(0), null(),
            null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0),
            integer(0), integer(1), integer(0), null(), null(), null(), null(), null(), null(), null(), null(),
            null(), null(), null(), null(), null(), null(), null()]),
        _insert('subathon_plan_rules', [blob(b'PC'), blob(u64(1)), integer(1), blob(b'rule-C'), blob(b'v1'),
            integer(1), blob(b'')]),
        _insert('subathon_plan_requests', [blob(b'PC'), blob(u64(1)), integer(1), blob(b'correction'),
            blob(b'v1'), blob(b'rule-C'), blob(b'main'), integer(0), integer(0), blob(b'-3'), blob(b'1'),
            integer(-3), blob(b'synthetic correction'), blob(b'none')]),
    ]


def _pending_setup():
    rows = _source_foundation()
    rows.extend([
        _insert('subathon_plan_headers', [blob(b'PO'), blob(b'event-PO'), null(), blob(b'event-PO'),
            blob(u64(7)), blob(u64(0)), blob(u64(1))]),
        _insert('subathon_plan_revisions', [blob(b'PO'), blob(u64(1)), null(), integer(3), integer(0), null(),
            null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0),
            integer(0), integer(1), integer(0), null(), null(), null(), null(), null(), null(), null(), null(),
            null(), null(), null(), null(), null(), null(), null()]),
        _insert('subathon_plan_requests', [blob(b'PO'), blob(u64(1)), integer(1), blob(b'time'), blob(b'v1'),
            blob(b'rule'), blob(b'main'), integer(0), integer(0), blob(b'5'), blob(b'1'), integer(5),
            blob(b'synthetic retained request'), blob(b'none')]),
        _insert('subathon_plan_requests', [blob(b'PO'), blob(u64(1)), integer(2), blob(b'score'), blob(b'v1'),
            blob(b'rule'), blob(b'score'), integer(1), integer(0), blob(b'1'), blob(b'1'), integer(1),
            blob(b'synthetic retained request'), blob(b'none')]),
        _insert('subathon_app_states', [blob(b'p'), blob(b's'), blob(OPAQUE), blob(b'cfg'), integer(1),
            integer(0), integer(55), integer(120000), integer(0), integer(0), integer(1)]),
        _insert('subathon_app_balances', [blob(b'p'), integer(2), blob(b'score'), integer(9223372036854775807)]),
        _insert('subathon_app_head', [integer(1), blob(b'p')]),
        _insert('subathon_hold_mutations_detection', [blob(b'dOverflow'), integer(0), integer(0), blob(u64(0)),
            integer(0), integer(1)]),
        _insert('subathon_hold_causes_numeric', [blob(b'hOverflow'), integer(0), blob(u64(0)), blob(u64(0)),
            blob(u64(7)), blob(b'event-PO'), null(), blob(b'event-PO'), blob(b'cfg'), blob(b'ctx'),
            blob(b'scope'), blob(b'hOverflow'), blob(b'c'), blob(OPAQUE), integer(0), null(),
            blob(b'dOverflow'), integer(0), blob(b'PO'), blob(u64(1)), integer(0), null(), null(), null(),
            integer(2), integer(1), integer(2), blob(b'score'), blob(b'9223372036854775808'), blob(b'1'),
            blob(b'f'), blob(OPAQUE)]),
        _insert('subathon_hold_cause_observations', [blob(b'hOverflow'), blob(b'p'), blob(b'o'), blob(OPAQUE),
            null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_occurrences', [blob(b'hOverflow'), integer(1), blob(b'time'), blob(b'v1'),
            blob(b'rule'), blob(b'main'), integer(0), blob(b'5'), blob(b'1'), integer(5),
            blob(b'synthetic retained request')]),
        _insert('subathon_hold_occurrences', [blob(b'hOverflow'), integer(2), blob(b'score'), blob(b'v1'),
            blob(b'rule'), blob(b'score'), integer(1), blob(b'1'), blob(b'1'), integer(1),
            blob(b'synthetic retained request')]),
        _insert('subathon_hold_detections', [blob(b'dOverflow'), blob(b'hOverflow'), null(), blob(b'v1'),
            blob(b'u'), blob(OPAQUE)]),
        _insert('subathon_hold_detection_observations', [blob(b'dOverflow'), blob(b'p'), blob(b'o'),
            blob(OPAQUE), null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_versions', [blob(b'hOverflow'), blob(b'v1'), integer(1), null(), integer(0),
            blob(b'dOverflow'), null(), null()]),
        _insert('subathon_hold_heads', [blob(b'hOverflow'), blob(b'v1'), integer(1)]),
        _insert('subathon_hold_mutations_detection', [blob(b'dPause1'), integer(0), integer(0), blob(u64(0)),
            integer(0), integer(1)]),
        _insert('subathon_hold_causes_policy', [blob(b'hPause'), integer(0), blob(u64(0)), blob(u64(0)),
            blob(u64(7)), blob(b'event-A'), null(), blob(b'event-A'), blob(b'cfg'), blob(b'ctx'),
            blob(b'scope'), blob(b'hPause'), blob(b'c'), blob(OPAQUE), integer(2), null(), blob(b'dPause1'),
            integer(0), blob(b'PA'), blob(u64(1)), integer(0), null(), null(), null(), integer(0),
            blob(b'unresolved synthetic policy')]),
        _insert('subathon_hold_cause_observations', [blob(b'hPause'), blob(b'p'), blob(b'o'), blob(OPAQUE),
            null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_detections', [blob(b'dPause1'), blob(b'hPause'), null(), blob(b'v1'),
            blob(b'u'), blob(OPAQUE)]),
        _insert('subathon_hold_detection_observations', [blob(b'dPause1'), blob(b'p'), blob(b'o'),
            blob(OPAQUE), null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_versions', [blob(b'hPause'), blob(b'v1'), integer(1), null(), integer(0),
            blob(b'dPause1'), null(), null()]),
        _insert('subathon_hold_heads', [blob(b'hPause'), blob(b'v1'), integer(1)]),
        _insert('subathon_hold_mutations_detection', [blob(b'dFail'), integer(0), integer(0),
            blob(u64(18446744073709551615)), integer(0), integer(1)]),
        _insert('subathon_hold_causes_numeric', [blob(b'hFail'), integer(0), blob(u64(18446744073709551615)),
            blob(u64(18446744073709551615)), blob(u64(18446744073709551615)), null(), null(), null(),
            blob(b'cfg'), blob(b'ctx'), blob(b'scope'), blob(b'hFail'), blob(b'c'), blob(OPAQUE), integer(0),
            null(), blob(b'dFail'), integer(1), null(), null(), integer(2), integer(0), blob(b'fc'),
            blob(OPAQUE), integer(2), integer(0), integer(2), blob(b'score'), blob(b'9223372036854775808'),
            blob(b'1'), blob(b'f'), blob(OPAQUE)]),
        _insert('subathon_hold_cause_observations', [blob(b'hFail'), null(), blob(b'o'), blob(OPAQUE), null(),
            null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_occurrences', [blob(b'hFail'), integer(1), null(), null(), null(), blob(b''),
            integer(1), blob(b'0'), blob(b'1'), integer(0), null()]),
        _insert('subathon_hold_occurrences', [blob(b'hFail'), integer(2), null(), null(), null(),
            blob(b'score'), integer(1), blob(b'9223372036854775808'), blob(b'1'), null(), null()]),
        _insert('subathon_hold_detections', [blob(b'dFail'), blob(b'hFail'), null(), blob(b'v1'), blob(b'u'),
            blob(OPAQUE)]),
        _insert('subathon_hold_detection_observations', [blob(b'dFail'), null(), blob(b'o'), blob(OPAQUE),
            null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_versions', [blob(b'hFail'), blob(b'v1'), integer(1), null(), integer(0),
            blob(b'dFail'), null(), null()]),
        _insert('subathon_hold_heads', [blob(b'hFail'), blob(b'v1'), integer(1)]),
        _insert('subathon_hold_mutations_detection', [blob(b'dReceipt'), integer(0), integer(1), blob(u64(8)),
            integer(0), integer(1)]),
        _insert('subathon_hold_causes_policy', [blob(b'hReceipt'), integer(1), blob(u64(8)), null(),
            blob(u64(8)), blob(b'event-C'), null(), blob(b'event-C'), blob(b'cfg'), blob(b'ctx'),
            blob(b'scope'), blob(b'hReceipt'), blob(b'c'), blob(OPAQUE), integer(2), null(), blob(b'dReceipt'),
            integer(0), blob(b'PC'), blob(u64(1)), integer(0), null(), null(), null(), integer(1),
            blob(b'unresolved synthetic policy')]),
        _insert('subathon_hold_cause_observations', [blob(b'hReceipt'), blob(b'p'), blob(b'o'), blob(OPAQUE),
            null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_occurrences', [blob(b'hReceipt'), integer(1), blob(b'correction'), blob(b'v1'),
            blob(b'rule-C'), blob(b'main'), integer(0), blob(b'-3'), blob(b'1'), integer(-3),
            blob(b'synthetic correction')]),
        _insert('subathon_hold_detections', [blob(b'dReceipt'), blob(b'hReceipt'), null(), blob(b'v1'),
            blob(b'u'), blob(OPAQUE)]),
        _insert('subathon_hold_detection_observations', [blob(b'dReceipt'), blob(b'p'), blob(b'o'),
            blob(OPAQUE), null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_versions', [blob(b'hReceipt'), blob(b'v1'), integer(1), null(), integer(0),
            blob(b'dReceipt'), null(), null()]),
        _insert('subathon_hold_heads', [blob(b'hReceipt'), blob(b'v1'), integer(1)]),
        _insert('subathon_hold_mutations_detection', [blob(b'dOld'), integer(0), integer(0),
            blob(u64(18446744073709551615)), integer(0), integer(1)]),
        _insert('subathon_hold_causes_policy', [blob(b'hOld'), integer(0), blob(u64(18446744073709551615)),
            blob(u64(18446744073709551615)), blob(u64(18446744073709551615)), null(), null(), null(),
            blob(b'cfg'), blob(b'ctx'), blob(b'scope'), blob(b'hOld'), blob(b'c'), blob(OPAQUE), integer(2),
            null(), blob(b'dOld'), integer(1), null(), null(), integer(2), integer(1), blob(b'fc'),
            blob(OPAQUE), integer(0), blob(b'unresolved synthetic policy')]),
        _insert('subathon_hold_cause_observations', [blob(b'hOld'), null(), blob(b'o'), blob(OPAQUE), null(),
            null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_detections', [blob(b'dOld'), blob(b'hOld'), null(), blob(b'v1'), blob(b'u'),
            blob(OPAQUE)]),
        _insert('subathon_hold_detection_observations', [blob(b'dOld'), null(), blob(b'o'), blob(OPAQUE),
            null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_versions', [blob(b'hOld'), blob(b'v1'), integer(1), null(), integer(0),
            blob(b'dOld'), null(), null()]),
        _insert('subathon_hold_heads', [blob(b'hOld'), blob(b'v1'), integer(1)]),
    ])
    # The known pause2..65 rows retain exact source bindings and original order.
    for position in range(2, 66):
        mutation = f"dPause{position}".encode("ascii")
        version = f"v{position}".encode("ascii")
        parent = f"v{position - 1}".encode("ascii")
        rows.extend([
            _insert("subathon_hold_mutations_detection", [blob(mutation), integer(0), integer(0),
                                                         blob(u64(0)), integer(0), integer(1)]),
            _insert("subathon_hold_detections", [blob(mutation), blob(b"hPause"), blob(parent),
                                                blob(version), blob(b"u"), blob(OPAQUE)]),
            _insert("subathon_hold_detection_observations", [blob(mutation), blob(b"p"), blob(b"o"),
                     blob(OPAQUE), null(), null(), null(), null(), integer(0), integer(0)]),
            _insert("subathon_hold_versions", [blob(b"hPause"), blob(version), integer(position),
                     blob(parent), integer(0), blob(mutation), null(), null()]),
            _statement("UPDATE main.subathon_hold_heads SET version_ref=?,version_count=? WHERE hold_ref=? AND version_ref=?",
                       [blob(version), integer(position), blob(b"hPause"), blob(parent)]),
        ])
    rows.extend([
        _insert('subathon_hold_mutations_resolution', [blob(b'rReplace'), integer(1), integer(0),
            blob(u64(18446744073709551615)), integer(2), integer(1), blob(b'reviewer'), blob(b'decision'),
            blob(b''), blob(b'options'), blob(OPAQUE)]),
        _insert('subathon_hold_mutations_detection', [blob(b'dNew'), integer(0), integer(0),
            blob(u64(18446744073709551615)), integer(0), integer(1)]),
        _insert('subathon_hold_causes_policy', [blob(b'hNew'), integer(0), blob(u64(18446744073709551615)),
            blob(u64(18446744073709551615)), blob(u64(18446744073709551615)), blob(b'event-B'), null(),
            blob(b'event-B'), blob(b'cfg'), blob(b'ctx'), blob(b'scope'), blob(b'hNew'), blob(b'c'),
            blob(OPAQUE), integer(2), blob(b'hOld'), blob(b'dNew'), integer(1), blob(b'PB'), blob(u64(1)),
            integer(2), integer(1), blob(b'fc'), blob(OPAQUE), integer(0), blob(b'unresolved synthetic policy')]),
        _insert('subathon_hold_cause_observations', [blob(b'hNew'), null(), blob(b'o'), blob(OPAQUE), null(),
            null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_detections', [blob(b'dNew'), blob(b'hNew'), null(), blob(b'v1'), blob(b'u'),
            blob(OPAQUE)]),
        _insert('subathon_hold_detection_observations', [blob(b'dNew'), null(), blob(b'o'), blob(OPAQUE),
            null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_versions', [blob(b'hNew'), blob(b'v1'), integer(1), null(), integer(0),
            blob(b'dNew'), null(), null()]),
        _insert('subathon_hold_versions', [blob(b'hOld'), blob(b'v2'), integer(2), blob(b'v1'), integer(1),
            blob(b'rReplace'), blob(b'hNew'), blob(b'dNew')]),
        _insert('subathon_hold_resolution_expected', [blob(b'rReplace'), blob(b'hFail'), blob(b'v1')]),
        _insert('subathon_hold_resolution_expected', [blob(b'rReplace'), blob(b'hOld'), blob(b'v1')]),
        _insert('subathon_hold_resolution_entries', [blob(b'rReplace'), blob(b'hOld'), blob(b'v1'),
            blob(b'v2'), blob(b'decision'), blob(b''), blob(b'hNew'), blob(b'dNew')]),
        _statement('UPDATE main.subathon_hold_heads SET version_ref=?,version_count=? WHERE hold_ref=? AND version_ref=?', [
            blob(b'v2'), integer(2), blob(b'hOld'), blob(b'v1')]),
        _insert('subathon_hold_heads', [blob(b'hNew'), blob(b'v1'), integer(1)]),
    ])
    return rows


def _timer_setup():
    rows = _source_foundation()
    rows.extend([
        _insert('subathon_plan_headers', [blob(b'PT'), blob(b'event-PT'), null(), blob(b'event-PT'),
            blob(u64(7)), blob(u64(0)), blob(u64(1))]),
        _insert('subathon_plan_revisions', [blob(b'PT'), blob(u64(1)), null(), integer(3), integer(0), null(),
            null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0),
            integer(0), integer(1), integer(0), null(), null(), null(), null(), null(), null(), null(), null(),
            null(), null(), null(), null(), null(), null(), null()]),
        _insert('subathon_plan_requests', [blob(b'PT'), blob(u64(1)), integer(1), blob(b'timer'), blob(b'v1'),
            blob(b'rule'), blob(b'main'), integer(0), integer(0), blob(b'-150'), blob(b'1'), integer(-150),
            blob(b'synthetic retained request'), blob(b'none')]),
        _insert('subathon_plan_requests', [blob(b'PT'), blob(u64(1)), integer(2), blob(b'score'), blob(b'v1'),
            blob(b'rule'), blob(b'score'), integer(1), integer(0), blob(b'2'), blob(b'1'), integer(2),
            blob(b'synthetic retained request'), blob(b'none')]),
        _insert('subathon_app_states', [blob(b't0'), blob(b's'), blob(OPAQUE), blob(b'cfg'), integer(1),
            integer(0), integer(100), integer(120000), integer(0), integer(7), integer(1)]),
        _insert('subathon_app_balances', [blob(b't0'), integer(2), blob(b'score'), integer(10)]),
        _insert('subathon_app_head', [integer(1), blob(b't0')]),
        _insert('subathon_app_states', [blob(b't1'), blob(b's'), blob(OPAQUE), blob(b'cfg'), integer(1),
            integer(0), integer(100), integer(120000), integer(0), integer(7), integer(1)]),
        _insert('subathon_app_balances', [blob(b't1'), integer(2), blob(b'score'), integer(12)]),
        _insert('subathon_app_operations', [blob(b'timer'), blob(b'original'), integer(0), integer(0),
            blob(u64(7)), blob(u64(0)), blob(b'PT'), blob(u64(1)), blob(b't0'), blob(b't0'), blob(b't1'),
            blob(b''), blob(b'c'), blob(OPAQUE), null()]),
        _insert('subathon_app_contributions', [integer(3), integer(0), null(), null(), null(), integer(0),
            integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(1), integer(0), integer(1),
            blob(b'timer'), blob(b'original'), blob(u64(0)), integer(1)]),
        _statement('UPDATE main.subathon_contributions SET application=1 WHERE contribution_id=?', [blob(u64(0))]),
        _insert('subathon_app_claims', [integer(0), blob(u64(0)), blob(b'timer'), blob(b'original')]),
        _insert('subathon_app_effects', [blob(b'timer'), blob(b'original'), integer(1), blob(b'PT'),
            blob(u64(1)), integer(2), integer(0), blob(b'-150'), blob(b'1'), integer(-150), blob(b'0'),
            blob(b'1'), null(), integer(2), integer(1), blob(b'held')]),
        _insert('subathon_app_components', [blob(b'timer'), blob(b'original'), integer(1), integer(0),
            integer(0), blob(b't0'), null()]),
        _insert('subathon_app_components', [blob(b'timer'), blob(b'original'), integer(1), integer(1),
            integer(0), blob(b't0'), null()]),
        _insert('subathon_app_components', [blob(b'timer'), blob(b'original'), integer(1), integer(4),
            integer(-150), blob(b't0'), null()]),
        _insert('subathon_app_effects', [blob(b'timer'), blob(b'original'), integer(2), blob(b'PT'),
            blob(u64(1)), integer(0), integer(1), blob(b'2'), blob(b'1'), integer(2), blob(b'0'), blob(b'1'),
            null(), null(), null(), blob(b'score')]),
        _insert('subathon_app_components', [blob(b'timer'), blob(b'original'), integer(2), integer(2),
            integer(2), blob(b't0'), blob(b'score')]),
        _insert('subathon_app_results', [blob(b'timer'), blob(b'original'), blob(b'r'), blob(OPAQUE),
            integer(2), integer(4), integer(0), integer(0), integer(0)]),
        _insert('subathon_app_audit', [blob(b'timer'), blob(b'original'), null(), null(), blob(b''),
            blob(b'u'), blob(OPAQUE)]),
        _statement('UPDATE main.subathon_app_head SET token=? WHERE singleton=1 AND token=?', [blob(b't1'),
            blob(b't0')]),
        _insert('subathon_hold_mutations_detection', [blob(b'dTimer'), integer(0), integer(0), blob(u64(0)),
            integer(0), integer(1)]),
        _insert('subathon_hold_causes_timer', [blob(b'hTimer'), integer(0), blob(u64(0)), blob(u64(0)),
            blob(u64(7)), blob(b'event-PT'), null(), blob(b'event-PT'), blob(b'cfg'), blob(b'ctx'),
            blob(b'timer'), blob(b'original'), blob(b'c'), blob(OPAQUE), integer(1), null(), blob(b'dTimer'),
            integer(0), blob(b'PT'), blob(u64(1)), integer(0), null(), null(), null(), integer(2), integer(1),
            integer(4), integer(-150), integer(2), integer(1), integer(-150), integer(1), blob(b'mixed'),
            blob(OPAQUE), blob(b'timer'), blob(b'original'), integer(1)]),
        _insert('subathon_hold_cause_observations', [blob(b'hTimer'), blob(b't0'), blob(b'o'), blob(OPAQUE),
            null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_occurrences', [blob(b'hTimer'), integer(1), blob(b'timer'), blob(b'v1'),
            blob(b'rule'), blob(b'main'), integer(0), blob(b'-150'), blob(b'1'), integer(-150),
            blob(b'synthetic retained request')]),
        _insert('subathon_hold_occurrences', [blob(b'hTimer'), integer(2), blob(b'score'), blob(b'v1'),
            blob(b'rule'), blob(b'score'), integer(1), blob(b'2'), blob(b'1'), integer(2),
            blob(b'synthetic retained request')]),
        _insert('subathon_hold_detections', [blob(b'dTimer'), blob(b'hTimer'), null(), blob(b'v1'), blob(b'u'),
            blob(OPAQUE)]),
        _insert('subathon_hold_detection_observations', [blob(b'dTimer'), blob(b't0'), blob(b'o'),
            blob(OPAQUE), null(), null(), null(), null(), integer(0), integer(0)]),
        _insert('subathon_hold_versions', [blob(b'hTimer'), blob(b'v1'), integer(1), null(), integer(0),
            blob(b'dTimer'), null(), null()]),
        _insert('subathon_hold_heads', [blob(b'hTimer'), blob(b'v1'), integer(1)]),
        _insert('subathon_app_states', [blob(b't2'), blob(b's'), blob(OPAQUE), blob(b'cfg'), integer(1),
            integer(0), integer(97), integer(120000), integer(0), integer(7), integer(1)]),
        _insert('subathon_app_balances', [blob(b't2'), integer(2), blob(b'score'), integer(12)]),
        _insert('subathon_app_operations', [blob(b'timer'), blob(b'correction'), integer(0), integer(1),
            blob(u64(8)), null(), blob(b'PC'), blob(u64(1)), blob(b't1'), blob(b't1'), blob(b't2'), blob(b''),
            blob(b'c'), blob(OPAQUE), null()]),
        _insert('subathon_app_claims', [integer(1), blob(u64(8)), blob(b'timer'), blob(b'correction')]),
        _insert('subathon_app_effects', [blob(b'timer'), blob(b'correction'), integer(1), blob(b'PC'),
            blob(u64(1)), integer(2), integer(0), blob(b'-3'), blob(b'1'), integer(-3), blob(b'0'), blob(b'1'),
            null(), integer(0), integer(0), blob(b'synthetic correction')]),
        _insert('subathon_app_components', [blob(b'timer'), blob(b'correction'), integer(1), integer(0),
            integer(-3), blob(b't1'), null()]),
        _insert('subathon_app_components', [blob(b'timer'), blob(b'correction'), integer(1), integer(1),
            integer(0), blob(b't1'), null()]),
        _insert('subathon_app_components', [blob(b'timer'), blob(b'correction'), integer(1), integer(4),
            integer(0), blob(b't1'), null()]),
        _insert('subathon_app_results', [blob(b'timer'), blob(b'correction'), blob(b'r'), blob(OPAQUE),
            integer(1), integer(3), integer(0), integer(0), integer(0)]),
        _insert('subathon_app_audit', [blob(b'timer'), blob(b'correction'), null(), null(), blob(b''),
            blob(b'u'), blob(OPAQUE)]),
        _statement('UPDATE main.subathon_app_head SET token=? WHERE singleton=1 AND token=?', [blob(b't2'),
            blob(b't1')]),
    ])
    return rows


def _pending_queries():
    # Expected result matrices are literal DATA, never evaluated against SQL.
    return [
        {"name": 'pending-originals-and-unclaimed',
         "sql": ('SELECT (SELECT COUNT(*) FROM subathon_app_operations),(SELECT COUNT(*) FROM '
                 'subathon_app_claims),(SELECT COUNT(*) FROM subathon_app_effects),(SELECT COUNT(*) FROM '
                 'subathon_app_jobs),(SELECT COUNT(*) FROM subathon_app_results)'
         ),
         "expected": [
             [integer(0), integer(0), integer(0), integer(0), integer(0)],
         ]},
        {"name": 'typed-global-heads',
         "sql": ('SELECT c.hold_ref,c.subject_kind,hex(c.subject_id),h.version_ref,h.version_count,v.state FROM '
                 'subathon_hold_causes c JOIN subathon_hold_heads h USING(hold_ref) JOIN subathon_hold_versions v ON '
                 'v.hold_ref=h.hold_ref AND v.version_ref=h.version_ref ORDER BY c.hold_ref'
         ),
         "expected": [
             [blob(b'hFail'), integer(0), text('FFFFFFFFFFFFFFFF'), blob(b'v1'), integer(1), integer(0)],
             [blob(b'hNew'), integer(0), text('FFFFFFFFFFFFFFFF'), blob(b'v1'), integer(1), integer(0)],
             [blob(b'hOld'), integer(0), text('FFFFFFFFFFFFFFFF'), blob(b'v2'), integer(2), integer(1)],
             [blob(b'hOverflow'), integer(0), text('0000000000000000'), blob(b'v1'), integer(1), integer(0)],
             [blob(b'hPause'), integer(0), text('0000000000000000'), blob(b'v65'), integer(65), integer(0)],
             [blob(b'hReceipt'), integer(1), text('0000000000000008'), blob(b'v1'), integer(1), integer(0)],
         ]},
        {"name": 'contiguous65',
         "sql": ('SELECT COUNT(*),MIN(position),MAX(position) FROM subathon_hold_versions WHERE '
                 "hold_ref=X'685061757365'"
         ),
         "expected": [
             [integer(65), integer(1), integer(65)],
         ]},
        {"name": 'known-zero-versus-failed-absent',
         "sql": ('SELECT '
                 'ordinal,typeof(finalized_units),finalized_units,hex(attempted_numerator),typeof(target_ref),length(target_ref)'
                 " FROM subathon_hold_occurrences WHERE hold_ref=X'684661696c' ORDER BY ordinal"
         ),
         "expected": [
             [integer(1), text('integer'), integer(0), text('30'), text('blob'), integer(0)],
             [integer(2), text('null'), null(), text('39323233333732303336383534373735383038'), text('blob'),
                 integer(5)],
         ]},
        {"name": 'pending-current-and-allocators',
         "sql": ('SELECT hex((SELECT token FROM subathon_app_head)),(SELECT main_remaining_ms FROM subathon_app_states'
                 " WHERE token=X'70'),(SELECT units FROM subathon_app_balances WHERE token=X'70'),hex((SELECT "
                 'last_allocated FROM subathon_receipt_allocator)),hex((SELECT last_allocated FROM '
                 'subathon_contribution_allocator))'
         ),
         "expected": [
             [text('70'), integer(55), integer(9223372036854775807), text('FFFFFFFFFFFFFFFF'),
                 text('FFFFFFFFFFFFFFFF')],
         ]},
        {"name": 'all-declared-occurrence-counts',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_causes c WHERE c.occurrence_count<>(SELECT COUNT(*) FROM '
                 'subathon_hold_occurrences o WHERE o.hold_ref=c.hold_ref)'
         ),
         "expected": [
             [integer(0)],
         ]},
        {"name": 'all-head-counts-and-terminal-locators',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_heads h WHERE h.version_count<>(SELECT COUNT(*) FROM '
                 'subathon_hold_versions v WHERE v.hold_ref=h.hold_ref) OR h.version_count<>(SELECT position FROM '
                 'subathon_hold_versions v WHERE v.hold_ref=h.hold_ref AND v.version_ref=h.version_ref)'
         ),
         "expected": [
             [integer(0)],
         ]},
        {"name": 'all-parent-position-links',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_versions v WHERE (v.position=1 AND v.parent_version_ref IS NOT '
                 'NULL) OR (v.position>1 AND NOT EXISTS(SELECT 1 FROM subathon_hold_versions p WHERE '
                 'p.hold_ref=v.hold_ref AND p.position=v.position-1 AND p.version_ref=v.parent_version_ref))'
         ),
         "expected": [
             [integer(0)],
         ]},
        {"name": 'all-mutation-role-and-count-collections',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_mutations m WHERE (m.mutation_kind=0 AND ((SELECT COUNT(*) FROM '
                 'subathon_hold_detections d WHERE d.mutation_ref=m.mutation_ref)<>1 OR EXISTS(SELECT 1 FROM '
                 'subathon_hold_resolution_entries e WHERE e.mutation_ref=m.mutation_ref))) OR (m.mutation_kind=1 AND '
                 '(m.expected_count<>(SELECT COUNT(*) FROM subathon_hold_resolution_expected e WHERE '
                 'e.mutation_ref=m.mutation_ref) OR m.entry_count<>(SELECT COUNT(*) FROM '
                 'subathon_hold_resolution_entries e WHERE e.mutation_ref=m.mutation_ref) OR EXISTS(SELECT 1 FROM '
                 'subathon_hold_detections d WHERE d.mutation_ref=m.mutation_ref)))'
         ),
         "expected": [
             [integer(0)],
         ]},
        {"name": 'replacement-both-directions',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_resolution_entries e JOIN subathon_hold_causes n ON '
                 'n.hold_ref=e.replacement_hold_ref JOIN subathon_hold_versions v ON v.hold_ref=e.hold_ref AND '
                 'v.version_ref=e.proposed_version_ref WHERE n.supersedes_hold_ref<>e.hold_ref OR '
                 'n.first_detection_ref<>e.replacement_detection_ref OR v.replacement_hold_ref<>e.replacement_hold_ref'
                 ' OR v.replacement_detection_ref<>e.replacement_detection_ref'
         ),
         "expected": [
             [integer(0)],
         ]},
    ]


def _timer_queries():
    # Expected result matrices are literal DATA, never evaluated against SQL.
    return [
        {"name": 'honest-held-timer-plus-score',
         "sql": ('SELECT request_ordinal,component_kind,units FROM subathon_app_components WHERE '
                 "operation_scope=X'74696d6572' AND operation_ref=X'6f726967696e616c' ORDER BY "
                 'request_ordinal,component_kind'
         ),
         "expected": [
             [integer(1), integer(0), integer(0)],
             [integer(1), integer(1), integer(0)],
             [integer(1), integer(4), integer(-150)],
             [integer(2), integer(2), integer(2)],
         ]},
        {"name": 'distinct-correction-source-and-claims',
         "sql": ('SELECT operation_ref,operation_kind,subject_kind,hex(receipt_id),hex(contribution_id) FROM '
                 'subathon_app_operations ORDER BY operation_ref'
         ),
         "expected": [
             [blob(b'correction'), integer(0), integer(1), text('0000000000000008'), text('')],
             [blob(b'original'), integer(0), integer(0), text('0000000000000007'), text('0000000000000000')],
         ]},
        {"name": 'timer-current',
         "sql": ('SELECT token,main_remaining_ms,time_debt_ms FROM subathon_app_states ORDER BY token'
         ),
         "expected": [
             [blob(b't0'), integer(100), integer(7)],
             [blob(b't1'), integer(100), integer(7)],
             [blob(b't2'), integer(97), integer(7)],
         ]},
        {"name": 'all-declared-occurrence-counts',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_causes c WHERE c.occurrence_count<>(SELECT COUNT(*) FROM '
                 'subathon_hold_occurrences o WHERE o.hold_ref=c.hold_ref)'
         ),
         "expected": [
             [integer(0)],
         ]},
        {"name": 'all-head-counts-and-terminal-locators',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_heads h WHERE h.version_count<>(SELECT COUNT(*) FROM '
                 'subathon_hold_versions v WHERE v.hold_ref=h.hold_ref) OR h.version_count<>(SELECT position FROM '
                 'subathon_hold_versions v WHERE v.hold_ref=h.hold_ref AND v.version_ref=h.version_ref)'
         ),
         "expected": [
             [integer(0)],
         ]},
        {"name": 'all-parent-position-links',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_versions v WHERE (v.position=1 AND v.parent_version_ref IS NOT '
                 'NULL) OR (v.position>1 AND NOT EXISTS(SELECT 1 FROM subathon_hold_versions p WHERE '
                 'p.hold_ref=v.hold_ref AND p.position=v.position-1 AND p.version_ref=v.parent_version_ref))'
         ),
         "expected": [
             [integer(0)],
         ]},
        {"name": 'all-mutation-role-and-count-collections',
         "sql": ('SELECT COUNT(*) FROM subathon_hold_mutations m WHERE (m.mutation_kind=0 AND ((SELECT COUNT(*) FROM '
                 'subathon_hold_detections d WHERE d.mutation_ref=m.mutation_ref)<>1 OR EXISTS(SELECT 1 FROM '
                 'subathon_hold_resolution_entries e WHERE e.mutation_ref=m.mutation_ref))) OR (m.mutation_kind=1 AND '
                 '(m.expected_count<>(SELECT COUNT(*) FROM subathon_hold_resolution_expected e WHERE '
                 'e.mutation_ref=m.mutation_ref) OR m.entry_count<>(SELECT COUNT(*) FROM '
                 'subathon_hold_resolution_entries e WHERE e.mutation_ref=m.mutation_ref) OR EXISTS(SELECT 1 FROM '
                 'subathon_hold_detections d WHERE d.mutation_ref=m.mutation_ref)))'
         ),
         "expected": [
             [integer(0)],
         ]},
    ]


PREFIXES = [('retained_context_vectors.json', '0f76fd2c1bb1766c46a47ddcd38f46c18d4704a8'),
 ('sqlite_receipts_vectors.json', 'a79e5e71a3b9ca101441852c7fb7f26f735e9626'),
 ('sqlite_contributions_vectors.json', '44df9918cc8494dc209e3b1812784a080860e06c'),
 ('sqlite_application_plans_vectors.json', '64eb8b4e0d709bc34ab6d33a4860a62883f06c6b'),
 ('sqlite_application_bundle_vectors.json', 'e8b6c77ef103c0d1f9803ea7eda73aebb0b76d5d')]


def build(seed_root):
    steps = catalogs(seed_root)
    manifest = steps[5]
    detections, versions = _detections_and_versions()
    return {
        "format": FORMAT,
        "provenance": PROVENANCE,
        "prefixes": [
            {"fixture": filename, "version": step["version"], "blob": original_blob,
             "payload_bytes": step["payload_bytes"],
             "payload_hex": migration_payload(step).hex(), "sha256_hex": step["sha256_hex"]}
            for step, (filename, original_blob) in zip(steps[:5], PREFIXES)
        ],
        "manifest": physical_manifest(manifest),
        "opaque_hex": OPAQUE.hex(),
        "causes": _causes(),
        "detections": detections,
        "versions": versions,
        "views": _views(versions),
        "resolutions": _resolutions(),
        "proposals": _proposals(),
        "byte_item_oracles": _byte_item_oracles(),
        "arrangements": {
            'pending': {"provenance": ('NEW never-applied source foundation; original17 raw0..21 plus freshly inserted application0; no '
 'retired app token/claim deleted/reused. Fresh p is initial head; PO complete1 only. hPause65 '
 'actual contiguous versions. Lastheads afterdata.'),
                        "setup": _pending_setup(), "queries": _pending_queries()},
            'timer': {"provenance": ('NEW never-applied source foundation, coherent PT -150 RequireReview plus actual score2, original '
 'claim and hTimer beside complete bundle. Distinct PC Receipt8 correction-3 is a separate actual '
 'original. Remaining entitlement/permission policy not established.'),
                        "setup": _timer_setup(), "queries": _timer_queries()},
        },
        "literal_arithmetic": _literal_arithmetic(),
        "history_pages": _history_pages(),
        "synthetic_data_meanings": _synthetic_data_meanings(),
    }



def render(value):
    """Retain the two original compact setup-row arrays; every other byte uses R1 layout."""
    # Shared rendering first enforces R1's no-float/type/UTF-8 rules. This only
    # preserves the legacy presentation of these two explicit named row arrays.
    rendered = render_standard(value).decode("utf-8")
    for name in ("pending", "timer"):
        rows = value["arrangements"][name]["setup"]
        expanded = json.dumps(rows, ensure_ascii=False, allow_nan=False, indent=2)
        expanded = expanded.replace("\n", "\n      ")
        compact = "[\n" + ",\n".join(
            "        " + json.dumps(row, ensure_ascii=False, allow_nan=False,
                                    separators=(",", ":")) for row in rows
        ) + "\n      ]"
        section = rendered.index(f'\n    "{name}": {{\n')
        start = rendered.index('      "setup": ', section) + len('      "setup": ')
        end = start + len(expanded)
        if rendered[start:end] != expanded:
            raise ValueError(f"Unexpected {name} setup presentation")
        rendered = rendered[:start] + compact + rendered[end:]
    return rendered.encode("utf-8")
