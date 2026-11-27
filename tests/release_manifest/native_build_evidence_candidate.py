"""Bind the reviewed native build evidence tests to production."""

import sys

from fixture import ROOT

sys.path.insert(0, str(ROOT))

from deploy.native.native_build_evidence import (  # noqa: E402, F401
    InputDifference,
    Qualified,
    Unqualified,
    qualify_native_build,
)
