#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check actual repository artifacts with the admitted real standards libraries."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import runpy
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCHEMAS = ROOT / "schemas"
PACKAGE = SCHEMAS / "http/reference-v1"
API_URI = "https://schemas.subathon.invalid/reference-v1/api.json"
PROGRESS_URI = "https://schemas.subathon.invalid/progress/state.schema.json"
DRAFT_URI = "https://json-schema.org/draft/2020-12/schema"

# Only structural OpenAPI children are visited. Example values and metadata stay data;
# real Schema Objects delegate child traversal to the selected Draft resource API.
OAS_CHILDREN = {
    "Document": (("paths", "map", "PathItem"), ("webhooks", "map", "PathItem"),
                 ("components", "one", "Components")),
    "Components": (("schemas", "map", "Schema"), ("responses", "map", "Response"),
                   ("parameters", "map", "Parameter"), ("requestBodies", "map", "RequestBody"),
                   ("headers", "map", "Header"), ("examples", "map", "Example"),
                   ("securitySchemes", "map", "SecurityScheme"), ("pathItems", "map", "PathItem"),
                   ("callbacks", "map", "Callback"), ("links", "map", "Link")),
    "PathItem": tuple((method, "one", "Operation") for method in
                      ("get", "put", "post", "delete", "options", "head", "patch", "trace"))
                + (("parameters", "array", "Parameter"),),
    "Operation": (("parameters", "array", "Parameter"), ("requestBody", "one", "RequestBody"),
                  ("responses", "map", "Response"), ("callbacks", "map", "Callback")),
    "Response": (("headers", "map", "Header"), ("content", "map", "MediaType")),
    "Parameter": (("schema", "one", "Schema"), ("content", "map", "MediaType"),
                  ("examples", "map", "Example")),
    "Header": (("schema", "one", "Schema"), ("content", "map", "MediaType"),
               ("examples", "map", "Example")),
    "RequestBody": (("content", "map", "MediaType"),),
    "MediaType": (("schema", "one", "Schema"), ("examples", "map", "Example"),
                  ("encoding", "map", "Encoding")),
    "Encoding": (("headers", "map", "Header"),),
}


def unique_object(pairs):
    result = {}
    for name, value in pairs:
        if name in result:
            raise ValueError("Duplicate decoded JSON object member")
        result[name] = value
    return result


def invalid_constant(_value):
    raise ValueError("Non-JSON numeric constant")


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_object,
                      parse_constant=invalid_constant)


def openapi_nodes(value, role="Document"):
    """Yield raw Schema Objects and structural references, without following aliases."""
    if role == "Schema":
        yield "schema", value
        return
    if not isinstance(value, dict):
        return  # The real OpenAPI structural validator reports malformed objects.
    if "$ref" in value:
        yield "reference", value["$ref"]
        return
    if role == "Callback":
        children = ((child, "PathItem") for child in value.values())
    else:
        children = []
        for name, shape, child_role in OAS_CHILDREN.get(role, ()):
            child = value.get(name)
            if shape == "map" and isinstance(child, dict):
                children.extend((member, child_role) for member in child.values())
            elif shape == "array" and isinstance(child, list):
                children.extend((member, child_role) for member in child)
            elif shape == "one" and child is not None:
                children.append((child, child_role))
    for child, child_role in children:
        yield from openapi_nodes(child, child_role)


class RepositorySchemas(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # This stdlib-only admission precedes every candidate import; -S prevents .pth
        # startup and only the admitted SDK directory is appended afterward.
        location = ROOT / "tools/schema_validation/check_dependencies.py"
        spec = importlib.util.spec_from_file_location("schema_dependencies", location)
        admission = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(admission)
        sdk = admission.check_sdk_metadata(ROOT)
        sys.path.append(str(sdk))

        from jsonschema import Draft202012Validator
        from jsonschema.exceptions import ValidationError
        from jsonschema_path import SchemaPath
        from jsonschema_path.accessors import SchemaAccessor
        from openapi_schema_validator import OAS31Validator
        from openapi_schema_validator._specifications import REGISTRY as official
        from openapi_spec_validator import OpenAPIV31SpecValidator
        from referencing import Registry
        from referencing.exceptions import NoSuchResource
        from referencing.jsonschema import DRAFT202012

        def deny(uri):
            raise NoSuchResource(ref=uri)

        cls.Draft = Draft202012Validator
        cls.OAS = OAS31Validator
        cls.ValidationError = ValidationError
        cls.SchemaPath = SchemaPath
        cls.SchemaAccessor = SchemaAccessor
        cls.resource_spec = DRAFT202012
        cls.official = Registry(retrieve=deny).with_resources(
            (uri, official[uri]) for uri in official)
        cls.api = read_json(PACKAGE / "api.json")
        cls.progress = read_json(ROOT / "progress/state.schema.json")
        cls.manifest = read_json(PACKAGE / "manifest.json")
        cls.registry = cls.official.with_resources((
            (API_URI, DRAFT202012.create_resource(cls.api)),
            (PROGRESS_URI, DRAFT202012.create_resource(cls.progress)),
        ))
        cls.schemas = [value for kind, value in openapi_nodes(cls.api) if kind == "schema"]
        cls.examples = []
        for binding in cls.manifest["examples"]:
            path = (PACKAGE / binding["path"]).resolve()
            if not path.is_relative_to(PACKAGE.resolve()):
                raise ValueError("Repository example path leaves the schema package")
            cls.examples.append((path, binding))
        fixtures = runpy.run_path(str(Path(__file__).with_name("schema_cases.py")))
        cls.openapi_cases = fixtures["OPENAPI_CASES"]
        cls.progress_cases = fixtures["PROGRESS_CASES"]

        # Reuse the admitted real bundled metaschema instead of the SDK's lazy URL
        # wrapper. The semantic validator and all assertion evaluation stay genuine.
        raw = (sdk / "openapi_spec_validator/resources/schemas/v3.1/schema.json").read_bytes()
        if (len(raw) != 34260 or hashlib.sha256(raw).hexdigest()
                != "e7cb616a2a10849a166c4e4a93c62c56cfea02cc00eadf287e2fb875e7124098"):
            raise ValueError("Bundled OpenAPI metaschema differs from its admitted artifact")
        structural = Draft202012Validator(json.loads(raw), registry=cls.official)

        class OfflineOpenAPI(OpenAPIV31SpecValidator):
            schema_validator = structural

        cls.OpenAPI = OfflineOpenAPI

    def specification(self, document):
        registry = self.official.with_resource(API_URI, self.resource_spec.create_resource(document))
        accessor = self.SchemaAccessor(document, registry.resolver(base_uri=API_URI),
                                       resolved_cache_maxsize=0)
        return self.OpenAPI(self.SchemaPath(accessor))

    def instance_validator(self, pointer):
        return self.OAS({"$ref": API_URI + "#" + pointer}, registry=self.registry)

    def schema_references(self, schema):
        resource = self.resource_spec.create_resource(schema)
        if isinstance(schema, dict) and "$ref" in schema:
            yield schema["$ref"]
        for child in resource.subresources():
            yield from self.schema_references(child.contents)

    def test_inventory_includes_every_actual_schema_target_and_example(self):
        self.assertEqual(self.manifest["documents"], [{"path": "api.json", "kind": "openapi31"}])
        schema_files = {path.resolve() for path in SCHEMAS.rglob("*.json")}
        example_files = {path.resolve() for path in (PACKAGE / "examples").glob("*.json")}
        declared_examples = [path for path, _binding in self.examples]
        self.assertEqual(set(declared_examples), example_files)
        self.assertEqual(len(declared_examples), len(set(declared_examples)))
        self.assertEqual(schema_files, example_files | {
            (PACKAGE / "api.json").resolve(), (PACKAGE / "manifest.json").resolve()})
        self.assertEqual(self.progress["$schema"], DRAFT_URI)
        components = set(self.api["components"]["schemas"])
        for validity in (True, False):
            self.assertEqual({target for _name, target, _value, valid in self.openapi_cases
                              if valid is validity}, components)
        names = [name for name, _target, _value, _valid in self.openapi_cases]
        self.assertEqual(len(names), len(set(names)))

    def test_whole_openapi_and_each_schema_definition(self):
        self.specification(self.api).validate()
        for schema in self.schemas:
            with self.subTest(schema=schema):
                self.OAS.check_schema(schema)

    def test_invalid_openapi_document_is_rejected(self):
        invalid = copy.deepcopy(self.api)
        del invalid["info"]["version"]  # OpenAPI requires info.version.
        with self.assertRaises(self.ValidationError):
            self.specification(invalid).validate()

    def test_all_current_references_resolve_without_retrieval(self):
        references = [value for kind, value in openapi_nodes(self.api) if kind == "reference"]
        for schema in self.schemas:
            references.extend(self.schema_references(schema))
        for reference in references:
            with self.subTest(reference=reference):
                self.assertIsInstance(reference, str)
                self.assertTrue(reference.startswith("#"), "Review a new external-resource policy")
                self.registry.resolver(base_uri=API_URI).lookup(reference)
        for reference in self.schema_references(self.progress):
            with self.subTest(progress_reference=reference):
                self.assertTrue(reference.startswith("#"), "Review a new external-resource policy")
                self.registry.resolver(base_uri=PROGRESS_URI).lookup(reference)

    def test_all_declared_positive_examples(self):
        for path, binding in self.examples:
            with self.subTest(example=path.name):
                self.assertEqual(binding["schema_document"], "api.json")
                self.instance_validator(binding["schema_pointer"]).validate(read_json(path))

    def test_independent_component_examples(self):
        for name, target, value, valid in self.openapi_cases:
            with self.subTest(case=name, expected_valid=valid):
                validator = self.instance_validator("/components/schemas/" + target)
                if valid:
                    validator.validate(value)
                else:
                    with self.assertRaises(self.ValidationError):
                        validator.validate(value)

    def test_progress_schema_and_actual_state(self):
        self.Draft.check_schema(self.progress)
        self.Draft(self.progress, registry=self.registry).validate(read_json(ROOT / "progress/state.json"))

    def test_independent_progress_examples(self):
        validator = self.Draft(self.progress, registry=self.registry)  # format is annotation-only.
        for name, value, valid in self.progress_cases:
            with self.subTest(case=name, expected_valid=valid):
                if valid:
                    validator.validate(value)
                else:
                    with self.assertRaises(self.ValidationError):
                        validator.validate(value)


if __name__ == "__main__":
    unittest.main(verbosity=2)
