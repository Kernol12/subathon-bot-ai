#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Binding admission source: no native call or successful validator substitute.

Definitions only on import. Different source review and explicit release precede
execution. The binding scaffold's own marker is distinct from actual native31.
"""

import builtins
import contextlib
import ctypes
import errno
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).parents[2]
SCRIPT = ROOT / "tools/schema_validate.py"
ARTIFACT_SUFFIX = "build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so"
SYMBOL = "subathon_schema_transport_v1"
# Literal ELF64 little-endian ET_DYN/x86-64/current-version/64-byte header.
# Never load this synthetic object: it contains no admitted native implementation.
ELF64 = (b"\x7fELF\x02\x01\x01\x00" + b"\0" * 8
         + b"\x03\x00\x3e\x00\x01\x00\x00\x00" + b"\0" * 28
         + b"\x40\x00" + b"\0" * 10)


def load_public():
    spec = importlib.util.spec_from_file_location("schema_native_binding_test_target", SCRIPT)
    target = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = target
    spec.loader.exec_module(target)
    return target


@contextlib.contextmanager
def header_reads(path):
    """Record genuine fixture reads through file-object or raw stdlib I/O."""
    selected = os.fspath(path)
    reads, descriptors = [], {}
    real_open, real_io_open = builtins.open, io.open
    real_os_open, real_read, real_pread = os.open, os.read, os.pread

    def identify(descriptor):
        state = os.fstat(descriptor)
        descriptors[descriptor] = state.st_dev, state.st_ino

    class FileView:
        def __init__(self, actual):
            self.actual = actual
            identify(actual.fileno())

        def __enter__(self):
            self.actual.__enter__()
            return self

        def __exit__(self, *failure):
            return self.actual.__exit__(*failure)

        def __getattr__(self, name):
            return getattr(self.actual, name)

        def read(self, *arguments):
            value = self.actual.read(*arguments)
            reads.append(value)
            return value

        def readinto(self, buffer):
            count = self.actual.readinto(buffer)
            reads.append(bytes(buffer[:count]))
            return count

    def tracked_file(opener, path, *arguments, **keywords):
        actual = opener(path, *arguments, **keywords)
        try:
            if isinstance(path, int):
                state = os.fstat(actual.fileno())
                selected_file = descriptors.get(path) == (state.st_dev, state.st_ino)
            else:
                selected_file = os.fspath(path) == selected
            return FileView(actual) if selected_file else actual
        except BaseException:
            actual.close()
            raise

    def tracked_open(path, *arguments, **keywords):
        descriptor = real_os_open(path, *arguments, **keywords)
        try:
            if os.fspath(path) == selected:
                identify(descriptor)
            return descriptor
        except BaseException:
            os.close(descriptor)
            raise

    def tracked_read(reader, descriptor, *arguments):
        value = reader(descriptor, *arguments)
        if descriptor in descriptors:
            state = os.fstat(descriptor)
            if descriptors[descriptor] == (state.st_dev, state.st_ino):
                reads.append(value)
        return value

    with mock.patch.object(builtins, "open", side_effect=lambda *a, **k: tracked_file(real_open, *a, **k)), \
            mock.patch.object(io, "open", side_effect=lambda *a, **k: tracked_file(real_io_open, *a, **k)), \
            mock.patch.object(os, "open", side_effect=tracked_open), \
            mock.patch.object(os, "read", side_effect=lambda *a: tracked_read(real_read, *a)), \
            mock.patch.object(os, "pread", side_effect=lambda *a: tracked_read(real_pread, *a)):
        yield reads


@contextlib.contextmanager
def no_backend_activity(target):
    """A pure mapping must neither touch resources nor silently swallow guards."""
    with contextlib.ExitStack() as stack:
        guards = [stack.enter_context(mock.patch.object(owner, name, side_effect=AssertionError("decoder did IO")))
                  for owner, name in ((target, "_load_native_transport"), (ctypes, "CDLL"),
                                      (builtins, "open"), (io, "open"), (os, "open"), (subprocess, "Popen"))]
        try:
            yield
        finally:
            for guard in guards:
                guard.assert_not_called()


class NativeBindingAcceptance(unittest.TestCase):
    def test_definition_only_import_has_no_native_or_file_side_effect(self):
        with mock.patch.object(ctypes, "CDLL", side_effect=AssertionError("import loaded native")) as load, \
                mock.patch.object(builtins, "open", side_effect=AssertionError("import opened file")) as builtin_open, \
                mock.patch.object(io, "open", side_effect=AssertionError("import opened file")) as file_open, \
                mock.patch.object(os, "open", side_effect=AssertionError("import opened descriptor")) as descriptor_open, \
                mock.patch.object(subprocess, "Popen", side_effect=AssertionError("import spawned")) as spawn:
            target = load_public()
        for guard in (load, builtin_open, file_open, descriptor_open, spawn):
            guard.assert_not_called()
        self.assertTrue(callable(target._load_native_transport))
        self.assertTrue(callable(target._decode_native_result))

    def test_actual_trusted_function_has_builtin_eight_argument_abi_and_local_now(self):
        target = load_public()
        real_cdll, libraries = ctypes.CDLL, []

        def actual_load(*arguments, **keywords):
            library = real_cdll(*arguments, **keywords)
            libraries.append(library)
            return library

        with mock.patch.object(ctypes, "CDLL", side_effect=actual_load) as load:
            entry = target._load_native_transport()  # No native invocation.
        load.assert_called_once_with(str(ROOT.resolve() / ARTIFACT_SUFFIX), mode=os.RTLD_NOW | os.RTLD_LOCAL)
        self.assertEqual(len(libraries), 1)
        self.assertIs(entry, getattr(libraries[0], SYMBOL))
        self.assertIsInstance(entry, ctypes._CFuncPtr)
        expected = (ctypes.POINTER(ctypes.c_ubyte), ctypes.c_uint) * 4
        self.assertEqual(len(entry.argtypes), 8)
        for actual, required in zip(entry.argtypes, expected):
            self.assertIs(actual, required)
        self.assertIs(entry.restype, ctypes.c_int)
        self.assertIsNone(getattr(entry, "errcheck", None))
        self.assertEqual(entry._flags_ & ctypes._FUNCFLAG_PYTHONAPI, 0)

    def test_missing_empty_and_short_fixed_artifacts_refuse_without_cdll(self):
        for payload in (None, b"", b"\x7fELF"):
            with self.subTest(payload=payload), tempfile.TemporaryDirectory(prefix="schema-binding-") as directory:
                target = load_public()
                script = Path(directory) / "tools/schema_validate.py"
                artifact = Path(directory) / ARTIFACT_SUFFIX
                script.parent.mkdir(parents=True); script.write_bytes(b"# synthetic trusted-location metadata\n")
                artifact.parent.mkdir(parents=True)
                if payload is not None:
                    artifact.write_bytes(payload)
                with mock.patch.object(target, "__file__", str(script)), \
                        mock.patch.object(ctypes, "CDLL", side_effect=AssertionError("synthetic artifact loaded")) as load:
                    actual = target._load_native_transport()
                self.assertIsNone(actual)
                load.assert_not_called()

    def test_wrong_elf_profiles_are_read_then_refused_without_cdll(self):
        rows = (("magic", 0, b"\0ELF"), ("class32", 4, b"\x01"),
                ("bigendian", 5, b"\x02"), ("identversion", 6, b"\x00"),
                ("relocatable", 16, b"\x01\x00"), ("aarch64", 18, b"\xb7\x00"),
                ("headersize", 52, b"\x3f\x00"))
        self.assertEqual(len(ELF64), 64)
        for label, offset, replacement in rows:
            with self.subTest(case=label), tempfile.TemporaryDirectory(prefix="schema-binding-") as directory:
                target = load_public()
                script = Path(directory) / "tools/schema_validate.py"
                artifact = Path(directory) / ARTIFACT_SUFFIX
                script.parent.mkdir(parents=True); script.write_bytes(b"# synthetic trusted-location metadata\n")
                artifact.parent.mkdir(parents=True)
                payload = ELF64[:offset] + replacement + ELF64[offset + len(replacement):]
                artifact.write_bytes(payload)
                with mock.patch.object(target, "__file__", str(script)), header_reads(artifact) as reads, \
                        mock.patch.object(ctypes, "CDLL", side_effect=AssertionError("synthetic artifact loaded")) as load:
                    actual = target._load_native_transport()
                self.assertIsNone(actual)
                load.assert_not_called()
                self.assertTrue(reads, "Malformed header was never actually inspected")
                self.assertTrue(all(type(value) is bytes for value in reads))
                self.assertIn(payload[:offset + len(replacement)], b"".join(reads))

    def test_loader_failure_classification_preserves_known_python_exceptions(self):
        rows = ((OSError(errno.ENOEXEC, "PRIVATE_NATIVE_LOADER_DETAIL"), False),
                (MemoryError("PRIVATE_NATIVE_LOADER_ALLOCATION"), True),
                (KeyboardInterrupt("PRIVATE_NATIVE_LOADER_INTERRUPT"), True))
        for failure, propagate in rows:
            with self.subTest(kind=type(failure).__name__):
                target = load_public()
                stdout, stderr = io.StringIO(), io.StringIO()
                with mock.patch.object(ctypes, "CDLL", side_effect=failure) as load, \
                        contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    if propagate:
                        with self.assertRaises(type(failure)) as caught:
                            target._load_native_transport()
                        self.assertIs(caught.exception, failure)
                    else:
                        self.assertIsNone(target._load_native_transport())
                load.assert_called_once_with(str(ROOT.resolve() / ARTIFACT_SUFFIX), mode=os.RTLD_NOW | os.RTLD_LOCAL)
                self.assertEqual(stdout.getvalue(), ""); self.assertEqual(stderr.getvalue(), "")

    def test_missing_versioned_symbol_refuses_after_real_trusted_load(self):
        target = load_public()
        real_cdll = ctypes.CDLL
        with mock.patch.object(real_cdll, "__getitem__", side_effect=AttributeError("PRIVATE_NATIVE_SYMBOL")) as symbol, \
                mock.patch.object(ctypes, "CDLL", wraps=real_cdll) as load:
            actual = target._load_native_transport()
        self.assertIsNone(actual)
        load.assert_called_once_with(str(ROOT.resolve() / ARTIFACT_SUFFIX), mode=os.RTLD_NOW | os.RTLD_LOCAL)
        symbol.assert_called_once_with(SYMBOL)

    def test_public_loader_none_refuses_environment_before_worker_entry(self):
        target = load_public()
        target._load_native_transport()  # Preserve the real binding-stub marker FIRST.
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(target, "_load_native_transport", return_value=None) as load, \
                mock.patch.object(target, "_worker_result", side_effect=AssertionError("old worker path entered")) as old_worker, \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            actual = target.validate_package("synthetic-missing-manifest.json")
        self.assertIs(actual, target.SchemaValidationOutcome.environment_unavailable)
        load.assert_called_once_with()
        old_worker.assert_not_called()
        self.assertEqual(stdout.getvalue(), ""); self.assertEqual(stderr.getvalue(), "")

    def test_quiescent_integer_mapping_is_literal_and_does_not_validate(self):
        target = load_public()
        rows = ((0, "ok"), (1, "validation_unavailable"), (2, "invalid_path"),
                (3, "io_unavailable"), (4, "environment_unavailable"), (5, "resource_limit"),
                (6, "invalid_json"), (7, "invalid_manifest"), (8, "unsupported_dialect"),
                (9, "unsupported_format"), (10, "dependency_unavailable"), (11, "invalid_schema"),
                (12, "unresolved_reference"), (13, "reference_cycle"), (14, "invalid_inventory"),
                (15, "unsupported_feature"), (16, "invalid_instance"), (17, "validation_unavailable"))
        with no_backend_activity(target):
            for raw, expected in rows:
                with self.subTest(raw=raw):
                    self.assertIs(target._decode_native_result(raw), target.SchemaValidationOutcome(expected))

    def test_quiescent_interrupted_and_unknown_integers_refuse_without_fake_signal(self):
        target = load_public()
        with no_backend_activity(target):
            for raw in (-2147483648, -1, 18, 19, 30, 32, 2147483647):
                with self.subTest(raw=raw):
                    self.assertIs(target._decode_native_result(raw), target.SchemaValidationOutcome.validation_unavailable)
        # No fabricated31: that distinct marker requires an ACTUAL C return31.


if __name__ == "__main__":
    unittest.main(verbosity=2)
