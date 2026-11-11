# SPDX-License-Identifier: GPL-3.0-or-later
"""CI-001 contract examples; all host facts, commands and writes are isolated."""
import builtins
from contextlib import ExitStack, redirect_stderr, redirect_stdout
from io import StringIO
import io
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

SOURCE_PATH = Path("/etc/apt/sources.list.d/ubuntu.sources")
MIRROR_URI = "mirror+file:/etc/apt/apt-mirrors.txt"
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
DIAGNOSTICS = [
    ["cat", "/etc/resolv.conf"], ["resolvectl", "status"],
    ["cat", "/etc/apt/apt-mirrors.txt"],
    *[["getent", "ahostsv4", host] for host in (
        "azure.archive.ubuntu.com", "archive.ubuntu.com", "security.ubuntu.com")],
]
PRIVILEGED = [
    ["sudo", "tee", "/etc/apt/apt-mirrors.txt"],
    ["sudo", "tee", "/etc/apt/apt.conf.d/99-subathon-network"],
    ["sudo", "apt-get", "update"],
]


class PrepareUbuntuAptTests(unittest.TestCase):
    def setUp(self):
        self.calls, self.responses = [], {}
        self.source = SOURCES
        self.stdout, self.stderr = StringIO(), StringIO()
        stack = ExitStack()
        self.addCleanup(stack.close)
        stack.enter_context(patch.dict(os.environ, {
            "GITHUB_ACTIONS": "true", "RUNNER_ENVIRONMENT": "github-hosted"}, clear=True))
        stack.enter_context(patch.object(platform, "freedesktop_os_release",
                                        return_value={"ID": "ubuntu", "VERSION_ID": "24.04"}))
        self.read_source = stack.enter_context(patch.object(
            Path, "read_text", autospec=True, side_effect=self.source_text))
        stack.enter_context(patch.object(subprocess, "run", side_effect=self.run_command))
        # A deviating helper must fail here rather than invoke a real local process/write.
        for name in ("Popen", "check_call", "check_output", "call"):
            stack.enter_context(patch.object(subprocess, name, side_effect=AssertionError("real process")))
        stack.enter_context(patch.object(os, "system", side_effect=AssertionError("real process")))
        stack.enter_context(patch.object(os, "open", side_effect=AssertionError("direct file access")))
        for name in ("open", "write_text", "write_bytes"):
            stack.enter_context(patch.object(Path, name, side_effect=AssertionError("direct file access")))
        def guarded_open(original):
            def read_only(file, mode="r", *args, **kwargs):
                self.assertFalse(any(flag in mode for flag in "wax+"), "direct file write")
                return original(file, mode, *args, **kwargs)
            return read_only
        stack.enter_context(patch("builtins.open", side_effect=guarded_open(builtins.open)))
        stack.enter_context(patch("io.open", side_effect=guarded_open(io.open)))
        stack.enter_context(redirect_stdout(self.stdout))
        stack.enter_context(redirect_stderr(self.stderr))

    def source_text(self, path, *args, **kwargs):
        self.assertEqual(path, SOURCE_PATH)
        self.assertEqual(args, ())
        self.assertEqual(kwargs, {"encoding": "utf-8"})
        if isinstance(self.source, Exception):
            raise self.source
        return self.source

    def run_command(self, command, **options):
        self.assertIn(command, DIAGNOSTICS + PRIVILEGED, "unexpected system command")
        self.assertFalse(options.get("shell", False))
        self.calls.append((command, options))
        if options.get("capture_output") and any(options.get(k) is not None for k in ("stdout", "stderr")):
            raise ValueError("stdout and stderr cannot be used with capture_output")
        if options.get("input") is not None and options.get("stdin") is not None:
            raise ValueError("stdin and input arguments may not both be used")
        if isinstance(options.get("input"), str):
            self.assertTrue(options.get("text") or options.get("encoding"), "str input needs text mode")
        response = self.responses.get(tuple(command), 0)
        if isinstance(response, Exception):
            raise response
        if response and options.get("check", False):
            raise subprocess.CalledProcessError(response, command)
        captured = options.get("capture_output", False)
        return subprocess.CompletedProcess(command, response,
            stdout=f"stdout for {' '.join(command)}\n" if captured else None,
            stderr=f"stderr for {' '.join(command)}\n" if captured else None)

    def test_success_has_exact_command_set_trusted_mirror_bytes_and_deadlines(self):
        self.assertEqual(apt.main(), 0)
        commands = [c for c, _ in self.calls]
        self.assertCountEqual(commands[:6], DIAGNOSTICS)
        self.assertEqual(commands[6:], PRIVILEGED)
        self.read_source.assert_called_once_with(SOURCE_PATH, encoding="utf-8")
        writes = self.calls[6:8]
        self.assertEqual(writes[0][1]["input"],
                         "https://archive.ubuntu.com/ubuntu/\nhttps://security.ubuntu.com/ubuntu/\n")
        self.assertEqual(writes[1][1]["input"],
                         'Acquire::Retries "1";\nAcquire::http::Timeout "15";\n'
                         'Acquire::https::Timeout "15";\nAPT::Update::Error-Mode "any";\n')
        for _, options in writes:
            self.assertTrue(options["check"])
            self.assertTrue(options["text"])
            self.assertEqual(options["stdout"], subprocess.DEVNULL)
            self.assertNotIn("stderr", options)
            self.assertEqual(options["timeout"], 10)
        self.assertTrue(self.calls[-1][1]["check"])
        self.assertEqual(self.calls[-1][1]["timeout"], 300)
        self.assertIn("Diagnostic only; not dependency admission", self.stdout.getvalue())
        for command, options in self.calls[:6]:
            self.assertFalse(options["check"])
            self.assertTrue(options["capture_output"])
            self.assertTrue(options["text"])
            self.assertEqual(options["timeout"], 10)
            self.assertIn(f"{' '.join(command)}: exit 0", self.stdout.getvalue())
            self.assertIn(f"stdout for {' '.join(command)}", self.stdout.getvalue())
            self.assertIn(f"stderr for {' '.join(command)}", self.stdout.getvalue() + self.stderr.getvalue())

    def test_failed_azure_lookup_is_reported_and_update_still_decides_success(self):
        self.responses[tuple(DIAGNOSTICS[3])] = 2
        self.assertEqual(apt.main(), 0)
        output = self.stdout.getvalue()
        self.assertIn("getent ahostsv4 azure.archive.ubuntu.com: exit 2", output)
        self.assertIn("stdout for getent ahostsv4 azure.archive.ubuntu.com", output)
        self.assertIn("stderr for getent ahostsv4 azure.archive.ubuntu.com",
                      output + self.stderr.getvalue())
        for host in ("archive.ubuntu.com", "security.ubuntu.com"):
            self.assertIn(f"getent ahostsv4 {host}: exit 0", output)
        self.assertEqual(self.calls[-1][0], PRIVILEGED[-1])

    def test_host_refusals_have_specific_reasons_and_no_commands(self):
        cases = [
            ({}, {"ID": "ubuntu", "VERSION_ID": "24.04"}, "requires GitHub-hosted Actions"),
            ({"GITHUB_ACTIONS": "false", "RUNNER_ENVIRONMENT": "github-hosted"},
             {"ID": "ubuntu", "VERSION_ID": "24.04"}, "requires GitHub-hosted Actions"),
            ({"GITHUB_ACTIONS": "", "RUNNER_ENVIRONMENT": "github-hosted"},
             {"ID": "ubuntu", "VERSION_ID": "24.04"}, "requires GitHub-hosted Actions"),
            ({"GITHUB_ACTIONS": "true", "RUNNER_ENVIRONMENT": "self-hosted"},
             {"ID": "ubuntu", "VERSION_ID": "24.04"}, "requires GitHub-hosted Actions"),
            ({"GITHUB_ACTIONS": "true", "RUNNER_ENVIRONMENT": "github-hosted"},
             {"ID": "ubuntu", "VERSION_ID": "22.04"}, "requires Ubuntu 24.04"),
            ({"GITHUB_ACTIONS": "true", "RUNNER_ENVIRONMENT": "github-hosted"},
             {"ID": "debian", "VERSION_ID": "24.04"}, "requires Ubuntu 24.04"),
        ]
        for env, release, reason in cases:
            with self.subTest(env=env, release=release), patch.dict(os.environ, env, clear=True), \
                    patch.object(platform, "freedesktop_os_release", return_value=release):
                self.assertEqual(apt.main(), 1)
                self.assertIn("Ubuntu APT setup failed:", self.stderr.getvalue())
                self.assertIn(reason, self.stderr.getvalue())
                self.assertNotIn("not implemented", self.stderr.getvalue())
                self.assertEqual(self.calls, [])
                self.stderr.truncate(0); self.stderr.seek(0)

    def test_source_layout_requires_every_stanza_and_accepts_comments_crlf(self):
        invalid = ["", "# URIs: " + MIRROR_URI + "\n",
                   SOURCES.replace(MIRROR_URI, "http://azure.archive.ubuntu.com/ubuntu/", 1),
                   SOURCES.replace(MIRROR_URI, "mirror+file:/other-list"),
                   SOURCES + "\nTypes: deb\nURIs: https://third-party.example\n",
                   SOURCES + "Enabled: no\n",
                   SOURCES.replace("URIs:", "URIs: " + MIRROR_URI + "\nURIs:", 1),
                   SOURCES.replace(MIRROR_URI, MIRROR_URI + "\n https://extra.example", 1)]
        for source in invalid:
            with self.subTest(source=source):
                self.source = source
                self.assertEqual(apt.main(), 1)
                self.assertIn("unsupported Ubuntu APT source layout", self.stderr.getvalue())
                self.assertEqual(self.calls, [])
                self.stderr.truncate(0); self.stderr.seek(0)
        self.source = ("# runner sources\n" + SOURCES).replace("\n", "\r\n")
        self.assertEqual(apt.main(), 0)

    def test_missing_host_identity_or_source_is_reported_without_commands(self):
        with patch.object(platform, "freedesktop_os_release", side_effect=OSError("identity unavailable")):
            self.assertEqual(apt.main(), 1)
            self.assertIn("identity unavailable", self.stderr.getvalue())
            self.assertEqual(self.calls, [])
        self.source = FileNotFoundError("source unavailable")
        self.assertEqual(apt.main(), 1)
        self.assertIn("source unavailable", self.stderr.getvalue())
        self.assertEqual(self.calls, [])

    def test_missing_or_timed_out_diagnostics_are_reported_but_not_admission(self):
        self.responses[tuple(DIAGNOSTICS[1])] = FileNotFoundError("resolvectl missing")
        self.responses[tuple(DIAGNOSTICS[3])] = subprocess.TimeoutExpired(DIAGNOSTICS[3], 10)
        self.assertEqual(apt.main(), 0)
        self.assertIn("resolvectl status: unavailable", self.stdout.getvalue())
        self.assertIn("getent ahostsv4 azure.archive.ubuntu.com: unavailable", self.stdout.getvalue())
        self.assertIn("resolvectl missing", self.stdout.getvalue())
        self.assertIn("timed out after 10 seconds", self.stdout.getvalue())
        self.assertEqual(self.calls[-1][0], PRIVILEGED[-1])

    def test_each_critical_failure_or_timeout_returns_nonzero_and_stops(self):
        for index, command in enumerate(PRIVILEGED):
            for failure in (1, subprocess.TimeoutExpired(command, 10 if index < 2 else 300),
                            FileNotFoundError("sudo missing")):
                with self.subTest(command=command, failure=failure):
                    self.responses = {tuple(command): failure}
                    self.calls.clear(); self.stderr.truncate(0); self.stderr.seek(0)
                    self.assertEqual(apt.main(), 1)
                    self.assertIn("Ubuntu APT setup failed:", self.stderr.getvalue())
                    self.assertNotIn("not implemented", self.stderr.getvalue())
                    self.assertEqual([c for c, _ in self.calls if c[0] == "sudo"], PRIVILEGED[:index + 1])


if __name__ == "__main__":
    unittest.main()
