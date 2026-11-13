# SPDX-License-Identifier: GPL-3.0-or-later
"""Additive CI-002 wire/safety contracts; synthetic host, source, streams and children.

Oracles come from ci-setup-gates-v1 §§2/3/5 and coordinator C-A1 precision.
No legacy TestCase is imported or inherited. Settings/readback remain manual gates.
"""
from contextlib import ExitStack, redirect_stderr, redirect_stdout
from io import StringIO
import io
import json
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
MIRROR = "mirror+file:/etc/apt/apt-mirrors.txt"
A = ("Types: deb\n"
     "URIs: mirror+file:/etc/apt/apt-mirrors.txt\n"
     "Suites: noble noble-updates noble-backports\n"
     "Components: main universe restricted multiverse\n"
     "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n"
     "Enabled: yes\n")
CI001_STANZAS = ("Types: deb\n"
                 "URIs: mirror+file:/etc/apt/apt-mirrors.txt\n"
                 "Suites: noble noble-updates noble-backports\n"
                 "Components: main universe restricted multiverse\n"
                 "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n\n"
                 "Types: deb\n"
                 "URIs: mirror+file:/etc/apt/apt-mirrors.txt\n"
                 "Suites: noble-security\n"
                 "Components: main universe restricted multiverse\n"
                 "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n")
RUNNER_LINE = ('CI_APT_V1 {"event":"runner","image_os":"ubuntu24",'
               '"image_version":"20261111.127.1","os_id":"ubuntu","os_patch":"24.04.5",'
               '"os_version":"24.04","redacted_fields":[],"unknown_fields":[],"v":1}\n')
UNSUPPORTED = "Ubuntu APT setup failed: unsupported Ubuntu APT source layout\n"
UNAVAILABLE = "Ubuntu APT setup failed: CI APT diagnostic unavailable\n"
UPDATE = ["sudo", "apt-get", "update"]
DIAGNOSTICS = [
    ["cat", "/etc/resolv.conf"], ["resolvectl", "status"],
    ["cat", "/etc/apt/apt-mirrors.txt"],
    ["getent", "ahostsv4", "azure.archive.ubuntu.com"],
    ["getent", "ahostsv4", "archive.ubuntu.com"],
    ["getent", "ahostsv4", "security.ubuntu.com"],
    ["getent", "ahostsv4", "mirrors.edge.kernel.org"],
]
WRITES = [["sudo", "tee", "/etc/apt/apt-mirrors.txt"],
          ["sudo", "tee", "/etc/apt/apt.conf.d/99-subathon-network"]]
DIAGNOSTIC_FIELDS = (
    "Architectures-Add", "Architectures-Remove", "Languages", "Targets", "PDiffs",
    "By-Hash", "Allow-Insecure", "Allow-Weak", "Allow-Downgrade-To-Insecure",
    "Trusted", "Check-Valid-Until", "Valid-Until-Min", "Valid-Until-Max",
    "Check-Date", "Date-Max-Future", "InRelease-Path", "Snapshot",
)
URI_CLASSES = (
    ("http://azure.archive.ubuntu.com/ubuntu/", "azure_archive"),
    ("http://azure.archive.ubuntu.com/ubuntu", "azure_archive"),
    ("https://azure.archive.ubuntu.com/ubuntu/", "azure_archive"),
    ("https://azure.archive.ubuntu.com/ubuntu", "azure_archive"),
    ("http://archive.ubuntu.com/ubuntu/", "ubuntu_archive"),
    ("http://archive.ubuntu.com/ubuntu", "ubuntu_archive"),
    ("https://archive.ubuntu.com/ubuntu/", "ubuntu_archive"),
    ("https://archive.ubuntu.com/ubuntu", "ubuntu_archive"),
    ("http://security.ubuntu.com/ubuntu/", "ubuntu_security"),
    ("http://security.ubuntu.com/ubuntu", "ubuntu_security"),
    ("https://security.ubuntu.com/ubuntu/", "ubuntu_security"),
    ("https://security.ubuntu.com/ubuntu", "ubuntu_security"),
)


def wire(data):
    """Serialize only independent expected data, never a producer result."""
    return "CI_APT_V1 " + json.dumps(data, sort_keys=True, separators=(",", ":"),
                                    ensure_ascii=True, allow_nan=False) + "\n"


def runner(*, os_patch="24.04.5", image_os="ubuntu24", image_version="20261111.127.1",
           redacted=(), unknown=()):
    return wire({"event": "runner", "image_os": image_os, "image_version": image_version,
                 "os_id": "ubuntu", "os_patch": os_patch, "os_version": "24.04",
                 "redacted_fields": list(redacted), "unknown_fields": list(unknown), "v": 1})


def refusal(reason, stanza, line, field, value_class, fields_omitted, stanzas_omitted):
    return wire({"event": "source_refused", "field": field, "fields_omitted": fields_omitted,
                 "line": line, "reason": reason, "stanza": stanza,
                 "stanzas_omitted": stanzas_omitted, "v": 1, "value_class": value_class})


class NamedEnvironment(dict):
    """Make unrelated environment inspection a fixture failure, even for missing keys."""
    def __getitem__(self, key):
        if key not in ("GITHUB_ACTIONS", "RUNNER_ENVIRONMENT", "ImageOS", "ImageVersion"):
            raise AssertionError("unrelated environment read")
        return super().__getitem__(key)

    def get(self, key, default=None):
        try:
            return self[key]
        except KeyError:
            return default

    def __iter__(self):
        raise AssertionError("environment dump")

    def keys(self):
        raise AssertionError("environment dump")

    def items(self):
        raise AssertionError("environment dump")

    def values(self):
        raise AssertionError("environment dump")


class ControlledStderr:
    """Readable even when simulating a broken writer; explicitly model retained bytes.

    Write errors retain nothing; short writes retain all but LF. Other bad return
    types/counts and flush errors retain the full write. This models this stub only,
    not OS durability. A persistent failure also rejects the best-effort legacy line.
    """
    def __init__(self, trace, target=None, fault=None, persistent=False):
        self.trace, self.target, self.fault = trace, target, fault
        self.persistent, self.failed = persistent, False
        self.chunks, self.attempts = [], []
        self.last_record = None

    def write(self, text):
        self.trace.append(("write", text))
        self.attempts.append(text)
        if self.failed and self.persistent:
            raise ValueError("synthetic closed stream")
        is_target = (text.startswith("CI_APT_V1 ")
                     and '"event":"' + str(self.target) + '"' in text)
        self.last_record = text if text.startswith("CI_APT_V1 ") else None
        if (is_target and not self.failed and self.fault is not None
                and self.fault.startswith("write_")):
            self.failed = True
            kind = self.fault.removeprefix("write_")
            if kind == "oserror":
                raise OSError("synthetic broken pipe")
            if kind == "valueerror":
                raise ValueError("synthetic closed stream")
            self.chunks.append(text[:-1] if kind == "short" else text)
            return {"none": None, "float": float(len(text)), "bool": True,
                    "string": str(len(text)), "short": len(text) - 1,
                    "overcount": len(text) + 1}[kind]
        self.chunks.append(text)
        return len(text)

    def flush(self):
        self.trace.append(("flush",))
        if (not self.failed and self.last_record is not None
                and '"event":"' + str(self.target) + '"' in self.last_record
                and self.fault is not None and self.fault.startswith("flush_")):
            self.failed = True
            if self.fault == "flush_oserror":
                raise OSError("synthetic flush failure")
            raise ValueError("synthetic closed stream")

    def getvalue(self):
        return "".join(self.chunks)


class AptSandbox:
    """Local fixture, not a TestCase: all helper I/O is synthetic or forbidden."""
    def __init__(self, case, source=A, *, release=None, env=None, target=None, fault=None,
                 persistent=False, update_response=0):
        self.case, self.source, self.update_response = case, source, update_response
        self.release = ({"ID": "ubuntu", "VERSION_ID": "24.04",
                         "VERSION": "24.04.5 LTS (Noble Numbat)"}
                        if release is None else release)
        self.env = ({"GITHUB_ACTIONS": "true", "RUNNER_ENVIRONMENT": "github-hosted",
                     "ImageOS": "ubuntu24", "ImageVersion": "20261111.127.1"}
                    if env is None else env)
        self.trace, self.calls = [], []
        self.stdout = StringIO()
        self.stderr = ControlledStderr(self.trace, target, fault, persistent)

    def __enter__(self):
        self.stack = ExitStack()
        # Register cleanup before patching so fixture assertion failures cannot leak patches.
        self.case.addCleanup(self.stack.close)
        enter = self.stack.enter_context
        enter(patch.object(os, "environ", NamedEnvironment(self.env)))
        self.os_release = enter(patch.object(platform, "freedesktop_os_release",
                                            side_effect=self.release_data))
        self.read_source = enter(patch.object(Path, "read_text", autospec=True,
                                             side_effect=self.source_text))
        original_validation = apt.validate_sources

        def validate(source):
            self.trace.append(("validate",))
            return original_validation(source)

        self.validation = enter(patch.object(apt, "validate_sources", side_effect=validate))
        enter(patch.object(subprocess, "run", side_effect=self.run_command))
        for name in ("Popen", "check_call", "check_output", "call"):
            enter(patch.object(subprocess, name, side_effect=AssertionError("real process")))
        enter(patch.object(os, "system", side_effect=AssertionError("real process")))
        enter(patch.object(os, "open", side_effect=AssertionError("direct file access")))
        for name in ("open", "read_bytes", "write_text", "write_bytes"):
            enter(patch.object(Path, name, side_effect=AssertionError("direct file access")))
        enter(patch("builtins.open", side_effect=AssertionError("uncontrolled file access")))
        enter(patch.object(io, "open", side_effect=AssertionError("uncontrolled file access")))
        enter(redirect_stdout(self.stdout))
        enter(redirect_stderr(self.stderr))
        return self

    def __exit__(self, *exc):
        return self.stack.__exit__(*exc)

    def release_data(self):
        self.trace.append(("os_release",))
        if isinstance(self.release, Exception):
            raise self.release
        return self.release

    def source_text(self, path, *args, **kwargs):
        self.case.assertEqual((path, args, kwargs), (SOURCE_PATH, (), {"encoding": "utf-8"}))
        self.trace.append(("source_read",))
        if isinstance(self.source, Exception):
            raise self.source
        return self.source

    def run_command(self, command, **options):
        self.case.assertIn(command, DIAGNOSTICS + WRITES + [UPDATE], "unexpected command")
        self.case.assertFalse(options.get("shell", False))
        self.calls.append((command, options))
        self.trace.append(("command",))
        if options.get("capture_output") and any(options.get(k) is not None
                                                 for k in ("stdout", "stderr")):
            raise ValueError("capture_output conflicts with redirection")
        if options.get("input") is not None and options.get("stdin") is not None:
            raise ValueError("input conflicts with stdin")
        if isinstance(options.get("input"), str):
            self.case.assertTrue(options.get("text") or options.get("encoding"))
        response = self.update_response if command == UPDATE else 0
        if isinstance(response, Exception):
            raise response
        if response and options.get("check", False):
            raise subprocess.CalledProcessError(response, command)
        captured = options.get("capture_output", False)
        return subprocess.CompletedProcess(command, response,
                                           stdout="" if captured else None,
                                           stderr="" if captured else None)


class AptDiagnosticsTests(unittest.TestCase):
    def assert_read_once(self, sandbox):
        sandbox.os_release.assert_called_once_with()
        sandbox.read_source.assert_called_once_with(SOURCE_PATH, encoding="utf-8")

    def assert_records(self, sandbox, expected):
        actual = [line for line in sandbox.stderr.getvalue().splitlines(keepends=True)
                  if line.startswith("CI_APT_V1 ")]
        self.assertEqual(actual, expected)
        for line in expected:
            self.assertTrue(line.isascii())
            self.assertLessEqual(len(line.encode("ascii")), 4096)
            self.assertTrue(line.endswith("\n"))
            self.assertNotIn("\r", line)
            self.assertEqual(sandbox.stderr.attempts.count(line), 1, "record must be ONE write")
            position = sandbox.trace.index(("write", line))
            self.assertEqual(sandbox.trace[position + 1:position + 2], [("flush",)])

    def assert_refused(self, source, expected):
        with AptSandbox(self, source) as sandbox:
            self.assertEqual(apt.main(), 1)
            self.assertEqual(sandbox.calls, [])
            self.assertEqual(sandbox.stdout.getvalue(), "", "refused source must not be dumped")
            self.assert_read_once(sandbox)
            self.assertIn(UNSUPPORTED, sandbox.stderr.getvalue())
            self.assert_records(sandbox, [RUNNER_LINE, expected])
            self.assertEqual(sandbox.stderr.getvalue(), RUNNER_LINE + expected + UNSUPPORTED)
            self.assertEqual(sandbox.trace[:5], [("os_release",), ("source_read",),
                                               ("write", RUNNER_LINE), ("flush",),
                                               ("validate",)])
            sandbox.validation.assert_called_once_with(source)

    def test_runner_record_on_admitted_a(self):
        """Future red #1: successful base setup must precede missing-record assertion."""
        with AptSandbox(self) as sandbox:
            self.assertEqual(apt.main(), 0, "admitted-A baseline must succeed")
            self.assert_read_once(sandbox)
            self.assertTrue(sandbox.calls, "admitted-A baseline must reach update")
            self.assertEqual(sandbox.calls[-1], (UPDATE, {"check": True, "timeout": 300}))
            self.assertIn(RUNNER_LINE, sandbox.stderr.getvalue(),
                          "intended assertion red: CI_APT_RUNNER_RECORD_MISSING")
            self.assert_records(sandbox, [RUNNER_LINE])
            self.assertEqual(sandbox.stderr.getvalue(), RUNNER_LINE)
            self.assertEqual(sandbox.trace[:6], [("os_release",), ("source_read",),
                                               ("write", RUNNER_LINE), ("flush",),
                                               ("validate",), ("command",)])

    def test_source_refused_record_for_fixed_label(self):
        """Future red #2: legacy refusal/zero-work prerequisites before protocol red."""
        expected = ('CI_APT_V1 {"event":"source_refused","field":"Snapshot",'
                    '"fields_omitted":6,"line":7,"reason":"unknown_field","stanza":1,'
                    '"stanzas_omitted":0,"v":1,"value_class":null}\n')
        with AptSandbox(self, A + "Snapshot: private\n") as sandbox:
            self.assertEqual(apt.main(), 1, "refused-input baseline must return 1")
            self.assertIn(UNSUPPORTED, sandbox.stderr.getvalue())
            self.assertEqual(sandbox.calls, [], "legacy guard must stop every child/write")
            self.assert_read_once(sandbox)
            self.assertIn(expected, sandbox.stderr.getvalue(),
                          "intended assertion red: CI_APT_SOURCE_REFUSED_RECORD_MISSING")
            self.assert_records(sandbox, [RUNNER_LINE, expected])
            self.assertNotIn("private", sandbox.stderr.getvalue())
            self.assertEqual(sandbox.stdout.getvalue(), "")

    def test_runner_metadata_fullmatch_and_ascii_boundaries(self):
        # Input, output are independent literal pairs, not classifier-derived values.
        cases = {
            "os_patch": ("VERSION", (
                ("24.04", "24.04"), ("24.04 LTS (Noble Numbat)", "24.04"),
                ("24.04.0", "24.04.0"), ("24.04.999 LTS (Noble Numbat)", "24.04.999"),
                ("24.04.000", "24.04.000"), ("24.04.1000", None),
                ("24.04.5 LTS", None), (" 24.04.5", None), ("24.04.5\n", None),
                ("24.04.٥", None), ("24.04.5 lts (Noble Numbat)", None),
                ("24.04.5 LTS (noble numbat)", None))),
            "image_os": ("ImageOS", (
                ("ubuntu24", "ubuntu24"), ("Ubuntu24", None), ("ubuntu22", None),
                (" ubuntu24", None), ("ubuntu24\n", None), ("ubuntu24\x1b[31m", None))),
            "image_version": ("ImageVersion", (
                ("20261111.1", "20261111.1"), ("20261111.123456", "20261111.123456"),
                ("20261111.123456.123456", "20261111.123456.123456"),
                ("00000000.0.0", "00000000.0.0"), ("20261111", None),
                ("2026111.1", None), ("202611111.1", None), ("20261111.1234567", None),
                ("20261111.1.1234567", None), ("20261111.1.2.3", None),
                ("20261111.", None), ("20261111.1 ", None), ("20261111.1\n", None),
                ("２０２６１１１１.1", None))),
        }
        for output_key, (input_key, pairs) in cases.items():
            for value, expected in pairs:
                with self.subTest(field=output_key, value=value), AptSandbox(self) as sandbox:
                    if input_key == "VERSION":
                        sandbox.release[input_key] = value
                    else:
                        os.environ[input_key] = value
                    self.assertEqual(apt.main(), 0)
                    self.assert_records(sandbox, [runner(**{output_key: expected},
                                                       redacted=(output_key,) if expected is None else ())])
                    self.assert_read_once(sandbox)

    def test_runner_missing_empty_and_injected_metadata(self):
        for empty in (None, ""):
            with self.subTest(empty=empty), AptSandbox(self) as sandbox:
                for mapping, key in ((sandbox.release, "VERSION"),
                                     (os.environ, "ImageOS"), (os.environ, "ImageVersion")):
                    if empty is None:
                        mapping.pop(key)
                    else:
                        mapping[key] = ""
                self.assertEqual(apt.main(), 0)
                self.assert_records(sandbox, [runner(os_patch=None, image_os=None,
                    image_version=None, unknown=("image_os", "image_version", "os_patch"))])
        with AptSandbox(self) as sandbox:
            sandbox.release.pop("VERSION")
            os.environ["ImageOS"] = "ubuntu24\x1b[31m"
            os.environ["ImageVersion"] = "20261111.127.1\n::error::synthetic-secret"
            os.environ["CLOUD"] = "synthetic-secret"
            os.environ["UNRELATED_TOKEN"] = "synthetic-secret"
            self.assertEqual(apt.main(), 0)
            self.assert_records(sandbox, [runner(os_patch=None, image_os=None, image_version=None,
                redacted=("image_os", "image_version"), unknown=("os_patch",))])
            self.assertNotIn("synthetic-secret", sandbox.stdout.getvalue() + sandbox.stderr.getvalue())
            self.assertNotIn("\x1b", sandbox.stdout.getvalue() + sandbox.stderr.getvalue())
            self.assert_read_once(sandbox)
        with AptSandbox(self) as sandbox:
            sandbox.release["VERSION"] = "synthetic-secret\n::error::injected"
            os.environ["ImageOS"] = "synthetic-secret"
            os.environ["ImageVersion"] = "synthetic-secret"
            self.assertEqual(apt.main(), 0)
            self.assert_records(sandbox, [runner(os_patch=None, image_os=None, image_version=None,
                redacted=("image_os", "image_version", "os_patch"))])
            self.assertNotIn("synthetic-secret", sandbox.stdout.getvalue() + sandbox.stderr.getvalue())

    def test_admitted_stanzas_comments_crlf_and_seven_fields(self):
        sources = (CI001_STANZAS,
                   ("  # runner sources\n\n# comment-only block\n\n" + CI001_STANZAS)
                   .replace("\n", "\r\n"),
                   A.replace("Enabled: yes\n", "Architectures: amd64\nEnabled:  yes  \n"),
                   "URIs: mirror+file:/etc/apt/apt-mirrors.txt\n")
        for index, source in enumerate(sources):
            with self.subTest(layout=index), AptSandbox(self, source) as sandbox:
                self.assertEqual(apt.main(), 0)
                self.assert_records(sandbox, [RUNNER_LINE])
                self.assert_read_once(sandbox)
                self.assertEqual(sandbox.calls[-1][0], UPDATE)

    def test_no_record_before_host_identity_and_source_read_succeed(self):
        cases = (
            ({"env": {}}, "requires GitHub-hosted Actions", 0, 0),
            ({"release": {"ID": "debian", "VERSION_ID": "24.04"}}, "requires Ubuntu 24.04", 1, 0),
            ({"release": OSError("synthetic identity unavailable")}, "synthetic identity unavailable", 1, 0),
            ({"source": FileNotFoundError("synthetic source unavailable")}, "synthetic source unavailable", 1, 1),
        )
        for options, message, os_reads, source_reads in cases:
            with self.subTest(message=message), AptSandbox(self, **options) as sandbox:
                self.assertEqual(apt.main(), 1)
                self.assertEqual(sandbox.stderr.getvalue(), "Ubuntu APT setup failed: " + message + "\n")
                self.assert_records(sandbox, [])
                self.assertEqual(sandbox.calls, [])
                self.assertEqual(sandbox.os_release.call_count, os_reads)
                self.assertEqual(sandbox.read_source.call_count, source_reads)
                sandbox.validation.assert_not_called()

    def test_refusal_reasons_and_literal_omitted_counts(self):
        cases = (
            (A.replace(MIRROR, "https://u:secret@extra.example/x?token=s"),
             ("uri_not_admitted", 1, 2, "URIs", "other", 5, 0)),
            (A.replace(MIRROR, MIRROR + " https://extra.example"),
             ("uri_not_admitted", 1, 2, "URIs", "multiple", 5, 0)),
            (A.replace("URIs: " + MIRROR + "\n", ""),
             ("missing_uri", 1, None, "URIs", "missing", 5, 0)),
            (A.replace("URIs: " + MIRROR, "URIs: " + MIRROR + "\nURIs: " + MIRROR),
             ("duplicate_field", 1, 3, "URIs", None, 6, 0)),
            (A.replace("URIs: " + MIRROR, "URIs mirror"),
             ("malformed_line", 1, 2, None, None, 5, 0)),
            (A.replace("URIs: " + MIRROR, "URIs mirror+file:/etc/apt/apt-mirrors.txt"),
             ("unknown_field", 1, 2, "<unknown>", None, 5, 0)),
            (A.replace(MIRROR, MIRROR + "\nX-Secret: opaque"),
             ("unknown_field", 1, 3, "<unknown>", None, 6, 0)),
            (A.replace(MIRROR, MIRROR + "\n https://extra.example"),
             ("continuation", 1, 3, None, None, 6, 0)),
            (A.replace("Enabled: yes", "Enabled: no"),
             ("enabled_not_admitted", 1, 6, "Enabled", "disabled", 5, 0)),
            (A.replace("Enabled: yes", "Enabled: YES"),
             ("enabled_not_admitted", 1, 6, "Enabled", "other", 5, 0)),
            ("", ("empty_source", None, None, None, None, 0, 0)),
            ("# comment\n\n", ("empty_source", None, None, None, None, 0, 0)),
            (A + "\nTypes: deb\nURIs: https://third-party.example\n",
             ("uri_not_admitted", 2, 9, "URIs", "other", 1, 1)),
        )
        for index, (source, expected) in enumerate(cases):
            with self.subTest(example=index):
                self.assert_refused(source, refusal(*expected))

    def test_all_diagnostic_only_fields_are_labeled_but_refused(self):
        for label in DIAGNOSTIC_FIELDS:
            with self.subTest(field=label):
                self.assert_refused(A + label + ": private\n",
                    refusal("unknown_field", 1, 7, label, None, 6, 0))

    def test_unlisted_field_names_and_case_variants_stay_opaque(self):
        for name in ("X-Secret", "snapshot", "SNAPSHOT", "Snapshot ", "Languages-Add",
                     "Targets-Add", "URIS", "architectures", "X-\x1b[31m", "X-秘密"):
            with self.subTest(name=name):
                self.assert_refused(A + name + ": opaque\n",
                    refusal("unknown_field", 1, 7, "<unknown>", None, 6, 0))

    def test_duplicate_admitted_field_labels_stay_closed(self):
        for label in ("Types", "URIs", "Suites", "Components", "Signed-By", "Enabled"):
            with self.subTest(field=label):
                self.assert_refused(A + label + ": opaque\n",
                    refusal("duplicate_field", 1, 7, label, None, 6, 0))
        self.assert_refused(A + "Architectures: amd64\nArchitectures: opaque\n",
            refusal("duplicate_field", 1, 8, "Architectures", None, 7, 0))

    def test_all_twelve_direct_uri_forms_are_labeled_but_refused(self):
        for uri, label in URI_CLASSES:
            with self.subTest(uri=uri):
                self.assert_refused(A.replace(MIRROR, uri),
                    refusal("uri_not_admitted", 1, 2, "URIs", label, 5, 0))

    def test_uri_classification_never_normalizes_or_exposes_values(self):
        cases = (
            ("", "empty"), (" \t ", "empty"),
            ("https://u:secret@archive.ubuntu.com/ubuntu/", "other"),
            ("https://archive.ubuntu.com:443/ubuntu/", "other"),
            ("https://archive.ubuntu.com/ubuntu/?token=s", "other"),
            ("https://archive.ubuntu.com/ubuntu/#opaque", "other"),
            ("https://archive.ubuntu.com/other", "other"),
            ("https://archive.ubuntu.com/ubuntu//", "other"),
            ("HTTPS://archive.ubuntu.com/ubuntu/", "other"),
            ("https://Archive.Ubuntu.com/ubuntu/", "other"),
            ("https://archive.ubuntu.com.evil.example/ubuntu/", "other"),
            ("https://archive.ubuntu.com/ubuntu/%2f", "other"),
            ("mirror+file:/other-list", "other"),
            ("https://archive.ubuntu.com/ubuntu/ https://extra.example", "multiple"),
            ("https://security.ubuntu.com/ubuntu\thttps://archive.ubuntu.com/ubuntu", "multiple"),
            ("  https://azure.archive.ubuntu.com/ubuntu/  ", "azure_archive"),
        )
        for uri, label in cases:
            with self.subTest(uri=uri):
                self.assert_refused(A.replace(MIRROR, uri),
                    refusal("uri_not_admitted", 1, 2, "URIs", label, 5, 0))

    def test_refusal_precedence_is_structural_then_stanza_then_uri(self):
        cases = (
            # Structural error in stanza 2 beats semantic URI/Enabled errors in stanza 1.
            ("URIs: https://extra.example\nEnabled: no\n\nURIs: " + MIRROR + "\nSnapshot: private\n",
             ("unknown_field", 2, 5, "Snapshot", None, 1, 1)),
            # First structural line wins even over a later higher-priority error kind.
            (A + "X-Secret: opaque\n continued\n",
             ("unknown_field", 1, 7, "<unknown>", None, 7, 0)),
            (A + " URIs no colon\n", ("continuation", 1, 7, None, None, 6, 0)),
            (A + "\tSnapshot: private\n", ("continuation", 1, 7, None, None, 6, 0)),
            (A + "Snapshot no colon\n", ("malformed_line", 1, 7, None, None, 6, 0)),
            ("Enabled: no\nURIs: https://extra.example\n",
             ("uri_not_admitted", 1, 2, "URIs", "other", 1, 0)),
            ("Enabled: no\nTypes: deb\n", ("missing_uri", 1, None, "URIs", "missing", 2, 0)),
            ("URIs: " + MIRROR + "\nEnabled: no\n\nURIs: https://extra.example\n",
             ("enabled_not_admitted", 1, 2, "Enabled", "disabled", 1, 1)),
        )
        for index, (source, expected) in enumerate(cases):
            with self.subTest(precedence=index):
                self.assert_refused(source, refusal(*expected))

    def test_splitlines_positions_and_comment_only_blocks(self):
        source = ("# only comment\n\nURIs: mirror+file:/etc/apt/apt-mirrors.txt\n"
                  " \t\n  # another comment-only block\n\n"
                  "Types: deb\n # inline ignored comment\nURIs: https://extra.example\n"
                  "Enabled: yes\n\n# final comment\n")
        # Only two evidence stanzas; selected block has three noncomment lines.
        for separator in ("\n", "\r\n", "\v", "\f", "\x85", "\u2028", "\u2029"):
            with self.subTest(separator=repr(separator)):
                self.assert_refused(source.replace("\n", separator),
                    refusal("uri_not_admitted", 2, 9, "URIs", "other", 2, 1))

    def test_large_opaque_input_has_exact_counts_and_complete_bounded_records(self):
        # 6 A lines + unknown + 1000 duplicate lines; 100 other evidence stanzas.
        source = (A + "X-Secret: " + "opaque" * 10000 + "\n"
                  + "Types: deb\n" * 1000 + ("\n" + A) * 100)
        self.assert_refused(source, refusal("unknown_field", 1, 7, "<unknown>", None, 1006, 100))
        with AptSandbox(self) as sandbox:
            os.environ["ImageVersion"] = "synthetic-secret" * 10000
            self.assertEqual(apt.main(), 0)
            self.assert_records(sandbox, [runner(image_version=None, redacted=("image_version",))])
            self.assertNotIn("synthetic-secret", sandbox.stdout.getvalue() + sandbox.stderr.getvalue())

    def test_source_unicode_error_is_a_behavioral_failure_not_an_unhandled_error(self):
        failures = (UnicodeDecodeError("utf-8", b"\xff", 0, 1, "synthetic invalid source"),
                    UnicodeError("synthetic decoding failure"))
        for failure in failures:
            with self.subTest(kind=type(failure).__name__), AptSandbox(self, failure) as sandbox:
                # Only this expected source failure is caught; setup/import/fixture errors escape.
                try:
                    result = apt.main()
                except UnicodeError as escaped:
                    self.assertIs(escaped, failure)
                    self.fail("source UnicodeError escaped main; expected main1/fixed decoding message")
                self.assertEqual(result, 1)
                self.assertEqual(sandbox.stderr.getvalue(),
                                 "Ubuntu APT setup failed: source text decoding failed\n")
                self.assert_records(sandbox, [])
                self.assertEqual(sandbox.calls, [])
                sandbox.validation.assert_not_called()
                self.assert_read_once(sandbox)

    def test_record_write_and_flush_failures_stop_before_children(self):
        faults = ("write_oserror", "write_valueerror", "write_none", "write_float",
                  "write_bool", "write_string", "write_short", "write_overcount",
                  "flush_oserror", "flush_valueerror")
        refused_line = refusal("unknown_field", 1, 7, "Snapshot", None, 6, 0)
        for target in ("runner", "source_refused"):
            for fault in faults:
                with self.subTest(target=target, fault=fault), AptSandbox(
                        self, A if target == "runner" else A + "Snapshot: private\n",
                        target=target, fault=fault) as sandbox:
                    self.assertEqual(apt.main(), 1)
                    self.assertEqual(sandbox.calls, [])
                    self.assert_read_once(sandbox)
                    self.assertTrue(sandbox.stderr.failed, "record failure must reach the controlled writer")
                    if target == "runner":
                        sandbox.validation.assert_not_called()
                    else:
                        sandbox.validation.assert_called_once_with(A + "Snapshot: private\n")
                    prior = "" if target == "runner" else RUNNER_LINE
                    attempted = RUNNER_LINE if target == "runner" else refused_line
                    if fault in ("write_oserror", "write_valueerror"):
                        retained = ""
                    elif fault == "write_short":
                        retained = attempted[:-1]
                    else:
                        retained = attempted
                    self.assertEqual(sandbox.stderr.getvalue(), prior + retained + UNAVAILABLE)
                    self.assertEqual(sandbox.stderr.attempts.count(attempted), 1)
                    self.assertEqual(sum(part.startswith(UNAVAILABLE.rstrip("\n"))
                                         for part in sandbox.stderr.attempts), 1)
                    self.assertNotIn(UNSUPPORTED, sandbox.stderr.getvalue())
                    if prior:
                        position = sandbox.trace.index(("write", prior))
                        self.assertEqual(sandbox.trace[position + 1], ("flush",))

    def test_unavailable_stderr_gets_one_best_effort_error_without_recursion(self):
        for target in ("runner", "source_refused"):
            for fault in ("write_oserror", "write_valueerror", "flush_oserror", "flush_valueerror"):
                with self.subTest(target=target, fault=fault), AptSandbox(
                        self, A if target == "runner" else A + "Snapshot: private\n",
                        target=target, fault=fault, persistent=True) as sandbox:
                    self.assertEqual(apt.main(), 1)
                    self.assertEqual(sandbox.calls, [])
                    self.assertTrue(sandbox.stderr.failed)
                    self.assertEqual(sum(part.startswith(UNAVAILABLE.rstrip("\n"))
                                         for part in sandbox.stderr.attempts), 1)
                    self.assertEqual(len(sandbox.stderr.attempts), 2 if target == "runner" else 3)
                    self.assertNotIn(UNSUPPORTED, "".join(sandbox.stderr.attempts))
                    if target == "runner":
                        sandbox.validation.assert_not_called()
                    else:
                        self.assertTrue(sandbox.stderr.getvalue().startswith(RUNNER_LINE))
                    self.assert_read_once(sandbox)

    def test_checked_update_keeps_inherited_output_and_never_retries(self):
        # Existing behavior, NOT intended red: no new protocol is required by this case.
        responses = (0, 100, subprocess.TimeoutExpired(UPDATE, 300),
                     FileNotFoundError("synthetic sudo unavailable"))
        for response in responses:
            with self.subTest(response=repr(response)), AptSandbox(
                    self, update_response=response) as sandbox:
                self.assertEqual(apt.main(), 0 if response == 0 else 1)
                updates = [(command, options) for command, options in sandbox.calls if command == UPDATE]
                self.assertEqual(updates, [(UPDATE, {"check": True, "timeout": 300})])
                self.assertEqual(sandbox.calls[-1], updates[0])
                self.assertEqual(len(sandbox.calls), 10, "no repeated diagnostic/write/update sequence")
                if response != 0:
                    self.assertIn("Ubuntu APT setup failed:", sandbox.stderr.getvalue())
                self.assertNotIn("update_attempt", sandbox.stderr.getvalue())
                self.assert_read_once(sandbox)


if __name__ == "__main__":
    unittest.main()
