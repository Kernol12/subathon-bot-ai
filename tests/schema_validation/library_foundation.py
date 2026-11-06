#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fixed real-library foundations; no project wrapper or package validator runs."""
import importlib.metadata
import hashlib
import io
import json
import os
import unittest
from contextlib import ExitStack
from fractions import Fraction
from pathlib import Path
from unittest.mock import patch


NETWORK_ATTEMPTS = []  # Never reset: a swallowed attempt cannot make any case green.
TRUSTED_LOCAL_READS = []
OAS31_RESOURCE = "openapi_spec_validator/resources/schemas/v3.1/schema.json"
OAS31_SHA256 = "e7cb616a2a10849a166c4e4a93c62c56cfea02cc00eadf287e2fb875e7124098"


class FoundationNetworkAttempt(BaseException):
    pass


def block_network(*_args, **_kwargs):
    NETWORK_ATTEMPTS.append("network attempted")
    raise FoundationNetworkAttempt("Library foundation forbids network")


def guarded_urlopen(url, *_args, **_kwargs):
    # The oracle is the member of independently hash-admitted wheel222fec..., not this file.
    distribution = importlib.metadata.distribution("openapi-spec-validator")
    resource = Path(distribution.locate_file(OAS31_RESOURCE)).absolute()
    if type(url) is str and url == resource.as_uri():
        if distribution.version != "0.9.0" or resource.is_symlink():
            raise FoundationNetworkAttempt("Untrusted bundled resource identity")
        with resource.open("rb") as stream:
            data = stream.read(34261)
        if len(data) != 34260 or hashlib.sha256(data).hexdigest() != OAS31_SHA256:
            raise FoundationNetworkAttempt("Bundled resource differs from admitted wheel")
        TRUSTED_LOCAL_READS.append("admitted OAS3.1 metaschema")
        return io.BytesIO(data)  # Real verified bundled bytes; never a validating callback.
    return block_network()


class LibraryFoundation(unittest.TestCase):
    def setUp(self):
        guards = ExitStack()
        self.addCleanup(guards.close)
        self.addCleanup(lambda: self.assertEqual(NETWORK_ATTEMPTS, []))
        for target in (
            "socket.getaddrinfo", "socket.create_connection", "socket.socket.connect",
            "socket.socket.connect_ex",
        ):
            guards.enter_context(patch(target, side_effect=block_network))
        guards.enter_context(patch("urllib.request.urlopen", side_effect=guarded_urlopen))
        guards.enter_context(patch.dict(os.environ, {
            "OPENAPI_SPEC_VALIDATOR_SCHEMA_VALIDATOR_BACKEND": "jsonschema",
            "OPENAPI_SPEC_VALIDATOR_RESOLVED_CACHE_MAXSIZE": "0",
            "OPENAPI_SCHEMA_VALIDATOR_COMPILED_VALIDATOR_CACHE_MAX_SIZE": "0",
        }))
        # Candidate imports happen here, AFTER guards, including first import.
        from jsonschema import Draft202012Validator
        from jsonschema.exceptions import ValidationError
        from jsonschema_path import SchemaPath
        from openapi_schema_validator import OAS31Validator
        from openapi_spec_validator import OpenAPIV31SpecValidator
        from referencing import Registry
        self.classes = (Draft202012Validator, OAS31Validator)
        self.error = ValidationError
        self.path_class = SchemaPath
        self.spec_class = OpenAPIV31SpecValidator
        self.registry_class = Registry

    def test_exact_admitted_twenty_versions(self):
        manifest = Path(__file__).resolve().parents[2] / "tools/schema_validation/wheels.json"
        packages = json.loads(manifest.read_text(encoding="utf-8"))["packages"]
        normalize = lambda name: name.lower().replace("_", "-")
        expected = {normalize(p["name"]): p["version"] for p in packages}
        self.assertEqual(len(packages), 20)
        self.assertEqual(len(expected), 20)
        actual = {normalize(d.metadata["Name"]): d.version
                  for d in importlib.metadata.distributions()}
        actual.pop("pip", None)  # The isolated install's bootstrap only, never a validator extra.
        self.assertEqual(actual, expected)

    def test_explicit_openapi31_and_separate_example_validation(self):
        schema = {"type": "integer"}
        document = {"openapi": "3.1.1", "info": {"title": "Synthetic", "version": "1"},
                    "paths": {"/sample": {"get": {"responses": {"200": {
                        "description": "Synthetic", "content": {"application/json": {
                            "schema": schema, "example": "wrong"}}}}}}}}
        path = self.path_class.from_dict(document, handlers={})
        self.spec_class(path).validate()  # Real spec success does NOT prove this example.
        self.assertEqual(TRUSTED_LOCAL_READS, ["admitted OAS3.1 metaschema"])
        instance_validator = self.classes[1](schema, registry=self.registry_class())
        instance_validator.validate(7)
        with self.assertRaises(self.error):
            instance_validator.validate("wrong")
        invalid = {"openapi": "3.1.1", "paths": {}}
        with self.assertRaises(self.error):
            self.spec_class(self.path_class.from_dict(invalid, handlers={})).validate()

    def test_exact_numeric_library_recipes(self):
        tenth = Fraction(1, 10)
        neighbor = Fraction("0.3000000000000000000000000000000000000001")
        for cls in self.classes:
            with self.subTest(validator=cls.__name__):
                integer = cls({"type": "integer"}, registry=self.registry_class())
                for spelling in ("1.0", "1e0"):
                    literal = Fraction(spelling)
                    self.assertEqual(literal.denominator, 1)
                    integer.validate(literal.numerator)  # Fixed normalized literal, no parser.
                for value in (True, Fraction(3, 2)):
                    with self.assertRaises(self.error):
                        integer.validate(value)
                schema = {"type": "number", "multipleOf": tenth, "minimum": tenth}
                cls.check_schema(schema)
                numbers = cls(schema, registry=self.registry_class())
                numbers.validate(Fraction(3, 10))
                for value in (neighbor, True, Fraction(1, 20)):
                    with self.assertRaises(self.error):
                        numbers.validate(value)
                count = Fraction("1.0")
                self.assertEqual(count.denominator, 1)
                array_schema = {"type": "array", "maxItems": count.numerator}
                cls.check_schema(array_schema)
                cls(array_schema, registry=self.registry_class()).validate([1])
                with self.assertRaises(self.error):
                    cls({"uniqueItems": True}, registry=self.registry_class()).validate(
                        [1, Fraction("1.0").numerator])
                with self.assertRaises(self.error):
                    cls({"enum": [tenth]}, registry=self.registry_class()).validate(
                        tenth + Fraction(1, 10**40))
                with self.assertRaises(self.error):
                    cls({"const": 1}, registry=self.registry_class()).validate(True)

    def test_literal_const_ref_is_data_without_retrieval(self):
        from referencing.exceptions import NoSuchResource
        retrievals = []
        def denied(uri):
            retrievals.append(uri)
            raise NoSuchResource(uri)
        value = {"$ref": "https://outside.invalid/data"}
        for cls in self.classes:
            cls({"const": value}, registry=self.registry_class(retrieve=denied)).validate(value)
        self.assertEqual(retrievals, [])

    def test_explicit_registry_productive_recursion_and_denied_missing(self):
        from referencing.exceptions import NoSuchResource, Unresolvable
        from referencing.jsonschema import DRAFT202012
        retrievals = []
        def denied(uri):
            retrievals.append(uri)
            raise NoSuchResource(uri)
        node_uri = "https://local.invalid/node"
        node = {"$id": node_uri, "type": "object", "additionalProperties": False,
                "properties": {"next": {"$ref": node_uri}}}
        registry = self.registry_class(retrieve=denied).with_resource(
            node_uri, DRAFT202012.create_resource(node))
        for cls in self.classes:
            validator = cls({"$ref": node_uri}, registry=registry)
            validator.validate({"next": {"next": {}}})
            with self.assertRaises(self.error):
                validator.validate({"next": False})
        self.assertEqual(retrievals, [])
        missing = "https://outside.invalid/missing"
        with self.assertRaises(Unresolvable):
            self.classes[0]({"$ref": missing}, registry=registry).validate(1)
        self.assertEqual(retrievals, [missing])

    def test_schemapath_uses_actual_replacement_handler(self):
        from referencing.exceptions import NoSuchResource, Unresolvable
        admitted = "https://local.invalid/integer"
        calls = []
        def memory_only(uri):
            calls.append(uri)
            if uri != admitted:
                raise NoSuchResource(uri)
            return {"type": "integer"}
        path = self.path_class.from_dict(
            {"$ref": admitted}, base_uri="https://local.invalid/root",
            handlers={"https": memory_only})
        self.assertEqual((path / "type").read_value(), "integer")
        self.assertEqual(calls, [admitted])
        missing = "https://outside.invalid/missing"
        refused = self.path_class.from_dict(
            {"$ref": missing}, base_uri="https://local.invalid/root",
            handlers={"https": memory_only})
        with self.assertRaises(Unresolvable):
            (refused / "type").read_value()
        self.assertEqual(calls, [admitted, missing])

    def test_unknown_formats_need_separate_admission(self):
        from jsonschema import FormatChecker
        checker = FormatChecker(formats=[])
        self.assertTrue(checker.conforms("anything", "subathon-unknown-format"))

    def test_sdk_default_data_ref_traversal_is_a_known_spec_validator_trap(self):
        from referencing.exceptions import NoSuchResource, Unresolvable
        retrievals = []
        def denied(uri):
            retrievals.append(uri)
            raise NoSuchResource(uri)
        data_uri = "https://outside.invalid/default-data"
        value = {"$ref": data_uri}
        schema = {"type": "object", "default": value}
        self.classes[1](schema, registry=self.registry_class(retrieve=denied)).validate(value)
        self.assertEqual(retrievals, [])
        document = {"openapi": "3.1.1", "info": {"title": "Synthetic", "version": "1"},
                    "paths": {}, "components": {"schemas": {"Sample": schema}}}
        path = self.path_class.from_dict(document, base_uri="https://local.invalid/api.json",
                                         handlers={"https": denied})
        # Upstream SchemaPath dereferences default DATA. This expected trap is
        # not wrapper acceptance, and the handler never performs a network read.
        with self.assertRaises(Unresolvable):
            self.spec_class(path).validate()
        self.assertEqual(retrievals, [data_uri])

    def test_sdk_uri_fragment_pointers_and_nested_resource_anchor_scope(self):
        from referencing.exceptions import NoSuchResource, PointerToNowhere
        from referencing.jsonschema import DRAFT202012
        retrievals = []
        def denied(uri):
            retrievals.append(uri)
            raise NoSuchResource(uri)
        root_uri = "https://local.invalid/pointers.json"
        leaf = {"$anchor": "leaf", "type": "integer"}
        child = {"$id": "nested/child.json", "$anchor": "child", "$defs": {"leaf": leaf}}
        schema = {"$id": root_uri, "$defs": {
            "a/b": {"type": "integer"}, "a~b": {"type": "string"},
            "p%2Fq": {"type": "boolean"}, "child": child}}
        registry = self.registry_class(retrieve=denied).with_resource(
            root_uri, DRAFT202012.create_resource(schema)).crawl()
        resolver = registry.resolver(base_uri=root_uri)
        self.assertEqual(resolver.lookup("#/$defs/a~1b").contents, {"type": "integer"})
        self.assertEqual(resolver.lookup("#/$defs/a~0b").contents, {"type": "string"})
        self.assertEqual(resolver.lookup("#/$defs/p%252Fq").contents, {"type": "boolean"})
        # lookup consumes a URI fragment, so raw '%' must be percent-encoded.
        with self.assertRaises(PointerToNowhere):
            resolver.lookup("#/$defs/p%2Fq")
        resolved_child = resolver.lookup("#/$defs/child")
        self.assertEqual(resolved_child.contents, child)
        self.assertEqual(resolved_child.resolver.lookup("#leaf").contents, leaf)
        self.assertEqual(resolver.lookup("nested/child.json#child").contents, child)
        self.assertEqual(resolver.lookup("nested/child.json#leaf").contents, leaf)
        self.assertEqual(retrievals, [])

    def test_sdk_schema_subresources_exclude_literal_data_contexts(self):
        from referencing.exceptions import NoSuchResource
        from referencing.jsonschema import DRAFT202012
        retrievals = []
        def denied(uri):
            retrievals.append(uri)
            raise NoSuchResource(uri)
        root_uri = "https://local.invalid/context-root.json"
        property_child = {"$id": "property-child", "type": "integer"}
        defs_child = {"$id": "defs-child", "type": "string"}
        items_child = {"$id": "items-child", "type": "boolean"}
        schema = {"$id": root_uri, "properties": {"$ref": property_child},
                  "$defs": {"chosen": defs_child}, "items": items_child,
                  "const": {"$id": "https://outside.invalid/const", "$ref": "ignored"},
                  "enum": [{"$id": "https://outside.invalid/enum", "$ref": "ignored"}],
                  "default": {"$id": "https://outside.invalid/default", "$ref": "ignored"},
                  "examples": [{"$id": "https://outside.invalid/example", "$ref": "ignored"}]}
        resource = DRAFT202012.create_resource(schema)
        self.assertCountEqual([r.contents for r in resource.subresources()],
                              [property_child, defs_child, items_child])
        registry = self.registry_class(retrieve=denied).with_resource(root_uri, resource).crawl()
        self.assertEqual(set(registry), {root_uri, "https://local.invalid/property-child",
                                        "https://local.invalid/defs-child",
                                        "https://local.invalid/items-child"})
        self.assertEqual(retrievals, [])

    def test_sdk_instance_validation_does_not_close_unused_schema_references(self):
        from referencing.exceptions import NoSuchResource, Unresolvable
        from referencing.jsonschema import DRAFT202012
        retrievals = []
        def denied(uri):
            retrievals.append(uri)
            raise NoSuchResource(uri)
        root_uri = "https://local.invalid/unused.json"
        missing = "https://outside.invalid/unused"
        schema = {"$id": root_uri, "type": "integer", "$defs": {"unused": {"$ref": missing}}}
        registry = self.registry_class(retrieve=denied).with_resource(
            root_uri, DRAFT202012.create_resource(schema)).crawl()
        for cls in self.classes:
            cls(schema, registry=registry).validate(7)
        self.assertEqual(retrievals, [])
        with self.assertRaises(Unresolvable):
            registry.resolver(base_uri=root_uri).lookup(missing)
        self.assertEqual(retrievals, [missing])

    def test_sdk_fixed_raw_default_cursor_retains_native_default_validation(self):
        from referencing.exceptions import NoSuchResource
        retrievals = []
        raw_reads = []
        def denied(uri):
            retrievals.append(uri)
            raise NoSuchResource(uri)
        literal_parts = ("components", "schemas", "Sample", "default")
        def fixture_path(raw_default):
            # Fixed fixture plumbing only: no general schema-role classifier.
            class LiteralDefaultCursor(self.path_class):
                def read_value(cursor):
                    if cursor.parts == literal_parts:
                        raw_reads.append(raw_default)
                        return raw_default  # REAL data, never a validation verdict.
                    return super().read_value()
            schema = {"type": "object", "properties": {"$ref": {"type": "string"}},
                      "required": ["$ref"], "additionalProperties": False, "default": raw_default}
            document = {"openapi": "3.1.1", "info": {"title": "Synthetic", "version": "1"},
                        "paths": {}, "components": {"schemas": {"Sample": schema}}}
            path = LiteralDefaultCursor.from_dict(
                document, base_uri="https://local.invalid/literal-default.json",
                handlers={"https": denied})
            self.assertIsInstance(path / "components" / "schemas" / "Sample" / "default",
                                  LiteralDefaultCursor)
            return path
        valid = {"$ref": "https://outside.invalid/literal-data"}
        invalid = {"$ref": 7}
        self.spec_class(fixture_path(valid)).validate()
        with self.assertRaises(self.error):
            self.spec_class(fixture_path(invalid)).validate()
        self.assertEqual(raw_reads, [valid, invalid])
        self.assertEqual(retrievals, [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
