# SPDX-License-Identifier: GPL-3.0-or-later
"""Specification-derived wrapper acceptance; definitions are inert on import.

Run only after independent source/oracle review. Every behavior case calls an
unmodified public entrypoint. Fault children exercise refusal/cleanup only.
"""

import builtins
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import select
import signal
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock


DRAFT = "https://json-schema.org/draft/2020-12/schema"
BASE = "https://schemas.subathon.invalid/package/"
ROOT = Path(__file__).parents[2]
SCRIPT = ROOT / "tools" / "schema_validate.py"


@contextlib.contextmanager
def changed_directory(path):
    original = os.getcwd()
    try:
        os.chdir(path)
        yield
    finally:
        os.chdir(original)


def encoded(value):
    return json.dumps(value, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


def schema(**keywords):
    return {"$schema": DRAFT, **keywords}


def openapi(**members):
    return {"openapi": "3.1.1", "info": {"title": "Synthetic", "version": "1"},
            "paths": {}, **members}


class Package:
    """Write only literal synthetic package inputs; no validation or expected oracle."""

    def __init__(self, root):
        self.root = Path(root)
        self.manifest = self.root / "manifest.json"
        self.documents = []
        self.examples = []

    def write(self, name, value):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(value if type(value) is bytes else encoded(value))
        return path

    def document(self, name, value, kind="jsonschema202012"):
        self.write(name, value)
        self.documents.append({"path": name, "kind": kind})
        return self

    def example(self, name, value, document="s.json", pointer=""):
        self.write(name, value)
        self.examples.append({"path": name, "schema_document": document,
                              "schema_pointer": pointer})
        return self

    def inline(self, pointer, target, document="api.json"):
        self.examples.append({"path": {"document": document, "pointer": pointer},
                              "schema_document": document, "schema_pointer": target})
        return self

    def finish(self, **changes):
        self.write("manifest.json", {"profile": "subathon.schema-package.v1",
                                     "documents": self.documents,
                                     "examples": self.examples, **changes})
        return self


class Poison:
    """Any conversion/iteration/repr callback is itself a test failure."""

    def __fspath__(self):
        raise AssertionError("path conversion called")

    def __str__(self):
        raise AssertionError("string conversion called")

    def __repr__(self):
        raise AssertionError("repr called")

    def __iter__(self):
        raise AssertionError("iteration called")


class StringSubclass(str):
    def __str__(self):
        raise AssertionError("subclass conversion called")


class WrapperAcceptance(unittest.TestCase):
    """Logical §§2–5, API §§1–5, context §§1–5, private process v1."""

    @classmethod
    def setUpClass(cls):
        # Explicit trusted local loading also works with Python -I -B.
        spec = importlib.util.spec_from_file_location("schema_validate_under_test", SCRIPT)
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        cls.target = module

    def package(self, value=None, specification=None):
        temporary = tempfile.TemporaryDirectory(prefix="schema-wrapper-")
        self.addCleanup(temporary.cleanup)
        package = Package(temporary.name)
        package.document("s.json", schema() if specification is None else specification)
        if value is not None:
            package.example("v.json", value)
        return package.finish()

    def outcome(self, package_or_path, expected):
        path = str(package_or_path.manifest) if isinstance(package_or_path, Package) else package_or_path
        stdout, stderr = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            actual = self.target.validate_package(path)
        self.assertIs(actual, self.target.SchemaValidationOutcome(expected))
        self.assertEqual(stdout.getvalue(), "")
        self.assertEqual(stderr.getvalue(), "")

    def cli(self, argv, expected, status):
        stdout, stderr = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            actual = self.target.main(argv)
        self.assertIs(type(actual), int)
        self.assertEqual(actual, status)
        self.assertEqual(stdout.getvalue(), '{"code":"' + expected + '"}\n')
        self.assertEqual(stderr.getvalue(), "")

    def test_minimal_draft_package_and_openapi_patches(self):
        self.outcome(self.package(b"1", schema(type="integer")), "ok")
        for patch in ("3.1.0", "3.1.1", "3.1.2"):
            with self.subTest(patch=patch):
                p = self.package()
                p.documents.clear()
                p.document("api.json", openapi(openapi=patch), "openapi31").finish()
                self.outcome(p, "ok")

    def test_api_path_types_never_convert(self):
        for index, value in enumerate((None, 3, b"manifest.json", Path("manifest.json"),
                                       Poison(), StringSubclass("manifest.json"))):
            with self.subTest(case=index):
                self.outcome(value, "invalid_path")

    def test_api_path_encoding_and_shape(self):
        for index, value in enumerate(("", "x\x00y", "\ud800", "x" * 4097,
                                       "https://outside.invalid/manifest.json", "file:///tmp/m.json")):
            with self.subTest(case=index):
                self.outcome(value, "invalid_path")

    def test_main_closed_argv_never_converts(self):
        rows = ([], (), ["--unknown"], ["--manifest=x"], ["--manifest"],
                ["--help", "extra"], ["--manifest", "a", "--manifest", "b"],
                ["--manifest", "a", "extra"], "--help", Poison(),
                ["--manifest", Poison()], [StringSubclass("--help")], [b"--help"])
        for index, row in enumerate(rows):
            with self.subTest(case=index):
                self.cli(row, "usage_error", 64)

    def test_help_static_without_worker_or_package_io(self):
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(subprocess, "Popen", side_effect=AssertionError("help spawned")), \
                mock.patch.object(builtins, "open", side_effect=AssertionError("help opened file")), \
                mock.patch.object(io, "open", side_effect=AssertionError("help opened file")), \
                mock.patch.object(os, "open", side_effect=AssertionError("help opened file")), \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            actual = self.target.main(["--help"])
        self.assertIs(type(actual), int)
        self.assertEqual(actual, 0)
        self.assertEqual(stdout.getvalue(), "Usage: schema_validate.py --manifest PATH\n")
        self.assertEqual(stderr.getvalue(), "")

    def test_main_tuple_none_and_option_looking_path(self):
        p = self.package(b"1", schema(type="integer"))
        self.cli(("--manifest", str(p.manifest)), "ok", 0)
        with mock.patch.object(sys, "argv", ["tool", "--manifest", str(p.manifest)]):
            self.cli(None, "ok", 0)
        with changed_directory(p.root):
            self.cli(["--manifest", "--help"], "io_unavailable", 3)

    def test_main_closed_codes_and_real_script_redaction(self):
        p = self.package(b'"SECRET-NOT-IN-DIAGNOSTIC"', schema(type="integer"))
        self.cli(["--manifest", str(p.manifest)], "invalid_instance", 2)
        # The direct raw call above is FIRST; script execution is extra parity.
        with tempfile.TemporaryFile() as stdout, tempfile.TemporaryFile() as stderr:
            child = subprocess.Popen([sys.executable, "-I", "-B", str(SCRIPT),
                                      "--manifest", str(p.manifest)],
                                     stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr)
            try:
                self.assertEqual(child.wait(timeout=15), 2)
            finally:
                if child.poll() is None:
                    child.kill()
                    child.wait(timeout=3)
            stdout.seek(0)
            stderr.seek(0)
            self.assertEqual(stdout.read(257), b'{"code":"invalid_instance"}\n')
            self.assertEqual(stderr.read(257), b"")

    def test_fresh_calls_do_not_cache_prior_success(self):
        p = self.package(b"1", schema(type="integer"))
        self.outcome(p, "ok")
        p.write("v.json", b'"changed"')
        self.outcome(p, "invalid_instance")
        p.write("v.json", b"2")
        self.outcome(p, "ok")

    def test_package_root_is_manifest_parent_not_cwd(self):
        p = self.package(b"1", schema(type="integer"))
        with tempfile.TemporaryDirectory() as elsewhere, changed_directory(elsewhere):
            Path("s.json").write_bytes(b"invalid cwd decoy")
            self.outcome(p, "ok")
        with changed_directory(p.root):
            self.outcome("manifest.json", "ok")

    def test_missing_manifest_and_declared_member(self):
        p = self.package(b"1")
        self.outcome(str(p.root / "absent.json"), "io_unavailable")
        (p.root / "v.json").unlink()
        self.outcome(p, "io_unavailable")

    def test_symlink_manifest_parent_and_members(self):
        p = self.package(b"1")
        (p.root / "link.json").symlink_to("manifest.json")
        self.outcome(str(p.root / "link.json"), "invalid_path")
        with tempfile.TemporaryDirectory() as elsewhere:
            alias = Path(elsewhere) / "alias"
            alias.symlink_to(p.root, target_is_directory=True)
            self.outcome(str(alias / "manifest.json"), "invalid_path")
        (p.root / "v.json").unlink()
        (p.root / "v.json").symlink_to("s.json")
        self.outcome(p, "invalid_path")

    def test_nonregular_files_are_refused_without_blocking(self):
        for kind in ("directory", "fifo"):
            with self.subTest(kind=kind):
                p = self.package(b"1")
                path = p.root / "v.json"
                path.unlink()
                if kind == "directory":
                    path.mkdir()
                else:
                    os.mkfifo(path)
                self.outcome(p, "invalid_path")

    def test_manifest_closed_shape_and_types(self):
        base = {"profile": "subathon.schema-package.v1", "documents": [{"path": "s.json", "kind": "jsonschema202012"}], "examples": []}
        rows = [[], None, {**base, "extra": 1}, {**base, "profile": "other"},
                {**base, "documents": []}, {**base, "documents": {}},
                {**base, "examples": None}, {**base, "expected_valid": False},
                {**base, "documents": [{"path": "s.json", "kind": "jsonschema"}]},
                {**base, "documents": [{"path": "s.json", "kind": "jsonschema202012", "extra": 1}]},
                {**base, "examples": [{"path": "v.json", "schema_document": "s.json"}]},
                {**base, "examples": [{"path": "v.json", "schema_document": "unknown.json", "schema_pointer": ""}]}]
        for index, raw in enumerate(rows):
            with self.subTest(case=index):
                p = self.package()
                p.write("manifest.json", raw)
                self.outcome(p, "invalid_manifest")

    def test_manifest_path_grammar_and_exact_256_boundary(self):
        for index, path in enumerate(("", "../x", "./x", "/x", "a//b", "a/../b", "a\\b", "a%2fb", "a:b", "é", "a\x00b", "d/" + "a" * 255)):
            with self.subTest(case=index):
                p = self.package()
                p.documents[0]["path"] = path
                p.finish()
                self.outcome(p, "invalid_path")
        p = self.package()
        p.documents.clear()
        p.document("sub/" + "a" * 252, schema()).finish()
        self.outcome(p, "ok")

    def test_manifest_pointer_and_inline_source_shape(self):
        rows = [("not/slash", "invalid_manifest"), ("/~2", "invalid_manifest"),
                ("/~", "invalid_manifest")]
        for pointer, expected in rows:
            with self.subTest(pointer=pointer):
                p = self.package(b"1")
                p.examples[0]["schema_pointer"] = pointer
                p.finish()
                self.outcome(p, expected)
        p = self.package(b"1")
        p.examples[0]["path"] = {"document": "s.json", "pointer": "", "extra": 1}
        p.finish()
        self.outcome(p, "invalid_manifest")

    def test_duplicate_declarations_bindings_and_file_aliases(self):
        for kind in ("document", "binding", "manifest", "document-example", "hardlink"):
            with self.subTest(kind=kind):
                p = self.package(b"1")
                if kind == "document":
                    p.documents.append(dict(p.documents[0]))
                elif kind == "binding":
                    p.examples.append(dict(p.examples[0]))
                elif kind == "manifest":
                    p.documents[0]["path"] = "manifest.json"
                elif kind == "document-example":
                    p.examples[0]["path"] = "s.json"
                else:
                    (p.root / "v.json").unlink()
                    os.link(p.root / "s.json", p.root / "v.json")
                p.finish()
                self.outcome(p, "invalid_manifest")

    def test_document_and_binding_count_limits_precede_missing_reads(self):
        for count, expected in ((32, "ok"), (33, "resource_limit")):
            with self.subTest(documents=count):
                p = self.package()
                p.documents.clear()
                for index in range(count):
                    p.document("s" + str(index) + ".json", schema())
                if count == 33:
                    (p.root / "s0.json").unlink()
                p.finish()
                self.outcome(p, expected)
        for count, expected in ((256, "ok"), (257, "resource_limit")):
            with self.subTest(bindings=count):
                p = self.package()
                for index in range(count):
                    p.example("e" + str(index) + ".json", b"null")
                if count == 257:
                    (p.root / "e0.json").unlink()
                p.finish()
                self.outcome(p, expected)

    def test_all_289_unique_files_are_admitted(self):
        p = self.package()
        p.documents.clear()
        for index in range(32):
            p.document("s" + str(index) + ".json", schema())
        for index in range(256):
            p.example("e" + str(index) + ".json", b"null", "s0.json")
        p.finish()
        self.outcome(p, "ok")

    def test_manifest_and_individual_file_byte_boundaries(self):
        for extra, expected in ((0, "ok"), (1, "resource_limit")):
            with self.subTest(manifest_extra=extra):
                p = self.package()
                raw = p.manifest.read_bytes()
                p.write("manifest.json", raw + b" " * (65536 + extra - len(raw)))
                self.outcome(p, expected)
                if extra:
                    self.cli(["--manifest", str(p.manifest)], "resource_limit", 4)
            with self.subTest(file_extra=extra):
                p = self.package(b"null")
                p.write("v.json", b"null" + b" " * (1048576 + extra - 4))
                self.outcome(p, expected)

    def test_total_raw_file_byte_boundary_includes_manifest(self):
        for extra, expected in ((0, "ok"), (1, "resource_limit")):
            with self.subTest(extra=extra):
                p = self.package()
                p.documents.clear()
                for index in range(8):
                    p.document("s" + str(index) + ".json", schema())
                p.finish()
                raw = encoded(schema())
                for index in range(7):
                    p.write("s" + str(index) + ".json", raw + b" " * (1048576 - len(raw)))
                last = 8388608 + extra - len(p.manifest.read_bytes()) - 7 * 1048576
                p.write("s7.json", raw + b" " * (last - len(raw)))
                self.outcome(p, expected)

    def test_strict_json_rejects_encoding_duplicates_and_grammar(self):
        rows = (b'\xef\xbb\xbfnull', b'"\xff"', b'"\\ud800"', b'{"type":1,"t\\u0079pe":2}',
                b"NaN", b"Infinity", b"-Infinity", b"/*c*/null", b"null null",
                b"01", b"+1", b"1.", b"[1,]", b"{'x':1}")
        for index, raw in enumerate(rows):
            with self.subTest(case=index):
                p = self.package(b"null")
                p.write("v.json", raw)
                self.outcome(p, "invalid_json")

    def test_json_depth_values_and_raw_string_boundaries(self):
        for count, expected in ((64, "ok"), (65, "resource_limit")):
            with self.subTest(depth=count):
                self.outcome(self.package(b"[" * count + b"0" + b"]" * count), expected)
        for count, expected in ((65535, "ok"), (65536, "resource_limit")):
            with self.subTest(array_members=count):
                self.outcome(self.package(b"[" + b",".join([b"0"] * count) + b"]"), expected)
            with self.subTest(object_members=count):
                raw = b"{" + b",".join(('"k' + str(i) + '":0').encode("ascii") for i in range(count)) + b"}"
                self.outcome(self.package(raw), expected)
        for extra, expected in ((0, "ok"), (1, "resource_limit")):
            with self.subTest(raw_string_extra=extra):
                self.outcome(self.package(b'"' + b"x" * (65534 + extra) + b'"'), expected)
            with self.subTest(escaped_string_extra=extra):
                self.outcome(self.package(b'"' + b"\\u0061" * 10922 + b"aa" + b"a" * extra + b'"'), expected)
            with self.subTest(member_name_token_extra=extra):
                self.outcome(self.package(b'{"' + b"x" * (65534 + extra) + b'":0}'), expected)

    def test_lexical_numeric_bounds_before_exact_construction(self):
        rows = ((b"1" * 256, "ok"), (b"1" * 257, "resource_limit"),
                (b"1e4096", "ok"), (b"1e4097", "resource_limit"),
                (b"1e-4096", "ok"), (b"1e-4097", "resource_limit"),
                (b"1e" + b"0" * 509 + b"1", "ok"),
                (b"1e" + b"0" * 510 + b"1", "resource_limit"))
        for index, (raw, expected) in enumerate(rows):
            with self.subTest(case=index):
                self.outcome(self.package(raw), expected)

    def test_first_failure_json_preflight_and_complete_file_order(self):
        p = self.package(b"[" * 65 + b"bad grammar")
        self.outcome(p, "resource_limit")
        p.write("v.json", b'\xef\xbb\xbf' + b"[" * 65)
        self.outcome(p, "invalid_json")
        p.write("s.json", schema(format="date-time"))
        p.write("v.json", b"bad json")
        self.outcome(p, "invalid_json")
        p.write("manifest.json", b'{"profile":"wrong","documents":[],"examples":[],"x":NaN}')
        self.outcome(p, "invalid_json")

    def test_exact_numeric_integer_boolean_and_keyword_semantics(self):
        rows = ((schema(type="integer"), b"1.0", "ok"),
                (schema(type="integer"), b"1e0", "ok"),
                (schema(type="integer"), b"true", "invalid_instance"),
                (schema(type="integer"), b"1.5", "invalid_instance"),
                (schema(const=1), b"true", "invalid_instance"),
                (schema(enum=[1]), b"1.0", "ok"),
                (schema(uniqueItems=True), b"[1,1.0]", "invalid_instance"),
                (schema(uniqueItems=True), b"[true,1]", "ok"),
                (schema(minItems=1.0), b"[]", "invalid_instance"),
                (schema(const=0), b"-0.0", "ok"))
        for index, (specification, raw, expected) in enumerate(rows):
            with self.subTest(case=index):
                self.outcome(self.package(raw, specification), expected)

    def test_exact_fraction_multipleof_and_neighbor(self):
        raw_schema = b'{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"number","multipleOf":0.1}'
        for raw, expected in ((b"0.3", "ok"), (b"0.3000000000000000000000000000000000000001", "invalid_instance")):
            with self.subTest(value=raw.decode("ascii")):
                self.outcome(self.package(raw, raw_schema), expected)

    def test_dialect_admission_is_explicit_and_precedes_format(self):
        for dialect in (None, "http://json-schema.org/draft-07/schema#", "https://outside.invalid/dialect"):
            with self.subTest(dialect=dialect):
                specification = {"format": "date-time"}
                if dialect is not None:
                    specification["$schema"] = dialect
                self.outcome(self.package(b"null", specification), "unsupported_dialect")
        for version in ("3.0.3", "3.2.0", "3.1.3"):
            with self.subTest(openapi=version):
                p = self.package()
                p.documents.clear()
                p.document("api.json", openapi(openapi=version), "openapi31").finish()
                self.outcome(p, "unsupported_dialect")
        self.outcome(self.package(b"null", schema(**{"$vocabulary": {"https://outside.invalid/vocab": True}})), "unsupported_dialect")

    def test_schema_format_only_not_literal_or_metadata(self):
        for name in ("date-time", "unknown"):
            with self.subTest(format=name):
                self.outcome(self.package(b'"x"', schema(format=name)), "unsupported_format")
        self.outcome(self.package(b'{"format":"date-time"}', schema(const={"format": "date-time"})), "ok")
        p = self.package()
        p.documents.clear()
        p.document("api.json", openapi(components={"securitySchemes": {"bearer": {"type": "http", "scheme": "bearer", "bearerFormat": "JWT"}}}), "openapi31").finish()
        self.outcome(p, "ok")

    def test_native_invalid_schema_in_unused_children_and_ref_siblings(self):
        rows = (schema(properties={"unused": {"type": 7}}),
                schema(**{"$defs": {"unused": {"minLength": -1}}}),
                schema(**{"$ref": "#/$defs/T", "$defs": {"T": True}, "type": 7}),
                schema(items=7))
        for index, specification in enumerate(rows):
            with self.subTest(case=index):
                self.outcome(self.package(b"null", specification), "invalid_schema")
        p = self.package()
        p.documents.clear()
        p.document("api.json", {"openapi": "3.1.1", "paths": {}}, "openapi31").finish()
        self.outcome(p, "invalid_schema")

    def test_literal_data_ref_id_format_and_reserved_property_names(self):
        literal = {"$ref": "https://outside.invalid/data", "$id": "https://outside.invalid/id", "format": "date-time", "x-thing": {"$dynamicRef": "#"}}
        for keyword in ("const", "enum", "default"):
            with self.subTest(keyword=keyword):
                specification = schema(**{keyword: [literal] if keyword == "enum" else literal})
                self.outcome(self.package(literal, specification), "ok")
        self.outcome(self.package({"$ref": "ordinary", "discriminator": 1}, schema(type="object", properties={"$ref": {"type": "string"}, "discriminator": {"type": "integer"}}, additionalProperties=False)), "ok")

    def test_profile_features_are_context_specific(self):
        for keywords in ({"$dynamicRef": "#"}, {"$dynamicAnchor": "x"}, {"discriminator": {"propertyName": "kind"}}):
            with self.subTest(keyword=next(iter(keywords))):
                self.outcome(self.package(b"null", schema(**keywords)), "unsupported_feature")
        for members in ({"webhooks": {}}, {"x-custom": {}}, {"components": {"examples": {"E": {"externalValue": "https://outside.invalid/x"}}}}, {"paths": {"/x": {"$ref": "#/components/pathItems/P", "summary": "mixed"}}, "components": {"pathItems": {"P": {}}}}):
            with self.subTest(case=list(members)):
                p = self.package()
                p.documents.clear()
                p.document("api.json", openapi(**members), "openapi31").finish()
                self.outcome(p, "unsupported_feature")

    def test_complete_unused_reference_closure_and_wrong_target_role(self):
        rows = (schema(**{"$ref": "https://outside.invalid/x"}),
                schema(**{"$defs": {"unused": {"$ref": "missing.json"}}}),
                schema(**{"$ref": "#/$defs/missing", "$defs": {"T": True}}),
                schema(**{"$ref": "#absent"}),
                schema(**{"$ref": "#/const", "const": {"type": "integer"}}))
        for index, specification in enumerate(rows):
            with self.subTest(case=index):
                self.outcome(self.package(b"1", specification), "unresolved_reference")
        p = self.package()
        p.documents.clear()
        p.document("api.json", openapi(components={"responses": {"R": {"$ref": "#/components/schemas/S"}}, "schemas": {"S": {"type": "object"}}}), "openapi31").finish()
        self.outcome(p, "unresolved_reference")

    def test_duplicate_resource_ids_anchors_and_outside_ids(self):
        rows = (schema(**{"$defs": {"A": {"$id": "same"}, "B": {"$id": "same"}}}),
                schema(**{"$defs": {"A": {"$anchor": "same"}, "B": {"$anchor": "same"}}}),
                schema(**{"$id": "https://outside.invalid/schema"}))
        for index, specification in enumerate(rows):
            with self.subTest(case=index):
                self.outcome(self.package(b"null", specification), "unresolved_reference")

    def test_manifest_pointer_escaping_percent_hash_and_unicode(self):
        names = (("a/b", "/$defs/a~1b"), ("a~b", "/$defs/a~0b"),
                 ("p%2Fq", "/$defs/p%2Fq"), ("#", "/$defs/#"), ("é", "/$defs/é"))
        for name, pointer in names:
            with self.subTest(name=name):
                p = self.package()
                p.write("s.json", schema(**{"$defs": {name: {"const": 7}}}))
                p.example("v.json", b"7", pointer=pointer).finish()
                self.outcome(p, "ok")
                p.write("v.json", b"8")
                self.outcome(p, "invalid_instance")

    def test_uri_reference_fragments_are_not_reencoded_as_raw_pointers(self):
        rows = (("a/b", "#/%24defs/a~1b"), ("a~b", "#/%24defs/a~0b"),
                ("p%2Fq", "#/%24defs/p%252Fq"), ("#", "#/%24defs/%23"),
                ("é", "#/%24defs/%C3%A9"))
        for name, reference in rows:
            with self.subTest(name=name):
                specification = schema(**{"$defs": {name: {"const": 7}}, "$ref": reference})
                self.outcome(self.package(b"7", specification), "ok")
                self.outcome(self.package(b"8", specification), "invalid_instance")

    def test_unicode_pointer_identity_is_not_normalized(self):
        p = self.package()
        p.write("s.json", schema(**{"$defs": {"é": {"const": 1}, "e\u0301": {"const": 2}}}))
        p.example("a.json", b"1", pointer="/$defs/é")
        p.example("b.json", b"2", pointer="/$defs/e\u0301").finish()
        self.outcome(p, "ok")
        p.write("b.json", b"1")
        self.outcome(p, "invalid_instance")

    def test_cross_document_scope_nested_ids_anchors_and_ref_siblings(self):
        p = self.package()
        p.write("s.json", schema(**{"$ref": "other.json#positive", "maximum": 5}))
        p.document("other.json", schema(**{"$defs": {"T": {"$id": "nested/number", "$anchor": "positive", "type": "integer", "minimum": 1}}, "$ref": "nested/number#positive"}))
        # Root anchor is deliberately on a nested resource; use its actual URI.
        p.write("s.json", schema(**{"$ref": "nested/number#positive", "maximum": 5}))
        p.example("v.json", b"3").finish()
        self.outcome(p, "ok")
        p.write("v.json", b"6")
        self.outcome(p, "invalid_instance")

    def test_oas_schema_scope_with_and_without_root_id(self):
        for has_id in (False, True):
            with self.subTest(root_id=has_id):
                sample = {"$defs": {"Inner": {"$id": "inner.json", "$anchor": "integer", "type": "integer"}}, "$ref": "inner.json#integer", "minimum": 2}
                if has_id:
                    sample["$id"] = "nested/sample.json"
                p = self.package()
                p.documents.clear()
                p.document("api.json", openapi(components={"schemas": {"Sample": sample}}), "openapi31")
                p.example("v.json", b"3", "api.json", "/components/schemas/Sample").finish()
                self.outcome(p, "ok")
                p.write("v.json", b"1")
                self.outcome(p, "invalid_instance")
                p.write("v.json", b'"wrong"')
                self.outcome(p, "invalid_instance")

    def test_same_instance_cycles_even_unused_or_conditional(self):
        rows = (schema(**{"$ref": "#"}),
                schema(**{"$defs": {"unused": {"$ref": "#/$defs/unused"}}}),
                schema(**{"if": False, "then": {"$ref": "#"}}),
                schema(**{"dependentSchemas": {"x": {"$ref": "#"}}}))
        for index, specification in enumerate(rows):
            with self.subTest(case=index):
                self.outcome(self.package(b"1", specification), "reference_cycle")
        p = self.package()
        p.documents.clear()
        p.document("api.json", openapi(components={"responses": {"A": {"$ref": "#/components/responses/B"}, "B": {"$ref": "#/components/responses/A"}}}), "openapi31").finish()
        self.outcome(p, "reference_cycle")

    def test_productive_recursive_schema_and_inventory_only_edges(self):
        node = schema(type="object", properties={"value": {"type": "integer"}, "next": {"$ref": "#"}}, required=["value"], additionalProperties=False)
        self.outcome(self.package({"value": 1, "next": {"value": 2}}, node), "ok")
        self.outcome(self.package({"value": 1, "next": {"value": "bad"}}, node), "invalid_instance")
        self.outcome(self.package({"name": 1}, schema(propertyNames={"$ref": "#"})), "ok")
        # $defs/contentSchema seed checking; they do not apply to this instance.
        self.outcome(self.package(b"1", schema(type="integer", **{"$defs": {"T": {"$ref": "#"}}, "contentSchema": {"$ref": "#"}})), "ok")

    def test_native_applicators_booleans_unevaluated_and_property_names(self):
        rows = ((schema(**{"$defs": {"F": False}, "$ref": "#/$defs/F"}), b"null", "invalid_instance"),
                (schema(propertyNames={"pattern": "^[a-z]+$"}), b'{"abc":1}', "ok"),
                (schema(propertyNames={"pattern": "^[a-z]+$"}), b'{"123":1}', "invalid_instance"),
                (schema(allOf=[{"properties": {"a": {"type": "integer"}}}], unevaluatedProperties=False), b'{"a":1,"b":2}', "invalid_instance"),
                (schema(prefixItems=[{"type": "integer"}], items=False), b"[1]", "ok"),
                (schema(prefixItems=[{"type": "integer"}], items=False), b"[1,2]", "invalid_instance"))
        for index, (specification, value, expected) in enumerate(rows):
            with self.subTest(case=index):
                self.outcome(self.package(value, specification), expected)

    def test_oas_default_literal_ref_preserved_and_raw_sibling_checked(self):
        for bad in (False, True):
            with self.subTest(bad_default=bad):
                raw = {"type": "object", "properties": {"$ref": {"type": "string"}}, "required": ["$ref"], "additionalProperties": False,
                       "default": {"$ref": 7 if bad else "https://outside.invalid/data"}}
                p = self.package()
                p.documents.clear()
                p.document("api.json", openapi(components={"schemas": {"T": {"type": "object"}, "S": {"$ref": "#/components/schemas/T", **raw}}}), "openapi31").finish()
                self.outcome(p, "invalid_schema" if bad else "ok")
        # Standalone Draft default is annotation, not an added instance assertion.
        self.outcome(self.package(b"1", schema(type="integer", default="wrong")), "ok")

    def test_schema_example_inventory_missing_then_invalid_instance(self):
        p = self.package()
        p.documents.clear()
        p.document("api.json", openapi(components={"schemas": {"S": {"type": "integer", "examples": [1, "wrong"]}}}), "openapi31")
        p.inline("/components/schemas/S/examples/0", "/components/schemas/S").finish()
        self.outcome(p, "invalid_inventory")
        p.inline("/components/schemas/S/examples/1", "/components/schemas/S").finish()
        self.outcome(p, "invalid_instance")
        p.write("api.json", openapi(components={"schemas": {"S": {"type": "integer", "examples": [1, 2]}}}))
        self.outcome(p, "ok")

    def test_inventory_null_example_and_no_associated_schema(self):
        p = self.package()
        p.documents.clear()
        media = {"schema": {"type": "null"}, "example": None}
        document = openapi(paths={"/x": {"get": {"responses": {"200": {"description": "ok", "content": {"application/json": media}}}}}})
        p.document("api.json", document, "openapi31")
        source = "/paths/~1x/get/responses/200/content/application~1json/example"
        target = "/paths/~1x/get/responses/200/content/application~1json/schema"
        p.inline(source, target).finish()
        self.outcome(p, "ok")
        media["example"] = 1
        p.write("api.json", document)
        self.outcome(p, "invalid_instance")
        del media["schema"]
        p.write("api.json", document)
        self.outcome(p, "unsupported_feature")

    def test_reference_example_physical_identity_complete_forced_schema_pairs(self):
        p = self.package()
        p.documents.clear()
        media_a = {"schema": {"type": "integer"}, "examples": {"E": {"$ref": "#/components/examples/E"}}}
        media_b = {"schema": {"minimum": 0}, "examples": {"E": {"$ref": "#/components/examples/E"}}}
        document = openapi(paths={"/x": {"get": {"responses": {"200": {"description": "ok", "content": {"a/json": media_a, "b/json": media_b}}}}}}, components={"examples": {"E": {"value": 1}}})
        p.document("api.json", document, "openapi31")
        pointer = "/components/examples/E/value"
        target_a = "/paths/~1x/get/responses/200/content/a~1json/schema"
        target_b = "/paths/~1x/get/responses/200/content/b~1json/schema"
        p.inline(pointer, target_a).finish()
        self.outcome(p, "invalid_inventory")
        p.inline(pointer, target_b).finish()
        self.outcome(p, "ok")
        p.inline(pointer, "/components/schemas/Extra")
        document["components"]["schemas"] = {"Extra": True}
        p.write("api.json", document)
        p.finish()
        self.outcome(p, "invalid_inventory")

    def test_free_component_example_requires_explicit_actual_schema_binding(self):
        p = self.package()
        p.documents.clear()
        p.document("api.json", openapi(components={"schemas": {"S": {"type": "integer"}}, "examples": {"unused": {"value": 1}}}), "openapi31").finish()
        self.outcome(p, "invalid_inventory")
        p.inline("/components/examples/unused/value", "/components/schemas/S").finish()
        self.outcome(p, "ok")

    def test_extra_misdirected_and_resolved_alias_inventory(self):
        for kind in ("extra", "misdirected", "alias"):
            with self.subTest(kind=kind):
                p = self.package()
                p.documents.clear()
                doc = openapi(components={"schemas": {"S": {"type": "integer", "example": 1}, "T": True}, "examples": {"E": {"value": 1}, "Alias": {"$ref": "#/components/examples/E"}}})
                p.document("api.json", doc, "openapi31")
                p.inline("/components/schemas/S/example", "/components/schemas/S")
                p.inline("/components/examples/E/value", "/components/schemas/T")
                if kind == "extra":
                    p.inline("/info/title", "/components/schemas/T")
                elif kind == "misdirected":
                    p.examples[0]["schema_pointer"] = "/components/schemas/T"
                else:
                    p.inline("/components/examples/Alias/value", "/components/schemas/T")
                p.finish()
                self.outcome(p, "invalid_inventory")

    def test_external_example_shared_across_actual_schemas(self):
        p = self.package()
        p.write("s.json", schema(**{"$defs": {"A": {"type": "integer"}, "B": {"minimum": 0}}}))
        p.example("v.json", b"1", pointer="/$defs/A")
        p.examples.append({"path": "v.json", "schema_document": "s.json", "schema_pointer": "/$defs/B"})
        p.finish()
        self.outcome(p, "ok")
        p.write("v.json", b"-1")
        self.outcome(p, "invalid_instance")

    def test_parameter_header_encoding_and_schema_annotation_inventory(self):
        p = self.package()
        p.documents.clear()
        doc = openapi(paths={"/x": {"parameters": [{"name": "q", "in": "query", "schema": {"type": "integer"}, "example": 1}], "post": {"requestBody": {"content": {"multipart/form-data": {"schema": {"type": "object", "properties": {"part": {"type": "string"}}}, "encoding": {"part": {"headers": {"X": {"schema": {"type": "integer"}, "example": 2}}}}}}}, "responses": {"200": {"description": "ok"}}}}}, components={"schemas": {"S": {"type": "object", "example": {"$ref": "https://outside.invalid/data"}}}})
        p.document("api.json", doc, "openapi31")
        p.inline("/paths/~1x/parameters/0/example", "/paths/~1x/parameters/0/schema")
        p.inline("/paths/~1x/post/requestBody/content/multipart~1form-data/encoding/part/headers/X/example", "/paths/~1x/post/requestBody/content/multipart~1form-data/encoding/part/headers/X/schema")
        p.inline("/components/schemas/S/example", "/components/schemas/S").finish()
        self.outcome(p, "ok")
        p.examples.pop(1)
        p.finish()
        self.outcome(p, "invalid_inventory")

    def test_standalone_inline_examples_are_complete_literal_data(self):
        literal = {"$ref": "https://outside.invalid/data", "format": "date-time"}
        p = self.package()
        p.write("s.json", schema(type="object", example=literal, examples=[literal], default=literal))
        p.inline("/example", "", "s.json")
        p.inline("/examples/0", "", "s.json").finish()
        self.outcome(p, "ok")
        p.examples.pop()
        p.finish()
        self.outcome(p, "invalid_inventory")

    def test_oas_closed_roles_include_unused_components_trace_and_content_forms(self):
        p = self.package()
        p.documents.clear()
        components = {
            "schemas": {"S": {"type": "integer"}},
            "parameters": {"P": {"name": "q", "in": "query", "content": {"application/json": {"schema": {"$ref": "#/components/schemas/S"}, "example": 1}}}},
            "headers": {"H": {"content": {"application/json": {"schema": {"type": "integer"}, "example": 2}}}},
            "responses": {"R": {"description": "ok", "headers": {"X": {"$ref": "#/components/headers/H"}}}},
            "requestBodies": {"B": {"content": {"application/json": {"schema": {"type": "integer"}, "example": 3}}}},
            "pathItems": {"P": {"trace": {"parameters": [{"$ref": "#/components/parameters/P"}], "requestBody": {"$ref": "#/components/requestBodies/B"}, "responses": {"200": {"$ref": "#/components/responses/R"}}}}},
            "securitySchemes": {"OAuth": {"type": "oauth2", "flows": {"clientCredentials": {"tokenUrl": "https://outside.invalid/token", "scopes": {"$ref": "ordinary scope name"}}}}},
        }
        p.document("api.json", openapi(paths={"/x": {"$ref": "#/components/pathItems/P"}}, components=components), "openapi31")
        for source, target in (
            ("/components/parameters/P/content/application~1json/example", "/components/parameters/P/content/application~1json/schema"),
            ("/components/headers/H/content/application~1json/example", "/components/headers/H/content/application~1json/schema"),
            ("/components/requestBodies/B/content/application~1json/example", "/components/requestBodies/B/content/application~1json/schema"),
        ):
            p.inline(source, target)
        p.finish()
        self.outcome(p, "ok")
        components["responses"]["Unused"] = {"$ref": "#/components/responses/Absent"}
        p.write("api.json", openapi(paths={}, components=components))
        self.outcome(p, "unresolved_reference")

    def test_all_documents_finish_dialect_admission_before_native_checks(self):
        p = self.package(specification=schema(type=7))
        p.document("later.json", {"$schema": "https://outside.invalid/dialect"}).finish()
        self.outcome(p, "unsupported_dialect")

    def test_required_inventory_growth_refuses_instead_of_truncating(self):
        p = self.package()
        p.write("s.json", schema(examples=[None] * 257))
        p.finish()
        self.outcome(p, "resource_limit")

    def test_context_resource_and_reference_edge_boundaries(self):
        for count, expected in ((4095, "ok"), (4096, "resource_limit")):
            with self.subTest(schema_children=count):
                self.outcome(self.package(specification=schema(**{"$defs": {"T" + str(i): True for i in range(count)}})), expected)
        for count, expected in ((255, "ok"), (256, "resource_limit")):
            with self.subTest(resource_children=count):
                self.outcome(self.package(specification=schema(**{"$defs": {"T" + str(i): {"$id": "r" + str(i)} for i in range(count)}})), expected)
        for count, expected in ((2048, "ok"), (2049, "resource_limit")):
            with self.subTest(ref_edges=count):
                definitions = {"T": True, **{"R" + str(i): {"$ref": "#/$defs/T"} for i in range(count)}}
                self.outcome(self.package(specification=schema(**{"$defs": definitions})), expected)

    def test_graph_keys_include_encoded_controls_and_complete_owned_spellings(self):
        # Literal prefix https://schemas.subathon.invalid/package/s.json#/%24defs/
        # is57 ASCII bytes. These names yield exactly4096 and4097-byte controls.
        for length, expected in ((4039, "ok"), (4040, "resource_limit")):
            with self.subTest(control_name_bytes=length):
                p = self.package()
                name = "a" * length
                p.write("s.json", schema(**{"$defs": {name: True}}))
                p.example("v.json", b"null", pointer="/$defs/" + name).finish()
                self.outcome(p, expected)
        p = self.package()
        name = "é" * 1000
        p.write("s.json", schema(**{"$defs": {name: True}}))
        p.example("v.json", b"null", pointer="/$defs/" + name).finish()
        # Raw pointer2007bytes fits4096; its encoded control URI is >6000.
        self.outcome(p, "resource_limit")
        p = self.package()
        p.documents.clear()
        # Each file is <1MiB;252 resources/contexts fit. Retained physical pointers
        # plus resolved resource URIs alone exceed1MiB, without counting aliases.
        for doc in ("a.json", "b.json"):
            definitions = {"k" * 3400 + str(i): {"$id": "r" * 3400 + doc[0] + str(i)} for i in range(125)}
            p.document(doc, schema(**{"$defs": definitions}))
        p.finish()
        self.outcome(p, "resource_limit")

    def test_context_phase_priority_across_independent_defects(self):
        rows = ((schema(**{"$dynamicRef": "#", "type": 7}), "unsupported_feature"),
                (schema(**{"type": 7, "$ref": "missing.json"}), "invalid_schema"),
                (schema(**{"$defs": {"A": {"$ref": "missing.json"}, "B": {"$ref": "#/$defs/B"}}}), "unresolved_reference"),
                (schema(**{"$ref": "#", "example": "wrong"}), "reference_cycle"))
        for index, (specification, expected) in enumerate(rows):
            with self.subTest(case=index):
                self.outcome(self.package(b"null", specification), expected)
        p = self.package()
        p.documents.clear()
        p.document("api.json", openapi(components={"schemas": {"S": {"type": "integer", "default": "wrong", "example": "wrong"}}}), "openapi31").finish()
        self.outcome(p, "invalid_inventory")
        p.inline("/components/schemas/S/example", "/components/schemas/S").finish()
        self.outcome(p, "invalid_schema")

    def fault(self, activation, outcome, status, expected="validation_unavailable", deadline=False, death=False):
        """Private v1 fault seam: REAL children, bounded channels, no fake success."""
        real_popen, real_clock = subprocess.Popen, time.monotonic
        saved = []
        ready_for_expiry = [False]
        base = real_clock()
        descriptor_owner = contextlib.ExitStack()
        owned_fds = set()
        reaped = False
        watchdogs = []
        watchdog_fired = threading.Event()

        def clock():
            return base + 11 if ready_for_expiry[0] else base

        def close_owned(fd):
            if fd in owned_fds:
                owned_fds.remove(fd)
                os.close(fd)

        def owned_pipe():
            ends = os.pipe()
            for fd in ends:
                owned_fds.add(fd)
                descriptor_owner.callback(close_owned, fd)
            return ends

        def spawn(argv, **kwargs):
            self.assertEqual(len(saved), 0)
            self.assertIn(type(argv), (list, tuple))
            self.assertEqual(len(argv), 12)
            self.assertTrue(Path(argv[0]).is_absolute())
            self.assertEqual(list(argv[1:3]), ["-I", "-S"])
            self.assertTrue(Path(argv[3]).is_absolute())
            self.assertEqual(list(argv[4::2]), ["--manifest", "--sdk-dir", "--activation-fd", "--outcome-fd"])
            self.assertTrue(Path(argv[7]).is_absolute())
            numbers = argv[9], argv[11]
            for number in numbers:
                self.assertEqual(str(int(number)), number)
                self.assertGreaterEqual(int(number), 3)
            first, second = map(int, numbers)
            self.assertNotEqual(first, second)
            self.assertEqual(set(kwargs["pass_fds"]), {first, second})
            self.assertEqual(len(kwargs["pass_fds"]), 2)
            for key in ("stdin", "stdout", "stderr"):
                self.assertEqual(kwargs[key], subprocess.DEVNULL)
            self.assertIs(kwargs["shell"], False)
            self.assertIs(kwargs["close_fds"], True)
            self.assertNotIn("preexec_fn", kwargs)
            self.assertFalse(kwargs.get("text", False))
            code = ("import os,signal; a=int(os.environ['TEST_A']); o=int(os.environ['TEST_O']); "
                    "os.write(a," + repr(activation) + "); os.close(a); "
                    "os.write(o," + repr(outcome) + "); os.close(o); ")
            descriptors = [first, second]
            env = {"TEST_A": str(first), "TEST_O": str(second)}
            ready_read = None
            if deadline:
                ready_read, ready_write = owned_pipe()
                release_read, release_writer = owned_pipe()
                descriptors.extend((ready_write, release_read))
                env.update(TEST_R=str(ready_write), TEST_B=str(release_read))
                code = ("import os; os.write(int(os.environ['TEST_R']),b'R'); "
                        "os.close(int(os.environ['TEST_R'])); os.read(int(os.environ['TEST_B']),1)")
            elif death:
                code += "os.kill(os.getpid(),signal.SIGKILL)"
            else:
                code += "os._exit(" + str(status) + ")"
            try:
                process = real_popen([sys.executable, "-I", "-S", "-c", code],
                                     shell=False, close_fds=True, stdin=subprocess.DEVNULL,
                                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                     pass_fds=tuple(descriptors), env=env)
                saved.append(process)
                def watchdog():
                    watchdog_fired.set()
                    try:
                        os.kill(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                timer = threading.Timer(5, watchdog)
                timer.daemon = True
                watchdogs.append(timer)
                timer.start()
            finally:
                if deadline:
                    close_owned(ready_write)
                    close_owned(release_read)
            if deadline:
                try:
                    # Kernel readiness timeout is a test watchdog, never its oracle.
                    readable, _, _ = select.select([ready_read], [], [], 3)
                    self.assertEqual(readable, [ready_read], "fault child readiness watchdog")
                    self.assertEqual(os.read(ready_read, 2), b"R")
                    ready_for_expiry[0] = True
                finally:
                    close_owned(ready_read)
            return process

        p = self.package(b"1", schema(type="integer"))
        try:
            with mock.patch.object(subprocess, "Popen", side_effect=spawn):
                if deadline:
                    with mock.patch.object(time, "monotonic", side_effect=clock):
                        self.outcome(p, expected)
                else:
                    self.outcome(p, expected)
            self.assertEqual(len(saved), 1)
            # No poll/wait/test cleanup is allowed before this proof of parent reap.
            with self.assertRaises(ChildProcessError):
                os.waitpid(saved[0].pid, os.WNOHANG)
            reaped = True
            self.assertFalse(watchdog_fired.is_set(), "real fault-child watchdog fired")
        finally:
            for timer in watchdogs:
                timer.cancel()
                timer.join(timeout=1)
            descriptor_owner.close()
            if saved and not reaped:
                try:
                    saved[0].kill()
                except ProcessLookupError:
                    pass
                saved[0].wait(timeout=3)

    def test_worker_activation_missing_extra_and_malformed_refuse_forged_ok(self):
        for activation in (b"", b"AA", b"B"):
            with self.subTest(activation=activation):
                self.fault(activation, b'{"code":"ok"}\n', 0)

    def test_worker_outcome_bounded_exact_closed_protocol(self):
        rows = ((b"x" * 257, 0), (b'{"code":"unknown"}\n', 0),
                (b'{"code":"ok"}', 0), (b'{"code":"ok"}\nextra', 0),
                (b'{"code":"usage_error"}\n', 64), (b"", 0),
                (b'{"code":"invalid_json","secret":"PRIVATE"}\n', 2))
        for index, (payload, status) in enumerate(rows):
            with self.subTest(protocol_case=index):
                self.fault(b"A", payload, status)

    def test_worker_status_mismatch_and_unclassified_signal(self):
        self.fault(b"A", b'{"code":"invalid_json"}\n', 0)
        self.fault(b"A", b"", 0, death=True)

    def test_worker_startup_deadline_kills_and_reaps(self):
        self.fault(b"", b"", 0, "resource_limit", deadline=True)


if __name__ == "__main__":
    unittest.main(verbosity=2)
