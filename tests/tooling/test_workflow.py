# SPDX-License-Identifier: GPL-3.0-or-later
"""Literal WORKFLOW-003 record/CLI examples, independent of implementation helpers."""
import copy
import hashlib
from contextlib import redirect_stderr, redirect_stdout
from html.parser import HTMLParser
from io import StringIO
import json
from pathlib import Path
import subprocess
import socket
import sys
import tempfile
import unittest
import urllib.request
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import progress

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


class Articles(HTMLParser):
    def __init__(self, source):
        super().__init__()
        self.rows = {}
        self.feed(source)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == "article" and "id" in attrs:
            self.rows[attrs["id"]] = attrs


class WorkflowRecordTests(unittest.TestCase):
    def test_policy_requires_new_claim_records_without_fabricating_legacy_reviews(self):
        historical = task("OLD-999", status="done")
        del historical["workflow"]
        historical["review"] = None  # Explicit grandfathering preserves missing old receipts.
        data = state(historical, task())
        exempt(data, "OLD-999")
        progress.validate(data)
        original = copy.deepcopy(data)
        for status in ("in_progress", "in_review", "done"):
            with self.subTest(status=status):
                candidate = copy.deepcopy(original)
                candidate["tasks"][1]["status"] = status
                if status == "done":
                    candidate["tasks"][1]["publication"] = publication()
                del candidate["tasks"][1]["workflow"]
                with self.assertRaises(ValueError):
                    progress.validate(candidate)
        for exemptions in (["OLD-999", "OLD-999"], ["OLD-999", "UNKNOWN-999"], ["OLD-999", "WFLOW-999"]):
            with self.subTest(exemptions=exemptions):
                candidate = copy.deepcopy(original)
                exempt(candidate, *exemptions)
                with self.assertRaises(ValueError):
                    progress.validate(candidate)
        unsupported = copy.deepcopy(original)
        unsupported["workflow_policy"]["version"] = 2
        with self.assertRaises(ValueError):
            progress.validate(unsupported)
        altered = copy.deepcopy(original)
        altered["workflow_policy"]["legacy_done_sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            progress.validate(altered)
        for status in ("planned", "ready", "blocked"):
            optional = task(status=status)
            del optional["workflow"]
            progress.validate(state(optional))
            optional["workflow"] = {"required_stages": ["contract"], "stages": {"contract": stage()}}
            optional["workflow"]["stages"]["contract"]["authors"] = []
            with self.subTest(optional_status=status), self.assertRaises(ValueError):
                progress.validate(state(optional))
        epic = task(kind="epic")
        del epic["workflow"]
        progress.validate(state(epic))
        legacy_fixture = state(historical)
        del legacy_fixture["workflow_policy"]
        legacy_fixture["tasks"][0]["kind"] = "scaffold"
        progress.validate(legacy_fixture)

        # Historical prerequisites need no manufactured publication observation.
        child = task("CHILD-999", status="ready")
        child["depends_on"] = [historical["id"]]
        inherited = state(historical, child)
        exempt(inherited, historical["id"])
        for status in ("ready", "in_progress"):
            child["status"] = status
            progress.validate(inherited)
        child["status"] = "ready"
        self.assertTrue(progress.context(inherited, child["id"])["available"])
        self.assertEqual(Articles(progress.render(inherited)).rows[child["id"]]["data-available"], "true")

    def test_nonexempt_contract_done_requires_separate_approved_review(self):
        for policy in (False, True):
            for review in (None, {"author": "same", "reviewer": "same", "outcome": "approved"},
                           {"author": "one", "reviewer": "two", "outcome": "changes_requested"}):
                with self.subTest(policy=policy, review=review):
                    done = task(status="done")
                    done.update(review=review, publication=publication())
                    data = state(done)
                    if not policy:
                        del data["workflow_policy"]
                    with self.assertRaises(ValueError):
                        progress.validate(data)

    def test_kind_minimum_stages_and_packet_shape_are_enforced(self):
        for kind, names in (("contract", ["contract"]), ("test_draft", ["tests"]),
                            ("test_review", ["tests"]), ("test_baseline", ["tests"]),
                            ("implementation", ["tests", "implementation"]), ("cleanup", ["implementation"])):
            good = task(kind=kind)
            progress.validate(state(good))
            with self.subTest(kind=kind):
                good["workflow"] = {"required_stages": ["tests"] if names == ["contract"] else ["contract"],
                                    "stages": {"tests" if names == ["contract"] else "contract": stage()}}
                with self.assertRaises(ValueError):
                    progress.validate(state(good))
        for key, value in (("authors", []), ("authors", ["author", "author"]),
                           ("revision", "B" * 40), ("revision", "b" * 39),
                           ("paths", []), ("paths", ["../outside"]),
                           ("paths", ["docs/no-such-workflow-file.md"]),
                           ("paths", ["docs/tasks/WORKFLOW-001.md"] * 2)):
            with self.subTest(field=key, value=value):
                candidate = task()
                candidate["workflow"]["stages"]["contract"][key] = value
                with self.assertRaises(ValueError):
                    progress.validate(state(candidate))
        for workflow in ({"required_stages": [], "stages": {}},
                         {"required_stages": ["contract", "contract"], "stages": {"contract": stage()}},
                         {"required_stages": ["contract"], "stages": {"implementation": stage()}},
                         {"required_stages": ["contract", "unknown"], "stages": {"contract": stage(), "unknown": stage()}}):
            with self.subTest(workflow=workflow), self.assertRaises(ValueError):
                candidate = task()
                candidate["workflow"] = workflow
                progress.validate(state(candidate))
        candidate = task(kind="implementation")
        candidate["workflow"] = {"required_stages": ["implementation"], "stages": {"implementation": stage()}}
        with self.assertRaises(ValueError):
            progress.validate(state(candidate))
        extra = task()
        extra["workflow"]["stages"]["tests"] = stage()
        with self.assertRaises(ValueError):
            progress.validate(state(extra))
        extra["workflow"]["required_stages"].append("tests")
        progress.validate(state(extra))  # Additional declared stages are allowed.
        unstarted = task(kind="implementation")
        unstarted["workflow"]["stages"] = {}
        progress.validate(state(unstarted))  # Future authors/paths/revisions are not invented.

    def test_review_order_requires_initial_verification_and_fresh_closing_cycles(self):
        rejected = receipt("closing", outcome="changes_requested", findings=["B1"], blocking_findings=["B1"])
        unapproved = receipt("verification", outcome="changes_requested", findings=["B1"], blocking_findings=["B1"])
        invalid = [
            [receipt("verification")], [receipt("initial"), receipt("closing")],
            [receipt("initial"), receipt("initial")],
            [receipt("initial"), unapproved, receipt("closing")],
            [receipt("initial"), receipt("verification"), unapproved, receipt("closing")],
            [receipt("initial"), receipt("verification", OLD_REV), receipt("closing")],
            [receipt("initial"), receipt("verification"), rejected, receipt("closing")],
            [receipt("initial"), receipt("verification"), receipt("closing"), receipt("closing")],
        ]
        for reviews in invalid:
            with self.subTest(phases=[r["phase"] for r in reviews]):
                candidate = task()
                candidate["workflow"]["stages"]["contract"] = stage(reviews)
                with self.assertRaises(ValueError):
                    progress.validate(state(candidate))
        for reviews in ([receipt("initial"), receipt("verification"), rejected,
                         receipt("verification"), receipt("closing")],
                        [receipt("initial"), receipt("verification"), receipt("closing"),
                         receipt("verification"), receipt("closing")],
                        [receipt("initial"), unapproved, receipt("verification"), receipt("closing")]):
            candidate = task(status="done")
            candidate["publication"] = publication()
            candidate["workflow"]["stages"]["contract"] = stage(reviews)
            progress.validate(state(candidate))

    def test_receipt_independence_evidence_findings_and_provider_are_validated(self):
        bad_receipts = [
            receipt("initial", provider="codex"), receipt("verification", provider="claude"),
            receipt("closing", provider="codex"), receipt("initial", reviewer="coauthor-session"),
            receipt("initial", model=""), receipt("initial", reviewer=""),
            receipt("initial", revision="short"), receipt("initial", evidence="http://example.test/review"),
            receipt("initial", findings=["B1", "B1"]),
            receipt("initial", blocking_findings=["B1"]),
            receipt("initial", outcome="changes_requested", blocking_findings=["B1"]),
            receipt("initial", outcome="changes_requested", findings=["B1"], blocking_findings=["B1", "B1"]),
            receipt("initial", findings=[1]),
            receipt("initial", findings=["B1"], blocking_findings=["B1"]),
            receipt("initial", outcome="changes_requested"),
            receipt("verification", reviewer="author-session"),
            receipt("closing", reviewer="coauthor-session"),
        ]
        for invalid in bad_receipts:
            with self.subTest(receipt=invalid):
                reviews = [receipt("initial"), receipt("verification"), receipt("closing")]
                reviews[{"initial": 0, "verification": 1, "closing": 2}[invalid["phase"]]] = invalid
                candidate = task()
                candidate["workflow"]["stages"]["contract"] = stage(reviews)
                with self.assertRaises(ValueError):
                    progress.validate(state(candidate))

    def test_review_times_require_observed_ordered_timezone_pairs(self):
        for times in ({"started_at": "2026-11-11T12:00:00Z"},
                      {"finished_at": "2026-11-11T12:00:00Z"},
                      {"started_at": "2026-11-11T12:00:00", "finished_at": "2026-11-11T12:01:00"},
                      {"started_at": "2026-11-11T12:02:00Z", "finished_at": "2026-11-11T12:01:00Z"},
                      {"started_at": "not-a-time", "finished_at": "not-a-time"}):
            with self.subTest(times=times):
                candidate = task()
                candidate["workflow"]["stages"]["contract"] = stage([receipt("initial", **times)])
                with self.assertRaises(ValueError):
                    progress.validate(state(candidate))
        candidate = task()
        candidate["workflow"]["stages"]["contract"] = stage([receipt("initial",
            started_at="2026-11-11T13:00:00+01:00", finished_at="2026-11-11T12:01:00Z")])
        progress.validate(state(candidate))

    def test_done_requires_complete_current_stages_and_publication(self):
        for reviews in ([], [receipt("initial")], [receipt("initial"), receipt("verification")],
                        [receipt("initial"), receipt("verification", OLD_REV), receipt("closing", OLD_REV)],
                        [receipt("initial"), receipt("verification"), receipt("closing"), receipt("verification")]):
            with self.subTest(reviews=reviews):
                candidate = task(status="done")
                candidate["publication"] = publication()
                candidate["workflow"]["stages"]["contract"] = stage(reviews)
                with self.assertRaises(ValueError):
                    progress.validate(state(candidate))
        with self.assertRaises(ValueError):
            progress.validate(state(task(status="done")))
        done = task(status="done")
        done["publication"] = publication("open")
        progress.validate(state(done))  # Artifact completion does not falsely imply merge.
        multiple = task(kind="implementation", status="done")
        multiple["publication"] = publication()
        multiple["workflow"]["stages"]["implementation"] = stage([])
        with self.assertRaises(ValueError):
            progress.validate(state(multiple))
        del multiple["workflow"]["stages"]["implementation"]
        with self.assertRaises(ValueError):
            progress.validate(state(multiple))

    def test_publication_shape_and_dependency_readiness_match_context_and_html(self):
        done = task(status="done")
        done["publication"] = publication("open")
        waiting = task("WFLOW-998", status="planned")
        waiting["depends_on"] = [done["id"]]
        data = state(done, waiting)
        progress.validate(data)
        self.assertFalse(progress.context(data, waiting["id"])["available"])
        self.assertEqual(Articles(progress.render(data)).rows[waiting["id"]]["data-available"], "false")
        waiting["status"] = "ready"
        with self.assertRaises(ValueError):
            progress.validate(data)
        done["publication"] = publication()
        progress.validate(data)
        self.assertTrue(progress.context(data, waiting["id"])["available"])
        self.assertEqual(Articles(progress.render(data)).rows[waiting["id"]]["data-available"], "true")
        for key, value in (("state", "closed"), ("pr", "https://example.test/other/pull/1"),
                           ("head", "short"), ("merge_commit", None), ("merged_at", None),
                           ("observed_at", "2026-11-11T11:59:59Z"),
                           ("observed_at", "2026-11-11T12:01:00"),
                           ("pr", "http://example.test/project/pull/42"),
                           ("merged_at", "2026-11-11T12:00:00")):
            with self.subTest(field=key, value=value):
                candidate = task()
                candidate["publication"] = publication()
                candidate["publication"][key] = value
                with self.assertRaises(ValueError):
                    progress.validate(state(candidate))
        equal = task()
        equal["publication"] = publication()
        equal["publication"]["observed_at"] = equal["publication"]["merged_at"]
        progress.validate(state(equal))
        for key, value in (("merge_commit", MERGE), ("merged_at", "2026-11-11T12:00:00Z")):
            candidate = task()
            candidate["publication"] = publication("open")
            candidate["publication"][key] = value
            with self.subTest(open_field=key), self.assertRaises(ValueError):
                progress.validate(state(candidate))


class WorkflowCliTests(unittest.TestCase):
    def invoke(self, data, command, snapshot=None, render_error=False, serialization_error=False):
        """Use disposable files; normalize argparse refusal into an observable exit code."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            state_path, html_path = root / "state.json", root / "PROGRESS.html"
            state_path.write_text(json.dumps(data, indent=3), encoding="utf-8")
            html_path.write_bytes(b"Previous HTML bytes")
            (root / "coordinator-checkpoint.md").write_bytes(b"Original authorization checkpoint")
            if snapshot is not None:
                (root / "snapshot.json").write_text(json.dumps(snapshot), encoding="utf-8")
            before = {p.name: p.read_bytes() for p in root.iterdir()}
            argv = ["progress.py", *command]
            if snapshot is not None:
                argv += ["--pr-json", str(root / "snapshot.json")]
            stdout, stderr = StringIO(), StringIO()
            real_paths = [ROOT / p for p in ("progress/state.json", "PROGRESS.html", "progress/coordinator-checkpoint.md")]
            real_before = [hashlib.sha256(p.read_bytes()).digest() for p in real_paths]
            with patch.object(progress, "STATE", state_path), patch.object(progress, "OUTPUT", html_path), \
                    patch.object(sys, "argv", argv), redirect_stdout(stdout), redirect_stderr(stderr), \
                    patch.object(subprocess, "run", side_effect=AssertionError("unexpected process")), \
                    patch.object(subprocess, "Popen", side_effect=AssertionError("unexpected process")), \
                    patch.object(socket, "socket", side_effect=AssertionError("unexpected network")), \
                    patch.object(urllib.request, "urlopen", side_effect=AssertionError("unexpected network")):
                def call():
                    try:
                        return progress.main()
                    except SystemExit as error:
                        return error.code
                if render_error:
                    # progress.render is the CLI's output-rendering seam, before writes.
                    with patch.object(progress, "render", side_effect=ValueError("render refused")):
                        result = call()
                elif serialization_error:
                    with patch.object(progress.json, "dumps", side_effect=ValueError("serialization refused")):
                        result = call()
                else:
                    result = call()
            after = {p.name: p.read_bytes() for p in root.iterdir()}
            self.assertEqual(real_before, [hashlib.sha256(p.read_bytes()).digest() for p in real_paths])
            return result, stdout.getvalue(), stderr.getvalue(), before, after

    def context(self, reviews, revision=REV):
        selected = task()
        selected["workflow"]["stages"]["contract"] = stage(reviews)
        selected["workflow"]["stages"]["contract"]["revision"] = revision
        result, stdout, stderr, before, after = self.invoke(state(selected), ["context", "WFLOW-999", "--stage", "contract"])
        self.assertEqual(result, 0, stderr)
        self.assertEqual(stderr, "")
        self.assertEqual(before, after, "Stage context must remain read-only")
        return json.loads(stdout), selected

    def test_stage_context_derives_packet_next_gate_and_observed_metrics(self):
        reviews = [
            receipt("initial", OLD_REV, outcome="changes_requested", findings=["B1"], blocking_findings=["B1"],
                    started_at="2026-11-11T12:00:00Z", finished_at="2026-11-11T12:00:10Z"),
            receipt("verification", findings=["N1"], started_at="2026-11-11T12:00:15Z", finished_at="2026-11-11T12:00:20Z"),
            receipt("closing", outcome="changes_requested", findings=["B1", "B2"], blocking_findings=["B2"]),
            receipt("verification", findings=["N2"]), receipt("closing", findings=["B2", "N3"]),
        ]
        output, selected = self.context(reviews)
        view = output["stage_context"]
        self.assertEqual({key: view[key] for key in ("name", "packet", "complete", "next_gate")},
                         {"name": "contract", "packet": selected["workflow"]["stages"]["contract"],
                          "complete": True, "next_gate": "complete"})
        metrics = view["metrics"]
        self.assertEqual(metrics["review_count"], 5)
        self.assertEqual(metrics["blocking_findings"], ["B1", "B2"])
        self.assertEqual(metrics["closing_only_findings"], ["B2", "N3"])
        self.assertEqual(metrics["elapsed_seconds"], 15)
        self.assertEqual(metrics["timed_reviews"], 2)
        self.assertIn("recorded", output["limitations"].lower())
        self.assertIn("not verified", output["limitations"].lower())
        for history, revision, expected in (([], REV, "initial"), ([receipt("initial")], REV, "verification"),
                ([receipt("initial"), receipt("verification")], REV, "closing"),
                ([receipt("initial"), receipt("verification", outcome="changes_requested", findings=["C1"], blocking_findings=["C1"])], REV, "verification"),
                (reviews[:3], REV, "verification"),
                ([receipt("initial"), receipt("verification"), receipt("closing"), receipt("verification")], REV, "closing"),
                ([receipt("initial"), receipt("verification"), receipt("closing")], OLD_REV, "verification")):
            with self.subTest(expected=expected):
                output, _ = self.context(history, revision)
                view = output["stage_context"]
                self.assertEqual(view["next_gate"], expected)
                self.assertFalse(view["complete"])
                if history != reviews[:3]:
                    self.assertEqual(view["metrics"]["timed_reviews"], 0)
                    self.assertIsNone(view["metrics"]["elapsed_seconds"])
        pending = task()
        pending["workflow"]["stages"] = {}
        result, stdout, stderr, before, after = self.invoke(state(pending), ["context", "WFLOW-999", "--stage", "contract"])
        self.assertEqual(result, 0, stderr)
        self.assertEqual(before, after)
        view = json.loads(stdout)["stage_context"]
        self.assertEqual((view["packet"], view["complete"], view["next_gate"]), (None, False, "authoring"))

    def test_default_context_keeps_existing_shape_and_unknown_stage_refuses_read_only(self):
        data = state(task())
        result, stdout, stderr, before, after = self.invoke(data, ["context", "WFLOW-999"])
        self.assertEqual(result, 0, stderr)
        self.assertEqual(before, after)
        output = json.loads(stdout)
        self.assertEqual(set(output), {"specification_version", "state_updated_at", "task", "dependencies",
                                      "dependents", "shared_brief_tasks", "available", "limitations"})
        self.assertEqual(output["task"], data["tasks"][0])
        result, stdout, stderr, before, after = self.invoke(data, ["context", "WFLOW-999", "--stage", "tests"])
        self.assertEqual(result, 1)
        self.assertEqual(stdout, "")
        self.assertTrue(stderr.strip())
        self.assertEqual(before, after)

    def test_reconcile_records_only_publication_without_claiming_checks_or_authority(self):
        data = state(task())
        data["tasks"][0]["verification"] = {"github_ci": "not_run"}
        snapshot = {"url": PR, "state": "MERGED", "headRefOid": REV,
                    "mergedAt": "2026-11-11T12:00:00Z", "mergeCommit": {"oid": MERGE}}
        for original_publication in (None, publication("open"), publication()):
            with self.subTest(original_publication=original_publication):
                candidate = copy.deepcopy(data)
                if original_publication is not None:
                    candidate["tasks"][0]["publication"] = original_publication
                result, _, stderr, before, after = self.invoke(candidate, ["reconcile", "WFLOW-999"], snapshot)
                self.assertEqual(result, 0, stderr)
                updated = json.loads(after["state.json"])
                progress.validate(updated)
                self.assertEqual(after["PROGRESS.html"].decode(), progress.render(updated))
                observed = updated["tasks"][0].pop("publication")
                self.assertEqual({k: observed[k] for k in ("state", "pr", "head", "merge_commit", "merged_at")},
                                 {"state": "merged", "pr": PR, "head": REV, "merge_commit": MERGE,
                                  "merged_at": "2026-11-11T12:00:00Z"})
                self.assertTrue(observed["observed_at"])
                original_task = copy.deepcopy(candidate["tasks"][0])
                original_task.pop("publication", None)
                self.assertEqual(updated["tasks"][0], original_task)
                self.assertNotEqual(after["PROGRESS.html"], before["PROGRESS.html"])
                self.assertEqual(after["coordinator-checkpoint.md"], before["coordinator-checkpoint.md"])
                self.assertEqual(set(before), set(after))
        open_snapshot = dict(snapshot, state="OPEN", mergedAt=None, mergeCommit=None)
        result, _, stderr, _, after = self.invoke(data, ["reconcile", "WFLOW-999"], open_snapshot)
        self.assertEqual(result, 0, stderr)
        observed = json.loads(after["state.json"])["tasks"][0]["publication"]
        self.assertEqual(observed["state"], "open")
        self.assertIsNone(observed["merged_at"])
        self.assertIsNone(observed["merge_commit"])

    def test_reconcile_refusals_and_render_failure_preserve_every_file(self):
        data = state(task())
        snapshot = {"url": PR, "state": "MERGED", "headRefOid": REV,
                    "mergedAt": "2026-11-11T12:00:00Z", "mergeCommit": {"oid": MERGE}}
        invalid = [dict(snapshot, url="https://example.test/unrelated/pull/7"),
                   dict(snapshot, url="http://example.test/project/pull/42"),
                   dict(snapshot, state="CLOSED"), dict(snapshot, headRefOid="short"),
                   dict(snapshot, mergedAt=None), dict(snapshot, mergeCommit=None),
                   dict(snapshot, mergeCommit="not-an-object"), dict(snapshot, mergeCommit={"oid": "short"}),
                   dict(snapshot, state="merged"), [], "not-an-object",
                   dict(snapshot, mergedAt="2026-11-11T12:00:00"),
                   dict(snapshot, state="OPEN"),
                   {k: v for k, v in snapshot.items() if k != "headRefOid"}]
        cases = [(copy.deepcopy(data), item, "WFLOW-999", False) for item in invalid]
        already_merged = copy.deepcopy(data)
        already_merged["tasks"][0]["publication"] = publication()
        cases.extend((already_merged, item, "WFLOW-999", False) for item in (
            dict(snapshot, state="OPEN", mergedAt=None, mergeCommit=None),
            dict(snapshot, mergedAt="2026-11-11T11:00:00Z"),
            dict(snapshot, mergeCommit={"oid": OLD_REV}), dict(snapshot, headRefOid=OLD_REV)))
        cases += [(data, snapshot, "WFLOW-404", False), (data, snapshot, "WFLOW-999", True)]
        for candidate, incoming, task_id, render_error in cases:
            with self.subTest(incoming=incoming, task_id=task_id, render_error=render_error):
                result, stdout, stderr, before, after = self.invoke(candidate, ["reconcile", task_id], incoming, render_error)
                self.assertEqual(result, 1)
                self.assertEqual(stdout, "")
                self.assertTrue(stderr.strip())
                self.assertEqual(before, after)
        result, stdout, stderr, before, after = self.invoke(data, ["reconcile", "WFLOW-999"], snapshot, serialization_error=True)
        self.assertEqual(result, 1)
        self.assertEqual(stdout, "")
        self.assertTrue(stderr.strip())
        self.assertEqual(before, after)


if __name__ == "__main__":
    unittest.main()
