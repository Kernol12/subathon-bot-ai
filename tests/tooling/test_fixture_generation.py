# SPDX-License-Identifier: GPL-3.0-or-later
"""Public independent reproduction and read-only drift checks; no product proof."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SEEDS = {
    "retained_context_vectors.json":
        "631260c6050c6e47c36ef69fbf6fecf67016407b7f9cf93d6b3246b9438f1dcf",
    "sqlite_receipts_vectors.json":
        "350e2f22db39661c7966996f8d7ae5da12fad91b929605280ff7930d1bec7e8a",
    "sqlite_contributions_vectors.json":
        "86eadfdf53367e7b3d7ae27f1885483aad9e8183a6b759e40ad526ae1d33ff23",
    "sqlite_application_plans_vectors.json":
        "9a603917d75019b3fe18c8dd6e7dbb05386d48f64b90ee340f5d2d66d21b968a",
}
# Frozen before candidate authoring in the independently reviewed parent task brief.
OUTPUTS = {
    "sqlite_reference_capture_vectors.json": (
        "48ddd9cd81bfa8b39b61a994a266cda7a68e356facfcda95be8a4c499876ba64",
        "4a3416559d4196f14dcd18506d1faf3a4e53e93a260294f4a52ad414cbb02b55"),
    "sqlite_account_sessions_vectors.json": (
        "2e7c373a3bd526c770b85a2b875950820ac4b04db7d35e61849aa02025e7c919",
        "8e4e7030349f4fa79276b886ccedfb5859b1868a9d4f7e06192cc2dd8d278fa2"),
}


def snapshot(directory):
    return {str(path.relative_to(directory)): path.read_bytes()
            for path in directory.rglob("*") if path.is_file()}


class FixtureGenerationTests(unittest.TestCase):
    def isolated_tree(self, parent):
        project = parent / "relocated-project"
        tools = project / "tools"
        tools.mkdir(parents=True)
        shutil.copyfile(ROOT / "tools/generate_fixtures.py", tools / "generate_fixtures.py")
        shutil.copytree(ROOT / "tools/fixture_sources", tools / "fixture_sources",
                        ignore=shutil.ignore_patterns("__pycache__"))
        seeds = project / "tests/fixtures"
        seeds.mkdir(parents=True)
        for name, expected in SEEDS.items():
            data = (ROOT / "tests/fixtures" / name).read_bytes()
            self.assertEqual(hashlib.sha256(data).hexdigest(), expected, name)
            (seeds / name).write_bytes(data)
        self.assertEqual({path.name for path in seeds.iterdir()}, set(SEEDS))
        return project

    def invoke(self, project, cwd, *arguments):
        return subprocess.run(
            [sys.executable, str(project / "tools/generate_fixtures.py"), *arguments],
            cwd=cwd, capture_output=True, text=True, encoding="utf-8", timeout=20)

    def assert_original_output(self, name, data):
        raw, typed = OUTPUTS[name]
        self.assertEqual(hashlib.sha256(data).hexdigest(), raw, name)
        canonical = json.dumps(json.loads(data), sort_keys=True, separators=(",", ":"),
                               ensure_ascii=False, allow_nan=False).encode("utf-8")
        self.assertEqual(hashlib.sha256(canonical).hexdigest(), typed, name)

    def test_independent_generation_and_selected_output_preservation(self):
        with tempfile.TemporaryDirectory() as temporary:
            parent = Path(temporary)
            project = self.isolated_tree(parent)
            cwd = parent / "unrelated-working-directory"
            cwd.mkdir()
            original_inputs = snapshot(project)
            with self.subTest(selection="default"):
                result = self.invoke(project, cwd, "--output-dir", "generated")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                output = cwd / "generated"
                self.assertEqual({path.name for path in output.iterdir()}, set(OUTPUTS))
                for name in OUTPUTS:
                    self.assert_original_output(name, (output / name).read_bytes())
                self.assertEqual(snapshot(project), original_inputs)

            with self.subTest(selection="account_sessions"):
                selected = cwd / "selected"
                selected.mkdir()
                account = selected / "sqlite_account_sessions_vectors.json"
                account.write_bytes(b"stale selected output")
                preserved = {
                    "sqlite_reference_capture_vectors.json": b"unselected reference sentinel",
                    "unrelated.txt": b"unrelated output sentinel",
                }
                for name, data in preserved.items():
                    (selected / name).write_bytes(data)
                result = self.invoke(project, cwd, "--fixture", "account_sessions",
                                     "--output-dir", "selected")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assert_original_output(account.name, account.read_bytes())
                self.assertEqual({path.name for path in selected.iterdir()},
                                 {account.name, *preserved})
                for name, data in preserved.items():
                    self.assertEqual((selected / name).read_bytes(), data, name)
                self.assertEqual(snapshot(project), original_inputs)

    def test_check_refuses_output_and_raw_seed_drift_without_writing(self):
        with tempfile.TemporaryDirectory() as temporary:
            parent = Path(temporary)
            project = self.isolated_tree(parent)
            cwd = parent / "unrelated-working-directory"
            cwd.mkdir()
            fixtures = project / "tests/fixtures"
            for name in OUTPUTS:
                data = (ROOT / "tests/fixtures" / name).read_bytes()
                self.assert_original_output(name, data)
                (fixtures / name).write_bytes(data)
            original = snapshot(project)
            result = self.invoke(project, cwd, "--check")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(snapshot(project), original)

            reference = fixtures / "sqlite_reference_capture_vectors.json"
            reference.write_bytes(reference.read_bytes() + b"\n")
            drifted = snapshot(project)
            result = self.invoke(project, cwd, "--check", "--fixture", "reference_capture")
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn(reference.name, result.stdout + result.stderr)
            self.assertEqual(snapshot(project), drifted)

            reference.write_bytes(original[str(reference.relative_to(project))])
            seed = fixtures / "retained_context_vectors.json"
            seed.write_bytes(seed.read_bytes() + b" \n")  # Valid JSON; only the raw pin changes.
            drifted = snapshot(project)
            result = self.invoke(project, cwd, "--check")
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn(seed.name, result.stdout + result.stderr)
            self.assertEqual(snapshot(project), drifted)


if __name__ == "__main__":
    unittest.main()
