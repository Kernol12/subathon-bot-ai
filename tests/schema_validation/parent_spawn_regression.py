# SPDX-License-Identifier: GPL-3.0-or-later
"""Native constructor ownership regressions; review before first execution.

REAL children and native Popen construction are retained. Test-only interception
holds the constructor after spawn; it never supplies a validating worker/result.
"""
import contextlib
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


class ParentSpawnRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("schema_validate_spawn_under_test", SCRIPT)
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        cls.target = module

    def exercise_spawn_failure(self, allocation):
        native_popen, native_pipe, native_close = subprocess.Popen, os.pipe, os.close
        native_execute_child = native_popen._execute_child
        parent_pairs, test_fds, children = [], set(), []
        entered_spawn = False
        constructor_held = threading.Event()
        release_constructor = threading.Event()
        constructor_interrupted = threading.Event()
        controller_done = threading.Event()
        controller_errors = []
        signal_sent = []
        native_cancellations = []
        factory_returned = []
        refusal = MemoryError("POST_NATIVE_SPAWN_ALLOCATION_REFUSAL")
        watchdog = None
        watchdog_fired = threading.Event()
        controller = None
        child_already_reaped = False
        ready_read = ready_write = release_read = release_write = None
        stdout, stderr = io.StringIO(), io.StringIO()
        original_handler = signal.getsignal(signal.SIGINT)
        self.assertIs(original_handler, signal.default_int_handler)
        main_thread_id = threading.main_thread().ident

        def close_test_fd(descriptor):
            if descriptor in test_fds:
                test_fds.remove(descriptor)
                native_close(descriptor)

        def pipe():
            pair = native_pipe()
            if not entered_spawn:
                parent_pairs.append(pair)
            return pair

        def expire(process):
            watchdog_fired.set()
            try:
                os.kill(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass

        def execute_child(process, *args, **kwargs):
            nonlocal watchdog
            # ACTUAL selected CPython method performs native spawn/exec FIRST.
            native_execute_child(process, *args, **kwargs)
            children.append(process)  # Strong reference also prevents __del__ polling.
            self.assertIs(type(process.pid), int)
            self.assertGreater(process.pid, 0)
            self.assertTrue(process._child_created)
            close_test_fd(ready_write)
            close_test_fd(release_read)
            timer = threading.Timer(5, expire, args=(process,))
            timer.daemon = True
            timer.start()
            watchdog = timer
            readable, _, _ = select.select([ready_read], [], [], 3)
            self.assertEqual(readable, [ready_read], "native child readiness watchdog")
            self.assertEqual(os.read(ready_read, 2), b"R")
            close_test_fd(ready_read)
            if allocation:
                # Code/ABI are real; only a precise post-spawn allocation fails.
                raise refusal
            try:
                constructor_held.set()
                self.assertTrue(release_constructor.wait(timeout=3), "constructor release watchdog")
            except KeyboardInterrupt as error:
                native_cancellations.append(error)
                constructor_interrupted.set()
                raise

        def interrupt_main():
            try:
                if not constructor_held.wait(timeout=3):
                    controller_errors.append("constructor did not reach native hold")
                    return
                signal.pthread_kill(main_thread_id, signal.SIGINT)
                signal_sent.append(True)
                # This bounded acknowledgement makes the old main-thread native
                # constructor observe SIGINT before release. A future owned spawn
                # thread need not receive Python's main-thread signal exception.
                constructor_interrupted.wait(timeout=1)
            except BaseException as error:
                controller_errors.append(type(error).__name__)
            finally:
                release_constructor.set()
                controller_done.set()

        def spawn(argv, **kwargs):
            nonlocal entered_spawn, ready_read, ready_write, release_read, release_write
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
            ready_read, ready_write = native_pipe()
            test_fds.update((ready_read, ready_write))
            release_read, release_write = native_pipe()
            test_fds.update((release_read, release_write))
            code = ("import os; os.write(int(os.environ['TEST_READY']),b'R'); "
                    "os.close(int(os.environ['TEST_READY'])); "
                    "os.read(int(os.environ['TEST_BLOCK']),1)")
            # The real native constructor above is patched for this call only.
            # Both protocol writes stay open; NO activation/outcome claims validity.
            process = native_popen(
                [sys.executable, "-I", "-S", "-c", code], shell=False, close_fds=True,
                stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                pass_fds=(activation, outcome, ready_write, release_read),
                env={"TEST_READY": str(ready_write), "TEST_BLOCK": str(release_read)})
            factory_returned.append(True)
            return process

        try:
            if not allocation:
                thread = threading.Thread(target=interrupt_main, daemon=True)
                thread.start()
                controller = thread
            caught = None
            result = None
            with mock.patch.object(os, "pipe", side_effect=pipe), \
                    mock.patch.object(native_popen, "_execute_child", new=execute_child), \
                    mock.patch.object(subprocess, "Popen", side_effect=spawn), \
                    contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                try:
                    result = self.target.validate_package("synthetic-manifest.json")
                except KeyboardInterrupt as error:
                    caught = error
            if controller is not None:
                self.assertTrue(controller_done.wait(timeout=3), "signal controller watchdog")
            self.assertEqual(len(children), 1)
            # Real ownership oracles BEFORE any test child poll/wait/kill/FD cleanup.
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
            self.assertFalse(watchdog_fired.is_set(), "native child watchdog fired")
            self.assertEqual(controller_errors, [])
            if allocation:
                self.assertIsNone(caught)
                self.assertIs(result, self.target.SchemaValidationOutcome.resource_limit)
            else:
                self.assertEqual(signal_sent, [True])
                self.assertIs(type(caught), KeyboardInterrupt)
                if native_cancellations:
                    self.assertEqual(len(native_cancellations), 1)
                    self.assertIs(caught, native_cancellations[0])
            self.assertIs(signal.getsignal(signal.SIGINT), original_handler)
            self.assertEqual(
                child_state, "parent-reaped",
                "native constructor ownership; factory returned=" + repr(factory_returned)
                + "; fd states=" + repr(descriptor_states))
            self.assertEqual(descriptor_states, ["closed"] * 4)
            self.assertEqual(stdout.getvalue(), "")
            self.assertEqual(stderr.getvalue(), "")
        finally:
            release_constructor.set()
            try:
                if controller is not None:
                    controller.join(timeout=4)
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
                                native_close(descriptor)
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

    def test_sigint_during_native_constructor_reaps_before_propagating(self):
        self.exercise_spawn_failure(allocation=False)

    def test_post_native_spawn_allocation_refusal_reaps_before_resource_limit(self):
        self.exercise_spawn_failure(allocation=True)


if __name__ == "__main__":
    unittest.main(verbosity=2)
