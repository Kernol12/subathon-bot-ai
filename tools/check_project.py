#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check baseline/task/spec consistency; does not claim product acceptance."""
from __future__ import annotations
import json
import re
import sys
import argparse
import workflow
from pathlib import Path
from workflow import evidence_path, local_path

ROOT = Path(__file__).resolve().parents[1]


def check_repository(root: Path) -> dict:
    """Check repository artifacts independently of tracker state and rendered HTML."""
    spec = (root / "docs/specification.md").read_text(encoding="utf-8")
    version = re.search(r"^Version: (\S+)", spec, re.M).group(1)
    declarations = [line for line in (root / "docs/STATUS.md").read_text(encoding="utf-8").splitlines()
                    if line.startswith("Specification version:")]
    if declarations != [f"Specification version: {version}"]:
        raise ValueError("STATUS must declare exactly the current Specification version")
    ids = re.findall(r"^\| ([A-Z][A-Z0-9-]*-\d+) \|", spec, re.M)
    if len(ids) != len(set(ids)):
        raise ValueError("Duplicate specification decision IDs")
    coverage = json.loads((root / "tests/acceptance-map.json").read_text(encoding="utf-8"))
    mapped = [item["id"] for item in coverage["decisions"]]
    if set(mapped) != set(ids) or len(mapped) != len(ids) or coverage["specification_version"] != version:
        raise ValueError("Acceptance map must track every current decision exactly once")
    for item in [*coverage["decisions"], *coverage["scenarios"]]:
        for path in item.get("tests", []):
            if not local_path(path, root).is_file():
                raise ValueError(f"Missing acceptance-map test artifact: {path}")
        for reference in item.get("evidence", []):
            path = evidence_path(reference, root)
            if path is not None and not path.is_file():
                raise ValueError(f"Missing acceptance-map evidence: {reference}")
    # Historical receipts live at the archive tag; keep the current record readable.
    evidence = root / "docs/evidence"
    if {path.relative_to(evidence).as_posix() for path in evidence.rglob("*") if path.is_file()} != {"README.md"}:
        raise ValueError("docs/evidence must contain only README.md; keep detailed receipts in the PR")
    presets = json.loads((root / "CMakePresets.json").read_text(encoding="utf-8"))
    if presets["version"] != 6:
        raise ValueError("Review the documented CMake/preset minimum before changing preset schema")
    names = {p["name"] for p in presets["configurePresets"]}
    for section in ("buildPresets", "testPresets"):
        if any(p["configurePreset"] not in names for p in presets[section]):
            raise ValueError("Preset references an unknown configuration")
    # Check actual Markdown links; fenced setup examples are intentionally not path manifests.
    for document in [root / "README.md", *list((root / "docs").rglob("*.md"))]:
        for href in re.findall(r"\[[^\]]+\]\(([^)]+)\)", document.read_text(encoding="utf-8")):
            if href.startswith(("https://", "http://", "#")):
                continue
            target = (document.parent / href.split("#")[0]).resolve()
            if not target.is_relative_to(root) or not target.exists():
                raise ValueError(f"Broken Markdown link in {document.relative_to(root)}: {href}")
    count = len(re.findall(r"\bTEST\(TimeCreditAcceptance,", (root / "tests/acceptance/time_credit_tests.cpp").read_text()))
    return dict(decisions=len(ids), time_credit_declarations=count)


def main(argv=()) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    try:
        check_repository(ROOT)
        print("PASS: repository checks")
        return 0
    except (OSError, ValueError, TypeError, KeyError, AttributeError) as error:
        print(f"Project check failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
