# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent literal oracles for context-publication-v1 (WORKFLOW-005).

Authored only: full independent test review must close before import or execution.
"""
import copy
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import progress

FULL_LIMIT = (
    "Recorded information only; live GitHub/CI, session liveness and checkout revisions "
    "are not verified. Availability grants no permission to resume stopped work."
)
COMPACT_LIMIT = FULL_LIMIT + (
    " Compact view omits non-blocking finding details and closing-only finding metrics; "
    "use full mode or review links."
)
BRIEF = "docs/tasks/WORKFLOW-001.md"
OLD, CURRENT = "a" * 40, "b" * 40
# These are invented fixture receipts, never claims of actual independent review.
REFS = [
    {"phase": "initial", "provider": "claude", "model": "synthetic",
     "reviewer": "fixture-initial", "revision": OLD, "outcome": "changes_requested",
     "evidence": "https://example.test/initial"},
    {"phase": "verification", "provider": "codex", "model": "synthetic",
     "reviewer": "fixture-verification-one", "revision": CURRENT, "outcome": "approved",
     "evidence": "https://example.test/verification-one"},
    {"phase": "closing", "provider": "claude", "model": "synthetic",
     "reviewer": "fixture-closing-one", "revision": CURRENT, "outcome": "changes_requested",
     "evidence": "https://example.test/closing-one"},
    {"phase": "verification", "provider": "codex", "model": "synthetic",
     "reviewer": "fixture-verification-two", "revision": CURRENT, "outcome": "approved",
     "evidence": "https://example.test/verification-two"},
    {"phase": "closing", "provider": "claude", "model": "synthetic",
     "reviewer": "fixture-closing-two", "revision": CURRENT, "outcome": "approved",
     "evidence": "https://example.test/closing-two"},
]


def task(task_id, **changes):
    value = {
        "id": task_id, "title": task_id, "module": "Quality and tooling",
        "kind": "scaffold", "status": "planned", "owner": "fixture-author",
        "brief": BRIEF, "summary": "Synthetic scope", "depends_on": [],
        "spec_refs": ["§17"], "notes": [], "verification": {},
        "evidence": [], "review": None,
    }
    value.update(changes)
    return value


def literal_state():
    reviews = copy.deepcopy(REFS)
    for review in reviews:
        review.update(findings=[], blocking_findings=[])
    reviews[0].update(findings=["B2", "advisory", "B1"], blocking_findings=["B2", "B1"],
                      started_at="2026-11-12T12:00:00Z", finished_at="2026-11-12T12:00:01.250Z")
    reviews[1].update(started_at="2026-11-12T13:00:00+01:00",
                      finished_at="2026-11-12T12:00:02.500Z")
    reviews[2].update(findings=["B3", "B2", "closing-advisory"], blocking_findings=["B3", "B2"])
    reviews[3].update(started_at="2026-11-12T12:00:03Z", finished_at="2026-11-12T12:00:03Z")
    selected = task("CTX-003", title="Kompakt ä", kind="implementation",
                    depends_on=["DEP-002", "DEP-001"], spec_refs=["§18", "§17"],
                    notes=["Bounded note; product stays stopped."], verification={"baseline": "not_run"})
    selected.update(
        evidence=["https://example.test/selected-evidence"],
        review={"author": "fixture-author", "reviewer": "fixture-other", "outcome": "changes_requested"},
        supplemental_workflow_evidence={"private": "supplemental sentinel"},
        workflow={"required_stages": ["tests", "implementation"], "stages": {
            "tests": {"authors": ["fixture-author"], "revision": CURRENT, "paths": [BRIEF], "reviews": reviews},
            "implementation": {"authors": ["other-stage-author"], "revision": OLD,
                               "paths": [BRIEF], "reviews": []}}})
    return {
        "schema_version": 1, "project": "Synthetic context", "baseline_version": "0.1.0",
        "specification_version": "0.45", "updated_at": "2026-11-12T12:01:00Z",
        "summary": "Synthetic", "verification": [],
        "activity": [{"date": "2026-11-12", "task": "CTX-999", "note": "activity sentinel"}],
        "tasks": [task("DEP-001", status="done", evidence=["https://example.test/dependency"]),
                  selected, task("CTX-004", depends_on=["CTX-003"]),
                  task("DEP-002", status="done", evidence=["https://example.test/dependency-two"]),
                  task("CTX-999", brief="docs/tasks/WORKFLOW-005.md", notes=["unrelated sentinel"])],
    }


def expected_compact():
    """Contract declaration order and values, independent of context/workflow helpers."""
    return {
        "specification_version": "0.45", "state_updated_at": "2026-11-12T12:01:00Z",
        "task": {"id": "CTX-003", "title": "Kompakt ä", "module": "Quality and tooling",
                 "kind": "implementation", "status": "planned", "owner": "fixture-author",
                 "brief": BRIEF, "summary": "Synthetic scope", "depends_on": ["DEP-002", "DEP-001"],
                 "spec_refs": ["§18", "§17"], "notes": ["Bounded note; product stays stopped."],
                 "verification": {"baseline": "not_run"}, "required_stages": ["tests", "implementation"]},
        "dependencies": [
            {"id": "DEP-002", "status": "done", "owner": "fixture-author", "brief": BRIEF, "publication": None},
            {"id": "DEP-001", "status": "done", "owner": "fixture-author", "brief": BRIEF, "publication": None}],
        "dependents": ["CTX-004"], "shared_brief_tasks": ["DEP-001", "CTX-004", "DEP-002"],
        "available": True, "limitations": COMPACT_LIMIT,
    }


class CompactContextTests(unittest.TestCase):
    def invoke(self, data, *options):
        """Use disposable input/output bytes and detect writes on every result path."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            state, output = root / "state.json", root / "PROGRESS.html"
            state.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
            output.write_bytes(b"Preserve this HTML exactly.\n")
            before = {p.relative_to(root): p.read_bytes() for p in root.rglob("*") if p.is_file()}
            stdout, stderr = StringIO(), StringIO()
            with patch.object(progress, "STATE", state), patch.object(progress, "OUTPUT", output), \
                 patch.object(sys, "argv", ["progress.py", "context", *options]), \
                 redirect_stdout(stdout), redirect_stderr(stderr):
                try:
                    code = progress.main()
                except SystemExit as error:
                    code = error.code
            self.assertEqual({p.relative_to(root): p.read_bytes() for p in root.rglob("*") if p.is_file()}, before)
            return code, stdout.getvalue(), stderr.getvalue()

    def success(self, result):
        code, stdout, stderr = result
        diagnostic = ("intended assertion red: COMPACT_CONTEXT_NOT_IMPLEMENTED"
                      if (code, stdout, stderr) ==
                         (1, "", "Progress error: COMPACT_CONTEXT_NOT_IMPLEMENTED\n")
                      else "unexpected setup/interface failure is NOT intended red: " + stderr)
        self.assertEqual(code, 0, diagnostic)
        self.assertEqual(stderr, "")
        return stdout

    def test_compact_cli_literal_shape_order_serialization_and_no_stage(self):
        data = literal_state()
        before = copy.deepcopy(data)
        expected = expected_compact()
        output = self.success(self.invoke(data, "CTX-003", "--compact"))
        self.assertEqual(output, json.dumps(expected, indent=2, ensure_ascii=False) + "\n")
        self.assertEqual(progress.context(data, "CTX-003", compact=True), expected)
        self.assertEqual(data, before)

    def test_selected_stage_keeps_all_refs_historical_blockers_and_exact_metrics(self):
        expected = expected_compact()
        expected["stage_context"] = {
            "name": "tests", "complete": True, "next_gate": "complete",
            "packet": {"authors": ["fixture-author"], "revision": CURRENT, "paths": [BRIEF]},
            "review_refs": REFS, "historical_blocking_findings": ["B2", "B1", "B3"],
            "metrics": {"review_count": 5, "elapsed_seconds": 3.75, "timed_reviews": 3},
        }
        output = self.success(self.invoke(literal_state(), "CTX-003", "--compact", "--stage", "tests"))
        self.assertEqual(output, json.dumps(expected, indent=2, ensure_ascii=False) + "\n")
        for excluded in ("supplemental sentinel", "activity sentinel", "other-stage-author",
                         "closing-advisory", "unrelated sentinel", "selected-evidence"):
            self.assertNotIn(excluded, output)

    def test_unstarted_and_untimed_stages_and_existing_next_gates(self):
        for reviews, complete, gate, count in ((None, False, "authoring", 0),
                                               ([], False, "initial", 0),
                                               (REFS[:1], False, "verification", 1),
                                               (REFS[:2], False, "closing", 2)):
            with self.subTest(gate=gate):
                data = literal_state()
                stages = data["tasks"][1]["workflow"]["stages"]
                if reviews is None:
                    del stages["tests"]
                else:
                    stages["tests"]["reviews"] = [dict(r, findings=["B"] if i == 0 else [],
                        blocking_findings=["B"] if i == 0 else []) for i, r in enumerate(reviews)]
                view = json.loads(self.success(self.invoke(data, "CTX-003", "--stage", "tests", "--compact")))
                self.assertEqual(view["stage_context"], {
                    "name": "tests", "complete": complete, "next_gate": gate,
                    "packet": None if reviews is None else {
                        "authors": ["fixture-author"], "revision": CURRENT, "paths": [BRIEF]},
                    "review_refs": [] if reviews is None else reviews,
                    "historical_blocking_findings": ["B"] if count else [],
                    "metrics": {"review_count": count, "elapsed_seconds": None, "timed_reviews": 0}})

    def test_optional_publication_empty_required_stages_and_availability(self):
        data = literal_state()
        publication = {"state": "open", "pr": "https://example.test/selected-evidence", "head": CURRENT,
                       "merge_commit": None, "merged_at": None, "observed_at": "2026-11-12T12:00:00Z"}
        selected = data["tasks"][1]
        selected["publication"] = publication
        expected = expected_compact()
        # Publication precedes the added required_stages in the declaration order.
        required = expected["task"].pop("required_stages")
        expected["task"].update(publication=publication, required_stages=required)
        self.assertEqual(json.loads(self.success(self.invoke(data, "CTX-003", "--compact"))), expected)
        for status, expected_available in (("planned", True), ("ready", True), ("blocked", False),
                                            ("in_progress", False), ("in_review", False)):
            with self.subTest(status=status):
                selected["status"] = status
                view = json.loads(self.success(self.invoke(data, "CTX-003", "--compact")))
                self.assertIs(view["available"], expected_available)
        leaf = json.loads(self.success(self.invoke(data, "CTX-999", "--compact")))
        self.assertEqual(leaf["task"]["required_stages"], [])
        self.assertNotIn("publication", leaf["task"])
        self.assertEqual(leaf["dependencies"], [])
        # Dependency publication is retained, including its null merge fields.
        data = literal_state()
        data["tasks"][0].update(publication=publication, evidence=[publication["pr"]])
        data["tasks"][1]["status"] = "planned"
        self.assertEqual(json.loads(self.success(self.invoke(data, "CTX-003", "--compact")))
                         ["dependencies"][1]["publication"], publication)
        data["tasks"][0]["status"] = "planned"
        self.assertIs(json.loads(self.success(self.invoke(data, "CTX-003", "--compact")))["available"], False)

    def test_default_context_bytes_and_full_selected_history_are_unchanged(self):
        data = literal_state()
        before = copy.deepcopy(data)
        expected = expected_compact()
        expected["task"] = copy.deepcopy(data["tasks"][1])
        expected["dependencies"] = [
            {"id": "DEP-002", "status": "done", "owner": "fixture-author", "brief": BRIEF,
             "evidence": ["https://example.test/dependency-two"], "verification": {}, "review": None},
            {"id": "DEP-001", "status": "done", "owner": "fixture-author", "brief": BRIEF,
             "evidence": ["https://example.test/dependency"], "verification": {}, "review": None}]
        expected["limitations"] = FULL_LIMIT
        self.assertEqual(self.success(self.invoke(data, "CTX-003")), json.dumps(expected, indent=2, ensure_ascii=False) + "\n")
        self.assertEqual(progress.context(data, "CTX-003"), expected)
        self.assertEqual(progress.context(data, "CTX-003", compact=False), expected)
        expected["stage_context"] = {
            "name": "tests", "packet": copy.deepcopy(data["tasks"][1]["workflow"]["stages"]["tests"]),
            "complete": True, "next_gate": "complete",
            "metrics": {"review_count": 5, "blocking_findings": ["B2", "B1", "B3"],
                        "closing_only_findings": ["B3", "closing-advisory"],
                        "elapsed_seconds": 3.75, "timed_reviews": 3}}
        self.assertEqual(self.success(self.invoke(data, "CTX-003", "--stage", "tests")),
                         json.dumps(expected, indent=2, ensure_ascii=False) + "\n")
        self.assertEqual(data, before)

    def test_lookup_validation_and_argparse_errors_precede_compact_ni(self):
        for options, message in ((("CTX-404", "--compact"), "Unknown task: CTX-404"),
                                 (("CTX-003", "--stage", "unknown", "--compact"),
                                  "CTX-003: unknown workflow stage: unknown")):
            with self.subTest(options=options):
                code, stdout, stderr = self.invoke(literal_state(), *options)
                self.assertEqual((code, stdout, stderr), (1, "", f"Progress error: {message}\n"))
        data = literal_state()
        data["tasks"][-1]["depends_on"] = ["MISSING-999"]
        code, stdout, stderr = self.invoke(data, "CTX-404", "--compact")
        self.assertEqual((code, stdout, stderr),
                         (1, "", "Progress error: CTX-999: unknown dependency MISSING-999\n"))
        for options in (("--compact",), ("CTX-003", "--not-an-option")):
            code, stdout, stderr = self.invoke(literal_state(), *options)
            self.assertEqual((code, stdout), (2, ""))
            self.assertIn("usage:", stderr)
        with self.assertRaisesRegex(ValueError, "Unknown task: CTX-404"):
            progress.context(literal_state(), "CTX-404", compact=True)


if __name__ == "__main__":
    unittest.main()
