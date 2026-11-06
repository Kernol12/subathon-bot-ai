# SPDX-License-Identifier: GPL-3.0-or-later
"""Raw OAS schema semantics and context-specific extension-profile regressions.

Literal oracles come from the accepted context contract, not worker algorithms.
The actual fresh worker, pinned libraries, protocol and OS handles are used.
"""
import contextlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


class WorkerContextRegression(unittest.TestCase):
    def worker_result(self, document, expected, *, kind="openapi31"):
        import sysconfig
        self.assertNotEqual(sys.prefix, sys.base_prefix, "requires admitted dedicated SDK venv")
        sdk = sysconfig.get_path("purelib")
        worker = Path(__file__).resolve().parents[2] / "tools/schema_validation/worker.py"
        with tempfile.TemporaryDirectory(prefix="schema-context-regression-") as directory:
            root = Path(directory)
            (root / "api.json").write_bytes(json.dumps(document, ensure_ascii=False).encode("utf-8"))
            manifest = {"profile": "subathon.schema-package.v1", "documents": [
                {"path": "api.json", "kind": kind}], "examples": []}
            (root / "manifest.json").write_bytes(json.dumps(manifest).encode("ascii"))
            with contextlib.ExitStack() as descriptors:
                owned = set()
                def close(fd):
                    if fd in owned:
                        owned.remove(fd)
                        os.close(fd)
                def pipe():
                    pair = os.pipe()
                    for fd in pair:
                        owned.add(fd)
                        descriptors.callback(close, fd)
                    os.set_blocking(pair[0], False)
                    return pair
                activation_read, activation_write = pipe()
                outcome_read, outcome_write = pipe()
                command = [sys.executable, "-I", "-S", str(worker), "--manifest",
                           str(root / "manifest.json"), "--sdk-dir", sdk,
                           "--activation-fd", str(activation_write), "--outcome-fd", str(outcome_write)]
                child = subprocess.Popen(command, shell=False, close_fds=True,
                    pass_fds=(activation_write, outcome_write),
                    env={"PATH": "/usr/bin:/bin", "LC_ALL": "C.UTF-8", "LANG": "C.UTF-8"},
                    stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                try:
                    close(activation_write)
                    close(outcome_write)
                    status = child.wait(timeout=10)
                    # Reads are finite/nonblocking even if an incorrect child
                    # leaves a descendant holding a pipe. No receipt prefix passes.
                    self.assertEqual(os.read(activation_read, 2), b"A")
                    self.assertEqual(os.read(activation_read, 1), b"")
                    line = os.read(outcome_read, 257)
                    self.assertEqual(os.read(outcome_read, 1), b"")
                    self.assertEqual(line, ('{"code":"' + expected + '"}\n').encode("ascii"))
                    self.assertEqual(status, 0 if expected == "ok" else 2)
                finally:
                    if child.poll() is None:
                        try:
                            child.kill()
                        finally:
                            child.wait(timeout=10)

    @staticmethod
    def oas(components):
        return {"openapi": "3.1.1", "info": {"title": "Synthetic", "version": "1"},
                "paths": {}, "components": components}

    def test_native_oas_semantics_keep_raw_schema_ref_siblings(self):
        # Native SchemaValidator.__call__ checks required minus local/allOf
        # properties on this RAW schema. Dereferencing A to B erases that check.
        self.worker_result(self.oas({"schemas": {"B": {}, "A": {
            "$ref": "#/components/schemas/B", "allOf": [{}], "required": []}}}), "ok")
        self.worker_result(self.oas({"schemas": {"B": {}, "A": {
            "$ref": "#/components/schemas/B", "allOf": [{}], "required": ["missing"]}}}), "invalid_schema")

    def test_oas_structural_extension_fields_refuse_before_native_checks(self):
        documents = (self.oas({"schemas": {"S": {"type": "integer", "x-extra": {}}}}),
                     self.oas({"parameters": {
            "P": {"name": "q", "in": "query", "schema": {"type": "integer"}},
            "Alias": {"$ref": "#/components/parameters/P", "x-extra": {}}}}))
        for case, document in enumerate(documents):
            with self.subTest(case=case):
                self.worker_result(document, "unsupported_feature")

    def test_dynamic_names_data_and_standalone_keywords_remain_native(self):
        self.worker_result(self.oas({"schemas": {"S": {"type": "object",
            "properties": {"x-extra": {"type": "integer"}}, "default": {"x-extra": 7}}}}), "ok")
        self.worker_result(self.oas({"schemas": {"S": {"type": "object", "default": {
            "x-extra": {"$ref": "https://outside.invalid/literal-data"}}}}}), "ok")
        self.worker_result({"$schema": "https://json-schema.org/draft/2020-12/schema",
            "x-extra": {"$ref": "https://outside.invalid/literal-data"}}, "ok", kind="jsonschema202012")


if __name__ == "__main__":
    unittest.main(verbosity=2)
