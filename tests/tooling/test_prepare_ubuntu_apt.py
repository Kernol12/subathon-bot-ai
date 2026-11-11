# SPDX-License-Identifier: GPL-3.0-or-later
"""CI-001 contract examples; no local sudo, package installs or network access."""
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
import os
from pathlib import Path
import platform
import subprocess
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import prepare_ubuntu_apt as apt

SOURCES = """Types: deb
URIs: mirror+file:/etc/apt/apt-mirrors.txt
Suites: noble noble-updates noble-backports
Components: main universe restricted multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: mirror+file:/etc/apt/apt-mirrors.txt
Suites: noble-security
Components: main universe restricted multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
"""


class PrepareUbuntuAptTests(unittest.TestCase):
    def setUp(self):
        self.calls = []
        self.output = StringIO()
        for mocker in (
            patch.dict(os.environ, {"GITHUB_ACTIONS": "true"}, clear=True),
            patch.object(platform, "freedesktop_os_release",
                         return_value={"ID": "ubuntu", "VERSION_ID": "24.04"}),
            patch.object(Path, "read_text", return_value=SOURCES),
            patch.object(subprocess, "run", side_effect=self.run_command),
            redirect_stdout(self.output), redirect_stderr(self.output),
        ):
            mocker.__enter__()
            self.addCleanup(mocker.__exit__, None, None, None)

    def run_command(self, command, **options):
        self.calls.append((command, options))
        return subprocess.CompletedProcess(command, 0)

    def test_exact_official_mirrors_and_bounded_strict_update(self):
        apt.prepare_apt()
        writes = [(c, o) for c, o in self.calls if c[:2] == ["sudo", "tee"]]
        self.assertEqual([c for c, _ in writes], [
            ["sudo", "tee", "/etc/apt/apt-mirrors.txt"],
            ["sudo", "tee", "/etc/apt/apt.conf.d/99-subathon-network"],
        ])
        self.assertEqual(writes[0][1]["input"],
                         "https://archive.ubuntu.com/ubuntu/\n"
                         "https://security.ubuntu.com/ubuntu/\n")
        self.assertEqual(writes[1][1]["input"],
                         'Acquire::Retries "1";\n'
                         'Acquire::http::Timeout "15";\n'
                         'Acquire::https::Timeout "15";\n'
                         'APT::Update::Error-Mode "any";\n')
        for _, options in writes:
            self.assertTrue(options["check"])
            self.assertEqual(options["timeout"], 10)
        command, options = self.calls[-1]
        self.assertEqual(command, ["sudo", "apt-get", "update"])
        self.assertTrue(options["check"])
        self.assertEqual(options["timeout"], 300)
        diagnostics = [(c, o) for c, o in self.calls if c[0] != "sudo"]
        commands = [c for c, _ in diagnostics]
        self.assertIn(["cat", "/etc/resolv.conf"], commands)
        self.assertIn(["resolvectl", "status"], commands)
        for host in ("azure.archive.ubuntu.com", "archive.ubuntu.com", "security.ubuntu.com"):
            self.assertIn(["getent", "ahostsv4", host], commands)
        for _, options in diagnostics:
            self.assertEqual(options["timeout"], 10)

    def test_azure_dns_failure_is_reported_but_actual_update_decides_success(self):
        def command_result(command, **options):
            self.run_command(command, **options)
            return subprocess.CompletedProcess(command, 2 if command[-1] == "azure.archive.ubuntu.com" else 0)

        with patch.object(subprocess, "run", side_effect=command_result):
            apt.prepare_apt()
        self.assertEqual(self.calls[-1][0], ["sudo", "apt-get", "update"])
        self.assertIn("azure.archive.ubuntu.com", self.output.getvalue())
        self.assertIn("2", self.output.getvalue())

    def test_local_or_unsupported_host_refuses_before_commands_or_writes(self):
        for env, release in (
            ({}, {"ID": "ubuntu", "VERSION_ID": "24.04"}),
            ({"GITHUB_ACTIONS": "true"}, {"ID": "ubuntu", "VERSION_ID": "22.04"}),
            ({"GITHUB_ACTIONS": "true"}, {"ID": "debian", "VERSION_ID": "24.04"}),
        ):
            with self.subTest(env=env, release=release), \
                    patch.dict(os.environ, env, clear=True), \
                    patch.object(platform, "freedesktop_os_release", return_value=release):
                with self.assertRaisesRegex(RuntimeError, "GitHub|Ubuntu|24.04"):
                    apt.prepare_apt()
                self.assertEqual(self.calls, [])

    def test_unexpected_source_layout_refuses_before_privileged_commands(self):
        for source in ("", SOURCES.replace("mirror+file:/etc/apt/apt-mirrors.txt",
                                         "https://unapproved.example/ubuntu")):
            with self.subTest(source=source), patch.object(Path, "read_text", return_value=source):
                with self.assertRaisesRegex(RuntimeError, "source|mirror|layout"):
                    apt.prepare_apt()
                self.assertFalse(any(c[0] == "sudo" for c, _ in self.calls))
                self.calls.clear()

    def test_diagnostic_timeout_or_missing_command_does_not_replace_update(self):
        def command_result(command, **options):
            self.run_command(command, **options)
            if command[0] == "resolvectl":
                raise FileNotFoundError("resolvectl")
            if command[0] == "getent":
                raise subprocess.TimeoutExpired(command, 10)
            return subprocess.CompletedProcess(command, 0)

        with patch.object(subprocess, "run", side_effect=command_result):
            apt.prepare_apt()
        self.assertEqual(self.calls[-1][0], ["sudo", "apt-get", "update"])
        self.assertIn("resolvectl", self.output.getvalue())
        self.assertIn("getent", self.output.getvalue())

    def test_write_failure_stops_before_update(self):
        def command_result(command, **options):
            self.run_command(command, **options)
            if command[:2] == ["sudo", "tee"]:
                raise subprocess.CalledProcessError(1, command)
            return subprocess.CompletedProcess(command, 0)

        with patch.object(subprocess, "run", side_effect=command_result):
            with self.assertRaises(subprocess.CalledProcessError):
                apt.prepare_apt()
        self.assertFalse(any(c[:2] == ["sudo", "apt-get"] for c, _ in self.calls))

    def test_update_failure_or_timeout_returns_nonzero(self):
        for failure in (subprocess.CalledProcessError(100, ["sudo", "apt-get", "update"]),
                        subprocess.TimeoutExpired(["sudo", "apt-get", "update"], 300)):
            with self.subTest(failure=type(failure).__name__):
                def command_result(command, **options):
                    self.run_command(command, **options)
                    if command[:2] == ["sudo", "apt-get"]:
                        raise failure
                    return subprocess.CompletedProcess(command, 0)

                with patch.object(subprocess, "run", side_effect=command_result):
                    self.assertNotEqual(apt.main(), 0)
                self.assertIn(["sudo", "apt-get", "update"], [c for c, _ in self.calls])
                self.calls.clear()


if __name__ == "__main__":
    unittest.main()
