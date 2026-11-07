#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Admit the existing isolated standards-library artifacts without importing them."""
import hashlib
import json
import os
from pathlib import Path
import sys


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def check_sdk_metadata(root):
    overrides = {"_PYTHON_SYSCONFIGDATA_NAME", "_PYTHON_SYSCONFIGDATA_PATH",
                 "_PYTHON_PROJECT_BASE"}
    require(not overrides.intersection(os.environ), "Uncontrolled sysconfig override")
    import sysconfig

    minor = sys.version_info[:2]
    require(sys.implementation.name == "cpython" and minor in {(3, 10), (3, 12), (3, 14)}
            and sys.platform == "linux" and os.uname().machine == "x86_64",
            "This dependency check requires its admitted Linux CPython target")
    target = f"cp{minor[0]}{minor[1]}"
    require(sysconfig.get_config_var("SOABI") == f"cpython-{minor[0]}{minor[1]}-x86_64-linux-gnu"
            and not bool(sysconfig.get_config_var("Py_DEBUG"))
            and not bool(sysconfig.get_config_var("Py_GIL_DISABLED")),
            "Actual interpreter ABI differs from the admitted target")
    # Project the no-site SDK from the venv executable; pre3.14 prefixes are unreliable.
    executable = Path(os.path.abspath(sys.executable))
    venv_root = executable.parent.parent
    config = venv_root / "pyvenv.cfg"
    require(config.is_file() and config.stat().st_size <= 4096,
            "Missing bounded selected-venv configuration")
    paths = sysconfig.get_paths(scheme="posix_prefix",
                               vars={"base": str(venv_root), "platbase": str(venv_root)})
    sdk = Path(paths["purelib"]).resolve(strict=True)
    require(sdk == Path(paths["platlib"]).resolve(strict=True),
            "Selected SDK does not have one installation root")
    packages = json.loads((root / "tools/schema_validation/wheels.json").read_text())["packages"]
    require(len(packages) == 20 and len({p["name"] for p in packages}) == 20,
            "Selected twenty-package metadata inventory differs")
    for package in packages:
        artifacts = [a for a in package["artifacts"] if target in a["candidate_targets"]]
        require(bool(artifacts), "No admitted artifact for actual interpreter")
        directories = {a["dist_info_directory"] for a in artifacts}
        require(len(directories) == 1, "Ambiguous distribution metadata identity")
        directory = sdk / next(iter(directories))
        observed = []
        for filename, ceiling in (("WHEEL", 4096), ("METADATA", 262144)):
            with (directory / filename).open("rb") as stream:
                payload = stream.read(ceiling + 1)
            require(len(payload) <= ceiling, "Installed metadata exceeds admitted ceiling")
            observed.append(hashlib.sha256(payload).hexdigest())
        require(any(observed == [a["wheel_metadata_sha256"], a["distribution_metadata_sha256"]]
                    for a in artifacts), "Installed metadata does not match one admitted artifact")
    return sdk
