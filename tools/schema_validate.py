#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded public parent for the isolated offline schema-package worker.

The parent admits caller/control strings and owns one child's pipes and lifetime.
Package snapshots, exact JSON and standards-library work belong to the worker.
"""
from enum import Enum
import errno
import os
import re
import select
import stat
import subprocess
import sys
import time
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
_WORKER_LINES = MappingProxyType({
    ('{"code":"' + outcome.value + '"}\n').encode("ascii"): outcome
    for outcome in SchemaValidationOutcome
    if outcome is not SchemaValidationOutcome.usage_error
})
_SYSCONFIG_OVERRIDES = (
    "_PYTHON_SYSCONFIGDATA_NAME", "_PYTHON_SYSCONFIGDATA_PATH", "_PYTHON_PROJECT_BASE"
)
_URI_SCHEME = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")
_CHILD_ENVIRONMENT = MappingProxyType({
    "PATH": "/usr/bin:/bin", "LC_ALL": "C.UTF-8", "LANG": "C.UTF-8"
})


class _Deadline(Exception):
    pass


class _ProtocolFailure(Exception):
    pass


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


def _regular_venv_config(path):
    """Own one bounded trusted configuration handle; never parse package bytes."""
    descriptor = os.open(path, os.O_RDONLY | os.O_CLOEXEC | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        before = os.fstat(descriptor)
        if not stat.S_ISREG(before.st_mode) or before.st_size > 4096:
            return False
        raw = os.read(descriptor, 4097)
        after = os.fstat(descriptor)
        identity = lambda entry: (entry.st_dev, entry.st_ino, entry.st_mode,
                                  entry.st_size, entry.st_mtime_ns, entry.st_ctime_ns)
        return len(raw) == before.st_size and len(raw) <= 4096 and identity(before) == identity(after)
    finally:
        os.close(descriptor)


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
        executable = os.path.normpath(executable)  # Preserve the venv symlink spelling.
        binary_directory = os.path.dirname(executable)
        if os.path.basename(binary_directory) != "bin":
            return None
        if not _bounded_path(sys.prefix, absolute=True) or not _bounded_path(sys.base_prefix, absolute=True):
            return None
        prefix = os.path.realpath(sys.prefix)
        venv_root = os.path.realpath(os.path.dirname(binary_directory))
        if (prefix == os.path.realpath(sys.base_prefix) or prefix != venv_root
                or not _bounded_path(prefix, absolute=True)
                or not _regular_venv_config(os.path.join(prefix, "pyvenv.cfg"))):
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


def _pipe(owned):
    """The frozen os.pipe seam owns both ends, including partial setup failures."""
    read_end, write_end = os.pipe()
    owned.update((read_end, write_end))
    if write_end < 3:
        import fcntl
        replacement = fcntl.fcntl(write_end, fcntl.F_DUPFD_CLOEXEC, 3)
        owned.add(replacement)
        _close(write_end, owned)
        write_end = replacement
    return read_end, write_end


def _close(descriptor, owned):
    owned.remove(descriptor)
    os.close(descriptor)


def _remaining(deadline):
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise _Deadline()
    return remaining


def _receive(process, activation, outcome, owned, deadline):
    """Admit complete bounded channels and normal exit, never a JSON prefix."""
    buffers = {activation: bytearray(2), outcome: bytearray(257)}
    views = {descriptor: memoryview(buffer) for descriptor, buffer in buffers.items()}
    lengths = {activation: 0, outcome: 0}
    pending = {activation, outcome}
    poller = select.poll()
    for descriptor in pending:
        os.set_blocking(descriptor, False)
        poller.register(descriptor, select.POLLIN | select.POLLHUP | select.POLLERR)
    while pending:
        events = poller.poll(max(1, int(_remaining(deadline) * 1000)))
        if not events:
            _remaining(deadline)
            continue
        for descriptor, event in events:
            _remaining(deadline)
            if descriptor not in pending or event & select.POLLNVAL:
                raise _ProtocolFailure()
            try:
                count = os.readv(descriptor, (views[descriptor][lengths[descriptor]:],))
            except BlockingIOError:
                continue
            if not count:
                pending.remove(descriptor)
                poller.unregister(descriptor)
                _close(descriptor, owned)
                continue
            lengths[descriptor] += count
            _remaining(deadline)
            if descriptor == activation and (lengths[activation] != 1 or buffers[activation][0] != 65):
                raise _ProtocolFailure()
            if descriptor == outcome and lengths[outcome] > 256:
                raise _ProtocolFailure()
    try:
        status = process.wait(timeout=_remaining(deadline))
    except subprocess.TimeoutExpired:
        raise _Deadline() from None
    _remaining(deadline)
    received = views[outcome][:lengths[outcome]]
    result = next((value for line, value in _WORKER_LINES.items() if received == line), None)
    if result is None or status != _EXIT_STATUS[result]:
        raise _ProtocolFailure()
    if lengths[activation] != 1 and not (lengths[activation] == 0 and result is SchemaValidationOutcome.environment_unavailable):
        raise _ProtocolFailure()
    return result


def _worker_result(path, installation):
    """One owner closes every descriptor and reaps on return or cancellation."""
    owned = set()
    process = None
    result = SchemaValidationOutcome.validation_unavailable
    cleanup_ok = True
    try:
        activation_read, activation_write = _pipe(owned)
        outcome_read, outcome_write = _pipe(owned)
        executable, worker, sdk = installation
        argv = [executable, "-I", "-S", worker, "--manifest", path, "--sdk-dir", sdk,
                "--activation-fd", str(activation_write), "--outcome-fd", str(outcome_write)]
        deadline = time.monotonic() + 10.0
        process = subprocess.Popen(
            argv, shell=False, close_fds=True, stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            pass_fds=(activation_write, outcome_write), env=dict(_CHILD_ENVIRONMENT))
        _close(activation_write, owned)
        _close(outcome_write, owned)
        result = _receive(process, activation_read, outcome_read, owned, deadline)
    except _Deadline:
        result = SchemaValidationOutcome.resource_limit
    except MemoryError:
        result = SchemaValidationOutcome.resource_limit
    except _ProtocolFailure:
        result = SchemaValidationOutcome.validation_unavailable
    except OSError as error:
        if error.errno == errno.ENOMEM:
            result = SchemaValidationOutcome.resource_limit
        elif error.errno in (errno.ENOSYS, errno.ENOTSUP):
            result = SchemaValidationOutcome.environment_unavailable
        else:
            result = SchemaValidationOutcome.validation_unavailable
    except (ValueError, AttributeError):
        result = SchemaValidationOutcome.validation_unavailable
    finally:
        while owned:
            descriptor = next(iter(owned))
            try:
                _close(descriptor, owned)
            except OSError:
                cleanup_ok = False
        if process is not None:
            try:
                if process.returncode is None:
                    try:
                        process.kill()
                    except ProcessLookupError:
                        pass
            finally:
                try:
                    process.wait()
                except (OSError, subprocess.SubprocessError):
                    cleanup_ok = False
    return result if cleanup_ok else SchemaValidationOutcome.validation_unavailable


def validate_package(manifest_path: str) -> SchemaValidationOutcome:
    """Observe one controlled worker snapshot; return no package data or handles."""
    if not _bounded_path(manifest_path) or _URI_SCHEME.match(manifest_path):
        return SchemaValidationOutcome.invalid_path
    try:
        installation = _installation()
        if installation is None:
            return SchemaValidationOutcome.environment_unavailable
        return _worker_result(manifest_path, installation)
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
