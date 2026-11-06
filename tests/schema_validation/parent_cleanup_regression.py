# SPDX-License-Identifier: GPL-3.0-or-later
"""Native cancellation plus post-return identity and pre-entry allocation ports.

Review precedes execution. Native faults are nonvalidating; the separate decode
complement requires real production0 before raising its prebuilt exception.
"""
import contextlib
import ctypes
import builtins
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import sys
import unittest
from unittest import mock


SCRIPT = Path(__file__).parents[2] / "tools" / "schema_validate.py"


def native_public_helper():
    spec = importlib.util.spec_from_file_location(
        "schema_cleanup_native_public_helper", Path(__file__).with_name("native_public_fault.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ParentCleanupRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("schema_validate_cleanup_under_test", SCRIPT)
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        cls.target = module

    def test_cancelled_descriptor_close_reaps_and_closes_before_propagating(self):
        helper = native_public_helper()
        # Real after-close native SIGINT and raw pre-fallback ownership proof.
        helper.run_child(self, "i25b")
        # A prebuilt Python exception is observable only AFTER actual native
        # quiescence; this complement preserves the original exact identity.
        helper.run_child(self, "cancel-identity")

    def test_initial_path_allocation_refusal_is_resource_limit_without_setup(self):
        refusal = MemoryError("INITIAL_ADMISSION_ALLOCATION_REFUSAL")
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(self.target, "_bounded_path", side_effect=refusal) as admission, \
                mock.patch.object(self.target, "_installation", side_effect=AssertionError("setup reached")) as installation, \
                mock.patch.object(subprocess, "Popen", side_effect=AssertionError("allocation refusal spawned")) as spawn, \
                mock.patch.object(builtins, "open", side_effect=AssertionError("allocation refusal read")) as builtin_open, \
                mock.patch.object(io, "open", side_effect=AssertionError("allocation refusal read")) as file_open, \
                mock.patch.object(os, "open", side_effect=AssertionError("allocation refusal read")) as descriptor_open, \
                mock.patch.object(self.target, "_load_native_transport", create=True,
                                  side_effect=AssertionError("allocation refusal loaded native")) as native_load, \
                mock.patch.object(ctypes, "CDLL", side_effect=AssertionError("allocation refusal loaded library")) as library_load, \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            actual = self.target.validate_package("manifest.json")
        self.assertIs(actual, self.target.SchemaValidationOutcome.resource_limit)
        admission.assert_called_once_with("manifest.json")
        installation.assert_not_called()
        spawn.assert_not_called()
        builtin_open.assert_not_called()
        file_open.assert_not_called()
        descriptor_open.assert_not_called()
        native_load.assert_not_called()
        library_load.assert_not_called()
        self.assertEqual(stdout.getvalue(), "")
        self.assertEqual(stderr.getvalue(), "")


if __name__ == "__main__":
    unittest.main(verbosity=2)
