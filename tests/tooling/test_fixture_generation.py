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
    "sqlite_application_bundle_vectors.json": (
        "aebff7061f91fdf7598b321963bae155c139ed1dc2d1705e3122268bed84c312",
        "a374acb6441ab366b67100d742da7883f98a44f782fccf736b0b06b33e7a20dc"),
    "sqlite_application_holds_vectors.json": (
        "c7f14e0aac721e186d5a327e2e4bd20f9617d9516f633f36ff7c205619082ef5",
        "996f84632e9b1fa3b77c84fd12655aa090f81236308993bb44dcb83ddf842f95"),
    "sqlite_original_claims_vectors.json": (
        "1f0eb63e8fe1b026c511eac3d9efb7b11053caa8e8599b24989dd1424fb8cf92",
        "a824e1a98c2f2a06916e8c65d2f5674ff1bd7718e7e92c9b1395dfba9f159803"),
    "sqlite_account_sessions_vectors.json": (
        "2e7c373a3bd526c770b85a2b875950820ac4b04db7d35e61849aa02025e7c919",
        "8e4e7030349f4fa79276b886ccedfb5859b1868a9d4f7e06192cc2dd8d278fa2"),
    "application_service_vectors.json": (
        "d6adb1ad1fa9effb1033fb3b7c722202ddddef00531c5819241f517101dbe109",
        "d95d24f1c3f2cc5c9dfa43e1bd7c8f914bef365f8c955c5a7903a9894e5fd06c"),
}
SERVICE_OUTPUT = "application_service_vectors.json"


# FIXTURE-002 brief's independent selector metadata, in the frozen old row order.
SELECTOR_ROLES = (
    ("contribution-max-receipt-prefers-subject-A", True),
    ("receipt-max-resolves-source-B", False),
    ("contribution-receipt8-prefers-subject-A", False),
    ("missing-contribution42-receipt9", False),
    ("receipt8-selects-original-subject-C", False),
    ("contribution42-receipt8-resolves-source-C", False),
)
STORAGE_OUTPUTS = {
    "bundle": "sqlite_application_bundle_vectors.json",
    "holds": "sqlite_application_holds_vectors.json",
    "original_claims": "sqlite_original_claims_vectors.json",
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
        value = json.loads(data)
        if name == STORAGE_OUTPUTS["original_claims"]:
            self.assertEqual(len(value["selectors"]), len(SELECTOR_ROLES))
            for row, (expected_name, expected_role) in zip(value["selectors"], SELECTOR_ROLES):
                self.assertEqual(row.pop("name"), expected_name)
                role = row.pop("check_lower_priority_corruption")
                self.assertIs(type(role), bool)
                self.assertIs(role, expected_role)
        elif name != SERVICE_OUTPUT:
            self.assertEqual(hashlib.sha256(data).hexdigest(), raw, name)
        canonical = json.dumps(value, sort_keys=True, separators=(",", ":"),
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

    def test_selected_family_preserves_other_outputs(self):
        selected_families = {**STORAGE_OUTPUTS, "application_service": SERVICE_OUTPUT}
        for family, filename in selected_families.items():
            with self.subTest(family=family), tempfile.TemporaryDirectory() as temporary:
                parent = Path(temporary)
                project = self.isolated_tree(parent)
                cwd = parent / "unrelated-working-directory"
                cwd.mkdir()
                selected = cwd / "selected"
                selected.mkdir()
                preserved = {name: b"unselected-family sentinel: " + name.encode("ascii")
                             for name in OUTPUTS if name != filename}
                preserved["unrelated.txt"] = b"unrelated sentinel"
                for name, data in preserved.items():
                    (selected / name).write_bytes(data)
                (selected / filename).write_bytes(b"stale selected output")
                original_inputs = snapshot(project)
                result = self.invoke(project, cwd, "--fixture", family,
                                     "--output-dir", "selected")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual({path.name for path in selected.iterdir()},
                                 {filename, *preserved})
                self.assert_original_output(filename, (selected / filename).read_bytes())
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
                if name == SERVICE_OUTPUT:
                    # Independent R1 presentation of the frozen legacy value, not a recipe result.
                    data = (json.dumps(json.loads(data), ensure_ascii=False, allow_nan=False,
                                       indent=2) + "\n").encode("utf-8")
                if name == STORAGE_OUTPUTS["original_claims"]:
                    value = json.loads(data)
                    if hashlib.sha256(data).hexdigest() == OUTPUTS[name][0]:
                        # Independent pre-adoption comparison: ONLY the brief's two fields.
                        self.assertEqual(len(value["selectors"]), len(SELECTOR_ROLES))
                        for row, (label, role) in zip(value["selectors"], SELECTOR_ROLES):
                            row["name"] = label
                            row["check_lower_priority_corruption"] = role
                        data = (json.dumps(value, ensure_ascii=False, allow_nan=False,
                                           indent=2) + "\n").encode("utf-8")
                self.assert_original_output(name, data)
                (fixtures / name).write_bytes(data)
            original = snapshot(parent)
            result = self.invoke(project, cwd, "--check")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(snapshot(parent), original)

            reference = fixtures / "sqlite_reference_capture_vectors.json"
            reference.write_bytes(reference.read_bytes() + b"\n")
            drifted = snapshot(parent)
            result = self.invoke(project, cwd, "--check", "--fixture", "reference_capture")
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn(reference.name, result.stdout + result.stderr)
            self.assertEqual(snapshot(parent), drifted)

            reference.write_bytes(original[str(reference.relative_to(parent))])
            holds = fixtures / STORAGE_OUTPUTS["holds"]
            holds.write_bytes(holds.read_bytes() + b"\n")
            drifted = snapshot(parent)
            result = self.invoke(project, cwd, "--check", "--fixture", "holds")
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn(holds.name, result.stdout + result.stderr)
            self.assertEqual(snapshot(parent), drifted)

            holds.write_bytes(original[str(holds.relative_to(parent))])
            service = fixtures / SERVICE_OUTPUT
            service.write_bytes(service.read_bytes() + b"\n")
            drifted = snapshot(parent)
            result = self.invoke(project, cwd, "--check", "--fixture", "application_service")
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn(service.name, result.stdout + result.stderr)
            self.assertEqual(snapshot(parent), drifted)

            service.write_bytes(original[str(service.relative_to(parent))])
            seed = fixtures / "retained_context_vectors.json"
            seed.write_bytes(seed.read_bytes() + b" \n")  # Valid JSON; only the raw pin changes.
            drifted = snapshot(parent)
            result = self.invoke(project, cwd, "--check")
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertIn(seed.name, result.stdout + result.stderr)
            self.assertEqual(snapshot(parent), drifted)


if __name__ == "__main__":
    unittest.main()
