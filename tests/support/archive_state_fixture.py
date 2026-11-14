# SPDX-License-Identifier: GPL-3.0-or-later
"""Frozen archive-format input builders; no candidate algorithms or current state."""
import hashlib

OLD_REV = "a" * 40
REV = "b" * 40
MERGE = "c" * 40
PR = "https://example.test/project/pull/42"


def receipt(phase, revision=REV, **changes):
    result = {
        "phase": phase, "provider": "codex" if phase == "verification" else "claude",
        "model": "synthetic-model", "reviewer": f"independent-{phase}",
        "revision": revision, "outcome": "approved",
        "evidence": f"https://example.test/reviews/{phase}",
        "findings": [], "blocking_findings": [],
    }
    result.update(changes)
    return result

def stage(reviews=None):
    return {
        "authors": ["author-session", "coauthor-session"], "revision": REV,
        "paths": ["docs/tasks/WORKFLOW-001.md"],
        "reviews": ([receipt("initial", OLD_REV), receipt("verification"), receipt("closing")]
                    if reviews is None else reviews),
    }

def publication(state="merged"):
    return {
        "state": state, "pr": PR, "head": REV,
        "merge_commit": MERGE if state == "merged" else None,
        "merged_at": "2026-11-11T12:00:00Z" if state == "merged" else None,
        "observed_at": "2026-11-11T12:01:00+00:00",
    }

def task(task_id="WFLOW-999", kind="contract", status="in_progress"):
    name = "contract" if kind == "contract" else "tests" if kind.startswith("test_") else "implementation"
    names = ["tests", "implementation"] if kind == "implementation" else [name]
    return {
        "id": task_id, "title": task_id, "module": "Quality and tooling", "kind": kind,
        "status": status, "owner": "synthetic-author", "depends_on": [],
        "spec_refs": ["§17"], "brief": "docs/tasks/WORKFLOW-001.md",
        "summary": "Synthetic bounded workflow task", "notes": ["Original task note"],
        "evidence": [PR], "verification": {"acceptance": "passed", "baseline": "observed_red"},
        "review": {"author": "author-session", "reviewer": "independent-session", "outcome": "approved"},
        "workflow": {"required_stages": names, "stages": {n: stage() for n in names}},
    }

def state(*tasks):
    return {
        "schema_version": 1, "project": "Synthetic workflow project", "baseline_version": "0.1.0",
        "specification_version": "0.45", "updated_at": "2026-11-11T12:02:00Z",
        "summary": "Synthetic state", "verification": [], "activity": [],
        "tasks": list(tasks) if tasks else [task()],
        "workflow_policy": {"version": 1, "legacy_done": [],
                            "legacy_done_sha256": hashlib.sha256(b"").hexdigest()},
    }

def exempt(data, *ids):
    """Literal fixture policy checksum; no production helper supplies the oracle."""
    data["workflow_policy"]["legacy_done"] = list(ids)
    raw = ("\n".join(sorted(ids)) + "\n").encode() if ids else b""
    data["workflow_policy"]["legacy_done_sha256"] = hashlib.sha256(raw).hexdigest()
