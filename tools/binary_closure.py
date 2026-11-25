# SPDX-License-Identifier: GPL-3.0-or-later
"""CI gate: every test program's loaded-library closure equals its committed list.

ADR-0013. The programs of one build tree are every command its CTest registration
starts and every ELF executable or shared object built in the tree (outside
CMakeFiles/), which covers children that tests launch by compiled-in path. For
each, ask the system loader which libraries it resolves (ldd: the DT_NEEDED closure
with RUNPATH, ld.so.cache and the test's ENVIRONMENT) and compare "soname => path"
with the tree's committed list. Any difference, unresolved library, loader
diagnostic, unlisted program or unsupported registration fails the job.

Usage in CI, after the tree is built and before its tests run (CTest's own test
discovery may list GoogleTest cases first):

    python3 tools/binary_closure.py \
        --expected tools/binary_closures/sqlite-reference-capture-debug.json \
        --build-dir build/sqlite-reference-capture-debug --root QT_PREFIX="$QT_PREFIX"

On failure (CI-004) the gate writes a `git apply` patch to --patch-out that adds or
re-points only the missing or changed entries, and prints the error lines followed by
that patch on stdout alone. CI uploads the patch as an artifact; the job still fails,
and a reviewed PR commits the patch. The check itself is unchanged.
Dynamically loaded Qt plugins are outside DT_NEEDED and are not covered here.
"""
from __future__ import annotations

import argparse
import difflib
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
    if "not a dynamic executable" in result.stdout + result.stderr:
        return "not a dynamic executable"
    if result.returncode != 0 or result.stderr.strip():
        raise ClosureError(f"ldd reported a problem for {program}: {result.stderr.strip()}")
    return result.stdout


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
            if any("=" not in value for value in values):
                raise ClosureError(f"{test.get('name')}: ENVIRONMENT entry without '='")
    return values


def _built(build_dir: Path) -> list[Path]:
    """ELF executables and shared objects built in the tree, outside CMake's probes."""
    found = []
    for path in build_dir.rglob("*"):
        if "CMakeFiles" in path.relative_to(build_dir).parts or path.is_symlink() or not path.is_file():
            continue
        if (os.access(path, os.X_OK) or ".so" in path.name) and _is_elf(path):
            found.append(path.resolve())
    return found


def _programs(ctest: dict, build_dir: Path) -> dict[Path, set[tuple[str, ...]]]:
    """Every program a test starts (command and ELF arguments) and every ELF built in the tree."""
    programs: dict[Path, set[tuple[str, ...]]] = {path: set() for path in _built(build_dir)}
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
    closures: dict[str, list[str] | str] = field(default_factory=dict)
    digests: dict[str, str] = field(default_factory=dict)


def check_tree(ctest: dict, expected: dict, build_dir: Path, repo: Path,
               roots: dict[str, str], ldd: Callable[[str, dict[str, str]], str] = run_ldd) -> TreeResult:
    """Compare one tree's programs with its expected file; never raises on a mere difference."""
    build_dir, repo = build_dir.resolve(), repo.resolve()
    result = TreeResult()
    closures = result.closures
    for program, environments in sorted(_programs(ctest, build_dir).items()):
        key = str(program.relative_to(repo)) if program.is_relative_to(repo) else portable(str(program), roots)
        result.digests[key] = hashlib.sha256(program.read_bytes()).hexdigest()
        if not _is_elf(program):
            closures[key] = NOT_ELF
            continue
        seen = []
        for values in sorted(environments or {()}):
            environment = dict(os.environ)
            environment.update(value.split("=", 1) for value in values)
            libraries = parse_ldd(ldd(str(program), environment))
            seen.append(sorted(name if name == path else f"{name} => {portable(path, roots)}"
                               for name, path in libraries.items()))
        if any(item != seen[0] for item in seen):
            result.errors.append(f"{key}: closure differs between its test environments")
        closures[key] = seen[0]

    listed = expected.get("programs", {})
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
        elif observed != (sorted(want) if isinstance(want, list) else want):
            if isinstance(want, list) and isinstance(observed, list):
                extra = sorted(set(observed) - set(want))
                missing = sorted(set(want) - set(observed))
                result.errors.append(f"{key}: profile {name!r} differs; unexpected {extra}, missing {missing}")
            else:
                result.errors.append(f"{key}: expected {want!r}, observed {observed!r}")
    return result


def fix_expected(expected: dict, closures: dict[str, list[str] | str]) -> dict:
    """The expected file changed only where the observed closures differ from it.

    A missing or mismatched program is pointed at an existing profile with the same
    closure, or at a new profile named after a digest of its contents; a listed but
    unregistered program is removed. Every other entry and every profile is kept.
    """
    profiles = {name: list(libs) for name, libs in expected.get("profiles", {}).items()}
    programs = dict(expected.get("programs", {}))
    for key in list(programs):
        if key not in closures:
            del programs[key]
    for key, closure in closures.items():
        current = programs.get(key)
        want = NOT_ELF if current == NOT_ELF else profiles.get(current)
        if want is not None and closure == (sorted(want) if isinstance(want, list) else want):
            continue
        if closure == NOT_ELF:
            programs[key] = NOT_ELF
            continue
        name = next((n for n, libs in profiles.items() if sorted(libs) == closure), None)
        if name is None:
            name = "observed-" + hashlib.sha256("\n".join(closure).encode()).hexdigest()[:12]
            profiles[name] = list(closure)
        programs[key] = name
    return {**expected, "profiles": profiles, "programs": dict(sorted(programs.items()))}


def render(expected: dict) -> str:
    """The canonical text of an expected file, as committed."""
    return json.dumps(expected, indent=2) + "\n"


def make_patch(path: str, old: str, new: str) -> str:
    """A unified diff of one file that `git apply` accepts from the repository root."""
    if old and not old.endswith("\n"):
        raise ClosureError(f"{path}: list file must end with a newline")
    hunks = difflib.unified_diff(old.splitlines(keepends=True), new.splitlines(keepends=True),
                                 f"a/{path}", f"b/{path}")
    return f"diff --git a/{path} b/{path}\n" + "".join(hunks)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--expected", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--root", action="append", default=[], metavar="NAME=PATH",
                        help="named runner-specific root, written as $NAME in the lists")
    parser.add_argument("--patch-out", type=Path,
                        help="on failure, write the list fix here as a git-apply patch")
    args = parser.parse_args(argv)
    roots = {}
    for item in args.root:
        name, _, path = item.partition("=")
        if not name or not path:
            raise ClosureError(f"--root {item!r} needs a name and a nonempty path")
        roots[name] = str(Path(path).resolve())
    text = args.expected.read_text(encoding="utf-8")
    try:
        expected = json.loads(text)
    except json.JSONDecodeError as error:
        raise ClosureError(f"{args.expected}: not valid JSON ({error})") from error
    if not isinstance(expected, dict) or expected.get("format") != 1:
        raise ClosureError(f"{args.expected}: unsupported format")
    repo = Path.cwd().resolve()
    list_path = args.expected.resolve()
    if not list_path.is_relative_to(repo):
        raise ClosureError(f"{args.expected}: must be inside the repository")
    ctest = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(args.build_dir), "--show-only=json-v1"], text=True))
    result = check_tree(ctest, expected, args.build_dir, repo, roots, run_ldd)
    if not result.errors:
        listed = expected.get("programs", {})
        for key, digest in sorted(result.digests.items()):
            print(f"{key} sha256={digest} profile={listed[key]}")
        print(f"{len(result.digests)} programs match {args.expected}")
        return 0
    output = "".join(f"error: {error}\n" for error in result.errors)
    fixed = render(fix_expected(expected, result.closures))
    if fixed != text:
        patch_text = make_patch(list_path.relative_to(repo).as_posix(), text, fixed)
        if args.patch_out:
            args.patch_out.parent.mkdir(parents=True, exist_ok=True)
            args.patch_out.write_text(patch_text, encoding="utf-8")
        output += patch_text
    sys.stdout.write(output)
    sys.stdout.flush()
    return 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ClosureError as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)
