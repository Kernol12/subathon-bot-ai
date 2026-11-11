#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare authenticated package acquisition on GitHub's Ubuntu 24.04 runners."""
import sys


def prepare_apt() -> None:
    """Configure the disposable runner and require a successful index refresh."""
    raise NotImplementedError("CI-001 Ubuntu APT preparation is not implemented")


def main() -> int:
    try:
        prepare_apt()
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
