# SPDX-License-Identifier: GPL-3.0-or-later
"""Unterminated raw-string boundaries through the real public parent/worker.

Definitions are inert. Independent review precedes the first reached-bug run.
"""
import contextlib
import importlib.util
import io
from pathlib import Path
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).parents[2] / "tools" / "schema_validate.py"
MANIFEST = (
    b'{"profile":"subathon.schema-package.v1",'
    b'"documents":[{"path":"schema.json","kind":"jsonschema202012"}],'
    b'"examples":[{"path":"value.json","schema_document":"schema.json",'
    b'"schema_pointer":""}]}'
)
SCHEMA = b'{"$schema":"https://json-schema.org/draft/2020-12/schema"}'


class WorkerLexicalRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("schema_validate_lexical_under_test", SCRIPT)
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        cls.target = module

    def unterminated_escape(self, byte_count, expected):
        # One opening quote + ASCII body + final backslash, with no closing quote.
        raw = b'"' + b"x" * (byte_count - 2) + b"\\"
        self.assertEqual(len(raw), byte_count)
        with tempfile.TemporaryDirectory(prefix="schema-lexical-") as temporary:
            root = Path(temporary)
            manifest = root / "manifest.json"
            manifest.write_bytes(MANIFEST)
            (root / "schema.json").write_bytes(SCHEMA)
            (root / "value.json").write_bytes(raw)
            stdout, stderr = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                actual = self.target.validate_package(str(manifest))
            self.assertIs(actual, self.target.SchemaValidationOutcome(expected))
            self.assertEqual(stdout.getvalue(), "")
            self.assertEqual(stderr.getvalue(), "")

    def test_unterminated_escape_65535_bytes_is_invalid_json(self):
        self.unterminated_escape(65535, "invalid_json")

    def test_unterminated_escape_65536_bytes_is_invalid_json(self):
        self.unterminated_escape(65536, "invalid_json")

    def test_unterminated_escape_65537_bytes_is_resource_limit(self):
        self.unterminated_escape(65537, "resource_limit")


if __name__ == "__main__":
    unittest.main(verbosity=2)
