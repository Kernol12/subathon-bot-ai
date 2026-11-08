# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent original-claim source/priority/byte literals; no query evaluation."""
from .catalogs import build as catalogs
from .common import blob, integer, null, text
from .bundle import insert, query, statement


def _setup():
    """All45 ordered literal rows. PA revision remains setup[9] with caller bindings30–33."""
    return [
        insert('subathon_retained_configurations', [blob(b'cfg'), blob(b'')]),
        insert('subathon_retained_contexts', [blob(b'ctx'), blob(b'cfg'), blob(b'')], columns='reference,configuration_ref,document'),
        statement('UPDATE main.subathon_receipt_allocator SET last_allocated=? WHERE singleton=1', [blob(b'\xff\xff\xff\xff\xff\xff\xff\xff')]),
        statement('UPDATE main.subathon_contribution_allocator SET last_allocated=? WHERE singleton=1', [blob(b'\xff\xff\xff\xff\xff\xff\xff\xff')]),
        insert('subathon_receipts', [blob(b'\x00\x00\x00\x00\x00\x00\x00\x07'), blob(b'synthetic'), blob(b'installation'), blob(b'event'), blob(b'receipt-A'), blob(b'synthetic.support'), integer(1), null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(b'\x00\x00\x00\x00\x00\x00\x00\x07'), integer(1), blob(b'synthetic.support'), integer(1), null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx'), integer(0)]),
        insert('subathon_contributions', [blob(b'\x00\x00\x00\x00\x00\x00\x00\x00'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x07'), null(), null(), null(), null(), integer(3), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(1), integer(0), integer(1)]),
        insert('subathon_contribution_observations', [blob(b'\x00\x00\x00\x00\x00\x00\x00\x07'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x00'), integer(0), integer(3), null(), null(), null(), null(), null(), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1)]),
        insert('subathon_plan_headers', [blob(b'PA'), blob(b'event-A'), null(), blob(b'event-A'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x07'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x00'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01')]),
        insert('subathon_plan_revisions', [blob(b'PA'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), null(), integer(3), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1), integer(0), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null()]),
        insert('subathon_receipts', [blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), blob(b'synthetic'), blob(b'installation'), blob(b'event'), blob(b'receipt-B'), blob(b'synthetic.support'), integer(1), null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), integer(1), blob(b'synthetic.support'), integer(1), null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx'), integer(0)]),
        insert('subathon_contributions', [blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), null(), null(), null(), null(), integer(3), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(1), integer(0), integer(1)]),
        insert('subathon_contribution_observations', [blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), integer(0), integer(3), null(), null(), null(), null(), null(), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1)]),
        insert('subathon_plan_headers', [blob(b'PB'), blob(b'event-B'), null(), blob(b'event-B'), blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01')]),
        insert('subathon_plan_revisions', [blob(b'PB'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), null(), integer(3), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1), integer(0), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null()]),
        insert('subathon_receipts', [blob(b'\x00\x00\x00\x00\x00\x00\x00\x08'), blob(b'synthetic'), blob(b'installation'), blob(b'event'), blob(b'receipt-C'), blob(b'synthetic.correction'), integer(1), null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx')]),
        insert('subathon_receipt_attempts', [blob(b'\x00\x00\x00\x00\x00\x00\x00\x08'), integer(1), blob(b'synthetic.correction'), integer(1), null(), null(), null(), null(), null(), null(), null(), null(), null(), integer(0), blob(b'connection'), blob(b'cfg'), blob(b'ctx'), integer(0)]),
        insert('subathon_plan_headers', [blob(b'PC'), blob(b'event-C'), null(), blob(b'event-C'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x08'), null(), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01')]),
        insert('subathon_plan_revisions', [blob(b'PC'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), null(), integer(3), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1), integer(0), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null(), null()]),
        insert('subathon_plan_rules', [blob(b'PC'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), integer(1), blob(b'rule-C'), blob(b'v1'), integer(1), blob(b'')]),
        insert('subathon_plan_requests', [blob(b'PC'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), integer(1), blob(b'correction'), blob(b'v1'), blob(b'rule-C'), blob(b'main'), integer(0), integer(0), blob(b'-3'), blob(b'1'), integer(-3), blob(b'synthetic correction'), blob(b'none')]),
        insert('subathon_app_states', [blob(b'a'), blob(b's'), blob(b''), blob(b'cfg'), integer(1), integer(0), integer(10), integer(120000), integer(0), integer(0), integer(0)]),
        insert('subathon_app_states', [blob(b'b'), blob(b's'), blob(b''), blob(b'cfg'), integer(1), integer(0), integer(10), integer(120000), integer(0), integer(0), integer(0)]),
        insert('subathon_app_states', [blob(b'c'), blob(b's'), blob(b''), blob(b'cfg'), integer(1), integer(0), integer(10), integer(120000), integer(0), integer(0), integer(0)]),
        insert('subathon_app_states', [blob(b'd'), blob(b's'), blob(b''), blob(b'cfg'), integer(1), integer(0), integer(7), integer(120000), integer(0), integer(0), integer(0)]),
        insert('subathon_app_operations', [blob(b'alpha'), blob(b'A'), integer(0), integer(0), blob(b'\x00\x00\x00\x00\x00\x00\x00\x07'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x00'), blob(b'PA'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), blob(b'a'), blob(b'a'), blob(b'b'), blob(b''), blob(b'c'), blob(b''), null()]),
        insert('subathon_app_contributions', [blob(b'alpha'), blob(b'A'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x00'), integer(3), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1), integer(1)]),
        insert('subathon_app_claims', [integer(0), blob(b'\x00\x00\x00\x00\x00\x00\x00\x00'), blob(b'alpha'), blob(b'A')]),
        insert('subathon_app_results', [blob(b'alpha'), blob(b'A'), blob(b'r'), blob(b''), integer(0), integer(0), integer(0), integer(0), integer(0)]),
        insert('subathon_app_audit', [blob(b'alpha'), blob(b'A'), null(), null(), null(), blob(b'u'), blob(b'')]),
        insert('subathon_app_operations', [blob(b'beta'), blob(b'B'), integer(0), integer(0), blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), blob(b'PB'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), blob(b'b'), blob(b'b'), blob(b'c'), blob(b''), blob(b'c'), blob(b''), null()]),
        insert('subathon_app_contributions', [blob(b'beta'), blob(b'B'), blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), integer(3), integer(0), null(), null(), null(), integer(0), integer(1), integer(1), blob(b'p'), null(), null(), null(), integer(0), integer(0), integer(1), integer(1)]),
        insert('subathon_app_claims', [integer(0), blob(b'\xff\xff\xff\xff\xff\xff\xff\xff'), blob(b'beta'), blob(b'B')]),
        insert('subathon_app_results', [blob(b'beta'), blob(b'B'), blob(b'r'), blob(b''), integer(0), integer(0), integer(0), integer(0), integer(0)]),
        insert('subathon_app_audit', [blob(b'beta'), blob(b'B'), null(), null(), null(), blob(b'u'), blob(b'')]),
        insert('subathon_app_operations', [blob(b'gamma'), blob(b'C'), integer(0), integer(1), blob(b'\x00\x00\x00\x00\x00\x00\x00\x08'), null(), blob(b'PC'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), blob(b'c'), blob(b'c'), blob(b'd'), blob(b''), blob(b'c'), blob(b''), null()]),
        insert('subathon_app_claims', [integer(1), blob(b'\x00\x00\x00\x00\x00\x00\x00\x08'), blob(b'gamma'), blob(b'C')]),
        insert('subathon_app_effects', [blob(b'gamma'), blob(b'C'), integer(1), blob(b'PC'), blob(b'\x00\x00\x00\x00\x00\x00\x00\x01'), integer(2), integer(0), blob(b'-3'), blob(b'1'), integer(-3), blob(b'0'), blob(b'1'), null(), integer(0), integer(0), blob(b'synthetic correction')]),
        insert('subathon_app_components', [blob(b'gamma'), blob(b'C'), integer(1), integer(0), integer(-3), blob(b'c'), null()]),
        insert('subathon_app_components', [blob(b'gamma'), blob(b'C'), integer(1), integer(1), integer(0), blob(b'c'), null()]),
        insert('subathon_app_components', [blob(b'gamma'), blob(b'C'), integer(1), integer(4), integer(0), blob(b'c'), null()]),
        insert('subathon_app_results', [blob(b'gamma'), blob(b'C'), blob(b'r'), blob(b''), integer(1), integer(3), integer(0), integer(0), integer(0)]),
        insert('subathon_app_audit', [blob(b'gamma'), blob(b'C'), null(), null(), null(), blob(b'u'), blob(b'')]),
        insert('subathon_app_head', [integer(1), blob(b'd')]),
    ]


def _queries():
    """Nine named independent SQL/result matrices, preserving type and priority evidence."""
    return [
        query(
            'tagged-global-claims',
            'SELECT subject_kind,hex(subject_id),operation_scope,operation_ref FROM subathon_app_claims ORDER BY operation_scope,operation_ref',
            [
                [integer(0), text('0000000000000000'), blob(b'alpha'), blob(b'A')],
                [integer(0), text('FFFFFFFFFFFFFFFF'), blob(b'beta'), blob(b'B')],
                [integer(1), text('0000000000000008'), blob(b'gamma'), blob(b'C')],
            ],
        ),
        query(
            'original-source-records',
            (
                'SELECT operation_scope,operation_ref,operation_kind,subject_kind,hex(receipt_id),CASE WHEN '
                'contribution_id IS NULL THEN NULL ELSE hex(contribution_id) '
                'END,head_token,before_token,after_token FROM subathon_app_operations ORDER BY '
                'operation_scope,operation_ref'
            ),
            [
                [
                    blob(b'alpha'),
                    blob(b'A'),
                    integer(0),
                    integer(0),
                    text('0000000000000007'),
                    text('0000000000000000'),
                    blob(b'a'),
                    blob(b'a'),
                    blob(b'b'),
                ],
                [
                    blob(b'beta'),
                    blob(b'B'),
                    integer(0),
                    integer(0),
                    text('FFFFFFFFFFFFFFFF'),
                    text('FFFFFFFFFFFFFFFF'),
                    blob(b'b'),
                    blob(b'b'),
                    blob(b'c'),
                ],
                [blob(b'gamma'), blob(b'C'), integer(0), integer(1), text('0000000000000008'), null(), blob(b'c'), blob(b'c'), blob(b'd')],
            ],
        ),
        query(
            'allocators-actual-MAX-exhausted',
            (
                "SELECT 'contribution',typeof(last_allocated),hex(last_allocated) FROM "
                'subathon_contribution_allocator WHERE singleton=1 UNION ALL SELECT '
                "'receipt',typeof(last_allocated),hex(last_allocated) FROM subathon_receipt_allocator WHERE "
                'singleton=1'
            ),
            [[text('contribution'), text('blob'), text('FFFFFFFFFFFFFFFF')], [text('receipt'), text('blob'), text('FFFFFFFFFFFFFFFF')]],
        ),
        query(
            'receipt-classes-unknown-absence',
            (
                'SELECT '
                'typeof(receipt_id),hex(receipt_id),typeof(actor_id),typeof(money_original_decimal),typeof(source'
                '_timestamp_utc_ms),typeof(payment_namespace),typeof(activity_namespace),schema_version,server_ti'
                'mestamp_utc_ms FROM subathon_receipts ORDER BY receipt_id'
            ),
            [
                [text('blob'), text('0000000000000007'), text('null'), text('null'), text('null'), text('null'), text('null'), integer(1), integer(0)],
                [text('blob'), text('0000000000000008'), text('null'), text('null'), text('null'), text('null'), text('null'), integer(1), integer(0)],
                [text('blob'), text('FFFFFFFFFFFFFFFF'), text('null'), text('null'), text('null'), text('null'), text('null'), integer(1), integer(0)],
            ],
        ),
        query(
            'states-empty-document-is-BLOB',
            'SELECT token,typeof(document),length(document),main_remaining_ms,balance_count FROM subathon_app_states ORDER BY token',
            [
                [blob(b'a'), text('blob'), integer(0), integer(10), integer(0)],
                [blob(b'b'), text('blob'), integer(0), integer(10), integer(0)],
                [blob(b'c'), text('blob'), integer(0), integer(10), integer(0)],
                [blob(b'd'), text('blob'), integer(0), integer(7), integer(0)],
            ],
        ),
        query(
            'one-head-and-coherent-source-counts',
            (
                'SELECT (SELECT token FROM subathon_app_head),(SELECT count(*) FROM subathon_receipts),(SELECT '
                'count(*) FROM subathon_receipt_attempts),(SELECT count(*) FROM subathon_contributions),(SELECT '
                'count(*) FROM subathon_contribution_observations),(SELECT count(*) FROM '
                'subathon_plan_headers),(SELECT count(*) FROM subathon_plan_revisions),(SELECT count(*) FROM '
                'subathon_plan_requests),(SELECT count(*) FROM subathon_app_operations),(SELECT count(*) FROM '
                'subathon_app_claims)'
            ),
            [[blob(b'd'), integer(3), integer(3), integer(2), integer(2), integer(3), integer(3), integer(1), integer(3), integer(3)]],
        ),
        query(
            'original-signed-correction',
            (
                'SELECT '
                'o.operation_kind,e.requested_units,e.arithmetic_role,e.deficit_policy,e.deficit_disposition,r.re'
                'versal_count FROM subathon_app_operations o JOIN subathon_app_effects e '
                'USING(operation_scope,operation_ref) JOIN subathon_app_results r '
                "USING(operation_scope,operation_ref) WHERE o.operation_scope=X'67616D6D61' AND "
                "o.operation_ref=X'43'"
            ),
            [[integer(0), integer(-3), integer(2), integer(0), integer(0), integer(0)]],
        ),
        query('no-claimed-orphan-head-or-body', 'PRAGMA foreign_key_check', []),
        query('integrity', 'PRAGMA integrity_check', [[text('ok')]]),
    ]


def _selectors():
    """Only the two FIXTURE-002 metadata fields extend these six unchanged ordered rows."""
    return [
        {
            'subject_kind': 0,
            'receipt_decimal': '18446744073709551615',
            'cid_decimal': '0',
            'expected_kind': 'subject',
            'expected_record': 'A',
            'name': 'contribution-max-receipt-prefers-subject-A',
            'check_lower_priority_corruption': True,
        },
        {
            'subject_kind': 1,
            'receipt_decimal': '18446744073709551615',
            'cid_decimal': None,
            'expected_kind': 'source',
            'expected_record': 'B',
            'name': 'receipt-max-resolves-source-B',
            'check_lower_priority_corruption': False,
        },
        {
            'subject_kind': 0,
            'receipt_decimal': '8',
            'cid_decimal': '0',
            'expected_kind': 'subject',
            'expected_record': 'A',
            'name': 'contribution-receipt8-prefers-subject-A',
            'check_lower_priority_corruption': False,
        },
        {
            'subject_kind': 0,
            'receipt_decimal': '9',
            'cid_decimal': '42',
            'expected_kind': None,
            'expected_record': None,
            'name': 'missing-contribution42-receipt9',
            'check_lower_priority_corruption': False,
        },
        {
            'subject_kind': 1,
            'receipt_decimal': '8',
            'cid_decimal': None,
            'expected_kind': 'subject',
            'expected_record': 'C',
            'name': 'receipt8-selects-original-subject-C',
            'check_lower_priority_corruption': False,
        },
        {
            'subject_kind': 0,
            'receipt_decimal': '8',
            'cid_decimal': '42',
            'expected_kind': 'source',
            'expected_record': 'C',
            'name': 'contribution42-receipt8-resolves-source-C',
            'check_lower_priority_corruption': False,
        },
    ]


def build(seed_root):
    """Reproduce the original claims corpus with only the reviewed selector annotations."""
    steps = catalogs(seed_root)
    return {
        'format': 'sqlite-original-claims-v1-independent-vectors',
        'provenance': (
            'Hand-authored concrete reviewed sqlite-original-claims-v1 data/selector/byte oracles with '
            'unchanged frozen prefix1-5 schema references. No new reader, production '
            'manifest/codec/allocator or observed rows generate expected values. Signed ORIGINAL C is '
            'synthetic storage correction, no financial/refund/current-policy authority. Candidate receipt8 '
            'belongs C and is NOT a claimed linked confirmation for A.'
        ),
        'prefixes': [{"fixture": name, "version": step["version"], "sha256_hex": step["sha256_hex"]}
                     for name, step in zip(['retained_context_vectors.json', 'sqlite_receipts_vectors.json', 'sqlite_contributions_vectors.json', 'sqlite_application_plans_vectors.json', 'sqlite_application_bundle_vectors.json'], steps[:5])],
        'setup': _setup(),
        'queries': _queries(),
        'selectors': _selectors(),
        'logical_byte_oracles': {
            'A': {
                'intent': 45,
                'snapshots': [32, 32, 32],
                'historical_contribution': 18,
                'result': 1,
                'audit': 1,
                'body': 161,
                'matched_wrapper': 162,
                'returned_vector_items': 0,
            },
            'B': {
                'intent': 44,
                'snapshots': [32, 32, 32],
                'historical_contribution': 18,
                'result': 1,
                'audit': 1,
                'body': 160,
                'matched_wrapper': 161,
                'returned_vector_items': 0,
            },
        },
        'old_nonempty_foundation': {
            'fixture': 'sqlite_application_bundle_vectors.json',
            'scenarios': ['original_prepare', 'original_finish'],
            'operation_scope': 'scope',
            'operation_ref': 'original',
            'returned_vector_items': 20,
            'source_receipt_decimal': '7',
            'subject_cid_decimal': '0',
            'remaining_boundary': 'Original raw-MAX fixture rows remain intentionally incomplete; reuse does not relabel them a healthy application or allocator history.',
        },
    }
