"""Independent owner inputs and modeled observations for frozen contract r3."""

from copy import deepcopy
from dataclasses import dataclass
from hashlib import sha256
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
ANCHOR = "f63a418b9bb9248a9cc9b0e438139ef2c4b47466"
LABEL = 'R\t"\\é\x1f'
FRAME = "fixture-frame/1"
PROJECT_NOTICES = ("COPYING", "LICENSE.md")
M10_EXTRA = (".github/workflows/ci.yml", "cmake/GoogleTestDependency.cmake",
             "tools/schema_validation/requirements.lock")
WEB = ("localization/generated/web/en.json", "localization/generated/web/de.json")
FONT_REVISION = "c4a321e123e4d4ff315f57f4e0adf294fe3a95be"
FONT_OFL = (
    "https://raw.githubusercontent.com/notofonts/latin-greek-cyrillic/"
    f"{FONT_REVISION}/OFL.txt"
)
OLD_CMAKE = {"bytes": "1557", "sha256": "4bc6b4f0914f32f077ed0422a5e9d30e1b7796b50b52f0baf0b34d298ea3b724"}
WHEEL_PATH = "vendor/annotated_types-0.8.0-py3-none-any.whl"
WHEEL = {"bytes": "13427", "sha256": "f072f4d804ea359e4eaf198b1af7a8b0943881a87f31bb764f8bf219bb9419e0"}
MAX_FILE = 16 * 1024 * 1024
MAX_PROJECTION = 32 * 1024 * 1024
MAX_MEMBERS = 512


class FixtureInputError(Exception):
    """A required independent input is absent or disagrees with its seal."""


def require(condition, detail):
    if not condition:
        raise FixtureInputError(detail)


def input_path(variable):
    supplied = os.environ.get(variable)
    require(bool(supplied), f"Supply the reviewed fixture input {variable}")
    return Path(supplied)


def read_file(path, limit=MAX_FILE):
    require(path.is_file() and not path.is_symlink(), f"Expected regular file: {path.name}")
    with path.open("rb") as stream:
        contents = stream.read(limit + 1)
    require(len(contents) <= limit, f"Fixture file exceeds its budget: {path.name}")
    return contents


def blob(path):
    object_name = f"{ANCHOR}:{path}"
    args = {"cwd": ROOT, "timeout": 15}
    size = int(subprocess.check_output(["git", "cat-file", "-s", object_name], **args))
    require(size <= MAX_FILE, f"Protected blob exceeds fixture budget: {path}")
    data = subprocess.check_output(["git", "cat-file", "blob", object_name], **args)
    require(len(data) == size, f"Protected blob length changed: {path}")
    return data


def identity(data):
    return {"bytes": str(len(data)), "sha256": sha256(data).hexdigest()}


def exact(data, pin, subject):
    require(identity(data) == pin, f"Independent input seal disagrees: {subject}")


@dataclass(frozen=True)
class File:
    data: bytes
    kind: str
    owners: tuple[str, ...] = ("project",)
    alias_target: str | None = None


def tracked(path, kind="source"):
    return File(blob(path), kind)


def source_files():
    return {"CMakeLists.txt": tracked("CMakeLists.txt"),
            **{path: tracked(path, "notice") for path in PROJECT_NOTICES}}


def observe(files):
    """Observe member bytes and the fixture frame before declaring owner reviews."""
    require(len(files) <= MAX_MEMBERS, "Fixture projection has too many members")
    require(sum(len(f.data) for f in files.values()) <= MAX_PROJECTION, "Fixture projection exceeds byte budget")
    frame = sha256(b"SBT-FIXTURE-1\n")
    frame_length = len(b"SBT-FIXTURE-1\n")
    members = []
    for path in sorted(files, key=str.encode):
        file = files[path]
        member = {"path": path, "kind": file.kind,
                  "components": sorted(file.owners, key=str.encode), **identity(file.data)}
        if file.alias_target is not None:
            member["alias_target"] = file.alias_target
        members.append(member)
        prefix = path.encode() + b"\0" + member["bytes"].encode("ascii") + b"\0"
        frame.update(prefix)
        frame.update(file.data)
        frame_length += len(prefix) + len(file.data)
    artifact = {"id": "A", "format": FRAME, "bytes": str(frame_length),
                "sha256": frame.hexdigest(), "inventory_sha256": sha256(inventory_bytes(members)).hexdigest()}
    return artifact, members


def inventory_bytes(members):
    # These fixture paths/IDs contain no controls. JSON's short escapes are immaterial.
    return (json.dumps(members, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n").encode()


def empty(scope, profile="source", platform="any", label=LABEL):
    return {"selection": {"label": label, "platform": platform, "profile": profile,
                          "recipe": FRAME, "revision": ANCHOR, "scope": scope},
            "artifact_id": "A", "selected_pins": [], "evidence": [],
            "components": [], "rights": [], "notices": [], "sources": [], "assets": [], "fonts": []}


def evidence(request, key, kind, subject, *, revision=ANCHOR, owner="fixture owner", scope=None, **facts):
    request["evidence"].append({
        "id": key, "kind": kind, "owner": owner, "revision": revision,
        "subject": subject, "scope": request["selection"]["scope"] if scope is None else scope,
        "facts": deepcopy(facts),
    })
    return key


def pin(request, subject, role, key, **facts):
    evidence(request, key, "selected-identity", subject, **facts)
    request["selected_pins"].append({"subject": subject, "role": role, "evidence": key})


def seal(request, files, *, font_roles=(), inputs=()):
    artifact, members = observe(files)
    request["expected"] = artifact
    composition = [{"path": row["path"], "components": list(row["components"])} for row in members]
    review = evidence(request, "fixture-profile", "observed-profile", "artifact",
                      owner="fixture packager", artifact=artifact, profile=request["selection"]["profile"],
                      complete=True, composition=composition, required_font_roles=list(font_roles),
                      inputs=list(inputs))
    request["observed"] = {"artifact": deepcopy(artifact), "review": review,
                           "scope": request["selection"]["scope"], "members": members,
                           "composition": composition}


def review_component(request, component, *, name, revision=ANCHOR, role="carried",
                     dependencies=(), notices=(), license_name, source_paths=(), patches=(),
                     source_inputs=(), mode="included", recipe=FRAME, build=None, access=None,
                     review_prefix, ids=None):
    """Declare case-scoped modeled reviews and their precise covered bytes."""
    rows = request["observed"]["members"]
    paths = [row["path"] for row in rows if component in row["components"]]
    covered = [row for row in rows if row["path"] in paths]
    subject = f"component:{component}"
    suffix = "" if component == "project" else f"-{component}"
    n, r, s = ids or tuple(f"{letter}{suffix}" for letter in "NRS")
    scope = request["selection"]["scope"]
    binding = {"artifact": request["observed"]["artifact"], "covered": covered, "build": build}
    notice = evidence(request, f"{review_prefix}N{suffix}", "notice-review", subject, revision=revision,
                      **binding, license=license_name, paths=list(notices),
                      texts=[row for row in rows if row["path"] in notices])
    right = evidence(request, f"{review_prefix}R{suffix}", "rights-review", subject, revision=revision,
                     **binding, state="permitted", conditions=[])
    source = evidence(request, f"{review_prefix}S{suffix}", "source-review", subject, revision=revision,
                      **binding, mode=mode, recipe=recipe, patches=list(patches),
                      paths=list(source_paths), prerequisites=list(source_inputs), access=access)
    request["notices"].append({"id": n, "component": component, "license": license_name,
                               "paths": list(notices), "review": notice})
    request["rights"].append({"id": r, "component": component, "scope": scope,
                              "state": "permitted", "conditions": [], "review": right})
    source_row = {"id": s, "component": component, "mode": mode, "revision": revision, "recipe": recipe,
                  "patches": list(patches), "paths": list(source_paths),
                  "prerequisites": list(source_inputs), "review": source}
    row = {"id": component, "name": name, "revision": revision, "role": role, "members": paths,
           "dependencies": list(dependencies), "notices": [n], "rights": [r], "sources": [s]}
    if build is not None:
        row["build"] = source_row["build"] = build
    if access is not None:
        source_row["access"] = access
    request["sources"].append(source_row)
    request["components"].append(row)


def prerequisite(request, component, revision, *, kind, owner, **facts):
    pin(request, f"component:{component}", kind, f"pin-{component}", owner=owner, revision=revision, **facts)
    request["components"].append({"id": component, "name": component, "role": kind, "revision": revision,
                                  "members": [], "dependencies": [], "rights": [], "notices": [], "sources": []})


def project(request, *, review_prefix="fixture-", ids=None, source_paths=("CMakeLists.txt",), **details):
    review_component(request, "project", name="Subathon Bot", license_name="GPL-3.0-or-later",
                     notices=details.pop("notices", PROJECT_NOTICES), source_paths=source_paths,
                     review_prefix=review_prefix, ids=ids, **details)


def f0():
    request = empty("fixture-source")
    files = source_files()
    seal(request, files)
    for path, file in files.items():
        pin(request, f"member:{path}", "source", f"selected-{path}", **identity(file.data))
    project(request)
    return request


def m10(*, wheel=False):
    request = empty("fixture-source-M10")
    files = source_files()
    files.update({path: tracked(path) for path in M10_EXTRA})
    add_m10_evidence(request)
    if wheel:
        data = read_file(input_path("RELEASE_001_WHEEL"), 13427)
        exact(data, WHEEL, WHEEL_PATH)
        files[WHEEL_PATH] = File(data, "payload", ())
        request["selected_pins"].append({"subject": f"member:{WHEEL_PATH}", "role": "acquisition",
                                         "evidence": "annotated-types-acquisition"})
    seal(request, files)
    for path in ("CMakeLists.txt", *PROJECT_NOTICES, *M10_EXTRA):
        pin(request, f"member:{path}", "source", f"selected-{path}", **identity(files[path].data))
    project(request, review_prefix="fixture-M10-", ids=("N10", "R10", "S10"),
            source_paths=("CMakeLists.txt", *M10_EXTRA))
    return request


def add_m10_evidence(request):
    ci = blob(M10_EXTRA[0])
    ci_pins = (
        ("qtbase", "943023b929d0c08eafe7341700b5ed7601284410b7afd6093f950be291d1d7cd"),
        ("icu", "1fdbcd67b1760f3a3f031c64637a9f19992aed2f35aa615e12a468f8c655d4da"),
    )
    for item, digest in ci_pins:
        require(f"{digest}  {item}.7z".encode() in ci, f"Wrong {item} source anchor")
        evidence(request, f"{item}-ci-pin", "ci-acquisition", f"component:{item}",
                 revision="Qt CI 6.12.0", archive=f"{item}.7z", sha256=digest,
                 producer={"path": M10_EXTRA[0], **identity(ci)})
    gtest = "063de7e9578f82b369302001269680b4b1553359"
    recipe = blob(M10_EXTRA[1])
    require(f"GIT_TAG {gtest}".encode() in recipe, "Wrong GoogleTest source anchor")
    evidence(request, "gtest-fetch", "source-acquisition", "component:gtest",
             revision=gtest, producer={"path": M10_EXTRA[1], **identity(recipe)})
    ubuntu_path = "tools/native_websocket_ubuntu.json"
    ubuntu = blob(ubuntu_path)
    jsoncpp = next(p for p in json.loads(ubuntu)["packages"] if p["name"] == "libjsoncpp25")
    evidence(request, "jsoncpp-ubuntu-pin", "ci-acquisition", "component:jsoncpp",
             revision=jsoncpp["version"], package=jsoncpp, producer={"path": ubuntu_path, **identity(ubuntu)})
    wheels_path = "tools/schema_validation/wheels.json"
    wheels = blob(wheels_path)
    annotated = next(p for p in json.loads(wheels)["packages"] if p["name"] == "annotated-types")
    selected = annotated["artifacts"][0]
    require({"bytes": str(selected["size_bytes"]), "sha256": selected["sha256"]} == WHEEL and
            selected["filename"] == WHEEL_PATH.split("/")[-1], "Wrong wheel acquisition anchor")
    evidence(request, "annotated-types-acquisition", "acquisition", f"member:{WHEEL_PATH}",
             revision=annotated["version"], **WHEEL, license_files=selected["license_files_sha256"],
             producer={"path": wheels_path, **identity(wheels)})


def m6():
    request = empty("fixture-D6", "web-viewer", label="Built-in viewer translations")
    files = source_files()
    files.update({path: tracked(path, "payload") for path in WEB})
    recipe = "viewer-translation-recipe"
    evidence(request, recipe, "generation-recipe", "component:project",
             catalog=identity(blob("localization/messages.json")),
             generator=identity(blob("tools/translation_catalog.py")),
             catalog_path="localization/messages.json", generator_path="tools/translation_catalog.py",
             outputs={path: identity(files[path].data) for path in WEB})
    ofl = read_checked("RELEASE_001_NOTO_OFL")
    evidence(request, "noto-ofl", "upstream-notice", "component:noto-sans", revision=FONT_REVISION,
             version="2.015", license="OFL-1.1", url=FONT_OFL, text=ofl.decode("utf-8"), **identity(ofl))
    build = evidence(request, "viewer-build", "observed-build", "component:project",
                     recipe=recipe, outputs={path: identity(files[path].data) for path in WEB})
    access = evidence(request, "viewer-source-access", "modeled-source-access", "component:project",
                      tree=ANCHOR, recipe=recipe, paths=["localization/messages.json", "tools/translation_catalog.py"],
                      equivalent_access=True, further_charge=False)
    seal(request, files, font_roles=("regular", "bold"), inputs=(recipe,))
    project(request, review_prefix="fixture-D6-", mode="companion", build=build, recipe=recipe,
            access=access, source_inputs=(recipe,))
    for language, path in zip(("en", "de"), WEB):
        asset = f"viewer-{language}"
        right = f"R-{asset}"
        review = evidence(request, f"fixture-D6-{right}", "rights-review", f"asset:{asset}",
                          artifact=request["observed"]["artifact"], path=path, **identity(files[path].data),
                          state="permitted", conditions=[])
        request["rights"].append({"id": right, "component": "project", "scope": "fixture-D6",
                                  "state": "permitted", "conditions": [], "review": review})
        request["assets"].append({"id": asset, "component": "project", "path": path,
                                  "revision": ANCHOR, "right": right})
    return request


def read_checked(variable):
    """Use a separate owner pin for an external data file, never its self-declared hash."""
    path = input_path(variable)
    digest = os.environ.get(f"{variable}_SHA256")
    require(bool(digest), f"Supply the independent seal {variable}_SHA256")
    data = read_file(path)
    require(sha256(data).hexdigest() == digest, f"External input disagrees with {variable}_SHA256")
    return data
