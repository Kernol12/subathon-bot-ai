#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent fixture-generation CLI interface; recipes await implementation."""
import argparse
from pathlib import Path
import sys

# A read-only check must not leave recipe bytecode files behind.
sys.dont_write_bytecode = True

from fixture_sources import account_sessions, reference_capture


RECIPES = {
    "reference_capture": reference_capture.build,
    "account_sessions": account_sessions.build,
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
        for name, recipe in RECIPES.items():
            if name in selected:
                recipe(seed_root)
    except NotImplementedError as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
