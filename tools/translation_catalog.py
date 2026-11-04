#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate one owned catalog and stage deterministic text plus real Qt translations."""

from __future__ import annotations

import hashlib
import json
import os
import re
import stat
import subprocess
import sys
import tempfile
import threading
import time
from collections.abc import Sequence
from contextlib import closing
from dataclasses import dataclass
from enum import Enum
from itertools import zip_longest
from pathlib import Path


_SOURCE_LIMIT = 4 * 1024 * 1024
_ARTIFACT_LIMIT = 64 * 1024 * 1024
_CHUNK = 65536
_ID = re.compile(r"[a-z][a-z0-9]*(?:[._][a-z0-9]+)*", re.ASCII)
_NAME = re.compile(r"[a-z][a-z0-9_]{0,31}", re.ASCII)
_TEXT_PATHS = ("qt/messages_de.ts", "qt/messages_en.ts", "web/de.json", "web/en.json")
_BINARY_PATHS = ("messages_de.qm", "messages_en.qm")
_STAGES = frozenset(("input", "schema", "parameters", "coverage", "template", "paths",
                     "inventory", "version", "en", "de", "publication", "comparison"))
_TOOL_REASONS = frozenset(("exit", "timeout", "output_limit", "missing_output",
                           "malformed_version", "version_mismatch", "invalid_output",
                           "output_file_limit"))
_HELP = (
    "usage: translation_catalog.py {validate,generate,check} ...\n"
    "validate --source PATH\n"
    "generate --source PATH --text-output DIR --binary-output DIR --lrelease PATH\n"
    "check --source PATH --text-output DIR --binary-output DIR --lrelease PATH\n"
)


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
    """Safe fixed diagnostic fields; never retain source bodies or child output."""

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
        self.message_id = message_id if (type(message_id) is str and len(message_id) <= 160
                                        and _ID.fullmatch(message_id)) else None
        self.locale = locale if locale in ("en", "de") else None
        self.parameter = parameter if (type(parameter) is str and _NAME.fullmatch(parameter)) else None
        self.stage = stage if type(stage) is str and stage in _STAGES else None
        self.tool_exit = tool_exit if type(tool_exit) is int else None
        self.tool_reason = tool_reason if type(tool_reason) is str and tool_reason in _TOOL_REASONS else None


class TranslationCatalogNotImplemented(RuntimeError):
    def __init__(self) -> None:
        super().__init__(
            "Translation catalog generation is intentionally unimplemented; review tests first"
        )


def _fail(code: CatalogErrorCode, **fields):
    raise CatalogFailure(code, **fields)


def _utf8_size(text: str) -> int:
    # Measure before allocating encoded/escaped copies. Surrogates are rejected first.
    return sum(1 if ord(c) < 0x80 else 2 if ord(c) < 0x800 else
               3 if ord(c) < 0x10000 else 4 for c in text)


def _xml_scalar(text: str) -> bool:
    return all(c in "\t\n\r" or 0x20 <= ord(c) <= 0xD7FF or
               0xE000 <= ord(c) <= 0xFFFD or 0x10000 <= ord(c) <= 0x10FFFF for c in text)


def _bounded(text: str, maximum: int, **fields) -> None:
    if _utf8_size(text) > maximum:
        _fail(CatalogErrorCode.RESOURCE_LIMIT, **fields)


@dataclass(frozen=True, slots=True)
class _Number:
    spelling: str
    integer: bool


def _pairs(items):
    result = {}
    for key, value in items:
        if key in result:
            _fail(CatalogErrorCode.DUPLICATE_KEY, stage="input")
        result[key] = value
    return result


def _decode(source: bytes):
    if type(source) is not bytes:
        _fail(CatalogErrorCode.INVALID_SCHEMA, stage="input")
    if len(source) > _SOURCE_LIMIT:
        _fail(CatalogErrorCode.RESOURCE_LIMIT, stage="input")
    try:
        text = source.decode("utf-8", errors="strict")
    except UnicodeDecodeError:
        _fail(CatalogErrorCode.INVALID_ENCODING, stage="input")
    if text.startswith("\ufeff"):
        _fail(CatalogErrorCode.INVALID_ENCODING, stage="input")

    # Count containers outside strings before the recursive stdlib parser runs.
    depth, quoted, escaped = 0, False, False
    for char in text:
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char in "[{":
            depth += 1
            if depth > 8:
                _fail(CatalogErrorCode.RESOURCE_LIMIT, stage="input")
        elif char in "]}":
            depth -= 1
    try:
        value = json.loads(
            text, object_pairs_hook=_pairs,
            parse_int=lambda token: _Number(token, True),
            parse_float=lambda token: _Number(token, False),
            parse_constant=lambda _: _fail(CatalogErrorCode.INVALID_JSON, stage="input"),
        )
    except (ValueError, RecursionError):
        _fail(CatalogErrorCode.INVALID_JSON, stage="input")

    def scalars(node):
        if type(node) is str:
            if any(0xD800 <= ord(c) <= 0xDFFF for c in node):
                _fail(CatalogErrorCode.INVALID_ENCODING, stage="input")
        elif type(node) is list:
            for item in node:
                scalars(item)
        elif type(node) is dict:
            for key, item in node.items():
                scalars(key)
                scalars(item)
    scalars(value)
    return value


def _template(text: str, names: set[str], selector: str | None, **fields) -> None:
    _bounded(text, 65536, **fields)
    if not _xml_scalar(text):
        _fail(CatalogErrorCode.INVALID_TEMPLATE, **fields)
    used, index = set(), 0
    while index < len(text):
        char = text[index]
        if char in "{}":
            if text[index:index + 2] == char * 2:
                index += 2
                continue
            if char == "}":
                _fail(CatalogErrorCode.INVALID_TEMPLATE, **fields)
            end = text.find("}", index + 1)
            name = text[index + 1:end] if end >= 0 else ""
            if not _NAME.fullmatch(name):
                _fail(CatalogErrorCode.INVALID_TEMPLATE, **fields)
            used.add(name)
            index = end + 1
        else:
            index += 1
    unknown = sorted(used - names)
    missing = sorted(names - used - ({selector} if selector else set()))
    if unknown or missing:
        _fail(CatalogErrorCode.PARAMETER_MISMATCH,
              parameter=(unknown or missing)[0], **fields)


def validate_catalog(source: bytes) -> Catalog:
    """Validate global stages in order, then return sorted immutable owned values."""
    value = _decode(source)
    root_keys = {"schema_version", "license", "source_locale", "locales", "messages"}
    if (type(value) is not dict or value.keys() != root_keys or
            value["schema_version"] != _Number("1", True) or
            value["license"] != "GPL-3.0-or-later" or value["source_locale"] != "en" or
            value["locales"] != ["en", "de"] or type(value["messages"]) is not list or
            not value["messages"]):
        _fail(CatalogErrorCode.INVALID_SCHEMA, stage="schema")
    messages = value["messages"]
    if len(messages) > 8192:
        _fail(CatalogErrorCode.RESOURCE_LIMIT, stage="schema")
    message_keys = {"id", "description", "parameters", "plural", "translations"}
    for message in messages:
        if type(message) is not dict or type(message.get("id")) is not str:
            _fail(CatalogErrorCode.INVALID_SCHEMA, stage="schema")
        _bounded(message["id"], 160, stage="schema")
        if not _ID.fullmatch(message["id"]):
            _fail(CatalogErrorCode.INVALID_SCHEMA, stage="schema")
    messages = sorted(messages, key=lambda message: message["id"])
    seen = set()
    for message in messages:
        fields = {"message_id": message["id"], "stage": "schema"}
        if message.keys() != message_keys:
            _fail(CatalogErrorCode.INVALID_SCHEMA, **fields)
        description = message["description"]
        if type(description) is not str or not description:
            _fail(CatalogErrorCode.INVALID_SCHEMA, **fields)
        _bounded(description, 4096, **fields)
        if not _xml_scalar(description):
            _fail(CatalogErrorCode.INVALID_SCHEMA, **fields)
        if message["id"] in seen:
            _fail(CatalogErrorCode.DUPLICATE_ID, **fields)
        seen.add(message["id"])

    for message in messages:
        fields = {"message_id": message["id"], "stage": "parameters"}
        parameters = message["parameters"]
        if type(parameters) is not dict:
            _fail(CatalogErrorCode.INVALID_SCHEMA, **fields)
        if len(parameters) > 32:
            _fail(CatalogErrorCode.RESOURCE_LIMIT, **fields)
        for name in sorted(parameters):
            _bounded(name, 32, **fields)
            if not _NAME.fullmatch(name) or parameters[name] not in ("text", "integer"):
                _fail(CatalogErrorCode.INVALID_SCHEMA, parameter=name, **fields)
        selector = message["plural"]
        if selector is not None and (type(selector) is not str or
                                     parameters.get(selector) != "integer"):
            _fail(CatalogErrorCode.INVALID_PLURAL, **fields)

    for message in messages:
        fields = {"message_id": message["id"], "stage": "coverage"}
        translations = message["translations"]
        if type(translations) is not dict or translations.keys() != {"en", "de"}:
            _fail(CatalogErrorCode.COVERAGE, **fields)
        keys = ("text",) if message["plural"] is None else ("one", "other")
        for locale in ("de", "en"):
            fields["locale"] = locale
            forms = translations[locale]
            if forms is None:
                _fail(CatalogErrorCode.COVERAGE, **fields)
            if type(forms) is not dict:
                _fail(CatalogErrorCode.INVALID_SCHEMA, **fields)
            if forms.keys() != set(keys):
                _fail(CatalogErrorCode.INVALID_PLURAL, **fields)
            for key in keys:
                if forms[key] is None or forms[key] == "":
                    _fail(CatalogErrorCode.COVERAGE, **fields)
                if type(forms[key]) is not str:
                    _fail(CatalogErrorCode.INVALID_SCHEMA, **fields)

    for message in messages:
        for locale in ("de", "en"):
            keys = ("text",) if message["plural"] is None else ("one", "other")
            for key in keys:
                _template(message["translations"][locale][key], set(message["parameters"]),
                          message["plural"], message_id=message["id"], locale=locale, stage="template")

    def forms(message, locale):
        texts = message["translations"][locale]
        return TextForms(texts["text"]) if message["plural"] is None else PluralForms(texts["one"], texts["other"])
    return Catalog(tuple(Message(message["id"], message["description"],
                                 tuple(Parameter(name, ParameterKind(message["parameters"][name]))
                                       for name in sorted(message["parameters"])),
                                 message["plural"], forms(message, "en"), forms(message, "de"))
                         for message in messages))


def _xml(text: str) -> str:
    escapes = {"&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&apos;", "\r": "&#13;"}
    return "".join(escapes.get(c, c) for c in text)


def _encoded_chunks(chunks) -> bytes:
    size, encoded = 0, []
    for chunk in chunks:
        count = _utf8_size(chunk)
        if count > _ARTIFACT_LIMIT - size:
            _fail(CatalogErrorCode.RESOURCE_LIMIT, stage="template")
        encoded.append(chunk.encode("utf-8"))
        size += count
    return b"".join(encoded)


def _ts_chunks(catalog: Catalog, locale: str):
    yield '<?xml version="1.0" encoding="utf-8"?>\n'
    yield f'<TS version="2.1" language="{locale}" sourcelanguage="en">\n  <context>\n    <name></name>\n'
    for message in catalog.messages:
        plural = ' numerus="yes"' if message.plural is not None else ""
        yield f'    <message id="{message.message_id}"{plural}>\n'
        source = message.english.text if isinstance(message.english, TextForms) else message.english.other
        yield f'      <source>{_xml(source)}</source>\n'
        yield f'      <extracomment>{_xml(message.description)}</extracomment>\n'
        forms = message.english if locale == "en" else message.german
        if isinstance(forms, TextForms):
            yield f'      <translation>{_xml(forms.text)}</translation>\n'
        else:
            yield '      <translation>\n'
            yield f'        <numerusform>{_xml(forms.one)}</numerusform>\n'
            yield f'        <numerusform>{_xml(forms.other)}</numerusform>\n'
            yield '      </translation>\n'
        yield '    </message>\n'
    yield '  </context>\n</TS>\n'


def _render(catalog: Catalog) -> tuple[TextArtifact, ...]:
    artifacts = []
    for locale in ("de", "en"):
        artifacts.append(TextArtifact(f"qt/messages_{locale}.ts", _encoded_chunks(_ts_chunks(catalog, locale))))
    for locale in ("de", "en"):
        messages = {}
        for message in catalog.messages:
            forms = message.english if locale == "en" else message.german
            messages[message.message_id] = {
                "parameters": {p.name: p.kind.value for p in message.parameters},
                "plural": message.plural,
                "forms": {"text": forms.text} if isinstance(forms, TextForms) else
                         {"one": forms.one, "other": forms.other},
            }
        value = {"schema_version": 1, "locale": locale, "fallback": "en", "messages": messages}
        escapes = {"<": "\\u003c", ">": "\\u003e", "&": "\\u0026", "\u2028": "\\u2028", "\u2029": "\\u2029"}
        chunks = ("".join(escapes.get(c, c) for c in chunk)
                  for chunk in json.JSONEncoder(ensure_ascii=False, indent=2).iterencode(value))
        data = _encoded_chunks(chunks)
        if len(data) == _ARTIFACT_LIMIT:
            _fail(CatalogErrorCode.RESOURCE_LIMIT, stage="template")
        artifacts.append(TextArtifact(f"web/{locale}.json", data + b"\n"))
    return tuple(artifacts)


def render_text(source: bytes) -> tuple[TextArtifact, ...]:
    return _render(validate_catalog(source))


def _path_text(value) -> bool:
    return (type(value) is str and bool(value) and "\0" not in value and
            not any(0xD800 <= ord(c) <= 0xDFFF for c in value))


def _identity(info) -> tuple[int, int]:
    return info.st_dev, info.st_ino


@dataclass(frozen=True, slots=True)
class _Source:
    requested: str
    resolved: Path
    identity: tuple[int, int]
    data: bytes


def _read_source(source: str) -> _Source:
    if not _path_text(source):
        _fail(CatalogErrorCode.IO_FAILURE, stage="paths")
    try:
        path = Path(source).resolve(strict=True)
        info = path.stat()
        if not stat.S_ISREG(info.st_mode):
            _fail(CatalogErrorCode.IO_FAILURE, stage="input")
        # Nonblocking also prevents a swapped FIFO from blocking between stat/open.
        descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_NONBLOCK", 0))
        with os.fdopen(descriptor, "rb") as stream:
            opened = os.fstat(stream.fileno())
            if not stat.S_ISREG(opened.st_mode) or _identity(opened) != _identity(info):
                _fail(CatalogErrorCode.IO_FAILURE, stage="input")
            chunks, remaining = [], _SOURCE_LIMIT + 1
            while remaining:
                block = stream.read(min(_CHUNK, remaining))
                if not block:
                    break
                chunks.append(block)
                remaining -= len(block)
            data = b"".join(chunks)
        if len(data) > _SOURCE_LIMIT:
            _fail(CatalogErrorCode.RESOURCE_LIMIT, stage="input")
        return _Source(source, path, _identity(info), data)
    except (OSError, ValueError, RuntimeError):
        _fail(CatalogErrorCode.IO_FAILURE, stage="input")


@dataclass(frozen=True, slots=True)
class _Layout:
    text: Path
    binary: Path
    tool: Path


def _within(path: Path, root: Path) -> bool:
    return path == root or root in path.parents


def _optional_lstat(path: Path):
    try:
        return path.lstat()
    except FileNotFoundError:
        return None


def _layout(request: BuildRequest, source: _Source) -> _Layout:
    """Inspect only the fixed owned directories; matching special files are errors."""
    try:
        text = Path(request.text_output).resolve(strict=False)
        binary = Path(request.binary_output).resolve(strict=False)
        tool = Path(request.lrelease).resolve(strict=False)
        if (_within(text, binary) or _within(binary, text) or
                any(_within(path, root) for path in (source.resolved, tool) for root in (text, binary))):
            _fail(CatalogErrorCode.IO_FAILURE, stage="paths")
        tool_info = _optional_lstat(tool)
        identities = {source.identity}
        if tool_info is not None:
            identities.add(_identity(tool_info))
        for root in (text, binary):
            info = _optional_lstat(root)
            if info is not None and (not stat.S_ISDIR(info.st_mode) or stat.S_ISLNK(info.st_mode)):
                _fail(CatalogErrorCode.IO_FAILURE, stage="paths")
        extras = False
        inventories = (
            (text / "qt", "messages_", ".ts", {"messages_en.ts", "messages_de.ts"}),
            (text / "web", "", ".json", {"en.json", "de.json"}),
            (binary, "messages_", ".qm", set(_BINARY_PATHS)),
        )
        for directory, prefix, suffix, expected in inventories:
            info = _optional_lstat(directory)
            if info is None:
                continue
            if not stat.S_ISDIR(info.st_mode) or stat.S_ISLNK(info.st_mode):
                _fail(CatalogErrorCode.IO_FAILURE, stage="inventory")
            with os.scandir(directory) as entries:
                for entry in entries:
                    if not (entry.name.startswith(prefix) and entry.name.endswith(suffix)):
                        continue
                    info = entry.stat(follow_symlinks=False)
                    if not stat.S_ISREG(info.st_mode):
                        _fail(CatalogErrorCode.IO_FAILURE, stage="inventory")
                    if _identity(info) in identities:
                        _fail(CatalogErrorCode.IO_FAILURE, stage="paths")
                    extras = extras or entry.name not in expected
        if extras:
            _fail(CatalogErrorCode.GENERATED_MISMATCH, stage="inventory")
        return _Layout(text, binary, tool)
    except (OSError, ValueError, RuntimeError):
        _fail(CatalogErrorCode.IO_FAILURE, stage="paths")


def _run_tool(tool: Path, arguments: list[str], timeout_ms: int, stage: str) -> bytes:
    """Drain both pipes with a shared capture ceiling, then reap this child only."""
    deadline = time.monotonic() + timeout_ms / 1000
    try:
        process = subprocess.Popen([str(tool), *arguments], stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, stdin=subprocess.DEVNULL, shell=False)
    except OSError:
        _fail(CatalogErrorCode.TOOL_UNAVAILABLE, stage=stage)
    buffers = (bytearray(), bytearray())
    lock, excessive = threading.Lock(), threading.Event()
    finished = (threading.Event(), threading.Event())
    read_errors = []
    captured = 0

    def drain(pipe, index):
        nonlocal captured
        try:
            while True:
                block = os.read(pipe.fileno(), 4096)
                if not block:
                    break
                with lock:
                    if len(block) > _CHUNK - captured:
                        excessive.set()
                    elif not excessive.is_set():
                        buffers[index].extend(block)
                        captured += len(block)
        except Exception as error:
            # Relay on the calling thread, without a worker traceback on stderr.
            with lock:
                read_errors.append(error)
        finally:
            finished[index].set()

    threads = tuple(threading.Thread(target=drain, args=(pipe, index), daemon=True)
                    for index, pipe in enumerate((process.stdout, process.stderr)))
    reason = None
    try:
        for thread in threads:
            thread.start()
        while process.poll() is None or not all(event.is_set() for event in finished):
            if excessive.is_set():
                reason = "output_limit"
                break
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                reason = "timeout"
                break
            # The readers perform blocking pipe reads; this wait is deadline bounded.
            finished[0].wait(min(remaining, 0.01))
            if finished[0].is_set():
                finished[1].wait(min(max(0, deadline - time.monotonic()), 0.01))
        if excessive.is_set():
            reason = "output_limit"
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=0.2)
            except subprocess.TimeoutExpired:
                process.kill()
        process.wait()
        # A normal Qt/error-only child closes both pipes at exit. Do not wait on an
        # unrelated descendant indefinitely if a supplied executable inherited them.
        for thread in threads:
            if thread.ident is not None:
                thread.join(timeout=0.2)
        process.stdout.close()
        process.stderr.close()
    if reason is not None:
        _fail(CatalogErrorCode.TOOL_FAILED, stage=stage, tool_exit=process.returncode, tool_reason=reason)
    if read_errors:
        if isinstance(read_errors[0], (OSError, ValueError)):
            _fail(CatalogErrorCode.IO_FAILURE, stage=stage)
        raise read_errors[0]
    if process.returncode != 0:
        _fail(CatalogErrorCode.TOOL_FAILED, stage=stage, tool_exit=process.returncode, tool_reason="exit")
    return bytes(buffers[0])


@dataclass(frozen=True, slots=True)
class _Staged:
    relative_path: str
    path: Path
    identity: tuple[int, int]
    size: int
    locale: str | None = None


def _bad_artifact(locale: str | None, reason="invalid_output"):
    if locale is not None:
        _fail(CatalogErrorCode.TOOL_FAILED, stage=locale, tool_exit=0, tool_reason=reason)
    _fail(CatalogErrorCode.IO_FAILURE, stage="publication")


def _staged(path: Path, relative: str, locale: str | None = None) -> _Staged:
    try:
        info = _optional_lstat(path)
        if info is None:
            _bad_artifact(locale, "missing_output")
        if not stat.S_ISREG(info.st_mode):
            _bad_artifact(locale)
        if info.st_size == 0:
            _bad_artifact(locale, "missing_output")
        if info.st_size > _ARTIFACT_LIMIT:
            _bad_artifact(locale, "output_file_limit")
        return _Staged(relative, path, _identity(info), info.st_size, locale)
    except OSError:
        _fail(CatalogErrorCode.IO_FAILURE, stage=locale or "publication")


def _read_chunks(path: Path, size: int, identity: tuple[int, int], changed, stage: str):
    """Read exactly one checked ordinary file, with at most a one-byte EOF probe."""
    try:
        info = _optional_lstat(path)
        if info is None:
            changed()
        if not stat.S_ISREG(info.st_mode) or _identity(info) != identity or info.st_size != size:
            changed()
        descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_NONBLOCK", 0))
        with os.fdopen(descriptor, "rb") as stream:
            info = os.fstat(stream.fileno())
            if not stat.S_ISREG(info.st_mode) or _identity(info) != identity or info.st_size != size:
                changed()
            remaining = size
            while remaining:
                block = stream.read(min(_CHUNK, remaining))
                if not block:
                    changed()
                remaining -= len(block)
                yield block
            if stream.read(1):
                changed()
            info = os.fstat(stream.fileno())
            if not stat.S_ISREG(info.st_mode) or _identity(info) != identity or info.st_size != size:
                changed()
    except OSError:
        _fail(CatalogErrorCode.IO_FAILURE, stage=stage)


def _stage_chunks(artifact: _Staged):
    return _read_chunks(artifact.path, artifact.size, artifact.identity,
                        lambda: _bad_artifact(artifact.locale), artifact.locale or "publication")


def _digest(artifact: _Staged) -> ArtifactDigest:
    digest = hashlib.sha256()
    with closing(_stage_chunks(artifact)) as chunks:
        for block in chunks:
            digest.update(block)
    return ArtifactDigest(artifact.relative_path, artifact.size, digest.hexdigest())


def _mismatch():
    _fail(CatalogErrorCode.GENERATED_MISMATCH, stage="comparison")


def _compare(artifact: _Staged, target: Path) -> None:
    try:
        info = _optional_lstat(target)
        if info is None:
            _mismatch()
        if not stat.S_ISREG(info.st_mode):
            _fail(CatalogErrorCode.IO_FAILURE, stage="comparison")
        if info.st_size != artifact.size:
            _mismatch()
        existing = _read_chunks(target, artifact.size, _identity(info), _mismatch, "comparison")
        staged = _stage_chunks(artifact)
        try:
            for actual, expected in zip_longest(existing, staged):
                if actual != expected:
                    _mismatch()
        finally:
            existing.close()
            staged.close()
    except OSError:
        _fail(CatalogErrorCode.IO_FAILURE, stage="comparison")


def _publish(artifact: _Staged, target: Path) -> None:
    temporary = None
    try:
        target.parent.mkdir(parents=True, exist_ok=True)
        descriptor, temporary = tempfile.mkstemp(prefix=f".{target.name}.", dir=target.parent)
        with os.fdopen(descriptor, "wb") as stream:
            with closing(_stage_chunks(artifact)) as chunks:
                for block in chunks:
                    if stream.write(block) != len(block):
                        _fail(CatalogErrorCode.IO_FAILURE, stage="publication")
        os.replace(temporary, target)
    except OSError:
        _fail(CatalogErrorCode.IO_FAILURE, stage="publication")
    finally:
        if temporary is not None:
            try:
                os.unlink(temporary)
            except FileNotFoundError:
                pass
            except OSError:
                _fail(CatalogErrorCode.IO_FAILURE, stage="publication")


def _build(request: BuildRequest, *, check: bool) -> BuildResult:
    if (type(request) is not BuildRequest or type(request.tool_timeout_ms) is not int or
            not 1 <= request.tool_timeout_ms <= 120000):
        _fail(CatalogErrorCode.INVALID_SCHEMA, stage="input")
    if not all(_path_text(value) for value in
               (request.source, request.text_output, request.binary_output, request.lrelease)):
        _fail(CatalogErrorCode.IO_FAILURE, stage="paths")
    source = _read_source(request.source)
    catalog = validate_catalog(source.data)
    layout = _layout(request, source)
    texts = _render(catalog)
    try:
        with tempfile.TemporaryDirectory(prefix="translation-catalog-") as directory:
            stage = Path(directory)
            staged_text = []
            for artifact in texts:
                path = stage / artifact.relative_path
                path.parent.mkdir(parents=True, exist_ok=True)
                if path.write_bytes(artifact.data) != len(artifact.data):
                    _fail(CatalogErrorCode.IO_FAILURE, stage="publication")
                staged = _staged(path, artifact.relative_path)
                if staged.size != len(artifact.data):
                    _fail(CatalogErrorCode.IO_FAILURE, stage="publication")
                staged_text.append(staged)
            version = _run_tool(layout.tool, ["-version"], request.tool_timeout_ms, "version")
            version = version.strip(b" \t\r\n\v\f")
            if version != b"lrelease version 6.12.0":
                reason = ("version_mismatch" if
                          re.fullmatch(rb"lrelease version [0-9]+\.[0-9]+\.[0-9]+", version)
                          else "malformed_version")
                _fail(CatalogErrorCode.TOOL_VERSION, stage="version", tool_exit=0, tool_reason=reason)
            staged_binary = []
            for locale in ("en", "de"):
                relative = f"messages_{locale}.qm"
                path = stage / relative
                _run_tool(layout.tool, ["-fail-on-unfinished", str(stage / f"qt/messages_{locale}.ts"),
                                        "-qm", str(path)], request.tool_timeout_ms, locale)
                staged_binary.append(_staged(path, relative, locale))
            staged_binary.sort(key=lambda artifact: artifact.relative_path)
            try:
                current = _read_source(request.source)
                if (current.resolved != source.resolved or current.identity != source.identity or
                        current.data != source.data or _layout(request, current) != layout):
                    _fail(CatalogErrorCode.IO_FAILURE, stage="paths")
            except CatalogFailure:
                _fail(CatalogErrorCode.IO_FAILURE, stage="paths")
            result = BuildResult(len(catalog.messages), tuple(_digest(a) for a in staged_text),
                                 tuple(_digest(a) for a in staged_binary), "6.12.0")
            for artifacts, root in ((staged_text, layout.text), (staged_binary, layout.binary)):
                for artifact in artifacts:
                    if check:
                        _compare(artifact, root / artifact.relative_path)
                    else:
                        _publish(artifact, root / artifact.relative_path)
            return result
    except OSError:
        _fail(CatalogErrorCode.IO_FAILURE, stage="publication")


def generate_catalog(request: BuildRequest) -> BuildResult:
    return _build(request, check=False)


def check_catalog(request: BuildRequest) -> BuildResult:
    return _build(request, check=True)


def _error_record(code: str, error: CatalogFailure | None = None) -> None:
    record = {"status": "error", "code": code,
              "message_id": error.message_id if error else None,
              "locale": error.locale if error else None,
              "parameter": error.parameter if error else None,
              "stage": error.stage if error else None,
              "tool_exit": error.tool_exit if error else None,
              "tool_reason": error.tool_reason if error else None}
    sys.stderr.write(json.dumps(record, ensure_ascii=True, separators=(",", ":")) + "\n")


def main(argv: Sequence[str] | None = None) -> int:
    try:
        arguments = list(sys.argv[1:] if argv is None else argv)
        if arguments == ["--help"]:
            sys.stdout.write(_HELP)
            return 0
        if arguments == ["--version"]:
            sys.stdout.write("translation-catalog 1\n")
            return 0
        valid = (bool(arguments) and all(type(arg) is str for arg in arguments) and
                 arguments[0] in ("validate", "generate", "check") and len(arguments) % 2 == 1)
        flags = {}
        if valid:
            expected = {"--source"} if arguments[0] == "validate" else {
                "--source", "--text-output", "--binary-output", "--lrelease"}
            for index in range(1, len(arguments), 2):
                key, value = arguments[index:index + 2]
                if key not in expected or key in flags or value.startswith("--"):
                    valid = False
                    break
                flags[key] = value
            valid = valid and flags.keys() == expected
        if not valid:
            _error_record("invalid_cli_arguments")
            return 64
        operation = arguments[0]
        if operation == "validate":
            message_count = len(validate_catalog(_read_source(flags["--source"]).data).messages)
            result = None
        else:
            request = BuildRequest(flags["--source"], flags["--text-output"],
                                   flags["--binary-output"], flags["--lrelease"])
            result = check_catalog(request) if operation == "check" else generate_catalog(request)
            message_count = result.message_count

        def summaries(items):
            return [{"path": a.relative_path, "bytes": a.byte_count, "sha256": a.sha256} for a in items]
        record = {"status": "ok", "operation": operation, "message_count": message_count,
                  "qt_version": result.qt_version if result else None,
                  "text": summaries(result.text) if result else [],
                  "binary": summaries(result.binary) if result else []}
        sys.stdout.write(json.dumps(record, ensure_ascii=True, separators=(",", ":")) + "\n")
        return 0
    except CatalogFailure as error:
        _error_record(error.code.value, error)
        return {CatalogErrorCode.GENERATED_MISMATCH: 1, CatalogErrorCode.IO_FAILURE: 74,
                CatalogErrorCode.TOOL_UNAVAILABLE: 69, CatalogErrorCode.TOOL_VERSION: 69,
                CatalogErrorCode.TOOL_FAILED: 70}.get(error.code, 65)
    except Exception:
        _error_record("internal_error")
        return 70


if __name__ == "__main__":
    raise SystemExit(main())
