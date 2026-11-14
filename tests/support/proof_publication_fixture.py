# SPDX-License-Identifier: GPL-3.0-or-later
"""Small literal archive, independent extraction/report oracle and disposable Git.

Authored only. No import/execution before independent test closing. The complete
archive identity universe is four tasks; the frozen policy is not 120 fake tasks.
"""
from copy import deepcopy
import json
from pathlib import Path
from publication_fixture import PublicationFixture
from frozen_proof_policy import LEGACY_DONE, LEGACY_DONE_SHA256

POLICY = 'progress/proof-policy.json'
RECORDS = 'progress/proofs'
IDENTITIES = 'docs/migrations/EPIC-029/archive/identities.json'
REPORT = 'docs/migrations/EPIC-029/archive/conversion-report.json'
SELECTED = 'PUB-901'
CONTRACT = 'docs/synthetic-publication.md'
ORACLE = 'tests/synthetic-publication.txt'
RUNTIME = 'tools/synthetic-publication.txt'
BRIEF = 'docs/synthetic-publication-brief.md'


def receipt(phase, revision):
    return dict(phase=phase, provider='codex' if phase == 'verification' else 'claude',
                model='synthetic-model', reviewer='synthetic-independent-'+phase,
                revision=revision, outcome='approved',
                evidence='https://example.test/reviews/'+phase, findings=[], blocking_findings=[])


def packet(revision, paths, complete=True):
    return dict(authors=['synthetic-author'], revision=revision, paths=list(paths),
                reviews=[receipt(p, revision) for p in ('initial', 'verification', 'closing')] if complete else [])


def observation(head, state='open', at='2026-11-12T12:00:00Z',
                merged_at='2026-11-12T11:00:00Z'):
    return dict(state=state, pr='https://github.com/Kernol12/subathon-bot-ai/pull/9901',
                head=head, merge_commit=head if state == 'merged' else None,
                merged_at=merged_at if state == 'merged' else None, observed_at=at)


class ProofPublicationFixture(PublicationFixture):
    def __init__(self, root, *, publication=False, supplemental=False, dormant=False):
        super().__init__(root, prepare_current=False)
        self.write(CONTRACT, b'Synthetic contract\n')
        self.write(ORACLE, b'Independent expected result: seven\n')
        self.write(RUNTIME, b'NI synthetic runtime\n')
        self.write(BRIEF, b'Synthetic archived acceptance\n')
        self.write('docs/retired-publication.md', b'Historical acceptance\n')
        self.source_revision = self.commit('Synthetic own source and NI baseline')
        wf = dict(required_stages=['contract', 'tests', 'implementation'], stages={
            'contract': packet(self.source_revision, [CONTRACT]),
            'tests': packet(self.source_revision, [ORACLE]),
            'implementation': packet(self.source_revision, [RUNTIME])})
        def task(identity, kind, number, workflow=None):
            value = dict(id=identity, title=identity, module='Quality and tooling', kind=kind,
                         status='in_progress' if workflow else 'planned', owner='synthetic-author',
                         depends_on=[], spec_refs=['§17'], brief=BRIEF, summary='Synthetic only',
                         notes=['Historical limitation one', 'Historical limitation two'],
                         evidence=['https://example.test/one', 'https://example.test/two'],
                         verification={'baseline': 'observed_red'}, review=None)
            if workflow is not None:
                value['workflow'] = workflow
            return value
        historical_id = 'APITEST-016' if supplemental else 'OLD-902'
        tasks = [task(SELECTED, 'implementation', 9901, wf),
                 task(historical_id, 'contract', 9902, dict(required_stages=['contract'], stages={
                     'contract': packet(self.source_revision, ['docs/retired-publication.md'])})),
                 task('ACCESS-002', 'contract', 9903), task('EPIC-901', 'epic', 9904)]
        if supplemental:
            tasks[1]['supplemental_workflow_evidence'] = json.loads((
                Path(__file__).parent/'apitest016-supplemental-b506434.json').read_text())
        tasks[1]['status'] = 'done'
        tasks[1]['publication'] = observation(self.source_revision, 'merged')
        tasks[1]['evidence'].append(tasks[1]['publication']['pr'])
        if publication:
            tasks[0]['publication'] = observation(self.source_revision)
            tasks[0]['evidence'].append(tasks[0]['publication']['pr'])
        if dormant:
            unstarted=task('WAIT-903', 'contract', 9905)
            unstarted.update(evidence=[], verification={'acceptance':'not_run'}, notes=[])
            tasks.append(unstarted)
        self.source = dict(schema_version=1, project='Synthetic publication fixture',
            baseline_version='0.1.0', specification_version='0.45',
            updated_at='2026-11-12T12:00:00Z', summary='Disposable archive',
            verification=[], activity=[])
        self.source['workflow_policy'] = dict(version=1, legacy_done=list(LEGACY_DONE),
                                               legacy_done_sha256=LEGACY_DONE_SHA256)
        self.source['tasks'] = tasks
        self.identities = dict(version=1, entries=[dict(task_id=t['id'], issue=dict(
            repository='Kernol12/subathon-bot-ai', number=9901+i)) for i, t in enumerate(tasks)])
        # Explicit extraction: only these input fields transfer, notes -> limitations.
        # This never asks convert/load/the guard for an expected record or report.
        records = []
        for t, identity in zip(tasks[:3], self.identities['entries']):
            r = dict(version=1, task_id=t['id'], issue=deepcopy(identity['issue']), kind=t['kind'],
                     brief=t['brief'], evidence=deepcopy(t['evidence']), limitations=deepcopy(t['notes']),
                     verification=deepcopy(t['verification']), review=None)
            for key in ('workflow', 'publication', 'supplemental_workflow_evidence'):
                if key in t:
                    r[key] = deepcopy(t[key])
            records.append(r)
        records.sort(key=lambda r: r['task_id'])
        self.records = {r['task_id']: r for r in records}
        self.report = dict(version=1, source_task_count=5 if dormant else 4, record_count=3,
            archive_only_ids=['EPIC-901', 'WAIT-903'] if dormant else ['EPIC-901'], enrolled_ids=['ACCESS-002', historical_id, SELECTED],
            mappings=deepcopy(self.identities['entries']), field_comparisons=[dict(task_id=i,
                kind=True, brief=True, evidence=True, limitations=True, workflow=True, publication=True,
                verification=True, review=True, supplemental_workflow_evidence=True)
                for i in (SELECTED, historical_id, 'ACCESS-002')], discrepancies=[])
        self.prepare_archive()

    def prepare_archive(self, edit=None):
        """Rebuild disposable archive before the first canonical proof introduction.

        reset is confined to this fixture's temporary repository. Candidate records
        remain the independent literal extraction, even when archive inputs change.
        """
        self.git('reset', '--hard', self.source_revision)
        for path in (self.root/'progress/proofs').glob('*.json'):
            path.unlink()
        (self.root/'progress/proof-policy.json').unlink(missing_ok=True)
        self.json('progress/state.json', self.source)
        self.json(IDENTITIES, self.identities)
        self.json(REPORT, self.report)
        if edit is not None:
            edit(self)
        self.archive = self.commit('Synthetic complete pinned archive')
        self.policy = dict(version=1, legacy_done=list(LEGACY_DONE), legacy_done_sha256=LEGACY_DONE_SHA256,
                           archive_commit=self.archive, required_records=sorted(self.records))
        # Target is an observed ancestor, never a guessed future hash.
        self.target = self.archive
        self.git('update-ref', 'refs/remotes/origin/main', self.target)
        # Archive bytes are historical inputs only; current tests use canonical records.
        for name in (IDENTITIES, REPORT, 'docs/retired-publication.md'):
            path = self.root/name
            if path.exists() or path.is_symlink():
                path.unlink()
        self.persist()

    @property
    def selected(self):
        return self.records[SELECTED]

    def json(self, name, value):
        self.write(name, (json.dumps(value, indent=2, ensure_ascii=False)+'\n').encode())

    def persist(self):
        self.json(POLICY, self.policy)
        directory = self.root/RECORDS
        directory.mkdir(exist_ok=True)
        for path in directory.glob('*.json'):
            path.unlink()
        for identity, record in self.records.items():
            self.json(f'{RECORDS}/{identity}.json', record)
        self.head = self.commit('Synthetic canonical proof observation')
        return self.head

    def existing(self):
        self.target = self.head
        self.git('update-ref', 'refs/remotes/origin/main', self.target)
        return self.target

    def arguments(self, *, bootstrap=True, stages=('contract',), complete=False,
                  schema_python='/fixture/schema-python', base=None, head='HEAD', task_id=SELECTED):
        result = ['--base', self.target if base is None else base, '--head', head,
                  '--task', task_id, '--schema-python', str(schema_python)]
        result += [arg for stage in stages for arg in ('--stage', stage)]
        if bootstrap:
            result.append('--bootstrap')
        if complete:
            result.append('--complete')
        return result
