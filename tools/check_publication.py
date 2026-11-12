#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""WORKFLOW-005 approved NI seams; no publication checks are implemented yet."""
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run_step(argv: list[str], *, git: bool = False) -> subprocess.CompletedProcess[bytes]:
    """Reserved child boundary; independent tests precede its implementation."""
    raise ValueError("PUBLICATION_CHECK_NOT_IMPLEMENTED")


def main(argv: list[str] | None = None) -> int:
    """Loadable explicit NI, never a successful publication check."""
    print("PUBLICATION_CHECK_NOT_IMPLEMENTED", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
