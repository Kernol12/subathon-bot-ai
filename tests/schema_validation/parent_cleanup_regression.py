# SPDX-License-Identifier: GPL-3.0-or-later
"""Focused parent cancellation/allocation regressions; import defines only tests.

Actual source/oracle review is required before the first reached-bug run. The real
fault child cannot validate anything; only cleanup/refusal behavior is observed.
"""
import contextlib
import builtins
import errno
import importlib.util
import io
import os
from pathlib import Path
import select
import signal
import subprocess
import sys
import threading
import unittest
from unittest import mock


SCRIPT = Path(__file__).parents[2] / "tools" / "schema_validate.py"


class ParentCleanupRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("schema_validate_cleanup_under_test", SCRIPT)
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        cls.target = module

    def test_cancelled_descriptor_close_reaps_and_closes_before_propagating(self):
        real_pipe, real_close, real_popen = os.pipe, os.close, subprocess.Popen
        parent_pairs, test_fds, children = [], set(), []
        entered_spawn = False
        triggered = []
        cancellation = KeyboardInterrupt("owned descriptor close cancellation")
        watchdog = None
        watchdog_fired = threading.Event()
        child_already_reaped = False
        stdout, stderr = io.StringIO(), io.StringIO()

        def close_test_fd(descriptor):
            if descriptor in test_fds:
                test_fds.remove(descriptor)
                real_close(descriptor)

        def pipe():
            pair = real_pipe()
            # Native Popen may make its own error pipe; those are not parent-owned
            # protocol channels and must not be used as this regression's oracle.
            if not entered_spawn:
                parent_pairs.append(pair)
            return pair

        def close(descriptor):
            real_close(descriptor)
            if (entered_spawn and children and not triggered
                    and descriptor in [pair[0] for pair in parent_pairs]):
                triggered.append(descriptor)
                # Model a Python signal exception AFTER the real Linux close.
                raise cancellation

        def spawn(argv, **kwargs):
            nonlocal entered_spawn, watchdog
            entered_spawn = True
            self.assertEqual(len(parent_pairs), 2)
            self.assertEqual(len(argv), 12)
            self.assertEqual(list(argv[1:3]), ["-I", "-S"])
            self.assertTrue(Path(argv[0]).is_absolute())
            self.assertTrue(Path(argv[3]).is_absolute())
            self.assertEqual(list(argv[4::2]), ["--manifest", "--sdk-dir", "--activation-fd", "--outcome-fd"])
            activation, outcome = int(argv[9]), int(argv[11])
            self.assertEqual((activation, outcome), (parent_pairs[0][1], parent_pairs[1][1]))
            self.assertEqual(set(kwargs["pass_fds"]), {activation, outcome})
            self.assertEqual(len(kwargs["pass_fds"]), 2)
            self.assertIs(kwargs["shell"], False)
            self.assertIs(kwargs["close_fds"], True)
            for name in ("stdin", "stdout", "stderr"):
                self.assertEqual(kwargs[name], subprocess.DEVNULL)
            ready_read, ready_write = real_pipe()
            test_fds.update((ready_read, ready_write))
            release_read, release_write = real_pipe()
            test_fds.update((release_read, release_write))
            code = (
                "import os; "
                "os.write(int(os.environ['TEST_ACTIVATION']),b'B'); "
                "os.write(int(os.environ['TEST_READY']),b'R'); "
                "os.close(int(os.environ['TEST_READY'])); "
                "os.read(int(os.environ['TEST_BLOCK']),1)"
            )
            process = real_popen(
                [sys.executable, "-I", "-S", "-c", code], shell=False, close_fds=True,
                stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                pass_fds=(activation, outcome, ready_write, release_read),
                env={"TEST_ACTIVATION": str(activation), "TEST_READY": str(ready_write),
                     "TEST_BLOCK": str(release_read)})
            children.append(process)
            close_test_fd(ready_write)
            close_test_fd(release_read)

            def expire():
                watchdog_fired.set()
                try:
                    os.kill(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass

            timer = threading.Timer(5, expire)
            timer.daemon = True
            timer.start()
            watchdog = timer
            readable, _, _ = select.select([ready_read], [], [], 3)
            self.assertEqual(readable, [ready_read], "fault child readiness watchdog")
            self.assertEqual(os.read(ready_read, 2), b"R")
            close_test_fd(ready_read)
            # Child remains blocked, keeps BOTH protocol writes open, and never
            # sends a successful/failure result that could stand in for validation.
            return process

        try:
            caught = None
            with mock.patch.object(os, "pipe", side_effect=pipe), \
                    mock.patch.object(os, "close", side_effect=close), \
                    mock.patch.object(subprocess, "Popen", side_effect=spawn), \
                    contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                try:
                    self.target.validate_package("synthetic-manifest.json")
                except KeyboardInterrupt as error:
                    caught = error
            self.assertEqual(len(children), 1)
            # Capture real state BEFORE any test poll/wait/close/kill cleanup. A
            # waitable exited child is also a failure, even though waitpid reaps it.
            try:
                wait_result = os.waitpid(children[0].pid, os.WNOHANG)
                child_state = "running" if wait_result[0] == 0 else "test-probe-reaped"
                child_already_reaped = wait_result[0] != 0
            except ChildProcessError:
                child_state = "parent-reaped"
                child_already_reaped = True
            descriptor_states = []
            for pair in parent_pairs:
                for descriptor in pair:
                    try:
                        os.fstat(descriptor)
                        descriptor_states.append("open")
                    except OSError as error:
                        if error.errno != errno.EBADF:
                            raise
                        descriptor_states.append("closed")
            self.assertIs(caught, cancellation)
            self.assertEqual(len(triggered), 1)
            self.assertFalse(watchdog_fired.is_set(), "fault child watchdog fired")
            self.assertEqual(
                child_state, "parent-reaped",
                "parent must reap before propagating cancellation; fd states=" + repr(descriptor_states))
            self.assertEqual(descriptor_states, ["closed"] * 4)
            self.assertEqual(stdout.getvalue(), "")
            self.assertEqual(stderr.getvalue(), "")
        finally:
            try:
                if watchdog is not None:
                    watchdog.cancel()
                    watchdog.join(timeout=1)
            finally:
                try:
                    for descriptor in tuple(test_fds):
                        close_test_fd(descriptor)
                    for pair in parent_pairs:
                        for descriptor in pair:
                            try:
                                real_close(descriptor)
                            except OSError as error:
                                if error.errno != errno.EBADF:
                                    raise
                finally:
                    if children and not child_already_reaped:
                        try:
                            children[0].kill()
                        except ProcessLookupError:
                            pass
                        finally:
                            children[0].wait(timeout=3)

    def test_initial_path_allocation_refusal_is_resource_limit_without_setup(self):
        refusal = MemoryError("INITIAL_ADMISSION_ALLOCATION_REFUSAL")
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(self.target, "_bounded_path", side_effect=refusal) as admission, \
                mock.patch.object(self.target, "_installation", side_effect=AssertionError("setup reached")) as installation, \
                mock.patch.object(subprocess, "Popen", side_effect=AssertionError("allocation refusal spawned")) as spawn, \
                mock.patch.object(builtins, "open", side_effect=AssertionError("allocation refusal read")) as builtin_open, \
                mock.patch.object(io, "open", side_effect=AssertionError("allocation refusal read")) as file_open, \
                mock.patch.object(os, "open", side_effect=AssertionError("allocation refusal read")) as descriptor_open, \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            actual = self.target.validate_package("manifest.json")
        self.assertIs(actual, self.target.SchemaValidationOutcome.resource_limit)
        admission.assert_called_once_with("manifest.json")
        installation.assert_not_called()
        spawn.assert_not_called()
        builtin_open.assert_not_called()
        file_open.assert_not_called()
        descriptor_open.assert_not_called()
        self.assertEqual(stdout.getvalue(), "")
        self.assertEqual(stderr.getvalue(), "")


if __name__ == "__main__":
    unittest.main(verbosity=2)
