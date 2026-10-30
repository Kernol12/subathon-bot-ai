#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Explicitly build the pinned SPIKE-001 dependencies in an ignored local directory."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

VERSION = "1.9.14"
DROGON_COMMIT = "3aae769cc405cf5abb9d477675c6430924f02c7c"
TRANTOR_COMMIT = "78e480f5e138acdc05f76c55a2716718cd846364"
REPOSITORY = "https://github.com/drogonframework/drogon.git"
OPTIONS = {
    "CMAKE_BUILD_TYPE": "Release",
    "CMAKE_CXX_STANDARD": "20",
    "CMAKE_CXX_STANDARD_REQUIRED": "ON",
    "CMAKE_INSTALL_LIBDIR": "lib",
    "BUILD_SHARED_LIBS": "ON",
    "USE_SUBMODULE": "ON",
    "BUILD_ORM": "OFF",
    "BUILD_CTL": "OFF",
    "BUILD_EXAMPLES": "OFF",
    "BUILD_TESTING": "OFF",
    "BUILD_DOC": "OFF",
    "BUILD_BROTLI": "OFF",
    "BUILD_YAML_CONFIG": "OFF",
    "BUILD_C-ARES": "OFF",
    "USE_SPDLOG": "OFF",
    "TRANTOR_USE_TLS": "openssl",
}


def run(*args: str, capture: bool = False) -> str:
    """Use argument arrays so dependency paths never become shell code."""
    if not capture:
        print("+", " ".join(args), flush=True)
    result = subprocess.run(args, check=True, text=True,
                            stdout=subprocess.PIPE if capture else None)
    return result.stdout.strip() if capture else ""


def verify_source(directory: Path, expected: str) -> None:
    if not (directory / ".git").exists():
        raise ValueError(f"Not a Git checkout: {directory}")
    if run("git", "-C", str(directory), "rev-parse", "HEAD", capture=True) != expected:
        raise ValueError(f"Unexpected source revision in {directory}; refusing to overwrite it")
    if run("git", "-C", str(directory), "status", "--porcelain", capture=True):
        raise ValueError(f"Modified or untracked source in {directory}; preserve/review it first")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path,
                        default=Path(__file__).resolve().parents[1] / "build/deps",
                        help="Dependency directory; pass the primary checkout's build/deps to share across worktrees")
    parser.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 1),
                        help="Parallel compilation jobs (default: at most 4)")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    root = args.root.expanduser().resolve()
    source = root / f"drogon-{VERSION}-source"
    build = root / f"drogon-{VERSION}-build"
    prefix = root / f"drogon-{VERSION}"
    manifest_path = prefix / "dependency-build.json"
    identity = {"drogon": DROGON_COMMIT, "trantor": TRANTOR_COMMIT, "options": OPTIONS}
    if prefix.exists() and any(prefix.iterdir()):
        if not manifest_path.is_file() or json.loads(manifest_path.read_text()) != identity:
            raise ValueError(f"Unrecognized install at {prefix}; refusing to overwrite it")
    root.mkdir(parents=True, exist_ok=True)
    if not source.exists():
        run("git", "clone", "--branch", f"v{VERSION}", "--single-branch", "--depth", "1",
            REPOSITORY, str(source))
    verify_source(source, DROGON_COMMIT)
    trantor = source / "trantor"
    if not (trantor / ".git").exists():
        run("git", "-C", str(source), "submodule", "update", "--init", "--depth", "1", "--", "trantor")
    verify_source(trantor, TRANTOR_COMMIT)
    verify_source(source, DROGON_COMMIT)
    configure = ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
                 f"-DCMAKE_INSTALL_PREFIX={prefix}"]
    configure.extend(f"-D{key}={value}" for key, value in OPTIONS.items())
    run(*configure)
    run("cmake", "--build", str(build), "--parallel", str(args.jobs))
    run("cmake", "--install", str(build))
    for name, directory in (("drogon", source), ("trantor", trantor)):
        notices = prefix / "share/licenses" / name
        notices.mkdir(parents=True, exist_ok=True)
        license_file = "LICENSE" if name == "drogon" else "License"
        shutil.copy2(directory / license_file, notices / license_file)
    manifest_path.write_text(json.dumps(identity, indent=2) + "\n")
    print(f"Local CMake prefix: {prefix}")
    print("Pass this directory through CMAKE_PREFIX_PATH; no global paths were changed.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Drogon setup failed: {error}", file=sys.stderr)
        sys.exit(1)
