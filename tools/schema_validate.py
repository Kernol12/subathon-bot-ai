#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Inert schema-package interface; behavior awaits the reviewed test baseline."""
from enum import Enum


class SchemaValidationOutcome(str, Enum):
    ok = "ok"
    usage_error = "usage_error"
    invalid_path = "invalid_path"
    io_unavailable = "io_unavailable"
    environment_unavailable = "environment_unavailable"
    resource_limit = "resource_limit"
    invalid_json = "invalid_json"
    invalid_manifest = "invalid_manifest"
    unsupported_dialect = "unsupported_dialect"
    unsupported_format = "unsupported_format"
    dependency_unavailable = "dependency_unavailable"
    invalid_schema = "invalid_schema"
    unresolved_reference = "unresolved_reference"
    reference_cycle = "reference_cycle"
    invalid_inventory = "invalid_inventory"
    unsupported_feature = "unsupported_feature"
    invalid_instance = "invalid_instance"
    validation_unavailable = "validation_unavailable"


class SchemaValidationNotImplemented(RuntimeError):
    def __init__(self):
        super().__init__(
            "Schema package validation is intentionally unimplemented; review tests first"
        )


# The test-first gate requires the raw call to reach the marker before any behavior.
def validate_package(manifest_path: str) -> SchemaValidationOutcome:
    raise SchemaValidationNotImplemented()


# Argument admission, help and output remain unimplemented alongside validation.
def main(argv: list[str] | tuple[str, ...] | None = None) -> int:
    raise SchemaValidationNotImplemented()


if __name__ == "__main__":
    raise SystemExit(main())
