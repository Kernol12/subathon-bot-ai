# SPDX-License-Identifier: GPL-3.0-or-later
"""MIGRATE-003 authored oracles. Independent closing MUST precede execution.

Real synthetic Git; host children alone are substituted. SDK integration is separate.
Each safety mutation starts from a passing control and excludes named NI refusals.
"""
from contextlib import contextmanager, redirect_stdout, redirect_stderr
from copy import deepcopy
from io import StringIO
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
sys.path.insert(0, str(ROOT/'tests/support'))
import check_publication as publication
import proofs
from proof_publication_fixture import (ProofPublicationFixture, POLICY, RECORDS, IDENTITIES,
                                       REPORT, SELECTED, CONTRACT, ORACLE, RUNTIME, BRIEF,
                                       packet, receipt, observation)

NI = 'PROOF_PUBLICATION_NOT_IMPLEMENTED'
WITNESS = b'{"tests":1,"failures":0,"errors":0,"skipped":0}\n'
LIMITATIONS = [
    'Recorded bindings are not authenticated approvals.',
    'Forbidden-path checks are not complete secret scanning.',
    'Current-head hosted, repository and owner gates are separate.',
    'Filesystem checks can observe ignored local files.',
    'Remote target-tip freshness and archive availability require coordinator readback.',
    'Frozen exemptions preserve history and do not authorize forward completion.',
    'Acceptance execution is a coordinator gate; proof completion does not validate result values.',
    'Per-task specification references are archived; active brief references require coordinator review.',
    'Intended-red execution is a coordinator gate; proof completion does not validate baseline execution.',
    'Concise completion notes are a coordinator rule; proof limitations have no aggregate character cap.',
]


class PreparedPublicationTests(unittest.TestCase):
    @contextmanager
    def fixture(self, **kwargs):
        with tempfile.TemporaryDirectory() as directory:
            yield ProofPublicationFixture(Path(directory)/'candidate', **kwargs)

    def invoke(self, f, arguments=None, responses=None):
        real = publication.run_step
        children = []
        def step(argv, *, git=False):
            if git:
                return real(argv, git=True)
            index = len(children)
            children.append(argv)
            value = (responses or {}).get(index, (0, WITNESS if index == 2 else b'', b''))
            return subprocess.CompletedProcess(argv, *value)
        before = f.snapshot()
        out, err = StringIO(), StringIO()
        with patch.object(publication, 'ROOT', f.root), patch.object(publication, 'run_step', side_effect=step), \
             redirect_stdout(out), redirect_stderr(err):
            try:
                code = publication.main(f.arguments() if arguments is None else arguments)
            except SystemExit as error:
                code = error.code
        self.assertEqual(f.snapshot(), before)
        return code, out.getvalue(), err.getvalue(), children

    def success(self, result):
        self.assertEqual(result[0], 0, result[2])
        self.assertEqual(result[2], '')
        return json.loads(result[1])

    def refusal(self, result):
        self.assertEqual(result[:2], (1, ''), result[2])
        self.assertIn('Publication error:', result[2])
        self.assertNotIn(NI, result[2], 'NI is not a safety refusal')
        self.assertNotIn('CUTOVER_', result[2], 'Cutover NI is not a safety refusal')

    def guard(self, f, *, bootstrap=True, task_id=SELECTED, base=None):
        before = f.snapshot()
        try:
            with patch.object(publication, 'ROOT', f.root):
                return publication.check_proof_policy(base or f.target, f.head, task_id=task_id, bootstrap=bootstrap)
        finally:
            self.assertEqual(f.snapshot(), before)

    def guard_refusal(self, f, **kwargs):
        with self.assertRaises(ValueError) as error:
            self.guard(f, **kwargs)
        self.assertNotIn(NI, str(error.exception))

    def bindings(self, f, stages=None, complete=False, bundle=None):
        with patch.object(publication, 'ROOT', f.root):
            return publication.check_proof_bindings(bundle or dict(policy=f.policy, records=f.records),
                SELECTED, stages or ['contract'], head=f.head, complete=complete)

    def test_unselected_new_id_cannot_borrow_old_approval_in_bootstrap_or_normal_mode(self):
        for bootstrap in (True,False):
            for complete in (False,True):
                with self.subTest(bootstrap=bootstrap,complete=complete), self.fixture() as f:
                    if not bootstrap: f.existing()
                    args = f.arguments(bootstrap=bootstrap,complete=complete,
                                       stages=('contract','tests','implementation'))
                    self.success(self.invoke(f,args))
                    brief='docs/unselected-new.md'
                    f.write(brief,b'Task ID: NEW-903\nWork type: contract\nIssue: https://github.com/Kernol12/subathon-bot-ai/issues/9905\n')
                    revision=f.commit('Synthetic unselected new brief')
                    f.records['NEW-903']=dict(version=1,task_id='NEW-903',kind='contract',brief=brief,
                        issue=dict(repository='Kernol12/subathon-bot-ai',number=9905),evidence=[],limitations=[],
                        workflow=dict(required_stages=['contract'],stages={'contract':packet(revision,[brief],complete=False)}))
                    f.policy['required_records']=sorted(f.records); f.persist()
                    self.refusal(self.invoke(f,args))
                    # The same identity is admissible when its own selected packet
                    # has complete reviews and binds the exact whole-line stanzas.
                    f.records['NEW-903']['workflow']['stages']['contract']=packet(revision,[brief])
                    f.persist()
                    self.success(self.invoke(f,f.arguments(bootstrap=bootstrap,task_id='NEW-903',complete=complete)))

    def test_bootstrap_newer_target_brief_cannot_revert_to_archive_and_equal_bytes_pass(self):
        for changed in (False,True):
            with self.subTest(changed=changed), self.fixture() as f:
                f.git('reset','--hard',f.archive)
                if changed: f.write(BRIEF,b'Newer reviewed target acceptance B\n')
                f.target=f.commit('Synthetic target before first proof introduction')
                f.git('update-ref','refs/remotes/origin/main',f.target)
                f.persist()
                self.guard(f)
                f.selected['brief']=f'https://github.com/Kernol12/subathon-bot-ai/blob/{f.archive}/{BRIEF}'
                f.persist()
                if changed: self.guard_refusal(f)
                else: self.guard(f)

    def test_published_archive_url_survives_later_local_brief_removal(self):
        with self.fixture() as f:
            self.guard(f)
            f.selected['brief']=f'https://github.com/Kernol12/subathon-bot-ai/blob/{f.archive}/{BRIEF}'
            f.persist(); self.guard(f)
            f.existing()
            (f.root/BRIEF).unlink(); f.persist()
            self.guard(f,bootstrap=False)

    def test_bootstrap_summary_exact_and_pending_selected_source(self):
        with self.fixture() as f:
            # This source had reviews at archive: use a new revision, preserving history.
            # A pending example is derived by advancing the revision, never deleting reviews.
            f.selected['workflow']['stages']['contract'] = deepcopy(f.source['tasks'][0]['workflow']['stages']['contract'])
            f.selected['workflow']['stages']['contract']['revision'] = f.head
            f.persist()
            result = self.invoke(f)
            summary = self.success(result)
            expected = dict(task=SELECTED, base=f.archive, head=f.head,
                commit_count=int(f.git('rev-list', '--count', f.archive+'..'+f.head).stdout),
                stages=[dict(name='contract', revision=f.selected['workflow']['stages']['contract']['revision'],
                             path_count=1, complete=False)],
                guard=dict(mode='bootstrap', record_count=3, archive_record_count=3, legacy_exempt=False),
                checks={k:'passed' for k in ('recorded_paths', 'outgoing_paths', 'proofs', 'policy_history',
                                            'project', 'proof_schema')},
                limitations=LIMITATIONS)
            self.assertEqual(result[1], json.dumps(expected, indent=2, ensure_ascii=False)+'\n')
            self.assertEqual(len(result[3]), 3)
            self.assertEqual(result[3][1][5:], [], 'Default repository check uses no tracker flag')
            for child in result[3][2:]:
                self.assertEqual(child[:5], ['/fixture/schema-python', '-I', '-S', '-B', '-c'])
            self.assertNotIn('baseline_supersessions', summary)
            self.assertEqual([Path(c[4]).name for c in result[3][:2]],['proofs.py','check_project.py'])
            self.assertEqual(result[3][0][5:],['validate'])
            self.assertEqual(result[3][2][6],'tests/schema_validation/test_proof_schemas.py')

    def test_complete_all_required_and_prospective_ni_replacement(self):
        with self.fixture() as f:
            summary=self.success(self.invoke(f, f.arguments(stages=('tests', 'contract', 'implementation'), complete=True)))
            self.assertEqual([stage['name'] for stage in summary['stages']],
                             ['tests','contract','implementation'])
            tests = deepcopy(f.selected['workflow']['stages']['tests'])
            ni_mode_blob = f.git('ls-tree', f.source_revision, '--', RUNTIME).stdout
            f.write(RUNTIME, b'Implemented synthetic source\n')
            revision = f.commit('Synthetic runtime replacement only')
            old = f.selected['workflow']['stages']['implementation']
            old['revision'] = revision
            old['reviews'] += [receipt('verification', revision), receipt('closing', revision)]
            f.persist()
            summary = self.success(self.invoke(f, f.arguments(stages=('contract','tests','implementation'), complete=True)))
            self.assertTrue(all(s['complete'] for s in summary['stages']))
            self.assertEqual(f.selected['workflow']['stages']['tests'], tests)
            self.assertEqual(f.git('ls-tree', f.source_revision, '--', RUNTIME).stdout, ni_mode_blob)
            self.assertNotEqual(f.git('ls-tree', f.head, '--', RUNTIME).stdout, ni_mode_blob)
            # Overlap in implementation cannot rescue the tests packet's changed oracle.
            f.write(ORACLE, b'Changed oracle\n')
            changed = f.commit('Synthetic stale test oracle')
            old['paths'].append(ORACLE); old['revision'] = changed
            old['reviews'] += [receipt('verification', changed), receipt('closing', changed)]
            f.persist()
            self.refusal(self.invoke(f, f.arguments(stages=('contract','tests','implementation'), complete=True)))

    def test_selected_binding_refusals_each_with_passing_control(self):
        for mutation in ('content', 'mode', 'missing', 'symlink', 'revision', 'stage', 'closing', 'subset'):
            with self.subTest(mutation=mutation), self.fixture() as f:
                args = f.arguments(stages=('contract','tests','implementation'), complete=True)
                self.success(self.invoke(f, args))
                stage = f.selected['workflow']['stages']['contract']
                if mutation == 'content': f.write(CONTRACT, b'Changed\n')
                elif mutation == 'mode': (f.root/CONTRACT).chmod(0o755)
                elif mutation == 'missing': (f.root/CONTRACT).unlink()
                elif mutation == 'symlink':
                    (f.root/CONTRACT).unlink(); (f.root/CONTRACT).symlink_to('synthetic-publication-brief.md')
                elif mutation == 'revision': stage['revision'] = 'f'*40
                elif mutation == 'stage': del f.selected['workflow']['stages']['tests']
                elif mutation == 'closing': f.selected['workflow']['stages']['tests']['reviews'].pop()
                elif mutation == 'subset': args = f.arguments(complete=True)
                f.persist()
                self.refusal(self.invoke(f, args))

    def test_metadata_and_unrelated_source_leave_current_oracles_bound(self):
        with self.fixture() as f:
            self.assertEqual(self.bindings(f)[0]['complete'], True)
            f.selected['evidence'].append('https://example.test/new')
            f.selected['limitations'].append('Later limitation')
            f.write('unrelated-source.txt', b'Unrelated\n'); f.persist()
            self.assertEqual(self.bindings(f), [dict(name='contract', revision=f.source_revision, path_count=1, complete=True)])
            self.guard(f)

    def test_history_single_mutations_against_base_and_archive_origin(self):
        # Same edits are rejected both at first bootstrap and against an existing base.
        mutations = ('enrollment','record','kind','issue','pin','exempt','required','workflow','packet',
                     'reviews','limitations','evidence','drop-review','drop-limitation','drop-evidence',
                     'verification','review','paths','authors','brief')
        for existing in (False, True):
            with self.fixture() as f:
                if existing: f.existing()
                self.guard(f, bootstrap=not existing)
                original_records, original_policy = deepcopy(f.records), deepcopy(f.policy)
                for mutation in mutations:
                    with self.subTest(existing=existing, mutation=mutation):
                        f.records, f.policy = deepcopy(original_records), deepcopy(original_policy)
                        r=f.selected; w=r['workflow']; p=w['stages']['contract']
                        if mutation=='enrollment':
                            del f.records['OLD-902']; f.policy['required_records'].remove('OLD-902')
                        elif mutation=='record': del f.records['OLD-902']
                        elif mutation=='kind': r['kind']='cleanup'
                        elif mutation=='issue': r['issue']['number']=9999
                        elif mutation=='pin': f.policy['archive_commit']=f.source_revision
                        elif mutation=='exempt': f.policy['legacy_done'].reverse()
                        elif mutation=='required': w['required_stages'].remove('contract'); del w['stages']['contract']
                        elif mutation=='workflow': del r['workflow']
                        elif mutation=='packet': del w['stages']['contract']
                        elif mutation=='reviews': p['reviews'][0]['evidence']='https://example.test/rewritten'
                        elif mutation=='drop-review': p['reviews'].pop()
                        elif mutation=='drop-limitation': r['limitations'].pop()
                        elif mutation=='drop-evidence': r['evidence'].pop()
                        elif mutation=='limitations': r['limitations'].reverse()
                        elif mutation=='evidence': r['evidence'].reverse()
                        elif mutation=='verification': r['verification']['baseline']='passed'
                        elif mutation=='review': del r['review']
                        elif mutation=='paths': p['paths']=[BRIEF]
                        elif mutation=='authors': p['authors'].append('new-author')
                        elif mutation=='brief': r['brief']=CONTRACT
                        f.persist(); self.guard_refusal(f, bootstrap=not existing)

    def test_revision_advance_is_pending_until_fresh_verification_closing(self):
        with self.fixture() as f:
            self.guard(f); f.existing()
            p=f.selected['workflow']['stages']['contract']
            p['revision']=f.head; p['paths']=[CONTRACT, BRIEF]; p['authors'].append('new-author')
            f.persist(); self.guard(f, bootstrap=False)
            self.assertFalse(self.bindings(f)[0]['complete'])
            self.refusal(self.invoke(f, f.arguments(bootstrap=False, stages=('contract','tests','implementation'), complete=True)))
            p['reviews'] += [receipt('verification', p['revision']), receipt('closing', p['revision'])]
            f.persist()
            self.success(self.invoke(f, f.arguments(bootstrap=False, stages=('contract','tests','implementation'), complete=True)))

    def test_historical_retired_packet_and_supplied_exemption_are_tree_free(self):
        with self.fixture() as f:
            self.guard(f)
            self.assertFalse((f.root/'docs/retired-publication.md').exists())
            f.write('docs/retired-publication.md',b'Historical acceptance\n')
            f.head=f.commit('Synthetic restore historical current binding')
            self.success(self.invoke(f, f.arguments(task_id='OLD-902')))
            (f.root/'docs/retired-publication.md').unlink()
            f.head=f.commit('Synthetic retire only historical bound path')
            self.refusal(self.invoke(f, f.arguments(task_id='OLD-902')))
            value=self.guard(f, task_id='ACCESS-002')
            self.assertTrue(value['legacy_exempt'])
            f.existing()
            self.assertEqual(self.guard(f,bootstrap=False,task_id='ACCESS-002'),
                             dict(mode='existing',record_count=3,archive_record_count=3,legacy_exempt=True))
            # Supplied exempt workflow validates but cannot authorize forward completion.
            f.records['ACCESS-002']['workflow']=dict(required_stages=['contract'], stages={
                'contract':packet(f.source_revision,[CONTRACT])})
            f.persist(); self.guard(f, bootstrap=False, task_id='ACCESS-002')
            self.refusal(self.invoke(f, f.arguments(bootstrap=False, task_id='ACCESS-002', complete=True)))
            f.records['ACCESS-002']['kind']='epic'; f.persist()
            self.guard_refusal(f, bootstrap=False, task_id='ACCESS-002')

    def test_same_pr_head_advance_time_and_merged_immutability(self):
        with self.fixture(publication=True) as f:
            self.guard(f); f.existing()
            first=deepcopy(f.selected['publication'])
            for state in ('open','merged'):
                at='2026-11-12T13:00:00Z' if state=='open' else '2026-11-12T14:00:00Z'
                f.selected['publication']=observation(f.head,state,at=at,
                                                       merged_at='2026-11-12T13:30:00Z')
                f.persist(); self.guard(f,bootstrap=False)
            f.existing()
            self.guard(f,bootstrap=False)
            merged=deepcopy(f.selected['publication'])
            for mutation in ('time','head','pr','remove'):
                f.selected['publication']=deepcopy(merged)
                if mutation=='remove': del f.selected['publication']
                elif mutation=='time': f.selected['publication']['observed_at']='2026-11-12T15:00:00Z'
                elif mutation=='head': f.selected['publication']['head']=f.archive
                else:
                    new='https://github.com/Kernol12/subathon-bot-ai/pull/9999'
                    f.selected['publication']['pr']=new; f.selected['evidence'].append(new)
                f.persist(); self.guard_refusal(f,bootstrap=False)
        for mutation in ('backwards','remove','reassociate'):
            with self.subTest(mutation=mutation), self.fixture(publication=True) as f:
                self.guard(f); f.existing()
                if mutation=='remove': del f.selected['publication']
                elif mutation=='backwards': f.selected['publication']['observed_at']='2026-11-12T11:59:00Z'
                else:
                    new='https://github.com/Kernol12/subathon-bot-ai/pull/9999'
                    f.selected['publication']['pr']=new; f.selected['evidence'].append(new)
                f.persist(); self.guard_refusal(f,bootstrap=False)

    def test_base_canonical_bootstrap_once_and_rollback(self):
        with self.fixture() as f:
            self.success(self.invoke(f))
            self.refusal(self.invoke(f, f.arguments(bootstrap=False)))
            self.refusal(self.invoke(f, f.arguments(base=f.source_revision)))
            f.git('update-ref','-d','refs/remotes/origin/main')
            self.refusal(self.invoke(f))
            f.git('update-ref','refs/remotes/origin/main',f.target)
            f.existing()
            result=self.invoke(f,f.arguments(bootstrap=False))
            summary=self.success(result)
            expected_guard=dict(mode='existing',record_count=3,archive_record_count=3,legacy_exempt=False)
            self.assertEqual(self.guard(f,bootstrap=False),expected_guard)
            expected=dict(task=SELECTED,base=f.target,head=f.head,commit_count=0,
                stages=[dict(name='contract',revision=f.source_revision,path_count=1,complete=True)],
                guard=expected_guard,checks={k:'passed' for k in ('recorded_paths','outgoing_paths','proofs',
                'policy_history','project','proof_schema')},limitations=LIMITATIONS)
            self.assertEqual(summary,expected)
            self.assertEqual(result[1],json.dumps(expected,indent=2,ensure_ascii=False)+'\n')
            self.refusal(self.invoke(f))
            for extra in (['--policy','alternate.json'], ['--records-dir','other']):
                result=self.invoke(f,f.arguments()+extra)
                self.assertEqual(result[:2],(2,''))
            # Delete both canonical locations in target history; eligibility is not reset.
            (f.root/POLICY).unlink(); shutil.rmtree(f.root/RECORDS)
            f.head=f.commit('Synthetic rollback of canonical inputs'); f.existing()
            f.persist(); self.refusal(self.invoke(f))

    def test_one_sided_base_and_unavailable_archive_never_bootstrap(self):
        for location in (POLICY, RECORDS):
            with self.subTest(location=location), self.fixture() as f:
                self.guard(f)
                f.git('reset','--hard',f.archive)
                if location==POLICY:
                    f.json(POLICY,f.policy)
                else:
                    for identity,record in f.records.items():
                        f.json(f'{RECORDS}/{identity}.json',record)
                f.head=f.commit('Synthetic first introduction of only one canonical input')
                f.existing()
                f.persist()
                self.guard_refusal(f)
                self.guard_refusal(f,bootstrap=False)
        with self.fixture() as f:
            self.guard(f)
            f.policy['archive_commit']='e'*40; f.persist(); self.guard_refusal(f)

    def test_canonical_discovery_and_strict_git_bytes(self):
        for name, content, symlink in (
                (POLICY,b'{"version":1,"version":1}',False),
                (f'{RECORDS}/{SELECTED}.json','bom',False),
                (f'{RECORDS}/.hidden',b'{}',False),
                (f'{RECORDS}/nested/OTHER-901.json',b'{}',False),
                (f'{RECORDS}/ACCESS-002.json','valid-record',True),
                (f'{RECORDS}/OLD-902.json','filename-mismatch',False)):
            with self.subTest(path=name), self.fixture() as f:
                self.guard(f)
                path=f.root/name
                if content=='bom': content=b'\xef\xbb\xbf'+path.read_bytes()
                elif content=='valid-record': content=path.read_bytes()
                elif content=='filename-mismatch': content=(f.root/f'{RECORDS}/{SELECTED}.json').read_bytes()
                if symlink:
                    path.unlink(); path.symlink_to(content.decode())
                else: f.write(name,content)
                f.head=f.commit('Synthetic single malformed Git input')
                self.guard_refusal(f)
        with self.fixture() as f:
            self.guard(f); f.existing(); self.guard(f,bootstrap=False)
            f.write(POLICY,b'{"version":1,"version":1}')
            f.head=f.commit('Synthetic malformed base-only transport'); f.existing()
            f.persist()  # Candidate is valid; only the target base has invalid bytes.
            self.guard_refusal(f,bootstrap=False)

    def test_archive_report_and_full_extraction_not_just_hashes(self):
        for mutation in (None,'report-count','report-mapping','report-comparison','report-only-true',
                         'missing-map','missing-report','missing-source','duplicate-map','source-omission','source-limit','archive-symlink'):
            with self.subTest(mutation=mutation), self.fixture() as f:
                self.guard(f)
                def edit(archive):
                    source, ids, report=deepcopy(f.source),deepcopy(f.identities),deepcopy(f.report)
                    if mutation=='report-count': report['record_count']=2
                    elif mutation=='report-mapping': report['mappings'][0]['issue']['number']=9999
                    elif mutation=='report-comparison': report['field_comparisons'][0]['brief']=False
                    elif mutation=='report-only-true': report={'all_fields_preserved':True}
                    elif mutation=='source-omission': source['tasks'].pop()
                    elif mutation=='source-limit': source['tasks'][0]['notes'][0]='Different archived limitation'
                    archive.json('progress/state.json',source); archive.json(IDENTITIES,ids); archive.json(REPORT,report)
                    if mutation=='missing-map': (archive.root/IDENTITIES).unlink()
                    elif mutation=='missing-report': (archive.root/REPORT).unlink()
                    elif mutation=='missing-source': (archive.root/'progress/state.json').unlink()
                    elif mutation=='duplicate-map': archive.write(IDENTITIES,b'{"version":1,"version":1,"entries":[]}')
                    elif mutation=='archive-symlink':
                        (archive.root/REPORT).unlink()
                        # A valid report as link bytes isolates the Git mode check.
                        (archive.root/REPORT).symlink_to(json.dumps(report))
                f.prepare_archive(edit)
                if mutation is None:
                    self.guard(f)
                else:
                    self.guard_refusal(f)

    def test_brief_relocation_requires_exact_archive_path_mode_and_bytes(self):
        with self.fixture() as f:
            self.guard(f); f.existing()
            f.selected['brief']=f'https://github.com/Kernol12/subathon-bot-ai/blob/{f.archive}/{BRIEF}'
            f.persist(); self.guard(f,bootstrap=False)
            for replacement in (CONTRACT, f'https://github.com/Kernol12/subathon-bot-ai/blob/{f.source_revision}/{BRIEF}',
                                f'https://github.com/Kernol12/subathon-bot-ai/blob/{f.archive}/{CONTRACT}'):
                f.selected['brief']=replacement; f.persist(); self.guard_refusal(f,bootstrap=False)
        with self.fixture() as f:
            self.guard(f)
            f.write(BRIEF,b'Updated acceptance after archive\n'); f.head=f.commit('Synthetic changed base brief')
            f.existing()
            self.guard(f,bootstrap=False)
            f.selected['brief']=f'https://github.com/Kernol12/subathon-bot-ai/blob/{f.archive}/{BRIEF}'
            f.persist(); self.guard_refusal(f,bootstrap=False)

    def test_new_id_exact_stanzas_and_selected_complete_packet_in_both_modes(self):
        for complete in (False,True):
            mutations=(None,'id','kind','issue','duplicate','unbound')
            if not complete: mutations+=('unreviewed','unselected')
            for mutation in mutations:
                with self.subTest(complete=complete,mutation=mutation), self.fixture() as f:
                    self.guard(f); f.existing()
                    brief='docs/new-publication.md'
                    text=('Task ID: NEW-903\nWork type: contract\n'
                          'Issue: https://github.com/Kernol12/subathon-bot-ai/issues/9905\n')
                    f.write(brief,text.encode()); rev=f.commit('Synthetic canonical new acceptance')
                    new=dict(version=1,task_id='NEW-903',issue=dict(repository='Kernol12/subathon-bot-ai',number=9905),
                             kind='contract',brief=brief,evidence=[],limitations=[],
                             workflow=dict(required_stages=['contract'],stages={'contract':packet(rev,[brief])}))
                    f.records['NEW-903']=new; f.policy['required_records']=sorted(f.records); f.persist()
                    args=f.arguments(bootstrap=False,task_id='NEW-903',complete=complete)
                    self.success(self.invoke(f,args))
                    if mutation is None: continue
                    if mutation=='id': text=text.replace('NEW-903','NEW-904')
                    elif mutation=='kind': text=text.replace('contract','Contract')
                    elif mutation=='issue': text=text.replace('/9905','/9906')
                    elif mutation=='duplicate': text+='Task ID: NEW-903\n'
                    elif mutation=='unbound': new['workflow']['stages']['contract']['paths']=[CONTRACT]
                    elif mutation=='unreviewed': new['workflow']['stages']['contract']['reviews']=[]
                    elif mutation=='unselected':
                        new['workflow']['required_stages'].append('tests')
                        new['workflow']['stages']['tests']=packet(rev,[CONTRACT])
                        args=f.arguments(bootstrap=False,task_id='NEW-903',stages=('tests',),complete=complete)
                    if mutation in ('id','kind','issue','duplicate'):
                        f.write(brief,text.encode()); changed=f.commit('Synthetic one bad stanza')
                        # Keep source binding/reviews current: the stanza must be the refusal.
                        new['workflow']['stages']['contract']=packet(changed,[brief])
                    f.persist(); self.refusal(self.invoke(f,args))

    def test_archive_origin_comparison_survives_corrupt_existing_base(self):
        with self.fixture() as f:
            self.guard(f)
            f.selected['verification']['baseline']='passed'; f.persist(); f.existing()
            self.guard_refusal(f,bootstrap=False)

    def test_archive_pin_must_precede_target_and_ancestor_proof_location_blocks_reset(self):
        with self.fixture() as f:
            self.guard(f)
            # Archive exists and is an ancestor of HEAD but is outgoing from this older target.
            f.target=f.source_revision
            f.git('update-ref','refs/remotes/origin/main',f.target)
            self.guard_refusal(f)

    def test_exempt_historical_workflow_can_gain_pending_and_reviewed_packets(self):
        with self.fixture() as f:
            self.guard(f); f.existing()
            r=f.records['ACCESS-002']
            r['workflow']=dict(required_stages=['contract'],stages={})
            f.persist(); self.guard(f,bootstrap=False)
            r['workflow']['stages']['contract']=packet(f.source_revision,[CONTRACT],complete=False)
            f.persist(); self.guard(f,bootstrap=False)
            r['workflow']['required_stages'].append('tests')
            f.persist(); self.guard(f,bootstrap=False)
            r['workflow']['stages']['tests']=packet(f.source_revision,[ORACLE])
            f.persist(); self.guard(f,bootstrap=False)
            with patch.object(publication,'ROOT',f.root):
                stages=publication.check_proof_bindings(dict(policy=f.policy,records=f.records),
                    'ACCESS-002',['contract','tests'],head=f.head)
            self.assertEqual([stage['complete'] for stage in stages],[False,True])
            self.refusal(self.invoke(f,f.arguments(bootstrap=False,task_id='ACCESS-002',
                                                  stages=('contract','tests'),complete=True)))

    def test_supplemental_historical_map_is_neither_upgraded_nor_removed(self):
        with self.fixture(supplemental=True) as f:
            self.guard(f); f.existing()
            original=deepcopy(f.records['APITEST-016'])
            for mutation in ('edit','remove'):
                f.records['APITEST-016']=deepcopy(original)
                r=f.records['APITEST-016']
                if mutation=='remove': del r['supplemental_workflow_evidence']
                else: r['supplemental_workflow_evidence']['format']='rewritten historical format'
                f.persist(); self.guard_refusal(f,bootstrap=False)

    def test_additive_archived_identity_uses_full_reconciled_kind_and_mapping(self):
        with self.fixture(dormant=True) as f:
            self.guard(f); f.existing()
            r=dict(version=1,task_id='WAIT-903',kind='contract',brief=BRIEF,
                   issue=dict(repository='Kernol12/subathon-bot-ai',number=9905),
                   evidence=[],limitations=[],verification={'acceptance':'not_run'},review=None)
            f.records['WAIT-903']=r; f.policy['required_records']=sorted(f.records)
            f.persist(); self.guard(f,bootstrap=False)
            for key,value in (('kind','cleanup'),('issue',dict(repository='Kernol12/subathon-bot-ai',number=9999))):
                original=r[key]; r[key]=value; f.persist(); self.guard_refusal(f,bootstrap=False)
                r[key]=original

    def test_selected_bundle_is_revalidated_not_trusted(self):
        with self.fixture() as f:
            self.bindings(f)
            bad=dict(policy=deepcopy(f.policy),records=deepcopy(f.records))
            bad['records'][SELECTED]['workflow']['stages']['contract']['reviews'][0]['reviewer']='synthetic-author'
            with self.assertRaises(ValueError) as error:
                self.bindings(f,bundle=bad)
            self.assertNotIn(NI,str(error.exception))

    def test_prepared_checkout_and_outgoing_private_history_keep_existing_guards(self):
        for mutation in ('dirty','head','private-history','merge-side'):
            with self.subTest(mutation=mutation), self.fixture() as f:
                self.success(self.invoke(f))
                args=f.arguments()
                if mutation=='dirty': f.write(CONTRACT,b'Uncommitted\n')
                elif mutation=='head': args=f.arguments(head=f.target)
                elif mutation=='private-history':
                    f.write('local-secrets/transient.txt')
                    f.git('add','--force','local-secrets/transient.txt'); f.commit('Synthetic private intermediate')
                    (f.root/'local-secrets/transient.txt').unlink(); f.head=f.commit('Remove private final tree')
                else:
                    f.git('checkout','-b','side')
                    f.write('local-secrets/side.txt'); f.git('add','--force','local-secrets/side.txt')
                    f.commit('Synthetic private side tree'); (f.root/'local-secrets/side.txt').unlink()
                    f.commit('Remove private side tree'); f.git('checkout','fixture-main')
                    f.write('unrelated-source.txt'); f.commit('Synthetic main change')
                    f.git('merge','--no-ff','--no-edit','side')
                self.refusal(self.invoke(f,args))

    def test_prepared_schema_witness_and_each_host_failure(self):
        with self.fixture() as f:
            self.success(self.invoke(f))
            for index in range(3):
                for response in ((1,b'',b'synthetic child failure'),(1,b'partial',b'synthetic child failure')):
                    result=self.invoke(f,responses={index:response})
                    self.refusal(result)
                    self.assertEqual(len(result[3]),index+1)
            for index in (2,):
                for stdout in (b'',WITNESS+b'\n',WITNESS+b' ',WITNESS.rstrip(),
                               b'{"tests":1,"failures":0,"errors":0,"skipped":1}\n'):
                    with self.subTest(index=index,stdout=stdout):
                        self.refusal(self.invoke(f,responses={index:(0,stdout,b'')}))

    def test_prepared_only_flags_require_proofs_mode(self):
        with self.fixture() as f:
            self.success(self.invoke(f))
            for flag in ('--complete','--bootstrap'):
                args=f.arguments(bootstrap=True,
                    stages=('contract','tests','implementation') if flag=='--complete' else ('contract',),
                    complete=(flag=='--complete'))
                self.success(self.invoke(f,args))
            for extra in (['--proofs'],['--unknown'],['--stage']):
                result=self.invoke(f,f.arguments()+extra)
                self.assertEqual(result[:2],(2,'')); self.assertEqual(result[3],[]); self.assertIn('usage:',result[2])
                self.assertNotIn(NI,result[2])


class StrictGitJsonTests(unittest.TestCase):
    def test_bytes_parser_matches_strict_filesystem_transport(self):
        self.assertEqual(proofs.read_json_bytes(b'{"a":[1,"seven"]}', 'synthetic Git blob'), {'a':[1,'seven']})
        for payload in (b'\xef\xbb\xbf{}',b'\xff',b'{"a":1,"a":2}',b'{"a":NaN}',
                        b'{"a":1e999}',b'{',b'['*100000+b']'*100000):
            with self.subTest(payload=payload[:40]):
                self.assertEqual(proofs.read_json_bytes(b'{}','positive'),{})
                with self.assertRaises(ValueError) as error:
                    proofs.read_json_bytes(payload,'synthetic Git blob')
                self.assertNotIn(NI,str(error.exception))


if __name__ == '__main__':
    unittest.main()
