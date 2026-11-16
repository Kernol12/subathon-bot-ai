#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build/seal corrected SDKs and technically gate each new consumer before use.

The default preserves API-012's Ubuntu recipe. The additive bounded-HTTP profile
also supports explicitly labelled local development. The gate checks passive ELF/
ABI closure before load, controlled identity, actual maps and unchanged seals.
Local runs are development evidence; acceptance requires gated CI on reviewed head.
"""
from __future__ import annotations

import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import stat
import subprocess
import sys

import setup_drogon

REPO = Path(__file__).resolve().parents[1]
PROFILE = REPO / "dependencies/native-websocket-profile"
PRESET = "native-websocket-profile-debug"
SYSTEM_LIB_DIRS = (Path("/lib/x86_64-linux-gnu"), Path("/usr/lib/x86_64-linux-gnu"))
COMMANDS: list[list[str]] = []
BUILD_TEMP: Path | None = None
SUCCESSOR = False
LOCAL_DEVELOPMENT = False
BINARY_DIR: Path | None = None
SOURCE_SEED: Path | None = None
PARENT_PROFILE = REPO / "dependencies/native-websocket-profile"
PROVENANCE = "share/subathon-native-profile"
CONSUMER = "native"
RULING = "https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6908247970"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def file_identity(path: Path) -> dict:
    return {"sha256": digest(path), "bytes": path.stat().st_size}


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def clean_env(sdk: Path | None = None) -> dict[str, str]:
    environment = {"PATH": "/usr/bin:/bin", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8"}
    if sdk is not None:
        environment["LD_LIBRARY_PATH"] = str(sdk / "lib")
    return environment


def run(*args: object, env: dict | None = None, timeout: int = 1200, echo: bool = True) -> str:
    command = [str(arg) for arg in args]
    COMMANDS.append(command)
    print("+", shlex.join(command), flush=True)
    if env is None and BUILD_TEMP is not None:
        env = os.environ.copy()
        env["TMPDIR"] = str(BUILD_TEMP)
    result = subprocess.run(command, cwd=REPO, env=env, capture_output=True,
                            text=True, encoding="utf-8", timeout=timeout)
    if echo:
        print(result.stdout, end="", flush=True)
    print(result.stderr, end="", file=sys.stderr, flush=True)
    result.check_returncode()
    return result.stdout


def inventory(root: Path, *, omit_identity: bool = False) -> dict[str, str]:
    """Hash every name, including hidden files and in-prefix file symlink aliases."""
    result = {}
    for path in sorted(root.rglob("*")):
        name = path.relative_to(root).as_posix()
        resolved = path.resolve(strict=True)
        require(resolved.is_relative_to(root), f"Escaping artifact: {path}")
        mode = path.lstat().st_mode
        if stat.S_ISDIR(mode):
            continue
        require(not path.is_symlink() or not resolved.is_dir(), f"Directory alias: {path}")
        require(stat.S_ISREG(resolved.stat().st_mode), f"Special artifact: {path}")
        require(stat.S_ISREG(mode) or stat.S_ISLNK(mode), f"Special artifact: {path}")
        if not (omit_identity and name == "dependency-build.json"):
            result[name] = digest(resolved)
    return result


def native_profile() -> dict:
    manifest = json.loads((PARENT_PROFILE / "manifest.json").read_text(encoding="utf-8"))
    require(manifest["format"] == 1 and manifest["profile"] ==
            "subathon.native-websocket-profile.v1.api012-candidate4", "Wrong profile")
    require(manifest["upstreams"] == {"drogon": setup_drogon.DROGON_COMMIT,
                                     "trantor": setup_drogon.TRANTOR_COMMIT}, "Pin drift")
    require(manifest["build_options"] == setup_drogon.OPTIONS, "Option drift")
    require([(p["file"], p["apply_at"]) for p in manifest["ordered_patches"]] == [
        ("0001-trantor-native-io.patch", "trantor"),
        ("0002-drogon-native-dispatch.patch", "drogon")], "Patch order/target drift")
    for patch in manifest["ordered_patches"]:
        require(digest(PARENT_PROFILE / patch["file"]) == patch["sha256"], "Patch digest mismatch")
    first = manifest["first_party"]
    require(first["file"] == "server/src/native_websocket_profile.cpp" and
            digest(REPO / first["file"]) == first["sha256"], "First-party digest mismatch")
    return manifest


def profile() -> dict:
    parent = native_profile()
    if not SUCCESSOR:
        return parent
    manifest = json.loads((PROFILE / "manifest.json").read_text(encoding="utf-8"))
    require(manifest["format"] == 1 and manifest["profile"] ==
            "subathon.bounded-http-foundation.v1" and manifest["abi_marker"] ==
            "DROGON_SUBATHON_BOUNDED_HTTP_V1", "Wrong successor profile")
    require(manifest["parent_manifest_sha256"] == digest(PARENT_PROFILE / "manifest.json") and
            manifest["upstreams"] == parent["upstreams"] and
            manifest["build_options"] == parent["build_options"], "Successor parent/recipe drift")
    require([(p["file"], p["apply_at"]) for p in manifest["ordered_patches"]] == [
        ("0003-trantor-bounded-http.patch", "trantor"),
        ("0004-drogon-bounded-http.patch", "drogon"),
        ("0005-native-source-binding.patch", "first_party")], "Successor patch order drift")
    for patch in manifest["ordered_patches"]:
        require(digest(PROFILE / patch["file"]) == patch["sha256"], "Successor patch digest drift")
    require(manifest["first_party"]["file"] == parent["first_party"]["file"] and
            manifest["first_party"]["sha256"] == parent["first_party"]["sha256"],
            "Successor original first-party source drift")
    return manifest


def elf(path: Path, receipts: Path) -> dict:
    """readelf only: never ldd, a loader invocation, dlopen or an executable probe."""
    text = run("readelf", "--wide", "--file-header", "--program-headers", "--dynamic",
               "--notes", "--version-info", "--dyn-syms", path, env=clean_env(), echo=False)
    require(re.search(r"Class:\s+ELF64\b", text) is not None and
            re.search(r"Machine:\s+Advanced Micro Devices X86-64\b", text) is not None,
            f"Non-x86-64 ELF: {path}")
    # A general loader emulator is out of scope. Refuse nonselected search mechanisms.
    require(not re.search(r"\((?:RPATH|AUDIT|DEPAUDIT|FILTER|AUXILIARY)\)", text) and
            "NODEFLIB" not in text, f"Unsupported ELF loading policy: {path}")
    metadata = {
        **file_identity(path),
        "needed": re.findall(r"\(NEEDED\).*?\[([^\]]+)\]", text),
        "soname": re.findall(r"\(SONAME\).*?\[([^\]]+)\]", text),
        "runpath": re.findall(r"\(RUNPATH\).*?\[([^\]]*)\]", text),
        "interpreter": re.findall(r"Requesting program interpreter:\s*([^\]]+)\]", text),
        **elf_symbols_and_versions(text),
    }
    receipts.mkdir(parents=True, exist_ok=True)
    (receipts / f"{metadata['sha256']}.readelf.txt").write_text(text, encoding="utf-8")
    return metadata


def elf_symbols_and_versions(text: str) -> dict:
    """Parse GNU readelf's C-locale dynamic tables, checking their declared counts."""
    counts = re.findall(r"Symbol table '\.dynsym' contains (\d+) entries:", text)
    rows = re.findall(
        r"^[ \t]*\d+:[ \t]+[0-9a-fA-F]{16}[ \t]+\S+[ \t]+(\S+)[ \t]+(\S+)"
        r"[ \t]+(\S+)[ \t]+(\S+)(?:[ \t]+(\S+)(?:[ \t]+\(\d+\))?)?[ \t]*$",
        text, re.MULTILINE)
    require(len(counts) == 1 and len(rows) == int(counts[0]), "Incomplete dynamic symbol table")
    symbols = []
    for kind, binding, visibility, index, raw in rows:
        if not raw:  # The mandatory empty symbol at index zero.
            continue
        name, separator, version = raw.partition("@")
        symbols.append({"name": name, "version": version.lstrip("@") if separator else None,
                        "default_version": version.startswith("@"), "type": kind,
                        "binding": binding, "visibility": visibility, "index": index})
    definition_counts = re.findall(r"Version definition section .* contains (\d+) entr", text)
    definitions = re.findall(r"Rev:\s+\d+\s+Flags:.*?Index:\s+\d+\s+Cnt:\s+\d+\s+Name:\s+(\S+)", text)
    require(len(definition_counts) <= 1 and
            len(definitions) == (int(definition_counts[0]) if definition_counts else 0),
            "Incomplete version definition table")
    need_counts = re.findall(r"Version needs section .* contains (\d+) entr", text)
    needs = {}
    active = None
    expected_versions = {}
    for line in text.splitlines():
        file_match = re.search(r"Version:\s+\d+\s+File:\s+(\S+)\s+Cnt:\s+(\d+)", line)
        if file_match:
            active, count = file_match.groups()
            require(active not in needs, "Duplicate version dependency")
            needs[active] = []
            expected_versions[active] = int(count)
        version_match = re.search(r"Name:\s+(\S+)\s+Flags:.*?Version:\s+\d+", line)
        if version_match:
            require(active is not None, "Version requirement without a dependency")
            needs[active].append(version_match.group(1))
    require(len(need_counts) <= 1 and len(needs) == (int(need_counts[0]) if need_counts else 0)
            and all(len(versions) == expected_versions[name] for name, versions in needs.items()),
            "Incomplete version requirement table")
    return {"symbols": symbols, "version_definitions": definitions, "version_requirements": needs}


def exported(symbol: dict) -> bool:
    return (symbol["index"] != "UND" and symbol["binding"] in ("GLOBAL", "WEAK", "UNIQUE")
            and symbol["visibility"] in ("DEFAULT", "PROTECTED"))


def admit_symbols(closure: dict, resolve_needed, probe: Path, sdk: Path) -> dict:
    """Check the selected ABI and all VERNEEDs passively, before --identity loads it."""
    versions = []
    providers = {}
    for path, metadata in closure.items():
        for name in metadata["needed"]:
            dependency = str(resolve_needed(name))
            require(closure[dependency]["soname"] == [name], f"Unexpected SONAME: {path}: {name}")
        for name, required in metadata["version_requirements"].items():
            dependency = str(resolve_needed(name))
            require(dependency in closure, f"Version dependency outside closure: {path}: {name}")
            available = set(closure[dependency]["version_definitions"])
            require(set(required) <= available, f"Unsatisfied versions: {path}: {name}: {required}")
            versions.append({"consumer": path, "file": name, "provider": dependency, "versions": required})
        for symbol in metadata["symbols"]:
            if exported(symbol):
                providers.setdefault(symbol["name"], []).append((path, symbol))
    drogon = str((sdk / "lib/libdrogon.so.1.9.14").resolve(strict=True))
    trantor = str((sdk / "lib/libtrantor.so.1.5.29").resolve(strict=True))
    resolutions = {}
    for path in (str(probe), drogon, trantor):
        resolutions[path] = []
        for symbol in closure[path]["symbols"]:
            if symbol["index"] != "UND" or symbol["binding"] == "WEAK":
                continue
            matches = [provider for provider, definition in providers.get(symbol["name"], [])
                       if (definition["version"] == symbol["version"] if symbol["version"] else
                           definition["version"] is None or definition["default_version"])]
            require(bool(matches), f"Unresolved strong symbol: {path}: {symbol}")
            resolutions[path].append({"name": symbol["name"], "version": symbol["version"],
                                      "providers": sorted(set(matches))})
    # Itanium mangled names bind the exact selected C++ signatures, including the
    # final listener bool (eight arguments) and the shared_ptr NativeProfileIo seam.
    required_exports = {
        drogon: ["_ZN6drogon20HttpAppFrameworkImpl11addListenerERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEtbS8_S8_bRKSt6vectorISt4pairIS6_S6_ESaISB_EEb",
                 "_ZN6drogon14native_profile7Runtime7factoryEv",
                 "_ZN6drogon14native_profile7Runtime7startedEv"],
        trantor: ["_ZN7trantor17TcpConnectionImpl22establishNativeProfileEv",
                  "_ZN7trantor17TcpConnectionImpl18setNativeProfileIoESt10shared_ptrINS_15NativeProfileIoEE",
                  "_ZN7trantor17TcpConnectionImpl25requestNativeProfileWriteEv",
                  "_ZN7trantor17TcpConnectionImpl25retireNativeProfileSocketEv"],
    }
    for path, names in required_exports.items():
        available = {s["name"] for s in closure[path]["symbols"]
                     if exported(s) and s["visibility"] == "DEFAULT" and s["type"] == "FUNC"}
        require(set(names) <= available, f"Missing patched API exports: {path}: {set(names) - available}")
    rtti = "_ZTIN6drogon14native_profile7SessionE"
    for path in (str(probe), drogon):
        require(any(s["name"] == rtti and exported(s) and s["visibility"] == "DEFAULT"
                    and s["type"] == "OBJECT" for s in closure[path]["symbols"]),
                f"Missing defined default-visible Session RTTI: {path}")
    return {"version_checks": versions, "strong_symbol_resolutions": resolutions,
            "patched_exports": required_exports, "session_rtti": {"symbol": rtti, "objects": [str(probe), drogon]}}


def copy_notice(source: Path, destination: Path, expected: str | None = None) -> str:
    require(source.is_file(), f"Missing notice: {source}")
    value = digest(source)
    require(expected is None or value == expected, f"Notice hash mismatch: {source}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)
    require(digest(destination) == value, f"Notice copy mismatch: {destination}")
    return value


def binary_directory() -> Path:
    return BINARY_DIR if BINARY_DIR is not None else REPO / "build" / PRESET


def local_packages(sdk: Path, build: Path) -> dict:
    """Actual Arch package identities/notices for local development, never Ubuntu acceptance."""
    require(SUCCESSOR, "Local development is successor-only")
    packages = ("glibc", "libgcc", "libstdc++", "gcc", "openssl", "jsoncpp", "zlib", "util-linux-libs")
    records = {}
    for name in packages:
        record = {"installed": run("pacman", "-Q", name), "notices": {}}
        directory = Path("/usr/share/licenses") / name
        if directory.is_dir():
            for path in sorted(directory.rglob("*")):
                if path.is_file():
                    relative = path.relative_to(directory)
                    record["notices"][str(relative)] = copy_notice(
                        path, sdk / "share/licenses/local" / name / relative)
        records[name] = record
    # Arch uses SPDX common-license metadata for these packages. Record that fact;
    # installed custom notices are copied above. This is not redistribution clearance.
    records["metadata"] = run("pacman", "-Qi", *packages)
    return records


def prepare(root: Path) -> None:
    global BUILD_TEMP
    manifest = profile()  # Verify first-party/patch bytes before downloading sources.
    require(not root.exists(), f"Fresh disposable root required: {root}")
    root.mkdir()
    if SUCCESSOR:
        BUILD_TEMP = root / "compiler-tmp"
        BUILD_TEMP.mkdir(mode=0o700)
    source, build, sdk = (root / name for name in ("source", "build", "sdk"))
    receipts = root / "receipts"
    receipts.mkdir()
    if SOURCE_SEED is None:
        run("git", "clone", "--branch", f"v{setup_drogon.VERSION}", "--single-branch",
            "--depth", "1", setup_drogon.REPOSITORY, source)
    else:
        # Clone objects, not the seed worktree's patched bytes; never hardlink or write there.
        run("git", "clone", "--no-hardlinks", SOURCE_SEED, source)
        run("git", "-C", source, "checkout", "--detach", setup_drogon.DROGON_COMMIT)
    setup_drogon.verify_source(source, setup_drogon.DROGON_COMMIT)
    if SOURCE_SEED is None:
        run("git", "-C", source, "submodule", "update", "--init", "--depth", "1", "--", "trantor")
    else:
        run("git", "clone", "--no-hardlinks", SOURCE_SEED / "trantor", source / "trantor")
        run("git", "-C", source / "trantor", "checkout", "--detach", setup_drogon.TRANTOR_COMMIT)
    trantor = source / "trantor"
    setup_drogon.verify_source(trantor, setup_drogon.TRANTOR_COMMIT)
    setup_drogon.verify_source(source, setup_drogon.DROGON_COMMIT)
    for directory in (source / ".git", source / ".git/modules/trantor"):
        require(not (directory / "objects/info/alternates").exists(), "Source object alternates are not admitted")
    pristine = {name: run("git", "-C", directory, "status", "--porcelain=v1",
                          "--untracked-files=all", "--ignored")
                for name, directory in (("drogon", source), ("trantor", trantor))}
    require(not any(pristine.values()), "Non-pristine input, including ignored files")
    directories = {"drogon": source, "trantor": trantor}
    parent = native_profile()
    for patch in parent["ordered_patches"]:
        run("git", "-C", directories[patch["apply_at"]], "apply", "--check", PARENT_PROFILE / patch["file"])
    for patch in parent["ordered_patches"]:
        run("git", "-C", directories[patch["apply_at"]], "apply", PARENT_PROFILE / patch["file"])
    if SUCCESSOR:
        first = root / "first-party"
        origin = parent["first_party"]["file"]
        copy_notice(REPO / origin, first / origin, parent["first_party"]["sha256"])
        directories["first_party"] = first
        for patch in manifest["ordered_patches"]:
            run("git", "-C", directories[patch["apply_at"]], "apply", "--check", PROFILE / patch["file"])
            run("git", "-C", directories[patch["apply_at"]], "apply", PROFILE / patch["file"])
        require(digest(first / origin) == manifest["first_party"]["derived_sha256"],
                "Native build-copy digest mismatch")
        # First-party source is compiled by the consumer, never inside either SDK DSO.
        copy_notice(first / origin, sdk / PROVENANCE / "native_websocket_profile.cpp",
                    manifest["first_party"]["derived_sha256"])
        del directories["first_party"]
    provenance = sdk / PROVENANCE
    provenance.mkdir(parents=True, exist_ok=True)
    sources = {}
    for name, directory in directories.items():
        (receipts / f"{name}.diff").write_text(
            run("git", "-C", directory, "diff", "--binary"), encoding="utf-8")
        (receipts / f"{name}.status").write_text(
            run("git", "-C", directory, "status", "--porcelain=v1"), encoding="utf-8")
        # NUL-delimited names retain non-ASCII upstream files; never parse Git's quoted display.
        names = run("git", "-C", directory, "ls-files", "-z", echo=False).split("\0")
        names += run("git", "-C", directory, "ls-files", "--others", "--exclude-standard", "-z", echo=False).split("\0")
        for relative in filter(None, names):
            path = directory / relative
            if path.is_file():  # The Trantor gitlink is walked separately.
                sources[f"{name}/{relative}"] = digest(path)
    write_json(provenance / "patched-source-sha256.json", sources)
    configure = ["cmake", "-S", source, "-B", build, "-G", "Ninja",
                 f"-DCMAKE_INSTALL_PREFIX={sdk}"]
    configure.extend(f"-D{key}={value}" for key, value in setup_drogon.OPTIONS.items())
    run(*configure)
    run("cmake", "--build", build, "--parallel", "2")
    run("cmake", "--install", build)
    commands = run("ninja", "-C", build, "-t", "commands", "drogon")
    standards = {"drogon": set(), "trantor": set()}
    for line in commands.splitlines():
        args = shlex.split(line)
        if "-c" not in args:
            continue
        unit = Path(args[args.index("-c") + 1])
        require(unit.is_relative_to(source), f"Unexpected compilation unit: {unit}")
        owner = "trantor" if unit.is_relative_to(trantor) else "drogon"
        standards[owner].add(tuple(arg for arg in args if arg.startswith("-std=")))
    require(standards == {"drogon": {("-std=c++20",)},
                          "trantor": {("-std=c++20" if SUCCESSOR else "-std=c++14",)}},
            f"Unexpected effective SDK language flags: {standards}")
    (provenance / "ninja-commands.txt").write_text(commands, encoding="utf-8")
    shutil.copyfile(build / "CMakeCache.txt", provenance / "CMakeCache.txt")
    for name in ("drogon", "trantor"):
        for suffix in ("diff", "status"):
            shutil.copyfile(receipts / f"{name}.{suffix}", provenance / f"{name}.{suffix}")
    for name, origin in (("share/licenses/drogon/LICENSE", source / "LICENSE"),
                         ("share/licenses/drogon/orm_lib-COPYING", source / "orm_lib/COPYING"),
                         ("share/licenses/trantor/License", trantor / "License")):
        copy_notice(origin, sdk / name)
    for name in ("COPYING", "LICENSE.md", "THIRD_PARTY_NOTICES.md"):
        copy_notice(REPO / name, provenance / name)
    for name in ("manifest.json", *[p["file"] for p in manifest["ordered_patches"]]):
        copy_notice(PROFILE / name, provenance / name)
    if SUCCESSOR:
        for name in ("manifest.json", *[p["file"] for p in parent["ordered_patches"]]):
            copy_notice(PARENT_PROFILE / name, provenance / "parent" / name)
    for name in (manifest["first_party"]["file"], "server/include/subathon/server/native_websocket_profile.hpp"):
        copy_notice(REPO / name, provenance / name)
    if LOCAL_DEVELOPMENT:
        package_records = local_packages(sdk, build)
        compiler_paths = re.findall(r"^CMAKE_CXX_COMPILER:FILEPATH=(.+)$",
                                   (build / "CMakeCache.txt").read_text(), re.MULTILINE)
        require(len(compiler_paths) == 1, "Expected one actual CMake C++ compiler")
        compiler = Path(compiler_paths[0]).resolve(strict=True)
    else:
        packages = json.loads((REPO / "tools/native_websocket_ubuntu.json").read_text())["packages"]
        require(len(packages) == 8, "Expected eight admitted Ubuntu packages")
        package_records = {}
        for item in packages:
            installed = run("dpkg-query", "--show", "--showformat=${Status}\n${Version}\n${Architecture}",
                            item["name"]).splitlines()
            require(installed == ["install ok installed", item["version"], item["architecture"]],
                    f"Changed package: {item['name']}")
            copy_notice(Path(item["copyright_path"]), sdk / "share/licenses/ubuntu" / item["name"] / "copyright",
                        item["copyright_sha256"])
            package_records[item["name"]] = item
        # Record host identities, not pins or a redistribution certification. Preserve complete notices.
        compiler_paths = re.findall(r"^CMAKE_CXX_COMPILER:FILEPATH=(.+)$",
                                   (build / "CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE)
        require(len(compiler_paths) == 1, "Expected one actual CMake C++ compiler")
        compiler = Path(compiler_paths[0]).resolve(strict=True)
        compiler_owner = run("dpkg-query", "--search", compiler).strip().split(": ")
        require(len(compiler_owner) == 2 and compiler_owner[1] == str(compiler),
                "Actual compiler must have one installed Ubuntu package owner")
        for name in dict.fromkeys(("libc6", "libc6-dev", "libgcc-s1", "libstdc++6", "gcc", "g++",
                                   compiler_owner[0])):
            notice_name = name.split(":")[0]  # dpkg owner may include an architecture qualifier.
            package_records[name] = {"installed": run(
                "dpkg-query", "--show", "--showformat=${binary:Package}\n${Version}\n${Architecture}\n${Status}", name),
                "copyright_sha256": copy_notice(Path("/usr/share/doc") / notice_name / "copyright",
                                                 sdk / "share/licenses/ubuntu" / name / "copyright")}
        write_json(provenance / "ubuntu-packages.json", package_records)
    toolchain = {"scope": "local development only" if LOCAL_DEVELOPMENT else "Ubuntu CI on reviewed head required for acceptance",
                 "upstreams": manifest["upstreams"], "options": setup_drogon.OPTIONS,
                 "pristine_status_before_patches": pristine,
                 "compiler_path": str(compiler),
                 "compiler_tmpdir": str(BUILD_TEMP) if BUILD_TEMP is not None else "inherited",
                 "compiler": run(compiler, "--version"), "cmake": run("cmake", "--version"),
                 "os": run("uname", "-srmo"), "packages": package_records}
    for name in ("libdrogon.so.1.9.14", "libtrantor.so.1.5.29"):
        elf(sdk / "lib" / name, provenance / "elf")
    write_json(provenance / "recipe-commands.json", COMMANDS)
    require(not (sdk / "dependency-build.json").exists() and not (sdk / "dependency-build.json").is_symlink(),
            "Refusing to replace a pre-existing SDK identity")
    identity = {"format": 1, "profile_manifest_sha256": digest(PROFILE / "manifest.json"),
                "artifacts": inventory(sdk, omit_identity=True), "toolchain": toolchain}
    require(digest(provenance / "manifest.json") == identity["profile_manifest_sha256"],
            "Copied manifest mismatch")
    write_json(sdk / "dependency-build.json", identity)  # Final SDK write, before consumer configure.
    write_json(receipts / "sdk-seal.json", {"identity": file_identity(sdk / "dependency-build.json"),
                                          "artifacts": identity["artifacts"]})
    print(json.dumps(identity, indent=2, sort_keys=True))


def checked_seal(sdk: Path) -> dict:
    profile()
    identity = json.loads((sdk / "dependency-build.json").read_text(encoding="utf-8"))
    require(set(identity) == {"format", "profile_manifest_sha256", "artifacts", "toolchain"} and
            type(identity["format"]) is int and identity["format"] == 1 and
            isinstance(identity["toolchain"], dict), "Bad SDK seal schema")
    require(identity["profile_manifest_sha256"] == digest(PROFILE / "manifest.json") ==
            digest(sdk / PROVENANCE / "manifest.json"), "SDK profile drift")
    require(identity["artifacts"] == inventory(sdk, omit_identity=True), "SDK changed after sealing")
    return file_identity(sdk / "dependency-build.json")


def configured_ctest() -> Path:
    cache = (binary_directory() / "CMakeCache.txt").read_text(encoding="utf-8")
    values = re.findall(r"^CMAKE_CTEST_COMMAND:[^=\n]+=(.+)$", cache, re.MULTILINE)
    require(len(values) == 1 and Path(values[0]).is_absolute() and "\n" not in values[0],
            "Expected one absolute configured CTest executable")
    command = Path(values[0]).resolve(strict=True)
    require(command.is_file() and os.access(command, os.X_OK), "Configured CTest is not executable")
    return command


def passive_inventory(environment: dict, ctest: Path) -> dict:
    driver = REPO / "tests/native_websocket_profile/test_native_ws_profile.py"
    tree = ast.parse(driver.read_text(encoding="utf-8"))
    owner, = [n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == "NativeProfile"]
    expected = [f"NativeProfile.{n.name}" for n in owner.body
                if isinstance(n, ast.FunctionDef) and n.name.startswith("test_")]
    require(len(expected) == len(set(expected)) == 44, "Expected exactly 44 authored native methods")
    found = run("/usr/bin/python3", driver, "--list", env=environment).splitlines()
    require(sorted(found) == sorted(expected), "Passive inventory differs from authored 44 methods")
    discovery = json.loads(run(ctest, "--test-dir", binary_directory(), "--show-only=json-v1", env=environment))
    native = [t for t in discovery["tests"] if t["name"].startswith("native-websocket-profile") or
              any(p["name"] == "LABELS" and "native-websocket-profile" in p["value"]
                  for p in t["properties"])]
    require(len(native) == 1 and native[0]["name"] == "native-websocket-profile.suite",
            "Expected one native owning suite")
    props = {p["name"]: p["value"] for p in native[0]["properties"]}
    require(set(props.get("LABELS", [])) == {"native-websocket-profile", "acceptance", "server", "loopback"}
            and not props.get("DISABLED", False) and not props.get("WILL_FAIL", False)
            and props.get("TIMEOUT") == 3300, "Native suite must be enabled required-success with 3300s bound")
    return {"methods": sorted(found), "ctest": native[0]}


def foundation_identity(root: Path, probe: Path, environment: dict, expected: dict,
                        seal: dict, discovery: dict, ctest_identity: dict, abi: dict) -> None:
    """Technical development gate; the coordinator ruling is not a binary review receipt."""
    sdk = root / "sdk"
    # Some glibc versions retain the loader cache mapping. It is data, not ELF:
    # bind its bytes before load and admit no other non-ELF file-backed mapping.
    elf_expected = expected
    expected = dict(expected)
    loader_cache = Path("/etc/ld.so.cache")
    if loader_cache.exists():
        require(loader_cache.is_file() and not loader_cache.is_symlink(), "Unexpected loader cache type")
        expected[str(loader_cache)] = file_identity(loader_cache)
    receipts = root / "receipts/foundation"
    receipts.mkdir(parents=True, exist_ok=True)
    scratch = receipts / "identity-cwd"
    require(not scratch.exists(), "Fresh foundation identity cwd required")
    scratch.mkdir(mode=0o700)
    try:
        result = subprocess.run([str(probe), '{"mode":"identity"}'], cwd=scratch,
                                env=environment, capture_output=True, text=True, timeout=15)
        (receipts / "identity.stdout").write_text(result.stdout, encoding="utf-8")
        (receipts / "identity.stderr").write_text(result.stderr, encoding="utf-8")
        require(result.returncode == 0 and not result.stderr and
                len(result.stdout.encode()) <= 2097152, "Foundation identity capture/exit fault")
        events = [json.loads(line) for line in result.stdout.splitlines()]
        identities = [e for e in events if e.get("event") == "identity"]
        require(len(identities) == 1 and identities[0].get("cplusplus") == 202002 and
                identities[0].get("native_backend") == "admitted_corrected" and
                0 < identities[0].get("read_sample_size", 0) <= 64,
                "Wrong foundation identity/ABI")
        require(all(e.get("event") in ("map", "maps_end", "identity") for e in events),
                "Unexpected foundation identity event")
        for phase in ("initial", "cleanup"):
            rows = [e["line"] for e in events if e.get("event") == "map" and e.get("phase") == phase]
            ends = [e for e in events if e.get("event") == "maps_end" and e.get("phase") == phase]
            require(len(ends) == 1 and 0 < len(rows) == ends[0]["count"] <= 256,
                    "Incomplete foundation identity maps")
            seen = set()
            for row in rows:
                fields = row.split(maxsplit=5)
                require(len(fields) >= 5, "Malformed foundation map")
                if len(fields) == 5 or fields[5].startswith("["):
                    continue
                path = fields[5]
                require(Path(path).is_absolute() and " (deleted)" not in path and path in expected,
                        "Unresolved/unexpected foundation mapping")
                require(file_identity(Path(path)) == expected[path], "Changed foundation mapped file")
                seen.add(path)
            require(set(elf_expected) <= seen <= set(expected),
                    "Foundation loaded map differs from passive ELF/data closure")
        write_json(receipts / "identity.json", events)
    finally:
        require(not any(scratch.iterdir()), "Unexpected identity scratch output")
        scratch.rmdir()
    require(checked_seal(sdk) == seal and
            all(file_identity(Path(p)) == value for p, value in expected.items()),
            "Foundation inputs changed during identity")
    tree = ast.parse((REPO / "tests/bounded_http_foundation/test_bounded_http.py").read_text())
    source_paths, = [ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and
                    any(isinstance(t, ast.Name) and t.id == "SOURCE_PATHS" for t in n.targets)]
    require(len(source_paths) == len(set(source_paths)) == 13, "Foundation source inventory drift")
    document = {"source_commit": run("git", "rev-parse", "HEAD").strip(),
                "review_receipt": RULING,
                "files": {p: value["sha256"] for p, value in expected.items()},
                "sources": {p: digest(REPO / p) for p in source_paths}, "environment": environment}
    write_json(receipts / "development-admission.json", document)
    write_json(receipts / "technical-gate.json", {
        "scope": "development only; no binary admitted by review",
        "coordinator_ruling": RULING, "sdk_seal": seal, "expected": expected,
        "inventory": discovery, "ctest": ctest_identity, "passive_abi": abi,
        "source_commit": document["source_commit"], "sources": document["sources"]})
    print("Foundation technical gate passed; development execution permitted")


def gate(root: Path) -> None:
    sdk = root / "sdk"
    environment = clean_env(sdk)
    receipts = root / "receipts" / CONSUMER if SUCCESSOR else root / "receipts"
    seal = checked_seal(sdk)
    ctest = configured_ctest()
    ctest_identity = {"path": str(ctest), **file_identity(ctest)}
    discovery = passive_inventory(environment, ctest)  # No probe/feature body.
    probe = (binary_directory() / ("tests/native_websocket_profile/native_websocket_probe"
             if CONSUMER == "native" else "tests/bounded_http_foundation/bounded_http_probe")).resolve(strict=True)
    require(not Path("/etc/ld.so.preload").exists(), "Unadmitted system-wide preload configuration")
    allowed_dirs = {d.resolve(strict=True) for d in (sdk / "lib", *SYSTEM_LIB_DIRS)}

    def resolve_needed(name: str) -> Path:
        require("/" not in name and name not in ("", ".", ".."), f"Nonselected NEEDED: {name}")
        local = sdk / "lib" / name
        if local.exists() or local.is_symlink():
            resolved = local.resolve(strict=True)
            require(resolved.is_relative_to(sdk), f"SDK alias escape: {local}")
            return resolved
        candidates = {p.resolve(strict=True) for d in SYSTEM_LIB_DIRS
                      if (p := d / name).exists() or p.is_symlink()}
        require(len(candidates) == 1, f"Unresolved/ambiguous host dependency: {name}: {candidates}")
        selected, = candidates
        require(selected.parent in allowed_dirs, f"Host dependency alias escape: {selected}")
        return selected

    closure = {}
    pending = [probe]
    while pending:
        path = pending.pop(0)
        if str(path) in closure:
            continue
        require(path.is_file(), f"Missing ELF file: {path}")
        metadata = elf(path, receipts / "elf")
        for runpath in metadata["runpath"]:
            for entry in runpath.split(":"):
                expanded = entry.replace("${ORIGIN}", str(path.parent)).replace("$ORIGIN", str(path.parent))
                require(Path(expanded).is_absolute() and Path(expanded).resolve(strict=True) in allowed_dirs,
                        f"Unadmitted RUNPATH: {path}: {entry}")
        if path == probe:
            require(metadata["interpreter"] == ["/lib64/ld-linux-x86-64.so.2"], "Wrong ELF interpreter")
        # libc may itself carry PT_INTERP; it must name the same admitted loader.
        if metadata["interpreter"]:
            selected_interpreter = metadata["interpreter"] == ["/lib64/ld-linux-x86-64.so.2"]
            if LOCAL_DEVELOPMENT and len(metadata["interpreter"]) == 1:
                selected_interpreter = (Path(metadata["interpreter"][0]).resolve(strict=True) ==
                                        Path("/lib64/ld-linux-x86-64.so.2").resolve(strict=True))
            require(selected_interpreter, f"Unadmitted ELF interpreter: {path}")
            interpreter = Path(metadata["interpreter"][0]).resolve(strict=True)
            require(interpreter.parent in allowed_dirs, "ELF interpreter alias escapes selected system directories")
            pending.append(interpreter)
        closure[str(path)] = metadata
        for name in metadata["needed"]:
            dependency = resolve_needed(name)
            pending.append(dependency)
    require(all(str((sdk / "lib" / name).resolve(strict=True)) in closure
                for name in ("libdrogon.so.1.9.14", "libtrantor.so.1.5.29")),
            "Consumer closure must include both corrected SDK libraries")
    write_json(receipts / "prospective-closure.json", closure)  # BEFORE any selected native load.
    abi = admit_symbols(closure, resolve_needed, probe, sdk)
    write_json(receipts / "passive-abi.json", abi)
    expected = {p: {key: item[key] for key in ("sha256", "bytes")} for p, item in closure.items()}
    require(checked_seal(sdk) == seal and all(file_identity(Path(p)) == item for p, item in expected.items()),
            "Inputs changed before identity")
    if SUCCESSOR:
        require(any(s["name"] == "DROGON_SUBATHON_BOUNDED_HTTP_V1" and exported(s)
                    for s in closure[str((sdk / "lib/libdrogon.so.1.9.14").resolve())]["symbols"]),
                "Missing successor ABI marker")
    # Identity has no listener. Actual loaded maps must match the passive closure.
    if CONSUMER == "foundation":
        foundation_identity(root, probe, environment, expected, seal, discovery, ctest_identity, abi)
        return
    result = subprocess.run([str(probe), "--identity"], cwd=REPO, env=environment,
                            capture_output=True, text=True, encoding="utf-8", timeout=15)
    (receipts / "identity.stdout").write_text(result.stdout, encoding="utf-8")
    (receipts / "identity.stderr").write_text(result.stderr, encoding="utf-8")
    require(result.returncode == 0 and not result.stderr and len(result.stdout.encode()) <= 65536,
            "Identity setup/capture/exit/stderr fault")
    lines = result.stdout.splitlines()
    require(len(lines) == 1, "Identity must emit exactly one event, no fixture/SDK faults")
    identity = json.loads(lines[0])
    require(identity.get("event") == "identity" and identity.get("backend") == "admitted_corrected" and
            identity.get("complete") is True and identity.get("incomplete_reason") == "none" and
            identity.get("consumer_cplusplus") == 202002, "Incomplete/wrong backend or consumer identity")
    actual = {}
    virtual = []
    main = []
    for item in identity["loaded_objects"]:
        kind, raw = item["kind"], item["path"]
        if kind == "virtual":
            virtual.append(raw)
            continue
        require(kind in ("main", "file") and Path(raw).is_absolute(), f"Unresolved loaded object: {item}")
        path = Path(raw).resolve(strict=True)
        require(str(path) not in actual, f"Duplicate loaded object: {path}")
        actual[str(path)] = file_identity(path)
        if kind == "main":
            main.append(path)
    require(main == [probe] and virtual == ["linux-vdso.so.1"] and actual == expected,
            "Loaded map differs from prospective main + vDSO + complete host-specific ELF closure")
    require(checked_seal(sdk) == seal, "SDK changed during identity")
    checked = {"identity": identity, "expected": expected, "actual": actual, "exact_match": True,
               "sdk_seal": seal, "inventory": discovery, "environment": environment,
               "ctest": ctest_identity, "passive_abi": "passive-abi.json"}
    write_json(receipts / "checked-identity.json", checked)
    require(file_identity(ctest) == {k: ctest_identity[k] for k in ("sha256", "bytes")},
            "Configured CTest changed during the gate")
    (receipts / "ctest-command.txt").write_text(str(ctest) + "\n", encoding="utf-8")
    print(json.dumps(checked, indent=2, sort_keys=True))


def main() -> None:
    global SUCCESSOR, PROFILE, PROVENANCE, BINARY_DIR, SOURCE_SEED, SYSTEM_LIB_DIRS
    global LOCAL_DEVELOPMENT, CONSUMER
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", choices=("prepare", "gate"))
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--profile", choices=("native", "bounded-http-foundation"), default="native")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--source-seed", type=Path)
    parser.add_argument("--local-development", action="store_true")
    parser.add_argument("--consumer", choices=("native", "foundation"), default="native")
    args = parser.parse_args()
    SUCCESSOR = args.profile == "bounded-http-foundation"
    LOCAL_DEVELOPMENT = args.local_development
    CONSUMER = args.consumer
    require(SUCCESSOR or not (LOCAL_DEVELOPMENT or args.source_seed or CONSUMER != "native"),
            "Development/source-seed/foundation options are successor-only")
    if SUCCESSOR:
        PROFILE = REPO / "dependencies/bounded-http-foundation"
        PROVENANCE = "share/subathon-bounded-http-foundation"
    if LOCAL_DEVELOPMENT:
        SYSTEM_LIB_DIRS = (Path("/usr/lib"), Path("/lib"))
    BINARY_DIR = args.build_dir.resolve() if args.build_dir else None
    SOURCE_SEED = args.source_seed.resolve(strict=True) if args.source_seed else None
    root = args.root.resolve()
    require(root.is_relative_to(Path(os.environ["RUNNER_TEMP"]).resolve()) and
            root != Path(os.environ["RUNNER_TEMP"]).resolve(), "Use a disposable RUNNER_TEMP child")
    if args.stage == "prepare":
        prepare(root)
    else:
        gate(root)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.SubprocessError) as error:
        sys.exit(f"Native WebSocket CI failed: {error}")
