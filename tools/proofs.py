#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Load recorded proof without authenticating Git, review identity or authorization."""
from __future__ import annotations

import argparse
from copy import deepcopy
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import stat
import sys
import tempfile

import workflow
from proof_shapes import LEGACY_DONE, LEGACY_DONE_SHA256, SUPPLEMENTAL_SHAPE

ID = r'[A-Z][A-Z0-9]*-[0-9]{3}'
KINDS = {'code_review', 'cleanup', 'contract', 'environment', 'feasibility',
         'implementation', 'scaffold', 'test_baseline', 'test_draft', 'test_review'}
CHECKS = {'blocked', 'draft', 'failed', 'not_run', 'observed_red', 'partial', 'passed'}
OPTIONAL = ('workflow', 'publication', 'verification', 'review', 'supplemental_workflow_evidence')
WARNING = ('Recorded information only; Git contents, live GitHub/CI, review identity '
           'and authorization are not verified.')
OMISSIONS = (' Compact view omits complete workflow, other stages, supplemental history, '
             'non-blocking finding details and closing-only finding metrics; use full mode or review links.')


def closed(value, required, optional=(), field='object'):
    if not isinstance(value, dict) or not set(required).issubset(value) or not set(value).issubset(set(required) | set(optional)):
        raise ValueError(f'{field}: missing or unknown fields (requires {required})')
    return value


def identifier(value):
    if not isinstance(value, str) or re.fullmatch(ID, value) is None:
        raise ValueError(f'Invalid task ID: {value!r}')
    return value


def version(value):
    if type(value) is not int or value != 1:
        raise ValueError('Unsupported version; expected integer 1')


def safe_path(path: Path, *, directory=False) -> Path:
    """lstat each component before opening anything, including canonical FIFO names."""
    if ".." in path.parts:
        raise ValueError(f"Parent traversal in supplied path: {path}")
    path = Path(os.path.abspath(path))
    for part in (*reversed(path.parents), path):
        mode = part.lstat().st_mode
        if stat.S_ISLNK(mode):
            raise ValueError(f'Symlink path component: {part}')
        if part != path and not stat.S_ISDIR(mode):
            raise ValueError(f'Ancestor is not a directory: {part}')
    if not (stat.S_ISDIR(mode) if directory else stat.S_ISREG(mode)):
        raise ValueError(f'Expected {"directory" if directory else "regular file"}: {path}')
    return path


def read_json(path: Path) -> object:
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError(f'{path}: duplicate key {key}')
            result[key] = value
        return result
    def constant(value):
        raise ValueError(f'{path}: nonfinite JSON number {value}')
    def floating(value):
        number = float(value)
        if not math.isfinite(number):
            raise ValueError(f'{path}: nonfinite JSON number {value}')
        return number
    try:
        return json.loads(safe_path(path).read_text(encoding='utf-8'), object_pairs_hook=pairs, parse_constant=constant, parse_float=floating)
    except (ValueError, UnicodeError) as error:
        raise ValueError(f'{path}: {error}') from error


def discover_records(directory: Path) -> list[Path]:
    safe_path(directory, directory=True)
    result = sorted(directory.iterdir(), key=lambda p: p.name)
    for path in result:
        safe_path(path)
        if re.fullmatch(ID + r'\.json', path.name) is None:
            raise ValueError(f'Noncanonical proof filename: {path}')
    return result


def validate_policy(policy: object) -> None:
    closed(policy, ('version', 'legacy_done', 'legacy_done_sha256', 'archive_commit', 'required_records'), field='policy')
    version(policy['version'])
    workflow.commit(policy['archive_commit'], 'policy.archive_commit')
    ids = workflow.strings(policy['legacy_done'], 'policy.legacy_done')
    if set(ids) != set(LEGACY_DONE):
        raise ValueError('policy.legacy_done differs from frozen 120-member set')
    digest = hashlib.sha256(('\n'.join(sorted(ids))+'\n').encode()).hexdigest()
    if policy['legacy_done_sha256'] != LEGACY_DONE_SHA256 or digest != LEGACY_DONE_SHA256:
        raise ValueError('policy.legacy_done checksum mismatch')
    required = workflow.strings(policy['required_records'], 'policy.required_records')
    for value in required:
        identifier(value)
    if required != sorted(required):
        raise ValueError('policy.required_records must be sorted')


def issue(value):
    closed(value, ('repository', 'number'), field='issue')
    if value['repository'] != 'Kernol12/subathon-bot-ai' or type(value['number']) is not int or value['number'] <= 0:
        raise ValueError('issue: expected owned repository and positive integer number')


def reference(value, root=None):
    return workflow.evidence_reference(value)


def brief(value):
    workflow.text(value, 'brief')
    if value.startswith('https://'):
        if re.fullmatch(r'https://github\.com/Kernol12/subathon-bot-ai/blob/[0-9a-f]{40}/[A-Za-z0-9_./-]+', value) is None:
            raise ValueError('brief: expected immutable canonical archive URL')
        workflow.lexical_path(value.split('/blob/', 1)[1].split('/', 1)[1], 'brief')
    else:
        workflow.lexical_path(value, 'brief')


def supplemental_shape(value, shape=SUPPLEMENTAL_SHAPE, field='supplemental_workflow_evidence'):
    """Check the explicit historical positional shape, not a JSON Schema interpreter."""
    if isinstance(shape, dict):
        closed(value, shape, field=field)
        for key, child in shape.items():
            supplemental_shape(value[key], child, f'{field}.{key}')
    elif isinstance(shape, list):
        if not isinstance(value, list) or len(value) != len(shape):
            raise ValueError(f'{field}: historical array length differs')
        for i, (item, child) in enumerate(zip(value, shape)):
            supplemental_shape(item, child, f'{field}[{i}]')
    elif type(value).__name__ != shape:
        raise ValueError(f'{field}: expected {shape}')


def validate_record(record: object) -> None:
    _validate_record_fields(record, KINDS)


def _validate_record_fields(record: object, kinds: set[str]) -> None:
    """Closed consumed fields, also checked for archive-only epic converter inputs."""
    closed(record, ('version', 'task_id', 'issue', 'kind', 'brief', 'evidence', 'limitations'), OPTIONAL, 'proof')
    version(record['version']); identifier(record['task_id']); issue(record['issue'])
    if not isinstance(record['kind'], str) or record['kind'] not in kinds:
        raise ValueError(f"{record['task_id']}: unknown proof kind")
    brief(record['brief'])
    for key in ('evidence', 'limitations'):
        if not isinstance(record[key], list) or any(not isinstance(v, str) for v in record[key]):
            raise ValueError(f'{key}: expected string array')
    for value in record['evidence']:
        reference(value)
    if 'workflow' in record:
        value = closed(record['workflow'], ('required_stages', 'stages'), field='workflow')
        closed(value['stages'], (), workflow.STAGES, 'workflow.stages')
        for packet in value['stages'].values():
            closed(packet, ('authors', 'revision', 'paths', 'reviews'), field='packet')
            if not isinstance(packet['reviews'], list):
                raise ValueError('packet.reviews must be an array')
            for review in packet['reviews']:
                closed(review, ('phase', 'provider', 'model', 'reviewer', 'revision', 'outcome', 'evidence', 'findings', 'blocking_findings'), ('started_at', 'finished_at'), 'receipt')
        workflow.validate_workflow(value, record['kind'], Path(), None, reference, tree_free=True)
    if 'publication' in record:
        closed(record['publication'], ('state', 'pr', 'head', 'merge_commit', 'merged_at', 'observed_at'), field='publication')
        workflow.validate_publication(record['publication'], record, Path(), reference)
    if 'verification' in record:
        values = record['verification']
        if not isinstance(values, dict) or any(not isinstance(k, str) or not isinstance(v, str) or v not in CHECKS for k, v in values.items()):
            raise ValueError('verification: unknown observation')
    if record.get('review') is not None:
        value = closed(record['review'], (), ('author', 'reviewer', 'evidence', 'baseline', 'revision', 'scope', 'outcome'), 'review')
        if any(not isinstance(v, str) for v in value.values()) or ('outcome' in value and value['outcome'] not in {'approved', 'changes_requested'}):
            raise ValueError('review: invalid historical fields')
    if 'supplemental_workflow_evidence' in record:
        if record['task_id'] != 'APITEST-016':
            raise ValueError('Supplemental history is only defined for APITEST-016')
        supplemental_shape(record['supplemental_workflow_evidence'])


def validate(policy: object, records: list[dict]) -> None:
    validate_policy(policy)
    if not isinstance(records, list):
        raise ValueError('records must be a list')
    ids, issues = set(), set()
    for record in records:
        try:
            validate_record(record)
        except ValueError as error:
            raise ValueError(f'{record.get("task_id", "proof") if isinstance(record, dict) else "proof"}: {error}') from error
        key = (record['issue']['repository'], record['issue']['number'])
        if record['task_id'] in ids or key in issues:
            raise ValueError(f"{record['task_id']}: duplicate task/issue identity")
        ids.add(record['task_id']); issues.add(key)
    if ids != set(policy['required_records']):
        raise ValueError('Record enrollment differs from policy.required_records')


def checked_bundle(bundle):
    closed(bundle, ('policy', 'records'), field='bundle')
    if not isinstance(bundle['records'], dict):
        raise ValueError('bundle.records must be an object')
    validate(bundle['policy'], list(bundle['records'].values()))
    if any(key != value['task_id'] for key, value in bundle['records'].items()):
        raise ValueError('bundle record key differs from task_id')


def selected(bundle, task_id):
    checked_bundle(bundle)
    if task_id not in bundle['records']:
        raise ValueError(f'{task_id}: unknown proof task')
    return bundle['records'][task_id]


def require_complete(bundle: dict, task_id: str) -> None:
    record = selected(bundle, task_id)
    if 'workflow' not in record:
        raise ValueError(f'{task_id}: missing workflow')
    value = record['workflow']
    for name in value['required_stages']:
        if not workflow.stage_complete(value['stages'].get(name)):
            raise ValueError(f'{task_id}: stage {name} is pending or stale')


def load(policy_path: Path, records_dir: Path) -> dict:
    policy = read_json(policy_path)
    paths = discover_records(records_dir)  # preflight every child before opening any
    records = []
    for path in paths:
        record = read_json(path)
        validate_record(record)
        if record['task_id'] != path.stem:
            raise ValueError(f'{path}: filename differs from task_id')
        records.append(record)
    validate(policy, records)
    return dict(policy=policy, records={r['task_id']: r for r in records})


def context(bundle: dict, task_id: str, *, stage: str | None = None, compact: bool = False) -> dict:
    record = selected(bundle, task_id)
    names = record.get('workflow', {}).get('required_stages', [])
    value = record
    if compact:
        value = {k: v for k, v in record.items() if k not in ('workflow', 'supplemental_workflow_evidence')}
    result = dict(proof=value, required_stages=names, limitations=WARNING+(OMISSIONS if compact else ''))
    if stage is not None:
        view = workflow.stage_context({'id': task_id, 'workflow': record.get('workflow')}, stage)
        result['stage_context'] = workflow.compact_stage_context(view) if compact else view
    return deepcopy(result)


def convert(source: dict, identities: dict, archive_commit: str) -> dict:
    workflow.commit(archive_commit, 'archive_commit')
    if not isinstance(source, dict) or not isinstance(source.get('tasks'), list) or 'workflow_policy' not in source:
        raise ValueError('source requires tasks and workflow_policy')
    closed(identities, ('version', 'entries'), field='identities'); version(identities['version'])
    if not isinstance(identities['entries'], list):
        raise ValueError('identities.entries must be an array')
    mapped, pairs = {}, set()
    for entry in identities['entries']:
        closed(entry, ('task_id', 'issue'), field='identity'); identifier(entry['task_id']); issue(entry['issue'])
        key = (entry['issue']['repository'], entry['issue']['number'])
        if entry['task_id'] in mapped or key in pairs:
            raise ValueError('Duplicate identity mapping')
        mapped[entry['task_id']] = entry['issue']; pairs.add(key)
    tasks = source['tasks']; ids = []
    for task in tasks:
        if not isinstance(task, dict):
            raise ValueError('source task must be an object')
        ids.append(identifier(task.get('id')))
    if len(set(ids)) != len(ids) or set(ids) != set(mapped):
        raise ValueError('source/mapping task enrollment differs')
    policy = source['workflow_policy']
    closed(policy, ('version', 'legacy_done', 'legacy_done_sha256'), field='source.workflow_policy')
    policy = dict(version=policy['version'], legacy_done=deepcopy(policy['legacy_done']),
                  legacy_done_sha256=policy['legacy_done_sha256'], archive_commit=archive_commit, required_records=[])
    validate_policy(policy)
    records, comparisons = [], []
    for task in tasks:
        if not isinstance(task.get('kind'), str) or task['kind'] not in KINDS | {'epic'} or not isinstance(task.get('status'), str) or task['status'] not in {'planned', 'ready', 'in_progress', 'in_review', 'blocked', 'done'}:
            raise ValueError(f"{task['id']}: invalid source kind/status")
        claimed = task['status'] in {'in_progress', 'in_review', 'done'} and task['id'] not in LEGACY_DONE
        if not isinstance(task.get('verification'), dict):
            raise ValueError(f"{task['id']}: source verification must be an object")
        record = dict(version=1, task_id=task['id'], issue=deepcopy(mapped[task['id']]),
                      kind=task['kind'], brief=task.get('brief'), evidence=deepcopy(task.get('evidence')),
                      limitations=deepcopy(task.get('notes', [])))
        for key in OPTIONAL:
            if key in task:
                record[key] = deepcopy(task[key])
        # Epics are validated as source inputs but never emitted as proof records.
        _validate_record_fields(record, KINDS | {'epic'})
        workflow.validate_task(task, source['workflow_policy'], Path(), None, reference, tree_free=True)
        if task['kind'] == 'epic':
            continue
        enrolled = (claimed or any(k in task for k in ('workflow', 'publication', 'supplemental_workflow_evidence'))
                    or task.get('review') is not None or bool(task['evidence'])
                    or any(v not in {'not_run', 'draft'} for v in task.get('verification', {}).values()))
        if enrolled:
            records.append(record)
            comparisons.append(dict(task_id=task['id'], **{key: record.get(key) == task.get('notes' if key == 'limitations' else key, [] if key == 'limitations' else None)
                and ((key in record) == (key in task) if key in OPTIONAL else True)
                for key in ('kind', 'brief', 'evidence', 'limitations', *OPTIONAL)}))
    records.sort(key=lambda r: r['task_id'])
    policy['required_records'] = [r['task_id'] for r in records]
    validate(policy, records)
    if any(not v for row in comparisons for k, v in row.items() if k != 'task_id'):
        raise ValueError('Field reconciliation discrepancy')
    report = dict(version=1, source_task_count=len(tasks), record_count=len(records),
                  archive_only_ids=sorted(set(ids)-set(policy['required_records'])), enrolled_ids=list(policy['required_records']),
                  mappings=[dict(task_id=t['id'], issue=deepcopy(mapped[t['id']])) for t in tasks],
                  field_comparisons=comparisons, discrepancies=[])
    return dict(policy=policy, records=records, report=report)


def serialized(value):
    return json.dumps(value, indent=2, ensure_ascii=False)+'\n'


def prepare_conversion(args):
    """Admit a full disposable source strictly, then publish only owned private output."""
    import progress
    root = safe_path(args.source_root, directory=True)
    src, ids = safe_path(args.source), safe_path(args.identities)
    repo = Path(__file__).resolve().parents[1]
    if src in {root/'progress/state.json', repo/'progress/state.json'}:
        raise ValueError('Production state cannot be conversion source')
    output = Path(os.path.abspath(args.output_dir))
    safe_path(output.parent, directory=True)
    if output.is_relative_to(repo) or output.is_relative_to(root):
        raise ValueError('Output must be outside repository/source root')
    if any(output == p or output.is_relative_to(p) or p.is_relative_to(output) for p in (src, ids)):
        raise ValueError('Overlapping conversion inputs/output')
    if output.exists() or output.is_symlink():
        raise ValueError('Output already exists')
    source = read_json(src)
    admission = deepcopy(source)
    if isinstance(admission, dict) and isinstance(admission.get('tasks'), list):
        for task in admission['tasks']:
            task.setdefault('notes', [])
    progress.validate(admission, root)
    result = convert(source, read_json(ids), args.archive_commit)
    owned = False
    try:
        with tempfile.TemporaryDirectory(prefix='.proof-prepare-', dir=output.parent) as temporary:
            prepared = Path(temporary)
            (prepared/'proofs').mkdir(mode=0o700)
            outputs = {'proof-policy.json': result['policy'], 'report.json': result['report']}
            outputs.update({'proofs/'+r['task_id']+'.json': r for r in result['records']})
            for name, value in outputs.items():
                with (prepared/name).open('x', encoding='utf-8', newline='\n') as stream:
                    os.chmod(stream.name, 0o600)
                    stream.write(serialized(value))
            load(prepared/'proof-policy.json', prepared/'proofs')
            output.mkdir(mode=0o700)  # exclusive reservation, never replace an existing path
            owned = True
            for child in prepared.iterdir():
                child.rename(output/child.name)
    except BaseException:
        if owned:
            shutil.rmtree(output)
        raise
    return dict(version=1, record_count=len(result['records']))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    for command in ('validate', 'context'):
        sub = commands.add_parser(command)
        if command == 'context':
            sub.add_argument('task_id'); sub.add_argument('--stage'); sub.add_argument('--compact', action='store_true')
        sub.add_argument('--policy', type=Path, required=True)
        sub.add_argument('--records-dir', type=Path, required=True)
    sub = commands.add_parser('convert')
    for name in ('source', 'source-root', 'identities', 'output-dir'):
        sub.add_argument('--'+name, type=Path, required=True)
    sub.add_argument('--archive-commit', required=True)
    args = parser.parse_args()
    try:
        if args.command == 'convert':
            result = prepare_conversion(args)
        else:
            bundle = load(args.policy, args.records_dir)
            result = (context(bundle, args.task_id, stage=args.stage, compact=args.compact) if args.command == 'context'
                      else dict(version=1, record_count=len(bundle['records'])))
        print(serialized(result), end='')
        return 0
    except (ValueError, OSError) as error:
        print(f'Proof error: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
