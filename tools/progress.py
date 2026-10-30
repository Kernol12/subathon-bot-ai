#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate agent task records and render an offline, deterministic HTML snapshot."""
from __future__ import annotations

import argparse
import heapq
import html
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import quote

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / "progress/state.json"
OUTPUT = ROOT / "PROGRESS.html"
STATUSES = {"planned", "ready", "in_progress", "in_review", "blocked", "done"}
CHECKS = {"not_run", "draft", "passed", "failed", "observed_red", "partial", "blocked"}
KINDS = {"scaffold", "environment", "contract", "test_draft", "test_review", "test_baseline", "implementation", "code_review", "feasibility", "epic"}


def local_path(value: str, root: Path = ROOT) -> Path:
    if not isinstance(value, str) or not value or "\\" in value or ":" in value:
        raise ValueError(f"Expected a project-relative path: {value!r}")
    path = Path(value)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError(f"Path escapes the project: {value!r}")
    resolved = (root / path).resolve()
    if not resolved.is_relative_to(root.resolve()):
        raise ValueError(f"Path escapes the project: {value!r}")
    return resolved


def validate(data: dict, root: Path = ROOT) -> None:
    if data.get("schema_version") != 1:
        raise ValueError("Unsupported progress schema_version")
    for key in ("project", "baseline_version", "specification_version", "updated_at", "summary"):
        if not isinstance(data.get(key), str) or not data[key].strip():
            raise ValueError(f"Missing/non-text field: {key}")
    if not isinstance(data.get("tasks"), list) or not data["tasks"]:
        raise ValueError("At least one task is required")
    by_id = {}
    for task in data["tasks"]:
        task_id = task.get("id", "")
        if not isinstance(task_id, str) or not re.fullmatch(r"[A-Z][A-Z0-9]*-\d{3}", task_id) or task_id in by_id:
            raise ValueError(f"Invalid/duplicate task ID: {task_id}")
        by_id[task_id] = task
        for key in ("title", "module", "owner", "summary"):
            if not isinstance(task.get(key), str) or not task[key].strip():
                raise ValueError(f"{task_id}: missing/non-text {key}")
        if task.get("status") not in STATUSES or task.get("kind") not in KINDS:
            raise ValueError(f"{task_id}: unknown status/kind")
        for key in ("depends_on", "spec_refs", "notes", "evidence"):
            if not isinstance(task.get(key), list) or any(not isinstance(x, str) for x in task[key]):
                raise ValueError(f"{task_id}: {key} must be an array of strings")
        for value in [task.get("brief"), *task["evidence"]]:
            if not local_path(value, root).is_file():
                raise ValueError(f"{task_id}: missing local document: {value}")
        if not isinstance(task.get("verification"), dict) or any(v not in CHECKS for v in task["verification"].values()):
            raise ValueError(f"{task_id}: invalid verification result")
        if task["status"] == "done":
            if not task["evidence"]:
                raise ValueError(f"{task_id}: done requires evidence")
            if task["kind"] in {"implementation", "test_review", "code_review", "test_baseline"}:
                review = task.get("review") or {}
                if (not review.get("author") or not review.get("reviewer")
                        or review["author"] == review["reviewer"] or review.get("outcome") != "approved"):
                    raise ValueError(f"{task_id}: done requires a recorded separate approved review")
            if task["kind"] == "implementation" and task["verification"].get("acceptance") != "passed":
                raise ValueError(f"{task_id}: implementation done requires passing acceptance evidence")
            if task["kind"] == "test_baseline" and task["verification"].get("baseline") != "observed_red":
                raise ValueError(f"{task_id}: baseline done requires observed_red evidence")
    visiting, visited = set(), set()

    def visit(task_id: str) -> None:
        if task_id in visiting:
            raise ValueError(f"Task dependency cycle at {task_id}")
        if task_id in visited:
            return
        visiting.add(task_id)
        task = by_id[task_id]
        for dependency in task["depends_on"]:
            if dependency not in by_id:
                raise ValueError(f"{task_id}: unknown dependency {dependency}")
            visit(dependency)
            if task["status"] in {"ready", "in_progress", "in_review", "done"} and by_id[dependency]["status"] != "done":
                raise ValueError(f"{task_id}: dependency {dependency} is not done")
        visiting.remove(task_id)
        visited.add(task_id)

    for task_id in by_id:
        visit(task_id)
    if not isinstance(data.get("verification"), list) or not isinstance(data.get("activity"), list):
        raise ValueError("Verification/activity must be arrays")
    for item in data["verification"]:
        if item.get("status") not in CHECKS or not isinstance(item.get("label"), str) or not isinstance(item.get("detail"), str):
            raise ValueError("Invalid overall verification record")
    for item in data["activity"]:
        if item.get("task") not in by_id or not all(isinstance(item.get(k), str) for k in ("date", "note")):
            raise ValueError("Invalid activity task/date/note")


def esc(value: object) -> str:
    return html.escape(str(value), quote=True)


def label(value: str) -> str:
    return value.replace("_", " ")


def badge(value: str) -> str:
    return f'<span class="badge {esc(value)}">{esc(label(value))}</span>'


def link(path: str, title: str) -> str:
    return f'<a href="{quote(path, safe="/._-")}">{esc(title)}</a>'


def execution_order(tasks: list[dict]) -> list[dict]:
    """Put prerequisites first, breaking ties by source order in the validated DAG."""
    positions = {task["id"]: i for i, task in enumerate(tasks)}
    remaining = [len(task["depends_on"]) for task in tasks]
    dependents = [[] for _ in tasks]
    for i, task in enumerate(tasks):
        for dependency in task["depends_on"]:
            dependents[positions[dependency]].append(i)
    ready = [i for i, count in enumerate(remaining) if count == 0]
    heapq.heapify(ready)
    ordered = []
    while ready:
        i = heapq.heappop(ready)
        ordered.append(tasks[i])
        for dependent in dependents[i]:
            remaining[dependent] -= 1
            if remaining[dependent] == 0:
                heapq.heappush(ready, dependent)
    if len(ordered) != len(tasks):
        raise ValueError("Task dependency cycle prevents execution ordering")
    return ordered


def render(data: dict) -> str:
    tasks = data["tasks"]
    by_id = {t["id"]: t for t in tasks}
    source_positions = {t["id"]: i for i, t in enumerate(tasks)}
    ordered = execution_order(tasks)
    available = {t["id"] for t in tasks
                 if t["kind"] != "epic" and t["status"] in {"planned", "ready"}
                 and all(by_id[d]["status"] == "done" for d in t["depends_on"])}
    done = sum(t["status"] == "done" for t in tasks)
    concrete = sum(t["kind"] != "epic" for t in tasks)
    blocked = sum(t["status"] == "blocked" for t in tasks)
    implemented = sum(t["kind"] == "implementation" and t["status"] == "done" for t in tasks)
    rows = []
    for position, task in enumerate(ordered):
        dependencies = ", ".join(
            f'<a href="#{esc(i)}">{esc(i)}</a> {badge(by_id[i]["status"])}' for i in task["depends_on"]
        ) or "No prerequisites"
        notes = "".join(f"<li>{esc(n)}</li>" for n in task["notes"]) or "<li>No task notes yet.</li>"
        evidence = " · ".join(link(p, Path(p).name) for p in task["evidence"]) or "No verification evidence recorded"
        checks = " ".join(f'<span class="check">{esc(label(k))} {badge(v)}</span>' for k, v in task["verification"].items()) or "No check outcome recorded"
        review = task.get("review")
        review_text = (f'{review.get("author", "?")} → {review.get("reviewer", "?")}: {review.get("outcome", "?")}' if review else "Not recorded")
        search = " ".join([task["id"], task["title"], task["module"], task["owner"], task["summary"], *task["notes"]]).lower()
        pickup = '<span class="badge available">Dependencies complete</span>' if task["id"] in available else ""
        rows.append(f'''<article id="{esc(task['id'])}" class="task" data-status="{esc(task['status'])}" data-module="{esc(task['module'])}" data-search="{esc(search)}" data-epic-order="{source_positions[task['id']]}" data-execution-order="{position}" data-available="{str(task['id'] in available).lower()}">
<details><summary><div class="task-heading"><span class="task-id">{esc(task['id'])}</span><span class="task-title">{esc(task['title'])}</span></div><div class="task-meta"><span>{esc(task['module'])}</span><span class="kind">{esc(label(task['kind']))}</span>{badge(task['status'])}{pickup}<span class="chevron" aria-hidden="true">⌄</span></div></summary>
<div class="task-detail"><p>{esc(task['summary'])}</p><div class="detail-grid"><div><h4>Ownership</h4><p>{esc(task['owner'])}</p><h4>Specification</h4><p>{esc(', '.join(task['spec_refs']))}</p></div><div><h4>Dependencies</h4><p>{dependencies}</p><h4>Independent review</h4><p>{esc(review_text)}</p></div></div><h4>Notes</h4><ul>{notes}</ul><h4>Verification</h4><p class="checks">{checks}</p><p class="evidence">{evidence}</p><p>{link(task['brief'], 'Open task brief ↗')}</p></div></details></article>''')
    modules = sorted({t["module"] for t in tasks})
    module_options = "".join(f'<option value="{esc(m)}">{esc(m)}</option>' for m in modules)
    status_options = "".join(f'<option value="{s}">{esc(label(s))}</option>' for s in sorted(STATUSES))
    checks_html = "".join(f'<div class="verification-item"><div><strong>{esc(v["label"])}</strong>{badge(v["status"])}</div><p>{esc(v["detail"])}</p></div>' for v in data["verification"])
    activity = "".join(f'<li><time>{esc(a["date"])}</time><div><a href="#{esc(a["task"])}">{esc(a["task"])}</a><p>{esc(a["note"])}</p></div></li>' for a in reversed(data["activity"][-12:]))
    first = next((t for t in ordered if t["id"] in available), None)
    next_task = f'{esc(first["id"])} · {esc(first["title"])}' if first else "No tasks ready to pick up"
    template = (ROOT / "tools/progress_template.html").read_text(encoding="utf-8")
    fields = {
        "PROJECT": esc(data["project"]), "BASELINE": esc(data["baseline_version"]),
        "SPEC_VERSION": esc(data["specification_version"]), "UPDATED": esc(data["updated_at"]),
        "SUMMARY": esc(data["summary"]), "DONE": str(done), "TOTAL": str(len(tasks)),
        "CONCRETE": str(concrete), "EPICS": str(len(tasks) - concrete), "READY": str(len(available)),
        "BLOCKED": str(blocked), "IMPLEMENTED": str(implemented), "NEXT_TASK": next_task,
        "MODULE_OPTIONS": module_options, "STATUS_OPTIONS": status_options,
        "TASK_ROWS": "\n".join(rows), "VERIFICATION": checks_html, "ACTIVITY": activity,
    }
    # Single substitution pass: editable notes containing template-like text must
    # remain literal rather than being reinterpreted as another template field.
    return re.sub(r"@@([A-Z_]+)@@", lambda match: fields[match[1]], template)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("validate")
    rendering = sub.add_parser("render")
    rendering.add_argument("--check", action="store_true", help="Fail if the HTML snapshot is stale")
    setting = sub.add_parser("set")
    setting.add_argument("task_id")
    setting.add_argument("--status", choices=sorted(STATUSES))
    setting.add_argument("--owner")
    setting.add_argument("--note", required=True)
    args = parser.parse_args()
    try:
        data = json.loads(STATE.read_text(encoding="utf-8"))
        validate(data)
        if args.command == "set":
            task = next((t for t in data["tasks"] if t["id"] == args.task_id), None)
            if task is None:
                raise ValueError(f"Unknown task: {args.task_id}")
            if args.status:
                task["status"] = args.status
            if args.owner:
                task["owner"] = args.owner
            task["notes"].append(args.note)
            date = datetime.now(timezone.utc).isoformat(timespec="seconds")
            data["updated_at"] = date
            data["activity"].append({"date": date, "task": args.task_id, "note": args.note})
            validate(data)
            STATE.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        if args.command in {"render", "set"}:
            output = render(data)
            if getattr(args, "check", False):
                if not OUTPUT.is_file() or OUTPUT.read_text(encoding="utf-8") != output:
                    raise ValueError("PROGRESS.html is stale; run python3 tools/progress.py render")
                print("Progress snapshot is current")
            else:
                OUTPUT.write_text(output, encoding="utf-8")
                print("Updated PROGRESS.html")
        else:
            print(f"Valid progress: {len(data['tasks'])} task records")
        return 0
    except (OSError, ValueError, TypeError, KeyError) as error:
        print(f"Progress error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
