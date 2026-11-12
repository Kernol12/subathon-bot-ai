# SPDX-License-Identifier: GPL-3.0-or-later
"""Disposable WORKFLOW-005 Git/source fixture shared by unit and SDK tests.

Copies working-tree bytes, including local test/NI files; never clones HEAD or
manufactures approvals. Do not execute before the full independent test review.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

SOURCE = Path(__file__).resolve().parents[2]
ENV = {"PATH": "/usr/bin:/bin", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8"}
GIT_ENV = dict(ENV, GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL="/dev/null",
               GIT_OPTIONAL_LOCKS="0", GIT_LITERAL_PATHSPECS="1")
LOCAL_SOURCES = (
    "tools/check_publication.py", "tools/progress.py",
    "tests/tooling/test_compact_context.py", "tests/tooling/test_publication.py",
    "tests/schema_validation/test_publication_integration.py",
    "tests/support/publication_fixture.py",
)


class PublicationFixture:
    def __init__(self, root):
        self.root = Path(root)
        self.root.mkdir(parents=True, exist_ok=True)
        # The project/link checks follow the shipped documentation and acceptance-map
        # closure. Copy tracked source artifacts, never ignored SDK/build/private data.
        listing = subprocess.run(["git", "ls-files", "-z"], cwd=SOURCE, env=GIT_ENV,
                                 check=True, capture_output=True, timeout=120).stdout
        paths = set(listing.decode("utf-8").split("\0")) - {""}
        paths.update(LOCAL_SOURCES)
        for name in sorted(paths - {"progress/state.json", "PROGRESS.html"}):
            source = SOURCE / name
            if not source.is_file():
                raise AssertionError(f"Missing working-tree fixture source: {name}")
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target, follow_symlinks=False)
        self.git("init", "--initial-branch=fixture-main")
        self.git("config", "user.name", "Synthetic fixture")
        self.git("config", "user.email", "fixture@example.test")
        self.git("config", "commit.gpgsign", "false")
        self.git("config", "core.hooksPath", "/dev/null")
        self.git("config", "core.filemode", "true")
        self.base = self.commit("Synthetic source snapshot; no review claims")
        self.data = {
            "schema_version": 1, "project": "Synthetic publication fixture",
            "baseline_version": "0.1.0", "specification_version": "0.45",
            "updated_at": "2026-11-12T12:00:00Z", "summary": "Disposable tooling state",
            "verification": [], "activity": [],
            "workflow_policy": {"version": 1, "legacy_done": [],
                                "legacy_done_sha256": hashlib.sha256(b"").hexdigest()},
            "tasks": [{"id": "WFIX-001", "title": "Synthetic packet", "module": "Quality and tooling",
                       "kind": "contract", "status": "in_progress", "owner": "fixture-author",
                       "brief": "docs/tasks/WORKFLOW-005.md", "summary": "No real review claims",
                       "depends_on": [], "spec_refs": ["§17"], "notes": [],
                       "verification": {"acceptance": "not_run"}, "evidence": [], "review": None,
                       "workflow": {"required_stages": ["contract", "tests", "implementation"], "stages": {
                           "contract": {"authors": ["fixture-author"], "revision": self.base,
                                        "paths": ["docs/contracts/context-publication-v1.md"], "reviews": []},
                           "tests": {"authors": ["fixture-author"], "revision": self.base,
                                     "paths": ["tests/tooling/test_publication.py"], "reviews": []}}}}],
        }
        self.save()
        self.head = self.commit("Record synthetic unreviewed packet bindings")

    @property
    def task(self):
        return self.data["tasks"][0]

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
        """Use the existing renderer only to prepare a current disposable snapshot."""
        (self.root / "progress/state.json").write_text(
            json.dumps(self.data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        subprocess.run([os.path.abspath(sys.executable), "-E", "-s", "-B",
                        str(self.root / "tools/progress.py"), "render"], cwd=self.root,
                       env=ENV, check=True, capture_output=True, timeout=120)

    def arguments(self, *, base=None, head="HEAD", stages=("contract",), schema_python="/fixture/schema-python"):
        return ["--base", self.base if base is None else base, "--head", head,
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
