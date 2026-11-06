# SPDX-License-Identifier: GPL-3.0-or-later
"""Native birth controls and distinct real-production post-return complements.

Review before execution. Python Popen constructor interception cannot exercise
the native owner; actual25/26 and after-quiescence decode faults preserve scope.
"""
import importlib.util
from pathlib import Path
import sys
import unittest


SCRIPT = Path(__file__).parents[2] / "tools" / "schema_validate.py"


def native_public_helper():
    spec = importlib.util.spec_from_file_location(
        "schema_spawn_native_public_helper", Path(__file__).with_name("native_public_fault.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ParentSpawnRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("schema_validate_spawn_under_test", SCRIPT)
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        cls.target = module

    def exercise_spawn_failure(self, allocation):
        helper = native_public_helper()
        if allocation:
            # Genuine committed birth/resource26 cleanup, then a distinct
            # prebuilt MemoryError after REAL production0/SDK validation.
            helper.run_child(self, "r26")
            helper.run_child(self, "postreturn-memory")
        else:
            # Real before-close native SIGINT; exact Python exception identity
            # is checked separately after a genuine quiescent production call.
            helper.run_child(self, "i25a")
            helper.run_child(self, "spawn-identity")

    def test_sigint_during_native_constructor_reaps_before_propagating(self):
        self.exercise_spawn_failure(allocation=False)

    def test_post_native_spawn_allocation_refusal_reaps_before_resource_limit(self):
        self.exercise_spawn_failure(allocation=True)


if __name__ == "__main__":
    unittest.main(verbosity=2)
