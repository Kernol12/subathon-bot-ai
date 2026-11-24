# SPDX-License-Identifier: GPL-3.0-or-later
"""Focused checks for the CI test-program library closure gate (ADR-0013).

Synthetic ldd output and ctest JSON only; no real binary is inspected or run.
Expected values follow the ADR rule: the observed set must equal the committed set.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import binary_closure as bc

LDD_QT = """\
\tlinux-vdso.so.1 (0x00007ffd5a1f2000)
\tlibQt6Sql.so.6 => /opt/qt/lib/libQt6Sql.so.6 (0x00007f0000000000)
\tlibQt6Core.so.6 => /opt/qt/lib/libQt6Core.so.6 (0x00007f0000100000)
\tlibc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f0000200000)
\t/lib64/ld-linux-x86-64.so.2 (0x00007f0000300000)
"""
QT_PROFILE = [
    "/lib64/ld-linux-x86-64.so.2",
    "libQt6Core.so.6 => $QT_PREFIX/lib/libQt6Core.so.6",
    "libQt6Sql.so.6 => $QT_PREFIX/lib/libQt6Sql.so.6",
    "libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6",
]


class ParseLdd(unittest.TestCase):
    def test_lists_resolved_libraries_and_interpreter_without_vdso(self):
        self.assertEqual(bc.parse_ldd(LDD_QT), {
            "libQt6Sql.so.6": "/opt/qt/lib/libQt6Sql.so.6",
            "libQt6Core.so.6": "/opt/qt/lib/libQt6Core.so.6",
            "libc.so.6": "/lib/x86_64-linux-gnu/libc.so.6",
            "/lib64/ld-linux-x86-64.so.2": "/lib64/ld-linux-x86-64.so.2",
        })

    def test_static_program_has_an_empty_closure(self):
        self.assertEqual(bc.parse_ldd("\tstatically linked\n"), {})

    def test_unresolved_library_fails_closed(self):
        with self.assertRaisesRegex(bc.ClosureError, "libQt6Sql.so.6"):
            bc.parse_ldd("\tlibQt6Sql.so.6 => not found\n")

    def test_unrecognised_line_fails_closed(self):
        with self.assertRaises(bc.ClosureError):
            bc.parse_ldd("\tsomething unexpected\n")


class Portable(unittest.TestCase):
    def test_longest_named_root_wins_and_other_paths_stay_absolute(self):
        roots = {"RUNNER": "/opt", "QT_PREFIX": "/opt/qt"}
        self.assertEqual(bc.portable("/opt/qt/lib/libQt6Core.so.6", roots),
                         "$QT_PREFIX/lib/libQt6Core.so.6")
        self.assertEqual(bc.portable("/opt/other", roots), "$RUNNER/other")
        self.assertEqual(bc.portable("/lib/x86_64-linux-gnu/libc.so.6", roots),
                         "/lib/x86_64-linux-gnu/libc.so.6")
        self.assertEqual(bc.portable("/opt/qtx/lib", {"QT_PREFIX": "/opt/qt"}), "/opt/qtx/lib")


class CheckTree(unittest.TestCase):
    """End-to-end over a temporary build tree with fake ELF files and a fake ldd."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name)
        self.build = self.repo / "build" / "qt"
        (self.build / "tests").mkdir(parents=True)
        self.writer = self.build / "tests" / "writer_tests"
        self.child = self.build / "tests" / "child_helper"
        for program in (self.writer, self.child):
            program.write_bytes(b"\x7fELF fake")
        self.script = self.build / "tests" / "run.sh"
        self.script.write_text("#!/bin/sh\n")
        self.ldd = {str(self.writer): LDD_QT, str(self.child): "\tstatically linked\n"}
        self.environments = []

    def run_ldd(self, program, environment):
        self.environments.append(environment)
        return self.ldd[program]

    def ctest(self, *tests):
        return {"tests": [dict(test) for test in tests]}

    def expected(self, **programs):
        return {"format": 1,
                "profiles": {"qt-sql": QT_PROFILE, "static": []},
                "programs": programs}

    def check(self, ctest, expected):
        return bc.check_tree(ctest, expected, self.build, self.repo,
                             {"QT_PREFIX": "/opt/qt"}, self.run_ldd)

    def test_matching_closures_pass_and_child_arguments_are_checked(self):
        ctest = self.ctest({"name": "a", "command": [str(self.writer), "-maxwarnings", "0"]},
                           {"name": "b", "command": [str(self.writer), str(self.child)]})
        result = self.check(ctest, self.expected(**{
            "build/qt/tests/writer_tests": "qt-sql",
            "build/qt/tests/child_helper": "static"}))
        self.assertEqual(result.errors, [])
        self.assertEqual(sorted(result.observed["programs"]),
                         ["build/qt/tests/child_helper", "build/qt/tests/writer_tests"])

    def test_extra_library_fails(self):
        self.ldd[str(self.writer)] = LDD_QT + "\tlibicuuc.so.73 => /usr/lib/libicuuc.so.73 (0x1)\n"
        ctest = self.ctest({"name": "a", "command": [str(self.writer)]})
        result = self.check(ctest, self.expected(**{"build/qt/tests/writer_tests": "qt-sql"}))
        self.assertEqual(len(result.errors), 1)
        self.assertIn("libicuuc.so.73", result.errors[0])

    def test_library_resolved_from_another_location_fails(self):
        self.ldd[str(self.writer)] = LDD_QT.replace(
            "/opt/qt/lib/libQt6Core.so.6", "/usr/lib/x86_64-linux-gnu/libQt6Core.so.6")
        ctest = self.ctest({"name": "a", "command": [str(self.writer)]})
        result = self.check(ctest, self.expected(**{"build/qt/tests/writer_tests": "qt-sql"}))
        self.assertEqual(len(result.errors), 1)
        self.assertIn("/usr/lib/x86_64-linux-gnu/libQt6Core.so.6", result.errors[0])

    def test_unlisted_program_and_unregistered_expectation_both_fail(self):
        ctest = self.ctest({"name": "a", "command": [str(self.writer)]})
        result = self.check(ctest, self.expected(**{
            "build/qt/tests/child_helper": "static"}))
        self.assertEqual(len(result.errors), 2)
        self.assertTrue(any("writer_tests" in error for error in result.errors))
        self.assertTrue(any("child_helper" in error for error in result.errors))

    def test_observed_file_passes_as_expected_file_including_outside_programs(self):
        outside = tempfile.TemporaryDirectory()
        self.addCleanup(outside.cleanup)
        python = Path(outside.name) / "python3"
        python.write_bytes(b"\x7fELF fake")
        self.ldd[str(python.resolve())] = LDD_QT
        ctest = self.ctest({"name": "a", "command": [str(self.writer)]},
                           {"name": "py", "command": [str(python), "check.py"]})
        first = self.check(ctest, {"format": 1, "profiles": {}, "programs": {}})
        self.assertEqual(len(first.errors), 2)
        self.assertEqual(self.check(ctest, first.observed).errors, [])

    def test_built_children_are_checked_and_cmake_probes_are_not(self):
        launched = self.build / "server" / "dev-server"
        probe = self.build / "CMakeFiles" / "a.out"
        for program in (launched, probe):
            program.parent.mkdir(parents=True, exist_ok=True)
            program.write_bytes(b"\x7fELF fake")
            program.chmod(0o755)
        self.ldd[str(launched)] = LDD_QT
        ctest = self.ctest({"name": "a", "command": [str(self.writer)]})
        result = self.check(ctest, self.expected(**{"build/qt/tests/writer_tests": "qt-sql"}))
        self.assertEqual(result.errors, ["build/qt/server/dev-server: registered but not listed"])

    def test_closure_that_depends_on_the_test_environment_fails(self):
        ldd = self.run_ldd
        self.run_ldd = lambda program, environment: (
            LDD_QT.replace("/opt/qt/lib/libQt6Core", "/elsewhere/libQt6Core")
            if environment.get("CLOSURE_TEST_SWITCH") else ldd(program, environment))
        ctest = self.ctest({"name": "a", "command": [str(self.writer)]},
                           {"name": "b", "command": [str(self.writer)], "properties": [
                               {"name": "ENVIRONMENT", "value": ["CLOSURE_TEST_SWITCH=1"]}]})
        result = self.check(ctest, self.expected(**{"build/qt/tests/writer_tests": "qt-sql"}))
        self.assertTrue(any("differs between its test environments" in e for e in result.errors))

    def test_non_elf_command_must_be_declared_not_elf(self):
        ctest = self.ctest({"name": "a", "command": [str(self.script)]})
        self.assertEqual(self.check(ctest, self.expected(**{
            "build/qt/tests/run.sh": bc.NOT_ELF})).errors, [])
        self.assertEqual(len(self.check(ctest, self.expected(**{
            "build/qt/tests/run.sh": "static"})).errors), 1)

    def test_ctest_environment_reaches_the_loader(self):
        ctest = self.ctest({"name": "a", "command": [str(self.writer)], "properties": [
            {"name": "ENVIRONMENT", "value": ["LD_LIBRARY_PATH=/opt/qt/lib"]}]})
        self.check(ctest, self.expected(**{"build/qt/tests/writer_tests": "qt-sql"}))
        self.assertEqual(self.environments[0]["LD_LIBRARY_PATH"], "/opt/qt/lib")

    def test_unsupported_registrations_fail_closed(self):
        expected = self.expected(**{"build/qt/tests/writer_tests": "qt-sql"})
        for test in ({"name": "no-command"},
                     {"name": "missing", "command": [str(self.build / "tests" / "absent")]},
                     {"name": "modified", "command": [str(self.writer)], "properties": [
                         {"name": "ENVIRONMENT_MODIFICATION", "value": ["A=set:1"]}]},
                     {"name": "malformed", "command": [str(self.writer)], "properties": [
                         {"name": "ENVIRONMENT", "value": ["NO_EQUALS"]}]}):
            with self.subTest(test["name"]), self.assertRaises(bc.ClosureError):
                self.check(self.ctest(test), expected)

    def test_unknown_profile_fails(self):
        ctest = self.ctest({"name": "a", "command": [str(self.writer)]})
        result = self.check(ctest, self.expected(**{"build/qt/tests/writer_tests": "nope"}))
        self.assertEqual(len(result.errors), 1)


class RunLdd(unittest.TestCase):
    def test_loader_diagnostics_on_stderr_fail_even_with_a_listing(self):
        answer = subprocess.CompletedProcess(["ldd"], 0, LDD_QT,
                                             "version `GLIBCXX_3.4.33' not found (required by x)\n")
        with patch.object(bc.subprocess, "run", return_value=answer), \
                self.assertRaisesRegex(bc.ClosureError, "GLIBCXX_3.4.33"):
            bc.run_ldd("x", dict(os.environ))


class ObservedFile(unittest.TestCase):
    def test_observed_file_round_trips_as_an_expected_file(self):
        observed = bc.observed_file({"b": QT_PROFILE, "a": QT_PROFILE, "c": bc.NOT_ELF})
        self.assertEqual(observed["programs"]["a"], observed["programs"]["b"])
        self.assertEqual(observed["programs"]["c"], bc.NOT_ELF)
        self.assertEqual(observed["profiles"][observed["programs"]["a"]], QT_PROFILE)
        json.dumps(observed)


if __name__ == "__main__":
    unittest.main()
