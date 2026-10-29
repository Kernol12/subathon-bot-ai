# SPDX-License-Identifier: GPL-3.0-or-later
"""Developer-tool regression tests; these do not establish product correctness."""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import progress


class ProgressTests(unittest.TestCase):
    def setUp(self):
        self.state = json.loads((ROOT / "progress/state.json").read_text(encoding="utf-8"))

    def test_baseline_is_valid(self):
        progress.validate(self.state)

    def test_unknown_dependency_is_rejected(self):
        self.state["tasks"][1]["depends_on"] = ["MISSING-999"]
        with self.assertRaisesRegex(ValueError, "unknown dependency"):
            progress.validate(self.state)

    def test_dependency_cycle_is_rejected(self):
        self.state["tasks"][0]["status"] = "planned"
        self.state["tasks"][1]["status"] = "planned"
        self.state["tasks"][0]["depends_on"] = [self.state["tasks"][1]["id"]]
        with self.assertRaisesRegex(ValueError, "cycle"):
            progress.validate(self.state)

    def test_cannot_start_implementation_before_red_baseline(self):
        task = next(t for t in self.state["tasks"] if t["id"] == "IMPL-001")
        task["status"] = "in_progress"
        with self.assertRaisesRegex(ValueError, "dependency"):
            progress.validate(self.state)

    def test_completed_implementation_requires_passing_acceptance(self):
        task = next(t for t in self.state["tasks"] if t["id"] == "IMPL-001")
        task.update(status="done", depends_on=[], evidence=["docs/evidence/baseline-validation.md"],
                    review={"author": "author-session", "reviewer": "reviewer-session", "outcome": "approved"})
        with self.assertRaisesRegex(ValueError, "passing acceptance"):
            progress.validate(self.state)

    def test_self_review_is_rejected(self):
        task = next(t for t in self.state["tasks"] if t["id"] == "IMPL-001")
        task.update(status="done", depends_on=[], evidence=["docs/evidence/baseline-validation.md"],
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


if __name__ == "__main__":
    unittest.main()
