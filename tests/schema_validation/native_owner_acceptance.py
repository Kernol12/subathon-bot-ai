#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Native negative source: one closed --row per fresh test process.

Review and BOTH-host helper foundations precede any run. Import defines only
stdlib helpers, literal rows and tests; no candidate/library/SDK discovery occurs.
"""

import argparse
import contextlib
import ctypes
import errno
import json
import os
from pathlib import Path
import select
import signal
import sys
import tempfile
import time
import unittest


# id, case, phase, errno, expected native integer, born, allowed si_status,
# minimum selected-phase adoptions, actual release-before-after-checkpoint.
ACTIVATION = (
    ("a1", 1, 0, 0, 17, True, (0, 9), 0, False),
    ("a2", 2, 0, 0, 17, True, (0, 9), 0, False),
    ("a3", 3, 0, 0, 17, True, (0, 9), 0, False),
)
OUTCOME = (
    ("o4", 4, 0, 0, 17, True, (0, 9), 0, False),
    ("o5", 5, 0, 0, 17, True, (0, 9), 0, False),
    ("o6", 6, 0, 0, 17, True, (0, 9), 0, False),
    ("o7", 7, 0, 0, 17, True, (0, 9), 0, False),
    ("o8", 8, 0, 0, 17, True, (64, 9), 0, False),
    ("o9", 9, 0, 0, 17, True, (0, 9), 0, False),
    ("o10", 10, 0, 0, 17, True, (2, 9), 0, False),
)
STATUS = (
    ("s11", 11, 0, 0, 17, True, (0, 9), 0, False),
    ("s12", 12, 0, 0, 17, True, (9,), 0, False),
)
SETUP = (
    ("e13", 13, 0, 0, 4, True, (3,), 0, False),
    ("e14", 14, 0, 0, 17, True, (0, 9), 0, False),
    ("e15", 15, 0, 0, 17, True, (3, 9), 0, False),
    ("e16", 16, 0, 0, 17, True, (3, 9), 0, False),
    ("e17", 17, 0, 0, 17, True, (3, 9), 0, False),
    ("e18", 18, 0, 0, 17, True, (3, 9), 0, False),
    ("e19", 19, 0, 0, 17, True, (3, 9), 0, False),
    ("e20", 20, 0, 0, 17, True, (3, 9), 0, False),
    ("e21", 21, 0, 0, 17, True, (0, 9), 0, False),
    ("e22", 22, 0, 0, 17, True, (2, 9), 0, False),
)
DEADLINE = (("d23", 23, 0, 0, 5, True, (9,), 0, False),)
PREBIRTH = (
    ("p1", 24, 2, errno.EACCES, 4, False, (), 0, False),
    ("p2", 24, 8, errno.ENOSYS, 4, False, (), 0, False),
    ("p3", 24, 10, errno.ENFILE, 5, False, (), 0, False),
    ("p4", 24, 12, errno.EMFILE, 5, False, (), 0, False),
    ("p5", 24, 14, errno.EACCES, 4, False, (), 0, False),
    ("p6", 24, 16, errno.ENOSYS, 4, False, (), 0, False),
    ("p7", 24, 16, errno.EAGAIN, 5, False, (), 0, False),
    ("p8", 24, 16, errno.EPERM, 4, False, (), 0, False),
)
WORK = (
    ("w1", 24, 4, errno.EINTR, 18, False, (), 0, False),
    ("w2", 24, 7, errno.EINTR, 18, False, (), 0, True),
    ("w3", 24, 9, errno.ENOMEM, 5, False, (), 2, False),
    ("w4", 24, 11, errno.ENOMEM, 5, False, (), 2, False),
    ("w5", 24, 13, errno.ENOMEM, 5, False, (), 1, False),
    ("w6", 24, 15, errno.ENOMEM, 5, False, (), 1, False),
    ("w7", 24, 17, errno.EINTR, 18, True, (0, 9), 1, False),
    ("w8", 24, 19, errno.EIO, 17, True, (0, 9), 0, True),
    ("w9", 24, 20, errno.EPERM, 4, True, (3,), 0, False),
    ("w10", 24, 22, errno.ENOSYS, 4, True, (3,), 0, False),
    ("w11", 24, 24, errno.ENOSYS, 4, True, (3,), 0, False),
    ("w12", 24, 26, errno.EACCES, 4, True, (3,), 0, False),
    ("w13", 24, 28, errno.EIO, 17, True, (0, 9), 0, False),
    ("w14", 24, 29, errno.EINTR, 18, True, (0, 9), 0, False),
    ("w15", 24, 30, errno.EIO, 17, True, (0, 9), 0, False),
    ("w16", 24, 31, errno.EINTR, 18, True, (0, 9), 0, False),
    ("w17", 24, 32, errno.EIO, 17, True, (0, 9), 0, False),
    ("w18", 24, 32, errno.EINTR, 18, True, (0, 9), 0, False),
)
CLEANUP = (
    ("c1", 24, 34, errno.EINTR, 17, True, (9,), 0, False),
    ("c2", 24, 35, errno.EIO, 17, True, (9,), 0, False),
    ("c3", 24, 37, errno.EIO, 17, True, (0,), 0, True),
)
INTERRUPT = (
    ("i25a", 25, 18, 0, 18, True, (9,), 0, False),
    ("i25b", 25, 19, 0, 18, True, (9,), 0, True),
)
RESOURCE = (("r26", 26, 17, 0, 5, True, (9,), 1, False),)
REAPER = (("l27", 27, 0, 0, 17, True, (0,), 0, False),)
PACKET = (("f28", 28, 0, 0, 17, True, (3, 9), 0, False),)


class SchemaTransportNotImplemented(RuntimeError):
    def __init__(self):
        super().__init__("Native schema transport is intentionally unimplemented")


class FixtureFailure(RuntimeError):
    """Setup, poison or disposal failure cannot count as native missing behavior."""


def installation(artifact):
    overrides = ("_PYTHON_SYSCONFIGDATA_NAME", "_PYTHON_SYSCONFIGDATA_PATH", "_PYTHON_PROJECT_BASE",
                 "LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT", "GLIBC_TUNABLES")
    if any(key in os.environ for key in overrides):
        raise FixtureFailure("Neutral admitted test environment required")
    import sysconfig

    minor = sys.version_info.minor
    if (sys.implementation.name != "cpython" or sys.version_info.major != 3 or minor not in (10, 12, 14)
            or sys.platform != "linux" or os.uname().machine != "x86_64" or sys.byteorder != "little"
            or sysconfig.get_config_var("SOABI") != "cpython-3" + str(minor) + "-x86_64-linux-gnu"
            or bool(sysconfig.get_config_var("Py_DEBUG")) or bool(sysconfig.get_config_var("Py_GIL_DISABLED"))):
        raise FixtureFailure("Ordinary admitted CPython ABI required")
    if (ctypes.sizeof(ctypes.c_int), ctypes.sizeof(ctypes.c_uint), ctypes.sizeof(ctypes.c_void_p)) != (4, 4, 8):
        raise FixtureFailure("Native ABI width mismatch")
    if not all(hasattr(os, name) for name in ("waitid", "P_PIDFD", "P_ALL", "WEXITED", "WNOHANG", "WNOWAIT")):
        raise FixtureFailure("Real nonconsuming pidfd wait oracles unavailable")
    interpreter = Path(sys.executable)
    if (not interpreter.is_absolute() or sys.prefix == sys.base_prefix
            or interpreter.parent.parent.resolve(strict=True) != Path(sys.prefix).resolve(strict=True)):
        raise FixtureFailure("Dedicated reviewed stdlib venv required")
    sdk = Path(sysconfig.get_path("purelib")).resolve(strict=True)
    if sdk != Path(sysconfig.get_path("platlib")).resolve(strict=True) or not sdk.is_dir():
        raise FixtureFailure("One admitted installed SDK directory required")
    root = Path(__file__).resolve(strict=True).parents[2]
    expected = root / "build/schema-transport-debug/lib/libsubathon_schema_fault_v1.so"
    if type(artifact) is not str or not Path(artifact).is_absolute() or Path(artifact) != expected:
        raise FixtureFailure("Exact explicit fault artifact required")
    if not expected.is_file() or expected.is_symlink():
        raise FixtureFailure("Reviewed real fault artifact required")
    worker = root / "tools/schema_validation/worker.py"
    child = root / "build/schema-transport-debug/tests/schema_validation/schema_transport_native_fault_child"
    if not worker.is_file() or not child.is_file() or not os.access(child, os.X_OK):
        raise FixtureFailure("Real worker and reviewed fixed negative child required")
    paths = tuple(str(path).encode("utf-8") for path in (interpreter, worker, sdk))
    if any(not 1 <= len(path) <= 4096 or b"\0" in path for path in paths):
        raise FixtureFailure("Trusted path is unrepresentable")
    return expected, paths


class FaultSession:
    """Reusable TEST context: actual C function + raw oracles, never a decoder.

    Public-port tests may redirect their private artifact loader to entry, the
    actual builtin CDLL function. They still assert their own public result.
    assert_evidence must run after that actual call and BEFORE context disposal.
    """

    def __init__(self, case, artifact, row):
        self.case, self.row = case, row
        expected, self.paths = installation(artifact)
        self.library = ctypes.CDLL(str(expected), mode=os.RTLD_NOW | os.RTLD_LOCAL)
        pointer = ctypes.POINTER(ctypes.c_ubyte)
        self.entry = self.library.subathon_schema_transport_v1
        self.entry.argtypes, self.entry.restype = [pointer, ctypes.c_uint] * 4, ctypes.c_int
        self.arm = self.library.subathon_schema_fault_arm_v1
        self.arm.argtypes, self.arm.restype = [ctypes.c_uint, ctypes.c_uint, ctypes.c_int], ctypes.c_int
        self.snapshot = self.library.subathon_schema_fault_snapshot_v1
        self.snapshot.argtypes, self.snapshot.restype = [ctypes.POINTER(ctypes.c_int64), ctypes.c_uint], ctypes.c_int
        self.report = self.library.subathon_schema_fault_report_v1
        self.report.argtypes, self.report.restype = [pointer, ctypes.c_uint], ctypes.c_int
        self.fallback = self.library.subathon_schema_fault_fallback_v1
        self.fallback.argtypes, self.fallback.restype = [], ctypes.c_int
        self.cells, self.packet = (ctypes.c_int64 * 1024)(), (ctypes.c_ubyte * 248)()
        self.fstat, self.waitid, self.getpid = os.fstat, os.waitid, os.getpid
        self.poller = select.poll()  # poll(), not an epoll FD or a new child.
        self.stack = contextlib.ExitStack()
        self.records, self.initial_fds = [], ()
        self.before_handler = signal.getsignal(signal.SIGINT)
        self.before_mask = signal.pthread_sigmask(signal.SIG_BLOCK, [])
        if row[1] == 25 and (self.before_handler is not signal.default_int_handler
                            or signal.SIGINT in self.before_mask):
            raise FixtureFailure("Real SIGINT rows need the ordinary unblocked Python handler")

    def _snapshot(self):
        count = self.snapshot(self.cells, 1024)
        if type(count) is not int or not 0 <= count <= 128:
            raise FixtureFailure("Invalid or poisoned native journal")
        return [tuple(self.cells[8 * row + column] for column in range(8)) for row in range(count)]

    def _dispose(self):
        if self.fallback() != 0:
            raise FixtureFailure("Native fixture disposal failed")

    def __enter__(self):
        try:
            self.stdout = self.stack.enter_context(tempfile.TemporaryFile())
            self.stderr = self.stack.enter_context(tempfile.TemporaryFile())
            saved_out, saved_err = os.dup(1), -1
            self.stack.callback(os.close, saved_out)
            saved_err = os.dup(2)
            self.stack.callback(os.close, saved_err)
            self.stack.callback(os.dup2, saved_out, 1)
            self.stack.callback(os.dup2, saved_err, 2)
            os.dup2(self.stdout.fileno(), 1)
            os.dup2(self.stderr.fileno(), 2)
            self.stdio = tuple(self.fstat(fd) for fd in (0, 1, 2))
            # Registered BEFORE arm, including arm/setup/31/KeyboardInterrupt errors.
            self.stack.callback(self._dispose)
            _, which, phase, error, _, _, _, _, _ = self.row
            if self.arm(which, phase, error) != 0:
                raise FixtureFailure("Native negative plan could not arm")
            initial = self._snapshot()
            self.initial_fds = tuple(entry[3] for entry in initial
                                     if entry[0] >> 32 == 2 and entry[1] == 3 and entry[3] >= 0)
            if len(self.initial_fds) != 6 or len(set(self.initial_fds)) != 6:
                raise FixtureFailure("Native fixture handle inventory differs")
            for descriptor in self.initial_fds:
                if not 0 <= descriptor <= 0x7fffffff:
                    raise FixtureFailure("Native fixture descriptor is unrepresentable")
                self.fstat(descriptor)
            return self
        except BaseException:
            self.stack.__exit__(*sys.exc_info())
            raise

    def __exit__(self, kind, value, traceback):
        return self.stack.__exit__(kind, value, traceback)

    def arguments(self, manifest):
        payloads = (str(manifest).encode("utf-8"), *self.paths)
        if any(not 1 <= len(value) <= 4096 or b"\0" in value for value in payloads):
            raise FixtureFailure("Native test request is unrepresentable")
        buffers = tuple((ctypes.c_ubyte * (len(value) + 1)).from_buffer_copy(value + b"\0") for value in payloads)
        values = []
        for buffer, payload in zip(buffers, payloads):
            values.extend((buffer, len(payload)))
        return buffers, values  # Caller retains every buffer across the actual C call.

    def _no_children(self, flags):
        try:
            self.waitid(os.P_ALL, 0, os.WEXITED | os.WNOHANG | os.WNOWAIT | flags)
        except ChildProcessError as error:
            return error.errno == errno.ECHILD
        return False  # None is live; a returned status is an unreaped zombie.

    def _guard(self, pid):
        candidates = {entry[3] for entry in self.records if entry[0] >> 32 == 2
                      and entry[1] == 2 and entry[2] == 0 and entry[4] == pid
                      and entry[3] in self.initial_fds and entry[5] == entry[3]}
        identified = []
        flags = os.WEXITED | os.WNOHANG | os.WNOWAIT | 0x20000000
        for descriptor in sorted(candidates):
            try:
                state = self.waitid(os.P_PIDFD, descriptor, flags)
            except ChildProcessError as error:
                if error.errno != errno.ECHILD:
                    raise
                identified.append((descriptor, True))
            except OSError as error:
                # Other bound slot is the capture PIPE, not a pidfd. A missing
                # actual guard still fails the exact-one assertion below.
                if error.errno not in (errno.EINVAL, errno.EBADF):
                    raise
            else:
                identified.append((descriptor, False))  # live/zombie is never gone.
        self.case.assertEqual(len(identified), 1, "Missing/ambiguous actual pinned identity")
        descriptor, reaped = identified[0]
        self.case.assertIn(descriptor, self.initial_fds)
        self.poller.register(descriptor, select.POLLIN)
        events = self.poller.poll(0)
        self.poller.unregister(descriptor)
        self.case.assertEqual(len(events), 1)
        self.case.assertEqual(events[0][0], descriptor)
        self.case.assertNotEqual(events[0][1] & (select.POLLIN | select.POLLHUP), 0)
        self.case.assertEqual(events[0][1] & (select.POLLERR | select.POLLNVAL), 0)
        self.case.assertTrue(reaped, "Live or unreaped child before fallback")
        # Fresh confirmation after pinned whole-process exit, never consuming.
        with self.case.assertRaises(ChildProcessError) as missing:
            self.waitid(os.P_PIDFD, descriptor, flags)
        self.case.assertEqual(missing.exception.errno, errno.ECHILD)

    def assert_evidence(self):
        case = self.case
        name, which, phase, _, expected, born, statuses, adopts, closed = self.row
        self.records = self._snapshot()
        # Collect actual kernel ownership facts BEFORE classification/phase
        # assertions can fail and before fallback can reap/close anything.
        owner_fds = {entry[3] for entry in self.records if entry[0] >> 32 == 0
                     and entry[1] == 3 and 0 <= entry[3] <= 0x7fffffff}
        fd_states = {}
        for descriptor in sorted(owner_fds):
            try:
                self.fstat(descriptor)
            except OSError as error:
                fd_states[descriptor] = error.errno
            else:
                fd_states[descriptor] = 0
        own_gone, group_gone = self._no_children(0x20000000), self._no_children(0)
        pids = {entry[4] for entry in self.records if entry[0] >> 32 == 2 and entry[1] == 2
                and entry[2] == 0 and entry[3] >= 0 and entry[5] == entry[3]
                and entry[4] > 0 and entry[4] != self.getpid()}
        case.assertEqual(len(pids), 1 if born else 0)
        child_pid = next(iter(pids)) if born else -1
        if born:
            self._guard(child_pid)
        case.assertTrue(own_gone, "Own-thread child remains before fallback")
        case.assertTrue(group_gone, "Thread-group child remains before fallback")
        for descriptor in sorted(owner_fds):
            case.assertEqual(fd_states[descriptor], errno.EBADF, "Owner FD live before fallback")
        if which == 28:
            self.assert_packet(child_pid)
        # Validate the closed journal representation, including per-writer sequence.
        counts = [0, 0, 0]
        writers = []
        for entry in self.records:
            sequence, kind, encoded, descriptor, event_pid, result, error, timestamp = entry
            writer, index = sequence >> 32, sequence & 0xffffffff
            writers.append(writer)
            case.assertIn(writer, (0, 1, 2)); case.assertEqual(index, counts[writer]); counts[writer] += 1
            case.assertIn(kind, tuple(range(1, 11))); case.assertNotEqual(kind, 9)
            case.assertTrue(encoded == 0 or 2 <= encoded <= 39)
            case.assertGreater(timestamp, 0)
            case.assertEqual(error, -result if kind == 2 and -4095 <= result < 0
                             else 0 if kind == 2 and result >= 0 else -1)
        case.assertLessEqual(counts[0], 36); case.assertLessEqual(counts[1], 27); case.assertLessEqual(counts[2], 24)
        case.assertEqual(writers, sorted(writers))  # Partition order, not total chronology.
        parent = [entry for entry in self.records if entry[0] >> 32 == 0]
        active, acquired = set(), set()
        acquisitions = 0
        commits = []
        for entry in parent:
            _, kind, encoded, descriptor, _, result, _, _ = entry
            if kind == 3 and descriptor >= 0:
                case.assertLessEqual(descriptor, 0x7fffffff)
                case.assertNotIn(descriptor, active)
                active.add(descriptor); acquired.add(descriptor); acquisitions += 1
                case.assertLessEqual(len(active), 8); case.assertLessEqual(acquisitions, 12)
            elif kind == 4 and descriptor >= 0:
                case.assertIn(descriptor, active); active.remove(descriptor)
            elif kind == 8:
                case.assertEqual(active, set(), "Commit before resource release")
                case.assertEqual(encoded, 39); case.assertEqual(result, expected); commits.append(entry)
        case.assertEqual(len(commits), 1); case.assertEqual(active, set())
        case.assertEqual(parent[-1][1], 8, "Owner activity follows final commit")
        case.assertEqual(acquired, owner_fds)
        # Test-owned slots and caller stdio remain live/unchanged; no new FD opens.
        for descriptor in self.initial_fds:
            self.fstat(descriptor)
        for descriptor, before in enumerate(self.stdio):
            after = self.fstat(descriptor)
            case.assertEqual((after.st_dev, after.st_ino, after.st_mode),
                             (before.st_dev, before.st_ino, before.st_mode))
        if born:
            actor = 2 if which == 27 else 0
            reaps = [entry for entry in self.records if entry[0] >> 32 == actor and entry[1:5] == (4, 32, -1, child_pid)]
            case.assertEqual(len(reaps), 1)
            case.assertNotEqual(reaps[0][5], 125, "Negative child fixture failed")
            case.assertIn(reaps[0][5], statuses)
        else:
            case.assertEqual(counts[1], 0)
        if phase:
            writer = 1 if which == 24 and phase in (20, 22, 24, 26) else 0
            kind = 2 if phase & 1 else 1
            selected = [entry for entry in self.records if entry[0] >> 32 == writer and entry[1] == kind and entry[2] == phase]
            case.assertEqual(len(selected), 1, "Selected phase not reached exactly once")
            marker = selected[0]
            if phase & 1:
                case.assertGreaterEqual(marker[5], 0, "AFTER did not execute real operation")
            prior_adopts = [entry for entry in parent if entry[1] == 3 and entry[2] // 2 == phase // 2
                            and entry[3] >= 0 and entry[0] < marker[0]]
            case.assertGreaterEqual(len(prior_adopts), adopts)
            if born and phase == 17:
                case.assertEqual(marker[5], child_pid, "AFTER birth did not preserve real native PID")
            if closed:
                prior_closes = [entry for entry in parent if entry[1] == 4 and entry[2] // 2 == phase // 2
                                and entry[3] >= 0 and entry[0] < marker[0]]
                case.assertGreaterEqual(len(prior_closes), 1, "AFTER preceded actual close/release")
            if which == 25 and phase == 18:
                following = [entry for entry in parent if entry[1] == 4 and entry[2] // 2 == 9
                             and entry[3] >= 0 and entry[0] > marker[0]]
                case.assertGreaterEqual(len(following), 1, "BEFORE lost ownership before close")
        if name == "c1":
            case.assertEqual(len([entry for entry in parent if entry[1:3] == (2, 35) and entry[5] == 0]), 1)
        if which in (23, 25, 26):
            ready = [entry for entry in self.records if entry[0] >> 32 == 2 and entry[1] == 7
                     and entry[4] == child_pid and entry[5] == 1]
            case.assertEqual(len(ready), 1)
        if which == 23:
            advances = [entry for entry in self.records if entry[0] >> 32 == 2 and entry[1] == 5 and entry[5] == 10000000001]
            case.assertEqual(len(advances), 1)
        if which == 25:
            sent = [entry for entry in self.records if entry[0] >> 32 == 2 and entry[1:3] == (6, phase)
                    and entry[4] == self.getpid() and entry[5] == 0]
            case.assertEqual(len(sent), 1)
        if 1 <= which <= 22 or which in (27, 28) or name in ("c1", "c2", "c3"):
            handoff = {entry[3] for entry in self.records if entry[0] >> 32 == 1
                       and entry[1] == 10 and entry[4] == child_pid}
            case.assertEqual(handoff, {0, 1, 2, 3, 4})
            failed_exec = [entry for entry in self.records if entry[0] >> 32 == 1
                           and entry[1:3] == (2, 26) and entry[5] < 0]
            case.assertEqual(failed_exec, [], "Fixed negative child could not exec")
        case.assertIs(signal.getsignal(signal.SIGINT), self.before_handler)
        case.assertEqual(signal.pthread_sigmask(signal.SIG_BLOCK, []), self.before_mask)
        case.assertEqual(self.fstat(self.stdout.fileno()).st_size, 0)
        case.assertEqual(self.fstat(self.stderr.fileno()).st_size, 0)

    def assert_packet(self, pid):
        case = self.case
        case.assertEqual(self.report(self.packet, 248), 248)
        packet = bytes(self.packet)
        case.assertEqual(packet[:4], b"NTF1")
        cursor = 4

        def integer(width, signed=True):
            nonlocal cursor
            value = int.from_bytes(packet[cursor:cursor + width], "little", signed=signed)
            cursor += width
            return value

        case.assertEqual(integer(4), pid); case.assertEqual(integer(4), 5); case.assertEqual(integer(4), 6)
        case.assertEqual([integer(4) for _ in range(17)], [0, 1, 2, 3, 4, 5] + [-1] * 11)
        case.assertEqual([integer(4) for _ in range(5)], [0] * 5)
        flags = [integer(4) for _ in range(5)]
        for value in flags:
            case.assertGreaterEqual(value, 0)
        case.assertEqual([value & os.O_ACCMODE for value in flags], [2, 2, 2, 1, 1])
        case.assertEqual([value & os.O_NONBLOCK for value in flags], [0] * 5)
        case.assertEqual(integer(8, False), 0)
        for _ in range(4):
            case.assertEqual((integer(4), integer(8, False), integer(8, False), integer(8, False)), (0, 0, 0, 0))
        case.assertEqual(integer(4), 0); case.assertEqual(cursor, 248)


class NativeOwnerAcceptance(unittest.TestCase):
    artifact = None
    row_id = None

    def one(self, rows):
        selected = [row for row in rows if row[0] == self.row_id]
        self.assertEqual(len(selected), 1, "Exactly one closed row per process required")
        row = selected[0]
        with tempfile.TemporaryDirectory(prefix="schema-native-negative-") as directory:
            root = Path(directory)
            fixtures = {
                "s.json": {"$schema": "https://json-schema.org/draft/2020-12/schema", "type": "integer"},
                "v.json": 1,
                "manifest.json": {"profile": "subathon.schema-package.v1",
                                  "documents": [{"path": "s.json", "kind": "jsonschema202012"}],
                                  "examples": [{"path": "v.json", "schema_document": "s.json", "schema_pointer": ""}]},
            }
            for name, value in fixtures.items():
                (root / name).write_text(json.dumps(value, separators=(",", ":")), encoding="utf-8")
            session = FaultSession(self, self.artifact, row)
            buffers, arguments = session.arguments(root / "manifest.json")
            with session:
                actual, interrupted = None, False
                started = time.monotonic()
                try:
                    actual = session.entry(*arguments)
                except KeyboardInterrupt:
                    interrupted = True
                if type(actual) is int and actual == 31:
                    raise SchemaTransportNotImplemented()  # Before ALL phase/resource assertions.
                elapsed = time.monotonic() - started
                session.assert_evidence()
                self.assertEqual(interrupted, row[1] == 25)
                if not interrupted:
                    self.assertIs(type(actual), int)
                    self.assertEqual(actual, row[4])
                if row[1] == 23:
                    self.assertGreaterEqual(elapsed, 0)
                    self.assertLess(elapsed, 3, "Offset deadline was replaced by a real wait")
            self.assertEqual(len(buffers), 4)  # Strong references survive the complete native call.

    def test_original_activation_rows(self): self.one(ACTIVATION)
    def test_original_outcome_rows(self): self.one(OUTCOME)
    def test_status_mismatch_and_unclassified_death(self): self.one(STATUS)
    def test_pre_activation_setup_rows(self): self.one(SETUP)
    def test_startup_deadline_from_original_t0(self): self.one(DEADLINE)
    def test_prebirth_facility_and_resource_rows(self): self.one(PREBIRTH)
    def test_acquisition_and_work_failure_rows(self): self.one(WORK)
    def test_cleanup_interrupt_and_release_once_rows(self): self.one(CLEANUP)
    def test_real_sigint_before_and_after_close(self): self.one(INTERRUPT)
    def test_known_resource_after_atomic_birth(self): self.one(RESOURCE)
    def test_competing_reaper_keeps_pinned_identity(self): self.one(REAPER)
    def test_raw_child_fd_signal_packet(self): self.one(PACKET)


def main():
    groups = (
        ("test_original_activation_rows", ACTIVATION), ("test_original_outcome_rows", OUTCOME),
        ("test_status_mismatch_and_unclassified_death", STATUS), ("test_pre_activation_setup_rows", SETUP),
        ("test_startup_deadline_from_original_t0", DEADLINE), ("test_prebirth_facility_and_resource_rows", PREBIRTH),
        ("test_acquisition_and_work_failure_rows", WORK), ("test_cleanup_interrupt_and_release_once_rows", CLEANUP),
        ("test_real_sigint_before_and_after_close", INTERRUPT), ("test_known_resource_after_atomic_birth", RESOURCE),
        ("test_competing_reaper_keeps_pinned_identity", REAPER), ("test_raw_child_fd_signal_packet", PACKET),
    )
    names = {row[0]: method for method, rows in groups for row in rows}
    if len(groups) != 12 or len(names) != 57 or sum(len(rows) for _, rows in groups) != 57:
        raise FixtureFailure("Closed native row inventory differs")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact", required=True)
    parser.add_argument("--row", required=True, choices=tuple(names))
    options = parser.parse_args()
    NativeOwnerAcceptance.artifact, NativeOwnerAcceptance.row_id = options.artifact, options.row
    suite = unittest.TestSuite([NativeOwnerAcceptance(names[options.row])])
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.testsRun != 1:
        raise SystemExit(2)
    raise SystemExit(0 if result.wasSuccessful() else 1)


if __name__ == "__main__":
    main()
