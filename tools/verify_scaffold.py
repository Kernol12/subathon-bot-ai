#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Direct GCC/Clang smoke check when CMake is unavailable; not an acceptance run."""
import argparse
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="g++", help="GCC/Clang-compatible compiler executable")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = root / "build/direct-smoke"
    out.mkdir(parents=True, exist_ok=True)
    executable = out / ("contract_smoke.exe" if sys.platform == "win32" else "contract_smoke")
    command = [args.cxx, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror",
               "-I", str(root / "core/include"), str(root / "core/src/time_credit.cpp"),
               str(root / "tests/scaffolding/contract_smoke.cpp"), "-o", str(executable)]
    print("Direct C++20 contract/scaffold check (not CMake/GoogleTest)", flush=True)
    try:
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        return 1
    print("PASS: compiled public contract, stub and infrastructure smoke; product behavior remains unimplemented")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
