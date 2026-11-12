#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Opt-in admitted-SDK publication checks; excluded from tooling discovery.

Run only after full independent test review, with the selected schema Python and
-I -S -B. No fake child results, dependency installs or real-repository writes.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
# -I excludes sibling paths; this shared fixture imports only the standard library.
sys.path.insert(0, str(ROOT / "tests/support"))
from publication_fixture import ENV, PublicationFixture

WITNESS = b'{"tests":1,"failures":0,"errors":0,"skipped":0}\n'
# Independent probe of the existing target, not a replacement validation engine.
# Its actual setUpClass still admits metadata before importing standards libraries.
SCHEMA_PROBE = r'''
import importlib.util
import sys
import unittest
spec = importlib.util.spec_from_file_location("fixture_schema_probe", sys.argv[1])
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
suite = unittest.TestLoader().loadTestsFromName(
    "RepositorySchemas.test_progress_schema_and_actual_state", module)
result = unittest.TextTestRunner(verbosity=2).run(suite)
if result.testsRun != 1 or result.failures or result.errors or result.skipped:
    raise SystemExit(1)
sys.stdout.write('{"tests":1,"failures":0,"errors":0,"skipped":0}\n')
'''


class PublicationSchemaIntegrationTests(unittest.TestCase):
    def run_checker(self, fixture, schema_python=None):
        before = fixture.snapshot()
        result = subprocess.run(
            [os.path.abspath(sys.executable), "-E", "-s", "-B", str(fixture.root / "tools/check_publication.py"),
             *fixture.arguments(schema_python=os.path.abspath(sys.executable) if schema_python is None else schema_python)],
            # Caller cwd deliberately differs from ROOT.
            cwd=fixture.root.parent, env=ENV, capture_output=True, timeout=120)
        self.assertEqual(fixture.snapshot(), before, "SDK checks must not mutate the disposable candidate")
        return result

    def probe_schema(self, fixture):
        return subprocess.run(
            [os.path.abspath(sys.executable), "-I", "-S", "-B", "-c", SCHEMA_PROBE,
             "tests/schema_validation/test_project_schemas.py"], cwd=fixture.root,
            env=ENV, capture_output=True, timeout=120)

    def test_valid_current_working_tree_copy_passes_real_admission_validator_and_publication(self):
        with tempfile.TemporaryDirectory() as directory:
            fixture = PublicationFixture(Path(directory) / "candidate")
            before = fixture.snapshot()
            probe = self.probe_schema(fixture)
            self.assertEqual(probe.returncode, 0, "SDK/setup failure is NOT intended red: " + probe.stderr.decode(errors="replace"))
            self.assertEqual(probe.stdout, WITNESS)
            self.assertIn(b"test_progress_schema_and_actual_state", probe.stderr)
            self.assertEqual(fixture.snapshot(), before)
            result = self.run_checker(fixture)
            self.assertEqual(result.returncode, 0,
                             "Publication must use the actual admitted schema target: " + result.stderr.decode(errors="replace"))
            summary = json.loads(result.stdout)
            self.assertEqual(summary["checks"]["schema"], "passed")
            self.assertEqual(summary["head"], fixture.head)
            self.assertEqual(summary["commit_count"], 1)

    def test_schema_invalid_semantic_valid_state_is_rejected_by_actual_target_and_checker(self):
        with tempfile.TemporaryDirectory() as directory:
            fixture = PublicationFixture(Path(directory) / "candidate")
            # The semantic validator permits missing review on an in-progress contract;
            # the shipped schema's task.required independently demands that member.
            del fixture.task["review"]
            fixture.save()  # Existing semantic validation + render must actually succeed.
            fixture.head = fixture.commit("Synthetic schema-invalid semantic-valid state")
            semantic = subprocess.run(
                [os.path.abspath(sys.executable), "-E", "-s", "-B", "tools/progress.py", "validate"],
                cwd=fixture.root, env=ENV, capture_output=True, timeout=120)
            self.assertEqual(semantic.returncode, 0, semantic.stderr)
            # Prove project/render checks also pass: failure must belong to the actual schema.
            project = subprocess.run(
                [os.path.abspath(sys.executable), "-E", "-s", "-B", "tools/check_project.py"],
                cwd=fixture.root, env=ENV, capture_output=True, timeout=120)
            self.assertEqual(project.returncode, 0, project.stderr)
            probe = self.probe_schema(fixture)
            self.assertEqual(probe.returncode, 1, probe.stderr)
            self.assertEqual(probe.stdout, b"")
            self.assertIn(b"ValidationError", probe.stderr)
            self.assertIn(b"'review' is a required property", probe.stderr)
            result = self.run_checker(fixture)
            self.assertEqual((result.returncode, result.stdout), (1, b""))
            self.assertTrue(result.stderr.strip())
            self.assertNotIn(b"PUBLICATION_CHECK_NOT_IMPLEMENTED", result.stderr,
                             "NI cannot prove rejection by the actual validator")
            self.assertIn(b"review", result.stderr)

    def test_empty_success_and_unavailable_schema_executables_never_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            fixture = PublicationFixture(Path(directory) / "candidate")
            for executable in ("/bin/true", str(fixture.root / "missing-schema-python")):
                with self.subTest(executable=executable):
                    result = self.run_checker(fixture, executable)
                    self.assertEqual((result.returncode, result.stdout), (1, b""))
                    self.assertTrue(result.stderr.strip())
                    self.assertNotIn(b"PUBLICATION_CHECK_NOT_IMPLEMENTED", result.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
