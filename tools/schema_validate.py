#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded public parent for the isolated offline schema-package worker.

Python admits bounded paths and calls one native owner before decoding its result.
All child/pipe/configuration ownership ends inside C; package work stays in the worker.
"""
from enum import Enum
import os
import re
import stat
import sys
from types import MappingProxyType


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
    """Retained baseline exception declaration; implemented calls do not raise it."""

    def __init__(self):
        super().__init__(
            "Schema package validation is intentionally unimplemented; review tests first"
        )


class SchemaNativeBindingNotImplemented(RuntimeError):
    """Retained binding-baseline declaration; implemented calls do not raise it."""

    def __init__(self):
        super().__init__("Native schema binding is intentionally unimplemented")


def _load_native_transport():
    """Admit only the fixed trusted ELF, then configure its genuine builtin ABI."""
    try:
        if (sys.platform != "linux" or sys.byteorder != "little"
                or os.uname().machine != "x86_64" or not _bounded_path(__file__)):
            return None
        import ctypes
        if (ctypes.sizeof(ctypes.c_ubyte), ctypes.sizeof(ctypes.c_int),
                ctypes.sizeof(ctypes.c_uint), ctypes.sizeof(ctypes.c_void_p)) != (1, 4, 4, 8):
            return None
        root = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        artifact = os.path.join(root, "build", "schema-transport-debug", "lib",
                                "libsubathon_schema_transport_v1.so")
        if not _bounded_path(artifact, absolute=True):
            return None
        expected = os.stat(artifact, follow_symlinks=False)
        if not stat.S_ISREG(expected.st_mode) or expected.st_size < 64:
            return None

        def identity(value):
            return (value.st_dev, value.st_ino, value.st_mode, value.st_size,
                    value.st_mtime_ns, value.st_ctime_ns)

        # FileIO owns the native handle before Python can observe a numeric FD.
        # No raw os.open result/publication or file handle crosses the C call.
        stable, header = False, b""
        source = open(artifact, "rb", buffering=0)
        try:
            before = os.fstat(source.fileno())
            if identity(before) == identity(expected):
                header = source.read(64)
                after = os.fstat(source.fileno())
                stable = identity(before) == identity(after)
        except BaseException:
            try:
                source.close()
            except BaseException:
                pass  # Preserve the original interruption/allocation exception.
            raise
        else:
            source.close()
        if (not stable or len(header) != 64 or header[:7] != b"\x7fELF\x02\x01\x01"
                or header[7] not in (0, 3)
                or header[16:24] != b"\x03\x00\x3e\x00\x01\x00\x00\x00"
                or header[52:54] != b"\x40\x00"):
            return None
        # The path remains a trusted reviewed build; this is not authentication
        # against an administrator replacing it between header read and dlopen.
        library = ctypes.CDLL(artifact, mode=os.RTLD_NOW | os.RTLD_LOCAL)
        entry = library.subathon_schema_transport_v1
        entry.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_uint] * 4
        entry.restype = ctypes.c_int
        return entry
    except (OSError, ImportError, AttributeError, ValueError):
        return None


_NATIVE_OUTCOMES = (
    SchemaValidationOutcome.ok,
    SchemaValidationOutcome.validation_unavailable,  # Native1 is reserved to CLI.
    SchemaValidationOutcome.invalid_path,
    SchemaValidationOutcome.io_unavailable,
    SchemaValidationOutcome.environment_unavailable,
    SchemaValidationOutcome.resource_limit,
    SchemaValidationOutcome.invalid_json,
    SchemaValidationOutcome.invalid_manifest,
    SchemaValidationOutcome.unsupported_dialect,
    SchemaValidationOutcome.unsupported_format,
    SchemaValidationOutcome.dependency_unavailable,
    SchemaValidationOutcome.invalid_schema,
    SchemaValidationOutcome.unresolved_reference,
    SchemaValidationOutcome.reference_cycle,
    SchemaValidationOutcome.invalid_inventory,
    SchemaValidationOutcome.unsupported_feature,
    SchemaValidationOutcome.invalid_instance,
    SchemaValidationOutcome.validation_unavailable,
)


def _decode_native_result(result: int) -> SchemaValidationOutcome:
    """Decode only after native quiescence; integer18 never invents a signal."""
    if type(result) is int and 0 <= result < len(_NATIVE_OUTCOMES):
        return _NATIVE_OUTCOMES[result]
    # Includes interrupted18, forbidden completed-native31 and unknown integers.
    return SchemaValidationOutcome.validation_unavailable


_EXIT_STATUS = MappingProxyType({
    outcome: (0 if outcome is SchemaValidationOutcome.ok else
              64 if outcome is SchemaValidationOutcome.usage_error else
              4 if outcome is SchemaValidationOutcome.resource_limit else
              3 if outcome in (SchemaValidationOutcome.io_unavailable,
                               SchemaValidationOutcome.environment_unavailable,
                               SchemaValidationOutcome.dependency_unavailable,
                               SchemaValidationOutcome.validation_unavailable) else 2)
    for outcome in SchemaValidationOutcome
})
_SYSCONFIG_OVERRIDES = (
    "_PYTHON_SYSCONFIGDATA_NAME", "_PYTHON_SYSCONFIGDATA_PATH", "_PYTHON_PROJECT_BASE"
)
_URI_SCHEME = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")


def _bounded_path(value, absolute=False):
    """Inspect only an exact builtin string, before encoding bounded contents."""
    if type(value) is not str or not 1 <= len(value) <= 4096 or "\0" in value:
        return False
    try:
        if len(value.encode("utf-8")) > 4096:
            return False
    except UnicodeEncodeError:
        return False
    return not absolute or os.path.isabs(value)


def _installation():
    """Bind the actual ordinary Linux venv without importing a selected backend."""
    # -I does not neutralize these direct sysconfig environment reads.
    if any(key in os.environ for key in _SYSCONFIG_OVERRIDES):
        return None
    try:
        if (sys.implementation.name != "cpython" or sys.platform != "linux"
                or sys.version_info[:2] not in ((3, 10), (3, 12), (3, 14))
                or os.uname().machine != "x86_64"):
            return None
        import sysconfig
        minor = sys.version_info.minor
        if (sysconfig.get_config_var("SOABI") != "cpython-3" + str(minor) + "-x86_64-linux-gnu"
                or bool(sysconfig.get_config_var("Py_DEBUG"))
                or bool(sysconfig.get_config_var("Py_GIL_DISABLED"))):
            return None
        executable = sys.executable
        if not _bounded_path(executable, absolute=True):
            return None
        binary_directory = os.path.dirname(os.path.normpath(executable))
        if os.path.basename(binary_directory) != "bin":
            return None
        if not _bounded_path(sys.prefix, absolute=True) or not _bounded_path(sys.base_prefix, absolute=True):
            return None
        prefix = os.path.realpath(sys.prefix)
        venv_root = os.path.realpath(os.path.dirname(binary_directory))
        if (prefix == os.path.realpath(sys.base_prefix) or prefix != venv_root
                or not _bounded_path(prefix, absolute=True)):
            return None
        paths = sysconfig.get_paths()
        purelib, platlib = paths.get("purelib"), paths.get("platlib")
        if not _bounded_path(purelib, absolute=True) or not _bounded_path(platlib, absolute=True):
            return None
        sdk = os.path.realpath(purelib)
        if (sdk != os.path.realpath(platlib) or not _bounded_path(sdk, absolute=True)
                or not stat.S_ISDIR(os.stat(sdk).st_mode)):
            return None
        worker = os.path.abspath(os.path.join(os.path.dirname(__file__), "schema_validation", "worker.py"))
        if not _bounded_path(worker, absolute=True):
            return None
        return executable, worker, sdk
    except (OSError, ValueError, TypeError, AttributeError, ImportError):
        return None


def _worker_result(manifest, installation, entry):
    """Prepare private immutable inputs, call C once, then decode after cleanup."""
    import ctypes
    paths = (manifest, *installation)
    payloads = tuple(value.encode("utf-8") for value in paths)
    buffers = tuple(value + b"\0" for value in payloads)
    pointer = ctypes.POINTER(ctypes.c_ubyte)
    pointers = tuple(ctypes.cast(ctypes.c_char_p(value), pointer) for value in buffers)
    arguments = (pointers[0], len(payloads[0]), pointers[1], len(payloads[1]),
                 pointers[2], len(payloads[2]), pointers[3], len(payloads[3]))
    # The immutable bytes/pointer tuples remain live until the one call returns.
    result = entry(*arguments)
    return _decode_native_result(result)


def validate_package(manifest_path: str) -> SchemaValidationOutcome:
    """Observe one controlled worker snapshot; return no package data or handles."""
    try:
        if not _bounded_path(manifest_path) or _URI_SCHEME.match(manifest_path):
            return SchemaValidationOutcome.invalid_path
        installation = _installation()
        if installation is None:
            return SchemaValidationOutcome.environment_unavailable
        entry = _load_native_transport()
        if entry is None:
            return SchemaValidationOutcome.environment_unavailable
        return _worker_result(manifest_path, installation, entry)
    except MemoryError:
        return SchemaValidationOutcome.resource_limit


def main(argv: list[str] | tuple[str, ...] | None = None) -> int:
    """Admit at most two exact argv strings before dispatch or fixed output."""
    arguments = sys.argv if argv is None else argv
    offset = 1 if argv is None else 0
    result = SchemaValidationOutcome.usage_error
    if type(arguments) in (list, tuple):
        count = len(arguments) - offset
        if count in (1, 2):
            first = arguments[offset]
            second = arguments[offset + 1] if count == 2 else None
            if type(first) is str and (count == 1 or type(second) is str):
                if count == 1 and first == "--help":
                    sys.stdout.write("Usage: schema_validate.py --manifest PATH\n")
                    return 0
                if count == 2 and first == "--manifest":
                    result = validate_package(second)
    sys.stdout.write('{"code":"' + result.value + '"}\n')
    return _EXIT_STATUS[result]


if __name__ == "__main__":
    raise SystemExit(main())
