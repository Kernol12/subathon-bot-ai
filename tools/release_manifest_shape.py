"""Structural validation and canonical JSON for release manifest assembly."""

import json
import math
import re


PROFILES = frozenset((
    "source", "server-native", "server-oci", "desktop-win-zip",
    "desktop-linux-archive", "desktop-arch-recipe", "web-management",
    "web-viewer", "web-overlay", "scene-portable",
))
FONT_ROLES = frozenset(("regular", "bold"))
CARRIED = frozenset(("carried", "embedded"))


def scalar(value):
    return type(value) is str and not re.search(r"[\ud800-\udfff]", value)


def token(value):
    return scalar(value) and bool(value) and not re.search(r"[\x00-\x1f\x7f-\x9f]", value)


def path(value):
    return (token(value) and not re.search(r"[:\\]", value)
            and all(part not in ("", ".", "..") for part in value.split("/")))


def decimal(value):
    return type(value) is str and re.fullmatch(r"0|[1-9][0-9]*", value) is not None


def digest(value):
    return type(value) is str and re.fullmatch(r"[0-9a-f]{64}", value) is not None


def choice(values):
    return lambda value: type(value) is str and value in values


def sequence(check):
    def valid(values):
        return (type(values) is list and all(check(value) for value in values)
                and len(set(values)) == len(values))
    return valid


def record(value, required, optional=None):
    optional = optional or {}
    return (mapping(value) and required.keys() <= value.keys()
            and all((required.get(key) or optional.get(key, _reject))(field)
                    for key, field in value.items()))


def mapping(value):
    return type(value) is dict and all(scalar(key) for key in value)


def _reject(_):
    return False


def _json_data(root):
    # Evidence can carry opaque acquisition metadata. Check its data shape
    # without recursing into arbitrarily deep or cyclic Python objects.
    active = set()
    checked = set()
    pending = [(iter((root,)), None)]
    while pending:
        items, owner = pending[-1]
        value = next(items, _END)
        if value is _END:
            pending.pop()
            active.discard(owner)
            checked.add(owner)
        elif type(value) in (dict, list):
            if id(value) in active:
                return False
            if id(value) in checked:
                continue
            if type(value) is dict and not all(scalar(key) for key in value):
                return False
            active.add(id(value))
            pending.append((iter(value.values() if type(value) is dict else value), id(value)))
        elif not (value is None or type(value) in (bool, int) or scalar(value)
                  or (type(value) is float and math.isfinite(value))):
            return False
    return True


_END = object()


def subject(value):
    if not token(value):
        return False
    if value == "artifact":
        return True
    kind, _, name = value.partition(":")
    if kind == "member":
        return path(name)
    if kind == "font":
        return name in FONT_ROLES
    return kind in ("component", "asset") and token(name)


TOKENS = sequence(token)
PATHS = sequence(path)
IDENTITY = {"bytes": decimal, "sha256": digest}
ARTIFACT = {**IDENTITY, "id": token, "format": token, "inventory_sha256": digest}
MEMBER = {
    **IDENTITY, "path": path, "kind": choice(("source", "payload", "notice", "recipe", "alias")),
    "components": TOKENS,
}
COMPOSITION = {"path": path, "components": TOKENS}
SELECTION = {
    "label": scalar, "platform": token, "profile": choice(PROFILES),
    "recipe": token, "revision": token, "scope": token,
}
COMPONENT = {
    "id": token, "name": scalar, "revision": token, "role": choice(CARRIED | {"external", "build-only"}),
    "members": PATHS, "dependencies": TOKENS, "rights": TOKENS, "notices": TOKENS, "sources": TOKENS,
}
RIGHT = {
    "id": token, "component": token, "scope": token,
    "state": choice(("permitted", "prohibited", "unknown")), "conditions": TOKENS, "review": token,
}
NOTICE = {"id": token, "component": token, "license": token, "paths": PATHS, "review": token}
SOURCE = {
    "id": token, "component": token, "mode": choice(("included", "companion", "approved-not-required")),
    "revision": token, "recipe": token, "patches": TOKENS, "paths": PATHS, "prerequisites": TOKENS,
    "review": token,
}
ASSET = {"id": token, "component": token, "path": path, "revision": token, "right": token}
FONT = {
    "id": token, "asset": token, "face": token, "weight": decimal,
    "role": choice(FONT_ROLES | {"custom", "fallback"}), "origin": token,
}
EVIDENCE = {
    "id": token, "kind": token, "owner": token, "revision": token, "subject": subject,
    "scope": token, "facts": lambda value: type(value) is dict and _json_data(value),
}
PIN = {"subject": subject, "role": token, "evidence": token}
OBSERVED = {
    "scope": token, "artifact": lambda value: record(value, ARTIFACT),
    "review": token, "members": lambda value: type(value) is list,
    "composition": lambda value: type(value) is list,
}
COLLECTIONS = ("selected_pins", "evidence", "components", "rights", "notices", "sources", "assets", "fonts")


def member(value):
    return (record(value, MEMBER, {"alias_target": path})
            and (value["kind"] == "alias") == ("alias_target" in value))


def keyed_rows(rows, key, check):
    if type(rows) is not list:
        return False
    seen = set()
    for row in rows:
        if not check(row) or row[key] in seen:
            return False
        seen.add(row[key])
    return True


def identity(value):
    return record(value, IDENTITY)


def _identity_map(value, key=token):
    return type(value) is dict and all(key(name) and identity(item) for name, item in value.items())


def _source_material(value):
    # An exact source archive, repository receipt, or closure keyed by input.
    return mapping(value) and bool(value) and ("bytes" in value) == ("sha256" in value) and all(
        (digest(item) if key == "sha256" else
         decimal(item) if key == "bytes" else
         TOKENS(item) if key == "patches" else token(item) or identity(item))
        for key, item in value.items()
    )


def _bool(value):
    return type(value) is bool


def _nullable_token(value):
    return value is None or token(value)


def _artifact(value):
    return record(value, ARTIFACT)


def _members(value):
    return keyed_rows(value, "path", member)


def _composition(value):
    return keyed_rows(value, "path", lambda row: record(row, COMPOSITION))


REVIEW_BINDING = {"artifact": _artifact, "covered": _members, "build": _nullable_token}
# Missing decision facts remain unqualified until the corresponding phase.
# Known facts of the wrong type are shape failures even in a later domain.
FACT_FIELDS = {
    "observed-profile": {
        "artifact": _artifact, "profile": choice(PROFILES), "complete": _bool,
        "composition": _composition, "required_font_roles": sequence(choice(FONT_ROLES)),
        "inputs": TOKENS, "revision": token, "recipe": token, "platform": token,
        "built_in": _bool,
    },
    "selected-identity": {
        **IDENTITY, "build": token, "output": identity, "files": _identity_map,
        "archives": _identity_map, "dependencies": TOKENS, "path": path,
        "artifact": _artifact, "source": _source_material,
    },
    "acquisition": IDENTITY,
    "observed-build": {
        "recipe": token, "output": identity, "outputs": _identity_map,
        "static": lambda value: type(value) is dict and all(
            token(name) and _identity_map(archives) for name, archives in value.items()),
        "sdk_identity": identity, "derived_binding": identity, "dependencies": TOKENS,
    },
    "observed-build-recipe": {
        "patches": TOKENS, "project_inputs": _identity_map, "first_party": _identity_map,
    },
    "generation-recipe": {
        "catalog": identity, "generator": identity, "catalog_path": path, "generator_path": path,
        "outputs": _identity_map,
    },
    "rights-review": {**REVIEW_BINDING, **IDENTITY, "path": path, "state": RIGHT["state"],
                      "conditions": TOKENS, "required_conditions": TOKENS},
    "rights-condition": {"satisfied": _bool, "condition": token, "artifact": _artifact},
    "notice-review": {**REVIEW_BINDING, "license": token, "paths": PATHS, "texts": _members,
                      "required_texts": lambda value: _identity_map(value, path),
                      "reviewed_delivery_paths": PATHS, "summary": token},
    "reviewed-notice-summary": {"artifact": _artifact, "build": token, "basis": token,
                                "applicable": _bool, "integration": choice(("embedded", "not-integrated"))},
    "source-review": {**REVIEW_BINDING, "mode": SOURCE["mode"], "recipe": token, "patches": TOKENS,
                      "paths": PATHS, "prerequisites": TOKENS, "access": _nullable_token, "basis": token},
    "modeled-source-access": {
        "tree": token, "recipe": token, "paths": PATHS,
        "equivalent_access": _bool, "further_charge": _bool, "sources": _source_material,
    },
    "patch": {**IDENTITY, "path": path},
    # The face and origin independently bind the delivered font. The face
    # captures renderer admission, while the origin captures transformation
    # and the notice decisions that apply to the delivered bytes.
    "font-face": {**IDENTITY, "path": path, "role": FONT["role"], "weight": decimal,
                  "face": token, "renderer": token, "admitted": _bool, "origin": token},
    "font-origin": {**IDENTITY, "path": path, "source": identity, "transformation": token,
                    "family": token, "license": token, "notices": TOKENS, "name_conditions_met": _bool},
}


def _facts_valid(row):
    facts = row["facts"]
    return all(check(facts[key]) for key, check in FACT_FIELDS.get(row["kind"], {}).items()
               if key in facts)


def _row_subject(name, row):
    if not mapping(row):
        return "artifact"
    if name in ("evidence", "selected_pins"):
        value = row.get("subject")
        return value if subject(value) else "artifact"
    if name in ("members", "composition"):
        value = row.get("path")
        return f"member:{value}" if path(value) else "artifact"
    if name == "fonts":
        role = row.get("role")
        return f"font:{role}" if choice(FONT_ROLES)(role) else "artifact"
    if name == "assets":
        value = row.get("id")
        return f"asset:{value}" if token(value) else "artifact"
    value = row.get("id" if name == "components" else "component")
    return f"component:{value}" if token(value) else "artifact"


def _index(name, rows, check, invalid, key=None):
    indexed = {}
    if type(rows) is not list:
        invalid.add("artifact")
        return indexed
    for row in rows:
        location = _row_subject(name, row)
        if not check(row):
            invalid.add(location)
            continue
        identifier = key(row) if key else row["id"]
        if identifier in indexed:
            invalid.update((_row_subject(name, indexed[identifier]), location))
        else:
            indexed[identifier] = row
    return indexed


def shape(request):
    """Return normalized lookup tables and the independently qualified shape defects."""
    invalid = set()
    if not record(request, {key: lambda _: True for key in (*COLLECTIONS, "selection", "artifact_id")},
                  {"expected": _artifact, "observed": lambda row: record(row, OBSERVED)}):
        invalid.add("artifact")
    if not mapping(request):
        return {}, invalid
    if not token(request.get("artifact_id")) or not record(request.get("selection"), SELECTION):
        invalid.add("artifact")

    checks = {
        "selected_pins": (lambda row: record(row, PIN), lambda row: (row["subject"], row["role"])),
        "evidence": (lambda row: record(row, EVIDENCE) and _facts_valid(row), None),
        "components": (lambda row: record(row, COMPONENT, {"build": token}), None),
        "rights": (lambda row: record(row, RIGHT), None),
        "notices": (lambda row: record(row, NOTICE), None),
        "sources": (lambda row: record(row, SOURCE, {"access": token, "build": token}), None),
        "assets": (lambda row: record(row, ASSET), None),
        "fonts": (lambda row: record(row, FONT), None),
    }
    indexes = {name: _index(name, request.get(name, []), check, invalid, key)
               for name, (check, key) in checks.items()}
    observed = request.get("observed")
    indexes["members"] = _index("members", observed.get("members", []) if mapping(observed) else [],
                                member, invalid, lambda row: row["path"])
    indexes["composition"] = _index(
        "composition", observed.get("composition", []) if mapping(observed) else [],
        lambda row: record(row, COMPOSITION), invalid, lambda row: row["path"],
    )
    _references(indexes, observed, invalid)
    return indexes, invalid


def _references(indexes, observed, invalid):
    evidence = indexes["evidence"]
    members = indexes["members"]
    components = indexes["components"]
    if mapping(observed):
        review = observed.get("review")
        if not token(review) or review not in evidence:
            invalid.add("artifact")
    for row in members.values():
        if row["kind"] == "alias":
            target = members.get(row["alias_target"])
            if target is None or target["kind"] == "alias":
                invalid.add(f"member:{row['path']}")

    # These indices establish that references exist. The later phases check
    # which records actually apply to the selected artifact and subjects.
    references = {
        "selected_pins": {"evidence": "evidence"},
        "components": {"rights": "rights", "notices": "notices", "sources": "sources",
                       "dependencies": "components", "build": "evidence"},
        "members": {"components": "components"},
        "composition": {"components": "components"},
        "rights": {"component": "components", "review": "evidence", "conditions": "evidence"},
        "notices": {"component": "components", "review": "evidence"},
        "sources": {"component": "components", "review": "evidence", "build": "evidence",
                    "access": "evidence", "patches": "evidence", "prerequisites": None},
        "assets": {"component": "components", "right": "rights"},
        "fonts": {"asset": "assets", "face": "evidence", "origin": "evidence"},
    }
    for name, fields in references.items():
        for row in indexes[name].values():
            for field, table in fields.items():
                if field not in row:
                    continue
                values = row[field] if type(row[field]) is list else (row[field],)
                missing = (any(value not in indexes[table] for value in values) if table else
                           any((value in evidence) == (value in components) for value in values))
                if missing:
                    invalid.add(_row_subject(name, row))
    fact_references = {
        "observed-profile": ("inputs",),
        "selected-identity": ("build",),
        "observed-build": ("recipe",),
        "observed-build-recipe": ("patches",),
        "notice-review": ("build", "summary"),
        "rights-review": ("build", "conditions"),
        "source-review": ("build", "access", "patches"),
        "reviewed-notice-summary": ("build", "basis"),
        "font-face": ("origin",),
    }
    for row in evidence.values():
        kind, facts = row["kind"], row["facts"]
        for field in fact_references.get(kind, ()):
            values = facts.get(field)
            if values is not None and any(
                    value not in evidence for value in (values if type(values) is list else (values,))):
                invalid.add(row["subject"])
        if kind == "source-review" and any(
                (key in evidence) == (key in components) for key in facts.get("prerequisites", [])):
            invalid.add(row["subject"])
        if kind == "font-origin" and any(key not in indexes["notices"] for key in facts.get("notices", [])):
            invalid.add(row["subject"])


_SHORT_ESCAPES = dict(zip("bfnrt", ("0008", "000c", "000a", "000d", "0009")))


def canonical(value):
    # Valid Unicode scalars sort in the same order by code point and UTF-8 bytes.
    encoded = json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    long_escapes = re.sub(r'\\(["\\bfnrt])',
                          lambda match: (r"\u" + _SHORT_ESCAPES[match[1]]
                                         if match[1] in _SHORT_ESCAPES else match[0]), encoded)
    return (long_escapes + "\n").encode("utf-8")


def ordered_row(row):
    return {key: sorted(value) if type(value) is list and key != "patches" else value
            for key, value in row.items()}


def ordered_rows(rows, key):
    return [ordered_row(row) for row in sorted(rows, key=lambda row: row[key])]
