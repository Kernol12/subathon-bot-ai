#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Builtin ABI/loader admission foundations, with no validator invocation.

Run only after separate source/build review:
  python3 -I -S -B native_loader_foundation.py --artifact ABS_TARGET_PATH
The null-manifest control must refuse before any native resource ownership.
"""

import ctypes
import os
from pathlib import Path
import stat
import sys
import unittest


ARTIFACT = None


class NativeLoaderFoundation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # The CMake-owned argument identifies exactly the admitted build location.
        # Definition-only import neither reads it nor loads any native artifact.
        expected = (
            Path(__file__).resolve().parents[2]
            / "build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so"
        )
        if type(ARTIFACT) is not str or not os.path.isabs(ARTIFACT):
            raise AssertionError("foundation needs an absolute trusted artifact")
        actual = Path(ARTIFACT)
        if actual != expected or not stat.S_ISREG(actual.lstat().st_mode):
            raise AssertionError("foundation artifact does not match trusted build")
        cls.artifact = str(actual)

    def assert_builtin_abi(self):
        self.assertEqual(sys.platform, "linux")
        self.assertEqual(os.uname().machine, "x86_64")
        self.assertGreaterEqual(sys.version_info[:2], (3, 10))
        self.assertEqual(sys.byteorder, "little")
        self.assertEqual(ctypes.sizeof(ctypes.c_ubyte), 1)
        self.assertEqual(ctypes.sizeof(ctypes.c_int), 4)
        self.assertEqual(ctypes.sizeof(ctypes.c_uint), 4)
        self.assertEqual(ctypes.sizeof(ctypes.c_void_p), 8)

    def test_builtin_abi_profile(self):
        self.assert_builtin_abi()
        print("native loader host:", sys.version.split()[0], os.uname(), file=sys.stderr)

    def test_actual_artifact_rejects_null_manifest_before_ownership(self):
        self.assert_builtin_abi()  # No reliance on another method running first.
        library = ctypes.CDLL(self.artifact, mode=os.RTLD_NOW | os.RTLD_LOCAL)
        entry = library.subathon_schema_transport_v1
        pointer = ctypes.POINTER(ctypes.c_ubyte)
        entry.argtypes = [pointer, ctypes.c_uint] * 4
        entry.restype = ctypes.c_int
        # Four genuine readable builtin arrays; lengths exclude their final NUL.
        # No custom conversion, callback, output object, errcheck or SDK import.
        payloads = (
            b"manifest.json", b"/usr/bin/python3",
            b"/synthetic/worker.py", b"/synthetic/sdk",
        )
        buffers = tuple(
            (ctypes.c_ubyte * (len(value) + 1)).from_buffer_copy(value + b"\0")
            for value in payloads
        )
        arguments = []
        for buffer, value in zip(buffers, payloads):
            arguments.extend((buffer, len(value)))
        arguments[0], arguments[1] = None, 0
        result = entry(*arguments)
        self.assertIs(type(result), int)
        self.assertEqual(result, 2)  # Actual invalid_path ABI refusal, never product ok.


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] != "--artifact":
        raise SystemExit("usage: native_loader_foundation.py --artifact ABS_TARGET_PATH")
    ARTIFACT = sys.argv[2]
    unittest.main(argv=[sys.argv[0]], verbosity=2)
