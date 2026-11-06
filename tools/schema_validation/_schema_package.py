# SPDX-License-Identifier: GPL-3.0-or-later
"""Owned no-follow file snapshots and strict, preflight-bounded JSON decoding."""
import errno
import json
import os
import re
import stat
from contextlib import ExitStack
from fractions import Fraction
from itertools import chain

from _schema_common import Refusal, refuse, pointer_ok, scalar_string, utf8_size


PATH = re.compile(r"[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)*\Z", re.ASCII)
NUMBER = re.compile(r"-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\Z", re.ASCII)


def relative_path(value):
    if (not scalar_string(value) or not value.isascii() or not 1 <= len(value) <= 256
            or not PATH.fullmatch(value) or any(p in {".", ".."} for p in value.split("/"))):
        refuse("invalid_path")
    return value


def os_error(error):
    if error.errno in {errno.ELOOP, errno.ENOTDIR, errno.EISDIR}:
        refuse("invalid_path")
    if error.errno in {errno.ENOSYS, errno.ENOTSUP}:
        refuse("environment_unavailable")
    if error.errno in {errno.ENOMEM, errno.EMFILE, errno.ENFILE}:
        refuse("resource_limit")
    refuse("io_unavailable")


def signature(info):
    return (info.st_dev, info.st_ino, info.st_mode, info.st_size,
            info.st_mtime_ns, info.st_ctime_ns)


class Files:
    """All handles live in one ExitStack, including traversal and repeated aliases."""
    def __init__(self):
        self.owned = ExitStack()
        self.descriptors = set()
        self.links = []
        self.total = 0

    def close(self, fd):
        if fd in self.descriptors:
            self.descriptors.remove(fd)
            os.close(fd)

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return self.owned.__exit__(*args)

    def opened(self, name, directory=None, want_directory=False):
        flags = os.O_RDONLY | os.O_CLOEXEC | os.O_NOFOLLOW | os.O_NONBLOCK
        if want_directory:
            flags |= os.O_DIRECTORY
        try:
            fd = os.open(name, flags, dir_fd=directory)
            self.descriptors.add(fd)
            self.owned.callback(self.close, fd)
            info = os.fstat(fd)
            if not (stat.S_ISDIR(info.st_mode) if want_directory else stat.S_ISREG(info.st_mode)):
                refuse("invalid_path")
            if directory is not None and not want_directory:
                self.links.append((directory, name, info.st_dev, info.st_ino, want_directory))
            return fd, info
        except OSError as error:
            os_error(error)

    def manifest(self, path):
        if (type(path) is not str or not scalar_string(path) or not 1 <= utf8_size(path) <= 4096
                or "\0" in path or re.match(r"^[A-Za-z][A-Za-z0-9+.-]*:", path)):
            refuse("invalid_path")
        root, _ = self.opened("/" if path.startswith("/") else ".", want_directory=True)
        parts = path.split("/")
        for part in parts[:-1]:
            if part and part != ".":
                previous = root
                root, _ = self.opened(part, root, True)
                self.close(previous)
        if not parts[-1] or parts[-1] in {".", ".."}:
            refuse("invalid_path")
        fd, info = self.opened(parts[-1], root)
        self.root = root
        self.manifest_identity = info.st_dev, info.st_ino
        return self.read(fd, info, 65536)

    def member(self, path):
        directory = self.root
        parts = path.split("/")
        for part in parts[:-1]:
            previous = directory
            directory, _ = self.opened(part, directory, True)
            if previous != self.root:
                self.close(previous)
        return self.opened(parts[-1], directory)

    def read(self, fd, before, ceiling):
        if before.st_size > ceiling or before.st_size < 0:
            refuse("resource_limit")
        try:
            data = os.read(fd, before.st_size + 1)
            if len(data) > ceiling:
                refuse("resource_limit")
            if len(data) != before.st_size or os.read(fd, 1) or signature(os.fstat(fd)) != signature(before):
                refuse("io_unavailable")
            self.total += len(data)
            if self.total > 8388608:
                refuse("resource_limit")
            return data
        except OSError as error:
            os_error(error)

    def stable(self):
        try:
            for parent, name, dev, ino, directory in self.links:
                info = os.stat(name, dir_fd=parent, follow_symlinks=False)
                if ((info.st_dev, info.st_ino) != (dev, ino)
                        or not (stat.S_ISDIR(info.st_mode) if directory else stat.S_ISREG(info.st_mode))):
                    refuse("io_unavailable")
        except OSError:
            refuse("io_unavailable")


def numeric_limits(token):
    if len(token) > 512:
        refuse("resource_limit")
    parts = re.split("[eE]", token, maxsplit=1)
    if sum(c.isdigit() for c in parts[0]) > 256:
        refuse("resource_limit")
    if len(parts) == 2:
        exp = parts[1].lstrip("+-").lstrip("0")
        if exp.isascii() and exp.isdigit() and (len(exp) > 4 or int(exp) > 4096):
            refuse("resource_limit")


def preflight(raw):
    # This scan bounds lexical ownership BEFORE the standard JSON decoder. It does
    # not accept JSON grammar; only json.loads decides complete syntax afterward.
    if raw.startswith(b"\xef\xbb\xbf"):
        refuse("invalid_json")
    try:
        text = raw.decode("utf-8", "strict")
    except UnicodeDecodeError:
        refuse("invalid_json")
    stack = []  # True=object currently expecting a key, False=array/value context.
    count = 0
    i = 0
    while i < len(raw):
        c = raw[i]
        if c in b" \t\r\n,:}":
            if c == 58 and stack and stack[-1] is not None:
                stack[-1] = False
            elif c == 44 and stack and stack[-1] is not None:
                stack[-1] = True
            elif c == 125 and stack:
                stack.pop()
            i += 1
            continue
        if c == 93:
            if stack:
                stack.pop()
            i += 1
            continue
        is_key = bool(stack and stack[-1] is True)
        if not is_key:
            count += 1
            if count > 65536:
                refuse("resource_limit")
        if c in (123, 91):
            stack.append(True if c == 123 else None)
            if len(stack) > 64:
                refuse("resource_limit")
            i += 1
        elif c == 34:
            start = i
            i += 1
            while i < len(raw):
                if raw[i] == 92:
                    # An absent escaped byte is a grammar error, not an extra
                    # raw byte. Count only actual bytes before strict decoding.
                    i += 2 if i + 1 < len(raw) else 1
                elif raw[i] == 34:
                    i += 1
                    break
                else:
                    i += 1
                if i - start > 65536:
                    refuse("resource_limit")
            if i - start > 65536:
                refuse("resource_limit")
        else:
            start = i
            while i < len(raw) and raw[i] not in b" \t\r\n,:{}[]\"":
                i += 1
            if i == start:
                i += 1
            if c in b"-0123456789":
                # Admit the span BEFORE copying/decoding it; a malformed numeric
                # token may occupy the complete file. Grammar remains json.loads.
                if i - start > 512:
                    refuse("resource_limit")
                numeric_limits(raw[start:i].decode("ascii", "ignore"))
    return text


class Lexeme:
    def __init__(self, text):
        self.text = text


def exact_number(text):
    numeric_limits(text)
    if not NUMBER.fullmatch(text):
        refuse("invalid_json")
    pieces = re.split("[eE]", text, maxsplit=1)
    exponent = int(pieces[1]) if len(pieces) == 2 else 0
    mantissa = pieces[0]
    scale = len(mantissa.split(".")[1]) if "." in mantissa else 0
    power = exponent - scale
    digits = sum(c.isdigit() for c in mantissa)
    if ((digits + max(power, 0)) * 3322 + 999) // 1000 + 1 > 16384 or (abs(min(power, 0)) * 3322 + 999) // 1000 + 1 > 16384:
        refuse("resource_limit")
    number = int(mantissa.replace(".", ""))
    value = Fraction(number * 10 ** power, 1) if power >= 0 else Fraction(number, 10 ** -power)
    if value.numerator.bit_length() > 16384 or value.denominator.bit_length() > 16384:
        refuse("resource_limit")
    return value.numerator if value.denominator == 1 else value


def decode(raw):
    text = preflight(raw)
    def pairs(items):
        value = {}
        for name, item in items:
            if name in value:
                refuse("invalid_json")
            value[name] = item
        return value
    def constant(_value):
        refuse("invalid_json")
    try:
        result = json.loads(text, object_pairs_hook=pairs, parse_int=Lexeme,
                            parse_float=Lexeme, parse_constant=constant)
    except (ValueError, RecursionError):
        refuse("invalid_json")
    del text
    holder = [result]
    pending = [(holder, 0)]
    while pending:
        parent, key = pending.pop()
        value = parent[key]
        if isinstance(value, Lexeme):
            parent[key] = exact_number(value.text)
        elif type(value) is str:
            if not scalar_string(value):
                refuse("invalid_json")
        elif type(value) is dict:
            for name in value:
                if not scalar_string(name):
                    refuse("invalid_json")
                pending.append((value, name))
        elif type(value) is list:
            pending.extend((value, i) for i in range(len(value)))
    return holder[0]


def manifest_shape(value, budget, package_keys):
    with budget.lease() as temporary:
        documents, bindings, external = _manifest_shape(value, temporary)
        # The returned external-order list remains live through actual reads.
        temporary.transfer(package_keys, sum(utf8_size(name) for name in external))
        return documents, bindings, external


def _manifest_shape(value, keys):
    if type(value) is not dict or set(value) != {"profile", "documents", "examples"} or value["profile"] != "subathon.schema-package.v1":
        refuse("invalid_manifest")
    docs, bindings = value["documents"], value["examples"]
    if type(docs) is not list or type(bindings) is not list or not docs:
        refuse("invalid_manifest")
    if len(docs) > 32 or len(bindings) > 256:
        refuse("resource_limit")
    names = set()
    oas = 0
    for doc in docs:
        if type(doc) is not dict or set(doc) != {"path", "kind"} or doc["kind"] not in ("openapi31", "jsonschema202012"):
            refuse("invalid_manifest")
        name = relative_path(doc["path"])
        if name in names:
            refuse("invalid_manifest")
        keys.claim(name)
        names.add(name)
        oas += doc["kind"] == "openapi31"
    if oas > 1:
        refuse("invalid_manifest")
    identities = set()
    external = []
    for binding in bindings:
        if type(binding) is not dict or set(binding) != {"path", "schema_document", "schema_pointer"}:
            refuse("invalid_manifest")
        target = relative_path(binding["schema_document"])
        if target not in names or not pointer_ok(binding["schema_pointer"]):
            refuse("invalid_manifest")
        source = binding["path"]
        if type(source) is str:
            relative_path(source)
            if source in names:
                refuse("invalid_manifest")
            if source not in external:
                keys.claim(source)
                external.append(source)
            keys.claim("external", source, target, binding["schema_pointer"])
            identity = ("external", source, target, binding["schema_pointer"])
        elif type(source) is dict and set(source) == {"document", "pointer"}:
            relative_path(source["document"])
            if source["document"] not in names or not pointer_ok(source["pointer"]):
                refuse("invalid_manifest")
            keys.claim("inline", source["document"], source["pointer"], target, binding["schema_pointer"])
            identity = ("inline", source["document"], source["pointer"], target, binding["schema_pointer"])
        else:
            refuse("invalid_manifest")
        if identity in identities:
            refuse("invalid_manifest")
        identities.add(identity)
    return docs, bindings, external


def load_package(path, budget):
    with budget.lease() as working, Files() as files:
        manifest_raw = files.manifest(path)
        manifest = decode(manifest_raw)
        documents, bindings, external = manifest_shape(manifest, budget, working)
        specs = {}
        for document in documents:
            budget.key(document["path"])
            specs[document["path"]] = document["kind"]
        handles = []
        seen = {files.manifest_identity: (None, None)}
        aliases = {}
        projected = len(manifest_raw)
        for name in chain(specs, external):
            fd, info = files.member(name)
            identity = info.st_dev, info.st_ino
            if identity in seen:
                previous, _ = seen[identity]
                if name in specs or previous in specs or previous is None:
                    refuse("invalid_manifest")
                budget.key(name)
                budget.key(previous)
                aliases[name] = previous
                continue
            if info.st_size > 1048576:
                refuse("resource_limit")
            projected += info.st_size
            if projected > 8388608:
                refuse("resource_limit")
            working.claim(name)
            seen[identity] = (name, fd)
            budget.key(name)
            budget.key(name)
            aliases[name] = name
            working.claim(name)
            handles.append((name, fd, info))
        raw, values = {None: manifest_raw}, {}
        for name, fd, info in handles:
            budget.key(name)
            raw[name] = files.read(fd, info, 1048576)
            budget.key(name)
            values[name] = decode(raw[name])
        for alias, actual in aliases.items():
            if alias not in values:
                budget.key(alias)
                values[alias] = values[actual]
        files.stable()
        return manifest, specs, bindings, raw, values, aliases
