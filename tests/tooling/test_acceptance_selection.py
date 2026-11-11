# SPDX-License-Identifier: GPL-3.0-or-later
"""Public acceptance selection outcomes; no compiler or optional SDK is required."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / "cmake/AcceptanceSuites.cmake"


class AcceptanceSelectionTests(unittest.TestCase):
    def select(self, definitions):
        # This runs the actual project function in CMake script mode. The result
        # is its public suite selection, not a mirror of private target flags.
        cmake = shutil.which("cmake")
        self.assertIsNotNone(cmake, "The documented CMake >=3.25 tool is required")
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            result_path = directory / "selection.txt"
            script = directory / "select.cmake"
            script.write_text(
                'cmake_minimum_required(VERSION 3.25)\n'
                'include("${SELECTION_MODULE}")\n'
                'subathon_resolve_acceptance_suites(selected)\n'
                'file(WRITE "${SELECTION_RESULT}" "${selected}")\n',
                encoding="utf-8")
            command = [cmake, f"-DSELECTION_MODULE:FILEPATH={MODULE}",
                       f"-DSELECTION_RESULT:FILEPATH={result_path}"]
            command += [f"-D{name}={value}" for name, value in definitions.items()]
            result = subprocess.run(command + ["-P", str(script)], capture_output=True,
                                    text=True, encoding="utf-8", timeout=10)
            selection = result_path.read_text(encoding="utf-8") if result_path.exists() else None
            return result, selection

    def assert_selection(self, definitions, expected):
        result, selection = self.select(definitions)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(selection, ";".join(expected))

    def test_disabled_gate_selects_no_acceptance_suites(self):
        self.assert_selection({"SUBATHON_BUILD_ACCEPTANCE_TESTS:BOOL": "OFF"}, [])

    def test_bare_enabled_gate_preserves_credit_default(self):
        self.assert_selection({"SUBATHON_BUILD_ACCEPTANCE_TESTS:BOOL": "ON"},
                              ["time-credit"])

    def test_explicit_empty_or_unknown_selector_is_refused(self):
        for selector, diagnostic in (
                ("", "Acceptance suite selection must not be empty"),
                ("not-a-suite", "Unknown acceptance suite")):
            with self.subTest(selector=selector):
                result, selection = self.select({
                    "SUBATHON_BUILD_ACCEPTANCE_TESTS:BOOL": "ON",
                    "SUBATHON_ACCEPTANCE_SUITES:STRING": selector})
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(diagnostic, result.stdout + result.stderr)
                self.assertIsNone(selection)

    def test_enabled_retired_cache_request_never_silently_selects_credit(self):
        for gate, explicit_selector in (("OFF", None), ("ON", None),
                                        ("ON", "time-credit")):
            with self.subTest(gate=gate, selector=explicit_selector):
                definitions = {
                    "SUBATHON_BUILD_ACCEPTANCE_TESTS:BOOL": gate,
                    "SUBATHON_BUILD_SQLITE_ACCOUNT_SESSION_TESTS:BOOL": "ON"}
                if explicit_selector is not None:
                    definitions["SUBATHON_ACCEPTANCE_SUITES:STRING"] = explicit_selector
                result, selection = self.select(definitions)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Retired acceptance option", result.stdout + result.stderr)
                self.assertIn("--fresh", result.stdout + result.stderr)
                self.assertIsNone(selection)

    def test_union_selects_existing_profile_closure_without_other_sdks(self):
        # Existing milestone adds exact numeric; editorial adds debt + lifecycle.
        # Repeating a selection is idempotent. An old disabled cache flag adds none.
        self.assert_selection({
            "SUBATHON_BUILD_ACCEPTANCE_TESTS:BOOL": "ON",
            "SUBATHON_ACCEPTANCE_SUITES:STRING":
                "milestone-progression;timer-editorial;milestone-progression",
            "SUBATHON_BUILD_SQLITE_ACCOUNT_SESSION_TESTS:BOOL": "OFF"},
            ["exact-numeric", "milestone-progression", "time-debt",
             "timer-editorial", "timer-lifecycle"])

    def test_native_websocket_profile_selection_is_independent_and_unique(self):
        self.assert_selection({
            "SUBATHON_BUILD_ACCEPTANCE_TESTS:BOOL": "ON",
            "SUBATHON_ACCEPTANCE_SUITES:STRING":
                "native-websocket-profile;native-websocket-profile"},
            ["native-websocket-profile"])

    def test_api_host_profiles_include_real_storage_and_corrected_transport(self):
        # These are public build-profile requirements: both host baselines need
        # the current physical store and corrected native transport. Native also
        # keeps the Qt host baseline selected; duplicates add no second runner.
        dependencies = [
            "application-service", "native-websocket-profile",
            "sqlite-account-sessions", "sqlite-bundle", "sqlite-contribution",
            "sqlite-hold", "sqlite-migrations", "sqlite-original-claim",
            "sqlite-plan", "sqlite-receipt", "sqlite-reference-capture",
            "sqlite-retained", "sqlite-work", "sqlite-writer"]
        for selector, own in (
                ("api-host", ["api-host"]),
                ("api-host-native;api-host;api-host-native",
                 ["api-host", "api-host-native"])):
            with self.subTest(selector=selector):
                self.assert_selection({
                    "SUBATHON_BUILD_ACCEPTANCE_TESTS:BOOL": "ON",
                    "SUBATHON_ACCEPTANCE_SUITES:STRING": selector},
                    sorted(dependencies + own))

    def test_account_session_selection_includes_compiled_migration_prefix(self):
        # The account manifest appends to the actual compiled initial-capture
        # factory, so an account-only request needs that complete SQLite prefix.
        self.assert_selection({
            "SUBATHON_BUILD_ACCEPTANCE_TESTS:BOOL": "ON",
            "SUBATHON_ACCEPTANCE_SUITES:STRING": "sqlite-account-sessions"},
            ["application-service", "sqlite-account-sessions", "sqlite-bundle",
             "sqlite-contribution", "sqlite-hold", "sqlite-migrations",
             "sqlite-original-claim", "sqlite-plan", "sqlite-receipt",
             "sqlite-reference-capture", "sqlite-retained", "sqlite-work",
             "sqlite-writer"])


if __name__ == "__main__":
    unittest.main()
