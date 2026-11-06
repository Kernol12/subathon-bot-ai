# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded metadata adapter; real selected SDKs own schema assertion semantics."""
import hashlib
import json
import os
from urllib.parse import unquote, urldefrag, urljoin

from jsonschema import Draft202012Validator
from jsonschema.exceptions import SchemaError, ValidationError
from jsonschema_path import SchemaPath
from jsonschema_path.accessors import SchemaAccessor
from openapi_schema_validator import OAS31Validator
from openapi_schema_validator._specifications import REGISTRY as OFFICIAL
from openapi_spec_validator import OpenAPIV31SpecValidator
from referencing import Registry, Specification
from referencing._core import Resolved
from referencing.exceptions import NoSuchResource, Unresolvable
from referencing.jsonschema import DRAFT202012

from _schema_common import BASE, DRAFT, OAS, Refusal, pointer_child, pointer_ok, raw_pointer, refuse
from _schema_index import REF_ROLES
from _schema_installation import metadata_error, regular_bytes


def deny(uri):
    raise NoSuchResource(ref=uri)


class Backend:
    def __init__(self, index, sdk):
        self.index = index
        self.budget = index.budget
        self.documents = {name: self.budget.key(BASE + name) for name in index.specs}
        self.resources = {}
        self.anchors = {}
        self.targets = {}
        self.by_object = {id(n.value): n for n in index.schemas if type(n.value) is dict}
        self.oas_documents = {id(n.value): n for n in index.roots if n.role == "Document"}
        self.schema_roots = {}
        for node in index.schemas:
            if node.parent_schema is None and index.specs[node.document] == "openapi31":
                self.schema_roots.setdefault(node.document, []).append(node)
        self.official = Registry(retrieve=deny).with_resources((uri, OFFICIAL[uri]) for uri in OFFICIAL).crawl()
        path = os.path.join(sdk, "openapi_spec_validator/resources/schemas/v3.1/schema.json")
        try:
            raw = regular_bytes(path, 34260)
            if len(raw) != 34260 or hashlib.sha256(raw).hexdigest() != "e7cb616a2a10849a166c4e4a93c62c56cfea02cc00eadf287e2fb875e7124098":
                refuse("dependency_unavailable")
            self.oas_meta = json.loads(raw)
        except (OSError, ValueError) as error:
            metadata_error(error)
            refuse("dependency_unavailable")
        self.oas_structure = Draft202012Validator(self.oas_meta, registry=self.official)

    def validator_class(self, node):
        return Draft202012Validator if node.dialect == DRAFT else OAS31Validator

    def validate(self, bindings, values, aliases):
        if self.index.unsupported or any(s is None for _, s in self.index.annotations + self.index.example_uses):
            refuse("unsupported_feature")
        self.structural()
        self.registry_and_scopes()
        self.closure()
        self.cycles()
        instances = self.inventory(bindings, values, aliases)
        self.semantics()
        for value, schema in instances:
            self.instance(schema, value, "invalid_instance")

    def structural(self):
        try:
            # Complete native structural validation precedes all package lookup.
            for name, kind in self.index.specs.items():
                if kind == "openapi31":
                    self.oas_structure.validate(self.index.values[name])
                for node in self.index.schemas:
                    if node.document == name:
                        self.validator_class(node).check_schema(node.value)
        except (SchemaError, ValidationError):
            refuse("invalid_schema")

    def add_resource(self, uri, node):
        if not uri.startswith(BASE) or urldefrag(uri)[1]:
            refuse("unresolved_reference")
        previous = self.resources.get(uri)
        if previous is not None and previous.identity != node.identity:
            refuse("unresolved_reference")
        if previous is None:
            if len(self.resources) >= 256:
                refuse("resource_limit")
            self.resources[self.budget.key(uri)] = node

    def registry_and_scopes(self):
        # Scope traversal follows actual indexed schema ancestry and native IDs.
        empty = Registry(retrieve=deny)
        for root in self.index.roots:
            self.add_resource(self.documents[root.document], root)
        for node in self.index.schemas:
            parent = node.parent_schema
            base = parent.uri if parent is not None else self.documents[node.document]
            resource = DRAFT202012.create_resource(node.value)
            identifier = resource.id()
            if identifier is not None:
                self.budget.key(identifier)
            scope = empty.resolver(base_uri=base).in_subresource(resource)
            # SchemaAccessor's public property exposes the SDK-computed base.
            node.uri = self.budget.key(SchemaAccessor(node.value, scope, resolved_cache_maxsize=0).base_uri)
            if identifier is not None:
                # A root's retrieval URI and its canonical ID may both name it.
                self.add_resource(node.uri, node)
            for anchor in resource.anchors():
                key = node.uri, anchor.name
                self.budget.key(anchor.name)
                if key in self.anchors:
                    refuse("unresolved_reference")
                self.anchors[key] = node

        def id_of(value):
            return DRAFT202012.id_of(value) if type(value) is bool or id(value) in self.by_object else None

        def subresources(value):
            root = self.oas_documents.get(id(value))
            if root is not None:
                return (n.value for n in self.schema_roots.get(root.document, ()))
            if type(value) is bool or id(value) in self.by_object:
                return DRAFT202012.subresources_of(value)
            return ()

        def anchors(_specification, value):
            return DRAFT202012.anchors_in(value) if type(value) is bool or id(value) in self.by_object else ()

        def enter(segments, resolver, subresource):
            resource = subresource
            value = resource.contents
            def matches(candidate):
                parts = self.parts(candidate.pointer)
                return len(parts) == len(segments) and all(a == str(b) for a, b in zip(parts, segments))
            node = self.by_object.get(id(value)) if type(value) is dict else None
            if node is None and type(value) is bool:
                # Boolean identities are shared; the exact known transition path
                # disambiguates them, never id(True) as a physical identity.
                for candidates in self.schema_roots.values():
                    node = next((n for n in candidates if n.value is value
                                 and matches(n)), None)
                    if node is not None:
                        break
            if (node is not None and node.parent_schema is None
                    and self.index.specs[node.document] == "openapi31"
                    and matches(node)):
                # A fresh SAME-base resolver is essential for no-id Schema roots:
                # Resource.pointer resets the accumulated OAS-container segments.
                fresh = self.registry.resolver(base_uri=self.documents[node.document])
                return fresh.in_subresource(resource)
            if node is not None or type(value) is bool:
                return DRAFT202012.maybe_in_subresource(segments, resolver, resource)
            return resolver

        bridge = Specification(name="subathon-oas31-physical-schema-context", id_of=id_of,
                               subresources_of=subresources, anchors_in=anchors, maybe_in_subresource=enter)
        registry = Registry(retrieve=deny)
        for root in self.index.roots:
            specification = bridge if root.role == "Document" else DRAFT202012
            registry = registry.with_resource(self.documents[root.document], specification.create_resource(root.value))
        self.registry = registry.crawl()
        # All IDs/anchors were checked BEFORE crawl could overwrite a declaration.
        for node in self.index.schemas:
            node.scope = self.registry.resolver(base_uri=node.uri)

    @staticmethod
    def parts(pointer):
        return tuple(x.replace("~1", "/").replace("~0", "~") for x in pointer[1:].split("/")) if pointer else ()

    def locate(self, node, reference):
        resolver = node.scope if node.role == "Schema" else self.registry.resolver(base_uri=self.documents[node.document])
        base = node.uri if node.role == "Schema" else self.documents[node.document]
        # Native lookup owns URI/resource/pointer/anchor semantics. Physical
        # metadata below verifies the required target role, not assertion truth.
        try:
            resolved = resolver.lookup(reference)
            absolute = urljoin(base, reference)
            self.budget.key(absolute)
            uri, fragment = urldefrag(absolute)
            resource = self.resources.get(uri)
            if resource is None:
                refuse("unresolved_reference")
            if fragment.startswith("/"):
                pointer = unquote(fragment, encoding="utf-8", errors="strict")
                if not pointer_ok(pointer):
                    refuse("unresolved_reference")
                value = resource.value
                physical = resource.pointer
                for part in self.parts(pointer):
                    if type(value) is dict and part in value:
                        value = value[part]
                    elif (type(value) is list and part.isascii() and part.isdigit()
                          and (part == "0" or not part.startswith("0")) and len(part) < 8 and int(part) < len(value)):
                        part = int(part)
                        value = value[part]
                    else:
                        refuse("unresolved_reference")
                    physical = pointer_child(physical, part)
                target = self.index.nodes.get((resource.document, physical, node.role))
            elif fragment:
                target = self.anchors.get((uri, fragment))
            else:
                target = resource if resource.role == node.role else None
            if target is None or target.role != node.role or target.value is not resolved.contents:
                refuse("unresolved_reference")
            return target
        except (Unresolvable, KeyError, IndexError, TypeError, ValueError, UnicodeError):
            refuse("unresolved_reference")

    def closure(self):
        for node in sorted(self.index.references, key=lambda n: (self.index.order[n.document], n.pointer)):
            self.targets[node.identity] = self.locate(node, node.value["$ref"])

    def cycles(self):
        outgoing = {node.identity: [] for node in self.index.schemas}
        for parent, child, kind in self.index.applications:
            if kind == "same":
                outgoing[parent.identity].append(child.identity)
        for source, target in self.targets.items():
            outgoing.setdefault(source, []).append(target.identity)
        colors = {}
        for start in outgoing:
            if colors.get(start):
                continue
            colors[start] = 1
            stack = [(start, iter(outgoing[start]))]
            while stack:
                current, edges = stack[-1]
                target = next(edges, None)
                if target is None:
                    colors[current] = 2
                    stack.pop()
                elif colors.get(target) == 1:
                    refuse("reference_cycle")
                elif not colors.get(target):
                    colors[target] = 1
                    stack.append((target, iter(outgoing.get(target, ()))))

    def alias(self, node):
        # Complete closure/cycle partition is already finished; no recursion or IO.
        while node.identity in self.targets:
            node = self.targets[node.identity]
        return node

    def inventory(self, bindings, values, aliases):
        required, free, forced = set(), set(), set()
        def require(value, schema):
            pair = value.identity, schema.identity
            if pair not in required:
                if len(required) >= 256:
                    refuse("resource_limit")
                required.add(pair)
            forced.add(value.identity)
        for value, schema in self.index.annotations:
            require(value, schema)
        for example, schema in self.index.example_uses:
            value = self.index.example_values.get(self.alias(example).identity)
            if value is not None:
                require(value, schema)
        for example in self.index.component_examples:
            value = self.index.example_values.get(self.alias(example).identity)
            if value is not None and value.identity not in forced:
                free.add(value.identity)
        if len(required) + len(free) > 256:
            refuse("resource_limit")
        actual, used_free, instances = set(), set(), []
        for binding in bindings:
            target = self.index.nodes.get((binding["schema_document"], binding["schema_pointer"], "Schema"))
            if target is None:
                refuse("invalid_inventory")
            source = binding["path"]
            if type(source) is str:
                identity = (aliases[source], "", "External")
                value = values[source]
            else:
                data = self.index.nodes.get((source["document"], source["pointer"], "Data"))
                if data is None:
                    refuse("invalid_inventory")
                identity, value = data.identity, data.value
                pair = identity, target.identity
                if pair not in required and identity not in free:
                    refuse("invalid_inventory")
                if identity in free:
                    used_free.add(identity)
            pair = identity, target.identity
            if pair in actual:
                refuse("invalid_inventory")
            actual.add(pair)
            instances.append((value, target))
        if not required.issubset(actual) or used_free != free:
            refuse("invalid_inventory")
        return instances

    def instance(self, schema, value, error):
        uri = self.budget.control(self.documents[schema.document], schema.pointer)
        try:
            self.validator_class(schema)({"$ref": uri}, registry=self.registry).validate(value)
        except (SchemaError, ValidationError):
            refuse(error)
        finally:
            # Temporary control ownership ends with this one native evaluation.
            self.budget.key_bytes -= len(uri.encode("utf-8")) + 4

    def path_cursor(self, name):
        backend = self
        class ContextAccessor(SchemaAccessor):
            def context(self, parts):
                document, pointer = name, ""
                value = backend.index.values[document]
                scope = backend.registry.resolver(base_uri=backend.documents[document])
                data = False
                for position in range(len(parts) + 1):
                    candidates = (backend.index.nodes.get((document, pointer, role)) for role in REF_ROLES | {"Schema"})
                    node = next((n for n in candidates if n is not None), None)
                    if node is not None:
                        node = backend.alias(node)
                        document, pointer, value = node.document, node.pointer, node.value
                        if node.role == "Schema":
                            scope = node.scope
                        else:
                            scope = backend.registry.resolver(base_uri=backend.documents[document])
                    data = data or (document, pointer, "Data") in backend.index.nodes
                    if position == len(parts):
                        return Resolved(contents=value, resolver=scope), data
                    part = parts[position]
                    value = value[part]
                    pointer = pointer_child(pointer, part)
                raise AssertionError("finite path")

            def get_resolved(self, parts):
                return self.context(parts)[0]

        class RawDataPath(SchemaPath):
            def read_value(self):
                # The real context accessor keeps literal DATA opaque, including
                # aliases reached through genuine typed Reference Object edges.
                resolved, data = self.accessor.context(self.parts)
                return resolved.contents if data else super().read_value()

        accessor = ContextAccessor(self.index.values[name], self.registry.resolver(base_uri=self.documents[name]), resolved_cache_maxsize=0)
        return RawDataPath(accessor)

    def semantics(self):
        # Keep the genuine native semantic validators while supplying the actual
        # admitted bundled metaschema, not the SDK's URL-reading lazy wrapper.
        class OfflineOpenAPI(OpenAPIV31SpecValidator):
            schema_validator = self.oas_structure
        try:
            for name, kind in self.index.specs.items():
                if kind == "openapi31":
                    OfflineOpenAPI(self.path_cursor(name)).validate()
            for value, schema in self.index.defaults:
                self.instance(schema, value.value, "invalid_schema")
        except (SchemaError, ValidationError):
            refuse("invalid_schema")
