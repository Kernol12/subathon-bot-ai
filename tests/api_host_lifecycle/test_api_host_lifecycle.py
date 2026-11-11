# SPDX-License-Identifier: GPL-3.0-or-later
"""APITEST-001: NEW real consumer, bounded Linux child/peer lifecycle oracles.

No probe module import, fixture listener, signal handler, synthetic native clock,
readiness/generation control or alternate host. --inventory is passive only.
The coordinator must first admit THIS binary's complete passive ELF/ABI closure,
SDK seal and initial/runtime maps, including its actual QSQLITE plugin, then pass
that independent receipt via --admission. Old API012 probe approval is insufficient.
Supervisor monotonic time bounds fixtures only; it is never sent to the product.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time
import unittest

PROBE = None
ADMISSION = None
STEP = 7.0
OVERALL = 20.0
CLEANUP = 2.0
MAX_COMMANDS = 64
MAX_COMMAND = 4096
MAX_EVENTS = 256
MAX_LINE = 65536
MAX_LOG = 1048576
MAX_PEERS = 8
MAX_WIRE = 4096
# Literal C++ observer vocabulary; serialization is internal acceptance protocol.
K = dict(inspection_enqueued=0, inspection_completed=1, installation_read_enqueued=2,
         installation_read_completed=3, activated=4, listener_ready=5,
         diagnostic_queued=6, diagnostic_cancelled=7, diagnostic_entered=8,
         wrapper_accepted=9, wrapper_dispatched=10, execution_entered=11,
         begin_attempted=12, precommit_read=13, actual_commit=14, execution_revoked=15,
         writer_completed=16, publication_entered=17, publication_revoked=18,
         local_handoff=19, stop_consumed=20, callbacks_revoked=21, sockets_removed=22,
         network_ack=23, writer_removed=24, writer_ack=25, framework_joined=26,
         io_joined=27, source_destroyed=28, dispatcher_destroyed=29, owner_cleanup=30, stopped=31, ingress_refused=32)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def file_identity(path):
    # Bounded streaming hashes, never read_bytes of arbitrary loaded DSO size.
    path = Path(path).resolve(strict=True)
    size = path.stat().st_size
    require(path.is_file() and 0 < size <= 134217728, "SETUP: file hash admission bound")
    digest = hashlib.sha256()
    total = 0
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1048576), b""):
            total += len(block)
            require(total <= size, "SETUP: artifact changed during hash")
            digest.update(block)
    require(total == size, "SETUP: artifact size changed")
    return dict(sha256=digest.hexdigest(), bytes=total)


def admitted_map(event, runtime=False):
    require(event.get("complete") is True and event.get("incomplete_reason") == "none" and
            event.get("backend") == "admitted_corrected" and
            event.get("consumer_cplusplus") == 202002, "SETUP: incomplete/wrong new-consumer identity")
    require(ADMISSION is not None, "SETUP: new consumer has no independent admission receipt")
    require(event.get("qt_runtime") == event.get("qt_compiled") and
            tuple(int(n) for n in event["qt_runtime"].split(".")) >= (6, 12, 0),
            "SETUP: selected Qt runtime/compile identity")
    objects = event.get("loaded_objects")
    require(isinstance(objects, list) and 0 < len(objects) <= 64, "SETUP: loaded object count")
    actual = {}
    for obj in objects:
        require(set(obj) == {"path", "kind", "load_address"}, "SETUP: loaded object shape")
        if obj["kind"] == "virtual":
            require(obj["path"] in ("linux-vdso.so.1", "linux-gate.so.1"), "SETUP: virtual object")
            continue
        require(obj["kind"] in ("main", "file") and len(obj["path"]) <= 512 and
                Path(obj["path"]).is_absolute(), "SETUP: unresolved loaded file")
        path = str(Path(obj["path"]).resolve(strict=True))
        require(path not in actual, "SETUP: duplicate actual loaded object")
        actual[path] = file_identity(path)
    for dependency in ("libdrogon.so", "libtrantor.so"):
        require(sum(Path(path).name.startswith(dependency) for path in actual) == 1,
                "SETUP: actual corrected SDK library absent/duplicated: " + dependency)
    if runtime:
        require(sum(Path(path).name == "libqsqlite.so" for path in actual) == 1,
                "SETUP: actual selected QSQLITE plugin absent/duplicated")
    expected = ADMISSION["runtime_objects" if runtime else "initial_objects"]
    require(actual == expected, "SETUP: actual loaded closure/hash differs from new-consumer admission")
    require(actual.get(str(PROBE)) == ADMISSION["consumer"], "SETUP: consumer hash mismatch")


class Child:
    """One process lifetime; synchronous nonblocking pipes, no reader threads.

    All logs <=1MiB, each line<=64KiB, pending/history<=256, 64 commands of4KiB,
    <=8 peers of4KiB per operation. Each step7s, scenario20s. Cleanup at most TWO
    attempts (TERM then KILL), each2s. Every failure is retained; kill is terminal,
    never graceful-stop evidence. Popen/fixtures are not instantiated by inventory.
    """
    def __init__(self, owner, *, store="current", port=0, block_body=False, factory=False, hold_inspection=False, hold_wrapper=False):
        self.end = time.monotonic() + OVERALL
        self.pending = []
        self.history = []
        self.parts = {"stdout": bytearray(), "stderr": bytearray()}
        self.total = 0
        self.commands = 0
        self.peers = []
        self.closed = False
        self.expected_terminal = None
        self.failures = []
        self.eof = set()
        self.directory = tempfile.TemporaryDirectory(prefix="api-host-")
        owner.addCleanup(self.directory.cleanup)  # also covers Popen failure and _exit stubs
        config = json.dumps(dict(store=store, port=port, block_body=block_body,
                                 fixture_root=self.directory.name, hold_inspection=hold_inspection, hold_wrapper=hold_wrapper), separators=(",", ":"))
        args = [str(PROBE), "--elapsed-factory" if factory else config]
        # Loader search/preload environment must equal the independently gated one.
        environment = dict(ADMISSION["environment"])
        require(set(environment) <= {"PATH", "LANG", "LC_ALL", "LD_LIBRARY_PATH",
                                      "QT_PLUGIN_PATH"}, "SETUP: unadmitted loader environment")
        self.process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, close_fds=True, env=environment,
                                        start_new_session=True, bufsize=0)
        owner.addCleanup(self.cleanup)
        for stream in (self.process.stdin, self.process.stdout, self.process.stderr):
            os.set_blocking(stream.fileno(), False)
        admitted_map(self.wait("identity"))
        if not factory:
            admitted_map(self.wait("runtime_identity"), runtime=True)
            self.wait("signal_installed")

    def remaining(self, deadline):
        left = min(self.end, deadline) - time.monotonic()
        require(left > 0, "FIXTURE watchdog, not intended red")
        return left

    def pump(self, deadline):
        streams = [stream for name, stream in (("stdout", self.process.stdout), ("stderr", self.process.stderr)) if name not in self.eof]
        ready, _, _ = select.select(streams, [], [], self.remaining(deadline))
        require(ready, "FIXTURE pipe/event watchdog, not intended red")
        for stream in ready:
            name = "stdout" if stream is self.process.stdout else "stderr"
            block = os.read(stream.fileno(), 4096)
            if not block:
                self.eof.add(name)
                require(not self.parts[name], "FIXTURE truncated output")
                continue
            self.total += len(block)
            require(self.total <= MAX_LOG, "FIXTURE log byte cap")
            part = self.parts[name]
            part.extend(block)
            while b"\n" in part:
                end = part.index(10) + 1
                line = bytes(part[:end])
                del part[:end]
                require(len(line) <= MAX_LINE, "FIXTURE line cap")
                if name == "stderr":
                    self.failures.append(line[:256].decode("utf-8", "replace"))
                    require(len(self.failures) <= 32, "FIXTURE stderr record cap")
                    continue
                event = json.loads(line)
                require(isinstance(event, dict), "FIXTURE event object")
                require(len(self.history) < MAX_EVENTS and len(self.pending) < MAX_EVENTS, "FIXTURE event cap")
                self.history.append(event)
                if event.get("event") == "unimplemented":
                    raise AssertionError("UNIMPLEMENTED production api-host: " + event["piece"])
                require(event.get("event") not in ("fixture_error", "host_failure"),
                        "FIXTURE/setup or unexpected host failure: " + str(event))
                self.pending.append(event)
            require(len(part) <= MAX_LINE, "FIXTURE partial line cap")

    def wait(self, name, **fields):
        deadline = time.monotonic() + STEP
        while True:
            for index, event in enumerate(self.pending):
                if event.get("event") == name and all(event.get(k) == v for k, v in fields.items()):
                    return self.pending.pop(index)
            require(len(self.eof) != 2, f"FIXTURE child exited {self.process.poll()} before {name}: {self.failures}")
            self.pump(deadline)

    def command(self, op):
        self.commands += 1
        require(self.commands <= MAX_COMMANDS, "FIXTURE command count")
        data = json.dumps(dict(op=op, request=self.commands), separators=(",", ":")).encode() + b"\n"
        require(len(data) <= MAX_COMMAND, "FIXTURE command bytes")
        _, ready, _ = select.select([], [self.process.stdin], [], self.remaining(time.monotonic() + STEP))
        require(ready and os.write(self.process.stdin.fileno(), data) == len(data), "FIXTURE command write")
        return self.wait("result", request=self.commands)

    def start(self):
        self.command("start")
        snapshot = self.wait("serving")["snapshot"]
        require(snapshot["state"] == 2, "Actual serving state")
        return snapshot

    def events(self, name):
        return [e for e in self.history if e.get("event") == "host_event" and e["kind"] == K[name]]

    def peer(self, port):
        require(len(self.peers) < MAX_PEERS, "FIXTURE peer cap")
        peer = socket.create_connection(("127.0.0.1", port), self.remaining(time.monotonic() + STEP))
        peer.settimeout(self.remaining(time.monotonic() + STEP))
        self.peers.append(peer)
        return peer

    def stop(self):
        self.command("stop")
        event = self.wait("stopped")
        snap = event["snapshot"]
        require(snap["state"] == 4 and snap["writer_barrier"] and snap["network_barrier"] and snap["owner_cleanup"], "Actual stopped barriers")
        require(snap["associations"] == "0" and snap["input_capacity"] == "0" and snap["response_capacity"] == "0", "Reservations released at barrier")
        deadline = time.monotonic() + STEP
        while self.process.poll() is None and len(self.eof) != 2:
            self.pump(deadline)
        require(self.process.wait(timeout=self.remaining(deadline)) == 0, "Graceful process exit")
        return snap

    def cleanup(self):
        if self.closed:
            return
        self.closed = True
        for peer in self.peers:
            peer.close()
        cleanup_errors = []
        # No blind graceful claim. Reap even assertion/constructor failure paths.
        for sig in (signal.SIGTERM, signal.SIGKILL):
            if self.process.poll() is not None:
                break
            try:
                os.killpg(self.process.pid, sig)
                self.process.wait(timeout=CLEANUP)
            except (ProcessLookupError, subprocess.TimeoutExpired) as error:
                cleanup_errors.append(type(error).__name__)
        reaped = self.process.poll() is not None
        for stream in (self.process.stdin, self.process.stdout, self.process.stderr):
            stream.close()
        require(reaped, "FIXTURE child not reaped after two attempts: " + str(cleanup_errors))
        self.directory.cleanup()
        if self.expected_terminal is not None:
            require(self.process.returncode == self.expected_terminal, "Wrong process-terminal category")
        # stderr is never silently treated as feature success; selected Trantor's
        # bind exit is asserted separately. A feature assertion may have preceded it.
        require(not self.failures or self.expected_terminal == 1, "FIXTURE unexpected stderr: " + str(self.failures))


def free_port():
    # A released OS port is just a candidate, not listener readiness. Any bind race
    # is an honest setup failure; HL11 intentionally holds its socket separately.
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        return reservation.getsockname()[1]


def send(peer, data):
    require(len(data) <= MAX_WIRE, "FIXTURE peer send budget")
    peer.sendall(data)


def receive(peer, wanted):
    require(0 < wanted <= MAX_WIRE, "FIXTURE receive budget")
    deadline = time.monotonic() + STEP
    result = bytearray()
    while len(result) < wanted:
        require(time.monotonic() < deadline, "FIXTURE peer read deadline")
        data = peer.recv(wanted - len(result))
        require(data, "Unexpected peer EOF")
        result.extend(data)
    return bytes(result)


def upgrade(child, port):
    peer = child.peer(port)
    send(peer, b"GET /api-host/ws HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n")
    header = bytearray()
    deadline = time.monotonic() + STEP
    while not header.endswith(b"\r\n\r\n"):
        require(len(header) < MAX_WIRE and time.monotonic() < deadline, "FIXTURE Upgrade bound")
        header.extend(receive(peer, 1))
    require(header.startswith(b"HTTP/1.1 101 "), "Actual production Upgrade")
    return peer


def frame(body):
    require(len(body) <= 125, "FIXTURE small masked binary frame")
    key = b"\x01\x02\x03\x04"
    return bytes((0x82, 0x80 | len(body))) + key + bytes(b ^ key[i % 4] for i, b in enumerate(body))


def response(peer):
    header = receive(peer, 2)
    require(header[0] == 0x82 and header[1] < 126, "Unmasked bounded binary reply")
    return receive(peer, header[1])


def diagnostic(peer, correlation="44"):
    send(peer, b"POST /api-host/diagnostic HTTP/1.1\r\nHost: localhost\r\nX-Host-Correlation: " + correlation.encode() + b"\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n")
    data = bytearray()
    deadline = time.monotonic() + STEP
    while not data.endswith(b"diag"):
        require(len(data) < MAX_WIRE and time.monotonic() < deadline, "FIXTURE HTTP response bound")
        block = peer.recv(1)
        require(block, "Diagnostic reply missing")
        data.extend(block)
    require(data.startswith(b"HTTP/1.1 200 ") and b"\r\n\r\n" in data, "Nonsecret diagnostic success")


class ApiHostNative(unittest.TestCase):
    def test_elapsed_factory_is_shipped_native_source(self):
        child = Child(self, factory=True)
        child.wait("elapsed_factory_ok")
        self.assertEqual(child.process.wait(timeout=STEP), 0)

    def test_ordinary_schema8_real_activation_HL06_HL08(self):
        port = free_port()
        child = Child(self, port=port)
        child.start()
        # Observation is not activation. Require actual chain events and protected
        # store-backed work; listener/open/DTO cannot satisfy these assertions.
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        self.assertEqual(response(peer), b"Adone")
        child.wait("publication")
        snap = child.command("inspect")
        self.assertTrue(snap["storage_active"])
        self.assertTrue(snap["elapsed_source_shared"])
        self.assertEqual(len(child.events("activated")), 1)
        self.assertEqual(len(child.events("inspection_completed")), 1)
        self.assertEqual(len(child.events("installation_read_completed")), 1)
        self.assertEqual(child.events("writer_completed")[0]["work"],
                         dict(transaction=1, error=0, value=True, ticket_revoked=True))
        child.stop()  # child directly asserts unchanged I/E/J and all session facts

    def test_diagnostics_before_activation_HL08_HL17(self):
        for store in ("fresh", "future"):
            with self.subTest(store=store):
                port = free_port()
                child = Child(self, port=port, store=store)
                snap = child.start()
                self.assertFalse(snap["storage_active"])
                peer = child.peer(port)
                diagnostic(peer)
                child.wait("diagnostic")
                snap = child.command("inspect")
                self.assertFalse(snap["storage_active"])
                self.assertFalse(child.events("execution_entered"))
                self.assertFalse(child.events("begin_attempted"))
                child.stop()
                child.cleanup()

    def test_old_wrong_malformed_missing_sources_refuse_without_setup(self):
        for store in ("old", "wrong", "malformed_catalog", "missing", "malformed"):
            with self.subTest(store=store):
                port = free_port()
                child = Child(self, port=port, store=store)
                child.start()
                peer = child.peer(port)
                diagnostic(peer)
                self.assertFalse(child.command("inspect")["storage_active"])
                self.assertFalse(child.events("activated"))
                child.stop()
                child.cleanup()

    def test_persistent_independent_correlations_HL13(self):
        port = free_port()
        child = Child(self, port=port)
        child.start()
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01one") + frame(b"B\x01two"))
        self.assertEqual({response(peer), response(peer)}, {b"Adone", b"Bdone"})
        child.command("inspect")
        self.assertEqual(len(child.events("actual_commit")), 2)
        peer.close()  # local handoff does not certify client receipt or replay
        child.stop()
        self.assertEqual(len(child.events("execution_entered")), 2)

    def test_ingress_capacity2_refuses_third_before_body_HL01(self):
        port = free_port()
        child = Child(self, port=port, hold_wrapper=True)
        child.start()
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        child.wait("wrapper_gate")  # actual accepted wrapper, BEFORE any work scope
        send(peer, frame(b"B\x01data") + frame(b"C\x01data"))
        refusal = child.wait("host_event", kind=K["ingress_refused"])
        self.assertEqual(refusal["refusal"], 5)  # Failure::resource_limit literal
        self.assertFalse(child.events("execution_entered"))
        release = b'{"op":"release"}\n'
        self.assertEqual(os.write(child.process.stdin.fileno(), release), len(release))
        # Refusal/control output is bounded but no application error schema is
        # invented here. Actual completion events establish the two executions.
        child.wait("publication")
        child.wait("publication")
        child.stop()
        self.assertEqual(len(child.events("execution_entered")), 2)
        self.assertEqual(len(child.events("actual_commit")), 2)

    def test_startup_stop_and_three_signals_HL10(self):
        child = Child(self, port=free_port())
        child.command("stop")  # after production signal install, BEFORE framework start
        for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGTERM):
            os.kill(child.process.pid, sig)
        # No fixture replacement of handler/request_stop or admission state.
        self.assertFalse(child.events("execution_entered"))
        child.wait("stopped")
        self.assertEqual(child.process.wait(timeout=STEP), 0)
        self.assertEqual(len(child.events("stop_consumed")), 1)

    def test_signals_during_startup_HL10(self):
        child = Child(self, port=free_port(), hold_inspection=True)
        # Start acknowledgment cannot arrive while the pre-work observation gate
        # is held. The fixture never pumps/reenters a live execution scope.
        data = b'{"op":"start","request":1}\n'
        self.assertEqual(os.write(child.process.stdin.fileno(), data), len(data))
        child.wait("startup_gate")
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGINT):
            os.kill(child.process.pid, sig)
        release = b'{"op":"release"}\n'
        self.assertEqual(os.write(child.process.stdin.fileno(), release), len(release))
        child.wait("stopped")
        self.assertEqual(child.process.wait(timeout=STEP), 0)
        self.assertEqual(len(child.events("stop_consumed")), 1)
        self.assertFalse(child.events("execution_entered"))

    def test_occupied_port_process_terminal_then_fresh_retry_HL11(self):
        with socket.socket() as occupied:
            occupied.bind(("127.0.0.1", 0))
            occupied.listen(1)
            port = occupied.getsockname()[1]
            child = Child(self, port=port)
            child.expected_terminal = 1
            # Do not wait for a start result or fabricate destructor/barrier proof.
            data = b'{"op":"start","request":1}\n'
            self.assertEqual(os.write(child.process.stdin.fileno(), data), len(data))
            deadline = time.monotonic() + STEP
            while child.process.poll() is None:
                child.pump(deadline)
            self.assertEqual(child.process.returncode, 1)
            self.assertFalse(child.events("stopped"))
            child.cleanup()
        retry = Child(self, port=port)
        retry.start()
        diagnostic(retry.peer(port))
        retry.stop()

    def test_idle_partial_and_uncooperative_peers_HL12(self):
        port = free_port()
        child = Child(self, port=port)
        child.start()
        idle = child.peer(port)
        partial_http = child.peer(port)
        send(partial_http, b"POST /api-host/diagnostic HTTP/1.1\r\nHost:")
        partial_upgrade = child.peer(port)
        send(partial_upgrade, b"GET /api-host/ws HTTP/1.1\r\nUpgrade: websocket\r\n")
        partial_frame = upgrade(child, port)
        send(partial_frame, b"\x82\x84\x01\x02")
        uncooperative = upgrade(child, port)
        # No close reply; actual production Close deadline is1000ms.
        child.stop()
        for peer in (idle, partial_http, partial_upgrade, partial_frame, uncooperative):
            data = peer.recv(MAX_WIRE)
            if data:
                self.assertLessEqual(len(data), MAX_WIRE)
                self.assertEqual(peer.recv(MAX_WIRE), b"")
        self.assertTrue(child.events("callbacks_revoked"))
        self.assertTrue(child.events("sockets_removed"))
        self.assertTrue(child.events("io_joined"))

    def test_close_deadline_not_reset_by_ping_or_signals_HL12_HL15(self):
        port = free_port()
        child = Child(self, port=port)
        child.start()
        peer = upgrade(child, port)
        first = child.command("stop")
        header = receive(peer, 2)
        self.assertEqual(header[0], 0x88)
        if header[1]:
            receive(peer, header[1])
        send(peer, bytes.fromhex("89 81 01 02 03 04 79"))  # literal Ping x after local Close
        os.kill(child.process.pid, signal.SIGTERM)
        second = child.command("inspect")
        self.assertEqual(second["drain_deadline"], first["drain_deadline"])
        self.assertEqual(second["stop_fences"], "1")
        child.wait("stopped")
        self.assertEqual(child.process.wait(timeout=STEP), 0)
        self.assertEqual(len(child.events("stop_consumed")), 1)

    def test_nonreturning_body_is_terminal_not_stopped_HL12(self):
        port = free_port()
        child = Child(self, port=port, block_body=True)
        child.start()
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        child.wait("nonreturning_body")
        os.kill(child.process.pid, signal.SIGTERM)
        # Owner cannot consume stop while body blocks. Supervisor termination says
        # nothing about rollback, connection removal or safe C++ destruction.
        self.assertIsNone(child.process.poll())
        self.assertFalse(child.events("stopped"))
        child.expected_terminal = -signal.SIGKILL
        os.killpg(child.process.pid, signal.SIGKILL)
        self.assertEqual(child.process.wait(timeout=STEP), -signal.SIGKILL)

    def test_actual_thread_and_stop_barrier_order(self):
        port = free_port()
        child = Child(self, port=port)
        child.start()
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        self.assertEqual(response(peer), b"Adone")
        child.stop()
        for name in ("execution_entered", "begin_attempted", "precommit_read", "writer_completed", "publication_entered", "writer_removed", "owner_cleanup"):
            events = child.events(name)
            self.assertTrue(events, name)
            self.assertTrue(all(event["owner_thread"] for event in events), name)
        self.assertTrue(all(not event["owner_thread"] for event in child.events("local_handoff")))
        positions = {name: next(i for i, event in enumerate(child.history)
                               if event.get("event") == "host_event" and event["kind"] == K[name])
                     for name in ("callbacks_revoked", "sockets_removed", "network_ack", "writer_removed", "writer_ack", "io_joined", "framework_joined", "owner_cleanup", "stopped")}
        self.assertLess(positions["callbacks_revoked"], positions["network_ack"])
        self.assertLess(positions["sockets_removed"], positions["network_ack"])
        self.assertLess(positions["writer_removed"], positions["writer_ack"])
        for name, position in positions.items():
            if name != "stopped":
                self.assertLess(position, positions["stopped"], name)


def main():
    global PROBE, ADMISSION
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", action="store_true")
    parser.add_argument("--probe", type=Path)
    parser.add_argument("--admission", type=Path)
    options, rest = parser.parse_known_args()
    if options.inventory:
        # Passive class dictionary inspection; no discovery/import/test fixture.
        print(json.dumps({"protocol": 1, "tests": sorted(name for name in ApiHostNative.__dict__ if name.startswith("test_")),
                          "child_seconds": OVERALL, "step_seconds": STEP, "cleanup_attempts": 2}))
        return
    require(options.probe is not None and options.admission is not None, "SETUP: --probe and new --admission required")
    PROBE = options.probe.resolve(strict=True)
    require(options.admission.stat().st_size <= 65536, "SETUP: admission receipt cap")
    ADMISSION = json.loads(options.admission.read_text(encoding="utf-8"))
    require(set(ADMISSION) == {"format", "consumer", "initial_objects", "runtime_objects", "environment"} and
            ADMISSION["format"] == 1, "SETUP: new-consumer admission receipt schema")
    require(file_identity(PROBE) == ADMISSION["consumer"], "SETUP: binary changed after admission")
    for name in ("initial_objects", "runtime_objects"):
        require(isinstance(ADMISSION[name], dict) and 0 < len(ADMISSION[name]) <= 64, "SETUP: receipt closure bound")
    unittest.main(argv=[sys.argv[0], *rest])


if __name__ == "__main__":
    main()
