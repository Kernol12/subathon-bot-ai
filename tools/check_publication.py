#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check recorded source bindings, outgoing paths and actual local publication checks."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import progress
import proofs
import workflow

ROOT = Path(__file__).resolve().parents[1]
OUTPUT_LIMIT = 1024 * 1024
DEADLINE = 120
SCHEMA_WITNESS = b'{"tests":1,"failures":0,"errors":0,"skipped":0}\n'
LIMITATIONS = [
    "Recorded bindings are not authenticated approvals.",
    "Forbidden-path checks are not complete secret scanning.",
    "Current-head hosted, repository and owner gates are separate.",
    "Filesystem checks can observe ignored local files.",
]
# Load the shipped suite without __main__ discovery. Its setUpClass owns SDK
# metadata admission and all standards-library imports; this driver is stdlib only.
SCHEMA_DRIVER = r'''
import importlib.util
import sys
import unittest

spec = importlib.util.spec_from_file_location("publication_schema", sys.argv[1])
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
suite = unittest.TestLoader().loadTestsFromName(
    "RepositorySchemas.test_progress_schema_and_actual_state", module)
if suite.countTestCases() != 1:
    raise SystemExit(1)
result = unittest.TextTestRunner(verbosity=2, stream=sys.stderr).run(suite)
if (result.testsRun != 1 or result.failures or result.errors or result.skipped
        or result.expectedFailures or result.unexpectedSuccesses):
    raise SystemExit(1)
sys.stdout.write('{"tests":1,"failures":0,"errors":0,"skipped":0}\n')
'''


# Run the unchanged aggregate, then validate all canonical inputs using the same
# admitted class and offline registry. No SDK imports occur in this parent tool.
PROOF_SCHEMA_DRIVER = r'''
import importlib.util
import json
from pathlib import Path
import sys
import unittest

spec = importlib.util.spec_from_file_location("publication_proof_schema", sys.argv[1])
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
suite = unittest.TestLoader().loadTestsFromName(
    "ProofSchemas.test_proof_and_policy_contract", module)
if suite.countTestCases() != 1:
    raise SystemExit(1)
result = unittest.TextTestRunner(verbosity=2, stream=sys.stderr).run(suite)
if (result.testsRun != 1 or result.failures or result.errors or result.skipped
        or result.expectedFailures or result.unexpectedSuccesses):
    raise SystemExit(1)
root = module.ROOT
sys.path.insert(0, str(root / 'tools'))
import proofs
bundle = proofs.load(root / 'progress/proof-policy.json', root / 'progress/proofs')
cls = module.ProofSchemas
for name, values in [('proof-policy.schema.json', [bundle['policy']]),
                     ('proof.schema.json', bundle['records'].values())]:
    schema = proofs.read_json(root / 'progress' / name)
    cls.Draft.check_schema(schema)
    validator = cls.Draft(schema, registry=cls.registry)
    for value in values:
        validator.validate(value)
sys.stdout.write('{"tests":1,"failures":0,"errors":0,"skipped":0}\n')
'''


def run_step(argv: list[str], *, git: bool = False) -> subprocess.CompletedProcess[bytes]:
    """Run one read-only child in the fixed environment, with no retries.

    Capture bounds are checked on completion; this is not a total allocation or
    process-tree guarantee. Shared process supervision belongs to WORKFLOW-007.
    """
    env = {"PATH": "/usr/bin:/bin", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8"}
    if git:
        env.update(GIT_OPTIONAL_LOCKS="0", GIT_LITERAL_PATHSPECS="1")
    try:
        result = subprocess.run(argv, cwd=ROOT, env=env, stdin=subprocess.DEVNULL,
                                capture_output=True, timeout=DEADLINE)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise ValueError(f"Child unavailable or deadline exceeded: {argv[0]}: {error}") from error
    if len(result.stdout) > OUTPUT_LIMIT or len(result.stderr) > OUTPUT_LIMIT:
        raise ValueError(f"Child output exceeds 1 MiB per stream: {argv[0]}")
    return result


def checked_step(argv: list[str], *, git: bool = False) -> bytes:
    """Keep failed child diagnostics on the private stderr path, never in the summary."""
    result = run_step(argv, git=git)
    if result.returncode != 0:
        diagnostic = (result.stdout + result.stderr).decode("utf-8", errors="replace")
        raise ValueError(f"Child failed (exit {result.returncode}): {argv!r}\n{diagnostic}")
    return result.stdout


def git_output(*args: str) -> bytes:
    # Inspect real objects: local replacement refs are not the objects sent by a push.
    return checked_step(["git", "--no-replace-objects", *args], git=True)


def resolve_commit(ref: str) -> str:
    """Peel one explicit revision to a commit without allowing Git options."""
    if not ref or ref.startswith("-") or "\0" in ref:
        raise ValueError("Expected a non-option Git commit reference")
    raw = git_output("rev-parse", "--verify", "--end-of-options", ref + "^{commit}")
    if re.fullmatch(rb"[0-9a-f]{40}\n", raw) is None:
        raise ValueError("Git did not resolve exactly one 40-hex commit")
    return raw[:-1].decode("ascii")


def nul_records(raw: bytes) -> list[bytes]:
    if not raw:
        return []
    if not raw.endswith(b"\0"):
        raise ValueError("Malformed NUL-delimited Git output")
    return raw[:-1].split(b"\0")


def path_components(raw: bytes) -> list[str]:
    """Decode Git names strictly, preserving literal tabs, newlines and metacharacters."""
    name = raw.decode("utf-8")
    parts = name.split("/")
    if any(part in {"", ".", ".."} for part in parts) or "\0" in name:
        raise ValueError("Malformed tracked Git path")
    return parts


def recorded_file(revision: str, name: str) -> tuple[bytes, bytes]:
    # Existing validation checks filesystem containment too. Git must independently
    # compare the literal tracked name, its regular-file mode and its object ID.
    progress.local_path(name, ROOT)
    literal = Path(name).as_posix()
    entries = nul_records(git_output("ls-tree", "-z", "--full-tree", revision, "--", literal))
    if len(entries) != 1:
        raise ValueError(f"Recorded path is missing or ambiguous: {name!r}")
    metadata, raw_name = entries[0].split(b"\t", 1)
    parts = path_components(raw_name)
    mode, kind, oid = metadata.split(b" ")
    if ("/".join(parts) != literal or mode not in {b"100644", b"100755"}
            or kind != b"blob" or re.fullmatch(rb"[0-9a-f]{40}", oid) is None):
        raise ValueError(f"Recorded path is not the selected regular Git file: {name!r}")
    return mode, oid


def check_outgoing(base: str, head: str) -> int:
    """Inspect full trees of all newly reachable commits, including side history/merges."""
    commits = git_output("rev-list", base + ".." + head).splitlines()
    for commit in commits:
        if re.fullmatch(rb"[0-9a-f]{40}", commit) is None:
            raise ValueError("Malformed Git revision list")
        listing = git_output("ls-tree", "-r", "-z", "--name-only", "--full-tree", commit.decode("ascii"))
        for raw_name in nul_records(listing):
            parts = path_components(raw_name)
            basename = parts[-1]
            if (parts[0] in {"local-evidence", "local-secrets"}
                    or any(part in {".venv", "node_modules"} for part in parts)
                    or (basename == ".env" or basename.startswith(".env.")) and basename != ".env.example"
                    or parts[:2] == ["docs", "evidence"] and parts != ["docs", "evidence", "README.md"]):
                raise ValueError(f"Forbidden outgoing Git path: {'/'.join(parts)!r}")
    return len(commits)


def check_checkout(base: str, head: str) -> None:
    """Bind checks to the actual repository and commit, including a valid detached HEAD."""
    top = git_output("rev-parse", "--show-toplevel").decode("utf-8").removesuffix("\n")
    if Path(top).resolve() != ROOT.resolve():
        raise ValueError("Checker ROOT is not the Git working-tree root")
    if head != resolve_commit("HEAD"):
        raise ValueError("Candidate head is not the actual checkout HEAD commit")
    git_output("merge-base", "--is-ancestor", base, head)
    merge = run_step(["git", "--no-replace-objects", "rev-parse", "--verify", "--quiet", "MERGE_HEAD"], git=True)
    if merge.returncode != 1:
        raise ValueError("Unresolved merge or invalid Git merge state")
    if git_output("status", "--porcelain=v1", "-z", "--untracked-files=all"):
        raise ValueError("Working tree has tracked changes, conflicts or non-ignored untracked files")


POLICY = 'progress/proof-policy.json'
RECORDS = 'progress/proofs'
ARCHIVE = 'docs/migrations/EPIC-029/archive/'
PREPARED_LIMITATIONS = LIMITATIONS + [
    'Remote target-tip freshness and archive availability require coordinator readback.',
    'Frozen exemptions preserve history and do not authorize forward completion.',
]


def tree_entry(revision: str, name: str):
    """Read a literal tree entry without consulting retired filesystem paths."""
    workflow.lexical_path(name, 'Git path')
    entries = nul_records(git_output('ls-tree', '-z', '--full-tree', revision, '--', name))
    if not entries:
        return None
    if len(entries) != 1:
        raise ValueError(f'Ambiguous Git path: {name}')
    metadata, raw_name = entries[0].split(b'\t', 1)
    mode, kind, oid = metadata.split(b' ')
    if '/'.join(path_components(raw_name)) != name:
        raise ValueError(f'Unexpected Git path: {name}')
    return mode, kind, oid


def regular_entry(revision: str, name: str):
    entry = tree_entry(revision, name)
    if entry is None or entry[0] not in {b'100644', b'100755'} or entry[1] != b'blob':
        raise ValueError(f'Missing or nonregular Git blob: {name}')
    return entry


def git_json(revision: str, name: str):
    try:
        entry = regular_entry(revision, name)
        return proofs.read_json_bytes(git_output('cat-file', 'blob', entry[2].decode('ascii')), name)
    except (OSError, ValueError) as error:
        raise ValueError(f'{name} at {revision}: {error}') from error


def tree_bundle(revision: str) -> dict:
    policy = git_json(revision, POLICY)
    directory = tree_entry(revision, RECORDS)
    if directory is None or directory[:2] != (b'040000', b'tree'):
        raise ValueError('Missing or non-directory canonical proofs')
    records = []
    listing = git_output('ls-tree', '-z', directory[2].decode('ascii'))
    for row in nul_records(listing):
        metadata, raw_name = row.split(b'\t', 1)
        name = raw_name.decode('utf-8')
        if re.fullmatch(proofs.ID + r'\.json', name) is None:
            raise ValueError(f'Noncanonical proof filename: {name!r}')
        record = git_json(revision, RECORDS+'/'+name)
        proofs.validate_record(record)
        if record['task_id'] != name[:-5]:
            raise ValueError(f'Proof filename/ID mismatch: {name}')
        records.append(record)
    proofs.validate(policy, records)
    return dict(policy=policy, records={r['task_id']: r for r in records})


def prefix(old, new, field):
    if new[:len(old)] != old:
        raise ValueError(f'Historical ordered prefix changed: {field}')


def forward_record(old: dict, new: dict, *, baseline: str, archive: str):
    """Compare provenance only; the shared workflow grammar owns review semantics."""
    for key in ('task_id', 'issue', 'kind'):
        if old[key] != new[key]:
            raise ValueError(f'Historical {key} changed: {old["task_id"]}')
    for key in ('verification', 'review', 'supplemental_workflow_evidence'):
        if (key in old) != (key in new) or old.get(key) != new.get(key):
            raise ValueError(f'Historical {key} changed or disappeared')
    for key in ('evidence', 'limitations'):
        prefix(old[key], new[key], key)
    if old['brief'] != new['brief']:
        path = old['brief']
        expected = f'https://github.com/Kernol12/subathon-bot-ai/blob/{archive}/{path}'
        if path.startswith('https://') or new['brief'] != expected:
            raise ValueError('Brief relocation is not the same archive path')
        if regular_entry(baseline, path) != regular_entry(archive, path):
            raise ValueError('Brief relocation changes baseline bytes or mode')
    if 'workflow' in old:
        if 'workflow' not in new:
            raise ValueError('Historical workflow disappeared')
        before, after = old['workflow'], new['workflow']
        prefix(before['required_stages'], after['required_stages'], 'required_stages')
        for name, packet in before['stages'].items():
            current = after['stages'].get(name)
            if current is None:
                raise ValueError(f'Historical packet disappeared: {name}')
            prefix(packet['reviews'], current['reviews'], 'reviews')
            if packet['revision'] == current['revision']:
                for key in ('authors', 'paths'):
                    if packet[key] != current[key]:
                        raise ValueError(f'Unchanged revision changed {key}: {name}')
    if 'publication' in old:
        before, after = old['publication'], new.get('publication')
        if after is None or before['pr'] != after['pr']:
            raise ValueError('Historical publication disappeared or changed PR')
        if before['state'] == 'merged':
            if before != after:
                raise ValueError('Merged publication is immutable')
        elif workflow.instant(after['observed_at'], 'observed_at') < workflow.instant(before['observed_at'], 'observed_at'):
            raise ValueError('Publication observation moved backwards')


def check_local_objects() -> None:
    """Refuse lazy-fetch configurations, including on Git without the env guard."""
    result = run_step(['git', '--no-replace-objects', 'config', '--get-regexp',
                       r'^(extensions\.partialclone|remote\..*\.promisor)$'], git=True)
    if result.returncode == 0:
        raise ValueError('Partial/promisor clone cannot supply offline proof publication')
    if result.returncode != 1:
        raise ValueError('Cannot inspect partial/promisor clone configuration')


def check_proof_policy(base: str, head: str, *, task_id: str,
                       bootstrap: bool = False) -> dict:
    """Check canonical Git inputs against target history and pinned archive origin."""
    check_local_objects()
    base, head = resolve_commit(base), resolve_commit(head)
    check_checkout(base, head)
    if git_output('rev-parse', '--is-shallow-repository') != b'false\n':
        raise ValueError('Full history is required for proof publication')
    # Traverse the entire reachable commit history even for an existing policy.
    # An archive equal to base must not conceal unavailable older parents.
    git_output('rev-list', head)
    if base != resolve_commit('refs/remotes/origin/main'):
        raise ValueError('Base must equal refs/remotes/origin/main')
    candidate = tree_bundle(head)
    # Filesystem discovery additionally rejects symlink ancestors and ignored children.
    actual = proofs.load(ROOT/POLICY, ROOT/RECORDS)
    if actual != candidate:
        raise ValueError('Canonical proof working bytes differ from HEAD')
    for path in [POLICY, *(RECORDS+'/'+i+'.json' for i in candidate['records'])]:
        recorded_file(head, path)
    proofs.selected(candidate, task_id)
    presence = [tree_entry(base, path) is not None for path in (POLICY, RECORDS)]
    if any(presence) != all(presence):
        raise ValueError('One-sided canonical proof inputs at base')
    if all(presence):
        if bootstrap:
            raise ValueError('Bootstrap refuses existing canonical proof policy')
        previous = tree_bundle(base)
    else:
        if not bootstrap:
            raise ValueError('Initial canonical proof introduction requires --bootstrap')
        for revision in git_output('rev-list', base).decode('ascii').splitlines():
            if any(tree_entry(revision, path) is not None for path in (POLICY, RECORDS)):
                raise ValueError('Canonical proof ancestor prevents bootstrap reset')
        previous = None
    policy = candidate['policy']
    archive = resolve_commit(policy['archive_commit'])
    if archive != policy['archive_commit']:
        raise ValueError('Archive must name its full commit')
    git_output('merge-base', '--is-ancestor', archive, base)
    source = git_json(archive, 'progress/state.json')
    identities = git_json(archive, ARCHIVE+'identities.json')
    extracted = proofs.convert(source, identities, archive)
    report = git_json(archive, ARCHIVE+'conversion-report.json')
    if (json.dumps(report, sort_keys=True, ensure_ascii=False)
            != json.dumps(extracted['report'], sort_keys=True, ensure_ascii=False)):
        raise ValueError('Archive conversion report differs from full reconciliation')
    origin = dict(policy=extracted['policy'], records={r['task_id']: r for r in extracted['records']})
    archived_ids = {entry['task_id'] for entry in identities['entries']}
    previous_records = previous['records'] if previous else {}
    new_ids = set(candidate['records']) - set(previous_records) - archived_ids
    if new_ids - {task_id}:
        raise ValueError('New identities must be selected for reviewed brief admission')

    def relocation_baseline(identity):
        # Already published archive URLs remain valid after their old local files
        # are retired. A newly changed pointer must match the actual target bytes.
        before = previous_records.get(identity)
        current = candidate['records'][identity]
        if before is not None and before['brief'] == current['brief']:
            return archive
        return base
    for baseline, bundle in [(archive, origin)] + ([(base, previous)] if previous else []):
        for key in ('archive_commit', 'legacy_done', 'legacy_done_sha256'):
            if bundle['policy'][key] != policy[key]:
                raise ValueError(f'Frozen policy {key} changed')
        if not set(bundle['records']) <= set(candidate['records']):
            raise ValueError('Historical record enrollment shrank')
        for identity, record in bundle['records'].items():
            compare_at = relocation_baseline(identity) if baseline == archive else base
            forward_record(record, candidate['records'][identity], baseline=compare_at, archive=archive)
    # Archive-only identities may enroll later, but cannot change their provenance.
    mapped = {i['task_id']: i['issue'] for i in identities['entries']}
    for task in source['tasks']:
        identity = task['id']
        if identity in candidate['records'] and identity not in origin['records']:
            old = dict(version=1, task_id=identity, issue=mapped[identity], kind=task['kind'],
                       brief=task['brief'], evidence=task['evidence'], limitations=task.get('notes', []))
            for key in proofs.OPTIONAL:
                if key in task:
                    old[key] = task[key]
            forward_record(old, candidate['records'][identity], baseline=relocation_baseline(identity), archive=archive)
    return dict(mode='existing' if previous else 'bootstrap', record_count=len(candidate['records']),
                archive_record_count=len(origin['records']), legacy_exempt=task_id in policy['legacy_done'])


def check_proof_bindings(bundle: dict, task_id: str, stages: list[str], *,
                         head: str, complete: bool = False) -> list[dict]:
    """Bind selected packets normally, including every required stage at completion."""
    check_local_objects()
    head = resolve_commit(head)
    record = proofs.selected(bundle, task_id)
    if len(stages) != len(set(stages)):
        raise ValueError('Duplicate selected stages')
    if complete:
        proofs.require_complete(bundle, task_id)
        if task_id in bundle['policy']['legacy_done']:
            raise ValueError('Frozen exemption does not authorize forward completion')
        if set(stages) != set(record['workflow']['required_stages']):
            raise ValueError('Complete selection must include every required stage')
    result = []
    for name in stages:
        selected = proofs.context(bundle, task_id, stage=name)['stage_context']
        packet = selected['packet']
        if packet is None:
            raise ValueError(f'Selected stage has not started: {name}')
        revision = resolve_commit(packet['revision'])
        for path in packet['paths']:
            if recorded_file(revision, path) != recorded_file(head, path):
                raise ValueError(f'Stale recorded path binding: {name}: {path!r}')
        result.append(dict(name=name, revision=revision, path_count=len(packet['paths']), complete=selected['complete']))
    archive = bundle['policy']['archive_commit']
    identities = git_json(archive, ARCHIVE+'identities.json')
    archived_ids = {i['task_id'] for i in identities['entries']}
    if task_id not in archived_ids:
        brief = record['brief']
        bound = any(s['complete'] and brief in record['workflow']['stages'][s['name']]['paths'] for s in result)
        if not bound:
            raise ValueError('New ID brief requires a selected complete reviewed packet')
        entry = regular_entry(head, brief)
        text = git_output('cat-file', 'blob', entry[2].decode('ascii')).decode('utf-8')
        for label, value in [('Task ID', task_id), ('Work type', record['kind']),
                             ('Issue', f'https://github.com/{record["issue"]["repository"]}/issues/{record["issue"]["number"]}')]:
            if re.findall(r'^'+re.escape(label)+r':(.*)$', text, re.M) != [' '+value]:
                raise ValueError(f'New ID brief must have exactly one canonical {label} stanza')
    return result

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    parser.add_argument("--task", required=True)
    parser.add_argument("--stage", action="append", required=True)
    parser.add_argument("--schema-python", required=True)
    parser.add_argument("--proofs", action="store_true")
    parser.add_argument("--complete", action="store_true")
    parser.add_argument("--bootstrap", action="store_true")
    args = parser.parse_args(argv)
    if (args.complete or args.bootstrap) and not args.proofs:
        parser.error("--complete and --bootstrap require --proofs")
    try:
        # abspath preserves the selected venv symlink and uses the original caller cwd.
        schema_python = os.path.abspath(args.schema_python)
        if len(args.stage) != len(set(args.stage)):
            raise ValueError("Duplicate selected stages")
        if args.proofs:
            check_local_objects()
        base, head = resolve_commit(args.base), resolve_commit(args.head)
        check_checkout(base, head)
        if args.proofs:
            guard = check_proof_policy(base, head, task_id=args.task, bootstrap=args.bootstrap)
            bundle = proofs.load(ROOT/POLICY, ROOT/RECORDS)
            stages = check_proof_bindings(bundle, args.task, args.stage, head=head, complete=args.complete)
        else:
            data = json.loads((ROOT / "progress/state.json").read_text(encoding="utf-8"))
            progress.validate(data, root=ROOT)
            task = progress.context(data, args.task)["task"]
            stages = []
            for name in args.stage:
                selected = workflow.stage_context(task, name)
                packet = selected["packet"]
                if packet is None:
                    raise ValueError(f"Selected stage has not started: {name}")
                revision = resolve_commit(packet["revision"])
                for path in packet["paths"]:
                    if recorded_file(revision, path) != recorded_file(head, path):
                        raise ValueError(f"Stale recorded path binding: {name}: {path!r}")
                stages.append({"name": name, "revision": revision, "path_count": len(packet["paths"]),
                               "complete": selected["complete"]})
        commit_count = check_outgoing(base, head)
        host = [os.path.abspath(sys.executable), "-E", "-s", "-B"]
        checked_step([*host, "tools/progress.py", "validate"])
        checked_step([*host, "tools/progress.py", "render", "--check"])
        checked_step([*host, "tools/check_project.py"])
        witness = checked_step([schema_python, "-I", "-S", "-B", "-c", SCHEMA_DRIVER,
                                "tests/schema_validation/test_project_schemas.py"])
        if witness != SCHEMA_WITNESS:
            raise ValueError("Actual schema driver did not emit the exact one-test success witness")
        if args.proofs:
            witness = checked_step([schema_python, "-I", "-S", "-B", "-c", PROOF_SCHEMA_DRIVER,
                                    "tests/schema_validation/test_proof_schemas.py"])
            if witness != SCHEMA_WITNESS:
                raise ValueError("Actual proof schema driver did not emit the exact one-test success witness")
        summary = {
            "task": args.task, "base": base, "head": head, "commit_count": commit_count,
            "stages": stages,
            "checks": {key: "passed" for key in
                       ("recorded_paths", "outgoing_paths", "progress", "render", "project", "schema")},
            "limitations": LIMITATIONS,
        }
        if args.proofs:
            summary = dict(task=args.task, base=base, head=head, commit_count=commit_count,
                           stages=stages, guard=guard,
                           checks={key: "passed" for key in ("recorded_paths", "outgoing_paths", "proofs",
                               "policy_history", "progress", "render", "project", "schema", "proof_schema")},
                           limitations=PREPARED_LIMITATIONS)
        print(json.dumps(summary, indent=2, ensure_ascii=False))
        return 0
    except (OSError, ValueError, TypeError, KeyError) as error:
        print(f"Publication error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
