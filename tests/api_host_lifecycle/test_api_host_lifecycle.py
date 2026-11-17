# SPDX-License-Identifier: GPL-3.0-or-later
"""APITEST-001: NEW real consumer, bounded Linux child/peer lifecycle oracles.

No probe module import, fixture listener, signal handler, synthetic native clock,
readiness control or alternate host. --inventory is passive only.
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
HASHES = {}
STEP = 7.0
OVERALL = 20.0
CLEANUP = 2.0
MAX_COMMANDS = 64
MAX_COMMAND = 4096
MAX_EVENTS = 256
MAX_LINE = 131072
MAX_LOG = 1048576
MAX_PEERS = 8
MAX_WIRE = 4096
# Literal C++ observer vocabulary; serialization is internal acceptance protocol.
K = {'inspection_enqueued': 0, 'inspection_completed': 1, 'installation_read_enqueued': 2, 'installation_read_completed': 3, 'activated': 4, 'listener_ready': 5, 'diagnostic_queued': 6, 'diagnostic_cancelled': 7, 'diagnostic_entered': 8, 'command_admitted': 9, 'command_entered': 10, 'unit_entered': 11, 'writer_completed': 12, 'command_ended': 13, 'command_cancelled': 14, 'forward_refused': 15, 'finish_refused': 16, 'local_handoff': 17, 'stop_consumed': 18, 'callbacks_revoked': 19, 'sockets_removed': 20, 'network_ack': 21, 'writer_removed': 22, 'writer_ack': 23, 'framework_joined': 24, 'io_joined': 25, 'source_destroyed': 26, 'dispatcher_destroyed': 27, 'owner_cleanup': 28, 'stopped': 29, 'ingress_refused': 30, 'operation_refused': 31}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def file_identity(path):
    # Bounded streaming hashes, never read_bytes of arbitrary loaded DSO size.
    path = Path(path).resolve(strict=True)
    size = path.stat().st_size
    require(path.is_file() and 0 < size <= 134217728, "SETUP: file hash admission bound")
    stamp = (str(path), size, path.stat().st_mtime_ns, path.stat().st_ctime_ns)
    if stamp in HASHES:
        return HASHES[stamp]
    digest = hashlib.sha256()
    total = 0
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1048576), b""):
            total += len(block)
            require(total <= size, "SETUP: artifact changed during hash")
            digest.update(block)
    require(total == size, "SETUP: artifact size changed")
    value = dict(sha256=digest.hexdigest(), bytes=total)
    require(len(HASHES) < 512, "SETUP: per-run hash cache bound")
    HASHES[stamp] = value
    return value


def admitted_map(event, runtime=False, phase=None):
    require(event.get("complete") is True and event.get("incomplete_reason") == "none" and
            event.get("backend") == "admitted_corrected" and
            event.get("consumer_cplusplus") == 202002, "SETUP: incomplete/wrong new-consumer identity")
    require(ADMISSION is not None, "SETUP: new consumer has no independent admission receipt")
    require(event.get("qt_runtime") == event.get("qt_compiled") and
            tuple(int(n) for n in event["qt_runtime"].split(".")) >= (6, 12, 0),
            "SETUP: selected Qt runtime/compile identity")
    objects = event.get("loaded_objects")
    require(isinstance(objects, list) and 0 < len(objects) <= 128, "SETUP: loaded object count")
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
    expected = ADMISSION[(phase + "_objects") if phase else ("runtime_objects" if runtime else "initial_objects")]
    require(actual == expected, "SETUP: actual loaded closure/hash differs from new-consumer admission")
    require(actual.get(str(PROBE)) == ADMISSION["consumer"], "SETUP: consumer hash mismatch")


class Child:
    """One process lifetime; synchronous nonblocking pipes, no reader threads.

    All logs <=1MiB, each line<=128KiB, pending/history<=256, 64 commands of4KiB,
    <=8 peers of4KiB per operation. Each step7s, scenario20s. Cleanup at most TWO
    attempts (TERM then KILL), each2s. Every failure is retained; kill is terminal,
    never graceful-stop evidence. Popen/fixtures are not instantiated by inventory.
    """
    def __init__(self, owner, *, store="current", port=0, block_body=False, factory=False, hold_inspection=False, hold_wrapper=False, gate_body=False, response_cap=0, diagnostic_throw=False, installation="I", hold_diagnostic=False, frame_ms=0, forward_after_signal=False, http_port=0, observer_throw=False, validate_options=None, peer_cap=0, body_cap=0):
        self.end = time.monotonic() + OVERALL
        self.pending = []
        self.history = []
        self.parts = {"protocol": bytearray(), "stdout": bytearray(), "stderr": bytearray()}
        self.total = 0
        self.commands = 0
        self.peers = []
        self.closed = False
        self.expected_terminal = None
        self.failures = []
        self.eof = set()
        self.directory = tempfile.TemporaryDirectory(prefix="api-host-")
        owner.addCleanup(self.directory.cleanup)  # also covers Popen failure and _exit stubs
        config = json.dumps(dict(store=store, port=port, http_port=http_port, observer_throw=observer_throw, peer_cap=peer_cap, body_cap=body_cap, block_body=block_body,
                                 fixture_root=self.directory.name, hold_inspection=hold_inspection, hold_wrapper=hold_wrapper, gate_body=gate_body, response_cap=response_cap, diagnostic_throw=diagnostic_throw, installation=installation, hold_diagnostic=hold_diagnostic, frame_ms=frame_ms, forward_after_signal=forward_after_signal), separators=(",", ":"))
        if validate_options is not None:
            config = json.dumps(dict(validate_options=validate_options), separators=(",", ":"))
        self.requested = (port, http_port)
        self.observer_throw = observer_throw
        args = [str(PROBE), "--elapsed-factory" if factory else config]
        # Loader search/preload environment must equal the independently gated one.
        environment = dict(ADMISSION["environment"])
        require(set(environment) <= {"PATH", "LANG", "LC_ALL", "LD_LIBRARY_PATH",
                                      "QT_PLUGIN_PATH"}, "SETUP: unadmitted loader environment")
        protocol_read, protocol_write = os.pipe()
        environment["APITEST_PROTOCOL_FD"] = str(protocol_write)
        try:
            self.process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE, close_fds=True, pass_fds=(protocol_write,), env=environment,
                                            start_new_session=True, bufsize=0)
        except BaseException:
            os.close(protocol_read)
            raise
        finally:
            os.close(protocol_write)
        self.protocol = os.fdopen(protocol_read, "rb", buffering=0)
        owner.addCleanup(self.cleanup)
        for stream in (self.process.stdin, self.protocol, self.process.stdout, self.process.stderr):
            os.set_blocking(stream.fileno(), False)
        admitted_map(self.wait("identity"))
        if not factory and validate_options is None:
            admitted_map(self.wait("runtime_identity"), runtime=True)
            self.wait("signal_installed")

    def remaining(self, deadline):
        left = min(self.end, deadline) - time.monotonic()
        require(left > 0, "FIXTURE watchdog, not intended red")
        return left

    def pump(self, deadline):
        streams = [stream for name, stream in (("protocol", self.protocol), ("stdout", self.process.stdout), ("stderr", self.process.stderr)) if name not in self.eof]
        if not streams:
            self.process.wait(timeout=self.remaining(deadline))
            return
        ready, _, _ = select.select(streams, [], [], self.remaining(deadline))
        require(ready, "FIXTURE pipe/event watchdog, not intended red")
        for stream in ready:
            name = "protocol" if stream is self.protocol else ("stdout" if stream is self.process.stdout else "stderr")
            block = os.read(stream.fileno(), 4096)
            if not block:
                self.eof.add(name)
                if name == "protocol":
                    require(not self.parts[name], "FIXTURE truncated protocol")
                elif self.parts[name]:
                    self.failures.append(bytes(self.parts[name][:256]).decode("utf-8", "replace"))
                    self.parts[name].clear()
                    require(len(self.failures) <= 32, "FIXTURE diagnostic record cap")
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
                if name != "protocol":
                    self.failures.append(line[:256].decode("utf-8", "replace"))
                    require(len(self.failures) <= 32, "FIXTURE diagnostic record cap")
                    continue
                try:
                    event = json.loads(line)
                except (ValueError, UnicodeError) as error:
                    raise AssertionError("FIXTURE non-JSON protocol record") from error
                require(isinstance(event, dict), "FIXTURE event object")
                require(len(self.history) < MAX_EVENTS and len(self.pending) < MAX_EVENTS, "FIXTURE event cap")
                if event.get("event") == "host_event":
                    writer_kinds = (K["writer_completed"], K["inspection_completed"], K["installation_read_completed"])
                    require(event.get("kind") in writer_kinds or "work" not in event, "Non-writer event has no SQL result")
                if event.get("event") == "unit_completion":
                    actual = self.events("writer_completed")
                    require(actual and actual[-1]["work"]["ticket_revoked"], "Actual revoked writer result precedes supplied completion")
                    require(all(actual[-1]["work"][key] == event[key] for key in ("transaction", "error", "value")),
                            "Supplied completion receives unchanged actual result")
                self.history.append(event)
                if event.get("event") == "unimplemented":
                    raise AssertionError("UNIMPLEMENTED production api-host: " + event["piece"])
                require(event.get("event") != "assertion", "PRODUCT assertion: " + str(event))
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
            require(len(self.eof) != 3, f"FIXTURE child exited {self.process.poll()} before {name}: {self.failures}")
            self.pump(deadline)

    def command(self, op):
        self.commands += 1
        require(self.commands <= MAX_COMMANDS, "FIXTURE command count")
        data = json.dumps(dict(op=op, request=self.commands), separators=(",", ":")).encode() + b"\n"
        require(len(data) <= MAX_COMMAND, "FIXTURE command bytes")
        _, ready, _ = select.select([], [self.process.stdin], [], self.remaining(time.monotonic() + STEP))
        require(ready and os.write(self.process.stdin.fileno(), data) == len(data), "FIXTURE command write")
        return self.wait("result", request=self.commands)

    def start(self, active=True):
        self.command("start")
        admitted_map(self.wait("serving_identity"), runtime=True, phase="serving")
        endpoints = self.wait("listeners")
        require(endpoints["address"] == "127.0.0.1" and
                0 < endpoints["port"] <= 65535 and 0 < endpoints["http_port"] <= 65535 and
                endpoints["port"] != endpoints["http_port"], "Actual dual endpoints")
        for index, key in enumerate(("port", "http_port")):
            require(not self.requested[index] or self.requested[index] == endpoints[key], "Fixed role port")
        require(endpoints["native_socket_options"] == endpoints["http_socket_options"] == 1,
                "Actual pre-listen REUSEADDR1 REUSEPORT0 on both")
        self.listeners = endpoints
        snapshot = self.wait("serving")["snapshot"]
        require(snapshot["state"] == 2, "Actual serving state")
        if active:
            self.wait("host_event", kind=K["activated"])
            snapshot = self.command("inspect")
            require(snapshot["storage_active"], "Actual ordinary startup activation")
        return snapshot

    def events(self, name):
        return [e for e in self.history if e.get("event") == "host_event" and e["kind"] == K[name]]

    def peer(self, port):
        require(len(self.peers) < MAX_PEERS, "FIXTURE peer cap")
        peer = socket.create_connection(("127.0.0.1", port), self.remaining(time.monotonic() + STEP))
        peer.settimeout(self.remaining(time.monotonic() + STEP))
        self.peers.append(peer)
        return peer

    def finish(self):
        admitted_map(self.wait("stopped_identity"), runtime=True, phase="stopped")
        event = self.wait("stopped")
        snap = event["snapshot"]
        require(snap["state"] == (5 if self.observer_throw else 4) and snap["writer_barrier"] and snap["network_barrier"] and snap["owner_cleanup"], "Actual stopped barriers")
        require(snap["associations"] == "0" and snap["input_capacity"] == "0" and snap["response_capacity"] == "0", "Reservations released at barrier")
        self.command("exit")  # production signal bridge stays alive through this handshake
        deadline = time.monotonic() + STEP
        while len(self.eof) != 3:
            self.pump(deadline)
        require(self.process.wait(timeout=self.remaining(deadline)) == 0, "Graceful process exit")
        return snap

    def stop(self):
        self.command("stop")
        return self.finish()

    def refused_activation(self):
        deadline = time.monotonic() + STEP
        while not any(e.get("refusal") is not None for name in ("inspection_completed", "installation_read_completed") for e in self.events(name)):
            self.pump(deadline)
        require(not self.events("activated"), "No activation after terminal refusal")

    def release(self):
        self.commands += 1
        require(self.commands <= MAX_COMMANDS, "FIXTURE command count including gate releases")
        data = b'{"op":"release"}\n'
        require(os.write(self.process.stdin.fileno(), data) == len(data), "FIXTURE release write")

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
        for stream in (self.process.stdin, self.protocol, self.process.stdout, self.process.stderr):
            stream.close()
        require(reaped, "FIXTURE child not reaped after two attempts: " + str(cleanup_errors))
        self.directory.cleanup()
        if self.expected_terminal is not None:
            require(self.process.returncode == self.expected_terminal, "Wrong process-terminal category")
        # Framework stdout/stderr is diagnostic only, retained under independent
        # byte/line/count caps. Product/fixture failures use the protocol + status.
        require(self.process.returncode not in (42, 43, 70) or self.expected_terminal is not None,
                "Child terminal failure: " + str(self.process.returncode) + " diagnostics=" + str(self.failures))


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


def drain_peer(peer):
    result = bytearray()
    deadline = time.monotonic() + STEP
    while True:
        require(time.monotonic() < deadline, "FIXTURE peer teardown watchdog")
        try:
            block = peer.recv(MAX_WIRE)
        except ConnectionResetError:
            return bytes(result)
        if not block:
            return bytes(result)
        result.extend(block)
        require(len(result) <= MAX_WIRE, "FIXTURE bounded teardown output")


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


FIXED400 = b"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
HTTP_DIAGNOSTIC = b"POST /api-host/diagnostic HTTP/1.1\r\nHost: localhost\r\nX-Host-Correlation: 44\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n"
WS_UPGRADE = b"GET /api-host/ws HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n"


def http_reply(peer):
    header = bytearray()
    while not header.endswith(b"\r\n\r\n"):
        require(len(header) < MAX_WIRE, "FIXTURE bounded HTTP header")
        header.extend(receive(peer, 1))
    lengths = [line.split(b":", 1)[1].strip() for line in header.split(b"\r\n") if line.lower().startswith(b"content-length:")]
    require(len(lengths) == 1 and lengths[0].isdigit(), "Actual response framing")
    size = int(lengths[0])
    require(0 <= size <= MAX_WIRE - len(header), "FIXTURE response operation bound")
    return bytes(header), receive(peer, size) if size else b""


def lock_identity():
    try:
        info = Path("/tmp/drogon.lock").lstat()
        return (info.st_dev, info.st_ino, info.st_mode, info.st_size, info.st_mtime_ns, info.st_ctime_ns)
    except FileNotFoundError:
        return None


class ApiHostNative(unittest.TestCase):
    def test_elapsed_factory_is_shipped_native_source(self):
        child = Child(self, factory=True)
        child.wait("elapsed_factory_ok")
        self.assertEqual(child.process.wait(timeout=STEP), 0)

    def test_ordinary_current_catalog_real_activation_HL06_HL08(self):
        port = free_port()
        child = Child(self, port=port)
        child.start()
        # Observation is not activation. Require actual chain events and protected
        # store-backed work; listener/open/DTO cannot satisfy these assertions.
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        self.assertEqual(response(peer), b"Adone")
        child.wait("unit_completion")
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
                child = Child(self, port=port, store="current" if store == "wrong_selector" else store,
                              installation="X" if store == "wrong_selector" else "I")
                child.start(active=False)
                child.refused_activation()
                snap = child.command("inspect")
                self.assertFalse(snap["storage_active"])
                peer = child.peer(child.listeners["http_port"])
                diagnostic(peer)
                child.wait("diagnostic")
                snap = child.command("inspect")
                self.assertFalse(snap["storage_active"])
                self.assertFalse(child.events("command_entered"))
                self.assertFalse(child.events("writer_completed"))
                child.stop()
                child.cleanup()

    def test_old_wrong_malformed_missing_sources_refuse_without_setup(self):
        for store in ("old", "previous_head", "wrong", "malformed_catalog", "missing", "malformed", "wrong_selector"):
            with self.subTest(store=store):
                port = free_port()
                child = Child(self, port=port, store="current" if store == "wrong_selector" else store,
                              installation="X" if store == "wrong_selector" else "I")
                child.start(active=False)
                child.refused_activation()
                peer = child.peer(child.listeners["http_port"])
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
        self.assertEqual(len(child.events("writer_completed")), 2)
        peer.close()  # local handoff does not certify client receipt or replay
        child.stop()
        self.assertEqual(len(child.events("command_entered")), 2)

    def test_ingress_capacity2_refuses_third_before_body_HL01(self):
        port = free_port()
        child = Child(self, port=port, hold_wrapper=True)
        child.start()
        peer = upgrade(child, port)
        other = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        child.wait("wrapper_gate")  # contracted Qt admitted phase, BEFORE entry checks/work scope
        send(other, frame(b"B\x01data") + frame(b"C\x01data"))
        refusal = child.wait("host_event", kind=K["ingress_refused"])
        self.assertEqual(refusal["refusal"], 5)  # Failure::resource_limit literal
        self.assertFalse(child.events("command_entered"))
        child.release()
        # Refusal/control output is bounded but no application error schema is
        # invented here. Actual completion events establish the two executions.
        child.wait("unit_completion")
        child.wait("unit_completion")
        child.wait("host_event", kind=K["local_handoff"])
        child.wait("host_event", kind=K["local_handoff"])
        child.command("inspect")
        send(peer, frame(b"D\x01data"))
        self.assertEqual({response(peer), response(peer)}, {b"Adone", b"Ddone"})
        child.wait("unit_completion")
        child.stop()
        self.assertEqual(len(child.events("command_entered")), 3)
        self.assertEqual(len(child.events("writer_completed")), 3)

    def test_startup_stop_and_three_signals_HL10(self):
        child = Child(self, port=free_port())
        child.command("stop")  # after production signal install, BEFORE framework start
        for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGTERM):
            os.kill(child.process.pid, sig)
        # No fixture replacement of handler/request_stop or admission state.
        self.assertFalse(child.events("command_entered"))
        child.finish()
        self.assertEqual(len(child.events("stop_consumed")), 1)

    def test_signals_during_startup_HL10(self):
        child = Child(self, port=free_port(), hold_inspection=True)
        # Start acknowledgment cannot arrive while the pre-work observation gate
        # is held. The fixture never pumps/reenters a live execution scope.
        child.commands = 1
        data = b'{"op":"start","request":1}\n'
        self.assertEqual(os.write(child.process.stdin.fileno(), data), len(data))
        child.wait("startup_gate")
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGINT):
            os.kill(child.process.pid, sig)
        child.release()
        child.finish()
        self.assertEqual(len(child.events("stop_consumed")), 1)
        self.assertFalse(child.events("command_entered"))

    def test_occupied_port_process_terminal_then_fresh_retry_HL11(self):
        with socket.socket() as occupied:
            occupied.bind(("127.0.0.1", 0))
            occupied.listen(1)
            port = occupied.getsockname()[1]
            child = Child(self, port=port)
            child.expected_terminal = 1
            # Do not wait for a start result or fabricate destructor/barrier proof.
            child.commands = 1
            data = b'{"op":"start","request":1}\n'
            self.assertEqual(os.write(child.process.stdin.fileno(), data), len(data))
            deadline = time.monotonic() + STEP
            while len(child.eof) != 3:
                child.pump(deadline)
            self.assertEqual(child.process.wait(timeout=child.remaining(deadline)), 1)
            self.assertFalse(child.events("stopped"))
            child.cleanup()
        retry = Child(self, port=port)
        retry.start()
        diagnostic(retry.peer(retry.listeners["http_port"]))
        retry.stop()

    def test_idle_partial_and_uncooperative_peers_HL12(self):
        port = free_port()
        child = Child(self, port=port)
        child.start()
        idle = child.peer(port)
        partial_http = child.peer(child.listeners["http_port"])
        send(partial_http, b"POST /api-host/diagnostic HTTP/1.1\r\nHost:")
        partial_upgrade = child.peer(port)
        send(partial_upgrade, b"GET /api-host/ws HTTP/1.1\r\nUpgrade: websocket\r\n")
        partial_frame = upgrade(child, port)
        send(partial_frame, b"\x82\x84\x01\x02")
        uncooperative = upgrade(child, port)
        http_idle = child.peer(child.listeners["http_port"])
        # No close reply; actual production Close deadline is1000ms.
        child.stop()
        for peer in (idle, partial_http, partial_upgrade, http_idle):
            data = drain_peer(peer)
            if data:
                # HTTP refusal or EOF is legal before WebSocket establishment.
                header, separator, _ = data.partition(b"\r\n\r\n")
                self.assertTrue(separator, "Complete bounded HTTP refusal header")
                status = header.split(b"\r\n", 1)[0].split(b" ")
                self.assertGreaterEqual(len(status), 2)
                self.assertIn(status[0], (b"HTTP/1.0", b"HTTP/1.1"))
                self.assertEqual(len(status[1]), 3)
                self.assertTrue(status[1].isdigit())
                self.assertTrue(400 <= int(status[1]) <= 599, "HTTP refusal status")
        for peer in (partial_frame, uncooperative):
            data = drain_peer(peer)
            if data:
                self.assertEqual(data[0], 0x88)  # only established WS uses Close framing
                self.assertGreaterEqual(len(data), 2)
                self.assertLessEqual(data[1], 125)
                self.assertEqual(len(data), 2 + data[1])
        self.assertTrue(child.events("callbacks_revoked"))
        self.assertTrue(child.events("sockets_removed"))
        self.assertTrue(child.events("io_joined"))

    def test_native_partial_frame_expiry_uses_shared_production_elapsed_HL15(self):
        port = free_port()
        child = Child(self, port=port, frame_ms=200)
        child.start()
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        self.assertEqual(response(peer), b"Adone")  # positive complete-frame control
        started = time.monotonic()  # supervisor observation, never host time input
        send(peer, frame(b"P\x01data")[:-1])  # real incomplete masked payload
        self.assertEqual(drain_peer(peer), b"", "Native deadline force-closes incomplete frame")
        elapsed = time.monotonic() - started
        self.assertGreaterEqual(elapsed, 0.200, "No premature expiry of the200ms frame budget")
        self.assertLess(elapsed, STEP, "Expiry without any host stop or fixture close")
        snap = child.command("inspect")
        self.assertEqual(snap["state"], 2)
        self.assertTrue(snap["elapsed_source_shared"])
        self.assertFalse(child.events("stop_consumed"))
        self.assertEqual(len(child.events("command_entered")), 1, "Partial P never enters")
        other = upgrade(child, port)
        send(other, frame(b"B\x01data"))
        self.assertEqual(response(other), b"Bdone")  # host/profile still serve another peer
        child.stop()
        self.assertEqual(len(child.events("writer_completed")), 2)
        self.assertEqual(len(child.events("writer_completed")), 2)

    def test_diagnostic_queued_before_signal_never_enters_HL12_HL17(self):
        port = free_port()
        child = Child(self, port=port, store="future", hold_diagnostic=True)
        child.start(active=False)
        child.refused_activation()
        peer = child.peer(child.listeners["http_port"])
        send(peer, b"POST /api-host/diagnostic HTTP/1.1\r\nHost: localhost\r\nX-Host-Correlation: 44\r\nContent-Length: 0\r\n\r\n")
        child.wait("diagnostic_gate")  # actual Qt queue observation; owner blocked before entry
        queued = child.events("diagnostic_queued")
        self.assertEqual(len(queued), 1)
        self.assertTrue(queued[0]["owner_thread"])
        self.assertFalse(child.events("stop_consumed"))
        self.assertFalse(child.events("diagnostic_entered"))
        os.kill(child.process.pid, signal.SIGTERM)
        child.release()  # no event pumping or status setter inside the observation gate
        child.finish()
        stop = child.events("stop_consumed")
        cancelled = child.events("diagnostic_cancelled")
        self.assertEqual(len(stop), 1)
        self.assertEqual(len(cancelled), 1)
        self.assertLess(int(queued[0]["ordinal"]), int(stop[0]["ordinal"]))
        self.assertLess(int(stop[0]["ordinal"]), int(cancelled[0]["ordinal"]))
        self.assertFalse(child.events("diagnostic_entered"))
        self.assertFalse(any(e.get("event") == "diagnostic" for e in child.history))
        self.assertFalse(child.events("local_handoff"))
        self.assertFalse(child.events("command_entered"))
        self.assertFalse(child.events("unit_entered"))
        self.assertFalse(child.events("writer_completed"))
        self.assertNotEqual(drain_peer(peer).split(b"\r\n\r\n")[-1], b"diag")

    def test_close_deadline_not_reset_by_ping_or_signals_HL12_HL15(self):
        port = free_port()
        child = Child(self, port=port)
        child.start()
        peer = upgrade(child, port)
        child.command("stop")
        child.wait("host_event", kind=K["stop_consumed"])
        first = child.command("inspect")
        self.assertIsNotNone(first["drain_deadline"])
        header = receive(peer, 2)
        self.assertEqual(header[0], 0x88)
        if header[1]:
            receive(peer, header[1])
        send(peer, bytes.fromhex("89 81 01 02 03 04 79"))  # literal Ping x after local Close
        os.kill(child.process.pid, signal.SIGTERM)
        second = child.command("inspect")
        self.assertEqual(second["drain_deadline"], first["drain_deadline"])
        self.assertEqual(second["stop_fences"], "1")
        child.finish()
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
        # The child explicitly observed an active nonreturning body before TERM.
        # Drain protocol for a bounded observation interval; no graceful claim.
        readable, _, _ = select.select([child.protocol], [], [], 0.1)
        if readable:
            child.pump(time.monotonic() + STEP)
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
        other = upgrade(child, port)
        send(other, frame(b"B\x01data"))
        self.assertEqual(response(other), b"Bdone")
        child.stop()
        for name in ("command_entered", "unit_entered", "writer_completed", "writer_removed", "dispatcher_destroyed", "owner_cleanup"):
            events = child.events(name)
            self.assertTrue(events, name)
            self.assertTrue(all(event["owner_thread"] for event in events), name)
        io = child.events("local_handoff")
        self.assertEqual(len(io), 2)
        self.assertEqual(len({event["thread_token"] for event in io}), 1)
        self.assertTrue(all(not event["owner_thread"] for event in io))
        self.assertTrue(child.wait("dispatch_destructor")["owner_thread"])
        positions = {name: next(i for i, event in enumerate(child.history)
                               if event.get("event") == "host_event" and event["kind"] == K[name])
                     for name in ("callbacks_revoked", "sockets_removed", "network_ack", "writer_removed", "writer_ack", "io_joined", "framework_joined", "owner_cleanup", "stopped")}
        self.assertLess(positions["callbacks_revoked"], positions["network_ack"])
        self.assertLess(positions["sockets_removed"], positions["network_ack"])
        self.assertLess(positions["writer_removed"], positions["writer_ack"])
        for name, position in positions.items():
            if name != "stopped":
                self.assertLess(position, positions["stopped"], name)


    def test_finishing_write_signal_and_queued_frame_HL12(self):
        port = free_port()
        child = Child(self, port=port, hold_wrapper=True, gate_body=True)
        child.start()
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        child.wait("wrapper_gate")
        send(peer, frame(b"B\x01data") + frame(b"C\x01data"))
        child.wait("host_event", kind=K["ingress_refused"], refusal=5)
        child.release()
        child.wait("executing_gate")
        os.kill(child.process.pid, signal.SIGTERM)
        child.release()
        child.finish()
        # C resource_limit was observed while A was paused BEFORE entry, proving
        # B retained the second slot. B's queued Qt admission event need not run
        # before A dispatches (AP15-C11-CL01).
        self.assertEqual(len(child.events("command_entered")), 1)
        self.assertEqual(len(child.events("unit_entered")), 1)
        outcomes = child.events("writer_completed")
        self.assertEqual(len(outcomes), 1)
        self.assertEqual(outcomes[0]["work"], dict(transaction=1, error=0, value=True, ticket_revoked=True))
        stop = child.events("stop_consumed")[0]
        cancelled = child.events("command_cancelled")
        self.assertEqual(len(cancelled), 1)
        self.assertLess(int(stop["ordinal"]), int(cancelled[0]["ordinal"]))
        self.assertNotIn("work", cancelled[0])
        self.assertLess(int(stop["ordinal"]), int(child.events("writer_ack")[0]["ordinal"]))
        self.assertLess(int(outcomes[0]["ordinal"]), int(child.events("writer_ack")[0]["ordinal"]))
        self.assertFalse(child.events("local_handoff"))

    def test_native_response_pressure_uses_bounded_refusal_HL16(self):
        port = free_port()
        child = Child(self, port=port, response_cap=3)
        child.start()
        peer = upgrade(child, port)
        send(peer, frame(b"A\x01data"))
        child.wait("host_event", refusal=5)
        snap = child.stop()
        self.assertNotIn(b"Adone", drain_peer(peer))
        self.assertEqual(snap["response_capacity"], "0")
        outcomes = child.events("writer_completed")
        self.assertEqual(len(outcomes), 1)
        self.assertEqual(outcomes[0]["work"], dict(transaction=1, error=0, value=True, ticket_revoked=True))

    def test_native_credentials_refuse_and_diagnostic_exception(self):
        port = free_port()
        child = Child(self, port=port, diagnostic_throw=True)
        child.start()
        peer = upgrade(child, port)
        send(peer, frame(b"C\x02data"))
        # Admission may succeed before the owner discovers the absent source.
        # Observe the actual terminal unavailable refusal, never forge its phase.
        refusal = child.wait("host_event", refusal=4)
        self.assertIn(refusal["kind"], (K["ingress_refused"], K["operation_refused"]))
        diagnostic_peer = child.peer(child.listeners["http_port"])
        send(diagnostic_peer, b"POST /api-host/diagnostic HTTP/1.1\r\nHost: localhost\r\nX-Host-Correlation: 44\r\nContent-Length: 0\r\n\r\n")
        child.wait("diagnostic_exception")
        child.stop()
        self.assertNotIn(b"Cdone", drain_peer(peer))
        self.assertNotEqual(b"diag", drain_peer(diagnostic_peer).split(b"\r\n\r\n")[-1])
        self.assertFalse(child.events("command_entered"))
        self.assertFalse(child.events("writer_completed"))
        self.assertFalse(child.events("unit_entered"))
        self.assertFalse(child.events("writer_completed"))

    def test_dual_ephemeral_endpoints_and_correct_positive_routes_L01_L02(self):
        child = Child(self)
        child.start()
        diagnostic(child.peer(child.listeners["http_port"]))
        peer = upgrade(child, child.listeners["port"])
        send(peer, frame(b"A\x01data"))
        self.assertEqual(response(peer), b"Adone")
        self.assertEqual(len([e for e in child.history if e.get("event") == "listeners"]), 1)
        child.stop()
        self.assertFalse(Path(child.listeners["scratch_root"]).exists())

    def test_fixed_and_ephemeral_role_combinations_L03(self):
        for role in ("http", "native", "both"):
            with self.subTest(role=role):
                first = free_port()
                for _ in range(8):
                    second = free_port()
                    if first != second:
                        break
                else:
                    self.fail("SETUP: eight distinct-port candidate attempts exhausted")
                child = Child(self, port=first if role != "http" else 0,
                              http_port=second if role != "native" else 0)
                child.start()
                diagnostic(child.peer(child.listeners["http_port"]))
                peer = upgrade(child, child.listeners["port"])
                send(peer, frame(b"A\x01data"))
                self.assertEqual(response(peer), b"Adone")
                child.stop()
                child.cleanup()

    def test_invalid_factory_options_before_host_L03(self):
        cases = [dict(port=41001, http_port=41001)]
        cases += [dict(address=a) for a in ("", "localhost", "127.1", " 127.0.0.1", "[::1]", "fe80::1%lo", "127.0.0.1\x00x", "1" * 46)]
        cases += [dict(scratch_parent=p) for p in ("relative", "/tmp/../tmp", "/tmp/./", "/" + "x" * 512)]
        for config in cases:
            with self.subTest(config=config):
                child = Child(self, validate_options=config)
                child.wait("factory_options_refused")
                self.assertEqual(child.process.wait(timeout=STEP), 0)
                child.cleanup()

    def test_cross_listener_fixed_refusals_and_fresh_positives_L05(self):
        child = Child(self)
        child.start()
        wrong_http = child.peer(child.listeners["port"])
        send(wrong_http, HTTP_DIAGNOSTIC)
        self.assertEqual(drain_peer(wrong_http), FIXED400)
        wrong_ws = child.peer(child.listeners["http_port"])
        send(wrong_ws, WS_UPGRADE)
        self.assertEqual(drain_peer(wrong_ws), FIXED400)
        child.command("inspect")
        self.assertFalse(child.events("diagnostic_entered"))
        self.assertFalse(child.events("command_entered"))
        diagnostic(child.peer(child.listeners["http_port"]))
        peer = upgrade(child, child.listeners["port"])
        send(peer, frame(b"A\x01data"))
        self.assertEqual(response(peer), b"Adone")
        child.stop()

    def test_no_static_index_traversal_or_default_leakage_L06(self):
        cases = [(method, path) for method in (b"GET", b"HEAD") for path in
                 (b"/", b"/index.html", b"/secret.txt", b"/%73ecret.txt", b"/../secret.txt",
                  b"/../upload/tmp/00/secret.txt", b"/%2e%2e/upload/tmp/00/secret.txt")]
        cases += [(b"OPTIONS", b"*"), (b"PUT", b"/api-host/diagnostic")]
        # One child per stimulus preserves the finite8-peer fixture cap without
        # recycling hidden peer accounting or changing the product's quotas.
        for method, path in cases:
            with self.subTest(method=method, path=path):
                child = Child(self)
                child.start()
                base = Path(child.listeners["scratch_root"])
                self.assertEqual(base.parent, Path(child.directory.name))
                self.assertTrue(base.name.startswith("subathon-host-"))
                self.assertEqual(base.stat().st_mode & 0o777, 0o700)
                for relative in ("empty/index.html", "empty/secret.txt", "secret.txt", "upload/tmp/00/secret.txt"):
                    target = base / relative
                    self.assertTrue(target.parent.is_dir())
                    with target.open("xb") as stream:
                        self.assertEqual(stream.write(b"PRIVATE-SENTINEL"), 16)
                peer = child.peer(child.listeners["http_port"])
                send(peer, method + b" " + path + b" HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n")
                wire = drain_peer(peer)
                self.assertLessEqual(len(wire), 1024)
                header, body = wire.split(b"\r\n\r\n", 1)
                self.assertTrue(header.startswith(b"HTTP/1.1 400 "))
                self.assertEqual(body, b"")
                self.assertNotIn(b"PRIVATE-SENTINEL", wire)
                self.assertNotIn(b"allow:", header.lower())
                self.assertNotIn(b"location:", header.lower())
                child.command("inspect")
                self.assertFalse(child.events("diagnostic_entered"))
                self.assertFalse(child.events("command_entered"))
                diagnostic(child.peer(child.listeners["http_port"]))
                child.stop()
                self.assertFalse(base.exists())
                child.cleanup()

    def test_http_occupied_port_and_reuseport_both_roles_L07(self):
        # Historical plain native-port case remains separate and unchanged.
        for role, reuse in (("http", False), ("http", True), ("native", True)):
            with self.subTest(role=role, reuse=reuse):
                lock_before = lock_identity()
                with socket.socket() as occupied:
                    occupied.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    if reuse:
                        occupied.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
                    occupied.bind(("127.0.0.1", 0))
                    occupied.listen(1)
                    port = occupied.getsockname()[1]
                    child = Child(self, port=port if role == "native" else 0,
                                  http_port=port if role == "http" else 0)
                    child.expected_terminal = 1
                    child.commands = 1
                    data = b'{"op":"start","request":1}\n'
                    self.assertEqual(os.write(child.process.stdin.fileno(), data), len(data))
                    deadline = time.monotonic() + STEP
                    while len(child.eof) != 3:
                        child.pump(deadline)
                    self.assertEqual(child.process.wait(timeout=child.remaining(deadline)), 1)
                    self.assertFalse(child.events("listener_ready"))
                    self.assertFalse(any(e.get("event") in ("listeners", "serving", "stopped") for e in child.history))
                    child.cleanup()  # supervisor owns terminal scratch residue
                self.assertEqual(lock_identity(), lock_before, "No drogon.lock probe effects")
                retry = Child(self, port=port if role == "native" else 0,
                              http_port=port if role == "http" else 0)
                retry.start()
                diagnostic(retry.peer(retry.listeners["http_port"]))
                peer = upgrade(retry, retry.listeners["port"])
                send(peer, frame(b"A\x01data"))
                self.assertEqual(response(peer), b"Adone")
                retry.stop()
                retry.cleanup()

    def test_endpoint_observer_exception_uses_failed_start_barriers_L02_L08(self):
        child = Child(self, observer_throw=True)
        child.command("start")
        child.wait("listeners")
        self.assertEqual(child.wait("startup_refused")["reason"], 4)
        snap = child.finish()
        self.assertEqual(snap["state"], 5)
        self.assertFalse(any(e.get("event") == "serving" for e in child.history))
        self.assertFalse(child.events("command_entered"))
        self.assertEqual(len(child.events("owner_cleanup")), 1)

    def test_shared_peer_limit_across_roles_L10(self):
        child = Child(self, peer_cap=2)
        child.start()
        http = child.peer(child.listeners["http_port"])
        diagnostic(http)  # establish first actual peer, retained keep-alive
        native = upgrade(child, child.listeners["port"])
        third = child.peer(child.listeners["http_port"])
        send(third, HTTP_DIAGNOSTIC)
        self.assertNotIn(b"diag", drain_peer(third))
        child.command("inspect")
        self.assertEqual(len(child.events("diagnostic_entered")), 1)
        send(http, HTTP_DIAGNOSTIC.replace(b"Connection: keep-alive", b"Connection: close"))
        self.assertEqual(http_reply(http)[1], b"diag")
        self.assertEqual(drain_peer(http), b"", "Actual server-side closure before replacement")
        http.close()
        # Synchronous protocol inspect observes Qt only; retry eligibility is
        # proved by actual serving on a fresh peer, under the ordinary IO turns.
        replacement = child.peer(child.listeners["http_port"])
        diagnostic(replacement)
        send(native, frame(b"A\x01data"))
        self.assertEqual(response(native), b"Adone")
        child.stop()

    def test_shared_associations_http_and_native_L10(self):
        child = Child(self, hold_diagnostic=True)
        child.start()
        http = child.peer(child.listeners["http_port"])
        send(http, HTTP_DIAGNOSTIC)
        child.wait("diagnostic_gate")  # HTTP A holds one global association on Qt
        native = upgrade(child, child.listeners["port"])
        # Coalesced records on ONE native connection fix B-before-C without
        # making an assumption about scheduling between two sockets.
        send(native, frame(b"B\x01data") + frame(b"C\x01data"))
        child.wait("host_event", kind=K["ingress_refused"], refusal=5)
        self.assertFalse(child.events("command_entered"))
        self.assertFalse(child.events("diagnostic_entered"))
        child.release()
        self.assertEqual(response(native), b"Bdone")
        self.assertEqual(http_reply(http)[1], b"diag")
        child.stop()
        self.assertEqual(len(child.events("diagnostic_entered")), 1)
        self.assertEqual(len(child.events("writer_completed")), 1)

    def test_body4_body5_and_exact_response_capacity_L11_L12(self):
        child = Child(self, body_cap=4, response_cap=4)
        child.start()
        peer = child.peer(child.listeners["http_port"])
        send(peer, b"POST /api-host/diagnostic HTTP/1.1\r\nHost: localhost\r\nX-Host-Correlation: 41\r\nContent-Length: 4\r\n\r\ndata")
        self.assertEqual(http_reply(peer)[1], b"diag")
        too_large = child.peer(child.listeners["http_port"])
        send(too_large, b"POST /api-host/diagnostic HTTP/1.1\r\nHost: localhost\r\nX-Host-Correlation: 42\r\nContent-Length: 5\r\n\r\n12345")
        self.assertNotIn(b"diag", drain_peer(too_large))
        native = upgrade(child, child.listeners["port"])
        send(native, frame(b"A\x01data"))
        self.assertEqual(response(native), b"Adone")  # five wire bytes: extra fixed scratch
        child.stop()
        self.assertEqual(len(child.events("diagnostic_entered")), 1)
        self.assertEqual(len(child.events("writer_completed")), 1)

    def test_second_coalesced_http_request_after_release_L04_L11(self):
        child = Child(self, hold_diagnostic=True)
        child.start()
        peer = child.peer(child.listeners["http_port"])
        send(peer, HTTP_DIAGNOSTIC + HTTP_DIAGNOSTIC)
        child.wait("diagnostic_gate")
        self.assertEqual(len(child.events("diagnostic_queued")), 1)
        self.assertFalse(child.events("diagnostic_entered"))
        child.release()
        self.assertEqual(http_reply(peer)[1], b"diag")
        self.assertEqual(http_reply(peer)[1], b"diag")
        child.stop()
        self.assertEqual(len(child.events("diagnostic_entered")), 2)
        self.assertFalse(child.events("writer_completed"))


    def test_pending_native_signal_cuts_forward_between_units(self):
        child = Child(self, gate_body=True, forward_after_signal=True)
        child.start()
        peer = upgrade(child, child.listeners["port"])
        send(peer, frame(b"A\x01data"))
        child.wait("executing_gate")
        os.kill(child.process.pid, signal.SIGTERM)
        child.release()
        child.wait("signal_forward_refused")
        child.finish()
        self.assertEqual(len(child.events("unit_entered")), 1)
        self.assertEqual(len(child.events("writer_completed")), 1)
        self.assertEqual(child.events("writer_completed")[0]["work"],
                         dict(transaction=1, error=0, value=True, ticket_revoked=True))
        refusal = child.events("forward_refused")
        self.assertEqual(len(refusal), 1)
        self.assertEqual(refusal[0]["refusal"], 3)
        self.assertNotIn("work", refusal[0])
        self.assertFalse(child.events("local_handoff"))

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
    require(options.admission.stat().st_size <= 524288, "SETUP: admission receipt cap")
    ADMISSION = json.loads(options.admission.read_text(encoding="utf-8"))
    require(set(ADMISSION) == {"format", "consumer", "initial_objects", "runtime_objects", "serving_objects", "stopped_objects", "environment"} and
            ADMISSION["format"] == 1, "SETUP: new-consumer admission receipt schema")
    require(file_identity(PROBE) == ADMISSION["consumer"], "SETUP: binary changed after admission")
    for name in ("initial_objects", "runtime_objects", "serving_objects", "stopped_objects"):
        require(isinstance(ADMISSION[name], dict) and 0 < len(ADMISSION[name]) <= 128, "SETUP: receipt closure bound")
    unittest.main(argv=[sys.argv[0], *rest])


if __name__ == "__main__":
    main()
