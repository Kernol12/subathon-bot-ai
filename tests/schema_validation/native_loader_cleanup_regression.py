#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real header-handle cleanup regression; source review precedes execution.

Import defines only helpers/tests. No native artifact, target module or SDK is
loaded here. The trace hook raises only a prebuilt negative Python exception.
"""

import builtins
import contextlib
import ctypes
import errno
import importlib.util
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).parents[2] / "tools/schema_validate.py"
# Literal nonexecutable fixture header; the CDLL tripwire forbids loading it.
ELF64 = (b"\x7fELF\x02\x01\x01\x00" + b"\0" * 8
         + b"\x03\x00\x3e\x00\x01\x00\x00\x00" + b"\0" * 28
         + b"\x40\x00" + b"\0" * 10)


def load_public():
    spec = importlib.util.spec_from_file_location("schema_loader_cleanup_target", SCRIPT)
    target = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = target
    spec.loader.exec_module(target)
    return target


class NativeLoaderCleanupRegression(unittest.TestCase):
    def test_prebody_fault_closes_real_header_before_original_exception_escapes(self):
        rows = (("interrupt", KeyboardInterrupt("PRIVATE_HEADER_INTERRUPT")),
                ("allocation", MemoryError("PRIVATE_HEADER_ALLOCATION")))
        for label, original in rows:
            with self.subTest(case=label), tempfile.TemporaryDirectory(prefix="schema-loader-cleanup-") as directory:
                target = load_public()
                script = Path(directory) / "tools/schema_validate.py"
                artifact = Path(directory) / "build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so"
                script.parent.mkdir(parents=True)
                script.write_bytes(b"# trusted-location test metadata only\n")
                artifact.parent.mkdir(parents=True)
                artifact.write_bytes(ELF64)
                stdout, stderr = io.StringIO(), io.StringIO()
                opened, triggered = [], []
                real_open = builtins.open
                loader_code = target._load_native_transport.__code__
                previous_trace = sys.gettrace()
                caught, kept_traceback = None, None

                def actual_open(*arguments, **keywords):
                    source = real_open(*arguments, **keywords)
                    # This is the genuine FileIO, not a proxy or fake descriptor.
                    try:
                        opened.append((source, source.fileno()))
                    except BaseException:
                        source.close()
                        raise
                    return source

                def trace(frame, event, argument):
                    if frame.f_code is not loader_code:
                        return None
                    if (event == "line" and opened and not triggered
                            and frame.f_locals.get("source") is opened[0][0]):
                        triggered.append(True)
                        raise original
                    return trace

                try:
                    with mock.patch.object(target, "__file__", str(script)), \
                            mock.patch.object(builtins, "open", side_effect=actual_open), \
                            mock.patch.object(ctypes, "CDLL", side_effect=AssertionError("synthetic native load")) as load, \
                            contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                        try:
                            sys.settrace(trace)
                            try:
                                target._load_native_transport()
                            except BaseException as error:
                                caught, kept_traceback = error, error.__traceback__
                        finally:
                            sys.settrace(previous_trace)

                        # FIRST ownership observation. No new FD is opened before
                        # fstat, and the actual file AND original traceback live.
                        states = []
                        for source, descriptor in opened:
                            try:
                                os.fstat(descriptor)
                            except OSError as error:
                                states.append(error.errno)
                            else:
                                states.append(0)
                        self.assertEqual(len(opened), 1)
                        self.assertIs(type(opened[0][0]), io.FileIO)
                        self.assertEqual(triggered, [True])
                        self.assertIs(caught, original)
                        self.assertIsNotNone(kept_traceback)
                        self.assertIs(caught.__traceback__, kept_traceback)
                        self.assertIs(sys.gettrace(), previous_trace)
                        load.assert_not_called()
                        self.assertEqual(stdout.getvalue(), "")
                        self.assertEqual(stderr.getvalue(), "")
                        self.assertEqual(states, [errno.EBADF], "Header FD live before fixture cleanup")
                finally:
                    sys.settrace(previous_trace)
                    # Disposal ONLY AFTER every raw fact/oracle; never rescues
                    # an open-FD failure or clears its original traceback early.
                    for source, _ in opened:
                        source.close()
                    original.__traceback__ = None


if __name__ == "__main__":
    unittest.main(verbosity=2)
