# SPDX-License-Identifier: GPL-3.0-or-later
"""Successor input and identity refusal checks; no compiler, loader or real consumer."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import native_websocket_ci as recipe


class BoundedHttpSdkTests(unittest.TestCase):
    def fixture(self, root):
        profile = root / "profile"
        shutil.copytree(ROOT / "dependencies/bounded-http-foundation", profile)
        return profile

    def test_successor_binds_parent_order_and_patch_bytes(self):
        # All mutations must fail before any build/consumer process can be invoked.
        for mutation in ("parent", "order", "patch", "original"):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as temp:
                profile = self.fixture(Path(temp))
                manifest = json.loads((profile / "manifest.json").read_text())
                if mutation == "parent":
                    manifest["parent_manifest_sha256"] = "0" * 64
                elif mutation == "order":
                    manifest["ordered_patches"].reverse()
                elif mutation == "patch":
                    p = profile / "0003-trantor-bounded-http.patch"
                    p.write_text(p.read_text() + "changed\n")
                else:
                    manifest["first_party"]["sha256"] = "0" * 64
                (profile / "manifest.json").write_text(json.dumps(manifest))
                with patch.object(recipe, "SUCCESSOR", True), patch.object(recipe, "PROFILE", profile), \
                     patch.object(recipe.subprocess, "run") as child:
                    with self.assertRaises(ValueError):
                        recipe.profile()
                    child.assert_not_called()

    def test_complete_current_successor_inputs_are_recognized_without_execution(self):
        with patch.object(recipe, "SUCCESSOR", True), \
             patch.object(recipe, "PROFILE", ROOT / "dependencies/bounded-http-foundation"), \
             patch.object(recipe.subprocess, "run") as child:
            manifest = recipe.profile()
            self.assertEqual(manifest["abi_marker"], "DROGON_SUBATHON_BOUNDED_HTTP_V1")
            child.assert_not_called()

    def test_foundation_identity_faults_issue_no_execution_receipt(self):
        # The passive closure is already an input here. Identity mismatch must not
        # create the receipt subsequently consumed by the frozen foundation runner.
        for fault in ("exit", "stderr", "backend", "missing-maps", "unknown-map"):
            with self.subTest(fault=fault), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                probe = root / "probe"
                probe.write_bytes(b"synthetic non-executable ELF stand-in")
                expected = {str(probe): recipe.file_identity(probe)}
                event = {"event": "identity", "cplusplus": 202002,
                         "native_backend": "stock" if fault == "backend" else "admitted_corrected",
                         "read_sample_size": 64}
                events = []
                if fault != "missing-maps":
                    for phase in ("initial", "cleanup"):
                        path = "/unexpected/DSO" if fault == "unknown-map" else str(probe)
                        events += [{"event": "map", "phase": phase,
                                    "line": "0-1 r-xp 00000000 00:00 1 " + path},
                                   {"event": "maps_end", "phase": phase, "count": 1}]
                events.insert(0, event)
                result = subprocess.CompletedProcess([], 1 if fault == "exit" else 0,
                    "\n".join(json.dumps(e) for e in events), "fault" if fault == "stderr" else "")
                with patch.object(recipe.subprocess, "run", return_value=result):
                    with self.assertRaises(ValueError):
                        recipe.foundation_identity(root, probe, {}, expected, {}, {}, {}, {})
                self.assertFalse((root / "receipts/foundation/development-admission.json").exists())
                self.assertFalse((root / "receipts/foundation/identity-cwd").exists())

    def test_successful_identity_receipt_has_canonical_commit_and_passive_data_seal(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            probe = root / "probe"
            probe.write_bytes(b"synthetic non-executable stand-in")
            expected = {str(probe): recipe.file_identity(probe)}
            events = [{"event": "identity", "cplusplus": 202002,
                       "native_backend": "admitted_corrected", "read_sample_size": 64}]
            for phase in ("initial", "cleanup"):
                events += [{"event": "map", "phase": phase,
                            "line": "0-1 r-xp 00000000 00:00 1 " + str(probe)},
                           {"event": "maps_end", "phase": phase, "count": 1}]
            result = subprocess.CompletedProcess([], 0,
                "\n".join(json.dumps(e) for e in events), "")
            with patch.object(recipe.subprocess, "run", return_value=result), \
                 patch.object(recipe, "checked_seal", return_value={}), \
                 patch.object(recipe, "run", return_value="a" * 40 + "\n"):
                recipe.foundation_identity(root, probe, {}, expected, {}, {}, {}, {})
            receipt = json.loads((root / "receipts/foundation/development-admission.json").read_text())
            self.assertEqual(receipt["source_commit"], "a" * 40)
            self.assertEqual(receipt["review_receipt"], recipe.RULING)
            self.assertEqual(receipt["files"][str(probe)], recipe.digest(probe))
            self.assertEqual(len(receipt["sources"]), 13)
            cache = Path("/etc/ld.so.cache")
            if cache.is_file():
                self.assertEqual(receipt["files"][str(cache)], recipe.digest(cache))

    def test_recipe_children_use_the_task_compiler_temporary_directory(self):
        with tempfile.TemporaryDirectory() as temp:
            scratch = Path(temp) / "compiler-tmp"
            scratch.mkdir()
            result = subprocess.CompletedProcess([], 0, "done\n", "")
            with patch.object(recipe, "BUILD_TEMP", scratch), \
                 patch.object(recipe.subprocess, "run", return_value=result) as child:
                self.assertEqual(recipe.run("synthetic-compiler", echo=False), "done\n")
            self.assertEqual(child.call_args.kwargs["env"]["TMPDIR"], str(scratch))


if __name__ == "__main__":
    unittest.main()
