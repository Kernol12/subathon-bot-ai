# SPDX-License-Identifier: GPL-3.0-or-later
"""CI gate: every test program's loaded-library closure equals its committed list.

ADR-0013. For each program a CTest tree registers (the command and any ELF argument
inside the build tree), ask the system loader which libraries it resolves (ldd:
the DT_NEEDED closure with RUNPATH, ld.so.cache and the test's ENVIRONMENT) and
compare "soname => path" with the committed list. Any difference, unresolved
library, unlisted program or unsupported registration fails the job.

Usage in CI, after the tree is built and before its first ctest run:

    python3 tools/binary_closure.py --expected tools/binary_closures/qt.json \
        --build-dir build/sqlite-reference-capture-debug --root QT_PREFIX="$QT_PREFIX"

On failure the observed file for that tree is printed; a reviewed PR may commit it.
Dynamically loaded Qt plugins are outside DT_NEEDED and are not covered here.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from dataclasses import dataclass, field
from typing import Callable

NOT_ELF = "not-elf"  # profile for a registered script or other non-ELF command
_RESOLVED = re.compile(r"^(\S+) => (/\S+) \(0x[0-9a-f]+\)$")
_DIRECT = re.compile(r"^(/\S+) \(0x[0-9a-f]+\)$")


class ClosureError(Exception):
    """A registration or loader answer the gate cannot judge; the job fails."""


def parse_ldd(text: str) -> dict[str, str]:
    """Map each needed name (or the interpreter path) to its resolved path."""
    libraries: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("linux-vdso.so"):
            continue
        if line in ("statically linked", "not a dynamic executable"):
            return {}
        if line.endswith("=> not found"):
            raise ClosureError(f"unresolved library: {line}")
        match = _RESOLVED.match(line) or _DIRECT.match(line)
        if match is None:
            raise ClosureError(f"unrecognised loader line: {line}")
        name = match.group(1)
        libraries[name] = match.group(2) if match.re is _RESOLVED else name
    return libraries


def portable(path: str, roots: dict[str, str]) -> str:
    """Replace the longest matching named root with $NAME so lists are runner-independent."""
    best = None
    for name, root in roots.items():
        root = root.rstrip("/")
        if (path == root or path.startswith(root + "/")) and (best is None or len(root) > len(best[1])):
            best = (name, root)
    return path if best is None else f"${best[0]}{path[len(best[1]):]}"


def run_ldd(program: str, environment: dict[str, str]) -> str:
    result = subprocess.run(["ldd", program], env=environment, capture_output=True, text=True)
    if result.returncode != 0 and "not a dynamic executable" not in result.stdout + result.stderr:
        raise ClosureError(f"ldd failed for {program}: {result.stderr.strip()}")
    return result.stdout or result.stderr


def _is_elf(path: Path) -> bool:
    with path.open("rb") as handle:
        return handle.read(4) == b"\x7fELF"


def _environment(test: dict) -> tuple[str, ...]:
    values: tuple[str, ...] = ()
    for prop in test.get("properties", []):
        if prop.get("name") == "ENVIRONMENT_MODIFICATION":
            raise ClosureError(f"{test.get('name')}: ENVIRONMENT_MODIFICATION is not supported")
        if prop.get("name") == "ENVIRONMENT":
            values = tuple(prop["value"])
    return values


def _programs(ctest: dict, build_dir: Path) -> dict[Path, set[tuple[str, ...]]]:
    """Every program a test starts: its command, plus ELF arguments inside the build tree."""
    programs: dict[Path, set[tuple[str, ...]]] = {}
    for test in ctest.get("tests", []):
        command = test.get("command")
        if not command:
            raise ClosureError(f"{test.get('name')}: registered without a command")
        environment = _environment(test)
        first = Path(command[0]).resolve()
        if not first.is_file():
            raise ClosureError(f"{test.get('name')}: program {first} does not exist")
        found = [first]
        for argument in command[1:]:
            path = Path(argument)
            if path.is_absolute() and path.is_file():
                path = path.resolve()
                if path.is_relative_to(build_dir) and _is_elf(path):
                    found.append(path)
        for program in found:
            programs.setdefault(program, set()).add(environment)
    return programs


@dataclass
class TreeResult:
    errors: list[str] = field(default_factory=list)
    observed: dict = field(default_factory=dict)
    digests: dict[str, str] = field(default_factory=dict)


def check_tree(ctest: dict, expected: dict, build_dir: Path, repo: Path,
               roots: dict[str, str], ldd: Callable[[str, dict[str, str]], str] = run_ldd) -> TreeResult:
    """Compare one CTest tree's programs with the expected file; never raises on a mere difference."""
    build_dir, repo = build_dir.resolve(), repo.resolve()
    result = TreeResult()
    closures: dict[str, list[str] | str] = {}
    for program, environments in sorted(_programs(ctest, build_dir).items()):
        key = str(program.relative_to(repo)) if program.is_relative_to(repo) else portable(str(program), roots)
        result.digests[key] = hashlib.sha256(program.read_bytes()).hexdigest()
        if not _is_elf(program):
            closures[key] = NOT_ELF
            continue
        seen = []
        for values in sorted(environments):
            environment = dict(os.environ)
            environment.update(value.split("=", 1) for value in values)
            libraries = parse_ldd(ldd(str(program), environment))
            seen.append(sorted(name if name == path else f"{name} => {portable(path, roots)}"
                               for name, path in libraries.items()))
        if any(item != seen[0] for item in seen):
            result.errors.append(f"{key}: closure differs between its test environments")
        closures[key] = seen[0]

    prefix = str(build_dir.relative_to(repo)) + "/" if build_dir.is_relative_to(repo) else str(build_dir) + "/"
    listed = {key: profile for key, profile in expected.get("programs", {}).items() if key.startswith(prefix)}
    profiles = expected.get("profiles", {})
    for key in sorted(listed.keys() - closures.keys()):
        result.errors.append(f"{key}: listed but not registered in this tree")
    for key, observed in closures.items():
        if key not in listed:
            result.errors.append(f"{key}: registered but not listed")
            continue
        name = listed[key]
        want = NOT_ELF if name == NOT_ELF else profiles.get(name)
        if want is None:
            result.errors.append(f"{key}: unknown profile {name!r}")
        elif observed != want:
            if isinstance(want, list) and isinstance(observed, list):
                extra = sorted(set(observed) - set(want))
                missing = sorted(set(want) - set(observed))
                result.errors.append(f"{key}: profile {name!r} differs; unexpected {extra}, missing {missing}")
            else:
                result.errors.append(f"{key}: expected {want!r}, observed {observed!r}")
    result.observed = observed_file(closures)
    return result


def observed_file(closures: dict[str, list[str] | str]) -> dict:
    """Group identical closures into numbered profiles, in the expected-file format."""
    profiles: dict[str, list[str]] = {}
    programs: dict[str, str] = {}
    for key in sorted(closures):
        closure = closures[key]
        if closure == NOT_ELF:
            programs[key] = NOT_ELF
            continue
        name = next((n for n, libs in profiles.items() if libs == closure), None)
        if name is None:
            name = f"closure-{len(profiles) + 1}"
            profiles[name] = list(closure)
        programs[key] = name
    return {"format": 1, "profiles": profiles, "programs": programs}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--expected", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--root", action="append", default=[], metavar="NAME=PATH",
                        help="named runner-specific root, written as $NAME in the lists")
    args = parser.parse_args(argv)
    roots = dict(item.split("=", 1) for item in args.root)
    roots = {name: str(Path(path).resolve()) for name, path in roots.items()}
    expected = json.loads(args.expected.read_text(encoding="utf-8"))
    if expected.get("format") != 1:
        raise ClosureError(f"{args.expected}: unsupported format")
    ctest = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))
    result = check_tree(ctest, expected, args.build_dir, Path.cwd(), roots)
    listed = expected.get("programs", {})
    for key, digest in sorted(result.digests.items()):
        print(f"{key} sha256={digest} profile={listed.get(key, 'unlisted')}")
    if result.errors:
        print("\n".join(f"error: {error}" for error in result.errors), file=sys.stderr)
        print("Observed closure file for this tree:", file=sys.stderr)
        print(json.dumps(result.observed, indent=2), file=sys.stderr)
        return 1
    print(f"{len(result.digests)} programs match {args.expected}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ClosureError as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)
