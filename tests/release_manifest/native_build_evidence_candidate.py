"""Tests-stage seam for the PACKAGE-001 technical evidence producer."""

from dataclasses import dataclass


@dataclass(frozen=True)
class Qualified:
    evidence: tuple[dict, ...]


@dataclass(frozen=True)
class Unqualified:
    findings: tuple[tuple[str, str], ...]


def qualify_native_build(selected, observed, repeated):
    """Receive the selected profile, independent inspection and clean rebuild."""
    return Unqualified((("not_implemented", selected["profile"]),))
