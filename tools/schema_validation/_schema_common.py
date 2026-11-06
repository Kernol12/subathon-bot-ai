# SPDX-License-Identifier: GPL-3.0-or-later
"""Private fixed budgets and constant refusals; no standards validation engine."""
from dataclasses import dataclass
from contextlib import contextmanager


DRAFT = "https://json-schema.org/draft/2020-12/schema"
OAS = "https://spec.openapis.org/oas/3.1/dialect/base"
BASE = "https://schemas.subathon.invalid/package/"
MEMORY = 536870912
EXIT = {"ok": 0, "usage_error": 64, "resource_limit": 4,
        "io_unavailable": 3, "environment_unavailable": 3,
        "dependency_unavailable": 3, "validation_unavailable": 3}
CODES = frozenset(EXIT) | {"invalid_path", "invalid_json", "invalid_manifest",
    "unsupported_dialect", "unsupported_format", "invalid_schema", "unresolved_reference",
    "reference_cycle", "invalid_inventory", "unsupported_feature", "invalid_instance"}


class Refusal(Exception):
    def __init__(self, code):
        self.code = code
        super().__init__(code)


def refuse(code):
    raise Refusal(code)


def utf8_size(value):
    # Input strings are already bounded by the lexical scan. Count without a copy.
    return sum(1 if ord(c) < 128 else 2 if ord(c) < 2048 else 3 if ord(c) < 65536 else 4
               for c in value)


def scalar_string(value):
    return type(value) is str and not any(0xD800 <= ord(c) <= 0xDFFF for c in value)


def pointer_child(pointer, key, budget=None):
    raw = str(key)
    size = utf8_size(pointer) + 1 + utf8_size(raw) + raw.count("~") + raw.count("/")
    if size > 4096:
        refuse("resource_limit")
    if budget is not None:
        budget.reserve(size)
    segment = raw.replace("~", "~0").replace("/", "~1")
    return pointer + "/" + segment


def pointer_ok(value):
    if not scalar_string(value) or utf8_size(value) > 4096:
        return False
    if value and not value.startswith("/"):
        return False
    i = 0
    while i < len(value):
        if value[i] == "~":
            if i + 1 == len(value) or value[i + 1] not in "01":
                return False
            i += 1
        i += 1
    return True


def raw_pointer(root, pointer):
    """Bounded RFC6901 data addressing only; this never interprets $ref or URI escapes."""
    value = root
    if not pointer_ok(pointer):
        refuse("invalid_inventory")
    if pointer:
        for part in pointer[1:].split("/"):
            part = part.replace("~1", "/").replace("~0", "~")
            if type(value) is dict and part in value:
                value = value[part]
            elif type(value) is list and part.isascii() and part.isdigit() and (part == "0" or not part.startswith("0")):
                number = int(part) if len(part) < 8 else len(value)
                if number >= len(value):
                    refuse("invalid_inventory")
                value = value[number]
            else:
                refuse("invalid_inventory")
    return value


class Budget:
    def __init__(self):
        self.key_bytes = 0

    def reserve(self, size, *, single=True):
        if size < 0 or (single and size > 4096) or self.key_bytes + size > 1048576:
            refuse("resource_limit")
        self.key_bytes += size

    def key(self, value):
        self.reserve(utf8_size(value))
        return value

    def lease(self):
        return KeyLease(self)

    def pointer(self, parent, key):
        return pointer_child(parent, key, self)

    def prefixed(self, prefix, suffix):
        self.reserve(utf8_size(prefix) + utf8_size(suffix))
        return prefix + suffix

    @contextmanager
    def temporary_pointer(self, parent, key):
        value = self.pointer(parent, key)
        try:
            yield value
        finally:
            self.key_bytes -= utf8_size(value)

    @contextmanager
    def uri_workspace(self, base, reference):
        # A checked upper bound covers URI joining BEFORE the library constructs
        # an alias. This is temporary working ownership, not a larger URI limit.
        upper = utf8_size(base) + utf8_size(reference) + 1
        # Separate temporary join result and native resolver-base aliases. This
        # is scratch admission; retain() admits the actual <=4096-byte key before
        # it can become a persistent field or be supplied to native lookup.
        upper *= 2
        self.reserve(upper, single=False)
        retained = [0]
        def admit(value):
            size = utf8_size(value)
            if size > 4096 or size > upper // 2:
                refuse("resource_limit")
            return value
        def retain(value):
            admit(value)
            if retained[0]:
                refuse("resource_limit")
            retained[0] = utf8_size(value)
            return value
        try:
            yield admit, retain
        finally:
            self.key_bytes -= upper - retained[0]

    def control(self, document_uri, pointer):
        # quote(safe='/~') preserves only unreserved bytes and slash. Reserve the
        # expanded absolute URI BEFORE quote/concatenation; '%' is never free.
        safe = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~/"
        projected = utf8_size(document_uri) + 1
        for char in pointer:
            projected += 1 if char in safe else 3 * utf8_size(char)
        if projected > 4096 or self.key_bytes + projected + 4 > 1048576:
            refuse("resource_limit")
        self.key_bytes += projected + 4  # URI occurrence plus the control '$ref' key.
        from urllib.parse import quote
        return document_uri + "#" + quote(pointer, safe="/~")


class KeyLease:
    """Explicit temporary key ownership, transferable without a free alias gap."""
    def __init__(self, budget):
        self.budget, self.bytes = budget, 0

    def __enter__(self):
        return self

    def claim(self, *values):
        for value in values:
            self.budget.key(value)
            self.bytes += utf8_size(value)

    def transfer(self, other, size):
        if other.budget is not self.budget or not 0 <= size <= self.bytes:
            raise AssertionError("key ownership")
        self.bytes -= size
        other.bytes += size

    def __exit__(self, *_args):
        self.budget.key_bytes -= self.bytes
        self.bytes = 0


@dataclass
class Node:
    document: str
    pointer: str
    role: str
    value: object
    number: int
    parent_schema: int | None = None
    dialect: int = 0  # 0=Draft202012, 1=OAS31; no retained dialect-URI alias.
    scope: object = None
    uri: str = ""

    @property
    def identity(self):
        return self.number
