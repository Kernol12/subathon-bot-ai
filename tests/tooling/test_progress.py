# SPDX-License-Identifier: GPL-3.0-or-later
"""Developer-tool regression tests; these do not establish product correctness."""
import copy
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from html.parser import HTMLParser
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import progress


class TaskRows(HTMLParser):
    def __init__(self, html):
        super().__init__()
        self.rows = []
        self.inputs = []
        self.feed(html)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == "article" and attrs.get("class") == "task":
            self.rows.append(attrs)
        if tag == "input":
            self.inputs.append(attrs)


def task(task_id, dependencies=(), status="planned", kind="scaffold"):
    return {
        "id": task_id, "title": task_id, "module": "Foundation", "kind": kind,
        "status": status, "owner": "Unassigned", "depends_on": list(dependencies),
        "spec_refs": ["§17"], "brief": "docs/tasks/BOOT-001.md", "summary": task_id,
        "notes": [], "verification": {}, "evidence": [], "review": None,
    }


class ProgressTests(unittest.TestCase):
    def setUp(self):
        self.state = json.loads((ROOT / "progress/state.json").read_text(encoding="utf-8"))

    def test_baseline_is_valid(self):
        progress.validate(self.state)

    def test_combined_notes_at_500_characters_are_valid_for_every_status(self):
        for status in ("planned", "ready", "in_progress", "in_review", "blocked", "done"):
            with self.subTest(status=status):
                record = task("NOTE-999", status=status)
                record.update(notes=["a" * 250, "b" * 250],
                              evidence=["https://example.com/proof"])
                self.state["tasks"] = [record]
                self.state["activity"] = []
                progress.validate(self.state)

    def test_combined_notes_over_500_characters_are_rejected_for_every_status(self):
        for status in ("planned", "ready", "in_progress", "in_review", "blocked", "done"):
            with self.subTest(status=status):
                record = task("NOTE-999", status=status)
                record.update(notes=["a" * 250, "b" * 250, "c"],
                              evidence=["https://example.com/proof"])
                self.state["tasks"] = [record]
                self.state["activity"] = []
                with self.assertRaisesRegex(ValueError, r"NOTE-999.*500.*shorten.*notes.*JSON"):
                    progress.validate(self.state)

    def test_set_over_limit_refuses_without_changing_state_or_html(self):
        record = task("NOTE-999")
        record["notes"] = ["a" * 250, "b" * 249]
        self.state["tasks"] = [record]
        self.state["activity"] = []
        with tempfile.TemporaryDirectory() as directory:
            state_path = Path(directory) / "state.json"
            html_path = Path(directory) / "PROGRESS.html"
            original_state = json.dumps(self.state, indent=3).encode("utf-8")
            original_html = b"Existing rendered snapshot must remain byte-identical."
            state_path.write_bytes(original_state)
            html_path.write_bytes(original_html)
            output = StringIO()
            with patch.object(progress, "STATE", state_path), \
                 patch.object(progress, "OUTPUT", html_path), \
                 patch.object(sys, "argv", ["progress.py", "set", "NOTE-999", "--status", "ready",
                                            "--owner", "new-owner", "--note", "xy"]), \
                 redirect_stderr(output), redirect_stdout(output):
                result = progress.main()
            self.assertEqual(result, 1, output.getvalue())
            self.assertRegex(output.getvalue(), r"NOTE-999.*500.*shorten.*notes.*JSON")
            self.assertEqual(state_path.read_bytes(), original_state)
            self.assertEqual(html_path.read_bytes(), original_html)

    def test_unknown_dependency_is_rejected(self):
        self.state["tasks"][1]["depends_on"] = ["MISSING-999"]
        with self.assertRaisesRegex(ValueError, "unknown dependency"):
            progress.validate(self.state)

    def test_dependency_cycle_is_rejected(self):
        self.state["tasks"] = [
            task("CYCLEA-999", ["CYCLEB-999"]),
            task("CYCLEB-999", ["CYCLEA-999"]),
        ]
        with self.assertRaisesRegex(ValueError, "cycle"):
            progress.validate(self.state)

    def test_cannot_start_implementation_before_red_baseline(self):
        self.state["tasks"] = [
            task("TEST-999", kind="test_baseline"),
            task("IMPL-999", ["TEST-999"], status="in_progress", kind="implementation"),
        ]
        self.state["activity"] = []
        with self.assertRaisesRegex(ValueError, "dependency"):
            progress.validate(self.state)

    def test_can_start_implementation_after_reviewed_red_baseline(self):
        baseline = task("TEST-999", status="done", kind="test_baseline")
        baseline.update(evidence=["docs/evidence/README.md"],
                        verification={"baseline": "observed_red"},
                        review={"author": "test-author-session", "reviewer": "test-reviewer-session",
                                "outcome": "approved"})
        self.state["tasks"] = [
            baseline,
            task("IMPL-999", ["TEST-999"], status="in_progress", kind="implementation"),
        ]
        self.state["activity"] = []
        progress.validate(self.state)

    def test_completed_implementation_requires_passing_acceptance(self):
        implementation = task("IMPL-999", status="done", kind="implementation")
        implementation.update(evidence=["docs/evidence/README.md"],
                              review={"author": "author-session", "reviewer": "reviewer-session",
                                      "outcome": "approved"})
        self.state["tasks"] = [implementation]
        self.state["activity"] = []
        for verification in ({}, {"acceptance": "failed"}):
            with self.subTest(verification=verification):
                implementation["verification"] = verification
                with self.assertRaisesRegex(ValueError, "passing acceptance"):
                    progress.validate(self.state)

    def test_completed_implementation_accepts_passing_acceptance_and_separate_review(self):
        implementation = task("IMPL-999", status="done", kind="implementation")
        implementation.update(evidence=["docs/evidence/README.md"],
                              verification={"acceptance": "passed"},
                              review={"author": "author-session", "reviewer": "reviewer-session",
                                      "outcome": "approved"})
        self.state["tasks"] = [implementation]
        self.state["activity"] = []
        progress.validate(self.state)

    def test_https_pull_request_evidence_is_valid_and_rendered_as_a_link(self):
        completed = task("DONE-999", status="done")
        completed["evidence"] = ["https://github.com/example/project/pull/42"]
        self.state["tasks"] = [completed]
        self.state["activity"] = []

        progress.validate(self.state)

        rendered = progress.render(self.state)
        self.assertIn('href="https://github.com/example/project/pull/42"', rendered)

    def test_done_cleanup_requires_a_separate_approved_review(self):
        cleanup = task("CLEAN-999", status="done", kind="cleanup")
        cleanup["evidence"] = ["https://github.com/example/project/pull/43"]
        self.state["tasks"] = [cleanup]
        self.state["activity"] = []

        with self.assertRaisesRegex(ValueError, "separate approved review"):
            progress.validate(self.state)

    def test_non_https_url_is_not_accepted_as_evidence(self):
        planned = task("PATH-999")
        planned["evidence"] = ["http://example.com/pull/44"]
        self.state["tasks"] = [planned]
        self.state["activity"] = []

        with self.assertRaisesRegex(ValueError, "project-relative path"):
            progress.validate(self.state)

    def test_self_review_is_rejected(self):
        task = next(t for t in self.state["tasks"] if t["id"] == "IMPL-001")
        task.update(status="done", depends_on=[], evidence=["docs/evidence/README.md"],
                    verification={"acceptance": "passed"}, review={"author": "same", "reviewer": "same", "outcome": "approved"})
        with self.assertRaisesRegex(ValueError, "separate approved review"):
            progress.validate(self.state)

    def test_text_and_attribute_injection_are_escaped(self):
        payload = '<img src=x onerror="alert(1)"><script>alert(1)</script>'
        self.state["tasks"][0]["title"] = payload
        self.state["tasks"][0]["notes"] = [payload]
        output = progress.render(self.state)
        self.assertNotIn(payload, output)
        self.assertIn('&lt;img src=x onerror=&quot;alert(1)&quot;&gt;', output)

    def test_template_tokens_in_notes_stay_literal(self):
        self.state["tasks"][0]["notes"] = ["Literal @@PROJECT@@ token"]
        self.assertIn("Literal @@PROJECT@@ token", progress.render(self.state))

    def test_paths_cannot_escape_or_use_url_schemes(self):
        for value in ("../secret", "/tmp/secret", "https://example.com/x", "javascript:alert(1)", "C:\\secret"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                progress.local_path(value)

    def test_duplicate_task_ids_are_rejected(self):
        self.state["tasks"].append(copy.deepcopy(self.state["tasks"][0]))
        with self.assertRaisesRegex(ValueError, "duplicate"):
            progress.validate(self.state)

    def test_invalid_verification_claim_is_rejected(self):
        self.state["tasks"][0]["verification"]["product"] = "probably fine"
        with self.assertRaisesRegex(ValueError, "verification"):
            progress.validate(self.state)

    def test_render_is_deterministic(self):
        self.assertEqual(progress.render(self.state), progress.render(copy.deepcopy(self.state)))

    def test_execution_view_orders_shuffled_diamond_and_epic_without_mutating_state(self):
        self.state["tasks"] = [
            task("EPIC-999", ["FINAL-999"], kind="epic"),
            task("FINAL-999", ["LEFT-999", "RIGHT-999"]),
            task("LEFT-999", ["ROOT-999"]),
            task("FREE-999"),
            task("RIGHT-999", ["ROOT-999"]),
            task("ROOT-999", status="done"),
        ]
        original = copy.deepcopy(self.state)
        rows = TaskRows(progress.render(self.state)).rows
        self.assertEqual([r["id"] for r in rows],
                         ["FREE-999", "ROOT-999", "LEFT-999", "RIGHT-999", "FINAL-999", "EPIC-999"])
        self.assertEqual({r["id"]: int(r["data-epic-order"]) for r in rows},
                         {"EPIC-999": 0, "FINAL-999": 1, "LEFT-999": 2,
                          "FREE-999": 3, "RIGHT-999": 4, "ROOT-999": 5})
        self.assertEqual(self.state, original)

    def test_availability_requires_all_done_dependencies_and_an_unclaimed_task(self):
        self.state["tasks"] = [
            task("DONE-999", status="done"),
            task("OPEN-999"),
            task("AVAILABLE-999", ["DONE-999"]),
            task("WAIT-999", ["DONE-999", "OPEN-999"]),
            task("ACTIVE-999", ["DONE-999"], status="in_progress"),
            task("REVIEW-999", ["DONE-999"], status="in_review"),
            task("HELD-999", ["DONE-999"], status="blocked"),
            task("EPIC-999", ["DONE-999"], kind="epic"),
            task("READY-999", ["DONE-999"], status="ready"),
        ]
        html = progress.render(self.state)
        available = {r["id"] for r in TaskRows(html).rows if r.get("data-available") == "true"}
        self.assertEqual(available, {"OPEN-999", "AVAILABLE-999", "READY-999"})
        self.assertIn('id="available-count">3<', html)
        self.assertIn('class="next-task">OPEN-999 · OPEN-999</div>', html)

    def test_full_backlog_execution_view_places_every_prerequisite_first(self):
        rows = TaskRows(progress.render(self.state)).rows
        positions = {r["id"]: i for i, r in enumerate(rows)}
        self.assertEqual(len(positions), len(self.state["tasks"]))
        violations = [(current["id"], dependency) for current in self.state["tasks"]
                      for dependency in current["depends_on"]
                      if positions[dependency] >= positions[current["id"]]]
        self.assertFalse(violations, f"{len(violations)} prerequisites follow dependents: {violations[:5]}")

    def test_no_available_tasks_does_not_offer_a_blocked_or_completed_handoff(self):
        self.state["tasks"] = [task("DONE-999", status="done"), task("HELD-999", status="blocked")]
        html = progress.render(self.state)
        self.assertTrue('id="available-count">0<' in html, "Available count must be zero")
        self.assertTrue('class="next-task">No tasks ready to pick up</div>' in html,
                        "No available task must produce an explicit empty handoff")

    def test_state_and_module_filters_offer_all_choices_as_checked_checkboxes(self):
        self.state["tasks"] = [task("ONE-999"), task("TWO-999")]
        self.state["tasks"][0]["module"] = 'R&D <Tools> "Ops"'
        self.state["tasks"][1]["module"] = "Core"
        html = progress.render(self.state)
        inputs = TaskRows(html).inputs
        states = [i for i in inputs if i.get("name") == "status"]
        modules = [i for i in inputs if i.get("name") == "module"]
        self.assertEqual({i["value"] for i in states},
                         {"planned", "ready", "in_progress", "in_review", "blocked", "done"})
        self.assertEqual({i["value"] for i in modules}, {'R&D <Tools> "Ops"', "Core"})
        self.assertTrue(all(i.get("type") == "checkbox" and "checked" in i for i in states + modules))
        self.assertNotIn('<Tools>', html)


if __name__ == "__main__":
    unittest.main()
