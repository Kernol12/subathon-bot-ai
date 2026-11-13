# SPDX-License-Identifier: GPL-3.0-or-later
"""Recorded review/publication invariants; no remote or source-identity verification."""
from __future__ import annotations

import hashlib
import os
import re
import stat
import tempfile
from datetime import datetime
from pathlib import Path
from urllib.parse import urlsplit

STAGES = {"contract", "tests", "implementation"}
PHASE_PROVIDERS = {"initial": "claude", "verification": "codex", "closing": "claude"}
BOOKKEEPING = {"progress/state.json", "PROGRESS.html", "progress/coordinator-checkpoint.md"}


def text(value: object, field: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{field} must be nonempty text")
    return value


def strings(value: object, field: str, *, nonempty: bool = False) -> list[str]:
    if not isinstance(value, list) or (nonempty and not value):
        raise ValueError(f"{field} must be {'a nonempty' if nonempty else 'an'} array of strings")
    for item in value:
        text(item, field)
    if len(value) != len(set(value)):
        raise ValueError(f"{field} must not contain duplicates")
    return value


def commit(value: object, field: str) -> str:
    if not isinstance(value, str) or re.fullmatch(r"[0-9a-f]{40}", value) is None:
        raise ValueError(f"{field} must be a lowercase 40-hex Git commit")
    return value


def instant(value: object, field: str) -> datetime:
    """Parse an observed instant, including UTC Z on the Python 3.10 floor."""
    raw = text(value, field)
    try:
        parsed = datetime.fromisoformat(raw[:-1] + "+00:00" if raw.endswith("Z") else raw)
    except ValueError as error:
        raise ValueError(f"{field} must be a timezone-aware ISO instant") from error
    if parsed.tzinfo is None or parsed.utcoffset() is None:
        raise ValueError(f"{field} must be a timezone-aware ISO instant")
    return parsed


def required(record: object, keys: set[str], field: str) -> dict:
    if not isinstance(record, dict) or not keys.issubset(record):
        raise ValueError(f"{field} requires {', '.join(sorted(keys))}")
    return record


def https(value: object, field: str, evidence_path, root: Path) -> str:
    url = text(value, field)
    if not url.startswith("https://") or evidence_path(url, root) is not None:
        raise ValueError(f"{field} must be an HTTPS URL")
    return url


def lexical_path(value: object, field: str = "path") -> str:
    value = text(value, field)
    if any(c in value for c in ("\\", ":", "\0")) or any(p in ("", ".", "..") for p in value.split("/")):
        raise ValueError(f"{field}: expected canonical relative path: {value!r}")
    return value


def evidence_reference(value: object) -> str | None:
    """Tree-independent HTTPS grammar shared with production evidence resolution."""
    if isinstance(value, str) and value.startswith("https://"):
        parsed = urlsplit(value)
        try:
            parsed.port
        except ValueError as error:
            raise ValueError(f"Invalid HTTPS evidence URL: {value!r}") from error
        if (parsed.scheme != "https" or not parsed.hostname or parsed.username is not None
                or parsed.password is not None or any(char.isspace() for char in value)):
            raise ValueError(f"Invalid HTTPS evidence URL: {value!r}")
        return None
    return lexical_path(value, "evidence")


def bookkeeping(value: str) -> bool:
    return value in BOOKKEEPING or value == "progress/proof-policy.json" or value.startswith("progress/proofs/")


def kind_minimum(kind: str) -> set[str]:
    if kind == "epic":
        return set()
    if kind == "contract":
        return {"contract"}
    if kind in {"test_draft", "test_review", "test_baseline"}:
        return {"tests"}
    return {"tests", "implementation"} if kind == "implementation" else {"implementation"}


def validate_workflow(value: object, kind: str, root: Path, local_path, evidence_path, *, tree_free=False) -> None:
    value = required(value, {"required_stages", "stages"}, "workflow")
    names = strings(value["required_stages"], "workflow.required_stages", nonempty=True)
    if not set(names).issubset(STAGES):
        raise ValueError("Unknown required workflow stage")
    if not kind_minimum(kind).issubset(names):
        raise ValueError("Required stages omit task-kind minimum")
    if not isinstance(value["stages"], dict) or not set(value["stages"]).issubset(names):
        raise ValueError("Started stages must be a subset of required stages")
    for stage in value["stages"].values():
        validate_stage(stage, root, local_path, evidence_path, tree_free=tree_free)


def legacy_done(policy: object) -> set[str]:
    """Check the recorded exception list's integrity, not its historical authenticity."""
    if policy is None:
        return set()
    policy = required(policy, {"version", "legacy_done", "legacy_done_sha256"}, "workflow_policy")
    if type(policy["version"]) is not int or policy["version"] != 1:
        raise ValueError("Unsupported workflow_policy version")
    ids = strings(policy["legacy_done"], "workflow_policy.legacy_done")
    raw = ("\n".join(sorted(ids)) + "\n").encode("utf-8") if ids else b""
    if policy["legacy_done_sha256"] != hashlib.sha256(raw).hexdigest():
        raise ValueError("workflow_policy legacy_done checksum does not match")
    return set(ids)


def tracked(task: dict, policy: dict | None) -> bool:
    return (policy is not None and task["kind"] != "epic"
            and task["id"] not in policy["legacy_done"])


def dependency_complete(task: dict, policy: dict | None = None) -> bool:
    return (task["status"] == "done" and
            (not tracked(task, policy) or task.get("publication", {}).get("state") == "merged"))


def validate_stage(stage: object, root: Path, local_path, evidence_path, *,
                   tree_free: bool = False) -> None:
    """Check packet ownership and ordered review claims without authenticating them."""
    stage = required(stage, {"authors", "revision", "paths", "reviews"}, "workflow stage")
    authors = strings(stage["authors"], "stage.authors", nonempty=True)
    commit(stage["revision"], "stage.revision")
    excluded = {(root / path).resolve() for path in BOOKKEEPING} if not tree_free else set()
    for value in strings(stage["paths"], "stage.paths", nonempty=True):
        lexical_path(value, "stage.paths")
        if bookkeeping(value):
            raise ValueError(f"Stage path is bookkeeping: {value}")
        if not tree_free:
            path = local_path(value, root)
            if not path.is_file() or path in excluded or bookkeeping(path.relative_to(root.resolve()).as_posix()):
                raise ValueError(f"Stage path must be an existing non-bookkeeping file: {value}")
    if not isinstance(stage["reviews"], list):
        raise ValueError("stage.reviews must be an array")
    previous = None
    for index, review in enumerate(stage["reviews"]):
        review = required(review, {"phase", "provider", "model", "reviewer", "revision", "outcome",
                                   "evidence", "findings", "blocking_findings"}, "review receipt")
        phase = review["phase"]
        if not isinstance(phase, str) or phase not in PHASE_PROVIDERS:
            raise ValueError("Unknown review phase")
        if review["provider"] != PHASE_PROVIDERS[phase]:
            raise ValueError(f"Review {phase} requires provider {PHASE_PROVIDERS[phase]}")
        text(review["model"], "review.model")
        if text(review["reviewer"], "review.reviewer") in authors:
            raise ValueError("Reviewers must be independent of every stage author")
        commit(review["revision"], "review.revision")
        https(review["evidence"], "review.evidence", evidence_path, root)
        findings = strings(review["findings"], "review.findings")
        blockers = strings(review["blocking_findings"], "review.blocking_findings")
        if not set(blockers).issubset(findings):
            raise ValueError("Review blockers must be a subset of findings")
        if (not isinstance(review["outcome"], str)
                or review["outcome"] not in {"approved", "changes_requested"}
                or bool(blockers) != (review["outcome"] == "changes_requested")):
            raise ValueError("Review outcome must agree with blocking findings")
        if ("started_at" in review) != ("finished_at" in review):
            raise ValueError("Review times must be supplied as a pair")
        if "started_at" in review:
            if instant(review["finished_at"], "review.finished_at") < instant(review["started_at"], "review.started_at"):
                raise ValueError("Review finish precedes its start")
        if (index == 0) != (phase == "initial"):
            raise ValueError("A stage starts with exactly one initial review")
        if phase == "closing" and (previous is None or previous["phase"] != "verification"
                                   or previous["outcome"] != "approved"
                                   or previous["revision"] != review["revision"]):
            raise ValueError("Closing requires immediately preceding approved verification of its revision")
        previous = review


def stage_complete(stage: dict | None) -> bool:
    """Require current closing and preceding verification on the same packet."""
    if stage is None or len(stage["reviews"]) < 3:
        return False
    prior, last = stage["reviews"][-2:]
    return (last["phase"] == "closing" and last["outcome"] == "approved"
            and last["revision"] == stage["revision"]
            and prior["phase"] == "verification" and prior["outcome"] == "approved"
            and prior["revision"] == stage["revision"])


def validate_publication(publication: object, task: dict, root: Path, evidence_path) -> None:
    publication = required(publication, {"state", "pr", "head", "merge_commit", "merged_at", "observed_at"},
                           "publication")
    url = https(publication["pr"], "publication.pr", evidence_path, root)
    if url not in task["evidence"]:
        raise ValueError("Publication PR must occur in task evidence")
    commit(publication["head"], "publication.head")
    observed = instant(publication["observed_at"], "publication.observed_at")
    if publication["state"] == "open":
        if publication["merge_commit"] is not None or publication["merged_at"] is not None:
            raise ValueError("Open publication must have null merge fields")
    elif publication["state"] == "merged":
        commit(publication["merge_commit"], "publication.merge_commit")
        if instant(publication["merged_at"], "publication.merged_at") > observed:
            raise ValueError("Publication was observed before its merge")
    else:
        raise ValueError("Unknown publication state")


def validate_task(task: dict, policy: dict | None, root: Path, local_path, evidence_path, *,
                  tree_free: bool = False) -> None:
    """Validate supplied records and enforce forward claim/completion gates."""
    is_tracked = tracked(task, policy)
    if is_tracked and task["status"] in {"in_progress", "in_review", "done"} and "workflow" not in task:
        raise ValueError(f"{task['id']}: claimed task requires workflow records")
    if "workflow" in task:
        try:
            validate_workflow(task["workflow"], task["kind"], root, local_path, evidence_path, tree_free=tree_free)
        except ValueError as error:
            raise ValueError(f"{task['id']}: {error}") from error
        names = task["workflow"]["required_stages"]
        stages = task["workflow"]["stages"]
        if is_tracked and task["status"] == "done" and not all(stage_complete(stages.get(name)) for name in names):
            raise ValueError(f"{task['id']}: done requires every stage's current closing approval")
    if "publication" in task:
        validate_publication(task["publication"], task, root, evidence_path)
    elif is_tracked and task["status"] == "done":
        raise ValueError(f"{task['id']}: done requires a publication observation")


def stage_context(task: dict, name: str) -> dict:
    """Derive one handoff and observed metrics from validated receipt order."""
    workflow = task.get("workflow")
    if workflow is None or name not in workflow["required_stages"]:
        raise ValueError(f"{task['id']}: unknown workflow stage: {name}")
    stage = workflow["stages"].get(name)
    complete = stage_complete(stage)
    next_gate = "authoring" if stage is None else "initial"
    if stage is not None and stage["reviews"]:
        last = stage["reviews"][-1]
        next_gate = ("complete" if complete else "closing" if last["phase"] == "verification"
                     and last["outcome"] == "approved" and last["revision"] == stage["revision"]
                     else "verification")
    reviews = stage["reviews"] if stage is not None else []
    blockers, closing_only, seen = [], [], set()
    elapsed, timed = 0.0, 0
    for review in reviews:
        if review["phase"] == "closing":
            closing_only.extend(finding for finding in review["findings"] if finding not in seen)
        seen.update(review["findings"])
        blockers.extend(finding for finding in review["blocking_findings"] if finding not in blockers)
        if "started_at" in review:
            elapsed += (instant(review["finished_at"], "finished_at")
                        - instant(review["started_at"], "started_at")).total_seconds()
            timed += 1
    return {"name": name, "packet": stage, "complete": complete, "next_gate": next_gate,
            "metrics": {"review_count": len(reviews), "blocking_findings": blockers,
                        "closing_only_findings": closing_only, "elapsed_seconds": elapsed if timed else None,
                        "timed_reviews": timed}}


def reconcile(data: dict, task_id: str, snapshot: object, observed_at: str,
              root: Path, evidence_path) -> None:
    """Import an operator-supplied observation without changing work or review claims."""
    task = next((task for task in data["tasks"] if task["id"] == task_id), None)
    if task is None:
        raise ValueError(f"Unknown task: {task_id}")
    snapshot = required(snapshot, {"url", "state", "headRefOid", "mergedAt", "mergeCommit"}, "PR snapshot")
    if not isinstance(snapshot["state"], str) or snapshot["state"] not in {"OPEN", "MERGED"}:
        raise ValueError("PR snapshot must be OPEN or MERGED")
    merge_commit = snapshot["mergeCommit"]
    if snapshot["state"] == "MERGED":
        merge_commit = required(merge_commit, {"oid"}, "mergeCommit")["oid"]
    publication = {"state": snapshot["state"].lower(), "pr": snapshot["url"], "head": snapshot["headRefOid"],
                   "merge_commit": merge_commit, "merged_at": snapshot["mergedAt"], "observed_at": observed_at}
    validate_publication(publication, task, root, evidence_path)
    old = task.get("publication")
    if old is not None:
        if old["pr"] != publication["pr"]:
            raise ValueError("Publication PR reassociation requires a reviewed metadata correction")
        if old["state"] == "merged":
            if (publication["state"] != "merged"
                    or any(old[key] != publication[key] for key in ("head", "merge_commit"))
                    or instant(old["merged_at"], "merged_at") != instant(publication["merged_at"], "merged_at")):
                raise ValueError("Merged publication fields are immutable")
            publication["merged_at"] = old["merged_at"]
    task["publication"] = publication
    data["updated_at"] = observed_at


def replace_outputs(state: Path, state_text: str, output: Path, html_text: str) -> None:
    """Preserve existing modes; prepare both files before non-pairwise-atomic replacement.

    Newly created outputs retain the temporary file's private mode.
    """
    prepared = []
    try:
        for destination, content in ((state, state_text), (output, html_text)):
            try:
                mode = stat.S_IMODE(destination.stat().st_mode)
            except FileNotFoundError:
                mode = None
            with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="\n", delete=False,
                                             dir=destination.parent, prefix=f".{destination.name}.") as stream:
                temporary = Path(stream.name)
                prepared.append((temporary, destination))
                stream.write(content)
            if mode is not None:
                temporary.chmod(mode)
        for temporary, destination in prepared:
            os.replace(temporary, destination)
    finally:
        for temporary, _ in prepared:
            temporary.unlink(missing_ok=True)


def compact_stage_context(stage: dict) -> dict:
    """Project an already derived stage without repeating its packet or finding prose."""
    packet = stage["packet"]
    ref_fields = ("phase", "provider", "model", "reviewer", "revision", "outcome", "evidence")
    return {
        "name": stage["name"],
        "complete": stage["complete"],
        "next_gate": stage["next_gate"],
        "packet": ({key: packet[key] for key in ("authors", "revision", "paths")}
                   if packet is not None else None),
        "review_refs": [{key: review[key] for key in ref_fields}
                        for review in packet["reviews"]] if packet is not None else [],
        "historical_blocking_findings": stage["metrics"]["blocking_findings"],
        "metrics": {key: stage["metrics"][key]
                    for key in ("review_count", "elapsed_seconds", "timed_reviews")},
    }
