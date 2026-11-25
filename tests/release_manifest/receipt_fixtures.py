"""Project already-produced receipt evidence into independently observed fixture frames."""

import json
import os
from pathlib import Path, PurePosixPath

from fixture import (
    ANCHOR, File, MAX_FILE, MAX_MEMBERS, MAX_PROJECTION, PROJECT_NOTICES, blob,
    empty, evidence, exact, identity, input_path, observe, pin, prerequisite, project, read_checked, read_file, require, review_component,
    seal, source_files,
)

PARENT = "dependencies/native-websocket-profile"
SUCCESSOR = "dependencies/bounded-http-foundation"
PROVENANCE = "share/subathon-bounded-http-foundation"
BASE_SOURCE = "server/src/native_websocket_profile.cpp"
HEADER = "server/include/subathon/server/native_websocket_profile.hpp"
TRANTOR_NOTICE = "share/licenses/trantor/License"
DROGON_NOTICES = ("share/licenses/drogon/LICENSE", "share/licenses/drogon/orm_lib-COPYING")
PATCHES = (
    ("0001-trantor-native-io.patch", PARENT, "3d6738d658f4ef8cee7c06bd338e1a4e08a55da51a935cb07487661765a1e988"),
    ("0002-drogon-native-dispatch.patch", PARENT, "10e950f63492627ca4b148d79f59630bbf41e47e52361d8aa4ef88bf4ef14852"),
    ("0003-trantor-bounded-http.patch", SUCCESSOR, "08249407fe9fa4a95860152d657ed2c440e74305561d0cd925a05d5eab28e5a4"),
    ("0004-drogon-bounded-http.patch", SUCCESSOR, "cfe4f4545094a5fe7531bfd9660e57330a14119ade8be5236d110297a19ef0ea"),
    ("0005-native-source-binding.patch", SUCCESSOR, "ca4b44419e11f3eea0fc3e7f4333ed9ffb0cb1e0d9edf37cd1aa1d15d5a77919"),
)
LIBRARIES = {"drogon": "lib/libdrogon.so.1.9.14", "trantor": "lib/libtrantor.so.1.5.29"}
UPSTREAMS = {"drogon": "3aae769cc405cf5abb9d477675c6430924f02c7c",
             "trantor": "78e480f5e138acdc05f76c55a2716718cd846364"}
NATIVE_DERIVED = "e2db9d0f97d757cc4ab740d473ab4dba6ce2b05d03c88897049921ba0ed5e92e"
SUMMARY = "THIRD_PARTY_NOTICES.md"
SODIUM_SOURCE = {"version": "1.0.22", "bytes": "2008529",
                 "sha256": "adbdd8f16149e81ac6078a03aca6fc03b592b89ef7b5ed83841c086191be3349"}
SODIUM_NOTICE = "legal/sodium.LICENSE"
GTEST_NOTICE = "legal/gtest.LICENSE"
PASSWORD_BINARY = "bin/password-test"


def sdk_bytes():
    """Independently observe files and raw links, then compare against the existing SDK seal."""
    root = input_path("RELEASE_001_SDK").resolve(strict=True)
    require(root.is_dir(), "Expected a complete SDK directory")
    receipt = json.loads(read_checked("RELEASE_001_SDK_SEAL"))
    stored = receipt["artifacts"]
    require(0 < len(stored) < MAX_MEMBERS, "SDK seal file count exceeds the fixture budget")
    regular = {}
    links = {}
    total = 0
    dir_count = 0
    for directory, subdirs, names in os.walk(root):
        dir_count += 1
        require(dir_count <= MAX_MEMBERS, "Too many SDK directories")
        require(not any((Path(directory) / name).is_symlink() for name in subdirs), "SDK directory alias")
        for name in names:
            path = Path(directory) / name
            key = path.relative_to(root).as_posix()
            require(len(regular) + len(links) < MAX_MEMBERS, "Too many SDK files")
            if path.is_symlink():
                links[key] = os.fsencode(os.readlink(path))
            else:
                regular[key] = read_file(path, min(MAX_PROJECTION - total, MAX_FILE))
                total += len(regular[key])
    require(set(stored) == (regular.keys() | links.keys()) - {"dependency-build.json"}, "SDK member set changed")
    raw_identity = regular["dependency-build.json"]
    exact(raw_identity, {"bytes": str(receipt["identity"]["bytes"]),
                         "sha256": receipt["identity"]["sha256"]}, "dependency-build.json")
    sdk = json.loads(raw_identity)
    require(sdk["format"] == 1 and sdk["artifacts"] == stored, "SDK identity and producer seal disagree")

    targets = {}
    for key, value in links.items():
        link_value = value.decode("utf-8")
        target = PurePosixPath(key).parent / link_value
        require("/" not in link_value and link_value not in ("", ".", ".."),
                f"Unsupported SDK alias value: {key}")
        visited = {key}
        while str(target) in links:
            require(str(target) not in visited, f"Cyclic SDK alias: {key}")
            visited.add(str(target))
            next_link = links[str(target)].decode("utf-8")
            require("/" not in next_link and next_link not in ("", ".", ".."),
                    f"Unsupported SDK alias chain: {key}")
            target = target.parent / next_link
        require(str(target) in regular, f"SDK alias lacks canonical target: {key}")
        targets[key] = str(target)

    expected_links = {
        alias: target.encode()
        for library, real in LIBRARIES.items()
        for alias, target in ((f"lib/lib{library}.so", f"lib{library}.so.1"),
                              (f"lib/lib{library}.so.1", PurePosixPath(real).name))
    }
    require(links == expected_links, "SDK link values or alias membership changed")
    for key, digest in stored.items():
        require(identity(regular[targets.get(key, key)])["sha256"] == digest, f"SDK producer seal changed: {key}")
    return regular, links, targets, sdk


def profile_inputs(request, regular, sdk):
    parent = blob(f"{PARENT}/manifest.json")
    successor = blob(f"{SUCCESSOR}/manifest.json")
    parent_id = identity(parent)
    successor_id = identity(successor)
    parent_doc, next_doc = json.loads(parent), json.loads(successor)
    require(parent_id["sha256"] == next_doc["parent_manifest_sha256"] ==
            "fa947feb11f2a00efad810c73af0c8e32d5f43d9a7c5ee88645112e28a1bf67e",
            "Protected parent manifest mismatch")
    require(sdk["profile_manifest_sha256"] == successor_id["sha256"], "Wrong SDK profile")
    require(regular[f"{PROVENANCE}/manifest.json"] == successor and
            regular[f"{PROVENANCE}/parent/manifest.json"] == parent, "Wrong copied profile manifests")
    require(sdk["toolchain"]["upstreams"] == parent_doc["upstreams"] == next_doc["upstreams"] == UPSTREAMS and
            sdk["toolchain"]["options"] == parent_doc["build_options"] == next_doc["build_options"] and
            next_doc["build_options"]["BUILD_ORM"] == "OFF", "Wrong SDK build inputs")

    patch_ids = []
    for name, directory, digest in PATCHES:
        packaged_path = f"{PROVENANCE}/" + ("parent/" if directory == PARENT else "") + name
        data = blob(f"{directory}/{name}")
        require(identity(data)["sha256"] == digest and regular[packaged_path] == data, f"Wrong SDK patch: {name}")
        patch_ids.append(evidence(request, name, "patch", "artifact",
                                  path=f"sdk/{packaged_path}", **identity(data)))
    original = blob(BASE_SOURCE)
    derived = regular[f"{PROVENANCE}/native_websocket_profile.cpp"]
    require(regular[f"{PROVENANCE}/{BASE_SOURCE}"] == original and
            identity(original)["sha256"] == "a5c2dd0a0a4ec87a769b4509686d0a5457f253b59417706e6445b036a535d0ac" and
            identity(derived)["sha256"] == NATIVE_DERIVED and regular[f"{PROVENANCE}/{HEADER}"] == blob(HEADER),
            "Wrong SDK first-party original, derived output or interface")
    recipe = evidence(request, "sdk-recipe", "observed-build-recipe", "artifact",
                      parent=parent_id, successor=successor_id, upstreams=UPSTREAMS,
                      options=next_doc["build_options"], patches=patch_ids,
                      first_party={"original": identity(original), "derived": identity(derived),
                                   "interface": identity(blob(HEADER))},
                      recipe_files={name: identity(regular[f"{PROVENANCE}/{name}"]) for name in
                                    ("ninja-commands.txt", "recipe-commands.json", "CMakeCache.txt",
                                     "patched-source-sha256.json")})
    return recipe, patch_ids


def archive_missing():
    request = empty("fixture-native-unobserved", "server-native", "linux-x86-64")
    regular, _, _, sdk = sdk_bytes()
    recipe, _ = profile_inputs(request, regular, sdk)
    for row in request["evidence"]:
        row["scope"] = "successor-sdk"
    evidence(request, "sealed-sdk-prefix", "sdk-observation", "artifact", scope="successor-sdk",
             recipe=recipe, identity=identity(regular["dependency-build.json"]),
             member_count=len(sdk["artifacts"]) + 1)
    evidence(request, "sealed-sdk-notices", "sdk-notice-evidence", "component:drogon", scope="successor-sdk",
             recipe=recipe, texts={path: identity(regular[path]) for path in (*DROGON_NOTICES, TRANTOR_NOTICE)})
    return request


def sdk_owner(path):
    if path.startswith(("include/drogon/", "lib/cmake/Drogon/", "lib/libdrogon.", "share/licenses/drogon/")):
        return ("drogon",)
    if path.startswith(("include/trantor/", "lib/cmake/Trantor/", "lib/libtrantor.", "share/licenses/trantor/")):
        return ("trantor",)
    if path.startswith("share/licenses/local/"):
        return (f"{path.split('/')[3]}-notices",)
    if path == "dependency-build.json" or path.startswith(f"{PROVENANCE}/"):
        binding = {"native_websocket_profile.cpp", BASE_SOURCE, HEADER, PATCHES[-1][0]}
        return ("project", "native-binding") if path.removeprefix(f"{PROVENANCE}/") in binding else ("project",)
    raise ValueError(f"Need explicit fixture ownership for SDK member {path}")


def sdk_kind(path):
    if path.startswith("share/licenses/") or Path(path).name in ("COPYING", "LICENSE.md", SUMMARY):
        return "notice"
    if path in LIBRARIES.values():
        return "payload"
    if path.endswith((".cmake", "/CMakeCache.txt", "/manifest.json", "/recipe-commands.json")):
        return "recipe"
    return "source"


SYSTEM_LIBRARIES = {
    "ld-linux-x86-64.so.2": "glibc", "libc.so.6": "glibc", "libm.so.6": "glibc",
    "libgcc_s.so.1": "libgcc", "libstdc++.so.6": "libstdc++",
    "libz.so.1": "zlib", "libuuid.so.1": "util-linux-libs", "libjsoncpp.so.27": "jsoncpp",
    "libssl.so.4": "openssl", "libcrypto.so.4": "openssl",
    "libbrotlicommon.so.1": "brotli", "libbrotlidec.so.1": "brotli",
    "libbrotlienc.so.1": "brotli", "libzstd.so.1": "zstd",
}


def sdk_runtime(request, regular, sdk):
    """Use the transitive subset of the existing passive closure rooted at these two SDK DSOs."""
    closure = json.loads(read_checked("RELEASE_001_SDK_LINKS"))
    require(0 < len(closure) <= MAX_MEMBERS, "SDK passive closure exceeds its record budget")
    roots = {}
    for owner, library in LIBRARIES.items():
        candidates = [key for key in closure if PurePosixPath(key).name == PurePosixPath(library).name]
        require(len(candidates) == 1, f"Missing exact passive root for {owner}")
        roots[owner] = candidates[0]
        row = closure[candidates[0]]
        exact(regular[library], {"bytes": str(row["bytes"]), "sha256": row["sha256"]}, owner)
    rows = {}
    pending = list(roots.values())
    while pending:
        key = pending.pop()
        if key in rows:
            continue
        row = closure[key]
        require(set(row["needed"]) == set(row["resolved"]), "Incomplete SDK runtime edge")
        rows[key] = row
        pending.extend(row["resolved"].values())

    owners = {path: owner for owner, path in roots.items()}
    for row in rows.values():
        for soname, resolved in row["resolved"].items():
            if resolved not in roots.values():
                owner = f"runtime-{SYSTEM_LIBRARIES[soname]}"
                require(owners.setdefault(resolved, owner) == owner, "Ambiguous runtime owner")

    edges = {owner: set() for owner in owners.values()}
    for key, row in rows.items():
        owner = owners[key]
        edges[owner].update(owners[path] for path in row["resolved"].values() if owners[path] != owner)
    packages = sdk["toolchain"]["packages"]
    for owner in sorted(edges.keys() - roots.keys()):
        selected = {row["soname"][0]: {"sha256": row["sha256"], "bytes": str(row["bytes"])}
                    for key, row in rows.items() if owners[key] == owner}
        package = packages.get(owner.removeprefix("runtime-"), {})
        revision = package.get("installed", "").strip() or "/".join(
            item["sha256"] for _, item in sorted(selected.items()))
        prerequisite(request, owner, revision, kind="external", owner="fixture linker",
                     files=selected, dependencies=sorted(edges[owner]))
        request["components"][-1]["dependencies"] = sorted(edges[owner])
    return edges


def companion(request, component, *, revision, recipe, sources):
    return evidence(request, f"access-{component}", "modeled-source-access", f"component:{component}",
                    revision=revision, tree=revision, recipe=recipe, sources=sources,
                    equivalent_access=True, further_charge=False)


def native(case, *, omit_trantor=False, stale=False, missing_patch=False, limited_rights=False):
    request = empty(f"fixture-D{case}", "server-native", "linux-x86-64", "Successor SDK projection")
    regular, links, targets, sdk = sdk_bytes()
    recipe, patches = profile_inputs(request, regular, sdk)
    edges = sdk_runtime(request, regular, sdk)
    files = source_files()
    files.update({f"sdk/{name}": File(data, sdk_kind(name), sdk_owner(name)) for name, data in regular.items()})
    files.update({f"sdk/{name}": File(data, "alias", sdk_owner(name), f"sdk/{targets[name]}")
                  for name, data in links.items()})
    original_inventory = files.copy()
    if omit_trantor:
        del files[f"sdk/{TRANTOR_NOTICE}"]
    seal(request, files, inputs=(recipe,))
    # Every case has its own scope and review records. The resealed M7 uses
    # the reduced observed coverage while keeping the original notice obligation.
    prefix = f"fixture-D{case}-"
    if omit_trantor:
        prefix = "fixture-D7-resealed-"
    sdk_build = evidence(request, "sdk-selected-build", "observed-build", "artifact",
                         recipe=recipe, sdk_identity=identity(regular["dependency-build.json"]),
                         outputs={owner: identity(regular[path]) for owner, path in LIBRARIES.items()},
                         derived_binding=identity(regular[f"{PROVENANCE}/native_websocket_profile.cpp"]))
    for component, path in LIBRARIES.items():
        pin(request, f"component:{component}", "build", f"pin-{component}",
            revision=UPSTREAMS[component], build=sdk_build, output=identity(regular[path]))
    pin(request, "component:native-binding", "build", "pin-native-binding",
        build=sdk_build, output=identity(regular[f"{PROVENANCE}/native_websocket_profile.cpp"]))
    project_notices = (*PROJECT_NOTICES,
                       *(f"sdk/{PROVENANCE}/{name}" for name in ("COPYING", "LICENSE.md", SUMMARY)))
    project_paths = [path for path, file in files.items()
                     if "project" in file.owners and file.kind in ("source", "recipe")]
    project(request, review_prefix=prefix, notices=project_notices, source_paths=project_paths,
            dependencies=("drogon", "trantor", "native-binding"), build=sdk_build, recipe=recipe,
            patches=patches, source_inputs=(recipe,))

    binding_paths = [path for path, file in files.items() if "native-binding" in file.owners]
    review_component(request, "native-binding", name="Native source binding", dependencies=("drogon", "trantor"),
                     notices=project_notices[:4], license_name="GPL-3.0-or-later", source_paths=binding_paths,
                     recipe=recipe, patches=patches, source_inputs=(recipe,), build=sdk_build, review_prefix=prefix)
    for owner, notice_paths, license_name, patch_indices in (
        ("drogon", DROGON_NOTICES, "MIT AND BSD-3-Clause", (1, 3)),
        ("trantor", (TRANTOR_NOTICE,), "BSD-3-Clause", (0, 2)),
    ):
        revision = UPSTREAMS[owner]
        access = companion(request, owner, revision=revision, recipe=recipe,
                           sources={owner: revision, "patches": [patches[index] for index in patch_indices]})
        paths = [f"sdk/{path}" for path in notice_paths]
        review_component(request, owner, name=owner, revision=revision, dependencies=sorted(edges[owner]),
                         notices=paths, license_name=license_name, recipe=recipe, mode="companion", access=access,
                         patches=[patches[index] for index in patch_indices], build=sdk_build,
                         source_inputs=(recipe,), review_prefix=prefix)
        facts = evidence_row(request, f"{prefix}N-{owner}")["facts"]
        facts["required_texts"] = {path: identity(original_inventory[path].data) for path in paths}
        facts["reviewed_delivery_paths"] = [path for path in paths if path in files]
    add_sdk_notice_reviews(request, files, sdk, prefix)

    if stale:
        request["expected"] = observe(original_inventory)[0]
    if missing_patch:
        row = decision(request, "sources", "native-binding")
        patch = PATCHES[-1][0]
        path = f"sdk/{PROVENANCE}/{patch}"
        row["patches"].remove(patch)
        row["paths"].remove(path)
        review = evidence_row(request, row["review"])["facts"]
        review["patches"].remove(patch)
        review["paths"].remove(path)
    if limited_rights:
        vary_native_rights(request, prefix)
    return request


def evidence_row(request, key):
    return next(row for row in request["evidence"] if row["id"] == key)


def decision(request, rows, component):
    return next(row for row in request[rows] if row["component"] == component)


def add_sdk_notice_reviews(request, files, sdk, prefix):
    packages = sdk["toolchain"]["packages"]
    licensed = {
        "gcc": "LicenseRef-GCC-Runtime-Library-Exception",
        "libgcc": "LicenseRef-GCC-Runtime-Library-Exception",
        "libstdc++": "LicenseRef-GCC-Runtime-Library-Exception",
        "jsoncpp": "MIT OR LicenseRef-Public-Domain",
        "openssl": "Apache-2.0",
        "util-linux-libs": "LicenseRef-Util-Linux-Copied-Notices",
        "zlib": "Zlib",
    }
    for name, license_name in licensed.items():
        paths = sorted(path for path, file in files.items() if f"{name}-notices" in file.owners)
        package = packages[name]
        require({Path(path).name: identity(files[path].data)["sha256"] for path in paths} == package["notices"],
                f"SDK copied notices changed: {name}")
        review_component(request, f"{name}-notices", name=f"{name} copied notices",
                         revision=package["installed"].strip(), notices=paths, license_name=license_name,
                         source_paths=paths, review_prefix=prefix)


def vary_native_rights(request, prefix):
    decision(request, "rights", "drogon")["state"] = "prohibited"
    evidence_row(request, f"{prefix}R-drogon")["facts"]["state"] = "prohibited"
    local = evidence(request, "trantor-condition-sdk-only", "rights-condition", "component:trantor",
                     scope="sdk-local", satisfied=True, condition="copy-trantor-notice")
    trantor = decision(request, "rights", "trantor")
    trantor["conditions"] = [local]
    facts = evidence_row(request, trantor["review"])["facts"]
    facts["conditions"] = [local]
    facts["required_conditions"] = ["copy-trantor-notice"]
    evidence_row(request, f"{prefix}N-trantor")["facts"]["reviewed_delivery_paths"].clear()


def linked_password(*, current_summary=False):
    request = empty("fixture-D5", "server-native", "linux-x86-64", "Password test link projection")
    receipt_path = input_path("RELEASE_001_PASSWORD_RECEIPT")
    receipt_bytes = read_checked("RELEASE_001_PASSWORD_RECEIPT")
    receipt = json.loads(receipt_bytes)
    require(receipt["target"] == "server_password_hashing_acceptance" and
            receipt["complete_link_closure"] is True, "D5 needs the exact complete password test linkage")
    static = receipt["static"]
    require(set(static) == {"sodium", "gtest"}, "D5 needs both exact static contributions")
    require(static["sodium"]["source"] == SODIUM_SOURCE, "Wrong sodium source archive")
    require(set(static["sodium"]["archives"]) == {"libsodium.a"}, "Wrong selected sodium archive")
    require(static["gtest"]["version"] and static["gtest"]["origin"] and static["gtest"]["revision"] and
            static["gtest"]["archives"], "Unresolved GTest source or static archive")

    root = receipt_path.parent
    exact(read_file(root / "archives/sodium/libsodium-1.0.22.tar.gz"), {
        key: SODIUM_SOURCE[key] for key in ("bytes", "sha256")
    }, "libsodium-1.0.22.tar.gz")
    files = source_files()
    files[SUMMARY] = File(blob(SUMMARY), "notice")
    require(b"libsodium | Passwords/credentials/backups | Selected; not integrated" in files[SUMMARY].data,
            "D5 pinned summary no longer supplies the stated stale assertion")
    selected = (
        (PASSWORD_BINARY, receipt["output"], "payload", ("project", "sodium", "gtest")),
        (SODIUM_NOTICE, static["sodium"]["notice"], "notice", ("sodium",)),
        (GTEST_NOTICE, static["gtest"]["notice"], "notice", ("gtest",)),
    )
    for path, pin_record, kind, owners in selected:
        data = read_file(root / path)
        exact(data, pin_record, path)
        files[path] = File(data, kind, owners)

    for owner, row in static.items():
        require(0 < len(row["archives"]) <= 8, "D5 static archive count exceeds the fixture budget")
        for name, seal_record in row["archives"].items():
            require(PurePosixPath(name).name == name and name.endswith(".a"), "Invalid static archive name")
            exact(read_file(root / "archives" / owner / name), seal_record, name)
    sources = receipt["project_sources"]
    require(set(sources) >= {"CMakeLists.txt", "tests/CMakeLists.txt", "server/CMakeLists.txt",
                             "server/src/password_hashing.cpp", "server/include/subathon/server/password_hashing.hpp",
                             "tests/acceptance/password_hashing_tests.cpp", "tests/support/password_hashing_failures.cpp",
                             "tests/support/password_hashing_failures.hpp", "cmake/GoogleTestDependency.cmake",
                             "cmake/SodiumDependency.cmake"},
            "D5 receipt omits project build inputs")
    require(len(sources) <= MAX_MEMBERS, "D5 source input count exceeds the fixture budget")
    for path, seal_record in sources.items():
        exact(blob(path), seal_record, path)

    recipe = evidence(request, "password-link-recipe", "observed-build-recipe", "artifact",
                      link=receipt["recipe"], receipt=identity(receipt_bytes), project_inputs=sources)
    build = evidence(request, "password-build", "observed-build", "artifact",
                     recipe=recipe, output=receipt["output"], static={
                         owner: row["archives"] for owner, row in static.items()
                     })
    external = receipt["external"]
    require(len(external) <= MAX_MEMBERS, "D5 external closure exceeds the fixture budget")
    for owner, row in external.items():
        prerequisite(request, owner, row["revision"], kind="external", owner="fixture linker", files=row["files"])
        request["components"][-1]["dependencies"] = row["dependencies"]
    seal(request, files, inputs=(recipe, build))
    access = companion(request, "project", revision=ANCHOR, recipe=recipe, sources=sources)
    project(request, review_prefix="fixture-D5-", mode="companion", build=build, recipe=recipe, access=access,
            source_inputs=(recipe,), dependencies=("sodium", "gtest", *external),
            notices=("COPYING", "LICENSE.md", SUMMARY))
    for owner, notice_path in (("sodium", SODIUM_NOTICE), ("gtest", GTEST_NOTICE)):
        row = static[owner]
        revision = SODIUM_SOURCE["version"] if owner == "sodium" else row["revision"]
        access = companion(request, owner, revision=revision, recipe=recipe, sources=row["source"])
        package = ({"version": row["version"], "origin": row["origin"]} if owner == "gtest"
                   else {"version": SODIUM_SOURCE["version"]})
        pin(request, f"component:{owner}", "build", f"pin-{owner}", revision=revision, build=build,
            archives=row["archives"], source=row["source"], package=package)
        review_component(request, owner, name=owner, revision=revision, role="embedded",
                         dependencies=row["dependencies"], notices=(notice_path,), license_name=row["license"],
                         source_paths=(), mode="companion", recipe=recipe, build=build, access=access,
                         source_inputs=(recipe,), review_prefix="fixture-D5-")

    integration = "embedded" if current_summary else "not-integrated"
    summary = evidence(request, "sodium-applicable-summary", "reviewed-notice-summary",
                       "component:sodium", build=build, applicable=True,
                       artifact=request["observed"]["artifact"], basis=recipe,
                       integration=integration)
    evidence_row(request, "fixture-D5-N-sodium")["facts"]["summary"] = summary
    return request
