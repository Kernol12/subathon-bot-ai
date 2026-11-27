"""Qualify one native profile from selected, observed and repeat build data."""

from copy import deepcopy
from dataclasses import dataclass
from hashlib import sha256
import re
from urllib.parse import urlsplit

from tools import release_manifest_shape as manifest


@dataclass(frozen=True)
class Qualified:
    release_inputs: dict
    build_runs: tuple[dict, dict]


@dataclass(frozen=True)
class InputDifference:
    subject: str
    field: str
    selected: object
    observed: object


@dataclass(frozen=True)
class Unqualified:
    findings: tuple[tuple[str, str], ...]
    differences: tuple[InputDifference, ...] = ()


_PLATFORMS = {
    "server-native": r"ubuntu-[0-9]+\.[0-9]+-x86_64",
    "desktop-linux-archive": r"ubuntu-[0-9]+\.[0-9]+-x86_64",
    "desktop-arch-recipe": r"arch-linux-x86_64",
    "desktop-win-zip": r"windows-11-x86_64",
}
_TOOLS = {"compiler", "linker", "cmake", "ninja"}
_ROLES = manifest.CARRIED | {"external", "build-only"}
_BUILD_FIELDS = ("profile", "revision", "project_material", "scope", "platform", "recipe",
                 "recipe_material", "presets", "environment")
_QT_DYNAMIC = {"qt-platform", "qt-qml-controls"}
_LICENSE_UNKNOWN = {"value": None, "source": None}


def _matches(pattern, value):
    return type(value) is str and re.fullmatch(pattern, value) is not None


def _exact_revision(value):
    return _matches(r"(?:v?[0-9]+(?:\.[0-9]+)+(?:[-+][0-9A-Za-z.]+)*|[0-9a-f]{40}|[0-9a-f]{64})", value)


def _token_sequence(value):
    return type(value) in (tuple, list) and bool(value) and all(manifest.token(item) for item in value)


def _identity_map(value):
    return manifest.mapping(value) and all(
        manifest.path(key) and manifest.identity(item) for key, item in value.items()
    )


def _material_locator(value):
    if not manifest.token(value) or any(char.isspace() for char in value):
        return False
    try:
        parsed = urlsplit(value)
        return (parsed.scheme in {"http", "https"} and bool(parsed.hostname)
                and parsed.path not in ("", "/") and parsed.username is None
                and parsed.password is None)
    except ValueError:
        return False


def _input_pin(name, item, windows):
    if not manifest.mapping(item):
        return False
    origin = [field for field in ("source", "vendor") if field in item]
    role = item.get("role")
    archives = item.get("archives")
    license_info = item.get("license", _LICENSE_UNKNOWN)
    return (
        _exact_revision(item.get("revision"))
        and manifest.identity(item.get("material"))
        and len(origin) == 1 and _material_locator(item[origin[0]])
        and ("patches" not in item or origin == ["source"] and _identity_map(item["patches"]))
        and (role != "embedded" or (_identity_map(archives) and bool(archives)))
        and (role == "embedded" or "archives" not in item)
        and manifest.record(license_info, {"value": lambda value: value is None or manifest.token(value),
                                           "source": lambda value: value is None or manifest.token(value)})
        and (license_info["value"] is None or license_info["source"] is not None)
        and manifest.choice(_ROLES)(role)
        and (name not in _TOOLS or role == "build-only")
        and (not windows or name not in {"compiler", "linker", "msvc-runtime"}
             or origin == ["vendor"] and _matches(r"14\.[34][0-9](?:\.[0-9]+)+", item["revision"]))
    )


def _run(row):
    return (
        manifest.token(row.get("run")) and _token_sequence(row.get("command"))
        and type(row.get("exit_status")) is int and row["exit_status"] == 0
        and manifest.identity(row.get("output"))
    )


def _token_map(value):
    return manifest.mapping(value) and all(
        manifest.token(name) and manifest.token(owner) for name, owner in value.items()
    )


def _member_layout(selected, findings):
    """Assign owners using declared paths or the compact profile layout."""
    inputs = selected["inputs"]
    runtime = selected["runtime"]
    members = selected["members"]
    embedded = {name for name, row in inputs.items() if row["role"] == "embedded"}
    carried = {name: owner for name, owner in runtime.items() if inputs[owner]["role"] == "carried"}
    for name in (embedded | {"qt"}).difference(runtime.values()):
        findings.add(("runtime_missing", name))
    for name, row in inputs.items():
        if row["role"] == "carried" and name not in carried.values():
            findings.add(("runtime_missing", name))

    declared = selected.get("member_paths")
    if declared is None:
        declared = _compact_paths(members, inputs, carried)
    if not manifest.record(declared, {
        "executable": manifest.path,
        "runtime": lambda value: manifest.mapping(value) and all(map(manifest.path, value.values())),
        "notices": _token_map,
    }):
        findings.add(("member_layout", "artifact"))
        return {}, {}

    executable = declared["executable"]
    if executable.endswith(".exe") != (selected["profile"] == "desktop-win-zip"):
        findings.add(("member_layout", executable))
    layout = {executable: ("payload", sorted({"project", *embedded}))}
    for name in carried.keys() | declared["runtime"].keys():
        path = declared["runtime"].get(name)
        if name not in carried or path is None or path in layout:
            findings.add(("member_layout", name))
        else:
            layout[path] = ("payload", [carried[name]])
    for path, owner in declared["notices"].items():
        if path in layout or (owner not in inputs and owner != "project"):
            findings.add(("member_layout", path))
        else:
            # A notice for a distro dependency is a project-delivered file.
            carried_owner = owner if inputs.get(owner, {}).get("role") in manifest.CARRIED else "project"
            layout[path] = ("notice", [carried_owner])
    for path in layout.keys() ^ members.keys():
        findings.add(("member_layout", path))
    return layout, declared["notices"]


def _compact_paths(members, inputs, carried):
    bins = [path for path in members if path.startswith("bin/")]
    runtime = {}
    notices = {}
    for path in sorted(members):
        directory, _, filename = path.rpartition("/")
        if directory == "notices" and filename.endswith(".txt"):
            matches = [name for name in inputs if name.casefold() == filename[:-4].casefold()]
            if len(matches) == 1:
                notices[path] = matches[0]
        elif directory == "runtime":
            matches = {filename, filename.removesuffix(".dll")} & carried.keys()
            if len(matches) == 1:
                runtime[matches.pop()] = path
    return dict(executable=bins[0] if len(bins) == 1 else None, runtime=runtime, notices=notices)


def qualify_native_build(selected, observed, repeated):
    """Return RELEASE-001 technical rows or findings for the selected profile.

    The three records bind one recipe, input set and member inventory. Recipes
    can declare explicit member paths (executable, runtime names to paths, and
    notice paths to owners) and declare runtime names requiring load inspection.
    Embedded inputs require inspected static archive identities. A separate
    final_artifact identity describes post-processing after the compared output.
    The recipe label and release input locators are descriptions of sealed
    materials. This function does not resolve locators or execute commands.
    """
    if not all(manifest.mapping(row) for row in (selected, observed, repeated)):
        return Unqualified((("invalid_input", "artifact"),))
    profile = selected.get("profile")
    if not manifest.choice(_PLATFORMS)(profile):
        return Unqualified((("invalid_input", "profile"),))
    findings = set()
    differences = []

    def compare(code, subject, field, expected, actual):
        if expected != actual:
            findings.add((code, subject))
            differences.append(InputDifference(subject, field, expected, actual))

    if not _matches(_PLATFORMS[profile], selected.get("platform")):
        findings.add(("platform_pin", profile))
    if (not _matches(r"(?:[0-9a-f]{40}|[0-9a-f]{64})", selected.get("revision"))
            or not manifest.identity(selected.get("project_material"))):
        findings.add(("project_pin", "revision"))
    if not manifest.token(selected.get("recipe")) or not manifest.identity(selected.get("recipe_material")):
        findings.add(("recipe_pin", "recipe"))
    if not manifest.token(selected.get("scope")):
        findings.add(("invalid_input", "scope"))
    presets = selected.get("presets")
    if not _token_sequence(presets) or len(presets) != 3:
        findings.add(("invalid_input", "presets"))
    if not manifest.record(selected.get("environment"), {"name": manifest.token, "material": manifest.identity}):
        findings.add(("environment_pin", "environment"))

    for field in _BUILD_FIELDS:
        code = "profile_mismatch" if field in ("profile", "platform") else "selected_build_mismatch"
        compare(code, profile if code == "profile_mismatch" else field, field,
                selected.get(field), observed.get(field))
        compare("rebuild_input_mismatch", field, field, observed.get(field), repeated.get(field))
    for field in ("member_paths", "settings", "dynamic_runtime"):
        compare("selected_build_mismatch", field, field, selected.get(field), observed.get(field))
        compare("rebuild_input_mismatch", field, field, observed.get(field), repeated.get(field))
    if "settings" in selected and not _token_map(selected["settings"]):
        findings.add(("invalid_input", "settings"))
    if repeated.get("profile") != profile or repeated.get("platform") != selected.get("platform"):
        findings.add(("profile_mismatch", profile))
    if not _run(observed):
        findings.add(("build_missing", profile))
    if not _run(repeated) or observed.get("run") == repeated.get("run"):
        findings.add(("rebuild_missing", profile))
    compare("rebuild_mismatch", profile, "output", observed.get("output"), repeated.get("output"))
    if "final_artifact" in selected or "final_artifact" in observed:
        final_artifact = selected.get("final_artifact")
        if not manifest.identity(final_artifact):
            findings.add(("artifact_pin", profile))
        compare("member_mismatch", profile, "final_artifact", final_artifact, observed.get("final_artifact"))

    inputs = selected.get("inputs")
    actual = observed.get("inputs")
    if not all(manifest.mapping(value) for value in (inputs, actual, repeated.get("inputs"))):
        findings.add(("invalid_input", "inputs"))
        return Unqualified(tuple(sorted(findings)), tuple(differences))
    for missing in _TOOLS - inputs.keys():
        findings.add(("input_pin", missing))
    for name in sorted(inputs.keys() | actual.keys()):
        pin = inputs.get(name)
        if not manifest.token(name) or name == "project" or not _input_pin(name, pin, profile == "desktop-win-zip"):
            findings.add(("input_pin", name))
        receipt = actual.get(name)
        if not (manifest.mapping(pin) and manifest.mapping(receipt)):
            compare("selected_input_mismatch", name, "input", pin, receipt)
            continue
        for field in sorted(pin.keys() | receipt.keys()):
            compare("selected_input_mismatch", name, field, pin.get(field), receipt.get(field))
    compare("rebuild_input_mismatch", "inputs", "inputs", actual, repeated["inputs"])

    qt = selected.get("release_qt")
    if (not _matches(r"6\.(?:1[2-9]|[2-9][0-9]|[1-9][0-9]{2,})\.(?:0|[1-9][0-9]*)", qt)
            or any(not manifest.mapping(row.get("qt")) or row["qt"].get("revision") != qt
                   for row in (inputs, actual, repeated["inputs"]))):
        findings.add(("qt_release_pin", "qt"))

    if manifest.mapping(inputs.get("qt")):
        expected_role = "external" if profile == "desktop-arch-recipe" else "carried"
        if inputs["qt"].get("role") != expected_role:
            findings.add(("runtime_mismatch", "qt"))
    _check_runtime(selected, observed, repeated, findings, compare)
    for row in (selected, observed, repeated):
        if not _identity_map(row.get("members")) or not row["members"]:
            findings.add(("invalid_input", "members"))
    if all(_identity_map(row.get("members")) for row in (selected, observed, repeated)):
        for path in sorted(selected["members"].keys() | observed["members"].keys()):
            compare("member_mismatch", path, "material", selected["members"].get(path),
                    observed["members"].get(path))
        compare("rebuild_mismatch", profile, "members", observed["members"], repeated["members"])
    if findings:
        return Unqualified(tuple(sorted(findings)), tuple(deepcopy(differences)))
    layout, notices = _member_layout(selected, findings)
    if findings:
        return Unqualified(tuple(sorted(findings)))
    return _release_rows(selected, observed, repeated, layout, notices)


def _check_runtime(selected, observed, repeated, findings, compare):
    required = selected.get("runtime")
    linked = observed.get("linked")
    loaded = observed.get("runtime_loaded")
    if not all(_token_map(value) for value in (required, linked, loaded)):
        findings.add(("invalid_input", "runtime"))
        return
    declared_dynamic = selected.get("dynamic_runtime", ())
    if (type(declared_dynamic) not in (list, tuple)
            or not all(manifest.token(name) and name in required for name in declared_dynamic)):
        findings.add(("invalid_input", "dynamic_runtime"))
        return
    dynamic = set(declared_dynamic) | (_QT_DYNAMIC & required.keys())
    if selected["profile"].startswith("desktop-") and not dynamic:
        findings.add(("invalid_input", "dynamic_runtime"))
        return

    for name in sorted(required.keys() | linked.keys() | loaded.keys()):
        owner = required.get(name)
        if owner is None:
            findings.add(("runtime_unrecorded", name))
            continue
        row = selected["inputs"].get(owner)
        if not manifest.mapping(row) or row.get("role") not in ("external", "embedded", "carried"):
            findings.add(("runtime_mismatch", name))
        expected_observations = loaded if name in dynamic else linked
        if name not in expected_observations:
            findings.add(("runtime_missing", name))
        for inspection in (linked, loaded):
            if name in inspection:
                compare("runtime_mismatch", name, "owner", owner, inspection[name])
    for field in ("linked", "runtime_loaded"):
        compare("rebuild_input_mismatch", field, field, observed.get(field), repeated.get(field))


def _release_rows(selected, observed, repeated, layout, notice_paths):
    profile = selected["profile"]
    revision = selected["revision"]
    scope = selected["scope"]
    inputs = selected["inputs"]
    members = []
    evidence = []
    pins = []
    components = []

    def add(kind, subject, facts, *, pin=None, at=revision, suffix=""):
        identifier = f"native:{kind}:{subject}{suffix}"
        evidence.append(dict(id=identifier, kind=kind, subject=subject, revision=at,
                             owner="PACKAGE-001", scope=scope, facts=facts))
        if pin is not None:
            pins.append(dict(subject=subject, role=pin, evidence=identifier))
        return identifier

    patches = [
        add("patch", f"component:{name}", {"path": path, **material}, at=row["revision"],
            suffix=f"/{path}")
        for name, row in sorted(inputs.items()) for path, material in sorted(row.get("patches", {}).items())
    ]
    recipe = add("observed-build-recipe", "artifact", {
        "recipe_identity": selected["recipe"], "presets": list(selected["presets"]),
        "environment": selected["environment"], "patches": patches,
        "project_material": selected["project_material"], "recipe_material": selected["recipe_material"],
        "release_qt": selected["release_qt"], "settings": selected.get("settings", {}),
    })
    add("observed-build", "artifact", {
        "recipe": recipe, "output": observed["output"], "run": observed["run"],
        "linked": observed["linked"], "runtime_loaded": observed["runtime_loaded"],
    })
    # RELEASE-001 relates build outputs to individual delivered members.
    member_build = add("observed-build", "artifact", {
        "recipe": recipe, "outputs": {path: observed["members"][path] for path in layout},
        "static": {name: row["archives"] for name, row in observed["inputs"].items() if "archives" in row},
    }, suffix="/members")

    for path, (kind, owners) in sorted(layout.items()):
        material = observed["members"][path]
        members.append(dict(path=path, kind=kind, components=owners, **material))
        add("acquisition", f"member:{path}", material, pin="acquisition")

    for name, row in sorted({"project": {"revision": revision, "role": "carried"}, **inputs}.items()):
        role = row["role"]
        subject = f"component:{name}"
        covered = [member["path"] for member in members if name in member["components"]]
        component = dict(id=name, name=name, revision=row["revision"], role=role, members=covered,
                         dependencies=sorted(set(selected["runtime"].values())) if name == "project" else [],
                         rights=[], notices=[], sources=[])
        if role in manifest.CARRIED:
            component["build"] = member_build
        components.append(component)
        if name == "project":
            continue
        facts = {
            **row["material"], "license": row.get("license", _LICENSE_UNKNOWN),
            "notice_paths": [path for path, owner in notice_paths.items() if owner == name],
        }
        if "source" in row:
            # The selected material may be a binary. Only the locator and
            # revision describe its source until source bytes are observed.
            facts["source"] = {"location": row["source"], "revision": row["revision"]}
        else:
            facts["vendor"] = row["vendor"]
        add("selected-identity", subject, facts,
            pin=role if role in ("external", "build-only") else "source", at=row["revision"])
        if role == "embedded":
            add("selected-identity", subject, {
                **row["material"], "build": member_build, "archives": row["archives"],
            }, pin="build", at=row["revision"], suffix="/static")

    artifact = dict(
        id=scope, format=selected["recipe"], **observed.get("final_artifact", observed["output"]),
        inventory_sha256=sha256(manifest.canonical(manifest.ordered_rows(members, "path"))).hexdigest(),
    )
    composition = [dict(path=row["path"], components=row["components"]) for row in members]
    selection = {field: selected[field] for field in ("profile", "scope", "recipe", "revision", "platform")}
    review = add("observed-profile", "artifact", {
        "artifact": artifact, "complete": True, "composition": composition, "inputs": [member_build],
        **{field: selection[field] for field in ("profile", "recipe", "revision", "platform")},
    })
    result = dict(
        selection=dict(label=profile, **selection), artifact_id=scope,
        expected=artifact, observed=dict(scope=scope, artifact=artifact, members=members,
                                        composition=composition, review=review),
        selected_pins=pins, evidence=evidence, components=components,
        rights=[], notices=[], sources=[], assets=[], fonts=[],
    )
    _, invalid = manifest.shape(result)
    if invalid:
        return Unqualified(tuple(("release_input_shape", subject) for subject in sorted(invalid)))
    return Qualified(deepcopy(result), (deepcopy(observed), deepcopy(repeated)))
