#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""One revision3 normalization-close regression; source review precedes execution.

Import defines only stdlib helpers/test. The real owner, fixture, SDK and generated
code must be admitted before running. This is not another native31 baseline.
"""

import argparse
import errno
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


def fault_context():
    path = Path(__file__).with_name("native_owner_acceptance.py")
    spec = importlib.util.spec_from_file_location("schema_normalization_fault_context", path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)  # Definitions only; its closed57 main is not invoked.
    return module


class NativeNormalizationCloseRegression(unittest.TestCase):
    artifact = None

    def test_after_original_close_interrupts_active_work_before_birth(self):
        helpers = fault_context()
        # NativeR3: case24/AFTER41/EINTR, literal18, NO child, actual close before
        # the negative continuation. No new case/child or accepted integer factory.
        row = ("normalization-close", 24, 41, errno.EINTR, 18, False, (), 0, True)
        with tempfile.TemporaryDirectory(prefix="schema-normalization-close-") as directory:
            root = Path(directory)
            files = {
                "s.json": {"$schema": "https://json-schema.org/draft/2020-12/schema", "type": "integer"},
                "v.json": 1,
                "manifest.json": {
                    "profile": "subathon.schema-package.v1",
                    "documents": [{"path": "s.json", "kind": "jsonschema202012"}],
                    "examples": [{"path": "v.json", "schema_document": "s.json", "schema_pointer": ""}],
                },
            }
            for name, value in files.items():
                (root / name).write_text(json.dumps(value, separators=(",", ":")), encoding="utf-8")
            session = helpers.FaultSession(self, self.artifact, row)
            buffers, arguments = session.arguments(root / "manifest.json")
            with session:  # Existing real fallback is registered BEFORE arm.
                actual = session.entry(*arguments)
                if type(actual) is not int or actual == 31:
                    raise helpers.FixtureFailure("Regression requires the reviewed owner, not an inert marker")
                records = session._snapshot()
                # Collect real kernel facts BEFORE any expected-value/phase
                # assertion can fail and before fallback can repair a leak.
                descriptors = {entry[3] for entry in records if entry[0] >> 32 == 0
                               and entry[1] == 3 and entry[3] >= 0}
                states = {}
                for descriptor in sorted(descriptors):
                    try:
                        session.fstat(descriptor)
                    except OSError as error:
                        states[descriptor] = error.errno
                    else:
                        states[descriptor] = 0
                own_gone = session._no_children(0x20000000)
                group_gone = session._no_children(0)
                parent = [entry for entry in records if entry[0] >> 32 == 0]
                markers = [entry for entry in parent if entry[1:3] == (2, 41)]
                self.assertEqual(len(markers), 1, "The new AFTER-close boundary was not reached once")
                marker = markers[0]
                self.assertEqual(marker[5], 0, "AFTER must preserve the real successful close return")
                releases = [entry for entry in parent if entry[1:3] == (4, 40)
                            and entry[3] >= 0 and entry[5] == 0 and entry[0] < marker[0]]
                self.assertEqual(len(releases), 1, "Real close/release ACK must precede the first AFTER41")
                source = releases[0][3]
                adoptions = [entry for entry in parent if entry[1] == 3 and entry[3] == source
                             and entry[0] < releases[0][0]]
                self.assertTrue(adoptions, "Released original has no genuine acquisition history")
                self.assertEqual(states[source], errno.EBADF)
                self.assertTrue(own_gone, "Own-thread live/zombie child before fallback")
                self.assertTrue(group_gone, "Thread-group live/zombie child before fallback")
                for descriptor in sorted(descriptors):
                    self.assertEqual(states[descriptor], errno.EBADF, "Owner FD live before fallback")
                # The current preserved work=false bug can run the genuine worker
                # and return0. Its real close/cleanup above must precede this red.
                self.assertEqual(actual, 18, "Active normalization EINTR must interrupt before birth")
                session.assert_evidence()  # Full literal18/no-birth/final39/no-log oracles.
            self.assertEqual(len(buffers), 4)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact", required=True)
    options = parser.parse_args(argv)
    NativeNormalizationCloseRegression.artifact = options.artifact
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(NativeNormalizationCloseRegression)
    if suite.countTestCases() != 1:
        raise RuntimeError("Exactly one normalization-close regression is required")
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.testsRun == 1 and result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
