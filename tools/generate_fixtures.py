#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reproduce reviewed independent fixture data from pinned seeds and literal recipes."""
import argparse
from pathlib import Path
import sys

# A read-only check must not leave recipe bytecode files behind.
sys.dont_write_bytecode = True

from fixture_sources import account_sessions, reference_capture
from fixture_sources.common import render


RECIPES = {
    "reference_capture": ("sqlite_reference_capture_vectors.json", reference_capture.build),
    "account_sessions": ("sqlite_account_sessions_vectors.json", account_sessions.build),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--output-dir", type=Path)
    parser.add_argument("--fixture", action="append", choices=tuple(RECIPES))
    args = parser.parse_args()
    selected = set(args.fixture) if args.fixture else set(RECIPES)
    seed_root = Path(__file__).resolve().parents[1] / "tests" / "fixtures"
    try:
        # Build every selected value before reading comparisons or writing outputs.
        generated = {filename: render(recipe(seed_root))
                     for name, (filename, recipe) in RECIPES.items() if name in selected}
        if args.check:
            for filename, expected in generated.items():
                path = seed_root / filename
                if path.read_bytes() != expected:
                    raise ValueError(f"Fixture drift: {path}")
            return 0
        args.output_dir.mkdir(parents=True, exist_ok=True)
        for filename, data in generated.items():
            (args.output_dir / filename).write_bytes(data)
        return 0
    except (OSError, ValueError, TypeError, KeyError, OverflowError) as error:
        print(f"Fixture generation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
