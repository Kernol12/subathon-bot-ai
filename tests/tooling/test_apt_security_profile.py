# SPDX-License-Identifier: GPL-3.0-or-later
"""CI-003 additive tests, authored for independent review before execution.

Oracles: ci-noble-security-source-v1 §§2–4 (e41a830), CI-002 v1 and C-A1.
P1/P2 are synthetic contract examples, never failed-host source captures.
"""
from contextlib import ExitStack, contextmanager
from pathlib import Path
import socket
import subprocess
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_apt_diagnostics as diagnostics

# Keep the fixture's exact producer object; importing the module does not expose
# its TestCase as a class in this module (which would duplicate discovery).
apt = diagnostics.apt

P1 = ("Types: deb\n"
      "URIs: mirror+file:/etc/apt/apt-mirrors.txt\n"
      "Suites: noble noble-updates noble-backports\n"
      "Components: main universe restricted multiverse\n"
      "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n"
      "\n"
      "Types: deb\n"
      "URIs: http://security.ubuntu.com/ubuntu/\n"
      "Suites: noble-security\n"
      "Components: main universe restricted multiverse\n"
      "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n")

P2 = ("## synthetic template-shaped header\n"
      "# comments do not count as fields\n"
      "\n"
      "Types: deb\n"
      "URIs: mirror+file:/etc/apt/apt-mirrors.txt\n"
      "# comment inside archive stanza\n"
      "Suites: noble noble-updates noble-backports\n"
      "Components: main universe restricted multiverse\n"
      "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n"
      "\n"
      "## synthetic security updates\n"
      "# comment block before fields\n"
      "Types: deb\n"
      "URIs: https://security.ubuntu.com/ubuntu\n"
      "Suites: noble-security\n"
      "# comment inside security stanza\n"
      "Components: main universe restricted multiverse\n"
      "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n"
      "# trailing comment\n")

ADMITTED = ('CI_APT_V1 {"event":"source_admitted",'
            '"profile":"noble_split_security_v1","v":1}\n')
SECURITY_REFUSAL = ("uri_not_admitted", 2, 8, "URIs", "ubuntu_security", 4, 1)


@contextmanager
def security_sandbox(case, source, **options):
    """Extend, without altering, the reviewed fixture's I/O/process barriers.

    These five legacy DNS barriers supplement its socket/getaddrinfo barriers;
    static implementation review must still audit every new I/O/lookup surface.
    """
    with ExitStack() as stack:
        for name in ("gethostbyname", "gethostbyname_ex", "gethostbyaddr",
                     "getfqdn", "getnameinfo"):
            stack.enter_context(patch.object(
                socket, name, side_effect=AssertionError("unexpected DNS lookup")))
        with diagnostics.AptSandbox(case, source, **options) as sandbox:
            yield sandbox


class AptSecurityProfileTests(unittest.TestCase):
    def assert_source_read(self, sandbox, source):
        sandbox.os_release.assert_called_once_with()
        sandbox.read_source.assert_called_once_with(
            diagnostics.SOURCE_PATH, encoding="utf-8")
        sandbox.validation.assert_called_once_with(source)

    def assert_admitted(self, sandbox, source, result, runner_line=diagnostics.RUNNER_LINE):
        self.assertEqual(result, 0, "complete reviewed split profile must reach update")
        self.assert_source_read(sandbox, source)
        self.assertEqual(sandbox.stderr.getvalue(), runner_line + ADMITTED)
        self.assertEqual(sandbox.stderr.attempts, [runner_line, ADMITTED])
        self.assertEqual(len(sandbox.stderr.attempts[1].encode("ascii")), 80)
        self.assertIsNone(sandbox.validation_error)
        # Assert the entire prefix before indexing any trace or child call.
        self.assertEqual(sandbox.trace[:9], [
            ("os_release",), ("source_read",), ("write", runner_line),
            ("flush",), ("validate",), ("write", ADMITTED), ("flush",),
            ("validate_end",), ("command",)])
        self.assertEqual(len(sandbox.calls), 10, "seven diagnostics, two writes, one update")
        self.assertEqual(sandbox.calls[-1],
                         (diagnostics.UPDATE, {"check": True, "timeout": 300}))

    def assert_refused(self, source, expected):
        # The reviewed wire formatter only serializes these independent literal
        # tuples; no producer table/parser supplies an expected result.
        line = diagnostics.refusal(*expected)
        with security_sandbox(self, source) as sandbox:
            self.assertEqual(apt.main(), 1, "ineligible profile must retain v1 refusal")
            self.assert_source_read(sandbox, source)
            self.assertEqual(sandbox.calls, [])
            self.assertEqual(sandbox.stdout.getvalue(), "")
            self.assertEqual(sandbox.stderr.getvalue(),
                             diagnostics.RUNNER_LINE + line + diagnostics.UNSUPPORTED)
            self.assertNotIn(ADMITTED, "".join(sandbox.stderr.attempts))
            self.assertEqual(sandbox.stderr.attempts.count(line), 1)
            self.assertIsInstance(sandbox.validation_error, RuntimeError)
            self.assertEqual(sandbox.trace[:8], [
                ("os_release",), ("source_read",), ("write", diagnostics.RUNNER_LINE),
                ("flush",), ("validate",), ("write", line), ("flush",),
                ("validate_end",)])

    def test_reviewed_split_profile_is_admitted(self):
        """The sole selected red; first establish the exact current-main baseline."""
        with security_sandbox(self, P1) as sandbox:
            result = apt.main()
            self.assert_source_read(sandbox, P1)
            self.assertIn(result, (0, 1), "main must return a normal setup result")
            self.assertTrue(sandbox.stderr.getvalue().startswith(diagnostics.RUNNER_LINE))
            self.assertEqual(sandbox.stderr.attempts.count(diagnostics.RUNNER_LINE), 1)
            if result == 1:
                expected = (
                    'CI_APT_V1 {"event":"runner","image_os":"ubuntu24",'
                    '"image_version":"20261111.127.1","os_id":"ubuntu","os_patch":"24.04.5",'
                    '"os_version":"24.04","redacted_fields":[],"unknown_fields":[],"v":1}\n'
                    'CI_APT_V1 {"event":"source_refused","field":"URIs","fields_omitted":4,'
                    '"line":8,"reason":"uri_not_admitted","stanza":2,"stanzas_omitted":1,'
                    '"v":1,"value_class":"ubuntu_security"}\n'
                    'Ubuntu APT setup failed: unsupported Ubuntu APT source layout\n')
                self.assertEqual(sandbox.stderr.getvalue(), expected)
                self.assertEqual(sandbox.calls, [], "baseline refusal must precede every child")
            self.assertEqual(result, 0,
                             'intended assertion red: CI_SECURITY_SPLIT_PROFILE_NOT_ADMITTED')
            self.assert_admitted(sandbox, P1, result)

    def test_all_four_literal_security_addresses(self):
        for uri in ("http://security.ubuntu.com/ubuntu",
                    "http://security.ubuntu.com/ubuntu/",
                    "https://security.ubuntu.com/ubuntu",
                    "https://security.ubuntu.com/ubuntu/"):
            source = P1.replace("http://security.ubuntu.com/ubuntu/", uri)
            with self.subTest(uri=uri), security_sandbox(self, source) as sandbox:
                self.assert_admitted(sandbox, source, apt.main())

    def test_admission_allowlist_is_independent_of_diagnostic_labels(self):
        # Removing a label must not revoke exact profile admission; adding a
        # label must not turn an unreviewed address into a supported source.
        # Code review also checks a separate literal allowlist: runtime patches
        # cannot detect a set derived from diagnostic labels at import time.
        with patch.dict(apt._PUBLIC_URI_LABELS, {}, clear=True):
            with security_sandbox(self, P1) as sandbox:
                self.assert_admitted(sandbox, P1, apt.main())
        with patch.dict(apt._PUBLIC_URI_LABELS,
                        {"https://unreviewed.example/ubuntu": "ubuntu_security"}):
            self.assert_refused(P1.replace("http://security.ubuntu.com/ubuntu/",
                                          "https://unreviewed.example/ubuntu"), SECURITY_REFUSAL)

    def test_field_order_token_order_and_ascii_value_whitespace(self):
        source = ("Signed-By:\t/usr/share/keyrings/ubuntu-archive-keyring.gpg \t\n"
                  "Components: multiverse\t restricted  universe \t\tmain\n"
                  "Suites:\tnoble-backports  \t noble-updates\t\tnoble \t\n"
                  "URIs: \tmirror+file:/etc/apt/apt-mirrors.txt\t \n"
                  "Types:\tdeb \t\n\n"
                  "Components: restricted \t main\t\tmultiverse  universe\n"
                  "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\t\n"
                  "URIs:\thttps://security.ubuntu.com/ubuntu/ \n"
                  "Types: deb\t\n"
                  "Suites:\t noble-security\t \n")
        with security_sandbox(self, source) as sandbox:
            self.assert_admitted(sandbox, source, apt.main())

    def test_profile_admission_does_not_require_image_metadata(self):
        expected_runner = (
            'CI_APT_V1 {"event":"runner","image_os":null,"image_version":null,'
            '"os_id":"ubuntu","os_patch":"24.04.5","os_version":"24.04",'
            '"redacted_fields":[],"unknown_fields":["image_os","image_version"],"v":1}\n')
        with security_sandbox(self, P1, env={
                "GITHUB_ACTIONS": "true", "RUNNER_ENVIRONMENT": "github-hosted"}) as sandbox:
            self.assert_admitted(sandbox, P1, apt.main(), expected_runner)

    def test_p2_comments_empty_blocks_and_decoded_line_endings(self):
        # These strings test validate_sources' decoded boundary. read_text's
        # unchanged universal-newline translation hides on-disk CR/CRLF spelling.
        for case_index, base in enumerate((
                P1, P2, P1.replace("\n\n", "\n\n# between\n\n\n"),
                "# Grüße\n\n\n" + P2 + "\n# 尾\n\n")):
            for ending in ("lf", "crlf", "mixed"):
                lines = base.rstrip("\n").split("\n")
                if ending == "lf":
                    source = "\n".join(lines)
                elif ending == "crlf":
                    source = "\r\n".join(lines)
                else:
                    source = "".join(line + ("\r\n" if i % 2 else "\n")
                                     for i, line in enumerate(lines[:-1])) + lines[-1]
                for final in ("", "\n", "\r\n"):
                    with self.subTest(layout=case_index, ending=ending, final=repr(final)):
                        text = source + final
                        with security_sandbox(self, text) as sandbox:
                            self.assert_admitted(sandbox, text, apt.main())

    def test_exact_types_suites_components_and_keyrings_in_each_stanza(self):
        # Each mutation changes one field only. Indices are fixed P1 line slots,
        # not positions discovered by a producer/parser or generated oracle.
        cases = (
            (0, "Types: deb deb-src"), (0, "Types: Deb"), (0, "Types:"),
            (0, "Types: deb # note"),
            (6, "Types: deb-src"), (6, "Types: deb deb"), (6, "Types: Deb"), (6, "Types:"),
            (2, "Suites:"),
            (2, "Suites: noble noble-updates"),
            (2, "Suites: noble noble-updates noble-backports noble-security"),
            (2, "Suites: noble noble noble-updates noble-backports"),
            (2, "Suites: Noble noble-updates noble-backports"),
            (8, "Suites: noble"), (8, "Suites:"),
            (8, "Suites: noble-security noble"),
            (8, "Suites: noble-security noble-security"),
            (8, "Suites: Noble-security"),
            (3, "Components:"), (3, "Components: main universe restricted"),
            (3, "Components: main universe restricted multiverse extra"),
            (3, "Components: main main universe restricted multiverse"),
            (3, "Components: Main universe restricted multiverse"),
            (9, "Components:"), (9, "Components: main universe restricted"),
            (9, "Components: main universe restricted multiverse extra"),
            (9, "Components: main universe restricted multiverse multiverse"),
            (9, "Components: main Universe restricted multiverse"),
            (4, "Signed-By:"), (4, "Signed-By: /other/keyring.gpg"),
            (4, "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg extra"),
            (4, "Signed-By: /usr/share/keyrings/Ubuntu-archive-keyring.gpg"),
            (10, "Signed-By: /other/keyring.gpg"),
            (10, "Signed-By:"),
            (10, "Signed-By: /usr/share/keyrings/Ubuntu-archive-keyring.gpg"),
            (10, "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg extra"),
            (10, "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg # note"),
            (10, "Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg/"),
        )
        for index, replacement in cases:
            with self.subTest(line=index + 1, replacement=replacement):
                lines = P1.split("\n")
                lines[index] = replacement
                self.assert_refused("\n".join(lines), SECURITY_REFUSAL)

    def test_every_required_key_and_no_optional_keys(self):
        missing = (
            (0, ("uri_not_admitted", 2, 7, "URIs", "ubuntu_security", 4, 1)),
            (1, ("missing_uri", 1, None, "URIs", "missing", 4, 1)),
            (2, ("uri_not_admitted", 2, 7, "URIs", "ubuntu_security", 4, 1)),
            (3, ("uri_not_admitted", 2, 7, "URIs", "ubuntu_security", 4, 1)),
            (4, ("uri_not_admitted", 2, 7, "URIs", "ubuntu_security", 4, 1)),
            (6, ("uri_not_admitted", 2, 7, "URIs", "ubuntu_security", 3, 1)),
            (7, ("missing_uri", 2, None, "URIs", "missing", 4, 1)),
            (8, ("uri_not_admitted", 2, 8, "URIs", "ubuntu_security", 3, 1)),
            (9, ("uri_not_admitted", 2, 8, "URIs", "ubuntu_security", 3, 1)),
            (10, ("uri_not_admitted", 2, 8, "URIs", "ubuntu_security", 3, 1)),
        )
        for index, expected in missing:
            with self.subTest(missing_line=index + 1):
                lines = P1.split("\n")
                del lines[index]
                self.assert_refused("\n".join(lines), expected)
        for field in ("Enabled: yes", "Architectures: amd64"):
            with self.subTest(extra=field, stanza=1):
                self.assert_refused(P1.replace("\n\n", "\n" + field + "\n\n"),
                                    ("uri_not_admitted", 2, 9, "URIs", "ubuntu_security", 4, 1))
            with self.subTest(extra=field, stanza=2):
                self.assert_refused(P1 + field + "\n",
                                    ("uri_not_admitted", 2, 8, "URIs", "ubuntu_security", 5, 1))

    def test_archive_uri_is_exact_mirror_literal(self):
        for uri, label in (
            ("http://azure.archive.ubuntu.com/ubuntu/", "azure_archive"),
            ("https://archive.ubuntu.com/ubuntu/", "ubuntu_archive"),
            ("http://security.ubuntu.com/ubuntu/", "ubuntu_security"),
            ("mirror+file:/etc/apt/apt-mirrors.txt/", "other"),
            ("MIRROR+file:/etc/apt/apt-mirrors.txt", "other"),
            ("", "empty"),
            ("mirror+file:/etc/apt/apt-mirrors.txt https://extra.example", "multiple"),
        ):
            with self.subTest(uri=uri):
                self.assert_refused(P1.replace("mirror+file:/etc/apt/apt-mirrors.txt", uri),
                                    ("uri_not_admitted", 1, 2, "URIs", label, 4, 1))

    def test_security_uri_requires_one_exact_allowlisted_literal(self):
        cases = (
            ("http://security.ubuntu.com/ubuntu//", "other"),
            ("http://user:secret@security.ubuntu.com/ubuntu/", "other"),
            ("http://security.ubuntu.com:80/ubuntu/", "other"),
            ("http://security.ubuntu.com/ubuntu/?query=1", "other"),
            ("http://security.ubuntu.com/ubuntu/#fragment", "other"),
            ("HTTP://security.ubuntu.com/ubuntu/", "other"),
            ("http://SECURITY.ubuntu.com/ubuntu/", "other"),
            ("http://security.ubuntu.com/%75buntu/", "other"),
            ("http://security.ubuntu.com/ubuntu/extra", "other"),
            ("https://unreviewed.example/ubuntu/", "other"),
            ("https://azure.archive.ubuntu.com/ubuntu/", "azure_archive"),
            ("http://archive.ubuntu.com/ubuntu", "ubuntu_archive"),
            ("", "empty"),
            ("http://security.ubuntu.com/ubuntu/ https://security.ubuntu.com/ubuntu", "multiple"),
        )
        for uri, label in cases:
            with self.subTest(uri=uri):
                self.assert_refused(P1.replace("http://security.ubuntu.com/ubuntu/", uri),
                                    ("uri_not_admitted", 2, 8, "URIs", label, 4, 1))

    def test_order_stanza_count_and_mixed_layouts_fall_back(self):
        archive, security = P1.split("\n\n")
        cases = (
            (security + "\n" + archive + "\n",
             ("uri_not_admitted", 1, 2, "URIs", "ubuntu_security", 4, 1)),
            (security, ("uri_not_admitted", 1, 2, "URIs", "ubuntu_security", 4, 0)),
            (P1 + "\nURIs: mirror+file:/etc/apt/apt-mirrors.txt\n",
             ("uri_not_admitted", 2, 8, "URIs", "ubuntu_security", 4, 2)),
            (P1.replace("\n\n", "\n"),
             ("duplicate_field", 1, 6, "Types", None, 9, 0)),
            (P1.replace("Suites: noble-security", "Suites: noble noble-security"),
             SECURITY_REFUSAL),
            ("URIs: http://security.ubuntu.com/ubuntu/\nSuites: noble-security\n",
             ("uri_not_admitted", 1, 1, "URIs", "ubuntu_security", 1, 0)),
        )
        for index, (source, expected) in enumerate(cases):
            with self.subTest(layout=index):
                self.assert_refused(source, expected)

    def test_structural_scan_finishes_before_profile_or_semantics(self):
        cases = (
            (P1 + "X-Test: opaque\n", ("unknown_field", 2, 12, "<unknown>", None, 5, 1)),
            (P1 + "Snapshot: opaque\n", ("unknown_field", 2, 12, "Snapshot", None, 5, 1)),
            (P1 + "Types: deb\n", ("duplicate_field", 2, 12, "Types", None, 5, 1)),
            (P1 + " URIs no colon\n", ("continuation", 2, 12, None, None, 5, 1)),
            (P1 + "\tSnapshot: opaque\n", ("continuation", 2, 12, None, None, 5, 1)),
            (P1 + "Snapshot no colon\n", ("malformed_line", 2, 12, None, None, 5, 1)),
            (P1 + "X-Test: opaque\n continued\n",
             ("unknown_field", 2, 12, "<unknown>", None, 6, 1)),
            (P1.replace("mirror+file:/etc/apt/apt-mirrors.txt", "https://extra.example")
             + "Snapshot: opaque\n", ("unknown_field", 2, 12, "Snapshot", None, 5, 1)),
            (P1 + "\nURIs: mirror+file:/etc/apt/apt-mirrors.txt\nX-Test: opaque\n",
             ("unknown_field", 3, 14, "<unknown>", None, 1, 2)),
            (P1.replace("Types: deb\n", "types: deb\n", 1),
             ("unknown_field", 1, 1, "<unknown>", None, 4, 1)),
            (P1.replace("Types: deb\n", "Types : deb\n", 1),
             ("unknown_field", 1, 1, "<unknown>", None, 4, 1)),
        )
        for index, (source, expected) in enumerate(cases):
            with self.subTest(structural=index):
                self.assert_refused(source, expected)

    def test_raw_non_ascii_and_control_field_values_are_ineligible(self):
        # v1 may strip these or treat a control as a line delimiter; neither
        # behavior can supply evidence of a match for the original raw string.
        for value in ("\u00a0", "\x1f", "\x00", "\x1b", "\x7f", "é"):
            for index, replacement in (
                (4, "Signed-By: " + value + "/usr/share/keyrings/ubuntu-archive-keyring.gpg"),
                (10, "Signed-By: " + value + "/usr/share/keyrings/ubuntu-archive-keyring.gpg"),
                (2, "Suites: noble" + value + "noble-updates noble-backports"),
                (9, "Components: main" + value + "universe restricted multiverse"),
            ):
                with self.subTest(raw=repr(value), line=index + 1):
                    lines = P1.split("\n")
                    lines[index] = replacement
                    self.assert_refused("\n".join(lines), SECURITY_REFUSAL)

    def test_raw_types_and_uri_prefixes_are_ineligible(self):
        # v1 strips NBSP/U+001F, while the other literal prefixes remain in
        # URI values. Types is not checked by v1; each case must still refuse.
        for prefix in ("\u00a0", "\x1f", "\x00", "\x1b", "\x7f", "é"):
            archive_refusal = SECURITY_REFUSAL
            security_refusal = SECURITY_REFUSAL
            if prefix not in ("\u00a0", "\x1f"):
                archive_refusal = ("uri_not_admitted", 1, 2, "URIs", "other", 4, 1)
                security_refusal = ("uri_not_admitted", 2, 8, "URIs", "other", 4, 1)
            cases = (
                (0, "Types: " + prefix + "deb", SECURITY_REFUSAL),
                (6, "Types: " + prefix + "deb", SECURITY_REFUSAL),
                (1, "URIs: " + prefix + "mirror+file:/etc/apt/apt-mirrors.txt",
                 archive_refusal),
                (7, "URIs: " + prefix + "http://security.ubuntu.com/ubuntu/",
                 security_refusal),
            )
            for index, replacement, expected in cases:
                with self.subTest(prefix=repr(prefix), line=index + 1):
                    lines = P1.split("\n")
                    lines[index] = replacement
                    self.assert_refused("\n".join(lines), expected)

    def test_only_column_zero_comments_and_truly_empty_separators_match(self):
        for indentation in (" ", "\t", "\u00a0", "\x1f"):
            with self.subTest(prefix=repr(indentation)):
                self.assert_refused(indentation + "# synthetic comment\n" + P1,
                                    ("uri_not_admitted", 2, 9, "URIs", "ubuntu_security", 4, 1))
            with self.subTest(inside=repr(indentation)):
                self.assert_refused(P1.replace(
                    "Components: main universe restricted multiverse\nSigned-By:",
                    "Components: main universe restricted multiverse\n" + indentation
                    + "# note\nSigned-By:", 1),
                    ("uri_not_admitted", 2, 9, "URIs", "ubuntu_security", 4, 1))
        # Normative closing correction: tab+# after SECURITY Components is
        # ignored by v1, so its line8/omitted4/1 URI refusal remains unchanged.
        self.assert_refused(P1.replace("Suites: noble-security\nComponents: main universe restricted multiverse\n",
                                      "Suites: noble-security\nComponents: main universe restricted multiverse\n\t# note\n"),
                            SECURITY_REFUSAL)
        for whitespace in (" ", "\t", " \t ", "\u00a0", "\x1f"):
            with self.subTest(separator=repr(whitespace)):
                self.assert_refused(P1.replace("\n\n", "\n" + whitespace + "\n"),
                                    SECURITY_REFUSAL)
            with self.subTest(leading_separator=repr(whitespace)):
                self.assert_refused(whitespace + "\n" + P1,
                                    ("uri_not_admitted", 2, 9, "URIs", "ubuntu_security", 4, 1))
            with self.subTest(trailing_separator=repr(whitespace)):
                self.assert_refused(P1 + whitespace + "\n", SECURITY_REFUSAL)

    def test_other_decoded_line_delimiters_are_ineligible(self):
        for separator in ("\r", "\v", "\f", "\x1c", "\x1d", "\x1e",
                          "\x85", "\u2028", "\u2029"):
            with self.subTest(separator=repr(separator)):
                self.assert_refused(P1.replace("\n", separator), SECURITY_REFUSAL)
            # A forbidden delimiter inside even a comment invalidates the raw
            # profile; v1 still sees two column-zero comments and a shifted P1.
            with self.subTest(comment_separator=repr(separator)):
                self.assert_refused("# before" + separator + "# after\n" + P1,
                                    ("uri_not_admitted", 2, 10, "URIs", "ubuntu_security", 4, 1))

    def test_mirror_only_legacy_ignored_lines_and_fields_stay_accepted(self):
        mirror_source = P1.replace("http://security.ubuntu.com/ubuntu/",
                                   "mirror+file:/etc/apt/apt-mirrors.txt")
        for case_index, source in enumerate((
                mirror_source, P1.split("\n\n")[0] + "\n",
                mirror_source.replace("\n\n", "\n\u00a0\n\t# ignored\n"),
                mirror_source + "Enabled: yes\nArchitectures: amd64\n",
                "URIs: mirror+file:/etc/apt/apt-mirrors.txt\nSuites: unknown\n")):
            with self.subTest(layout=case_index), security_sandbox(self, source) as sandbox:
                self.assertEqual(apt.main(), 0)
                self.assert_source_read(sandbox, source)
                self.assertEqual(sandbox.stderr.getvalue(), diagnostics.RUNNER_LINE)
                self.assertEqual(sandbox.stderr.attempts, [diagnostics.RUNNER_LINE])
                self.assertEqual(len(sandbox.calls), 10)
                self.assertEqual(sandbox.calls[-1],
                                 (diagnostics.UPDATE, {"check": True, "timeout": 300}))

    def test_admitted_setup_preserves_exact_commands_options_and_write_bytes(self):
        expected_mirrors = ("https://mirrors.edge.kernel.org/ubuntu/\tpriority:1\n"
                            "https://archive.ubuntu.com/ubuntu/\tpriority:2\n"
                            "https://security.ubuntu.com/ubuntu/\tpriority:3\n")
        expected_config = ('Acquire::Retries "1";\nAcquire::http::Timeout "15";\n'
                           'Acquire::https::Timeout "15";\nAPT::Update::Error-Mode "any";\n')
        with security_sandbox(self, P1) as sandbox:
            self.assert_admitted(sandbox, P1, apt.main())
            expected = [(command, {"check": False, "capture_output": True,
                                   "text": True, "timeout": 10})
                        for command in diagnostics.DIAGNOSTICS]
            expected += [
                (diagnostics.WRITES[0], {"input": expected_mirrors, "check": True,
                                        "text": True, "stdout": subprocess.DEVNULL, "timeout": 10}),
                (diagnostics.WRITES[1], {"input": expected_config, "check": True,
                                        "text": True, "stdout": subprocess.DEVNULL, "timeout": 10}),
                (diagnostics.UPDATE, {"check": True, "timeout": 300}),
            ]
            self.assertEqual(sandbox.calls, expected)

    def test_admission_record_count_type_write_and_flush_faults_stop_children(self):
        faults = ("write_oserror", "write_valueerror", "write_none", "write_float",
                  "write_bool", "write_string", "write_short", "write_overcount",
                  "flush_oserror", "flush_valueerror")
        for fault in faults:
            with self.subTest(fault=fault), security_sandbox(
                    self, P1, target="source_admitted", fault=fault) as sandbox:
                self.assertEqual(apt.main(), 1)
                self.assert_source_read(sandbox, P1)
                self.assertEqual(sandbox.calls, [])
                self.assertTrue(sandbox.stderr.failed, "admission emission must reach controlled fault")
                self.assertEqual(sandbox.stderr.attempts,
                                 [diagnostics.RUNNER_LINE, ADMITTED, diagnostics.UNAVAILABLE])
                self.assertNotIn(diagnostics.UNSUPPORTED, "".join(sandbox.stderr.attempts))
                retained = ("" if fault in ("write_oserror", "write_valueerror") else
                            ADMITTED[:-1] if fault == "write_short" else ADMITTED)
                self.assertEqual(sandbox.stderr.getvalue(),
                                 diagnostics.RUNNER_LINE + retained + diagnostics.UNAVAILABLE)
                self.assertEqual(sandbox.trace[:5], [
                    ("os_release",), ("source_read",), ("write", diagnostics.RUNNER_LINE),
                    ("flush",), ("validate",)])
                self.assertIn(("write", ADMITTED), sandbox.trace)
                self.assertIn(("validate_end",), sandbox.trace)
                self.assertLess(sandbox.trace.index(("write", ADMITTED)),
                                sandbox.trace.index(("validate_end",)))
                if fault.startswith("flush_"):
                    position = sandbox.trace.index(("write", ADMITTED))
                    self.assertEqual(sandbox.trace[position + 1:position + 2], [("flush",)])

    def test_persistent_admission_emission_failure_does_not_log_recursively(self):
        for fault in ("write_oserror", "flush_valueerror"):
            with self.subTest(fault=fault), security_sandbox(
                    self, P1, target="source_admitted", fault=fault, persistent=True) as sandbox:
                self.assertEqual(apt.main(), 1)
                self.assert_source_read(sandbox, P1)
                self.assertEqual(sandbox.calls, [])
                self.assertTrue(sandbox.stderr.failed, "new call site must fail before best-effort error")
                self.assertEqual(sandbox.stderr.attempts,
                                 [diagnostics.RUNNER_LINE, ADMITTED, diagnostics.UNAVAILABLE])
                self.assertEqual(sandbox.stderr.getvalue(), diagnostics.RUNNER_LINE
                                 + (ADMITTED if fault == "flush_valueerror" else ""))
                self.assertNotIn(diagnostics.UNSUPPORTED, "".join(sandbox.stderr.attempts))
                self.assertIn(("write", ADMITTED), sandbox.trace)
                self.assertIn(("validate_end",), sandbox.trace)
                self.assertLess(sandbox.trace.index(("write", ADMITTED)),
                                sandbox.trace.index(("validate_end",)))

    def test_later_update_failure_retains_admission_and_fails_once(self):
        for response in (100, subprocess.TimeoutExpired(diagnostics.UPDATE, 300),
                         FileNotFoundError("synthetic update unavailable")):
            with self.subTest(response=repr(response)), security_sandbox(
                    self, P1, update_response=response) as sandbox:
                self.assertEqual(apt.main(), 1)
                self.assert_source_read(sandbox, P1)
                records = [line for line in sandbox.stderr.getvalue().splitlines(keepends=True)
                           if line.startswith("CI_APT_V1 ")]
                self.assertEqual(records, [diagnostics.RUNNER_LINE, ADMITTED],
                                 "admission must precede later update failure")
                self.assertEqual(len(sandbox.calls), 10, "no repeated sequence or extra child")
                self.assertEqual(sandbox.calls[-1],
                                 (diagnostics.UPDATE, {"check": True, "timeout": 300}))
                self.assertEqual(sum(command == diagnostics.UPDATE for command, _ in sandbox.calls), 1)
                self.assertTrue(sandbox.stderr.getvalue().startswith(diagnostics.RUNNER_LINE + ADMITTED))
                self.assertIn("Ubuntu APT setup failed:", sandbox.stderr.getvalue())
                self.assertNotIn("source_refused", sandbox.stderr.getvalue())
                self.assertNotIn(diagnostics.UNSUPPORTED, sandbox.stderr.getvalue())
                self.assertIn(("write", ADMITTED), sandbox.trace)
                self.assertIn(("validate_end",), sandbox.trace)
                self.assertIn(("command",), sandbox.trace)
                position = sandbox.trace.index(("write", ADMITTED))
                self.assertEqual(sandbox.trace[position + 1:position + 3],
                                 [("flush",), ("validate_end",)])
                self.assertLess(position + 2, sandbox.trace.index(("command",)))


if __name__ == "__main__":
    unittest.main()
