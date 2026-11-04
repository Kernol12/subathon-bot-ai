#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned catalog API and explicit missing-feature stubs; no generator behavior yet."""

from __future__ import annotations

import sys
from collections.abc import Sequence
from dataclasses import dataclass
from enum import Enum


class ParameterKind(str, Enum):
    TEXT = "text"
    INTEGER = "integer"


@dataclass(frozen=True, slots=True)
class Parameter:
    name: str
    kind: ParameterKind


@dataclass(frozen=True, slots=True)
class TextForms:
    text: str


@dataclass(frozen=True, slots=True)
class PluralForms:
    one: str
    other: str


@dataclass(frozen=True, slots=True)
class Message:
    message_id: str
    description: str
    parameters: tuple[Parameter, ...]
    plural: str | None
    english: TextForms | PluralForms
    german: TextForms | PluralForms


@dataclass(frozen=True, slots=True)
class Catalog:
    messages: tuple[Message, ...]


@dataclass(frozen=True, slots=True)
class TextArtifact:
    relative_path: str
    data: bytes


@dataclass(frozen=True, slots=True)
class ArtifactDigest:
    relative_path: str
    byte_count: int
    sha256: str


@dataclass(frozen=True, slots=True)
class BuildRequest:
    source: str
    text_output: str
    binary_output: str
    lrelease: str
    tool_timeout_ms: int = 120000


@dataclass(frozen=True, slots=True)
class BuildResult:
    message_count: int
    text: tuple[ArtifactDigest, ...]
    binary: tuple[ArtifactDigest, ...]
    qt_version: str


class CatalogErrorCode(str, Enum):
    INVALID_ENCODING = "invalid_encoding"
    INVALID_JSON = "invalid_json"
    DUPLICATE_KEY = "duplicate_key"
    INVALID_SCHEMA = "invalid_schema"
    DUPLICATE_ID = "duplicate_id"
    COVERAGE = "coverage"
    INVALID_TEMPLATE = "invalid_template"
    PARAMETER_MISMATCH = "parameter_mismatch"
    INVALID_PLURAL = "invalid_plural"
    RESOURCE_LIMIT = "resource_limit"
    TOOL_UNAVAILABLE = "tool_unavailable"
    TOOL_VERSION = "tool_version"
    TOOL_FAILED = "tool_failed"
    GENERATED_MISMATCH = "generated_mismatch"
    IO_FAILURE = "io_failure"


class CatalogFailure(Exception):
    """Bounded diagnostic data; validation and CLI formatting remain unimplemented."""

    def __init__(
        self,
        code: CatalogErrorCode,
        *,
        message_id: str | None = None,
        locale: str | None = None,
        parameter: str | None = None,
        stage: str | None = None,
        tool_exit: int | None = None,
        tool_reason: str | None = None,
    ) -> None:
        super().__init__(f"Translation catalog failure: {code.value}")
        self.code = code
        self.message_id = message_id
        self.locale = locale
        self.parameter = parameter
        self.stage = stage
        self.tool_exit = tool_exit
        self.tool_reason = tool_reason


class TranslationCatalogNotImplemented(RuntimeError):
    def __init__(self) -> None:
        super().__init__(
            "Translation catalog generation is intentionally unimplemented; review tests first"
        )


# Every feature entry remains an immediate marker until its reviewed intended-red gate.
def validate_catalog(source: bytes) -> Catalog:
    raise TranslationCatalogNotImplemented()


def render_text(source: bytes) -> tuple[TextArtifact, ...]:
    raise TranslationCatalogNotImplemented()


def generate_catalog(request: BuildRequest) -> BuildResult:
    raise TranslationCatalogNotImplemented()


def check_catalog(request: BuildRequest) -> BuildResult:
    raise TranslationCatalogNotImplemented()


def main(argv: Sequence[str] | None = None) -> int:
    raise TranslationCatalogNotImplemented()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TranslationCatalogNotImplemented:
        sys.stderr.write(
            '{"status":"unimplemented","marker":"TranslationCatalogNotImplemented"}\n'
        )
        raise SystemExit(86)
