# SPDX-License-Identifier: GPL-3.0-or-later
(
    'Independent bundle literals and finite named SQL arrangements; no SQL evaluation.\n\nThe '
    'archived, independently supplied storage corpus is the oracle. Framing/type\nhelpers only spell '
    'its literal bytes/SQL once; expected semantics remain DATA.\n'
)
from .catalogs import build as catalogs
from .common import blob, integer, migration_payload, null, text, u64


# Original long exact-number anchors; reused only for ASCII/hex framing.
LARGE_NUMERATOR = (
    '211827230783211499573446315835588886399684088457307159941424992892243772443233777676283635477710'
    '754196127291798469005319207821082221508778089447374464773473602124582903365702874253815798206035'
    '670068940671527452304151603217786573226141847445035729311001540261564637954558161930473908943079'
    '422944600113408989753446632513002652653244430787317574401072994922018114559777266806001096512943'
    '154391128767649039563348463431925265921613251732074301069061968589859466025591748372777930887737'
    '607242396353537259452110462391078126893397557496680110401048989851555713007989732693141605785273'
    '302431581234230080246739978337030258694859437339414863274180142251394715259280941142713394873114'
    '840859032855282278079302730351456307114885676973265300438255937647881066522810720908008142246167'
    '989619644242694768688884232719545964627174197323325223274627433470171983112049586613227741932313'
    '706225408711397280104552037480723144165419984779638028985524870910813963060121298640780542569446'
    '627403721645400000635925643930244947533668689405682601545368188205215556187230338459798074286863'
    '228589623849666483978745110265812793880108137927816260894211818225504355447430157819406826133305'
    '351571286914235224890124493481814054530042835492716075473206087736758968734701459337686305611725'
    '30826515239141377'
)
LARGE_DENOMINATOR = (
    '105913615391605749786723157917794443199842044228653579970712496446121886221616888838141817738855'
    '377098063645899234502659603910541110754389044723687232386736801062291451682851437126907899103017'
    '835034470335763726152075801608893286613070923722517864655500770130782318977279080965236954471539'
    '711472300056704494876723316256501326326622215393658787200536497461009057279888633403000548256471'
    '577195564383824519781674231715962632960806625866037150534530984294929733012795874186388965443868'
    '803621198176768629726055231195539063446698778748340055200524494925777856503994866346570802892636'
    '651215790617115040123369989168515129347429718669707431637090071125697357629640470571356697436557'
    '420429516427641139039651365175728153557442838486632650219127968823940533261405360454004071123083'
    '994809822121347384344442116359772982313587098661662611637313716735085991556024793306613870966156'
    '853112704355698640052276018740361572082709992389819014492762435455406981530060649320390271284723'
    '313701860822700000317962821965122473766834344702841300772684094102607778093615169229899037143431'
    '614294811924833241989372555132906396940054068963908130447105909112752177723715078909703413066652'
    '675785643457117612445062246740907027265021417746358037736603043868379484367350729668843152805862'
    '65413257619570688'
)
OPAQUE = b"\xef\xbb\xbfa\x00\xff"


# Explicit INSERT column order from the independently supplied literal arrangements.
# This is source data, never parsed from DDL or observed database metadata.
COLUMNS = {
    'subathon_retained_configurations': 'reference,document',
    'subathon_retained_contexts': 'reference,document,configuration_ref',
    'subathon_receipts': (
        'receipt_id,provider,source_account,identifier_namespace,opaque_id,event_type,schema_version,sour'
        'ce_timestamp_utc_ms,actor_id,actor_display_name,money_original_decimal,money_currency,payment_na'
        'mespace,payment_opaque_id,activity_namespace,activity_opaque_id,server_timestamp_utc_ms,connecti'
        'on_ref,configuration_ref,modifier_context_ref'
    ),
    'subathon_receipt_attempts': (
        'receipt_id,ordinal,event_type,schema_version,source_timestamp_utc_ms,actor_id,actor_display_name'
        ',money_original_decimal,money_currency,payment_namespace,payment_opaque_id,activity_namespace,ac'
        'tivity_opaque_id,server_timestamp_utc_ms,connection_ref,configuration_ref,modifier_context_ref,d'
        'isposition'
    ),
    'subathon_contributions': (
        'contribution_id,original_receipt_id,activity_provider,activity_source_account,activity_namespace'
        ',activity_id,payment_evidence,monetary,original_decimal,currency,resolution_ref,missing_rate,rev'
        'iew,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_ref,application,rever'
        'sal,settlement'
    ),
    'subathon_contribution_observations': (
        'receipt_id,contribution_id,kind,evidence,activity_provider,activity_source_account,activity_name'
        'space,activity_id,verification_ref,support_monetary,support_original_decimal,support_currency,su'
        'pport_resolution_ref,support_missing_rate,support_review,support_evidence_policy,support_policy_'
        'ref,support_decision_choice,support_decision_ref,support_decision_actor_ref,support_application,'
        'support_reversal,support_settlement'
    ),
    'subathon_plan_headers': 'plan_ref,event_ref,parent_event_ref,root_event_ref,receipt_id,contribution_id,head_revision',
    'subathon_plan_revisions': (
        'plan_ref,revision,parent_revision,payment_evidence,monetary,original_decimal,currency,resolution'
        '_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_'
        'ref,application,reversal,settlement,valuation_state,historical_resolution_ref,historical_origina'
        'l_decimal,historical_currency,comparison_currency,effective_rate_decimal,fetched_rate_decimal,sn'
        'apshot_ref,rate_source_ref,rate_timestamp_utc_ms,fetch_timestamp_utc_ms,manual_decision_ref,over'
        'ride_authority,override_actor_ref,override_reason,override_decision_ref'
    ),
    'subathon_plan_requests': (
        'plan_ref,revision,ordinal,action_ref,action_version_ref,rule_ref,target_ref,unit,requires_histor'
        'ical_valuation,exact_numerator,exact_denominator,requested_units,calculation_explanation,reversa'
        'l_behavior_ref'
    ),
    'subathon_plan_rules': 'plan_ref,revision,ordinal,rule_ref,version_ref,selected,explanation',
    'subathon_plan_modifiers': 'plan_ref,revision,ordinal,definition_version_ref,instance_revision_ref,target_ref,group_ref,group_operator,target_operator,original_factor_decimal',
    'subathon_app_states': 'token,format_ref,document,configuration_ref,lifecycle,phase,main_remaining_ms,grace_remaining_ms,awaiting_confirmation,time_debt_ms,balance_count',
    'subathon_app_balances': 'token,balance_kind,target_ref,units',
    'subathon_app_head': 'singleton,token',
    'subathon_app_operations': (
        'operation_scope,operation_ref,operation_kind,subject_kind,receipt_id,contribution_id,plan_ref,pl'
        'an_revision,head_token,before_token,after_token,advance_explanation,choices_format_ref,choices_d'
        'ocument,reversal_ref'
    ),
    'subathon_app_contributions': (
        'operation_scope,operation_ref,contribution_id,payment_evidence,monetary,original_decimal,currenc'
        'y,resolution_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,dec'
        'ision_actor_ref,application,reversal,settlement,application_after'
    ),
    'subathon_app_claims': 'subject_kind,subject_id,operation_scope,operation_ref',
    'subathon_app_effects': (
        'operation_scope,operation_ref,request_ordinal,plan_ref,plan_revision,arithmetic_role,unit,exact_'
        'numerator,exact_denominator,requested_units,residual_numerator,residual_denominator,maximum_main'
        '_ms,deficit_policy,deficit_disposition,application_explanation'
    ),
    'subathon_app_components': 'operation_scope,operation_ref,request_ordinal,component_kind,units,before_token,target_ref',
    'subathon_app_jobs': 'operation_scope,operation_ref,ordinal,job_ref,source_request_ordinal,group_ref,group_ordinal,format_ref,document',
    'subathon_app_results': 'operation_scope,operation_ref,format_ref,document,effect_count,component_count,job_count,transition_count,reversal_count',
    'subathon_app_audit': 'operation_scope,operation_ref,actor_ref,decision_ref,reason,format_ref,document',
    'subathon_app_reversals': (
        'operation_scope,operation_ref,request_ordinal,original_scope,original_operation,original_request'
        '_ordinal,original_component_kind,basis_position,fraction_numerator,fraction_denominator,entitlem'
        'ent_numerator,entitlement_denominator,residual_numerator,residual_denominator,previous_cumulativ'
        'e,cumulative,requested_increment,full_refund'
    ),
    'subathon_app_reversal_heads': 'original_scope,original_operation,original_request_ordinal,original_component_kind,basis_position,cumulative,last_scope,last_operation,last_request_ordinal',
}


def statement(sql, bindings):
    return {"sql": sql, "bindings": bindings}


def insert(table, bindings, *, columns=None):
    """Frame one supplied literal row; optional columns preserve exact declared variants."""
    names = COLUMNS[table] if columns is None else columns
    placeholders = ",".join("?" for _ in bindings)
    return statement(f"INSERT INTO main.{table}({names}) VALUES ({placeholders})", bindings)


def query(name, sql, expected):
    """Attach independently supplied expected rows without evaluating SQL."""
    return {"name": name, "sql": sql, "expected": expected}


def physical_manifest(step):
    return {"catalog_format": 1, "store_identity": "subathon.sqlite-store",
            "supported_starting_versions": list(range(step["version"] + 1)),
            **{key: step[key] for key in ("version", "id", "sql", "expected_schema", "payload_bytes")},
            "payload_hex": migration_payload(step).hex(), "sha256_hex": step["sha256_hex"]}


def _exact(name, numerator, denominator):
    # ASCII hex framing only; no rational reduction or arithmetic result calculation.
    return {"name": name, "numerator": numerator, "denominator": denominator,
            "numerator_hex": numerator.encode("ascii").hex(),
            "denominator_hex": denominator.encode("ascii").hex()}


def _uint64_vectors():
    """Frozen full-range scalar boundary literals."""
    return [
        {'decimal': '0', 'hex': '0000000000000000'},
        {'decimal': '1', 'hex': '0000000000000001'},
        {'decimal': '9007199254740993', 'hex': '0020000000000001'},
        {'decimal': '9223372036854775807', 'hex': '7fffffffffffffff'},
        {'decimal': '9223372036854775808', 'hex': '8000000000000000'},
        {'decimal': '18446744073709551615', 'hex': 'ffffffffffffffff'},
    ]


def _signed64_vectors():
    """Frozen full-range scalar boundary literals."""
    return [{'decimal': '-9223372036854775808'}, {'decimal': '9223372036854775807'}]


def _exact_vectors():
    """Seven independent exact-number anchors, including the long boundary literal."""
    return [
        _exact('zero', '0', '1'),
        _exact('negative_above_uint64', '-18446744073709551617', '2'),
        _exact('half', '1', '2'),
        _exact('negative_half', '-1', '2'),
        _exact('third', '1', '3'),
        _exact('negative_third', '-1', '3'),
        _exact('above_4096_bits', LARGE_NUMERATOR, LARGE_DENOMINATOR),
    ]


def _historical_vectors():
    """Original named independent historical vectors; expected outcomes stay literal."""
    return [
        {
            'name': 'all_optional_present',
            'historical_resolution_ref': blob(b'\x00\xff'),
            'historical_original_decimal': blob(b'20.0000'),
            'historical_currency': blob(b'EUR'),
            'comparison_currency': blob(b'usd'),
            'effective_rate_decimal': blob(b'1.25000'),
            'fetched_rate_decimal': blob(b'1.249900'),
            'snapshot_ref': blob(OPAQUE),
            'rate_source_ref': blob(b''),
            'rate_timestamp_utc_ms': integer(-9223372036854775808),
            'fetch_timestamp_utc_ms': integer(9223372036854775807),
            'manual_decision_ref': blob(b''),
        },
        {
            'name': 'all_optional_absent',
            'historical_resolution_ref': blob(b'\x00\xff'),
            'historical_original_decimal': blob(b'5.0000'),
            'historical_currency': blob(b'EUR'),
            'comparison_currency': blob(b'usd'),
            'effective_rate_decimal': blob(b'1.25000'),
            'fetched_rate_decimal': null(),
            'snapshot_ref': blob(OPAQUE),
            'rate_source_ref': null(),
            'rate_timestamp_utc_ms': null(),
            'fetch_timestamp_utc_ms': null(),
            'manual_decision_ref': null(),
        },
    ]


def _arithmetic_vectors():
    """Original named independent arithmetic vectors; expected outcomes stay literal."""
    return [
        {
            'id': 'three-snapshots',
            'persisted_head_main': '400',
            'advanced_before_main': '0',
            'main_effect_sum': '300',
            'after_main': '300',
            'cas_token': 'start',
            'advanced_before_token': 'advanced-before',
            'after_token': 'original-after',
            'meaning': 'Supplied clock-only advance400ms is an upstream precondition. Arithmetic applies0+300, never400+300; head is advanced only at finalCAS.',
        },
        {
            'id': 'SB01',
            'request': '600000',
            'before_main': '3480000',
            'before_debt': '0',
            'cap': '3600000',
            'main_delta': '120000',
            'debt_delta': '0',
            'cap_limited': '480000',
            'after_main': '3600000',
            'after_debt': '0',
        },
        {
            'id': 'SB03-debt',
            'request': '10',
            'before_main': '55',
            'before_debt': '2',
            'cap': '60',
            'main_delta': '5',
            'debt_delta': '-2',
            'cap_limited': '3',
            'after_main': '60',
            'after_debt': '0',
        },
        {
            'id': 'SB03-above-cap',
            'request': '8',
            'before_main': '100',
            'before_debt': '5',
            'cap': '60',
            'main_delta': '0',
            'debt_delta': '-5',
            'cap_limited': '3',
            'after_main': '100',
            'after_debt': '0',
        },
        {
            'id': 'AT09-stop',
            'request': '-150',
            'before_main': '100',
            'before_debt': '7',
            'main_delta': '-100',
            'debt_delta': '0',
            'unapplied': '-50',
            'after_main': '0',
            'after_debt': '7',
        },
        {
            'id': 'AT09-carry',
            'request': '-150',
            'before_main': '100',
            'before_debt': '7',
            'main_delta': '-100',
            'debt_delta': '50',
            'unapplied': '0',
            'after_main': '0',
            'after_debt': '57',
        },
        {
            'id': 'AT09-review',
            'request': '-150',
            'before_main': '100',
            'before_debt': '7',
            'main_delta': '0',
            'debt_delta': '0',
            'unapplied': '-150',
            'after_main': '100',
            'after_debt': '7',
        },
        {
            'id': 'AT10-min',
            'request': '-9223372036854775808',
            'before_main': '1',
            'before_debt': '0',
            'main_delta': '-1',
            'debt_delta': '9223372036854775807',
            'unapplied': '0',
            'after_main': '0',
            'after_debt': '9223372036854775807',
        },
        {
            'id': 'SB11-half-even-a',
            'exact_numerator': '3',
            'exact_denominator': '2',
            'request': '2',
            'residual_numerator': '-1',
            'residual_denominator': '2',
        },
        {
            'id': 'SB11-half-even-b',
            'exact_numerator': '5',
            'exact_denominator': '2',
            'request': '2',
            'residual_numerator': '1',
            'residual_denominator': '2',
        },
    ]


def _reversal_vectors():
    """Original named independent reversal vectors; expected outcomes stay literal."""
    return [
        {
            'id': 'SB13',
            'original_credits': ['100', '50'],
            'fraction_numerator': '1',
            'fraction_denominator': '5',
            'previous': ['0', '0'],
            'cumulative': ['20', '10'],
            'increments': ['-20', '-10'],
            'request_ordinal': '1',
            'requested_units': '-30',
            'meaning': 'One immutable current request has TWO original basis links; upstream same-financial-basis verification is a precondition.',
        },
        {
            'id': 'multi-quarter',
            'original_credits': ['100', '200'],
            'fraction_numerator': '1',
            'fraction_denominator': '4',
            'previous': ['0', '0'],
            'cumulative': ['25', '50'],
            'increments': ['-25', '-50'],
            'request_ordinal': '1',
            'requested_units': '-75',
        },
        {
            'id': 'multi-full',
            'original_credits': ['100', '200'],
            'fraction_numerator': '1',
            'fraction_denominator': '1',
            'previous': ['25', '50'],
            'cumulative': ['100', '200'],
            'increments': ['-75', '-150'],
            'request_ordinal': '1',
            'requested_units': '-225',
        },
        {
            'id': 'SB14',
            'original_credit': '100',
            'request': '-20',
            'component': '-20',
            'invalid_component': '-1000',
            'fraction_numerator': '1',
            'fraction_denominator': '5',
            'cumulative': '20',
        },
        {
            'id': 'SB15-initial',
            'original_credit': '100',
            'position': '0',
            'cumulative': '0',
            'fraction_numerator': '0',
            'fraction_denominator': '1',
            'full_refund': False,
            'last': None,
        },
        {
            'id': 'SB15-after',
            'original_credit': '100',
            'position': '1',
            'cumulative': '20',
            'fraction_numerator': '1',
            'fraction_denominator': '5',
            'full_refund': False,
            'last': {
                'scope': 'scope',
                'operation': 'refund',
                'request_ordinal': '1',
                'original_operation': 'original',
                'original_request_ordinal': '1',
                'component_kind': '0',
            },
        },
        {
            'id': 'SB05',
            'original_credit': '1',
            'fractions': [['1', '3'], ['2', '3'], ['1', '1']],
            'cumulative': ['0', '1', '1'],
            'increments': ['0', '-1', '0'],
            'residuals': [['1', '3'], ['-1', '3'], ['0', '1']],
            'whole_count_cumulative': ['0', '0', '1'],
        },
        {
            'id': 'AT11',
            'time_credit': '300000',
            'score_credit': '40000000',
            'fractions': [['1', '4'], ['1', '1']],
            'time_cumulative': ['75000', '300000'],
            'time_increments': ['-75000', '-225000'],
            'score_cumulative': ['10000000', '40000000'],
            'score_increments': ['-10000000', '-30000000'],
        },
    ]


def _control_vectors():
    """Original named independent control vectors; expected outcomes stay literal."""
    return [
        {
            'id': 'AT21-abandon',
            'same_unit_duplicate': 'same provisional original record',
            'fresh_second_key': 'subject claimed with state-b freshness',
            'outside_before_commit': 'pending/old-head/no records',
            'outcome': 'known rollback/whole prior snapshot unchanged/no payload',
        },
        {
            'id': 'AT21-commit',
            'same_unit_duplicate': 'same provisional original record',
            'fresh_second_key': 'subject claimed',
            'outcome': 'exactly one complete durable record after actual COMMIT and normal completion',
        },
        {
            'id': 'AT17',
            'fault': 'lose real successful COMMIT acknowledgement',
            'outcome': 'unknown/no normal payload/closed writer/no replay',
            'physical': 'whole bundle may be durable; not pending proof',
        },
        {'id': 'AT18', 'fault': 'lose real successful rollback acknowledgement', 'outcome': 'unknown outranks local reason/no replay'},
        {
            'id': 'AT19',
            'input': 'hard end reached before ordinary credit, even paused/Grace/wait',
            'transitions': ['HardEndReached', 'Finished'],
            'ordinary_credit_may_reopen': False,
        },
        {
            'id': 'AT19-zero-elapsed',
            'input': 'valid Main exhaustion with Grace mode',
            'transitions': ['MainExpired', 'GraceStarted'],
            'timestamps': 'none invented',
        },
        {
            'id': 'AT10-overflow',
            'input': 'Main0/debt0 plus reversal INT64_MIN CarryTimeDebt',
            'outcome': 'required debt exceeds signedMAX; no valid prepared application/partial rows',
        },
    ]


def _literal_setup():
    """All56 ordered independent source/setup rows; no policy decisions."""
    return [
        insert('subathon_retained_configurations', [blob(b'cfg-arrival'), blob(b'\x00\xff')]),
        insert('subathon_retained_configurations', [blob(b'cfg-known'), blob(b'')]),
        insert('subathon_retained_contexts', [blob(b'ctx'), blob(OPAQUE), blob(b'cfg-known')]),
        insert('subathon_receipts', [blob(u64(1)), blob(b'p'), blob(b'a'), blob(b'event'), blob(b'1'), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'20.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-1'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(u64(1)), integer(1), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'20.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-1'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx'), integer(0)]),
        insert('subathon_receipts', [blob(u64(7)), blob(b'p'), blob(b'a'), blob(b'event'), blob(b'7'), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'20.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-7'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(u64(7)), integer(1), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'20.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-7'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx'), integer(0)]),
        insert('subathon_receipts', [blob(u64(8)), blob(b'p'), blob(b'a'), blob(b'event'), blob(b'8'), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'5.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-8'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(u64(8)), integer(1), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'5.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-8'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx'), integer(0)]),
        insert('subathon_receipts', [blob(u64(9)), blob(b'p'), blob(b'a'), blob(b'event'), blob(b'9'), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'15.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-9'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(u64(9)), integer(1), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'15.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-9'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx'), integer(0)]),
        insert('subathon_receipts', [blob(u64(10)), blob(b'p'), blob(b'a'), blob(b'event'), blob(b'10'), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'4.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-10'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(u64(10)), integer(1), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'4.0000'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-10'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx'), integer(0)]),
        insert('subathon_receipts', [blob(u64(18446744073709551615)), blob(b'p'), blob(b'a'), blob(b'event'), blob(b'18446744073709551615'), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'9007199254740993.00000000000000000001'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-18446744073709551615'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(u64(18446744073709551615)), integer(1), blob(b'synthetic.payment'), integer(4294967295), integer(-9223372036854775808), blob(b''), null(), blob(b'9007199254740993.00000000000000000001'), blob(b'EUR'), blob(b'payment'), blob(b'financial-basis'), blob(b'activity'), blob(b'source-18446744073709551615'), integer(9223372036854775807), blob(b''), blob(b'cfg-arrival'), blob(b'ctx'), integer(0)]),
        insert('subathon_contributions', [blob(u64(0)), blob(u64(1)), blob(b'p'), blob(b'a'), blob(b'support'), blob(b'support-0'), integer(2), integer(1), blob(b'20.0000'), blob(b'EUR'), blob(b'current-resolution'), integer(0), integer(2), integer(1), blob(b'current-policy'), integer(1), blob(OPAQUE), blob(b'\x00\xff'), integer(0), integer(0), integer(1)]),
        insert('subathon_contributions', [blob(u64(18446744073709551615)), blob(u64(18446744073709551615)), null(), null(), null(), null(), integer(2), integer(1), blob(b'20.0000'), blob(b'EUR'), blob(b'current-resolution'), integer(0), integer(2), integer(1), blob(b'current-policy'), integer(1), blob(OPAQUE), blob(b'\x00\xff'), integer(0), integer(0), integer(1)]),
        insert('subathon_contribution_observations', [blob(u64(1)), blob(u64(0)), integer(0), integer(0), blob(b'p'), blob(b'a'), blob(b'support'), blob(b'support-0'), blob(b'synthetic-proof'), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1)]),
        insert('subathon_contribution_observations', [blob(u64(7)), blob(u64(0)), integer(1), integer(2), blob(b'p'), blob(b'a'), blob(b'support'), blob(b'support-0'), blob(b'synthetic-proof'), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null()]),
        insert('subathon_contribution_observations', [blob(u64(8)), blob(u64(0)), integer(1), integer(2), blob(b'p'), blob(b'a'), blob(b'support'), blob(b'support-0'), blob(b'synthetic-proof'), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null()]),
        insert('subathon_contribution_observations', [blob(u64(9)), blob(u64(0)), integer(1), integer(2), blob(b'p'), blob(b'a'), blob(b'support'), blob(b'support-0'), blob(b'synthetic-proof'), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null()]),
        insert('subathon_contribution_observations', [blob(u64(10)), blob(u64(0)), integer(1), integer(2), blob(b'p'), blob(b'a'), blob(b'support'), blob(b'support-0'), blob(b'synthetic-proof'), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null()]),
        insert('subathon_contribution_observations', [blob(u64(18446744073709551615)), blob(u64(18446744073709551615)), integer(0), integer(0), null(), null(), null(), null(), null(), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1)]),
        statement('UPDATE main.subathon_receipt_allocator SET last_allocated=? WHERE singleton=1', [blob(u64(18446744073709551615))]),
        statement('UPDATE main.subathon_contribution_allocator SET last_allocated=? WHERE singleton=1', [blob(u64(18446744073709551615))]),
        insert('subathon_plan_headers', [blob(b'plan-P'), blob(b'event-7'), blob(b''), blob(b'root'), blob(u64(7)), blob(u64(0)), blob(u64(1))]),
        insert('subathon_plan_revisions', [blob(b'plan-P'), blob(u64(1)), null(), integer(0), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1), integer(2), blob(b'\x00\xff'), blob(b'20.0000'), blob(b'EUR'), blob(b'usd'), blob(b'1.25000'), blob(b'1.249900'), blob(OPAQUE), blob(b''), integer(-9223372036854775808), integer(9223372036854775807), blob(b''), null(), null(), null(), null()]),
        insert('subathon_plan_requests', [blob(b'plan-P'), blob(u64(1)), integer(1), blob(b'same-action'), blob(b'action-v1'), blob(b'rule-v1'), blob(b'timer'), integer(0), integer(0), blob(b'100'), blob(b'1'), integer(100), blob(OPAQUE), blob(b'reversal-v1')]),
        insert('subathon_plan_requests', [blob(b'plan-P'), blob(u64(1)), integer(2), blob(b'same-action'), blob(b'action-v1'), blob(b'rule-v1'), blob(b'timer'), integer(0), integer(0), blob(b'200'), blob(b'1'), integer(200), blob(OPAQUE), blob(b'reversal-v1')]),
        insert('subathon_plan_requests', [blob(b'plan-P'), blob(u64(1)), integer(3), blob(b'same-action'), blob(b'action-v1'), blob(b'rule-v1'), blob(b'score'), integer(1), integer(0), blob(b'100'), blob(b'1'), integer(100), blob(OPAQUE), blob(b'reversal-v1')]),
        insert('subathon_plan_requests', [blob(b'plan-P'), blob(u64(1)), integer(4), blob(b'same-action'), blob(b'action-v1'), blob(b'rule-v1'), blob(b'count'), integer(2), integer(0), blob(b'1'), blob(b'1'), integer(1), blob(OPAQUE), blob(b'reversal-v1')]),
        insert('subathon_plan_rules', [blob(b'plan-P'), blob(u64(1)), integer(1), blob(b'rule-v1'), blob(b'rule-definition-v1'), integer(0), blob(OPAQUE)]),
        insert('subathon_plan_rules', [blob(b'plan-P'), blob(u64(1)), integer(2), blob(b'rule-v1'), blob(b'rule-definition-v1'), integer(1), blob(OPAQUE)]),
        insert('subathon_plan_modifiers', [blob(b'plan-P'), blob(u64(1)), integer(1), blob(b'modifier-v1'), blob(b'instance-v1'), blob(b'timer'), blob(b'g'), blob(b'multiply'), blob(b'add'), blob(b'1.25000')]),
        insert('subathon_plan_headers', [blob(b'plan-R1'), blob(b'event-8'), null(), blob(b'root'), blob(u64(8)), blob(u64(0)), blob(u64(1))]),
        insert('subathon_plan_revisions', [blob(b'plan-R1'), blob(u64(1)), null(), integer(0), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1), integer(2), blob(b'\x00\xff'), blob(b'5.0000'), blob(b'EUR'), blob(b'usd'), blob(b'1.25000'), null(), blob(OPAQUE), null(), null(), null(), null(), null(), null(), null(), null()]),
        insert('subathon_plan_requests', [blob(b'plan-R1'), blob(u64(1)), integer(1), blob(b'same-action'), blob(b'action-v1'), blob(b'rule-v1'), blob(b'timer'), integer(0), integer(0), blob(b'-75'), blob(b'1'), integer(-75), blob(OPAQUE), blob(b'reversal-v1')]),
        insert('subathon_plan_rules', [blob(b'plan-R1'), blob(u64(1)), integer(1), blob(b'rule-v1'), blob(b'rule-definition-v1'), integer(0), blob(OPAQUE)]),
        insert('subathon_plan_rules', [blob(b'plan-R1'), blob(u64(1)), integer(2), blob(b'rule-v1'), blob(b'rule-definition-v1'), integer(1), blob(OPAQUE)]),
        insert('subathon_plan_modifiers', [blob(b'plan-R1'), blob(u64(1)), integer(1), blob(b'modifier-v1'), blob(b'instance-v1'), blob(b'timer'), blob(b'g'), blob(b'multiply'), blob(b'add'), blob(b'1.25000')]),
        insert('subathon_plan_headers', [blob(b'plan-R2'), blob(b'event-9'), null(), blob(b'root'), blob(u64(9)), blob(u64(0)), blob(u64(1))]),
        insert('subathon_plan_revisions', [blob(b'plan-R2'), blob(u64(1)), null(), integer(0), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1), integer(2), blob(b'\x00\xff'), blob(b'15.0000'), blob(b'EUR'), blob(b'usd'), blob(b'1.25000'), blob(b'1.249900'), blob(OPAQUE), blob(b''), integer(-9223372036854775808), integer(9223372036854775807), blob(b''), null(), null(), null(), null()]),
        insert('subathon_plan_requests', [blob(b'plan-R2'), blob(u64(1)), integer(1), blob(b'same-action'), blob(b'action-v1'), blob(b'rule-v1'), blob(b'timer'), integer(0), integer(0), blob(b'-225'), blob(b'1'), integer(-225), blob(OPAQUE), blob(b'reversal-v1')]),
        insert('subathon_plan_rules', [blob(b'plan-R2'), blob(u64(1)), integer(1), blob(b'rule-v1'), blob(b'rule-definition-v1'), integer(0), blob(OPAQUE)]),
        insert('subathon_plan_rules', [blob(b'plan-R2'), blob(u64(1)), integer(2), blob(b'rule-v1'), blob(b'rule-definition-v1'), integer(1), blob(OPAQUE)]),
        insert('subathon_plan_modifiers', [blob(b'plan-R2'), blob(u64(1)), integer(1), blob(b'modifier-v1'), blob(b'instance-v1'), blob(b'timer'), blob(b'g'), blob(b'multiply'), blob(b'add'), blob(b'1.25000')]),
        insert('subathon_plan_headers', [blob(b'plan-S'), blob(b'event-10'), null(), blob(b'root'), blob(u64(10)), blob(u64(0)), blob(u64(1))]),
        insert('subathon_plan_revisions', [blob(b'plan-S'), blob(u64(1)), null(), integer(0), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1), integer(2), blob(b'\x00\xff'), blob(b'4.0000'), blob(b'EUR'), blob(b'usd'), blob(b'1.25000'), blob(b'1.249900'), blob(OPAQUE), blob(b''), integer(-9223372036854775808), integer(9223372036854775807), blob(b''), null(), null(), null(), null()]),
        insert('subathon_plan_requests', [blob(b'plan-S'), blob(u64(1)), integer(1), blob(b'same-action'), blob(b'action-v1'), blob(b'rule-v1'), blob(b'score'), integer(1), integer(0), blob(b'-20'), blob(b'1'), integer(-20), blob(OPAQUE), blob(b'reversal-v1')]),
        insert('subathon_plan_rules', [blob(b'plan-S'), blob(u64(1)), integer(1), blob(b'rule-v1'), blob(b'rule-definition-v1'), integer(0), blob(OPAQUE)]),
        insert('subathon_plan_rules', [blob(b'plan-S'), blob(u64(1)), integer(2), blob(b'rule-v1'), blob(b'rule-definition-v1'), integer(1), blob(OPAQUE)]),
        insert('subathon_plan_modifiers', [blob(b'plan-S'), blob(u64(1)), integer(1), blob(b'modifier-v1'), blob(b'instance-v1'), blob(b'timer'), blob(b'g'), blob(b'multiply'), blob(b'add'), blob(b'1.25000')]),
        insert('subathon_app_states', [blob(b'start'), blob(b'subathon.storage.synthetic-state.v1'), blob(OPAQUE), blob(b'cfg-arrival'), integer(1), integer(0), integer(400), integer(90000), integer(0), integer(0), integer(2)]),
        insert('subathon_app_balances', [blob(b'start'), integer(2), blob(b'score'), integer(0)]),
        insert('subathon_app_balances', [blob(b'start'), integer(3), blob(b'count'), integer(0)]),
        insert('subathon_app_head', [integer(1), blob(b'start')]),
    ]


def _original_prepare():
    """Ordered original prepare DATA, with every exact typed binding retained."""
    return [
        insert('subathon_app_states', [blob(b'advanced-before'), blob(b'subathon.storage.synthetic-state.v1'), blob(OPAQUE), blob(b'cfg-arrival'), integer(1), integer(0), integer(0), integer(90000), integer(0), integer(0), integer(2)]),
        insert('subathon_app_balances', [blob(b'advanced-before'), integer(2), blob(b'score'), integer(0)]),
        insert('subathon_app_balances', [blob(b'advanced-before'), integer(3), blob(b'count'), integer(0)]),
        insert('subathon_app_states', [blob(b'original-after'), blob(b'subathon.storage.synthetic-state.v1'), blob(OPAQUE), blob(b'cfg-arrival'), integer(1), integer(0), integer(300), integer(90000), integer(0), integer(0), integer(2)]),
        insert('subathon_app_balances', [blob(b'original-after'), integer(2), blob(b'score'), integer(100)]),
        insert('subathon_app_balances', [blob(b'original-after'), integer(3), blob(b'count'), integer(1)]),
        insert('subathon_app_operations', [blob(b'scope'), blob(b'original'), integer(0), integer(0), blob(u64(7)), blob(u64(0)), blob(b'plan-P'), blob(u64(1)), blob(b'start'), blob(b'advanced-before'), blob(b'original-after'), blob(b'synthetic elapsed400ms before application'), blob(b'subathon.storage.synthetic-choice.v1'), blob(b''), null()]),
        insert('subathon_app_contributions', [blob(b'scope'), blob(b'original'), blob(u64(0)), integer(2), integer(1), blob(b'20.0000'), blob(b'EUR'), blob(b'current-resolution'), integer(0), integer(2), integer(1), blob(b'current-policy'), integer(1), blob(OPAQUE), blob(b'\x00\xff'), integer(0), integer(0), integer(1), integer(1)]),
        insert('subathon_app_claims', [integer(0), blob(u64(0)), blob(b'scope'), blob(b'original')]),
        insert('subathon_app_effects', [blob(b'scope'), blob(b'original'), integer(1), blob(b'plan-P'), blob(u64(1)), integer(1), integer(0), blob(b'100'), blob(b'1'), integer(100), blob(b'0'), blob(b'1'), integer(1000), null(), null(), blob(OPAQUE)]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'original'), integer(1), integer(0), integer(100), blob(b'advanced-before'), null()]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'original'), integer(1), integer(1), integer(0), blob(b'advanced-before'), null()]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'original'), integer(1), integer(5), integer(0), blob(b'advanced-before'), null()]),
        insert('subathon_app_effects', [blob(b'scope'), blob(b'original'), integer(2), blob(b'plan-P'), blob(u64(1)), integer(1), integer(0), blob(b'200'), blob(b'1'), integer(200), blob(b'0'), blob(b'1'), integer(1000), null(), null(), blob(OPAQUE)]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'original'), integer(2), integer(0), integer(200), blob(b'advanced-before'), null()]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'original'), integer(2), integer(1), integer(0), blob(b'advanced-before'), null()]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'original'), integer(2), integer(5), integer(0), blob(b'advanced-before'), null()]),
        insert('subathon_app_effects', [blob(b'scope'), blob(b'original'), integer(3), blob(b'plan-P'), blob(u64(1)), integer(0), integer(1), blob(b'100'), blob(b'1'), integer(100), blob(b'0'), blob(b'1'), null(), null(), null(), blob(OPAQUE)]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'original'), integer(3), integer(2), integer(100), blob(b'advanced-before'), blob(b'score')]),
        insert('subathon_app_effects', [blob(b'scope'), blob(b'original'), integer(4), blob(b'plan-P'), blob(u64(1)), integer(0), integer(2), blob(b'1'), blob(b'1'), integer(1), blob(b'0'), blob(b'1'), null(), null(), null(), blob(OPAQUE)]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'original'), integer(4), integer(3), integer(1), blob(b'advanced-before'), blob(b'count')]),
        statement('UPDATE main.subathon_contributions SET application=1 WHERE contribution_id=? AND application=0', [blob(u64(0))]),
    ]


def _original_finish():
    """Ordered original finish DATA, with every exact typed binding retained."""
    return [
        insert('subathon_app_jobs', [blob(b'scope'), blob(b'original'), integer(1), blob(b'job-1'), integer(1), blob(b'g'), integer(1), blob(b'subathon.storage.synthetic-job.v1'), blob(OPAQUE)]),
        insert('subathon_app_jobs', [blob(b'scope'), blob(b'original'), integer(2), blob(b'job-2'), integer(2), blob(b'g'), integer(2), blob(b'subathon.storage.synthetic-job.v1'), blob(b'')]),
        insert('subathon_app_results', [blob(b'scope'), blob(b'original'), blob(b'subathon.storage.synthetic-result.v1'), blob(b''), integer(4), integer(8), integer(2), integer(0), integer(0)]),
        insert('subathon_app_audit', [blob(b'scope'), blob(b'original'), blob(b''), null(), blob(OPAQUE), blob(b'subathon.storage.synthetic-audit.v1'), blob(b'')]),
        statement('UPDATE main.subathon_app_head SET token=? WHERE singleton=1 AND token=?', [blob(b'original-after'), blob(b'start')]),
    ]


def _reversal_one():
    """Ordered reversal one DATA, with every exact typed binding retained."""
    return [
        insert('subathon_app_states', [blob(b'refund1-after'), blob(b'subathon.storage.synthetic-state.v1'), blob(OPAQUE), blob(b'cfg-arrival'), integer(1), integer(0), integer(225), integer(90000), integer(0), integer(0), integer(2)]),
        insert('subathon_app_balances', [blob(b'refund1-after'), integer(2), blob(b'score'), integer(100)]),
        insert('subathon_app_balances', [blob(b'refund1-after'), integer(3), blob(b'count'), integer(1)]),
        insert('subathon_app_operations', [blob(b'scope'), blob(b'refund1'), integer(1), integer(0), blob(u64(8)), blob(u64(0)), blob(b'plan-R1'), blob(u64(1)), blob(b'original-after'), blob(b'original-after'), blob(b'refund1-after'), blob(b''), blob(b'subathon.storage.synthetic-choice.v1'), blob(b''), blob(b'refund-1')]),
        insert('subathon_app_contributions', [blob(b'scope'), blob(b'refund1'), blob(u64(0)), integer(2), integer(1), blob(b'20.0000'), blob(b'EUR'), blob(b'current-resolution'), integer(0), integer(2), integer(1), blob(b'current-policy'), integer(1), blob(OPAQUE), blob(b'\x00\xff'), integer(1), integer(0), integer(1), integer(1)]),
        insert('subathon_app_effects', [blob(b'scope'), blob(b'refund1'), integer(1), blob(b'plan-R1'), blob(u64(1)), integer(2), integer(0), blob(b'-75'), blob(b'1'), integer(-75), blob(b'0'), blob(b'1'), null(), integer(0), integer(0), blob(OPAQUE)]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'refund1'), integer(1), integer(0), integer(-75), blob(b'original-after'), null()]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'refund1'), integer(1), integer(1), integer(0), blob(b'original-after'), null()]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'refund1'), integer(1), integer(4), integer(0), blob(b'original-after'), null()]),
        insert('subathon_app_reversals', [blob(b'scope'), blob(b'refund1'), integer(1), blob(b'scope'), blob(b'original'), integer(1), integer(0), integer(1), blob(b'1'), blob(b'4'), blob(b'25'), blob(b'1'), blob(b'0'), blob(b'1'), integer(0), integer(25), integer(-25), integer(0)]),
        insert('subathon_app_reversal_heads', [blob(b'scope'), blob(b'original'), integer(1), integer(0), integer(1), integer(25), blob(b'scope'), blob(b'refund1'), integer(1)]),
        insert('subathon_app_reversals', [blob(b'scope'), blob(b'refund1'), integer(1), blob(b'scope'), blob(b'original'), integer(2), integer(0), integer(1), blob(b'1'), blob(b'4'), blob(b'50'), blob(b'1'), blob(b'0'), blob(b'1'), integer(0), integer(50), integer(-50), integer(0)]),
        insert('subathon_app_reversal_heads', [blob(b'scope'), blob(b'original'), integer(2), integer(0), integer(1), integer(50), blob(b'scope'), blob(b'refund1'), integer(1)]),
        insert('subathon_app_results', [blob(b'scope'), blob(b'refund1'), blob(b'subathon.storage.synthetic-result.v1'), blob(b''), integer(1), integer(3), integer(0), integer(0), integer(2)]),
        insert('subathon_app_audit', [blob(b'scope'), blob(b'refund1'), blob(b''), null(), blob(OPAQUE), blob(b'subathon.storage.synthetic-audit.v1'), blob(b'')]),
        statement('UPDATE main.subathon_app_head SET token=? WHERE singleton=1 AND token=?', [blob(b'refund1-after'), blob(b'original-after')]),
    ]


def _reversal_two():
    """Ordered reversal two DATA, with every exact typed binding retained."""
    return [
        insert('subathon_app_states', [blob(b'refund2-after'), blob(b'subathon.storage.synthetic-state.v1'), blob(OPAQUE), blob(b'cfg-arrival'), integer(1), integer(0), integer(0), integer(90000), integer(0), integer(0), integer(2)]),
        insert('subathon_app_balances', [blob(b'refund2-after'), integer(2), blob(b'score'), integer(100)]),
        insert('subathon_app_balances', [blob(b'refund2-after'), integer(3), blob(b'count'), integer(1)]),
        insert('subathon_app_operations', [blob(b'scope'), blob(b'refund2'), integer(1), integer(0), blob(u64(9)), blob(u64(0)), blob(b'plan-R2'), blob(u64(1)), blob(b'refund1-after'), blob(b'refund1-after'), blob(b'refund2-after'), blob(b''), blob(b'subathon.storage.synthetic-choice.v1'), blob(b''), blob(b'refund-2')]),
        insert('subathon_app_contributions', [blob(b'scope'), blob(b'refund2'), blob(u64(0)), integer(2), integer(1), blob(b'20.0000'), blob(b'EUR'), blob(b'current-resolution'), integer(0), integer(2), integer(1), blob(b'current-policy'), integer(1), blob(OPAQUE), blob(b'\x00\xff'), integer(1), integer(0), integer(1), integer(1)]),
        insert('subathon_app_effects', [blob(b'scope'), blob(b'refund2'), integer(1), blob(b'plan-R2'), blob(u64(1)), integer(2), integer(0), blob(b'-225'), blob(b'1'), integer(-225), blob(b'0'), blob(b'1'), null(), integer(0), integer(0), blob(OPAQUE)]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'refund2'), integer(1), integer(0), integer(-225), blob(b'refund1-after'), null()]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'refund2'), integer(1), integer(1), integer(0), blob(b'refund1-after'), null()]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'refund2'), integer(1), integer(4), integer(0), blob(b'refund1-after'), null()]),
        insert('subathon_app_reversals', [blob(b'scope'), blob(b'refund2'), integer(1), blob(b'scope'), blob(b'original'), integer(1), integer(0), integer(2), blob(b'1'), blob(b'1'), blob(b'100'), blob(b'1'), blob(b'0'), blob(b'1'), integer(25), integer(100), integer(-75), integer(1)]),
        statement((
            'UPDATE main.subathon_app_reversal_heads SET basis_position=2,cumulative=?,last_operation=? '
            'WHERE original_scope=? AND original_operation=? AND original_request_ordinal=? AND '
            'original_component_kind=0'
        ), [integer(100), blob(b'refund2'), blob(b'scope'), blob(b'original'), integer(1)]),
        insert('subathon_app_reversals', [blob(b'scope'), blob(b'refund2'), integer(1), blob(b'scope'), blob(b'original'), integer(2), integer(0), integer(2), blob(b'1'), blob(b'1'), blob(b'200'), blob(b'1'), blob(b'0'), blob(b'1'), integer(50), integer(200), integer(-150), integer(1)]),
        statement((
            'UPDATE main.subathon_app_reversal_heads SET basis_position=2,cumulative=?,last_operation=? '
            'WHERE original_scope=? AND original_operation=? AND original_request_ordinal=? AND '
            'original_component_kind=0'
        ), [integer(200), blob(b'refund2'), blob(b'scope'), blob(b'original'), integer(2)]),
        insert('subathon_app_results', [blob(b'scope'), blob(b'refund2'), blob(b'subathon.storage.synthetic-result.v1'), blob(b''), integer(1), integer(3), integer(0), integer(0), integer(2)]),
        insert('subathon_app_audit', [blob(b'scope'), blob(b'refund2'), blob(b''), null(), blob(OPAQUE), blob(b'subathon.storage.synthetic-audit.v1'), blob(b'')]),
        statement('UPDATE main.subathon_app_head SET token=? WHERE singleton=1 AND token=?', [blob(b'refund2-after'), blob(b'refund1-after')]),
    ]


def _score_reversal():
    """Ordered score reversal DATA, with every exact typed binding retained."""
    return [
        insert('subathon_app_states', [blob(b'score-after'), blob(b'subathon.storage.synthetic-state.v1'), blob(OPAQUE), blob(b'cfg-arrival'), integer(1), integer(0), integer(0), integer(90000), integer(0), integer(0), integer(2)]),
        insert('subathon_app_balances', [blob(b'score-after'), integer(2), blob(b'score'), integer(80)]),
        insert('subathon_app_balances', [blob(b'score-after'), integer(3), blob(b'count'), integer(1)]),
        insert('subathon_app_operations', [blob(b'scope'), blob(b'score-refund'), integer(1), integer(0), blob(u64(10)), blob(u64(0)), blob(b'plan-S'), blob(u64(1)), blob(b'refund2-after'), blob(b'refund2-after'), blob(b'score-after'), blob(b''), blob(b'subathon.storage.synthetic-choice.v1'), blob(b''), blob(b'score-refund')]),
        insert('subathon_app_contributions', [blob(b'scope'), blob(b'score-refund'), blob(u64(0)), integer(2), integer(1), blob(b'20.0000'), blob(b'EUR'), blob(b'current-resolution'), integer(0), integer(2), integer(1), blob(b'current-policy'), integer(1), blob(OPAQUE), blob(b'\x00\xff'), integer(1), integer(0), integer(1), integer(1)]),
        insert('subathon_app_effects', [blob(b'scope'), blob(b'score-refund'), integer(1), blob(b'plan-S'), blob(u64(1)), integer(0), integer(1), blob(b'-20'), blob(b'1'), integer(-20), blob(b'0'), blob(b'1'), null(), null(), null(), blob(OPAQUE)]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'score-refund'), integer(1), integer(2), integer(-20), blob(b'refund2-after'), blob(b'score')]),
        insert('subathon_app_reversals', [blob(b'scope'), blob(b'score-refund'), integer(1), blob(b'scope'), blob(b'original'), integer(3), integer(2), integer(1), blob(b'1'), blob(b'5'), blob(b'20'), blob(b'1'), blob(b'0'), blob(b'1'), integer(0), integer(20), integer(-20), integer(0)]),
        insert('subathon_app_reversal_heads', [blob(b'scope'), blob(b'original'), integer(3), integer(2), integer(1), integer(20), blob(b'scope'), blob(b'score-refund'), integer(1)]),
        insert('subathon_app_results', [blob(b'scope'), blob(b'score-refund'), blob(b'subathon.storage.synthetic-result.v1'), blob(b''), integer(1), integer(1), integer(0), integer(0), integer(1)]),
        insert('subathon_app_audit', [blob(b'scope'), blob(b'score-refund'), blob(b''), null(), blob(OPAQUE), blob(b'subathon.storage.synthetic-audit.v1'), blob(b'')]),
        statement('UPDATE main.subathon_app_head SET token=? WHERE singleton=1 AND token=?', [blob(b'score-after'), blob(b'refund2-after')]),
    ]


def _boundary_raw_only():
    """Ordered boundary raw only DATA, with every exact typed binding retained."""
    return [
        insert('subathon_app_states', [blob(b'boundary'), blob(b'subathon.storage.synthetic-state.v1'), blob(b''), blob(b'cfg-arrival'), integer(1), integer(0), integer(0), integer(90000), integer(0), integer(0), integer(2)]),
        insert('subathon_app_balances', [blob(b'boundary'), integer(2), blob(b'score'), integer(-9223372036854775808)]),
        insert('subathon_app_balances', [blob(b'boundary'), integer(3), blob(b'count'), integer(9223372036854775807)]),
        insert('subathon_plan_headers', [blob(b'raw-MAX'), blob(b'raw-event'), null(), blob(b'raw-root'), blob(u64(18446744073709551615)), blob(u64(18446744073709551615)), blob(u64(18446744073709551615))]),
        insert('subathon_plan_revisions', [blob(b'raw-MAX'), blob(u64(1)), null(), integer(0), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1), integer(0), null(), null(), null(), null()], columns=(
            'plan_ref,revision,parent_revision,payment_evidence,monetary,original_decimal,currency,resolution'
            '_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_'
            'ref,application,reversal,settlement,valuation_state,override_authority,override_actor_ref,overri'
            'de_reason,override_decision_ref'
        )),
        insert('subathon_plan_revisions', [blob(b'raw-MAX'), blob(b'\xff\xff\xff\xff\xff\xff\xff\xfe'), blob(u64(1)), integer(0), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1), integer(0), integer(2), blob(b'actor'), blob(b'reason'), blob(b'decision')], columns=(
            'plan_ref,revision,parent_revision,payment_evidence,monetary,original_decimal,currency,resolution'
            '_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_'
            'ref,application,reversal,settlement,valuation_state,override_authority,override_actor_ref,overri'
            'de_reason,override_decision_ref'
        )),
        insert('subathon_plan_revisions', [blob(b'raw-MAX'), blob(u64(18446744073709551615)), blob(b'\xff\xff\xff\xff\xff\xff\xff\xfe'), integer(0), integer(1), blob(b'20.0000'), blob(b'EUR'), null(), integer(1), integer(0), integer(0), blob(b'captured-policy'), null(), null(), null(), integer(0), integer(0), integer(1), integer(0), integer(2), blob(b'actor'), blob(b'reason'), blob(b'decision')], columns=(
            'plan_ref,revision,parent_revision,payment_evidence,monetary,original_decimal,currency,resolution'
            '_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_'
            'ref,application,reversal,settlement,valuation_state,override_authority,override_actor_ref,overri'
            'de_reason,override_decision_ref'
        )),
        insert('subathon_plan_requests', [blob(b'raw-MAX'), blob(u64(18446744073709551615)), integer(1), blob(b'raw'), blob(b'v1'), blob(b'r'), blob(b'score'), integer(1), integer(0), blob(LARGE_NUMERATOR.encode("ascii")), blob(LARGE_DENOMINATOR.encode("ascii")), integer(0), blob(b'raw-only'), blob(b'raw-only')]),
        insert('subathon_app_operations', [blob(b'scope'), blob(b'raw-boundary'), integer(0), integer(0), blob(u64(18446744073709551615)), blob(u64(18446744073709551615)), blob(b'raw-MAX'), blob(u64(18446744073709551615)), blob(b'start'), blob(b'start'), blob(b'boundary'), blob(b''), blob(b'subathon.storage.synthetic-choice.v1'), blob(b''), null()]),
        insert('subathon_app_effects', [blob(b'scope'), blob(b'raw-boundary'), integer(1), blob(b'raw-MAX'), blob(u64(18446744073709551615)), integer(0), integer(1), blob(LARGE_NUMERATOR.encode("ascii")), blob(LARGE_DENOMINATOR.encode("ascii")), integer(0), blob(LARGE_NUMERATOR.encode("ascii")), blob(LARGE_DENOMINATOR.encode("ascii")), null(), null(), null(), blob(OPAQUE)]),
        insert('subathon_app_components', [blob(b'scope'), blob(b'raw-boundary'), integer(1), integer(2), integer(0), blob(b'start'), blob(b'score')]),
        insert('subathon_app_states', [blob(b'null-boundary-state'), blob(b'subathon.storage.synthetic-state.v1'), blob(OPAQUE), blob(b'cfg-arrival'), integer(1), integer(0), integer(400), integer(90000), integer(0), integer(0), integer(2)]),
        insert('subathon_app_balances', [blob(b'null-boundary-state'), integer(2), blob(b'score'), integer(0)]),
        insert('subathon_app_balances', [blob(b'null-boundary-state'), integer(3), blob(b'count'), integer(0)]),
        insert('subathon_app_operations', [blob(b'scope'), blob(b'null-boundary'), integer(0), integer(1), blob(u64(1)), null(), blob(b'plan-P'), blob(u64(1)), blob(b'start'), blob(b'start'), blob(b'null-boundary-state'), blob(b''), blob(b'subathon.storage.synthetic-choice.v1'), blob(b''), null()]),
    ]


def _queries():
    """Every original named SELECT and its independently supplied result matrix."""
    return {
        'original': [
            query(
                'state-original-after',
                (
                    'SELECT hex(token),main_remaining_ms,time_debt_ms,(SELECT units FROM subathon_app_balances b '
                    'WHERE b.token=s.token AND balance_kind=2),(SELECT units FROM subathon_app_balances b WHERE '
                    'b.token=s.token AND balance_kind=3) FROM subathon_app_states s WHERE '
                    "token=X'6f726967696e616c2d6166746572'"
                ),
                [[text('6F726967696E616C2D6166746572'), integer(300), integer(0), integer(100), integer(1)]],
            ),
            query(
                'three-snapshots',
                (
                    'SELECT '
                    'h.main_remaining_ms,b.main_remaining_ms,a.main_remaining_ms,hex(o.head_token),hex(o.before_token'
                    '),hex(o.after_token) FROM subathon_app_operations o JOIN subathon_app_states h ON '
                    'h.token=o.head_token JOIN subathon_app_states b ON b.token=o.before_token JOIN '
                    "subathon_app_states a ON a.token=o.after_token WHERE operation_ref=X'6F726967696E616C'"
                ),
                [
                    [
                        integer(400),
                        integer(0),
                        integer(300),
                        text('7374617274'),
                        text('616476616E6365642D6265666F7265'),
                        text('6F726967696E616C2D6166746572'),
                    ],
                ],
            ),
            query(
                'original-bundle-counts',
                "SELECT effect_count,component_count,job_count,transition_count,reversal_count FROM subathon_app_results WHERE operation_ref=X'6F726967696E616C'",
                [[integer(4), integer(8), integer(2), integer(0), integer(0)]],
            ),
            query(
                'current-application-only',
                "SELECT application FROM subathon_contributions WHERE contribution_id=X'0000000000000000'",
                [[integer(1)]],
            ),
            query(
                'group-and-envelope',
                (
                    'SELECT '
                    'ordinal,typeof(source_request_ordinal),source_request_ordinal,hex(job_ref),hex(group_ref),group_'
                    'ordinal,typeof(document),hex(document) FROM subathon_app_jobs ORDER BY ordinal'
                ),
                [
                    [integer(1), text('integer'), integer(1), text('6A6F622D31'), text('67'), integer(1), text('blob'), text('EFBBBF6100FF')],
                    [integer(2), text('integer'), integer(2), text('6A6F622D32'), text('67'), integer(2), text('blob'), text('')],
                ],
            ),
            query(
                'audit-null-empty',
                (
                    'SELECT '
                    'typeof(actor_ref),length(actor_ref),typeof(decision_ref),typeof(reason),hex(reason),typeof(docum'
                    "ent),length(document) FROM subathon_app_audit WHERE operation_ref=X'6F726967696E616C'"
                ),
                [[text('blob'), integer(0), text('null'), text('blob'), text('EFBBBF6100FF'), text('blob'), integer(0)]],
            ),
        ],
        'reversals': [
            query(
                'two-links-one-current-request',
                (
                    'SELECT hex(operation_ref),request_ordinal,COUNT(*),SUM(requested_increment) FROM '
                    'subathon_app_reversals WHERE original_component_kind=0 GROUP BY operation_ref,request_ordinal '
                    'ORDER BY operation_ref'
                ),
                [[text('726566756E6431'), integer(1), integer(2), integer(-75)], [text('726566756E6432'), integer(1), integer(2), integer(-225)]],
            ),
            query(
                'basis-history',
                (
                    'SELECT '
                    'original_request_ordinal,basis_position,previous_cumulative,cumulative,requested_increment,hex(f'
                    'raction_numerator),hex(fraction_denominator),full_refund FROM subathon_app_reversals WHERE '
                    'original_component_kind=0 ORDER BY original_request_ordinal,basis_position'
                ),
                [
                    [integer(1), integer(1), integer(0), integer(25), integer(-25), text('31'), text('34'), integer(0)],
                    [integer(1), integer(2), integer(25), integer(100), integer(-75), text('31'), text('31'), integer(1)],
                    [integer(2), integer(1), integer(0), integer(50), integer(-50), text('31'), text('34'), integer(0)],
                    [integer(2), integer(2), integer(50), integer(200), integer(-150), text('31'), text('31'), integer(1)],
                ],
            ),
            query(
                'both-progress-paths-same-last',
                (
                    'SELECT '
                    'h.original_request_ordinal,h.basis_position,h.cumulative,hex(h.last_operation),r.basis_position,'
                    'r.cumulative,p.basis_position,p.cumulative FROM subathon_app_reversal_heads h JOIN '
                    'subathon_app_reversals r ON r.operation_scope=h.last_scope AND r.operation_ref=h.last_operation '
                    'AND r.request_ordinal=h.last_request_ordinal AND r.original_scope=h.original_scope AND '
                    'r.original_operation=h.original_operation AND '
                    'r.original_request_ordinal=h.original_request_ordinal AND '
                    'r.original_component_kind=h.original_component_kind JOIN subathon_app_reversals p ON '
                    'p.original_scope=h.original_scope AND p.original_operation=h.original_operation AND '
                    'p.original_request_ordinal=h.original_request_ordinal AND '
                    'p.original_component_kind=h.original_component_kind AND p.basis_position=h.basis_position WHERE '
                    'h.original_component_kind=0 ORDER BY h.original_request_ordinal'
                ),
                [
                    [integer(1), integer(2), integer(100), text('726566756E6432'), integer(2), integer(100), integer(2), integer(100)],
                    [integer(2), integer(2), integer(200), text('726566756E6432'), integer(2), integer(200), integer(2), integer(200)],
                ],
            ),
            query(
                'state-refund2-after',
                (
                    'SELECT hex(token),main_remaining_ms,time_debt_ms,(SELECT units FROM subathon_app_balances b '
                    'WHERE b.token=s.token AND balance_kind=2),(SELECT units FROM subathon_app_balances b WHERE '
                    'b.token=s.token AND balance_kind=3) FROM subathon_app_states s WHERE '
                    "token=X'726566756e64322d6166746572'"
                ),
                [[text('726566756E64322D6166746572'), integer(0), integer(0), integer(100), integer(1)]],
            ),
        ],
        'typed': [
            query(
                'nullable-operation-subject',
                "SELECT typeof(contribution_id),hex(contribution_id),subject_kind FROM subathon_app_operations WHERE operation_ref=X'6E756C6C2D626F756E64617279'",
                [[text('null'), text(''), integer(1)]],
            ),
            query(
                'actual-full-unsigned-foreign-fields',
                (
                    'SELECT '
                    'typeof(receipt_id),length(receipt_id),hex(receipt_id),typeof(contribution_id),length(contributio'
                    'n_id),hex(contribution_id),typeof(plan_revision),length(plan_revision),hex(plan_revision) FROM '
                    "subathon_app_operations WHERE operation_ref=X'7261772D626F756E64617279'"
                ),
                [
                    [
                        text('blob'),
                        integer(8),
                        text('FFFFFFFFFFFFFFFF'),
                        text('blob'),
                        integer(8),
                        text('FFFFFFFFFFFFFFFF'),
                        text('blob'),
                        integer(8),
                        text('FFFFFFFFFFFFFFFF'),
                    ],
                ],
            ),
            query(
                'signed-balances',
                "SELECT balance_kind,units,typeof(units) FROM subathon_app_balances WHERE token=X'626F756E64617279' ORDER BY balance_kind",
                [[integer(2), integer(-9223372036854775808), text('integer')], [integer(3), integer(9223372036854775807), text('integer')]],
            ),
            query(
                'large-components-raw-only',
                "SELECT typeof(exact_numerator),hex(exact_numerator),typeof(exact_denominator),hex(exact_denominator) FROM subathon_app_effects WHERE operation_ref=X'7261772D626F756E64617279'",
                [
                    [
                        text('blob'),
                        {
                            'type': 'text',
                            'value': LARGE_NUMERATOR.encode("ascii").hex(),
                        },
                        text('blob'),
                        {
                            'type': 'text',
                            'value': LARGE_DENOMINATOR.encode("ascii").hex(),
                        },
                    ],
                ],
            ),
            query(
                'sparse-aggregate-explicitly-invalid',
                (
                    'SELECT COUNT(*),hex(MIN(revision)),hex(MAX(revision)),(SELECT hex(head_revision) FROM '
                    "subathon_plan_headers WHERE plan_ref=X'7261772D4D4158') FROM subathon_plan_revisions WHERE "
                    "plan_ref=X'7261772D4D4158'"
                ),
                [[integer(3), text('0000000000000001'), text('FFFFFFFFFFFFFFFF'), text('FFFFFFFFFFFFFFFF')]],
            ),
            query(
                'support-source-distinct-from-event',
                (
                    'SELECT '
                    'hex(c.original_receipt_id),hex(p.receipt_id),hex(p.contribution_id),typeof(p.parent_event_ref),l'
                    'ength(p.parent_event_ref) FROM subathon_contributions c JOIN subathon_plan_headers p ON '
                    "p.contribution_id=c.contribution_id WHERE p.plan_ref=X'706C616E2D50'"
                ),
                [[text('0000000000000001'), text('0000000000000007'), text('0000000000000000'), text('blob'), integer(0)]],
            ),
            query(
                'all11-historical-plan-P',
                (
                    'SELECT typeof(historical_resolution_ref),hex(CAST(historical_resolution_ref AS '
                    'BLOB)),typeof(historical_original_decimal),hex(CAST(historical_original_decimal AS '
                    'BLOB)),typeof(historical_currency),hex(CAST(historical_currency AS '
                    'BLOB)),typeof(comparison_currency),hex(CAST(comparison_currency AS '
                    'BLOB)),typeof(effective_rate_decimal),hex(CAST(effective_rate_decimal AS '
                    'BLOB)),typeof(fetched_rate_decimal),hex(CAST(fetched_rate_decimal AS '
                    'BLOB)),typeof(snapshot_ref),hex(CAST(snapshot_ref AS '
                    'BLOB)),typeof(rate_source_ref),hex(CAST(rate_source_ref AS '
                    'BLOB)),typeof(rate_timestamp_utc_ms),hex(CAST(rate_timestamp_utc_ms AS '
                    'BLOB)),typeof(fetch_timestamp_utc_ms),hex(CAST(fetch_timestamp_utc_ms AS '
                    'BLOB)),typeof(manual_decision_ref),hex(CAST(manual_decision_ref AS BLOB)) FROM '
                    "subathon_plan_revisions WHERE plan_ref=X'706c616e2d50'"
                ),
                [
                    [
                        text('blob'),
                        text('00FF'),
                        text('blob'),
                        text('32302E30303030'),
                        text('blob'),
                        text('455552'),
                        text('blob'),
                        text('757364'),
                        text('blob'),
                        text('312E3235303030'),
                        text('blob'),
                        text('312E323439393030'),
                        text('blob'),
                        text('EFBBBF6100FF'),
                        text('blob'),
                        text(''),
                        text('integer'),
                        text('2D39323233333732303336383534373735383038'),
                        text('integer'),
                        text('39323233333732303336383534373735383037'),
                        text('blob'),
                        text(''),
                    ],
                ],
            ),
            query(
                'all11-historical-plan-R1',
                (
                    'SELECT typeof(historical_resolution_ref),hex(CAST(historical_resolution_ref AS '
                    'BLOB)),typeof(historical_original_decimal),hex(CAST(historical_original_decimal AS '
                    'BLOB)),typeof(historical_currency),hex(CAST(historical_currency AS '
                    'BLOB)),typeof(comparison_currency),hex(CAST(comparison_currency AS '
                    'BLOB)),typeof(effective_rate_decimal),hex(CAST(effective_rate_decimal AS '
                    'BLOB)),typeof(fetched_rate_decimal),hex(CAST(fetched_rate_decimal AS '
                    'BLOB)),typeof(snapshot_ref),hex(CAST(snapshot_ref AS '
                    'BLOB)),typeof(rate_source_ref),hex(CAST(rate_source_ref AS '
                    'BLOB)),typeof(rate_timestamp_utc_ms),hex(CAST(rate_timestamp_utc_ms AS '
                    'BLOB)),typeof(fetch_timestamp_utc_ms),hex(CAST(fetch_timestamp_utc_ms AS '
                    'BLOB)),typeof(manual_decision_ref),hex(CAST(manual_decision_ref AS BLOB)) FROM '
                    "subathon_plan_revisions WHERE plan_ref=X'706c616e2d5231'"
                ),
                [
                    [
                        text('blob'),
                        text('00FF'),
                        text('blob'),
                        text('352E30303030'),
                        text('blob'),
                        text('455552'),
                        text('blob'),
                        text('757364'),
                        text('blob'),
                        text('312E3235303030'),
                        text('null'),
                        text(''),
                        text('blob'),
                        text('EFBBBF6100FF'),
                        text('null'),
                        text(''),
                        text('null'),
                        text(''),
                        text('null'),
                        text(''),
                        text('null'),
                        text(''),
                    ],
                ],
            ),
        ],
        'preservation': query(
            'all-current-fields-except-application',
            (
                'SELECT typeof(contribution_id),hex(CAST(contribution_id AS '
                'BLOB)),typeof(original_receipt_id),hex(CAST(original_receipt_id AS '
                'BLOB)),typeof(activity_provider),hex(CAST(activity_provider AS '
                'BLOB)),typeof(activity_source_account),hex(CAST(activity_source_account AS '
                'BLOB)),typeof(activity_namespace),hex(CAST(activity_namespace AS '
                'BLOB)),typeof(activity_id),hex(CAST(activity_id AS '
                'BLOB)),typeof(payment_evidence),hex(CAST(payment_evidence AS '
                'BLOB)),typeof(monetary),hex(CAST(monetary AS '
                'BLOB)),typeof(original_decimal),hex(CAST(original_decimal AS '
                'BLOB)),typeof(currency),hex(CAST(currency AS '
                'BLOB)),typeof(resolution_ref),hex(CAST(resolution_ref AS '
                'BLOB)),typeof(missing_rate),hex(CAST(missing_rate AS BLOB)),typeof(review),hex(CAST(review AS '
                'BLOB)),typeof(evidence_policy),hex(CAST(evidence_policy AS '
                'BLOB)),typeof(policy_ref),hex(CAST(policy_ref AS '
                'BLOB)),typeof(decision_choice),hex(CAST(decision_choice AS '
                'BLOB)),typeof(decision_ref),hex(CAST(decision_ref AS '
                'BLOB)),typeof(decision_actor_ref),hex(CAST(decision_actor_ref AS '
                'BLOB)),typeof(reversal),hex(CAST(reversal AS BLOB)),typeof(settlement),hex(CAST(settlement AS '
                "BLOB)) FROM subathon_contributions WHERE contribution_id=X'0000000000000000'"
            ),
            [
                [
                    text('blob'),
                    text('0000000000000000'),
                    text('blob'),
                    text('0000000000000001'),
                    text('blob'),
                    text('70'),
                    text('blob'),
                    text('61'),
                    text('blob'),
                    text('737570706F7274'),
                    text('blob'),
                    text('737570706F72742D30'),
                    text('integer'),
                    text('32'),
                    text('integer'),
                    text('31'),
                    text('blob'),
                    text('32302E30303030'),
                    text('blob'),
                    text('455552'),
                    text('blob'),
                    text('63757272656E742D7265736F6C7574696F6E'),
                    text('integer'),
                    text('30'),
                    text('integer'),
                    text('32'),
                    text('integer'),
                    text('31'),
                    text('blob'),
                    text('63757272656E742D706F6C696379'),
                    text('integer'),
                    text('31'),
                    text('blob'),
                    text('EFBBBF6100FF'),
                    text('blob'),
                    text('00FF'),
                    text('integer'),
                    text('30'),
                    text('integer'),
                    text('31'),
                ],
            ],
        ),
        'prepared': query(
            'actual-prepared-before-job',
            (
                'SELECT (SELECT count(*) FROM subathon_app_operations),(SELECT count(*) FROM '
                'subathon_app_effects),(SELECT count(*) FROM subathon_app_components),(SELECT count(*) FROM '
                "subathon_app_states WHERE token=X'6F726967696E616C2D6166746572' AND "
                'main_remaining_ms=300),(SELECT application FROM subathon_contributions WHERE '
                "contribution_id=X'0000000000000000'),(SELECT hex(token) FROM subathon_app_head),(SELECT "
                'count(*) FROM subathon_app_jobs),(SELECT count(*) FROM subathon_app_results),(SELECT count(*) '
                'FROM subathon_app_audit)'
            ),
            [[integer(1), integer(4), integer(8), integer(1), integer(1), text('7374617274'), integer(0), integer(0), integer(0)]],
        ),
        'pending': query(
            'independent-earlier-commit-visible',
            (
                'SELECT (SELECT application FROM subathon_contributions WHERE '
                "contribution_id=X'0000000000000000'),(SELECT hex(token) FROM subathon_app_head),(SELECT "
                'count(*) FROM subathon_app_operations),(SELECT count(*) FROM subathon_app_effects),(SELECT '
                'count(*) FROM subathon_app_states)'
            ),
            [[integer(0), text('7374617274'), integer(0), integer(0), integer(1)]],
        ),
    }


def _constraint_refusals():
    """Original constraint refusals literals; no product behavior is inferred."""
    return [
        {
            'name': 'receipt-text',
            'statement': statement('UPDATE subathon_app_operations SET receipt_id=? WHERE operation_ref=?', [text('12345678'), blob(b'original')]),
        },
        {
            'name': 'receipt-integer',
            'statement': statement('UPDATE subathon_app_operations SET receipt_id=? WHERE operation_ref=?', [integer(7), blob(b'original')]),
        },
        {
            'name': 'receipt-seven-bytes',
            'statement': statement('UPDATE subathon_app_operations SET receipt_id=? WHERE operation_ref=?', [blob(b'\x01\x01\x01\x01\x01\x01\x01'), blob(b'original')]),
        },
        {
            'name': 'receipt-nine-bytes',
            'statement': statement('UPDATE subathon_app_operations SET receipt_id=? WHERE operation_ref=?', [blob(b'\x01\x01\x01\x01\x01\x01\x01\x01\x01'), blob(b'original')]),
        },
        {
            'name': 'receipt-zero',
            'statement': statement('UPDATE subathon_app_operations SET receipt_id=? WHERE operation_ref=?', [blob(u64(0)), blob(b'original')]),
        },
        {
            'name': 'receipt-null',
            'statement': statement('UPDATE subathon_app_operations SET receipt_id=? WHERE operation_ref=?', [null(), blob(b'original')]),
        },
        {
            'name': 'missing-typed-source',
            'statement': statement('UPDATE subathon_app_operations SET contribution_id=? WHERE operation_ref=?', [blob(u64(1)), blob(b'original')]),
        },
        {'name': 'required-envelope-null', 'statement': statement('UPDATE subathon_app_states SET document=NULL WHERE token=?', [blob(b'start')])},
        {'name': 'bad-state-phase', 'statement': statement('UPDATE subathon_app_states SET phase=2 WHERE token=?', [blob(b'start')])},
        {'name': 'negative-main', 'statement': statement('UPDATE subathon_app_states SET main_remaining_ms=-1 WHERE token=?', [blob(b'start')])},
        {
            'name': 'incomplete-own-money',
            'statement': statement('UPDATE subathon_app_contributions SET currency=NULL WHERE operation_ref=?', [blob(b'original')]),
        },
        {
            'name': 'incomplete-decision',
            'statement': statement('UPDATE subathon_app_contributions SET decision_actor_ref=NULL WHERE operation_ref=?', [blob(b'original')]),
        },
        {'name': 'duplicate-claim', 'statement': statement('INSERT INTO subathon_app_claims SELECT * FROM subathon_app_claims', [])},
        {
            'name': 'wrong-component-parent',
            'statement': statement('UPDATE subathon_app_components SET request_ordinal=99 WHERE operation_ref=? AND request_ordinal=3', [blob(b'original')]),
        },
        {
            'name': 'wrong-counter-target',
            'statement': statement('UPDATE subathon_app_components SET target_ref=? WHERE operation_ref=? AND request_ordinal=3', [blob(b'missing'), blob(b'original')]),
        },
        {
            'name': 'unexpected-time-target',
            'statement': statement('UPDATE subathon_app_components SET target_ref=? WHERE operation_ref=? AND request_ordinal=1 AND component_kind=0', [blob(b'score'), blob(b'original')]),
        },
        {
            'name': 'noninteger-component',
            'statement': statement('UPDATE subathon_app_components SET units=? WHERE operation_ref=? AND request_ordinal=3', [text('wrong'), blob(b'original')]),
        },
        {
            'name': 'empty-exact',
            'statement': statement('UPDATE subathon_app_effects SET exact_numerator=? WHERE operation_ref=? AND request_ordinal=1', [blob(b''), blob(b'original')]),
        },
        {
            'name': 'wrong-exact-class',
            'statement': statement('UPDATE subathon_app_effects SET exact_denominator=? WHERE operation_ref=? AND request_ordinal=1', [text('1'), blob(b'original')]),
        },
        {
            'name': 'bad-role-unit',
            'statement': statement('UPDATE subathon_app_effects SET unit=2 WHERE operation_ref=? AND request_ordinal=1', [blob(b'original')]),
        },
        {
            'name': 'group-half-null',
            'statement': statement('UPDATE subathon_app_jobs SET group_ordinal=NULL WHERE operation_ref=? AND ordinal=1', [blob(b'original')]),
        },
        {
            'name': 'duplicate-job-ref',
            'statement': statement('UPDATE subathon_app_jobs SET job_ref=? WHERE operation_ref=? AND ordinal=2', [blob(b'job-1'), blob(b'original')]),
        },
        {
            'name': 'group-duplicate-ordinal',
            'statement': statement('UPDATE subathon_app_jobs SET group_ordinal=1 WHERE operation_ref=? AND ordinal=2', [blob(b'original')]),
        },
        {
            'name': 'zero-link-position',
            'statement': statement('UPDATE subathon_app_reversals SET basis_position=0 WHERE operation_ref=? AND original_request_ordinal=1', [blob(b'refund1')]),
        },
        {
            'name': 'duplicate-current-basis',
            'statement': statement('INSERT INTO subathon_app_reversals SELECT * FROM subathon_app_reversals WHERE operation_ref=? AND original_request_ordinal=1', [blob(b'refund1')]),
        },
        {
            'name': 'missing-progress-link',
            'statement': statement('UPDATE subathon_app_reversal_heads SET basis_position=3 WHERE original_component_kind=0 AND original_request_ordinal=1', []),
        },
        {
            'name': 'invalid-refundable-kind',
            'statement': statement('UPDATE subathon_app_reversals SET original_component_kind=5 WHERE operation_ref=?', [blob(b'refund1')]),
        },
        {
            'name': 'zero-receipt-subject',
            'statement': statement("INSERT INTO subathon_app_claims VALUES(1,X'0000000000000000',X'73636F7065',X'726566756E6431')", []),
        },
    ]


def _runtime_only_admissions():
    """Original runtime only admissions literals; no product behavior is inferred."""
    return [
        {
            'name': 'noncanonical-rational',
            'steps': [
                statement('UPDATE subathon_app_effects SET exact_numerator=?,exact_denominator=? WHERE operation_ref=? AND request_ordinal=1', [blob(b'2'), blob(b'4'), blob(b'original')]),
            ],
            'queries': [
                query(
                    'noncanonical-bytes',
                    "SELECT hex(exact_numerator),hex(exact_denominator) FROM subathon_app_effects WHERE operation_ref=X'6F726967696E616C' AND request_ordinal=1",
                    [[text('32'), text('34')]],
                ),
            ],
            'required_runtime_check': 'Runtime must reject canonical grammar/reduction and copied-request mismatch; engine checks BLOB presence only.',
        },
        {
            'name': 'component-conservation',
            'steps': [
                statement('UPDATE subathon_app_components SET units=99 WHERE operation_ref=? AND request_ordinal=1 AND component_kind=0', [blob(b'original')]),
            ],
            'queries': [
                query(
                    'request-v-main',
                    (
                        'SELECT e.requested_units,c.units FROM subathon_app_effects e JOIN subathon_app_components c '
                        "USING(operation_scope,operation_ref,request_ordinal) WHERE e.operation_ref=X'6F726967696E616C' "
                        'AND e.request_ordinal=1 AND c.component_kind=0'
                    ),
                    [[integer(100), integer(99)]],
                ),
            ],
            'required_runtime_check': 'Runtime must check local request partition and advanced-before to after deltas.',
        },
        {
            'name': 'SB14-direct-copy-not-just-aggregate',
            'steps': [
                statement('UPDATE subathon_app_components SET units=-1000 WHERE operation_ref=?', [blob(b'score-refund')]),
                statement('UPDATE subathon_app_balances SET units=-900 WHERE token=? AND balance_kind=2', [blob(b'score-after')]),
            ],
            'queries': [
                query(
                    'SB14-wrong-direct-amount',
                    (
                        'SELECT e.requested_units,c.units,b.units FROM subathon_app_effects e JOIN '
                        'subathon_app_components c USING(operation_scope,operation_ref,request_ordinal) JOIN '
                        'subathon_app_operations o USING(operation_scope,operation_ref) JOIN subathon_app_balances b ON '
                        "b.token=o.after_token AND b.balance_kind=2 WHERE e.operation_ref=X'73636F72652D726566756E64'"
                    ),
                    [[integer(-20), integer(-1000), integer(-900)]],
                ),
            ],
            'required_runtime_check': 'Even though 100-1000=-900, sole direct component must equal copied immutable request-20, not-1000.',
        },
        {
            'name': 'claim-subject-equivalence',
            'steps': [statement('UPDATE subathon_app_claims SET subject_id=?', [blob(u64(1))])],
            'queries': [
                query(
                    'different-typed-subject',
                    'SELECT hex(c.subject_id),hex(o.contribution_id) FROM subathon_app_claims c JOIN subathon_app_operations o USING(operation_scope,operation_ref)',
                    [[text('0000000000000001'), text('0000000000000000')]],
                ),
            ],
            'required_runtime_check': 'Polymorphic claim must match explicitly tagged operation CID/receipt; no fake cross-domain FK.',
        },
        {
            'name': 'head-cumulative-equivalence',
            'steps': [statement('UPDATE subathon_app_reversal_heads SET cumulative=99 WHERE original_request_ordinal=1 AND original_component_kind=0', [])],
            'queries': [
                query(
                    'head-v-last-cumulative',
                    (
                        'SELECT h.cumulative,r.cumulative FROM subathon_app_reversal_heads h JOIN subathon_app_reversals '
                        'r ON r.operation_scope=h.last_scope AND r.operation_ref=h.last_operation AND '
                        'r.request_ordinal=h.last_request_ordinal AND r.original_scope=h.original_scope AND '
                        'r.original_operation=h.original_operation AND '
                        'r.original_request_ordinal=h.original_request_ordinal AND '
                        'r.original_component_kind=h.original_component_kind WHERE h.original_request_ordinal=1 AND '
                        'h.original_component_kind=0'
                    ),
                    [[integer(99), integer(100)]],
                ),
            ],
            'required_runtime_check': 'Both head paths must select SAME position/cumulative/occurrence, not merely existing FKs.',
        },
        {
            'name': 'two-valid-head-paths-different-occurrences',
            'steps': [
                statement('UPDATE subathon_app_reversal_heads SET last_operation=? WHERE original_request_ordinal=1 AND original_component_kind=0', [blob(b'refund1')]),
            ],
            'queries': [
                query(
                    'head-v-last-position',
                    (
                        'SELECT h.basis_position,h.cumulative,r.basis_position,r.cumulative FROM '
                        'subathon_app_reversal_heads h JOIN subathon_app_reversals r ON r.operation_scope=h.last_scope '
                        'AND r.operation_ref=h.last_operation AND r.request_ordinal=h.last_request_ordinal AND '
                        'r.original_scope=h.original_scope AND r.original_operation=h.original_operation AND '
                        'r.original_request_ordinal=h.original_request_ordinal AND '
                        'r.original_component_kind=h.original_component_kind WHERE h.original_request_ordinal=1 AND '
                        'h.original_component_kind=0'
                    ),
                    [[integer(2), integer(100), integer(1), integer(25)]],
                ),
            ],
            'required_runtime_check': 'Individually satisfied FK paths are insufficient; last key must identify actual terminal position2.',
        },
        {
            'name': 'declared-balance-count',
            'steps': [statement('UPDATE subathon_app_states SET balance_count=3 WHERE token=?', [blob(b'start')])],
            'queries': [
                query(
                    'count-mismatch',
                    "SELECT balance_count,(SELECT count(*) FROM subathon_app_balances b WHERE b.token=s.token) FROM subathon_app_states s WHERE token=X'7374617274'",
                    [[integer(3), integer(2)]],
                ),
            ],
            'required_runtime_check': 'Selected closure must compare actual metadata count, not trust declared3.',
        },
        {
            'name': 'declared-result-count',
            'steps': [statement('UPDATE subathon_app_results SET effect_count=? WHERE operation_ref=?', [integer(9223372036854775807), blob(b'original')])],
            'queries': [
                query(
                    'result-count-mismatch',
                    (
                        'SELECT effect_count,(SELECT count(*) FROM subathon_app_effects e WHERE '
                        'e.operation_scope=r.operation_scope AND e.operation_ref=r.operation_ref),typeof(effect_count) '
                        "FROM subathon_app_results r WHERE operation_ref=X'6F726967696E616C'"
                    ),
                    [[integer(9223372036854775807), integer(4), text('integer')]],
                ),
            ],
            'required_runtime_check': 'Actual count/type must be validated before bounds; typedMAX is not a healthy huge application.',
        },
        {
            'name': 'group-gap',
            'steps': [statement('UPDATE subathon_app_jobs SET group_ordinal=3 WHERE operation_ref=? AND ordinal=2', [blob(b'original')])],
            'queries': [
                query(
                    'group-gap',
                    "SELECT group_ordinal FROM subathon_app_jobs WHERE operation_ref=X'6F726967696E616C' ORDER BY ordinal",
                    [[integer(1)], [integer(3)]],
                ),
            ],
            'required_runtime_check': 'Runtime requires contiguous group ordinals1..count; uniqueness alone permits1,3.',
        },
        {
            'name': 'original-application-after-role',
            'steps': [statement('UPDATE subathon_app_contributions SET application_after=0 WHERE operation_ref=?', [blob(b'original')])],
            'queries': [
                query(
                    'wrong-role-after',
                    "SELECT application,application_after FROM subathon_app_contributions WHERE operation_ref=X'6F726967696E616C'",
                    [[integer(0), integer(0)]],
                ),
            ],
            'required_runtime_check': 'Original requires application_after1 while every unrelated historical/current dimension remains separate.',
        },
    ]


def _fault():
    """Original fault literals; no product behavior is inferred."""
    return {
        'create_trigger': (
            'CREATE TRIGGER storage002_job_fault BEFORE INSERT ON subathon_app_jobs WHEN '
            "NEW.operation_scope=X'73636F7065' AND NEW.operation_ref=X'6F726967696E616C' AND EXISTS(SELECT 1 "
            'FROM subathon_app_operations WHERE operation_scope=NEW.operation_scope AND '
            'operation_ref=NEW.operation_ref) AND EXISTS(SELECT 1 FROM subathon_app_effects WHERE '
            'operation_scope=NEW.operation_scope AND operation_ref=NEW.operation_ref AND request_ordinal=1) '
            "AND EXISTS(SELECT 1 FROM subathon_app_states WHERE token=X'6F726967696E616C2D6166746572' AND "
            'main_remaining_ms=300) AND EXISTS(SELECT 1 FROM subathon_contributions WHERE '
            "contribution_id=X'0000000000000000' AND application=1) AND EXISTS(SELECT 1 FROM "
            "subathon_app_head WHERE singleton=1 AND token=X'7374617274') BEGIN SELECT "
            "RAISE(ABORT,'synthetic bundle job fault after prepared state and application patch'); END"
        ),
        'drop_trigger': 'DROP TRIGGER storage002_job_fault',
        'statement': insert('subathon_app_jobs', [blob(b'scope'), blob(b'original'), integer(1), blob(b'job-1'), integer(1), blob(b'g'), integer(1), blob(b'subathon.storage.synthetic-job.v1'), blob(OPAQUE)]),
        'meaning': (
            'Disposable unsupported test trigger; predicate requires THIS pending0 to staged1 application '
            'patch and actual intent/effect/after-state, while head remains old. No accepted catalog trigger '
            'and no future facade reach proof.'
        ),
    }


def build(seed_root):
    """Reproduce the frozen bundle corpus from four pinned seeds/source declarations."""
    steps = catalogs(seed_root)
    return {
        'format': 'sqlite-application-bundle-v1-independent-vectors',
        'provenance': (
            'Reviewed physical14 SQL literals plus immutable prefix1-4 JSON. Manually '
            'enumerated14table/22NULLindex inventory and independent Python stdlib byte '
            'framing/hashlib/Fraction arithmetic. Expected schema/query rows are supplied before engine '
            'observations; never generated by a new producer, manifest, codec, store, SQLite output or root '
            'prototype. Raw scenarios are storage-only synthetic foundation, not validated '
            'application/API/permission/refund-policy proof.'
        ),
        'manifest': physical_manifest(steps[4]),
        'table_columns': [
            {'table': 'subathon_app_states', 'columns': 11},
            {'table': 'subathon_app_balances', 'columns': 4},
            {'table': 'subathon_app_head', 'columns': 2},
            {'table': 'subathon_app_operations', 'columns': 15},
            {'table': 'subathon_app_contributions', 'columns': 19},
            {'table': 'subathon_app_claims', 'columns': 4},
            {'table': 'subathon_app_effects', 'columns': 16},
            {'table': 'subathon_app_components', 'columns': 7},
            {'table': 'subathon_app_transitions', 'columns': 4},
            {'table': 'subathon_app_jobs', 'columns': 9},
            {'table': 'subathon_app_results', 'columns': 9},
            {'table': 'subathon_app_audit', 'columns': 7},
            {'table': 'subathon_app_reversals', 'columns': 18},
            {'table': 'subathon_app_reversal_heads', 'columns': 9},
        ],
        'uint64_vectors': _uint64_vectors(),
        'signed64_vectors': _signed64_vectors(),
        'exact_vectors': _exact_vectors(),
        'historical_vectors': _historical_vectors(),
        'arithmetic_vectors': _arithmetic_vectors(),
        'reversal_vectors': _reversal_vectors(),
        'control_vectors': _control_vectors(),
        'literal_setup': _literal_setup(),
        'scenarios': {
            'original_prepare': _original_prepare(),
            'original_finish': _original_finish(),
            'reversal_one': _reversal_one(),
            'reversal_two': _reversal_two(),
            'score_reversal': _score_reversal(),
            'boundary_raw_only': _boundary_raw_only(),
        },
        'queries': _queries(),
        'constraint_refusals': _constraint_refusals(),
        'runtime_only_admissions': _runtime_only_admissions(),
        'fault': _fault(),
        'boundaries': (
            'Raw-MAX plan has actual rows1/MAX-1/MAX with count3/headMAX: INVALID complete history, never '
            'valid application/exhaustion/import. Raw-boundary and null-boundary operations are '
            'intentionally incomplete storage-class evidence, never valid bundles or supported '
            'source-classification proof. Typed actualBLOB/INTEGER values do not establish canonical '
            'arithmetic, closure or permission. All source/application policy preconditions remain upstream; '
            'Main/debt repayment financial allocation remains disabled.'
        ),
    }
