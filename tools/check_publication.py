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
if result.testsRun != 1 or result.failures or result.errors or result.skipped:
    raise SystemExit(1)
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
        result = subprocess.run(argv, cwd=ROOT, env=env, capture_output=True, timeout=DEADLINE)
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
    return checked_step(["git", *args], git=True)


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
    merge = run_step(["git", "rev-parse", "--verify", "--quiet", "MERGE_HEAD"], git=True)
    if merge.returncode != 1:
        raise ValueError("Unresolved merge or invalid Git merge state")
    if git_output("status", "--porcelain=v1", "-z", "--untracked-files=all"):
        raise ValueError("Working tree has tracked changes, conflicts or non-ignored untracked files")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    parser.add_argument("--task", required=True)
    parser.add_argument("--stage", action="append", required=True)
    parser.add_argument("--schema-python", required=True)
    args = parser.parse_args(argv)
    try:
        # abspath preserves the selected venv symlink and uses the original caller cwd.
        schema_python = os.path.abspath(args.schema_python)
        if len(args.stage) != len(set(args.stage)):
            raise ValueError("Duplicate selected stages")
        base, head = resolve_commit(args.base), resolve_commit(args.head)
        check_checkout(base, head)
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
        summary = {
            "task": args.task, "base": base, "head": head, "commit_count": commit_count,
            "stages": stages,
            "checks": {key: "passed" for key in
                       ("recorded_paths", "outgoing_paths", "progress", "render", "project", "schema")},
            "limitations": LIMITATIONS,
        }
        print(json.dumps(summary, indent=2, ensure_ascii=False))
        return 0
    except (OSError, ValueError, TypeError, KeyError) as error:
        print(f"Publication error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
