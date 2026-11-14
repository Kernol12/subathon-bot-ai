# SPDX-License-Identifier: GPL-3.0-or-later
"""Acceptance-map links and the readable evidence policy, independent of product tests."""
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
import check_project


class ProjectArtifactTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        for name, content in {
            "docs/STATUS.md": "Specification version: 0.45\n",
            "docs/specification.md": "Version: 0.45\n| TEST-01 | Test |\n",
            "docs/evidence/README.md": "Readable evidence summary.\n",
            "tests/acceptance/example.cpp": "// Local authored test artifact.\n",
            "tests/acceptance/time_credit_tests.cpp": "// No compiled tests in this fixture.\n",
            "CMakePresets.json": '{"version":6,"configurePresets":[],"buildPresets":[],"testPresets":[]}',
            "README.md": "Project.\n",
        }.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")
        self.coverage = {
            "specification_version": "0.45",
            "decisions": [{"id": "TEST-01", "tests": ["tests/acceptance/example.cpp"],
                           "evidence": ["docs/evidence/README.md"]}],
            "scenarios": [],
        }

    def run_check(self):
        (self.root / "tests/acceptance-map.json").write_text(json.dumps(self.coverage), encoding="utf-8")
        output = StringIO()
        with patch.object(check_project, "ROOT", self.root), \
             redirect_stdout(output), redirect_stderr(output):
            result = check_project.main()
        return result, output.getvalue()

    def test_accepts_local_and_https_evidence_without_fetching_it(self):
        self.coverage["scenarios"] = [{"tests": ["tests/acceptance/example.cpp"], "evidence": [
            "https://github.com/example/project/blob/evidence-archive/docs/evidence/receipt.md#review"]}]
        result, output = self.run_check()
        self.assertEqual(result, 0, output)

    def test_rejects_missing_or_remote_test_artifacts_and_invalid_evidence(self):
        for field, value in (
            ("tests", "tests/acceptance/missing.cpp"),
            ("tests", "https://example.com/test.cpp"),
            ("evidence", "docs/evidence/missing.md"),
            ("evidence", "http://example.com/receipt"),
            ("evidence", "https:///receipt"),
        ):
            with self.subTest(field=field, value=value):
                self.coverage["scenarios"] = []
                control,detail=self.run_check(); self.assertEqual(control,0,detail)
                self.coverage["scenarios"] = [{field: [value]}]
                result, output = self.run_check()
                self.assertEqual(result, 1, output)

    def test_requires_the_readable_summary(self):
        control,detail=self.run_check(); self.assertEqual(control,0,detail)
        (self.root / "docs/evidence/README.md").unlink()
        self.coverage["decisions"][0]["evidence"] = []
        result, output = self.run_check()
        self.assertEqual(result, 1, output)

    def test_rejects_extra_evidence_receipts(self):
        control,detail=self.run_check(); self.assertEqual(control,0,detail)
        (self.root / "docs/evidence/old-receipt.md").write_text("Archived receipt.\n", encoding="utf-8")
        result, output = self.run_check()
        self.assertEqual(result, 1, output)


if __name__ == "__main__":
    unittest.main()
