"""Data-only assembly interface for RELEASE-001."""

from dataclasses import dataclass
from typing import TypedDict


class Evidence(TypedDict):
    id: str
    kind: str
    owner: str
    revision: str
    subject: str
    scope: str
    facts: dict


class SelectedPin(TypedDict):
    subject: str
    role: str
    evidence: str


class Observation(TypedDict):
    scope: str
    artifact: dict
    members: list[dict]
    review: str
    composition: list[dict]


class ArtifactInputs(TypedDict, total=False):
    expected: dict
    observed: Observation


class ReleaseRequest(ArtifactInputs):
    selection: dict
    artifact_id: str
    selected_pins: list[SelectedPin]
    evidence: list[Evidence]
    components: list[dict]
    rights: list[dict]
    notices: list[dict]
    sources: list[dict]
    assets: list[dict]
    fonts: list[dict]


@dataclass(frozen=True)
class Finding:
    artifact: str
    code: str
    subject: str


@dataclass(frozen=True)
class Complete:
    manifest_bytes: bytes


@dataclass(frozen=True)
class Blocked:
    findings: tuple[Finding, ...]


def assemble(request: ReleaseRequest) -> Complete | Blocked:
    """Assemble one selected scope from owned data records."""
    raise NotImplementedError("RELEASE-001 manifest assembly is missing")
