#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Structural acceptance checks for the #831 binding revision."""
from pathlib import Path
import runpy
import unittest


LEGACY = "subathon.scene-bindings-conditions-timeline.v1"
TYPED = "subathon.scene-announcement-binding.v2"
RESOURCE = "616e6e6f756e63656d656e7473"


def reference(version, definition, **fields):
    return {
        "version": version,
        "kind": "AnnouncementBinding",
        "resourceId": RESOURCE,
        "definitionId": definition,
        **fields,
    }


class AnnouncementBindingShapes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        helpers = runpy.run_path(str(Path(__file__).with_name("test_project_schemas.py")))
        repository_schemas = helpers["RepositorySchemas"]
        repository_schemas.setUpClass()
        cls.Draft = repository_schemas.Draft
        cls.registry = repository_schemas.scene_registry
        cls.bindings_uri = helpers["SCENE_IDS"]["bindings"]

    def assert_shape(self, target, value, expected):
        validator = self.Draft({"$ref": self.bindings_uri + "#/$defs/" + target},
                               registry=self.registry)
        errors = list(validator.iter_errors(value))
        self.assertEqual(not errors, expected,
                         errors[0].message if errors else "unexpectedly accepted shape")

    def test_legacy_ids_still_validate(self):
        for identity in ("7231", "62696e64696e672d7231"):
            with self.subTest(identity=identity):
                binding = reference(LEGACY, identity)
                self.assert_shape("AnnouncementBindingRef", binding, True)
                del binding["version"]
                self.assert_shape("AnnouncementBindingDefinition",
                                  {**binding, "operand": {"field": "announcement.current"}}, True)

    def test_typed_reference_and_definition(self):
        for kind, identity in (("event", "6576656e742d7232"), ("milestone", "6d696c652d7232")):
            binding = reference(TYPED, identity, contentKind=kind)
            definition = {**binding, "operand": {"field": "announcement.current"}}
            for target, value in (("AnnouncementBindingRef", binding),
                                  ("AnnouncementBindingDefinition", definition)):
                with self.subTest(kind=kind, target=target):
                    self.assert_shape(target, value, True)

    def test_invalid_typed_and_legacy_shapes(self):
        for version, fields in ((TYPED, {}), (TYPED, {"contentKind": "mixed"}),
                                (LEGACY, {"contentKind": "event"})):
            binding = reference(version, "7232", **fields)
            with self.subTest(version=version, fields=fields):
                self.assert_shape("AnnouncementBindingRef", binding, False)
        for fields in ({}, {"contentKind": "mixed"}, {"contentKind": "event", "extra": True}):
            binding = reference(TYPED, "7232", **fields)
            with self.subTest(typed_definition_fields=fields):
                self.assert_shape("AnnouncementBindingDefinition",
                                  {**binding, "operand": {"field": "announcement.current"}}, False)
        binding = reference(TYPED, "7232", contentKind="event")
        self.assert_shape("AnnouncementBindingRef", {**binding, "extra": True}, False)
        for operand in ({"field": "timer.main"}, {"field": "announcement.current", "extra": 1}):
            with self.subTest(operand=operand):
                self.assert_shape("AnnouncementBindingDefinition",
                                  {**binding, "operand": operand}, False)


if __name__ == "__main__":
    unittest.main()
