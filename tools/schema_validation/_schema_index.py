# SPDX-License-Identifier: GPL-3.0-or-later
"""Closed physical-role metadata and inventory; assertion truth belongs to SDKs."""
from _schema_common import DRAFT, OAS, Node, refuse, utf8_size


MAP_SCHEMA = {"$defs": "inventory", "definitions": "inventory", "properties": "descending",
              "patternProperties": "descending", "dependentSchemas": "same"}
ARRAY_SCHEMA = {"allOf": "same", "anyOf": "same", "oneOf": "same", "prefixItems": "descending"}
SINGLE_SCHEMA = {"additionalProperties": "descending", "items": "descending", "contains": "descending",
    "unevaluatedItems": "descending", "unevaluatedProperties": "descending", "propertyNames": "descending",
    "if": "same", "then": "same", "else": "same", "not": "same", "contentSchema": "inventory"}
OPERATIONS = ("get", "put", "post", "delete", "options", "head", "patch", "trace")
REF_ROLES = {"Response", "Parameter", "RequestBody", "Header", "Example", "SecurityScheme", "PathItem"}


class Index:
    def __init__(self, specs, values, budget):
        self.specs, self.values = specs, values
        self.budget = budget
        self.order = {name: i for i, name in enumerate(specs)}
        self.nodes = {}
        self.records = []
        self.schemas = []
        self.roots = []
        self.references = []
        self.applications = []
        self.annotations = []
        self.defaults = []
        self.example_uses = []
        self.component_examples = []
        self.example_values = {}
        self.unsupported = []
        for name, kind in specs.items():
            root = self.add(name, "", "Schema" if kind == "jsonschema202012" else "Document", values[name])
            self.roots.append(root)
            self.walk(root)
        self.ordered = sorted(self.nodes.values(), key=lambda n: (self.order[n.document], n.pointer, n.role))
        self.schemas.sort(key=lambda n: (self.order[n.document], n.pointer))

    def add(self, document, pointer, role, value, parent=None, edge="inventory"):
        self.budget.reserve(utf8_size(document) + len(role), single=False)
        identity = document, pointer, role
        if identity in self.nodes:
            self.budget.key_bytes -= utf8_size(document) + len(role) + utf8_size(pointer)
            return self.nodes[identity]
        if len(self.nodes) >= 4096:
            refuse("resource_limit")
        # The sole physical-key table owns each pre-reserved spelling. Other
        # graph tables use the bounded integer record number, never copied keys.
        node = Node(document, pointer, role, value, len(self.records), parent.number if parent else None, edge)
        self.nodes[identity] = node
        self.records.append(node)
        if role == "Schema":
            node.dialect = DRAFT if self.specs[document] == "jsonschema202012" else OAS
            if parent is not None:
                node.dialect = parent.dialect
                self.applications.append((parent.number, node.number, edge))
            self.schemas.append(node)
        return node

    def child(self, parent, key, role):
        value = parent.value
        if type(value) is dict and key in value:
            child = self.add(parent.document, self.budget.pointer(parent.pointer, key), role, value[key])
            self.walk(child)
            return child
        return None

    def mapping(self, parent, key, role):
        value = parent.value
        if type(value) is dict and type(value.get(key)) is dict:
            result = []
            with self.budget.temporary_pointer(parent.pointer, key) as pointer:
                for name in sorted(value[key]):
                    child = self.add(parent.document, self.budget.pointer(pointer, name), role, value[key][name])
                    self.walk(child)
                    result.append(child)
            return result
        return []

    def array(self, parent, key, role):
        value = parent.value
        if type(value) is dict and type(value.get(key)) is list:
            with self.budget.temporary_pointer(parent.pointer, key) as pointer:
                for i, item in enumerate(value[key]):
                    self.walk(self.add(parent.document, self.budget.pointer(pointer, i), role, item))

    def data(self, parent, pointer, value):
        return self.add(parent.document, pointer, "Data", value)

    def walk(self, node):
        value, role = node.value, node.role
        if type(value) is not dict:
            return  # Malformed typed locations remain for native structural validation.
        if (self.specs[node.document] == "openapi31" and role not in {"Data", "Scopes", "SecurityRequirement"}
                and any(key.startswith("x-") for key in value)):
            self.unsupported.append(node)
        if role == "Schema":
            if "$ref" in value and type(value["$ref"]) is str:
                self.reference(node)
            for keyword in ("$dynamicRef", "$dynamicAnchor", "discriminator"):
                if keyword in value:
                    self.unsupported.append(node)
            for key in ("example", "default", "const", "enum"):
                if key in value:
                    child = self.data(node, self.budget.pointer(node.pointer, key), value[key])
                    if key == "example":
                        self.annotations.append((child.number, node.number))
                    elif key == "default" and self.specs[node.document] == "openapi31":
                        self.defaults.append((child.number, node.number))
            if type(value.get("examples")) is list:
                if len(value["examples"]) > 256:
                    refuse("resource_limit")
                with self.budget.temporary_pointer(node.pointer, "examples") as prefix:
                    for i, item in enumerate(value["examples"]):
                        p = self.budget.pointer(prefix, i)
                        self.annotations.append((self.data(node, p, item).number, node.number))
            for keyword, edge in MAP_SCHEMA.items():
                if type(value.get(keyword)) is dict:
                    with self.budget.temporary_pointer(node.pointer, keyword) as p:
                        for key in sorted(value[keyword]):
                            child = self.add(node.document, self.budget.pointer(p, key), "Schema", value[keyword][key], node, edge)
                            self.walk(child)
            for keyword, edge in ARRAY_SCHEMA.items():
                if type(value.get(keyword)) is list:
                    with self.budget.temporary_pointer(node.pointer, keyword) as p:
                        for i, item in enumerate(value[keyword]):
                            self.walk(self.add(node.document, self.budget.pointer(p, i), "Schema", item, node, edge))
            for keyword, edge in SINGLE_SCHEMA.items():
                if keyword in value:
                    self.walk(self.add(node.document, self.budget.pointer(node.pointer, keyword), "Schema", value[keyword], node, edge))
            return
        if role == "Data":
            return  # Atomic borrowed data, including arbitrarily keyword-looking keys.
        if role in REF_ROLES and "$ref" in value:
            if type(value["$ref"]) is str:
                self.reference(node)
            if role == "PathItem" and set(value) != {"$ref"}:
                self.unsupported.append(node)
            return
        if role == "Document":
            self.mapping(node, "paths", "PathItem")
            self.child(node, "components", "Components")
            self.child(node, "info", "Info")
            self.array(node, "servers", "Server")
            self.array(node, "security", "SecurityRequirement")
            self.array(node, "tags", "Tag")
            self.child(node, "externalDocs", "ExternalDocs")
            if "webhooks" in value:
                self.unsupported.append(node)
        elif role == "Components":
            for key, child_role in (("schemas", "Schema"), ("responses", "Response"), ("parameters", "Parameter"),
                    ("requestBodies", "RequestBody"), ("headers", "Header"), ("examples", "Example"),
                    ("securitySchemes", "SecurityScheme"), ("pathItems", "PathItem")):
                children = self.mapping(node, key, child_role)
                if key == "examples":
                    self.component_examples.extend(child.number for child in children)
            if "callbacks" in value or "links" in value:
                self.unsupported.append(node)
        elif role == "PathItem":
            for key in OPERATIONS:
                self.child(node, key, "Operation")
            self.array(node, "parameters", "Parameter")
            self.array(node, "servers", "Server")
        elif role == "Operation":
            self.array(node, "parameters", "Parameter")
            self.child(node, "requestBody", "RequestBody")
            self.child(node, "responses", "Responses")
            self.array(node, "security", "SecurityRequirement")
            self.array(node, "servers", "Server")
            self.child(node, "externalDocs", "ExternalDocs")
            if "callbacks" in value:
                self.unsupported.append(node)
        elif role == "Responses":
            for key in sorted(value):
                if not key.startswith("x-"):
                    self.walk(self.add(node.document, self.budget.pointer(node.pointer, key), "Response", value[key]))
        elif role == "Response":
            self.mapping(node, "headers", "Header")
            self.mapping(node, "content", "MediaType")
            if "links" in value:
                self.unsupported.append(node)
        elif role == "RequestBody":
            self.mapping(node, "content", "MediaType")
        elif role in {"Parameter", "Header", "MediaType"}:
            associated = self.child(node, "schema", "Schema")
            if "example" in value:
                self.annotations.append((self.data(node, self.budget.pointer(node.pointer, "example"), value["example"]).number,
                                         associated.number if associated is not None else None))
            for child in self.mapping(node, "examples", "Example"):
                self.example_uses.append((child.number, associated.number if associated is not None else None))
            if role == "MediaType":
                self.mapping(node, "encoding", "Encoding")
            else:
                self.mapping(node, "content", "MediaType")
        elif role == "Encoding":
            self.mapping(node, "headers", "Header")
        elif role == "Example":
            if "value" in value:
                self.example_values[node.identity] = self.data(node, self.budget.pointer(node.pointer, "value"), value["value"]).number
            if "externalValue" in value:
                self.unsupported.append(node)
        elif role == "Info":
            self.child(node, "contact", "Contact")
            self.child(node, "license", "License")
        elif role == "Server":
            self.mapping(node, "variables", "ServerVariable")
        elif role == "Tag":
            self.child(node, "externalDocs", "ExternalDocs")
        elif role == "SecurityScheme":
            self.child(node, "flows", "OAuthFlows")
        elif role == "OAuthFlows":
            for key in ("implicit", "password", "clientCredentials", "authorizationCode"):
                self.child(node, key, "OAuthFlow")
        elif role == "OAuthFlow":
            self.child(node, "scopes", "Scopes")

    def reference(self, node):
        if len(self.references) >= 2048:
            refuse("resource_limit")
        self.budget.key(node.value["$ref"])
        self.references.append(node)

    def admit_dialects_formats(self):
        for doc, kind in self.specs.items():
            value = self.values[doc]
            if kind == "openapi31" and type(value) is dict:
                if "openapi" in value and value["openapi"] not in ("3.1.0", "3.1.1", "3.1.2"):
                    refuse("unsupported_dialect")
                if "jsonSchemaDialect" in value and value["jsonSchemaDialect"] != OAS:
                    refuse("unsupported_dialect")
            elif kind == "jsonschema202012":
                if type(value) is not dict or value.get("$schema") != DRAFT:
                    refuse("unsupported_dialect")
        for node in self.schemas:
            value = node.value
            if type(value) is not dict:
                continue
            allowed = {DRAFT} if self.specs[node.document] == "jsonschema202012" else {DRAFT, OAS}
            if "$schema" in value:
                if type(value["$schema"]) is not str or value["$schema"] not in allowed:
                    refuse("unsupported_dialect")
                node.dialect = value["$schema"]
            elif node.parent_schema is not None:
                node.dialect = self.records[node.parent_schema].dialect
            if "$vocabulary" in value:
                vocab = value["$vocabulary"]
                known = {"https://json-schema.org/draft/2020-12/vocab/" + x for x in
                         ("core", "applicator", "validation", "unevaluated", "meta-data", "format-annotation", "content")}
                if type(vocab) is not dict or any(k not in known for k in vocab):
                    refuse("unsupported_dialect")
        for node in self.schemas:
            if type(node.value) is dict and "format" in node.value:
                refuse("unsupported_format")
