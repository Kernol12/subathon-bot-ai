#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Direct native ABI behavior baseline; review and green foundations precede execution.

Import defines only stdlib helpers/tests. No old Python validator, fault DSO,
successful substitute or candidate package is imported by this module.
"""

import argparse
import contextlib
import ctypes
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest


DRAFT = "https://json-schema.org/draft/2020-12/schema"


class SchemaTransportNotImplemented(RuntimeError):
    """The actual inert native ABI returned its reviewed missing-feature value."""

    def __init__(self):
        super().__init__("Native schema transport is intentionally unimplemented")


def write_json(path, value):
    path.write_bytes(json.dumps(value, ensure_ascii=False, separators=(",", ":")).encode("utf-8"))


class NativeTransportAcceptance(unittest.TestCase):
    artifact = None

    @classmethod
    def setUpClass(cls):
        # Setup failures are hard fixture errors, NEVER the native missing-feature red.
        overrides = ("_PYTHON_SYSCONFIGDATA_NAME", "_PYTHON_SYSCONFIGDATA_PATH", "_PYTHON_PROJECT_BASE")
        if any(key in os.environ for key in overrides):
            raise RuntimeError("Native acceptance requires neutral sysconfig environment")
        import sysconfig

        minor = sys.version_info.minor
        if (sys.implementation.name != "cpython" or sys.version_info.major != 3 or
                minor not in (10, 12, 14) or sys.platform != "linux" or
                os.uname().machine != "x86_64" or sys.byteorder != "little" or
                sysconfig.get_config_var("SOABI") != "cpython-3" + str(minor) + "-x86_64-linux-gnu" or
                bool(sysconfig.get_config_var("Py_DEBUG")) or
                bool(sysconfig.get_config_var("Py_GIL_DISABLED"))):
            raise RuntimeError("Native acceptance requires an admitted ordinary CPython ABI")
        if (ctypes.sizeof(ctypes.c_int), ctypes.sizeof(ctypes.c_uint),
                ctypes.sizeof(ctypes.c_void_p)) != (4, 4, 8):
            raise RuntimeError("Native acceptance ABI width mismatch")
        interpreter = Path(sys.executable)
        if (not interpreter.is_absolute() or sys.prefix == sys.base_prefix or
                Path(sys.prefix).resolve(strict=True) != interpreter.parent.parent.resolve(strict=True)):
            raise RuntimeError("Native acceptance requires the dedicated stdlib venv")
        purelib = Path(sysconfig.get_path("purelib")).resolve(strict=True)
        platlib = Path(sysconfig.get_path("platlib")).resolve(strict=True)
        if purelib != platlib or not purelib.is_dir():
            raise RuntimeError("Native acceptance requires one admitted SDK directory")
        root = Path(__file__).resolve(strict=True).parents[2]
        worker = (root / "tools" / "schema_validation" / "worker.py").resolve(strict=True)
        if not worker.is_file():
            raise RuntimeError("Native acceptance real worker is missing")
        expected = root / "build" / "schema-transport-debug" / "lib" / "libsubathon_schema_transport_v1.so"
        if type(cls.artifact) is not str or not Path(cls.artifact).is_absolute():
            raise RuntimeError("Native acceptance requires the explicit built artifact")
        artifact = Path(cls.artifact).resolve(strict=True)
        if artifact != expected.resolve(strict=True) or not artifact.is_file():
            raise RuntimeError("Native acceptance artifact is outside the admitted build")
        cls.interpreter = str(interpreter).encode("utf-8")  # Preserve venv executable spelling.
        cls.worker = str(worker).encode("utf-8")
        cls.sdk = str(purelib).encode("utf-8")
        if any(not 1 <= len(value) <= 4096 or b"\0" in value
               for value in (cls.interpreter, cls.worker, cls.sdk)):
            raise RuntimeError("Native acceptance trusted path is unrepresentable")
        cls.library = ctypes.CDLL(str(artifact), mode=os.RTLD_NOW | os.RTLD_LOCAL)
        cls.transport = cls.library.subathon_schema_transport_v1
        byte_pointer = ctypes.POINTER(ctypes.c_ubyte)
        cls.transport.argtypes = [byte_pointer, ctypes.c_uint] * 4
        cls.transport.restype = ctypes.c_int
        # No call, custom converter, errcheck, callback or package import occurs here.

    def package(self, document=None, instance=1):
        temporary = tempfile.TemporaryDirectory(prefix="schema-native-literal-")
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        write_json(root / "s.json", document if document is not None else
                   {"$schema": DRAFT, "type": "integer"})
        write_json(root / "v.json", instance)
        write_json(root / "manifest.json", {
            "profile": "subathon.schema-package.v1",
            "documents": [{"path": "s.json", "kind": "jsonschema202012"}],
            "examples": [{"path": "v.json", "schema_document": "s.json", "schema_pointer": ""}],
        })
        return root

    def observe(self, manifest, expected, trusted=None):
        """Call the REAL eight-argument C ABI; 31 always raises the named marker."""
        paths = (manifest, *(trusted if trusted is not None else
                              (self.interpreter, self.worker, self.sdk)))
        self.assertEqual(len(paths), 4)
        for value in paths:
            self.assertIs(type(value), bytes)
            self.assertLessEqual(len(value), 4096)
        # Each immutable bytes buffer includes its explicit trailing NUL and stays
        # alive across the call. Invalid spellings remain deliberately bounded.
        buffers = tuple(value + b"\0" for value in paths)
        byte_pointer = ctypes.POINTER(ctypes.c_ubyte)
        arguments = []
        for payload, buffer in zip(paths, buffers):
            arguments.extend((ctypes.cast(ctypes.c_char_p(buffer), byte_pointer), len(payload)))
        with contextlib.ExitStack() as owner:
            stdout = owner.enter_context(tempfile.TemporaryFile())
            stderr = owner.enter_context(tempfile.TemporaryFile())
            saved_stdout = os.dup(1)
            owner.callback(os.close, saved_stdout)
            saved_stderr = os.dup(2)
            owner.callback(os.close, saved_stderr)
            with contextlib.ExitStack() as redirects:
                redirects.callback(os.dup2, saved_stdout, 1)
                os.dup2(stdout.fileno(), 1)
                redirects.callback(os.dup2, saved_stderr, 2)
                os.dup2(stderr.fileno(), 2)
                actual = type(self).transport(*arguments)
            if type(actual) is int and actual == 31:
                raise SchemaTransportNotImplemented()
            self.assertIs(type(actual), int)
            self.assertGreaterEqual(actual, 0)
            self.assertLessEqual(actual, 18)
            self.assertNotEqual(actual, 1, "CLI-only usage_error is not a transport result")
            stdout.seek(0)
            stderr.seek(0)
            self.assertEqual(stdout.read(257), b"")
            self.assertEqual(stderr.read(257), b"")
            self.assertEqual(actual, expected)

    def observe_package(self, root, expected):
        self.observe(str(root / "manifest.json").encode("utf-8"), expected)

    def test_real_draft_package(self):
        self.observe_package(self.package(), 0)

    def test_real_openapi_patch_versions(self):
        for version in ("3.1.0", "3.1.1", "3.1.2"):
            with self.subTest(version=version):
                root = self.package()
                write_json(root / "api.json", {
                    "openapi": version, "info": {"title": "Synthetic", "version": "1"}, "paths": {},
                })
                write_json(root / "manifest.json", {
                    "profile": "subathon.schema-package.v1",
                    "documents": [{"path": "api.json", "kind": "openapi31"}], "examples": [],
                })
                self.observe_package(root, 0)

    def test_manifest_path_admission(self):
        for index, path in enumerate((b"", b"\0", b"\xff", b"a\0b")):
            with self.subTest(case=index):
                self.observe(path, 2)

    def test_trusted_path_admission(self):
        root = self.package()
        rows = ((b"", self.worker, self.sdk),
                (b"relative-python", self.worker, self.sdk),
                (self.interpreter, b"relative-worker", self.sdk),
                (self.interpreter, self.worker, b"relative-sdk"))
        for index, trusted in enumerate(rows):
            with self.subTest(case=index):
                self.observe(str(root / "manifest.json").encode("utf-8"), 4, trusted)

    def test_missing_manifest_and_member(self):
        for missing in ("manifest.json", "v.json"):
            with self.subTest(missing=missing):
                root = self.package()
                (root / missing).unlink()
                self.observe_package(root, 3)

    def test_strict_json_refusal(self):
        root = self.package()
        (root / "v.json").write_bytes(b'{"PRIVATE":')
        self.observe_package(root, 6)

    def test_closed_manifest_refusal(self):
        root = self.package()
        write_json(root / "manifest.json", {
            "profile": "subathon.schema-package.v1",
            "documents": [{"path": "s.json", "kind": "jsonschema202012"}],
            "examples": [], "PRIVATE-extra": True,
        })
        self.observe_package(root, 7)

    def test_manifest_byte_limit(self):
        root = self.package()
        (root / "manifest.json").write_bytes(b" " * 65537)
        self.observe_package(root, 5)

    def test_dialect_and_format_refusals(self):
        rows = (({"$schema": "https://json-schema.org/draft/2019-09/schema"}, 8),
                ({"$schema": DRAFT, "format": "PRIVATE-unsupported"}, 9))
        for index, (document, expected) in enumerate(rows):
            with self.subTest(case=index):
                self.observe_package(self.package(document), expected)

    def test_native_invalid_schema(self):
        self.observe_package(self.package({"$schema": DRAFT, "type": 7}), 11)

    def test_real_invalid_instance(self):
        self.observe_package(self.package(instance="PRIVATE-literal"), 16)

    def test_complete_unresolved_reference(self):
        self.observe_package(self.package({
            "$schema": DRAFT, "$defs": {"unused": {"$ref": "missing.json"}},
        }), 12)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Reviewed direct native schema behavior baseline")
    parser.add_argument("--artifact", required=True)
    options = parser.parse_args(argv)
    NativeTransportAcceptance.artifact = options.artifact
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(NativeTransportAcceptance)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
