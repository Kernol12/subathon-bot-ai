#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check baseline/task/spec consistency; does not claim product acceptance."""
from __future__ import annotations
import json
import re
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote
from progress import ROOT, STATE, OUTPUT, local_path, render, validate


class OfflineLinks(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []
        self.ids = []
    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.append(attrs["id"])
        if tag in {"script", "img", "iframe", "link"} and ("src" in attrs or "href" in attrs):
            raise ValueError("Progress page must not need external assets")
        if tag == "a" and "href" in attrs:
            self.links.append(attrs["href"])


def main() -> int:
    try:
        data = json.loads(STATE.read_text(encoding="utf-8"))
        validate(data)
        spec = (ROOT / "docs/specification.md").read_text(encoding="utf-8")
        version = re.search(r"^Version: (\S+)", spec, re.M).group(1)
        if version != data["specification_version"]:
            raise ValueError("Progress/specification version mismatch")
        ids = re.findall(r"^\| ([A-Z][A-Z0-9-]*-\d+) \|", spec, re.M)
        if len(ids) != len(set(ids)):
            raise ValueError("Duplicate specification decision IDs")
        for task in data["tasks"]:
            for ref in task["spec_refs"]:
                if ref not in ids and not re.fullmatch(r"§(?:[1-9]|1[0-9]|2[01])", ref):
                    raise ValueError(f"{task['id']}: unknown specification reference {ref}")
        coverage = json.loads((ROOT / "tests/acceptance-map.json").read_text(encoding="utf-8"))
        mapped = [item["id"] for item in coverage["decisions"]]
        if set(mapped) != set(ids) or len(mapped) != len(ids) or coverage["specification_version"] != version:
            raise ValueError("Acceptance map must track every current decision exactly once")
        for item in [*coverage["decisions"], *coverage["scenarios"]]:
            for path in item.get("tests", []) + item.get("evidence", []):
                if not local_path(path).is_file():
                    raise ValueError(f"Missing acceptance-map artifact: {path}")
        presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
        if presets["version"] != 6:
            raise ValueError("Review the documented CMake/preset minimum before changing preset schema")
        names = {p["name"] for p in presets["configurePresets"]}
        for section in ("buildPresets", "testPresets"):
            if any(p["configurePreset"] not in names for p in presets[section]):
                raise ValueError("Preset references an unknown configuration")
        expected = render(data)
        if not OUTPUT.is_file() or OUTPUT.read_text(encoding="utf-8") != expected:
            raise ValueError("Stale progress snapshot: run python3 tools/progress.py render")
        parser = OfflineLinks()
        parser.feed(expected)
        if len(parser.ids) != len(set(parser.ids)):
            raise ValueError("Duplicate HTML element IDs")
        for href in parser.links:
            if href.startswith("#"):
                if href[1:] not in parser.ids:
                    raise ValueError(f"Missing HTML anchor {href}")
            elif not local_path(unquote(href)).is_file():
                raise ValueError(f"Missing HTML link {href}")
        # Check actual Markdown links; fenced setup examples are intentionally not path manifests.
        for document in [ROOT / "README.md", *list((ROOT / "docs").rglob("*.md"))]:
            for href in re.findall(r"\[[^\]]+\]\(([^)]+)\)", document.read_text(encoding="utf-8")):
                if href.startswith(("https://", "http://", "#")):
                    continue
                target = (document.parent / href.split("#")[0]).resolve()
                if not target.is_relative_to(ROOT) or not target.exists():
                    raise ValueError(f"Broken Markdown link in {document.relative_to(ROOT)}: {href}")
        count = len(re.findall(r"\bTEST\(TimeCreditAcceptance,", (ROOT / "tests/acceptance/time_credit_tests.cpp").read_text()))
        print(f"PASS: {len(data['tasks'])} task records, {len(ids)} decisions, local links, presets and current HTML")
        print(f"INFO: {count} draft GoogleTest cases; this check does not compile/run them or approve the baseline")
        return 0
    except (OSError, ValueError, TypeError, KeyError, AttributeError) as error:
        print(f"Project check failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
