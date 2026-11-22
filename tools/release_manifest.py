"""Data-only assembly interface for RELEASE-001."""

from dataclasses import dataclass
from hashlib import sha256
from typing import TypedDict

from . import release_manifest_shape as _shape


class Evidence(TypedDict):
    id: str
    kind: str
    owner: str
    revision: str
    subject: str
    scope: str
    facts: dict


class SelectedPin(TypedDict):
    subject: str
    role: str
    evidence: str


class Observation(TypedDict):
    scope: str
    artifact: dict
    members: list[dict]
    review: str
    composition: list[dict]


class ArtifactInputs(TypedDict, total=False):
    expected: dict
    observed: Observation


class ReleaseRequest(ArtifactInputs):
    selection: dict
    artifact_id: str
    selected_pins: list[SelectedPin]
    evidence: list[Evidence]
    components: list[dict]
    rights: list[dict]
    notices: list[dict]
    sources: list[dict]
    assets: list[dict]
    fonts: list[dict]


@dataclass(frozen=True)
class Finding:
    artifact: str
    code: str
    subject: str


@dataclass(frozen=True)
class Complete:
    manifest_bytes: bytes


@dataclass(frozen=True)
class Blocked:
    findings: tuple[Finding, ...]


def assemble(request: ReleaseRequest) -> Complete | Blocked:
    """Assemble one selected scope from owned data records."""
    artifact = request.get("artifact_id") if _shape.mapping(request) else None
    artifact = artifact if _shape.token(artifact) else ""
    tables, invalid = _shape.shape(request)
    if invalid:
        return Blocked(tuple(Finding(artifact, "invalid_input", subject) for subject in sorted(invalid)))

    assembly = _Assembly(request, tables)
    for check in (assembly.inventory, assembly.composition, assembly.rights,
                  assembly.notices, assembly.sources, assembly.fonts):
        findings = check()
        if findings:
            return Blocked(tuple(Finding(artifact, code, subject) for code, subject in sorted(findings)))
    return Complete(assembly.manifest())


def _identity(row):
    return {field: row[field] for field in ("bytes", "sha256") if field in row}


def _matches(fields, **expected):
    return all(key in fields and fields[key] == value for key, value in expected.items())


def _as_set(fields, key, expected):
    return key in fields and set(fields[key]) == set(expected)


def _matches_rows(fields, key, expected, by="path"):
    return key in fields and _shape.ordered_rows(fields[key], by) == _shape.ordered_rows(expected, by)


class _Assembly:
    def __init__(self, request, tables):
        self.request = request
        self.tables = tables
        self.selection = request["selection"]
        self.scope = self.selection["scope"]
        self.observed = request.get("observed", {})
        self.artifact = self.observed.get("artifact")
        self.members = tables["members"]
        self.components = tables["components"]
        self.evidence = tables["evidence"]
        self.pins = tables["selected_pins"]

    def _fact(self, identifier, kind, *subjects, revision=None):
        row = self.evidence.get(identifier)
        if (row is not None and row["scope"] == self.scope and row["kind"] == kind
                and row["subject"] in subjects and (revision is None or row["revision"] == revision)):
            return row["facts"]
        return {}

    def _pin(self, identifier, subject):
        row = self.evidence[identifier]
        if (row["scope"] == self.scope and row["subject"] == subject
                and row["kind"] in ("selected-identity", "acquisition")):
            return row
        return None

    def _profile(self):
        return self.evidence[self.observed["review"]]["facts"]

    def inventory(self):
        if "expected" not in self.request or not self.observed:
            return {("inventory_missing", "artifact")}
        review = self.evidence[self.observed["review"]]
        profile = review["facts"]
        if (review["kind"] != "observed-profile" or review["subject"] != "artifact"
                or profile.get("complete") is not True):
            return {("inventory_missing", "artifact")}

        findings = set()
        if (self.observed["scope"] != self.scope
                or review["scope"] != self.scope or review["revision"] != self.selection["revision"]
                or self.artifact["id"] != self.request["artifact_id"]
                or self.artifact != self.request["expected"]
                or not _matches(profile, artifact=self.artifact, profile=self.selection["profile"])
                or profile.get("recipe", self.artifact["format"]) != self.selection["recipe"]
                or any(profile.get(key, self.selection[key]) != self.selection[key]
                       for key in ("revision", "platform"))
                or sha256(_shape.canonical(_shape.ordered_rows(self.members.values(), "path"))).hexdigest()
                != self.artifact["inventory_sha256"]):
            findings.add(("identity_conflict", "artifact"))

        for (subject, role), pin in self.pins.items():
            selected = self._pin(pin["evidence"], subject)
            if selected is None or not self._selected_identity(subject, role, selected):
                findings.add(("identity_conflict", subject))
        return findings

    def _selected_identity(self, subject, role, selected):
        facts = selected["facts"]
        kind, _, name = subject.partition(":")
        if selected["kind"] == "acquisition" and kind != "member":
            return False
        if kind == "member":
            actual = self.members.get(name)
            return (actual is not None and _shape.identity(_identity(facts))
                    and _identity(actual) == _identity(facts)
                    and facts.get("path", name) == name)
        if kind == "artifact":
            if "artifact" in facts:
                return facts["artifact"] == self.artifact
            return _shape.identity(_identity(facts)) and _identity(facts) == _identity(self.artifact)
        if kind != "component" or name not in self.components:
            return False
        component = self.components[name]
        if selected["revision"] != component["revision"]:
            return False
        if role != "build":
            if role in ("external", "build-only"):
                return component["role"] == role
            # A selected component source pin binds its declared revision.
            return role == "source" and not {"build", "output", "archives"} & facts.keys()
        if not component.get("build") or facts.get("build") != component["build"]:
            return False
        build = self._build(component)
        if not build:
            return False
        if "output" in facts and facts["output"] not in self._outputs(component, build).values():
            return False
        if "archives" in facts and (
                not facts["archives"] or build.get("static", {}).get(name) != facts["archives"]):
            return False
        return "output" in facts or "archives" in facts

    def _build(self, component):
        return self._fact(component.get("build"), "observed-build",
                          f"component:{component['id']}", "artifact")

    def _covered(self, component):
        return [self.members[path] for path in component["members"] if path in self.members]

    def _outputs(self, component, build):
        # An aggregate receipt can key outputs by component or by delivered path.
        # A singular output may be shared by all static constituents of its carrier.
        outputs = build.get("outputs", {})
        common = (outputs.get(component["id"]), build.get("output"), build.get("derived_binding"))
        return {row["path"]: identity for row in self._covered(component)
                if row["kind"] != "alias"
                and (identity := _identity(row)) in (*common, outputs.get(row["path"]))}

    def composition(self):
        findings = set()
        profile = self._profile()
        if "composition" not in profile:
            findings.add(("unmapped_content", "artifact"))
        declared = {row["path"]: set(row["components"]) for row in profile.get("composition", [])}
        observed = {path: set(row["components"]) for path, row in self.tables["composition"].items()}
        mapped = {name: set() for name in self.components}

        for path in self.members.keys() | observed.keys() | declared.keys():
            owners = set(self.members.get(path, {}).get("components", []))
            if not owners or owners != observed.get(path) or owners != declared.get(path):
                findings.add(("unmapped_content", f"member:{path}"))
            for owner in owners:
                if owner in mapped:
                    mapped[owner].add(path)
                else:
                    findings.add(("unmapped_content", f"member:{path}"))

        inputs = [self.evidence[key] for key in profile.get("inputs", [])]
        if any(row["scope"] != self.scope for row in inputs):
            findings.add(("unmapped_content", "artifact"))
        builds = {row["id"] for row in inputs if row["kind"] == "observed-build" and row["scope"] == self.scope}
        asset_paths = {}
        for asset in self.tables["assets"].values():
            asset_paths.setdefault(asset["component"], set()).add(asset["path"])
        for name, component in self.components.items():
            carried = component["role"] in _shape.CARRIED
            if (set(component["members"]) != mapped[name] or carried != bool(mapped[name])
                    or not self._resolved(component, asset_paths.get(name, set()))):
                findings.add(("unmapped_content", f"component:{name}"))
                continue
            if component.get("build"):
                builds.add(component["build"])

        for build in builds:
            findings.update(self._build_closure(build))
        for name, asset in self.tables["assets"].items():
            if asset["path"] not in mapped[asset["component"]]:
                findings.add(("unmapped_content", f"asset:{name}"))
        return findings

    def _resolved(self, component, asset_paths):
        name = component["id"]
        if component["role"] not in _shape.CARRIED:
            pin = self.pins.get((f"component:{name}", component["role"]))
            selected = self._pin(pin["evidence"], pin["subject"]) if pin else None
            if selected is None:
                return False
            facts = selected["facts"]
            return (bool(facts.get("files") or _shape.identity(_identity(facts)))
                    and ("dependencies" not in facts
                         or _as_set(facts, "dependencies", component["dependencies"])))

        payloads = {row["path"] for row in self._covered(component) if row["kind"] == "payload"}
        build = self._build(component)
        if "build" not in component:
            return payloads <= asset_paths and component["role"] == "carried"
        if not build or not build.get("recipe"):
            return False
        static = build.get("static", {})
        outputs = self._outputs(component, build)
        if component["role"] == "embedded":
            return bool(static.get(name) and outputs) and payloads <= outputs.keys() | asset_paths
        if name in static:
            return False
        return payloads <= outputs.keys() | asset_paths

    def _build_closure(self, identifier):
        facts = self.evidence[identifier]["facts"]
        owners = {name: row for name, row in self.components.items() if row.get("build") == identifier}
        findings = set()
        outputs = [identity for component in owners.values() for identity in self._outputs(component, facts).values()]
        if any(facts[key] not in outputs for key in ("output", "derived_binding") if key in facts):
            findings.add(("unmapped_content", "artifact"))
        for name in facts.get("static", {}):
            if name not in owners or owners[name]["role"] != "embedded":
                findings.add(("unmapped_content", f"component:{name}"))
        for target, identity in facts.get("outputs", {}).items():
            if target in owners:
                if identity not in [_identity(row) for row in self._covered(owners[target])]:
                    findings.add(("unmapped_content", f"component:{target}"))
            elif (not _shape.path(target) or target not in self.members
                  or _identity(self.members[target]) != identity
                  or not owners.keys() & set(self.members[target]["components"])):
                subject = f"member:{target}" if _shape.path(target) else "artifact"
                findings.add(("unmapped_content", subject))
        return findings

    def _review(self, row, kind, component, *, revision=None):
        facts = self._fact(row["review"], kind, f"component:{component['id']}",
                           revision=component["revision"] if revision is None else revision)
        if (_matches(facts, artifact=self.artifact)
                and _matches_rows(facts, "covered", self._covered(component))
                and facts.get("build") == component.get("build")):
            return facts
        return {}

    def rights(self):
        findings = set()
        rights = self.tables["rights"]
        linked = set()
        by_subject = {}
        for row in rights.values():
            subject = self.evidence[row["review"]]["subject"]
            by_subject.setdefault(subject, {})[row["id"]] = row
        for name, component in self.components.items():
            if component["role"] in _shape.CARRIED:
                subject = f"component:{name}"
                linked.update(component["rights"])
                candidates = {key: rights[key] for key in component["rights"]}
                candidates.update(by_subject.get(subject, {}))
                findings.update(self._rights_for(candidates.values(), component["rights"], subject, component))
        for name, asset in self.tables["assets"].items():
            subject = f"asset:{name}"
            linked.add(asset["right"])
            candidates = {asset["right"]: rights[asset["right"]], **by_subject.get(subject, {})}
            findings.update(self._rights_for(candidates.values(), [asset["right"]], subject,
                                             self.components[asset["component"]], asset))
        prohibited = {subject for code, subject in findings if code == "redistribution_prohibited"}
        for key in rights.keys() - linked:
            row = rights[key]
            subject = self.evidence[row["review"]]["subject"]
            if not subject.startswith(("component:", "asset:")):
                subject = f"component:{row['component']}"
            if subject not in prohibited:
                findings.add(("rights_unknown", subject))
        return findings

    def _rights_for(self, rows, linked, subject, component, asset=None):
        applicable = []
        for row in rows:
            if row["component"] != component["id"] or row["scope"] != self.scope:
                continue
            if asset is None:
                facts = self._review(row, "rights-review", component)
            else:
                facts = self._fact(row["review"], "rights-review", subject, revision=asset["revision"])
                if not _matches(facts, artifact=self.artifact, path=asset["path"],
                                **_identity(self.members[asset["path"]])):
                    facts = {}
            if _matches(facts, state="prohibited"):
                return {("redistribution_prohibited", subject)}
            applicable.append((row, facts))

        if len(applicable) == 1:
            row, facts = applicable[0]
            if (row["id"] in linked and _matches(facts, state="permitted") and row["state"] == "permitted"
                    and _as_set(facts, "conditions", row["conditions"])
                    and self._conditions(row, facts, subject)):
                return set()
        return {("rights_unknown", subject)}

    def _conditions(self, row, facts, subject):
        conditions = set()
        for key in row["conditions"]:
            condition = self._fact(key, "rights-condition", subject)
            if (condition.get("satisfied") is not True or "condition" not in condition
                    or condition.get("artifact", self.artifact) != self.artifact):
                return False
            conditions.add(condition["condition"])
        return "required_conditions" not in facts or _as_set(facts, "required_conditions", conditions)

    def notices(self):
        findings = set()
        linked = set()
        for name, component in self.components.items():
            if component["role"] not in _shape.CARRIED:
                continue
            subject = f"component:{name}"
            if not component["notices"]:
                findings.add(("notice_missing", subject))
            for key in component["notices"]:
                linked.add(key)
                row = self.tables["notices"][key]
                status = self._notice(row, component)
                findings.update((code, subject) for code in status)
        for key in self.tables["notices"].keys() - linked:
            findings.add(("notice_missing", f"component:{self.tables['notices'][key]['component']}"))
        return findings

    def _notice(self, row, component):
        subject = f"component:{component['id']}"
        facts = self._review(row, "notice-review", component)
        if row["component"] != component["id"] or not facts:
            return {"notice_missing"}
        if not _matches(facts, license=row["license"]):
            return {"notice_conflict"} if "license" in facts else {"notice_missing"}

        status = set()
        reviewed = {text["path"]: text for text in facts.get("texts", [])}
        required = set(facts.get("paths", [])) | facts.get("required_texts", {}).keys()
        delivered = set(facts.get("reviewed_delivery_paths", row["paths"]))
        if not required or not required <= set(row["paths"]) or not _as_set(facts, "paths", row["paths"]):
            status.add("notice_missing")
        for path in required | set(row["paths"]):
            member = self.members.get(path)
            text = reviewed.get(path)
            if member is None or text is None or path not in delivered or member["kind"] != "notice":
                status.add("notice_missing")
            elif (_shape.ordered_row(member) != _shape.ordered_row(text)
                  or facts.get("required_texts", {}).get(path, _identity(text)) != _identity(member)):
                status.add("notice_conflict")

        if "summary" in facts:
            summary = self._fact(facts["summary"], "reviewed-notice-summary", subject)
            build = self._build(component)
            if not _matches(summary, artifact=self.artifact, build=component.get("build"),
                            basis=build.get("recipe")) or "applicable" not in summary:
                status.add("notice_missing")
            elif summary["applicable"]:
                if "integration" not in summary:
                    status.add("notice_missing")
                elif (summary["integration"] != "embedded" or component["role"] != "embedded"
                      or not build.get("static", {}).get(component["id"])):
                    status.add("notice_conflict")
        return status

    def sources(self):
        findings = set()
        linked = set()
        for name, component in self.components.items():
            if component["role"] not in _shape.CARRIED:
                continue
            linked.update(component["sources"])
            if (not component["sources"] or any(
                    not self._source(self.tables["sources"][key], component) for key in component["sources"])):
                findings.add(("source_missing", f"component:{name}"))
        for key in self.tables["sources"].keys() - linked:
            findings.add(("source_missing", f"component:{self.tables['sources'][key]['component']}"))
        return findings

    def _source(self, row, component):
        subject = f"component:{component['id']}"
        facts = self._review(row, "source-review", component, revision=row["revision"])
        recipe_id = row["recipe"]
        build = self._build(component)
        if (row["component"] != component["id"]
                or not _matches(facts, mode=row["mode"], recipe=recipe_id, patches=row["patches"])
                or not _as_set(facts, "paths", row["paths"])
                or not _as_set(facts, "prerequisites", row["prerequisites"])
                or row.get("access") != facts.get("access") or row.get("build") != component.get("build")
                or (build and build.get("recipe") != recipe_id)):
            return False
        if any(path not in self.members or self.members[path]["kind"] in ("payload", "alias")
               for path in row["paths"]):
            return False
        if any(key in self.evidence and self.evidence[key]["scope"] != self.scope
               for key in row["prerequisites"]):
            return False

        recipe = (self._fact(recipe_id, "observed-build-recipe", subject, "artifact")
                  or self._fact(recipe_id, "generation-recipe", subject, "artifact"))
        if build and not recipe:
            return False
        if not build and not recipe and recipe_id != self.selection["recipe"]:
            return False
        if row["mode"] == "approved-not-required":
            return (bool(facts.get("basis")) and not row["paths"]
                    and not row["patches"] and "access" not in row)

        access = {}
        if row["mode"] == "included":
            if not row["paths"] or "access" in row:
                return False
            if not self._included_inputs(row, component, recipe):
                return False
        else:
            access = self._fact(row.get("access"), "modeled-source-access", subject, revision=row["revision"])
            if (not _matches(access, tree=row["revision"], recipe=recipe_id,
                             equivalent_access=True, further_charge=False)
                    or not (access.get("sources") or access.get("paths"))):
                return False
            inputs = recipe.get("project_inputs")
            if (inputs and component["revision"] == self.selection["revision"]
                    and access.get("sources") != inputs):
                return False
            if "catalog_path" in recipe or "generator_path" in recipe:
                if not {recipe.get("catalog_path"), recipe.get("generator_path")} <= set(access.get("paths", [])):
                    return False
            build_pin = self.pins.get((subject, "build"))
            if build_pin:
                selected = self.evidence[build_pin["evidence"]]["facts"]
                if "source" in selected and selected["source"] != access.get("sources"):
                    return False
        return self._patches(row, recipe, access, subject)

    def _included_inputs(self, row, component, recipe):
        if component["revision"] != self.selection["revision"]:
            return True
        included = {path: _identity(self.members[path]) for path in row["paths"]}
        if any(included.get(path) != identity for path, identity in recipe.get("project_inputs", {}).items()):
            return False
        # A shared build recipe's first-party inputs apply to the component
        # that actually conveys that source projection.
        first_party = list(recipe.get("first_party", {}).values())
        covered = [_identity(member) for member in self._covered(component)]
        return (not any(identity in covered for identity in first_party)
                or all(identity in included.values() for identity in first_party))

    def _patches(self, row, recipe, access, subject):
        patches = row["patches"]
        selected = recipe.get("patches", [])
        access_sources = access.get("sources")
        scoped = access_sources.get("patches", selected) if type(access_sources) is dict else selected
        if patches != scoped or [patch for patch in selected if patch in patches] != patches:
            return False
        for key in patches:
            patch = self._fact(key, "patch", subject, "artifact")
            member = self.members.get(patch.get("path"))
            if not _shape.identity(_identity(patch)):
                return False
            if member is not None and _identity(patch) != _identity(member):
                return False
            if row["mode"] == "included" and (member is None or member["path"] not in row["paths"]):
                return False
        return True

    def fonts(self):
        findings = set()
        profile = self._profile()
        required = set(profile.get("required_font_roles", []))
        name = self.selection["profile"]
        graphical = name.startswith(("desktop-", "web-")) or name == "scene-portable"
        if (required or profile.get("built_in") is True
                or (graphical and "required_font_roles" not in profile and "built_in" not in profile)):
            required.update(_shape.FONT_ROLES)
        qualified = set()
        for font in self.tables["fonts"].values():
            role = font["role"]
            if self._font(font):
                qualified.add(role)
            else:
                subject = f"font:{role}" if role in _shape.FONT_ROLES else f"asset:{font['asset']}"
                findings.add(("font_unverified", subject))
        findings.update(("font_unverified", f"font:{role}") for role in required - qualified)
        return findings

    def _font(self, font):
        asset = self.tables["assets"][font["asset"]]
        member = self.members[asset["path"]]
        role, subject = font["role"], f"asset:{asset['id']}"
        face = self._fact(font["face"], "font-face", subject, revision=asset["revision"])
        origin = self._fact(font["origin"], "font-origin", subject, revision=asset["revision"])
        bindings = {"path": member["path"], **_identity(member)}
        if (member["kind"] != "payload" or not 1 <= len(font["weight"]) <= 4
                or not 1 <= int(font["weight"]) <= 1000
                or not _matches(face, **bindings, role=role, weight=font["weight"],
                         origin=font["origin"], admitted=True)
                or not (face.get("face") and face.get("renderer"))
                or not _matches(origin, **bindings, name_conditions_met=True)
                or not (origin.get("source") and origin.get("transformation"))):
            return False
        if role in _shape.FONT_ROLES:
            weight = {"regular": "400", "bold": "700"}[role]
            if font["weight"] != weight or not _matches(origin, family="Noto Sans", license="OFL-1.1"):
                return False
        component = self.components[asset["component"]]
        notices = [self.tables["notices"][key] for key in component["notices"]]
        return (bool(origin.get("notices"))
                and set(origin["notices"]) <= {row["id"] for row in notices if row["license"] == origin.get("license")})

    def manifest(self):
        return _shape.canonical({
            "artifact": self.artifact,
            "selection": self.selection,
            "version": "release-manifest/v1",
            **{collection: _shape.ordered_rows(self.tables[collection].values(), key)
               for collection, key in (("assets", "id"), ("components", "id"), ("fonts", "id"),
                                       ("members", "path"), ("notices", "id"), ("rights", "id"),
                                       ("sources", "id"))},
        })
