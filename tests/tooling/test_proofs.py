# SPDX-License-Identifier: GPL-3.0-or-later
"""MIGRATE-002 literal oracles. Authored only: full test review precedes execution.

Synthetic issue ordinals are fixtures, never real issue mappings. Existing workflow
helpers supply mechanics only; expected proof/context values below are hand authored.
"""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tests/support'))
import proofs
import workflow
from frozen_proof_policy import LEGACY_DONE, LEGACY_DONE_SHA256
from archive_state_fixture import task, state, stage, receipt, publication

REV = 'b' * 40
ARCHIVE = 'c' * 40
NI = 'PROOF_RECORDS_NOT_IMPLEMENTED'
WARNING = ('Recorded information only; Git contents, live GitHub/CI, review identity '
           'and authorization are not verified.')
OMISSIONS = (' Compact view omits complete workflow, other stages, supplemental history, '
             'non-blocking finding details and closing-only finding metrics; use full mode or review links.')


def policy(*ids):
    return dict(version=1, legacy_done=list(LEGACY_DONE), legacy_done_sha256=LEGACY_DONE_SHA256,
                archive_commit=ARCHIVE, required_records=sorted(ids))


def record(task_id='CONVERT-901', number=9901):
    return dict(version=1, task_id=task_id,
                issue=dict(repository='Kernol12/subathon-bot-ai', number=number),
                kind='contract', brief='docs/retired-synthetic.md',
                evidence=['https://example.test/receipt'], limitations=['Only synthetic checks.'])


def reviewed_record():
    value = record()
    rejected = receipt('closing', outcome='changes_requested', findings=['SYN-C1'],
                       blocking_findings=['SYN-C1'])
    packet = stage([receipt('initial', 'a' * 40), receipt('verification'), rejected,
                    receipt('verification'), receipt('closing')])
    packet['authors'] = ['codex:synthetic-author']
    packet['paths'] = ['docs/retired-synthetic.md']
    value['workflow'] = dict(required_stages=['contract'], stages=dict(contract=packet))
    return value


def bundle(value):
    return dict(policy=policy(value['task_id']), records={value['task_id']: value})


def supplemental():
    return json.loads((ROOT / 'tests/support/apitest016-supplemental-b506434.json').read_text())


def supplemental_mutations():
    """Manually selected single edits, independent of any schema/skeleton generator."""
    baseline = supplemental()
    extra = copy.deepcopy(baseline)
    extra['complete_workflow']['stages']['tests']['invented'] = 'extra'
    missing = copy.deepcopy(baseline)
    del missing['authority']
    leaf = copy.deepcopy(baseline)
    leaf['format'] = 17
    length = copy.deepcopy(baseline)
    length['complete_workflow']['required_stages'].append('contract')
    boolean = copy.deepcopy(baseline)
    boolean['complete_workflow']['stages']['implementation']['artifacts']['consumer']['bytes'] = True
    return [extra, missing, leaf, length, boolean]


def immutable_brief_refusals():
    base = f'https://github.com/Kernol12/subathon-bot-ai/blob/{ARCHIVE}/docs/a.md'
    return [base.replace(ARCHIVE, 'main'), base.replace(ARCHIVE, ARCHIVE[:7]),
            base.replace('Kernol12/subathon-bot-ai', 'other/project'),
            base.replace('github.com', 'example.test'), base.replace('/blob/', '/tree/'),
            base.replace('/blob/', '/raw/'), base.replace('https:', 'http:'),
            'https://example.test/brief', base.replace('/docs/a.md', '/docs/../a.md')]


def issue_shape_refusals():
    return [{'repository': 'other/project', 'number': 9901},
            {'repository': 'Kernol12/subathon-bot-ai', 'number': True},
            {'repository': 'Kernol12/subathon-bot-ai', 'number': 0},
            dict(record()['issue'], url='invented')]


def policy_shape_refusals():
    values = []
    for key, value in [('version', True), ('archive_commit', 'short'),
                       ('legacy_done_sha256', 'x'*64), ('unknown', 'field')]:
        bad = policy(); bad[key] = value; values.append(bad)
    for key in policy():
        bad = policy(); del bad[key]; values.append(bad)
    return values


def nested_shape_refusals():
    """One authored shape/enum edit per valid baseline, shared by both validators."""
    baseline = reviewed_record()
    baseline.update(publication=publication(), review={'author': 'historical', 'outcome': 'approved'},
                    verification={'ci': 'passed'})
    baseline['evidence'].append(baseline['publication']['pr'])
    edits = [
        (('workflow', 'unknown'), True),
        (('workflow', 'stages', 'contract', 'unknown'), True),
        (('workflow', 'stages', 'contract', 'reviews', 0, 'unknown'), True),
        (('publication', 'unknown'), True), (('review', 'unknown'), True),
        (('verification', 'ci'), 'green'), (('verification', 'product'), 'probably fine'), (('review', 'outcome'), 'green'),
        (('review', 'author'), 17), (('publication', 'state'), 'closed'),
        (('workflow', 'required_stages'), []),
        (('workflow', 'required_stages'), ['unknown']),
        (('workflow', 'required_stages'), ['contract', 'unknown']),
        (('workflow', 'required_stages'), ['contract', 'contract']),
        (('workflow', 'stages', 'unknown'), copy.deepcopy(baseline['workflow']['stages']['contract'])),
        (('workflow', 'stages', 'contract', 'reviews', 0, 'phase'), 'unknown'),
        (('workflow', 'stages', 'contract', 'reviews', 0, 'provider'), 'unknown'),
        (('workflow', 'stages', 'contract', 'reviews', 0, 'outcome'), 'green'),
        (('workflow', 'stages', 'contract', 'reviews', 0, 'evidence'), 'docs/report.md')]
    values = []
    for path, value in edits:
        bad = copy.deepcopy(baseline); target = bad
        for key in path[:-1]: target = target[key]
        target[path[-1]] = value
        values.append(bad)
    opened = copy.deepcopy(baseline); opened['publication'] = publication('open')
    bad = copy.deepcopy(opened); bad['publication']['merge_commit'] = 'c'*40
    values.append(bad)
    bad = copy.deepcopy(opened); bad['publication']['merged_at'] = '2026-11-13T12:00:00Z'
    values.append(bad)
    return baseline, values


def shape_positive_records():
    """Contract forms with independent literal expectations, never validator output."""
    values = [record(), reviewed_record()]
    archived = record(); archived['brief'] = f'https://github.com/Kernol12/subathon-bot-ai/blob/{ARCHIVE}/docs/a.md'
    archived.update(limitations=['', 'x'*600, 'same', 'same'], evidence=['docs/retired.md'], verification={})
    values.append(archived)
    for review in (None, {}, {'author': 'same', 'reviewer': 'same'}):
        value = record(); value['review'] = review; values.append(value)
    opened = reviewed_record(); opened['publication'] = publication('open')
    opened['evidence'].append(opened['publication']['pr']); values.append(opened)
    timed = reviewed_record(); timed['workflow']['stages']['contract']['reviews'][0].update(
        started_at='2026-11-13T13:00:00+01:00', finished_at='2026-11-13T12:00:10Z')
    values.append(timed)
    minima = {'contract': ['contract'], 'implementation': ['tests', 'implementation'],
              'test_draft': ['tests'], 'test_review': ['tests'], 'test_baseline': ['tests'],
              'code_review': ['implementation'], 'cleanup': ['implementation'],
              'environment': ['implementation'], 'feasibility': ['implementation'], 'scaffold': ['implementation']}
    for kind, names in minima.items():
        value = record(); value['kind'] = kind
        value['workflow'] = dict(required_stages=names, stages={}); values.append(value)
    extra = reviewed_record(); extra['workflow']['required_stages'].append('tests'); values.append(extra)
    rejected = reviewed_record(); rejected['workflow']['stages']['contract']['reviews'] = rejected['workflow']['stages']['contract']['reviews'][:3]
    values.append(rejected)
    values.append(record('ACCESS-002'))
    return values


def mapping(*tasks):
    return dict(version=1, entries=[dict(task_id=t['id'], issue=dict(
        repository='Kernol12/subathon-bot-ai', number=9901+i)) for i, t in enumerate(tasks)])


def source(*tasks):
    data = state(*tasks)
    data['workflow_policy'] = {k: v for k, v in policy().items()
                               if k in ('version', 'legacy_done', 'legacy_done_sha256')}
    return data


class ProofTests(unittest.TestCase):
    def refuses(self, function, *args, **kwargs):
        with self.assertRaises(ValueError) as caught:
            function(*args, **kwargs)
        self.assertNotEqual(str(caught.exception), NI, 'NI is not evidence of semantic refusal')
        self.assertTrue(str(caught.exception).strip())

    def refuses_input(self, function, *args):
        with self.assertRaises((ValueError, OSError)) as caught:
            function(*args)
        self.assertNotIn(NI, str(caught.exception))
        self.assertTrue(str(caught.exception).strip())

    def test_literal_frozen_policy_and_explicit_empty_enrollment(self):
        self.assertEqual(len(LEGACY_DONE), 120)
        self.assertEqual(len(set(LEGACY_DONE)), 120)
        self.assertEqual(hashlib.sha256(('\n'.join(sorted(LEGACY_DONE))+'\n').encode()).hexdigest(),
                         LEGACY_DONE_SHA256)
        self.assertIsNone(proofs.validate_policy(policy()))
        self.assertIsNone(proofs.validate(policy(), []))
        for bad in policy_shape_refusals():
            self.refuses(proofs.validate_policy, bad)
        for key, value in [('legacy_done', list(LEGACY_DONE[:-1])),
                           ('legacy_done_sha256', '0'*64), ('version', True),
                           ('required_records', ['CONVERT-902', 'CONVERT-901']),
                           ('required_records', ['CONVERT-901']*2), ('unknown', 1)]:
            p = policy(); p[key] = value
            self.refuses(proofs.validate_policy, p)
        p = policy(); p['legacy_done'].reverse()
        self.assertIsNone(proofs.validate_policy(p))  # membership, not list order, at load
        p['legacy_done'][0] = 'CONVERT-999'
        p['legacy_done_sha256'] = hashlib.sha256(('\n'.join(sorted(p['legacy_done']))+'\n').encode()).hexdigest()
        self.refuses(proofs.validate_policy, p)  # recomputation cannot create exemptions

    def test_nested_closed_shapes_and_schema_positive_forms(self):
        baseline, negatives = nested_shape_refusals()
        self.assertIsNone(proofs.validate_record(baseline))
        for value in shape_positive_records():
            self.assertIsNone(proofs.validate_record(value))
        for bad in negatives:
            self.refuses(proofs.validate_record, bad)

    def test_record_shapes_and_lexical_references(self):
        valid = record()
        self.assertIsNone(proofs.validate_record(valid))
        good = record(); good['evidence'] = ['https://github.com/example/project/pull/42']
        self.assertIsNone(proofs.validate_record(good))
        for value in ('http://example.com/pull/44',):
            bad = copy.deepcopy(good); bad['evidence'] = [value]
            self.refuses(proofs.validate_record, bad)
        path_control = record(); path_control['workflow'] = dict(required_stages=['contract'], stages={
            'contract':dict(authors=['a'],revision=REV,paths=['docs/retired-synthetic.md'],reviews=[])})
        self.assertIsNone(proofs.validate_record(path_control))
        for value in ('../secret', '/tmp/secret', 'https://example.com/x', 'javascript:alert(1)', 'C:\\secret'):
            bad = record(); bad['workflow'] = dict(required_stages=['contract'], stages={
                'contract':dict(authors=['a'],revision=REV,paths=[value],reviews=[])})
            self.refuses(proofs.validate_record, bad)
        for key, value in [('version', True), ('version', 1.0), ('version', 2),
                           ('task_id', 'CONVERT-901\n'), ('kind', 'epic'), ('owner', 'invented'),
                           ('limitations', None), ('brief', '../outside'), ('brief', 'a//b'),
                           ('brief', '/abs'), ('brief', 'a/./b'), ('brief', 'a\\b'),
                           ('brief', 'a:b'), ('brief', 'a\x00b'), ('evidence', ['']),
                           ('evidence', ['https://user:pass@example.test/x']),
                           ('evidence', ['https://example.test:bad/x'])]:
            with self.subTest(key=key, value=value):
                bad = copy.deepcopy(valid); bad[key] = value
                self.refuses(proofs.validate_record, bad)
        for key in valid:
            bad = copy.deepcopy(valid); del bad[key]
            self.refuses(proofs.validate_record, bad)
        for issue in issue_shape_refusals():
            bad = record(); bad['issue'] = issue
            self.refuses(proofs.validate_record, bad)
        for number in (True, 1.0, 0, -1):
            bad = record(); bad['issue']['number'] = number
            self.refuses(proofs.validate_record, bad)
        for brief in ('docs/absent.md', f'https://github.com/Kernol12/subathon-bot-ai/blob/{ARCHIVE}/docs/absent.md'):
            good = record(); good['brief'] = brief; good['limitations'] = ['', 'x'*600, 'same', 'same']
            self.assertIsNone(proofs.validate_record(good))
        for brief in immutable_brief_refusals():
            bad = record(); bad['brief'] = brief
            self.refuses(proofs.validate_record, bad)
        base = f'https://github.com/Kernol12/subathon-bot-ai/blob/{ARCHIVE}/docs/'
        for suffix in ('a.md?x=1', 'a.md#x', '%2Fescape', '%252e%252e/a', '%61.md', '%5cescape'):
            bad = record(); bad['brief'] = base+suffix
            self.refuses(proofs.validate_record, bad)

    def test_enrollment_identity_and_exempt_records_remain_strict(self):
        one, two = record(), record('CONVERT-902', 9902)
        self.assertIsNone(proofs.validate(policy('CONVERT-901', 'CONVERT-902'), [two, one]))
        for records in ([], [one, two], [one, one]):
            self.refuses(proofs.validate, policy('CONVERT-901'), records)
        two['issue'] = copy.deepcopy(one['issue'])
        self.refuses(proofs.validate, policy('CONVERT-901', 'CONVERT-902'), [one, two])
        exempt = record('ACCESS-002')
        self.assertIsNone(proofs.validate(policy('ACCESS-002'), [exempt]))
        self.refuses(proofs.require_complete, bundle(exempt), 'ACCESS-002')
        exempt['version'] = False
        self.refuses(proofs.validate, policy('ACCESS-002'), [exempt])

    def test_read_json_strict_decoding(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary).resolve()/'input.json'
            path.write_bytes(b'{"nested":{"text":"Gr\xc3\xbc\xc3\x9fe"}}')
            self.assertEqual(proofs.read_json(path), {'nested': {'text': 'Grüße'}})
            for raw in (b'{"a":1,"a":2}', b'{"x":{"a":1,"\\u0061":2}}', b'{',
                        b'NaN', b'Infinity', b'-Infinity', b'"\xff"', b'\xef\xbb\xbf{}'):
                path.write_bytes(raw)
                self.refuses(proofs.read_json, path)

    def write_bundle(self, root, *records):
        directory = root/'records'; directory.mkdir()
        path = root/'policy.json'
        path.write_text(json.dumps(policy(*(r['task_id'] for r in records))))
        for r in records:
            (directory/(r['task_id']+'.json')).write_text(json.dumps(r))
        return path, directory

    def test_sorted_discovery_load_and_missing_inputs(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            one, two = record(), record('CONVERT-902', 9902)
            path, directory = self.write_bundle(root, two, one)
            self.assertEqual(proofs.discover_records(directory),
                             [directory/'CONVERT-901.json', directory/'CONVERT-902.json'])
            loaded = proofs.load(path, directory)
            self.assertEqual(loaded, dict(policy=policy('CONVERT-901', 'CONVERT-902'),
                                          records={'CONVERT-901': one, 'CONVERT-902': two}))
            self.assertEqual(list(loaded['records']), ['CONVERT-901', 'CONVERT-902'])
            (directory/'CONVERT-901.json').unlink()
            self.refuses(proofs.load, path, directory)
            path.unlink()
            self.refuses_input(proofs.load, path, directory)
        with tempfile.TemporaryDirectory() as temporary:
            path, directory = self.write_bundle(Path(temporary).resolve())
            self.assertEqual(proofs.load(path, directory), dict(policy=policy(), records={}))
            directory.rmdir()
            self.refuses_input(proofs.load, path, directory)  # empty enrollment != missing directory
            directory.write_text('regular file')
            self.refuses_input(proofs.load, path, directory)
            directory.unlink(); directory.mkdir()
            path.unlink(); path.mkdir()
            self.refuses_input(proofs.load, path, directory)

    def test_discovery_rejects_unexpected_children_filename_and_all_ancestor_symlinks(self):
        for kind in ('symlink', 'fifo', 'directory'):
            with self.subTest(canonical_child=kind), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary).resolve()
                path, directory = self.write_bundle(root, record())
                proofs.load(path, directory)
                canonical = directory/'CONVERT-901.json'; raw = canonical.read_bytes(); canonical.unlink()
                if kind == 'symlink':
                    outside = root/'identical.json'; outside.write_bytes(raw); canonical.symlink_to(outside)
                elif kind == 'fifo':
                    import os
                    os.mkfifo(canonical)
                else: canonical.mkdir()
                self.refuses(proofs.load, path, directory)
        for child in ('.hidden', 'notes.txt', 'CONVERT-901.JSON', 'convert-901.json', 'nested', 'link', 'fifo'):
            with self.subTest(child=child), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary).resolve()
                path, directory = self.write_bundle(root, record())
                proofs.load(path, directory)  # known passing baseline before one mutation
                p = directory/child
                if child == 'nested': p.mkdir()
                elif child == 'link': p.symlink_to(path)
                elif child == 'fifo':
                    import os
                    os.mkfifo(p)
                else: p.write_text('{}')
                self.refuses(proofs.load, path, directory)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            path, directory = self.write_bundle(root, record())
            proofs.load(path, directory)
            (directory/'CONVERT-901.json').write_text(json.dumps(record('CONVERT-902')))
            self.refuses(proofs.load, path, directory)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve(); real = root/'real'; real.mkdir()
            path, directory = self.write_bundle(real, record())
            proofs.load(path, directory)
            alias = root/'ancestor'; alias.symlink_to(real, target_is_directory=True)
            self.refuses(proofs.load, alias/'policy.json', directory)
            self.refuses(proofs.load, path, alias/'records')
            direct = real/'alias-policy.json'; direct.symlink_to(path)
            self.refuses(proofs.load, direct, directory)
            dir_alias = real/'alias-records'; dir_alias.symlink_to(directory, target_is_directory=True)
            self.refuses(proofs.load, path, dir_alias)

    def test_shared_review_engine_tree_free_and_strict_production_default(self):
        value = reviewed_record(); packet = value['workflow']['stages']['contract']
        self.assertIsNone(workflow.validate_stage(packet, ROOT, workflow.lexical_path,
                                                proofs.reference, tree_free=True))
        # Deliberately require delegation through the owning module attribute, so
        # the single grammar remains observable instead of a copied enforcement loop.
        for name in ('../x', '/abs', 'progress/state.json', 'PROGRESS.html',
                     'progress/coordinator-checkpoint.md', 'progress/proof-policy.json',
                     'progress/proofs/CONVERT-901.json'):
            bad = copy.deepcopy(value); bad['workflow']['stages']['contract']['paths'] = [name]
            self.refuses(workflow.validate_stage, bad['workflow']['stages']['contract'], ROOT,
                         workflow.lexical_path, proofs.reference, tree_free=True)
            self.refuses(proofs.validate_record, bad)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            for name in ('progress/proof-policy.json', 'progress/proofs/CONVERT-901.json'):
                file = root/name; file.parent.mkdir(parents=True, exist_ok=True); file.write_text('{}')
                bad = copy.deepcopy(packet); bad['paths'] = [name]
                self.refuses(workflow.validate_stage, bad, root, workflow.lexical_path, proofs.reference, tree_free=True)
        with patch.object(workflow, 'validate_stage', wraps=workflow.validate_stage) as shared:
            proofs.validate_record(value)
            self.assertTrue(shared.called, 'Use the owning workflow engine')
            self.assertTrue(all(c.kwargs.get('tree_free') is True for c in shared.call_args_list))

    def test_review_independence_order_outcomes_times_and_rejected_history(self):
        valid = reviewed_record()
        self.assertIsNone(proofs.validate_record(valid))
        for index, key, value in [(0, 'reviewer', 'codex:synthetic-author'), (0, 'provider', 'codex'),
                (0, 'phase', 'verification'), (1, 'phase', 'initial'),
                (1, 'blocking_findings', ['unlisted']), (1, 'outcome', 'changes_requested'),
                (1, 'model', ''), (1, 'findings', ['X', 'X']),
                (4, 'revision', 'a'*40), (0, 'started_at', '2026-11-13T12:00:00Z')]:
            bad = copy.deepcopy(valid); bad['workflow']['stages']['contract']['reviews'][index][key] = value
            self.refuses(proofs.validate_record, bad)
        bad = copy.deepcopy(valid)
        del bad['workflow']['stages']['contract']['reviews'][3]
        self.refuses(proofs.validate_record, bad)  # closing cannot follow rejected closing
        for start, finish in [('2026-11-13T12:00:00', '2026-11-13T12:01:00'),
                              ('2026-11-13T12:02:00Z', '2026-11-13T12:01:00Z')]:
            bad = copy.deepcopy(valid)
            bad['workflow']['stages']['contract']['reviews'][0].update(started_at=start, finished_at=finish)
            self.refuses(proofs.validate_record, bad)
        good = copy.deepcopy(valid)
        good['workflow']['stages']['contract']['reviews'][0].update(
            started_at='2026-11-13T13:00:00+01:00', finished_at='2026-11-13T12:00:10Z')
        self.assertIsNone(proofs.validate_record(good))
        self.assertEqual(proofs.context(bundle(good), 'CONVERT-901', stage='contract')['stage_context']['metrics']['elapsed_seconds'], 10)

    def test_completion_currency_pending_minima_and_extra_stages(self):
        good = reviewed_record()
        self.assertIsNone(proofs.require_complete(bundle(good), 'CONVERT-901'))
        pending = []
        no_workflow = record(); pending.append(no_workflow)
        unstarted = copy.deepcopy(good); unstarted['workflow']['stages'] = {}; pending.append(unstarted)
        rejected = copy.deepcopy(good); rejected['workflow']['stages']['contract']['reviews'] = rejected['workflow']['stages']['contract']['reviews'][:3]; pending.append(rejected)
        stale = copy.deepcopy(good)
        for r in stale['workflow']['stages']['contract']['reviews'][-2:]: r['revision'] = 'a'*40
        pending.append(stale)
        extra = copy.deepcopy(good); extra['workflow']['required_stages'].append('tests'); pending.append(extra)
        for value in pending:
            self.assertIsNone(proofs.validate_record(value))
            self.refuses(proofs.require_complete, bundle(value), 'CONVERT-901')
            if 'workflow' in value and value is not extra:
                self.assertFalse(proofs.context(bundle(value), 'CONVERT-901', stage='contract')['stage_context']['complete'])
        minima = {'contract': ['contract'], 'implementation': ['tests', 'implementation'],
                  'test_draft': ['tests'], 'test_review': ['tests'], 'test_baseline': ['tests'],
                  'code_review': ['implementation'], 'cleanup': ['implementation'],
                  'environment': ['implementation'], 'feasibility': ['implementation'], 'scaffold': ['implementation']}
        for kind, names in minima.items():
            value = record(); value['kind'] = kind
            value['workflow'] = dict(required_stages=names, stages={})
            self.assertIsNone(proofs.validate_record(value))
            bad = copy.deepcopy(value); bad['workflow']['required_stages'] = ['contract'] if kind != 'contract' else ['tests']
            self.refuses(proofs.validate_record, bad)
        bad = record(); bad['kind'] = 'implementation'
        bad['workflow'] = dict(required_stages=['implementation'], stages={})
        self.refuses(proofs.validate_record, bad)  # implementation alone omits required tests
        self.refuses(proofs.context, bundle(record()), 'CONVERT-901', stage='contract')
        bad = copy.deepcopy(good); bad['workflow']['stages']['tests'] = stage()
        self.refuses(proofs.validate_record, bad)

    def test_full_compact_context_exact_projection_and_nonaliasing(self):
        value = reviewed_record(); value['verification'] = {'baseline': 'not_run', 'ci': 'failed'}
        value['review'] = {'author': 'same', 'reviewer': 'same', 'outcome': 'approved'}
        data = bundle(value); before = copy.deepcopy(data)
        self.assertEqual(proofs.context(data, 'CONVERT-901'),
                         dict(proof=value, required_stages=['contract'], limitations=WARNING))
        compact = proofs.context(data, 'CONVERT-901', stage='contract', compact=True)
        proof = {k: v for k, v in value.items() if k != 'workflow'}
        packet = value['workflow']['stages']['contract']
        refs = [{k: r[k] for k in ('phase', 'provider', 'model', 'reviewer', 'revision', 'outcome', 'evidence')}
                for r in packet['reviews']]
        self.assertEqual(compact, dict(proof=proof, required_stages=['contract'], limitations=WARNING+OMISSIONS,
            stage_context=dict(name='contract', complete=True, next_gate='complete',
                packet={k: packet[k] for k in ('authors', 'revision', 'paths')}, review_refs=refs,
                historical_blocking_findings=['SYN-C1'], metrics=dict(review_count=5, elapsed_seconds=None, timed_reviews=0))))
        full = proofs.context(data, 'CONVERT-901', stage='contract')
        full['proof']['workflow']['stages']['contract']['reviews'].clear()
        full['stage_context']['packet']['reviews'].clear()
        full['stage_context']['metrics']['blocking_findings'].clear()
        self.assertEqual(data, before)
        compact['proof']['limitations'].append('projection edit')
        compact['stage_context']['packet']['authors'].append('projection edit')
        self.assertEqual(data, before)
        self.refuses(proofs.context, data, 'CONVERT-404')
        self.refuses(proofs.context, data, 'CONVERT-901', stage='tests')
        unrelated = record('CONVERT-902', 9902); unrelated['version'] = False
        data['policy'] = policy('CONVERT-901', 'CONVERT-902'); data['records']['CONVERT-902'] = unrelated
        self.refuses(proofs.context, data, 'CONVERT-901')
        self.refuses(proofs.require_complete, data, 'CONVERT-901')

    def test_publication_observations_never_replace_current_approvals(self):
        value = reviewed_record(); value['publication'] = publication()
        value['evidence'].append(value['publication']['pr'])
        self.assertIsNone(proofs.validate_record(value))
        for key, bad_value in [('merged_at', '2026-11-12T12:00:00'),
                               ('observed_at', '2026-11-10T12:00:00Z'), ('merge_commit', None)]:
            bad = copy.deepcopy(value); bad['publication'][key] = bad_value
            self.refuses(proofs.validate_record, bad)
        bad = copy.deepcopy(value); bad['evidence'].remove(value['publication']['pr'])
        self.refuses(proofs.validate_record, bad)
        value['workflow']['stages'] = {}
        self.assertIsNone(proofs.validate_record(value))
        self.refuses(proofs.require_complete, bundle(value), 'CONVERT-901')

    def test_frozen_supplemental_shape_and_no_canonical_completion_promotion(self):
        self.assertEqual(hashlib.sha256((ROOT/'tests/support/apitest016-supplemental-b506434.json').read_bytes()).hexdigest(),
                         'ae0160f99c453ff699189bd1a9c3a14e359495f2271187cfe6faab0e4e8b35cf')
        value = record('APITEST-016'); value['supplemental_workflow_evidence'] = supplemental()
        self.assertIsNone(proofs.validate_record(value))
        # Supplemental shape is explicit stdlib data, never ambient state/Git/SDK.
        import builtins
        original_import = builtins.__import__
        def stdlib_only(name, *args, **kwargs):
            self.assertFalse(name == 'jsonschema' or name.startswith('jsonschema.'))
            return original_import(name, *args, **kwargs)
        with patch('builtins.open', side_effect=AssertionError('unexpected file read')), \
             patch.object(Path, 'open', side_effect=AssertionError('unexpected path read')), \
             patch.object(subprocess, 'run', side_effect=AssertionError('unexpected subprocess')), \
             patch('builtins.__import__', side_effect=stdlib_only):
            self.assertIsNone(proofs.validate_record(value))
        self.refuses(proofs.require_complete, bundle(value), 'APITEST-016')
        for mutation in supplemental_mutations():
            bad = copy.deepcopy(value); bad['supplemental_workflow_evidence'] = mutation
            self.refuses(proofs.validate_record, bad)
        bad = copy.deepcopy(value); bad['task_id'] = 'CONVERT-901'
        self.refuses(proofs.validate_record, bad)


class ConversionTests(unittest.TestCase):
    refuses = ProofTests.refuses

    def test_bounded_conversion_enrollment_preservation_report_and_nonaliasing(self):
        quiet = task('CONVERT-901', status='planned')
        for key in ('workflow', 'publication'): quiet.pop(key, None)
        quiet.update(evidence=[], verification={'acceptance': 'not_run'}, review=None,
                     notes=['Only synthetic checks.'])
        data = source(quiet); identities = mapping(quiet)
        empty = proofs.convert(data, identities, ARCHIVE)
        self.assertEqual(empty['records'], [])
        self.assertEqual(empty['policy'], policy())
        quiet['evidence'] = ['https://example.test/receipt']
        quiet['notes'] = ['same', 'same', 'Grüße']
        pending = task('CONVERT-902', kind='implementation')
        pending['workflow']['stages'] = {}
        pending['verification'] = {'baseline': 'observed_red', 'github_ci': 'failed'}
        pending['notes'] = ['Baseline observed; implementation pending.']
        epic = task('EPIC-999', kind='epic', status='planned'); epic.pop('workflow')
        data = source(pending, epic, quiet); identities = mapping(pending, epic, quiet)
        before = copy.deepcopy((data, identities))
        result = proofs.convert(data, identities, ARCHIVE)
        expected_quiet = record()
        expected_quiet.update(issue=identities['entries'][2]['issue'], evidence=quiet['evidence'],
                              limitations=quiet['notes'], verification=quiet['verification'], review=None,
                              brief=quiet['brief'])
        expected_pending = dict(version=1, task_id='CONVERT-902', issue=identities['entries'][0]['issue'],
            kind='implementation', brief=pending['brief'], evidence=pending['evidence'],
            limitations=pending['notes'], workflow=pending['workflow'],
            verification=pending['verification'], review=pending['review'])
        self.assertEqual(set(result), {'policy', 'records', 'report'})
        self.assertEqual(result['records'], [expected_quiet, expected_pending])
        self.assertEqual(result['policy'], policy('CONVERT-901', 'CONVERT-902'))
        comparisons = [dict(task_id=t, **{key: True for key in (
            'kind', 'brief', 'evidence', 'limitations', 'workflow', 'publication',
            'verification', 'review', 'supplemental_workflow_evidence')})
            for t in ('CONVERT-901', 'CONVERT-902')]
        report = result['report']
        self.assertEqual(list(result['policy']), list(policy()))
        for actual, expected in zip(result['records'], [expected_quiet, expected_pending]):
            self.assertEqual(list(actual), list(expected))
        self.assertEqual(list(report), ['version', 'source_task_count', 'record_count', 'archive_only_ids',
            'enrolled_ids', 'mappings', 'field_comparisons', 'discrepancies'])
        self.assertEqual(set(report), {'version', 'source_task_count', 'record_count', 'archive_only_ids',
            'enrolled_ids', 'mappings', 'field_comparisons', 'discrepancies'})
        # Field-comparison row ordering is not normative; compare rows by identity.
        self.assertEqual(sorted(report['field_comparisons'], key=lambda r: r['task_id']),
                         sorted(comparisons, key=lambda r: r['task_id']))
        report_orderless = copy.deepcopy(report); report_orderless['field_comparisons'] = comparisons
        self.assertEqual(report_orderless, dict(version=1, source_task_count=3, record_count=2,
            archive_only_ids=['EPIC-999'], enrolled_ids=['CONVERT-901', 'CONVERT-902'],
            mappings=identities['entries'], field_comparisons=comparisons, discrepancies=[]))
        b = dict(policy=result['policy'], records={r['task_id']: r for r in result['records']})
        view = proofs.context(b, 'CONVERT-902', stage='tests')['stage_context']
        self.assertEqual((view['packet'], view['next_gate'], view['complete']), (None, 'authoring', False))
        self.refuses(proofs.require_complete, b, 'CONVERT-902')
        result['records'][0]['limitations'].append('new')
        result['records'][1]['workflow']['required_stages'].append('contract')
        result['report']['mappings'][0]['issue']['number'] = 17
        result['policy']['legacy_done'].clear()
        self.assertEqual((data, identities), before)
        # Returned record and report identity must not alias one another either.
        self.assertEqual(result['records'][1]['issue']['number'], 9901)

    def test_conversion_absent_notes_historical_fields_and_claim_refusals(self):
        value = task('CONVERT-901'); value.pop('notes')
        value['workflow']['stages']['contract'] = reviewed_record()['workflow']['stages']['contract']
        value['review'] = {}  # optional empty historical object must remain present
        data = source(value)
        self.assertEqual(proofs.convert(data, mapping(value), ARCHIVE)['records'][0]['limitations'], [])
        result = proofs.convert(data, mapping(value), ARCHIVE)['records'][0]
        self.assertEqual(result['review'], {})
        self.assertEqual(result['workflow'], value['workflow'])
        for key, change in [('workflow_policy', None), ('tasks', [])]:
            bad = copy.deepcopy(data); bad[key] = change
            self.refuses(proofs.convert, bad, mapping(value), ARCHIVE)
        bad = copy.deepcopy(data); del bad['workflow_policy']
        self.refuses(proofs.convert, bad, mapping(value), ARCHIVE)
        bad = copy.deepcopy(data); del bad['tasks'][0]['workflow']
        self.refuses(proofs.convert, bad, mapping(value), ARCHIVE)
        for identity in ({'version': 1, 'entries': []},
                         {'version': 1, 'entries': mapping(value)['entries']*2},
                         dict(mapping(value), unknown=True)):
            self.refuses(proofs.convert, data, identity, ARCHIVE)
        other = copy.deepcopy(value); other['id'] = 'CONVERT-902'
        pair = mapping(value, other); pair['entries'][1]['issue'] = copy.deepcopy(pair['entries'][0]['issue'])
        self.refuses(proofs.convert, source(value, other), pair, ARCHIVE)
        extra = mapping(value, other)
        self.refuses(proofs.convert, data, extra, ARCHIVE)
        self.refuses(proofs.convert, data, mapping(value), 'short')
        bad = copy.deepcopy(data); bad['tasks'][0]['notes'] = None
        self.refuses(proofs.convert, bad, mapping(value), ARCHIVE)

    def test_enrollment_is_evidence_based_and_never_status_age_exemption(self):
        quiet = task('CONVERT-901', status='planned')
        quiet.pop('workflow'); quiet.update(evidence=[], verification={'a': 'draft'}, review=None)
        baseline = source(quiet)
        self.assertEqual(proofs.convert(baseline, mapping(quiet), ARCHIVE)['records'], [])
        for key, value in [('review', {}), ('verification', {'a': 'failed'}),
                           ('evidence', ['https://example.test/evidence']),
                           ('workflow', {'required_stages': ['contract'], 'stages': {}})]:
            data = copy.deepcopy(baseline); data['tasks'][0][key] = value
            self.assertEqual(len(proofs.convert(data, mapping(quiet), ARCHIVE)['records']), 1)
        for status in ('blocked', 'ready'):
            data = copy.deepcopy(baseline); data['tasks'][0]['status'] = status
            self.assertEqual(proofs.convert(data, mapping(quiet), ARCHIVE)['records'], [])
        for status in ('in_progress', 'in_review', 'done'):
            data = copy.deepcopy(baseline); data['tasks'][0]['status'] = status
            self.refuses(proofs.convert, data, mapping(quiet), ARCHIVE)
        exempt = copy.deepcopy(quiet); exempt['id'] = 'ACCESS-002'; exempt['status'] = 'planned'
        self.assertEqual(proofs.convert(source(exempt), mapping(exempt), ARCHIVE)['records'], [])
        exempt['evidence'] = ['https://example.test/historical']
        self.assertEqual(len(proofs.convert(source(exempt), mapping(exempt), ARCHIVE)['records']), 1)


    def test_epic_claims_do_not_invent_workflow_or_publication_gates(self):
        # The pinned tracker predicate excludes epics; supplied grammar still applies.
        for status in ('planned', 'in_progress', 'in_review', 'done'):
            epic = task('EPIC-999', kind='epic', status=status)
            epic.pop('workflow')
            original = copy.deepcopy(epic)
            with self.subTest(epic_status=status):
                result = proofs.convert(source(epic), mapping(epic), ARCHIVE)
                self.assertEqual(result['records'], [])
                self.assertEqual(result['policy']['required_records'], [])
                self.assertEqual(result['report']['archive_only_ids'], ['EPIC-999'])
                self.assertEqual(epic, original)
                malformed = copy.deepcopy(epic)
                malformed['workflow'] = {'unknown': True}
                self.refuses(proofs.convert, source(malformed), mapping(malformed), ARCHIVE)


class ProofCliTests(unittest.TestCase):
    def invoke(self, *args):
        return subprocess.run([sys.executable, '-B', str(ROOT/'tools/proofs.py'), *map(str, args)],
                              capture_output=True, text=True, timeout=60)

    def assert_error(self, result, code=1):
        self.assertEqual(result.returncode, code, result.stderr)
        self.assertEqual(result.stdout, '')
        self.assertTrue(result.stderr)
        if code == 1:
            self.assertTrue(result.stderr.startswith('Proof error: '))
            self.assertNotIn(NI, result.stderr)
            self.assertNotIn('CUTOVER_', result.stderr, 'NI is not an input-safety refusal')

    def test_default_validate_disposable_bundle_and_input_refusals(self):
        from context_fixture import invoke_context
        value = record()
        data = bundle(value)
        expected = '{\n  "version": 1,\n  "record_count": 1\n}\n'
        def absent_policy(root):
            (root/'progress/proof-policy.json').unlink()
        def absent_directory(root):
            directory=root/'progress/proofs'
            for path in directory.iterdir(): path.unlink()
            directory.rmdir()
        def absent_record(root):
            (root/'progress/proofs/CONVERT-901.json').unlink()
        def malformed_record(root):
            (root/'progress/proofs/CONVERT-901.json').write_text('{',encoding='utf-8')
        for mutation in (absent_policy,absent_directory,absent_record,malformed_record):
            with self.subTest(mutation=mutation.__name__):
                good=invoke_context(self,data,['validate'])
                self.assertEqual(good[:3],(0,expected,''),good[2])
                bad=invoke_context(self,data,['validate'],prepare=mutation)
                self.assertEqual(bad[:2],(1,''))
                self.assertTrue(bad[2].startswith('Proof error: '),bad[2])
                self.assertNotIn('CUTOVER_',bad[2])
                self.assertNotIn(NI,bad[2])

    def test_actual_canonical_bundle_is_valid(self):
        """Ordinary CI duty; pending setup until the real cutover candidate exists.

        Never treat absent production inputs as intended red or use an archive
        fallback. This remains in ordinary tooling discovery without a skip.
        """
        policy_path = ROOT/'progress/proof-policy.json'
        directory = ROOT/'progress/proofs'
        declared = json.loads(policy_path.read_text(encoding='utf-8'))
        inventory = sorted(p.stem for p in directory.iterdir() if p.suffix == '.json')
        self.assertEqual(inventory, sorted(declared['required_records']))
        result = self.invoke('validate')
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, json.dumps(dict(version=1, record_count=len(inventory)), indent=2)+'\n', ''))


    def test_one_disposable_full_source_cli_conversion_synthetic_mapping_only(self):
        from proof_publication_fixture import ProofPublicationFixture, POLICY, RECORDS, REPORT, SELECTED
        from publication_fixture import ENV
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            f = ProofPublicationFixture(root/'candidate')
            output = root/'converted'
            def invoke(pin=None, destination=output, *extra, joined_pin=False):
                before = f.snapshot()
                selected = f.archive if pin is None else pin
                pin_args = ['--archive-commit='+selected] if joined_pin else ['--archive-commit', selected]
                result = subprocess.run([sys.executable, '-B', 'tools/proofs.py', 'convert',
                    *pin_args,
                    '--output-dir', str(destination), *extra], cwd=f.root, env=ENV,
                    capture_output=True, text=True, timeout=60)
                self.assertEqual(f.snapshot(), before)
                return result
            result = invoke()
            self.assertEqual((result.returncode, result.stdout, result.stderr),
                             (0, '{\n  "version": 1,\n  "record_count": 3\n}\n', ''),
                             'CUTOVER_ARCHIVE_NOT_IMPLEMENTED is NI; unavailable objects are setup')
            actual_policy=json.loads((output/'proof-policy.json').read_text())
            self.assertEqual(actual_policy, f.policy)
            self.assertEqual(list(actual_policy),list(f.policy))
            for identity,expected in f.records.items():
                value=json.loads((output/'proofs'/(identity+'.json')).read_text())
                self.assertEqual(value,expected)
                self.assertEqual(list(value),list(expected))
            report=json.loads((output/'report.json').read_text())
            self.assertEqual(report,f.report)
            self.assertEqual(list(report),['version','source_task_count','record_count','archive_only_ids',
                'enrolled_ids','mappings','field_comparisons','discrepancies'])
            fields=('kind','brief','evidence','limitations','workflow','publication','verification','review','supplemental_workflow_evidence')
            self.assertEqual([row['task_id'] for row in report['field_comparisons']],[SELECTED,'OLD-902','ACCESS-002'])
            for row in report['field_comparisons']:
                self.assertEqual(list(row),['task_id',*fields])
                self.assertTrue(all(type(row[k]) is bool and row[k] for k in fields))
            self.assertEqual(len({row['task_id'] for row in report['mappings']}),4)
            self.assertEqual({p.stem:json.loads(p.read_text()) for p in (output/'proofs').iterdir()}, f.records)
            self.assertEqual({p.relative_to(output).as_posix() for p in output.rglob('*') if p.is_file()},
                             {'proof-policy.json','report.json'} | {'proofs/'+i+'.json' for i in f.records})
            self.assertEqual(output.stat().st_mode & 0o777, 0o700)
            for path in output.rglob('*.json'):
                self.assertEqual(path.stat().st_mode & 0o777, 0o600)
                self.assertEqual(path.read_text(), json.dumps(json.loads(path.read_text()), indent=2, ensure_ascii=False)+'\n')
            for path in output.rglob('*.json'):
                self.assertNotIn(str(root),path.read_text())
                self.assertNotIn(str(ROOT),path.read_text())
            existing=root/'existing'; existing.mkdir()
            self.assert_error(invoke(destination=existing))
            self.assertEqual(list(existing.iterdir()),[])
            self.assert_error(invoke(destination=output/'proof-policy.json'/'overlap'))
            saved = {p.relative_to(output):p.read_bytes() for p in output.rglob('*') if p.is_file()}
            self.assert_error(invoke())
            self.assertEqual({p.relative_to(output):p.read_bytes() for p in output.rglob('*') if p.is_file()}, saved)
            for pin in (f.archive[:7], 'f'*40, 'HEAD'):
                self.assert_error(invoke(pin, root/'refused'))
                self.assertFalse((root/'refused').exists())
            self.assert_error(invoke('--all', root/'refused'), 2)
            self.assertFalse((root/'refused').exists())
            self.assert_error(invoke('--all', root/'refused', joined_pin=True))
            self.assertFalse((root/'refused').exists())
            self.assert_error(invoke(destination=f.root/'inside'))
            self.assertFalse((f.root/'inside').exists())
            alias = root/'alias'; alias.symlink_to(root, target_is_directory=True)
            self.assert_error(invoke(destination=alias/'refused'))
            self.assertFalse((root/'refused').exists())
            # Current working bytes cannot replace the pinned Git transport.
            f.write('progress/state.json', b'not archive JSON\n')
            again = root/'again'
            result = invoke(destination=again)
            self.assertEqual((result.returncode, result.stderr), (0, ''))
            self.assertEqual(json.loads((again/'report.json').read_text()), f.report)
            # Each invalid archive starts from the exact independently authored
            # source/report, after the successful complete-extraction control.
            # ADR-0012 scope retires baseline 749–755 live brief-existence and
            # 756–761 full live-inventory admission. Full archive admission and
            # protected readback own those duties; this adapter reads three blobs.
            for mutation in ('report','identity','unknown-source-field','nonregular-report','nonregular-state','nonregular-identities'):
                def edit(archive_fixture):
                    if mutation == 'report':
                        changed = copy.deepcopy(f.report); changed['record_count'] = 2
                        archive_fixture.json(REPORT, changed)
                    elif mutation == 'identity':
                        changed = copy.deepcopy(f.identities)
                        changed['entries'][0]['issue']['number'] = 12345
                        archive_fixture.json('docs/migrations/EPIC-029/archive/identities.json', changed)
                    elif mutation == 'unknown-source-field':
                        changed = copy.deepcopy(f.source); changed['unknown'] = 'cannot infer a new source version'
                        archive_fixture.json('progress/state.json', changed)
                    elif mutation.startswith('nonregular-'):
                        name={'nonregular-report':REPORT,'nonregular-state':'progress/state.json',
                              'nonregular-identities':'docs/migrations/EPIC-029/archive/identities.json'}[mutation]
                        path=archive_fixture.root/name; path.unlink(); path.symlink_to('absent-synthetic-target')
                with self.subTest(archive_mutation=mutation):
                    f.prepare_archive(edit)
                    refused = root/('refused-'+mutation)
                    self.assert_error(invoke(destination=refused))
                    self.assertFalse(refused.exists())

    def test_explicit_inputs_usage_and_read_only_json_serialization(self):
        for args in ([], ['convert', '--source', 'x'],
                     ['validate', '--unknown']):
            self.assert_error(self.invoke(*args), 2)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            value = record(); value['limitations'] = ['Grüße']
            path, directory = ProofTests.write_bundle(self, root, value)
            before_entries = set(root.rglob('*'))
            before = {p: p.read_bytes() for p in before_entries if p.is_file()}
            args = ['--policy', path, '--records-dir', directory]
            valid = self.invoke('validate', *args)
            self.assertEqual((valid.returncode, valid.stdout, valid.stderr),
                             (0, '{\n  "version": 1,\n  "record_count": 1\n}\n', ''))
            context = self.invoke('context', 'CONVERT-901', *args, '--compact')
            expected = dict(proof=value, required_stages=[], limitations=WARNING+OMISSIONS)
            self.assertEqual((context.returncode, context.stdout, context.stderr),
                             (0, json.dumps(expected, indent=2, ensure_ascii=False)+'\n', ''))
            self.assert_error(self.invoke('context', 'CONVERT-404', *args))
            self.assertEqual(set(root.rglob('*')), before_entries)
            self.assertEqual(before, {p: p.read_bytes() for p in root.rglob('*') if p.is_file()})



if __name__ == '__main__':
    unittest.main()
