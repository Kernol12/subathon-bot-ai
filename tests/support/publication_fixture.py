# SPDX-License-Identifier: GPL-3.0-or-later
"""Disposable WORKFLOW-005 Git/source fixture shared by unit and SDK tests.

Copies working-tree bytes, including local test/NI files; never clones HEAD or
manufactures approvals. Do not execute before the full independent test review.
"""
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys

SOURCE = Path(__file__).resolve().parents[2]
ENV = {"PATH": "/usr/bin:/bin", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8"}
GIT_ENV = dict(ENV, GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL="/dev/null",
               GIT_OPTIONAL_LOCKS="0", GIT_LITERAL_PATHSPECS="1")
LOCAL_SOURCES = (
    "tools/check_project.py", "tools/proofs.py",
    "tests/support/proof_publication_fixture.py",
    "tests/tooling/test_proof_publication.py",
    "tests/tooling/test_repository_checks.py",
    "tests/schema_validation/test_proof_publication_integration.py",
    "tools/check_publication.py",
    "tests/tooling/test_compact_context.py", "tests/tooling/test_publication.py",
    "tests/support/publication_fixture.py",
)


class PublicationFixture:
    def __init__(self, root, *, prepare_current=True):
        self.root = Path(root)
        self.root.mkdir(parents=True, exist_ok=True)
        # The project/link checks follow the shipped documentation and acceptance-map
        # closure. Copy tracked source artifacts, never ignored SDK/build/private data.
        listing = subprocess.run(["git", "ls-files", "-z"], cwd=SOURCE, env=GIT_ENV,
                                 check=True, capture_output=True, timeout=120).stdout
        paths = set(listing.decode("utf-8").split("\0")) - {""}
        paths.update(LOCAL_SOURCES)
        paths -= {'tests/tooling/test_progress.py', 'tests/schema_validation/test_publication_integration.py'}
        paths = {name for name in paths if name != 'progress/proof-policy.json'
                 and not name.startswith('progress/proofs/')}
        for name in sorted(paths - {"progress/state.json", "PROGRESS.html"}):
            source = SOURCE / name
            if not source.is_file():
                raise RuntimeError(f"Missing working-tree fixture source: {name}")
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target, follow_symlinks=False)
        # A valid repository control must precede every STATUS mutation. Never copy
        # the real canonical bundle into a synthetic Git/source fixture.
        status = self.root/'docs/STATUS.md'
        version = re.search(r'^Version: (\S+)',
                            (self.root/'docs/specification.md').read_text(encoding='utf-8'), re.M)
        if version is None:
            raise ValueError('Synthetic repository specification has no version')
        status.write_text('Specification version: '+version.group(1)+'\n', encoding='utf-8')
        self.git("init", "--initial-branch=fixture-main")
        self.git("config", "user.name", "Synthetic fixture")
        self.git("config", "user.email", "fixture@example.test")
        self.git("config", "commit.gpgsign", "false")
        self.git("config", "core.hooksPath", "/dev/null")
        self.git("config", "core.filemode", "true")
        self.base = self.commit("Synthetic source snapshot; no review claims")
        from frozen_proof_policy import LEGACY_DONE, LEGACY_DONE_SHA256
        self.policy = dict(version=1, legacy_done=list(LEGACY_DONE),
                           legacy_done_sha256=LEGACY_DONE_SHA256,
                           archive_commit=self.base, required_records=['WFIX-001'])
        self.record = dict(version=1, task_id='WFIX-001',
            issue=dict(repository='Kernol12/subathon-bot-ai', number=9901),
            kind='contract', brief='docs/tasks/WORKFLOW-005.md',
            limitations=[], evidence=[], verification={'acceptance':'not_run'}, review=None,
            workflow={'required_stages':['contract','tests','implementation'], 'stages':{
                'contract':{'authors':['fixture-author'], 'revision':self.base,
                            'paths':['docs/contracts/context-publication-v1.md'], 'reviews':[]},
                'tests':{'authors':['fixture-author'], 'revision':self.base,
                         'paths':['tests/tooling/test_publication.py'], 'reviews':[]}}})
        if prepare_current:
            # Independent historical extraction setup: never call the converter.
            from copy import deepcopy
            historical = dict(id='WFIX-001', title='Synthetic packet', module='Quality and tooling',
                kind='contract',status='in_progress',owner='fixture-author',depends_on=[],spec_refs=['§17'],
                brief=self.record['brief'],summary='No real review claims',notes=[],evidence=[],
                verification=deepcopy(self.record['verification']),review=None,workflow=deepcopy(self.record['workflow']))
            source=dict(schema_version=1,project='Synthetic publication fixture',baseline_version='0.1.0',
                specification_version='0.45',updated_at='2026-11-12T12:00:00Z',summary='Historical archive',
                verification=[],activity=[],tasks=[historical],workflow_policy=dict(version=1,
                    legacy_done=list(LEGACY_DONE),legacy_done_sha256=LEGACY_DONE_SHA256))
            identities=dict(version=1,entries=[dict(task_id='WFIX-001',issue=deepcopy(self.record['issue']))])
            report=dict(version=1,source_task_count=1,record_count=1,archive_only_ids=[],enrolled_ids=['WFIX-001'],
                mappings=deepcopy(identities['entries']),field_comparisons=[dict(task_id='WFIX-001',
                    kind=True,brief=True,evidence=True,limitations=True,workflow=True,publication=True,
                    verification=True,review=True,supplemental_workflow_evidence=True)],discrepancies=[])
            for name,value in (('progress/state.json',source),
                    ('docs/migrations/EPIC-029/archive/identities.json',identities),
                    ('docs/migrations/EPIC-029/archive/conversion-report.json',report)):
                self.write(name,(json.dumps(value,indent=2)+'\n').encode())
            self.policy['archive_commit']=self.commit('Synthetic immutable archive')
            self.save()
            self.target=self.commit('Synthetic initial canonical bundle')
            self.git('update-ref','refs/remotes/origin/main',self.target)
            self.record['limitations'].append('Synthetic observation only')
            self.save()
            self.head=self.commit('Record synthetic unreviewed packet observation')

    @property
    def task(self):
        return self.record

    def git(self, *args, check=True):
        return subprocess.run(["git", *args], cwd=self.root, env=GIT_ENV, check=check,
                              capture_output=True, timeout=120)

    def commit(self, message):
        self.git("add", "--all")
        self.git("commit", "--quiet", "--allow-empty", "-m", message)
        return self.git("rev-parse", "HEAD").stdout.decode("ascii").strip()

    def write(self, name, content=b"Synthetic fixture content\n"):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        return path

    def save(self):
        """Write only the independent canonical fixture, without invoking a renderer."""
        self.write('progress/proof-policy.json', (json.dumps(self.policy, indent=2)+'\n').encode())
        self.write('progress/proofs/WFIX-001.json', (json.dumps(self.record, indent=2)+'\n').encode())

    def arguments(self, *, base=None, head="HEAD", stages=("contract",), schema_python="/fixture/schema-python"):
        return ["--base", self.target if base is None else base, "--head", head,
                "--task", "WFIX-001", *[arg for stage in stages for arg in ("--stage", stage)],
                "--schema-python", str(schema_python)]

    def snapshot(self):
        """Bytes/modes outside .git plus refs/index/status: no candidate writes allowed."""
        files = {}
        for path in self.root.rglob("*"):
            relative = path.relative_to(self.root)
            if relative.parts[0] == ".git":
                continue
            if path.is_symlink():
                files[str(relative)] = ("link", os.readlink(path))
            elif path.is_file():
                files[str(relative)] = (path.stat().st_mode & 0o777, path.read_bytes())
        return (files, self.git("show-ref").stdout,
                (self.root / ".git/index").read_bytes(),
                self.git("status", "--porcelain=v1", "-z", "--untracked-files=all").stdout)
