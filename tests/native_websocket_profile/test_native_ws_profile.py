# SPDX-License-Identifier: GPL-3.0-or-later
"""Literal RFC/profile oracles against a real, separately admitted loopback probe.

Stock wire/callback failures and Unimplemented first-party resource failures are
reported separately. No fixture interprets absent native counters as zero.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import queue
import select
import socket
import struct
import subprocess
import sys
import threading
import time
import unittest

PROBE: Path | None = None
CHILD_SECONDS = 10
IO_SECONDS = 4
MAX_PAYLOAD = 1048576
MAX_WIRE = MAX_PAYLOAD + 14
MAX_LINE = 65536
MAX_LOG = 2 * 1024 * 1024
MAX_EVENTS = 256

REFERENCE = dict(connections=32, incomplete_upgrades=16, frame_bytes=1048576,
                 message_bytes=1048576, frames_per_message=4096,
                 callback_leases_per_connection=2, callback_bytes_per_connection=2097152,
                 callback_bytes_global=16777216, raw_capacity_bytes=1048590,
                 assembly_capacity_bytes=1048576, application_output_bytes=1048590,
                 control_output_bytes=254, frames_per_turn=64, upgrade_ms=5000,
                 frame_ms=10000, message_ms=30000, close_ms=1000)
SMALL = dict(connections=2, incomplete_upgrades=2, frame_bytes=64, message_bytes=64,
             frames_per_message=4, callback_leases_per_connection=2,
             callback_bytes_per_connection=128, callback_bytes_global=256,
             raw_capacity_bytes=128, assembly_capacity_bytes=128,
             application_output_bytes=128, control_output_bytes=254,
             frames_per_turn=2, upgrade_ms=1000, frame_ms=10, message_ms=30, close_ms=10)


def frame(opcode: int, body: bytes = b"", *, fin=True, mask=True) -> bytes:
    """Independent client encoding; malformed-header oracles below are literal bytes."""
    if len(body) > MAX_PAYLOAD + 1:
        raise AssertionError("peer input cap")
    first = (0x80 if fin else 0) | opcode
    length = len(body)
    second = 0x80 if mask else 0
    if length < 126:
        header = bytes((first, second | length))
    elif length <= 65535:
        header = bytes((first, second | 126)) + struct.pack("!H", length)
    else:
        header = bytes((first, second | 127)) + struct.pack("!Q", length)
    if not mask:
        return header + body
    key = b"\x01\x02\x03\x04"
    return header + key + bytes(value ^ key[i % 4] for i, value in enumerate(body))


def close_body(code: int, reason=b"") -> bytes:
    return struct.pack("!H", code) + reason


class Probe:
    """Own one native lifetime, bounded pipe readers and at most one control request."""

    def __init__(self, owner, *, profile=False, factory=False, limits=None, **settings):
        self.end = time.monotonic() + CHILD_SECONDS
        self.process = None
        self.readers = []
        self.reader_stop = threading.Event()
        self.events = queue.Queue(MAX_EVENTS)
        self.pending = []
        self.tail = []
        self.error = None
        self.log_bytes = 0
        self.lock = threading.Lock()
        self.request = 0
        self.cleaned = False
        self.cleanup_attempts = 0
        self.cleanup_errors = []
        self.cleanup_error_count = 0
        self.sockets = []
        owner.addCleanup(self.check_cleanup)
        config = dict(mode="factory" if factory else "profile" if profile else "wire",
                      clock="manual" if profile else "native", limits=REFERENCE if limits is None else limits)
        config.update(settings)
        encoded = json.dumps(config, separators=(",", ":"))
        if len(encoded) > 4096:
            raise AssertionError("settings cap")
        try:
            self.process = subprocess.Popen([str(PROBE), encoded], stdin=subprocess.PIPE,
                                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                            bufsize=8192, close_fds=True)
            for name in ("stdout", "stderr"):
                os.set_blocking(getattr(self.process, name).fileno(), False)
                reader = threading.Thread(target=self._read, args=(name,), daemon=False)
                reader.start()
                self.readers.append(reader)
            self.identity = self.wait("identity")
            if self.identity["consumer_cplusplus"] < 202002:
                raise AssertionError("SETUP: native consumer is not C++20")
            if not self.identity["complete"]:
                raise AssertionError("SETUP: incomplete actual loaded-object inventory")
            self.factory = None if settings.get("factory_action") == "after_start" else self.wait("factory", repeat=False)
            self.port = None
        except BaseException:
            self.cleanup()
            raise

    def remaining(self):
        value = self.end - time.monotonic()
        if value <= 0:
            raise AssertionError("bounded scenario deadline; inspect native/setup attribution")
        return min(IO_SECONDS, value)

    def _read(self, name):
        # Nonblocking owned pipes let cleanup stop/join readers even after a failed reap.
        fd = getattr(self.process, name).fileno()
        poller = select.poll()
        poller.register(fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        partial = bytearray()
        try:
            while not self.reader_stop.is_set():
                if not poller.poll(100):
                    continue
                try:
                    block = os.read(fd, 4096)
                except (BlockingIOError, InterruptedError):
                    continue
                if not block:
                    if partial and self.error is None:
                        self.error = "truncated probe output line"
                    return
                with self.lock:
                    self.log_bytes += len(block)
                    if self.log_bytes > MAX_LOG:
                        self.error = "fixture output exceeded finite reader bounds"
                # At most MAX_LINE+4096 bytes live here; after overflow, drain without retaining.
                if self.error:
                    partial.clear()
                    continue
                partial.extend(block)
                while b"\n" in partial:
                    end = partial.index(10) + 1
                    line = bytes(partial[:end])
                    del partial[:end]
                    if len(line) > MAX_LINE:
                        self.error = "fixture output line exceeded finite bounds"
                        break
                    with self.lock:
                        self.tail.append((name, line[:512].decode("utf-8", "replace")))
                        del self.tail[:-24]
                    if name != "stdout":
                        continue
                    try:
                        value = json.loads(line)
                    except (ValueError, UnicodeDecodeError):
                        self.error = "non-JSON probe stdout"
                        break
                    if not isinstance(value, dict):
                        self.error = "non-object probe event"
                        break
                    try:
                        self.events.put_nowait(value)
                    except queue.Full:
                        self.error = "fixture event queue exceeded finite bounds"
                        break
                if len(partial) > MAX_LINE:
                    self.error = "fixture output line exceeded finite bounds"
                if self.error:
                    partial.clear()
        except BaseException as error:
            if self.error is None:
                self.error = f"pipe reader: {type(error).__name__}"

    def wait(self, name, **fields):
        def match(v):
            return v.get("event") == name and all(v.get(k) == val for k, val in fields.items())
        for i, v in enumerate(self.pending):
            if match(v):
                return self.pending.pop(i)
        while True:
            if self.error:
                raise AssertionError(self.error)
            try:
                v = self.events.get(timeout=min(0.1, self.remaining()))
            except queue.Empty:
                if self.process.poll() is not None and not any(r.is_alive() for r in self.readers):
                    raise AssertionError(f"SETUP/PROBE exited {self.process.returncode} before {name}: {self.tail}")
                continue
            if v.get("event") == "fixture_error":
                raise AssertionError(f"SETUP/FIXTURE failure: {v}")
            if match(v):
                return v
            if len(self.pending) >= MAX_EVENTS:
                raise AssertionError("pending event cap")
            self.pending.append(v)

    def start(self, require_profile=False):
        if require_profile and self.factory["status"] != "created":
            raise AssertionError(f"MISSING_PROFILE_BEHAVIOR (not native capacity evidence): {self.factory}")
        if self.identity["backend"] == "admitted_corrected" and self.factory["status"] != "created":
            raise AssertionError(f"SETUP: valid corrected profile factory failed: {self.factory}")
        ready = self.wait("ready")
        self.port = ready["port"]
        self.ordinary_port = ready.get("ordinary_port")
        if not isinstance(self.port, int) or not 0 < self.port < 65536:
            raise AssertionError("SETUP: invalid actual listener port")
        if require_profile and not ready["profile"]:
            raise AssertionError("SETUP: factory/profile mismatch")
        return self

    def control(self, op, **fields):
        self.request += 1
        if self.request > 512:
            raise AssertionError("command quota")
        request = dict(op=op, request=self.request, **fields)
        data = json.dumps(request, separators=(",", ":")).encode() + b"\n"
        if len(data) > 4096:
            raise AssertionError("command cap")
        self.process.stdin.write(data)
        self.process.stdin.flush()
        return self.wait("result", request=self.request)

    def inspect(self, peer):
        result = self.control("inspect", connection=peer.id)
        if result["status"] != "ok":
            raise AssertionError(f"missing actual-owner snapshot: {result}")
        return result["snapshot"]

    def progress(self, peer, *, read=0, consumed=0):
        # Only actual source counters establish network progress; stdin ack does not.
        for _ in range(64):
            result = self.inspect(peer)
            if result["ws_bytes_read"] >= read and result["ws_bytes_consumed"] >= consumed:
                return result
        raise AssertionError("actual native network progress not observed within bounded controls")

    def global_snapshot(self):
        result = self.control("global")
        if result["status"] != "ok":
            raise AssertionError(f"missing actual global-owner snapshot: {result}")
        return result["snapshot"]

    def retire(self, owners):
        for _ in range(64):
            self.control("barrier")
            value = self.global_snapshot()
            if value["owners_live_or_retiring"] == owners:
                return value
        raise AssertionError("actual retirement not observed; barrier is not retirement proof")

    def set_time(self, now, *, unavailable=False):
        result = self.control("clock", now=now, unavailable=unavailable)
        if result["status"] != "ok":
            raise AssertionError(result)

    def assert_no_data(self):
        # Call only after a real native closed event or a positive consumed-byte barrier.
        if len(self.pending) > MAX_EVENTS:
            raise AssertionError("pending event cap already violated")
        for value in self.pending:
            if value.get("event") == "message":
                raise AssertionError(f"unexpected application data: {value}")
        while True:
            try:
                value = self.events.get_nowait()
            except queue.Empty:
                break
            # Keep the actual offending observation as the primary assertion, even at cap.
            if value.get("event") == "message":
                raise AssertionError(f"unexpected application data: {value}")
            if value.get("event") == "fixture_error":
                raise AssertionError(f"SETUP/FIXTURE failure: {value}")
            if len(self.pending) >= MAX_EVENTS:
                raise AssertionError(f"pending event cap during no-data observation: {value}")
            self.pending.append(value)

    def cleanup(self):
        """Best effort, nonraising; TestCase reports its independent cleanup verdict later."""
        if self.cleaned or self.cleanup_attempts >= 2:
            return
        self.cleanup_attempts += 1

        def problem(step, error=None):
            self.cleanup_error_count += 1
            if len(self.cleanup_errors) < 16:
                self.cleanup_errors.append(step + (f":{type(error).__name__}" if error else ""))

        for peer in self.sockets:
            try:
                peer.close_socket()
            except BaseException as error:
                problem("socket_close", error)
        if self.process is None:
            self.cleaned = True
            return
        try:
            if self.process.stdin and not self.process.stdin.closed:
                self.process.stdin.close()
        except BaseException as error:
            problem("stdin_close", error)
        try:
            self.process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            problem("forced_kill_required")
        except BaseException as error:
            problem("graceful_wait", error)
        if self.process.poll() is None:
            try:
                self.process.kill()
            except BaseException as error:
                problem("kill", error)
        # Always attempt reaping, even if close/wait/kill failed.
        try:
            code = self.process.wait(timeout=2)
            if code != 0:
                problem(f"nonzero_exit_{code}")
        except BaseException as error:
            problem("reap", error)
        if self.process.poll() is None:
            self.reader_stop.set()
        for index, reader in enumerate(self.readers):
            try:
                reader.join(timeout=2)
                if reader.is_alive():
                    problem(f"reader_{index}_drain_incomplete")
                    self.reader_stop.set()
                    reader.join(timeout=1)
                    if reader.is_alive():
                        problem(f"reader_{index}_unjoined")
            except BaseException as error:
                problem(f"reader_{index}_join", error)
                self.reader_stop.set()
        for name in ("stdin", "stdout", "stderr"):
            stream = getattr(self.process, name)
            try:
                if stream and not stream.closed:
                    stream.close()
            except BaseException as error:
                problem(f"{name}_close", error)
        if self.error:
            problem(f"capture_{self.error}")
        streams_closed = all(getattr(self.process, name) is None or getattr(self.process, name).closed
                             for name in ("stdin", "stdout", "stderr"))
        sockets_closed = all(peer.socket.fileno() == -1 for peer in self.sockets)
        self.cleaned = (self.process.poll() is not None and streams_closed and sockets_closed
                        and not any(r.is_alive() for r in self.readers))

    def check_cleanup(self):
        # Registered before Popen so even constructor failures keep their primary assertion.
        # Helpers call nonraising cleanup in finally; this distinct unittest cleanup verdict
        # reports all failures without replacing that protocol/setup assertion.
        self.cleanup()
        if self.cleanup_error_count:
            raise AssertionError(f"FIXTURE_CLEANUP separate verdict: count={self.cleanup_error_count}, "
                                 f"steps={self.cleanup_errors}, bounded_context={self.tail}")


class Peer:
    def __init__(self, probe, *, upgrade=True, tiny_receive=False, port=None):
        self.probe = probe
        if len(probe.sockets) == 64:
            raise AssertionError("fixture peer cap")
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        probe.sockets.append(self)
        self.buffer = bytearray()
        self.id = None
        if tiny_receive:
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 16384)
        self.socket.settimeout(probe.remaining())
        self.socket.connect(("127.0.0.1", probe.port if port is None else port))
        if not upgrade:
            return
        self.send(b"GET /native-profile HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                  b"Connection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                  b"Sec-WebSocket-Version: 13\r\n\r\n")
        header = bytearray()
        while b"\r\n\r\n" not in header:
            if len(header) == 16384:
                raise AssertionError("upgrade header cap")
            self.socket.settimeout(probe.remaining())
            block = self.socket.recv(min(1024, 16384 - len(header)))
            if not block:
                raise AssertionError("native upgrade failed before headers")
            header.extend(block)
        headers, rest = header.split(b"\r\n\r\n", 1)
        self.buffer.extend(rest)
        lines = headers.decode("ascii").split("\r\n")
        if lines[0].split()[1] != "101":
            raise AssertionError(f"native upgrade refused: {lines[0]}")
        fields = {name.lower(): value.strip() for name, value in
                  (line.split(":", 1) for line in lines[1:])}
        # RFC6455's literal sample handshake, not an expected value from the probe.
        if fields.get("sec-websocket-accept") != "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=":
            raise AssertionError("native upgrade accept mismatch")
        if "sec-websocket-extensions" in fields:
            raise AssertionError("native profile negotiated an extension")
        self.id = probe.wait("opened")["connection"]

    def send(self, data):
        if len(data) > MAX_WIRE + 16:
            raise AssertionError("peer send cap")
        self.socket.settimeout(self.probe.remaining())
        self.socket.sendall(data)

    def exact(self, length):
        if length > MAX_WIRE:
            raise AssertionError("peer receive cap")
        while len(self.buffer) < length:
            self.socket.settimeout(self.probe.remaining())
            block = self.socket.recv(min(16384, max(1, length - len(self.buffer))))
            if not block:
                raise EOFError("native TCP EOF before complete expected frame")
            self.buffer.extend(block)
            if len(self.buffer) > MAX_WIRE:
                raise AssertionError("peer receive backlog cap")
        out = bytes(self.buffer[:length])
        del self.buffer[:length]
        return out

    def receive(self):
        first, second = self.exact(2)
        if first & 0x70 or second & 0x80:
            raise AssertionError("server RSV/mask violation")
        n = second & 127
        if n == 126:
            n = struct.unpack("!H", self.exact(2))[0]
            if n < 126:
                raise AssertionError("nonminimal server frame")
        elif n == 127:
            n = struct.unpack("!Q", self.exact(8))[0]
            if n <= 65535 or n >> 63:
                raise AssertionError("nonminimal/highbit server frame")
        if n > MAX_PAYLOAD:
            raise AssertionError("server frame cap")
        self.last_fin = bool(first & 0x80)
        if (first & 15) >= 8 and not self.last_fin:
            raise AssertionError("fragmented server control")
        return first & 15, self.exact(n)

    def receive_message(self):
        opcode, first = self.receive()
        if opcode not in (1, 2):
            raise AssertionError(f"expected data, received control/opcode {opcode}")
        body = bytearray(first)
        for _ in range(4096):
            if self.last_fin:
                return opcode, bytes(body)
            next_opcode, fragment = self.receive()
            if next_opcode != 0 or len(body) + len(fragment) > MAX_PAYLOAD:
                raise AssertionError("invalid/beyond-bound server fragmentation")
            body.extend(fragment)
        raise AssertionError("server fragment count cap")

    def refused_upgrade(self):
        # Resource admission may refuse HTTP or close TCP; the profile fixes no HTTP error schema.
        header = bytearray()
        while b"\r\n\r\n" not in header:
            self.socket.settimeout(self.probe.remaining())
            try:
                block = self.socket.recv(min(1024, 16384 - len(header)))
            except ConnectionResetError:
                block = b""
            if not block:
                return
            header.extend(block)
            if len(header) >= 16384:
                raise AssertionError("refusal header cap")
        first = header.split(b"\r\n", 1)[0].decode("ascii").split()
        if len(first) < 2 or not first[0].startswith("HTTP/") or first[1] == "101":
            raise AssertionError("full profile owner pool admitted an extra Upgrade")

    def eof(self):
        if self.buffer:
            raise AssertionError(f"unexpected buffered wire after closure: {bytes(self.buffer[:32]).hex()}")
        self.socket.settimeout(self.probe.remaining())
        try:
            data = self.socket.recv(1)
        except ConnectionResetError:
            data = b""
        if data:
            raise AssertionError(f"unexpected wire before EOF: {data.hex()}")

    def close_socket(self):
        self.socket.close()


class NativeProfile(unittest.TestCase):
    def start(self, *, profile=False, **settings):
        probe = Probe(self, profile=profile, **settings)
        try:
            return probe.start(require_profile=profile)
        except BaseException:
            probe.cleanup()
            raise

    def message(self, probe, peer, kind, body):
        value = probe.wait("message", connection=peer.id)
        self.assertEqual(value["kind"], kind)
        self.assertEqual(value["payload"]["length"], len(body))
        self.assertEqual(value["payload"]["sha256"], hashlib.sha256(body).hexdigest())
        if len(body) <= 32:
            self.assertEqual(value["payload"]["hex"], body.hex())
        return value

    def closed(self, probe, peer):
        peer.eof()
        peer.close_socket()
        return probe.wait("closed", connection=peer.id)

    def peer_close(self, body, expected=None, *, prefix=b""):
        probe = self.start()
        try:
            peer = Peer(probe)
            peer.send(prefix + frame(8, body))
            self.assertEqual(peer.receive(), (8, body if expected is None else expected))
            self.closed(probe, peer)
            probe.assert_no_data()
        finally:
            probe.cleanup()

    def invalid(self, wire, code=1002):
        probe = self.start()
        try:
            peer = Peer(probe)
            peer.send(wire)
            self.assertEqual(peer.receive(), (8, close_body(code)))
            self.closed(probe, peer)
            probe.assert_no_data()
        finally:
            probe.cleanup()

    def http_peer(self, probe, *, ordinary=True):
        port = probe.ordinary_port if ordinary else probe.port
        self.assertIsInstance(port, int, "SETUP: missing actual sibling port")
        self.assertGreater(port, 0)
        self.assertLess(port, 65536)
        if ordinary:
            self.assertNotEqual(port, probe.port)
        return Peer(probe, upgrade=False, port=port)

    def http_headers(self, peer):
        # Independent peer-side HTTP observation only; no parser/guard in the native fixture.
        header = bytearray()
        for _ in range(4096):
            header.extend(peer.exact(1))
            if header.endswith(b"\r\n\r\n"):
                break
        else:
            self.fail("HTTP observation header cap")
        lines = header.decode("ascii").split("\r\n")
        status_line = lines[0].split()
        self.assertEqual(status_line[0], "HTTP/1.1")
        self.assertEqual(len(status_line[1]), 3)
        fields = {}
        for line in lines[1:-2]:
            name, value = line.split(":", 1)
            name = name.lower()
            self.assertNotIn(name, fields)
            fields[name] = value.strip()
        return int(status_line[1]), fields, len(header)

    def ordinary_request(self, probe, peer, ordinal):
        peer.send(b"GET /ordinary-probe HTTP/1.1\r\nHost: localhost\r\n"
                  b"Connection: keep-alive\r\n\r\n")
        status, fields, _ = self.http_headers(peer)
        self.assertEqual(status, 200)
        self.assertNotIn("transfer-encoding", fields)
        self.assertEqual(fields["content-length"], "12")
        self.assertEqual(peer.exact(12), b"ordinary-ok\n")
        self.assertEqual(fields["x-probe-request"], str(ordinal))
        self.assertEqual(probe.wait("ordinary_request")["ordinal"], ordinal)

    def upgrade_owners(self, probe, expected):
        for _ in range(64):
            value = probe.global_snapshot()
            if value["upgrade_owners"] == expected:
                return value
        self.fail("actual incomplete-upgrade ownership/retirement not observed")

    def assert_no_native_delivery(self, probe):
        # A refused data ownership claim would still expose an illicit native data callback.
        probe.assert_no_data()
        self.assertFalse(any(v.get("event") == "callback_refused" for v in probe.pending))

    def test_local_close_preserves_inflight_fragment_sequence_until_peer_close(self):
        # Profile sections 2/3: a local Close ends delivery, not a legal peer sequence.
        p = self.start(profile=True, limits=SMALL)
        c = Peer(p)
        first = bytes.fromhex("01 81 01 02 03 04 e3")  # Nonfinal Text e2.
        c.send(first)
        before = p.progress(c, consumed=7)
        self.assertEqual(before["frames_consumed"], 1)
        self.assertEqual(before["message_deadline"], 1030000000)
        self.assertEqual(before["first_peer_close"], {"kind": "no_close_frame"})
        self.assert_no_native_delivery(p)
        r = p.control("close", connection=c.id, code=1000)
        self.assertEqual((r["status"], r["disposition"]), ("ok", "started"))
        self.assertEqual(c.receive(), (8, b"\x03\xe8"))
        # Legal nonfinal continuation 82, then Ping p. A Pong and consumed counters
        # establish parsing past the continuation before the final fragment/peer Close.
        c.send(bytes.fromhex("00 81 01 02 03 04 83 89 81 05 06 07 08 75"))
        self.assertEqual(c.receive(), (10, b"p"))
        closing = p.progress(c, consumed=21)
        self.assertEqual(closing["frames_consumed"], 3)
        self.assertTrue(closing["close_sent"])
        self.assertFalse(closing["transport_closed"])
        self.assertEqual(closing["close_deadline"], 1010000000)
        self.assertEqual(closing["first_peer_close"], {"kind": "no_close_frame"})
        self.assert_no_native_delivery(p)
        # Final ac completes scalar e2 82 ac; valid peer 1001/bye must be retained.
        c.send(bytes.fromhex("80 81 01 02 03 04 ad 88 85 01 02 03 04 02 eb 61 7d 64"))
        closed = self.closed(p, c)  # EOF also rejects any second local Close or data frame.
        self.assertEqual(closed["inspection"]["status"], "ok")
        terminal = closed["inspection"]["snapshot"]
        self.assertTrue(terminal["transport_closed"])
        self.assertTrue(terminal["close_sent"])
        self.assertEqual(terminal["frames_consumed"], 5)
        self.assertEqual(terminal["ws_bytes_consumed"], 39)
        self.assertEqual(terminal["first_peer_close"],
                         {"kind": "close_with_status", "code": 1001, "reason_hex": "627965"})
        self.assert_no_native_delivery(p)

    def test_native_traffic_outlives_framework_http_idle_timeout(self):
        p = self.start(profile=True, clock="native", limits=SMALL, idle_observer=True)
        c = Peer(p)
        # Real IO-loop observations pace active native Ping/Pong service past the 2s
        # HTTP idle timeout and its <=1s wheel tick. No ordering sleep or fixture send.
        for ordinal in range(1, 8):
            marker = p.wait("idle_observed", connection=c.id)
            self.assertEqual(marker["ordinal"], ordinal)
            self.assertEqual(marker["inspection"]["status"], "ok")
            s = marker["inspection"]["snapshot"]
            self.assertFalse(s["transport_closed"])
            self.assertFalse(s["close_sent"])
            self.assertIsNone(s["frame_deadline"])
            self.assertIsNone(s["message_deadline"])
            self.assertIsNone(s["close_deadline"])
            c.send(bytes.fromhex("89 81 01 02 03 04 79"))
            self.assertEqual(c.receive(), (10, b"x"))
        self.assertGreaterEqual(marker["elapsed_ns"], 3500000000)
        c.send(bytes.fromhex("88 80 01 02 03 04"))
        self.assertEqual(c.receive(), (8, b""))
        closed = self.closed(p, c)
        self.assertEqual(closed["inspection"]["status"], "ok")
        self.assertEqual(closed["inspection"]["snapshot"]["first_peer_close"],
                         {"kind": "close_without_status"})
        self.assert_no_native_delivery(p)

    def test_marked_listener_refuses_complete_http_before_ordinary_dispatch(self):
        p = self.start(profile=True, limits=SMALL, ordinary_listener=True)
        native = self.http_peer(p, ordinary=False)
        native.send(b"GET /ordinary-probe HTTP/1.1\r\nHost: localhost\r\n"
                    b"Connection: close\r\n\r\n")
        status, _, total = self.http_headers(native)
        self.assertGreaterEqual(status, 400)
        self.assertLessEqual(status, 599)  # No error-body/status schema is imposed.
        # A complete HTTP refusal has finite wire/EOF bounds, independent of its encoding.
        for _ in range(4096 - total + 1):
            native.socket.settimeout(p.remaining())
            try:
                block = native.socket.recv(1)
            except ConnectionResetError:
                block = b""
            if not block:
                break
            total += 1
            self.assertLessEqual(total, 4096, "HTTP refusal observation cap")
        else:
            self.fail("bounded native HTTP refusal did not drain/close")
        native.close_socket()
        self.assertEqual(self.upgrade_owners(p, 0)["owners_live_or_retiring"], 0)
        ordinary = self.http_peer(p)
        # The first route invocation must be this ordinary request, never the refused one.
        self.ordinary_request(p, ordinary, 1)
        self.assertEqual(p.global_snapshot()["upgrade_owners"], 0)

    def test_ordinary_listener_is_not_charged_and_serves_with_native_slots_full(self):
        p = self.start(profile=True, limits=SMALL, ordinary_listener=True)
        ordinary = self.http_peer(p)
        self.ordinary_request(p, ordinary, 1)
        empty = p.global_snapshot()
        self.assertEqual(empty["upgrade_owners"], 0)
        self.assertEqual(empty["owners_live_or_retiring"], 0)
        a, b = Peer(p, upgrade=False), Peer(p, upgrade=False)
        partial = b"GET /native-profile HTTP/1.1\r\nUpgrade: websocket\r\n"
        a.send(partial)
        b.send(partial)
        full = self.upgrade_owners(p, 2)
        self.assertEqual(full["owners_live_or_retiring"], 0)
        self.ordinary_request(p, ordinary, 2)
        fresh = self.http_peer(p)
        self.ordinary_request(p, fresh, 3)  # New accepts must also bypass native admission.
        full = p.global_snapshot()
        self.assertEqual(full["upgrade_owners"], 2)
        self.assertEqual(full["upgrade_owners_peak"], 2)
        self.assertEqual(full["owners_live_or_retiring"], 0)

    def test_ordinary_keepalive_survives_native_upgrade_deadline(self):
        p = self.start(profile=True, limits=SMALL, ordinary_listener=True)
        ordinary = self.http_peer(p)
        self.ordinary_request(p, ordinary, 1)
        native = Peer(p, upgrade=False)
        native.send(b"GET /native-profile HTTP/1.1\r\nUpgrade: websocket\r\n")
        self.assertEqual(self.upgrade_owners(p, 1)["owners_live_or_retiring"], 0)
        p.set_time(2000000000)  # Initial 1s + literal 1000ms upgrade deadline, equality.
        native.refused_upgrade()  # Actual refusal/close, not the clock-command ack.
        native.close_socket()
        self.assertEqual(self.upgrade_owners(p, 0)["owners_live_or_retiring"], 0)
        self.ordinary_request(p, ordinary, 2)
        fresh = self.http_peer(p)
        self.ordinary_request(p, fresh, 3)
        self.assertEqual(p.global_snapshot()["upgrade_owners"], 0)

    def test_ordinary_listener_refuses_websocket_upgrade_without_stock_fallback(self):
        p = self.start(profile=True, limits=SMALL, ordinary_listener=True)
        attempted = self.http_peer(p)
        attempted.send(b"GET /native-profile HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                       b"Connection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                       b"Sec-WebSocket-Version: 13\r\n\r\n")
        attempted.refused_upgrade()
        attempted.close_socket()
        ordinary = self.http_peer(p)
        self.ordinary_request(p, ordinary, 1)  # Positive owner-dispatch barrier.
        self.assertFalse(any(v.get("event") == "opened" for v in p.pending))
        actual = p.global_snapshot()
        self.assertEqual(actual["upgrade_owners"], 0)
        self.assertEqual(actual["owners_live_or_retiring"], 0)

    def test_ws01_persistent_text_binary_and_empty_messages(self):
        p = self.start()
        c = Peer(p)
        c.send(bytes.fromhex("81 81 01 02 03 04 40 82 81 05 06 07 08 47"))
        self.message(p, c, "text", b"A")
        self.message(p, c, "binary", b"B")
        c.send(frame(1) + frame(2))
        self.message(p, c, "text", b"")
        self.message(p, c, "binary", b"")
        c.send(frame(1, b"still open"))
        self.message(p, c, "text", b"still open")

    def test_ws02_fragmented_scalar_and_control_are_separate(self):
        p = self.start()
        c = Peer(p)
        c.send(bytes.fromhex("01 81 01 02 03 04 e3 89 81 05 06 07 08 75 80 82 09 0a 0b 0c 8b a6"))
        self.assertEqual(c.receive(), (10, b"p"))
        self.message(p, c, "text", b"\xe2\x82\xac")
        c.send(frame(2, b"\x00\xff", fin=False) + frame(10, b"ignored") + frame(0, b"\x80"))
        self.message(p, c, "binary", b"\x00\xff\x80")

    def test_ws03_close_does_not_consume_unfinished_data(self):
        self.peer_close(close_body(1000), prefix=bytes.fromhex("01 81 01 02 03 04 40"))

    def test_ws04_ws05_ws09_ws20_peer_close_exact_body_and_direction(self):
        cases = [(b"", b""), (close_body(1001, b"bye"), close_body(1001, b"bye")),
                 (close_body(4001), close_body(4001)), (close_body(1010), b"")]
        for body, reply in cases:
            with self.subTest(body=body.hex()):
                self.peer_close(body, reply)

    def test_ws06_ws07_ws08_close_body_validation(self):
        cases = [(bytes.fromhex("88 81 01 02 03 04 02"), 1002),
                 (bytes.fromhex("88 84 01 02 03 04 02 ea c3 ab"), 1007),
                 (bytes.fromhex("88 82 01 02 03 04 02 ef"), 1002)]
        for wire, code in cases:
            with self.subTest(wire=wire.hex()):
                self.invalid(wire, code)
        self.peer_close(close_body(1012))

    def test_status_set_boundaries_and_reserved_holes(self):
        for code in (1000, 1002, 1003, 1007, 1008, 1009, 1011, 1013, 1014, 3000, 4999):
            with self.subTest(valid=code):
                self.peer_close(close_body(code))
        for code in (0, 999, 1004, 1005, 1006, 1015, 1016, 2999, 5000, 65535):
            with self.subTest(invalid=code):
                self.invalid(frame(8, close_body(code)))

    def test_close_reason_and_text_unicode_scalar_boundaries(self):
        invalid = (b"\xc0\xaf", b"\xed\xa0\x80", b"\xf4\x90\x80\x80",
                   b"\x80", b"\xe2\x82", b"\xf0\x80\x80\x80")
        for body in invalid:
            with self.subTest(reason=body.hex()):
                self.invalid(frame(8, close_body(1000, body)), 1007)
            with self.subTest(text=body.hex()):
                self.invalid(frame(1, body), 1007)
        self.peer_close(close_body(1000, b"a" * 123))
        self.invalid(frame(8, close_body(1000, b"a" * 124)))
        p = self.start()
        c = Peer(p)
        valid = b"\x00\xc2\x80\xed\x9f\xbf\xf4\x8f\xbf\xbf"
        c.send(frame(1, valid))
        self.message(p, c, "text", valid)

    def test_ws12_pong_does_not_become_application_data(self):
        p = self.start()
        c = Peer(p)
        c.send(bytes.fromhex("8a 80 01 02 03 04") + frame(1, b"after pong"))
        self.message(p, c, "text", b"after pong")
        p.control("native_send", connection=c.id, length=1, octet=81)
        self.assertEqual(c.receive(), (1, b"Q"))  # Any illicit Pong would precede this frame.
        p.assert_no_data()

    def test_ws13_ws14_ws16_header_and_sequence_validation(self):
        wires = ["81 01 41", "c1 80 01 02 03 04", "09 80 01 02 03 04",
                 "80 80 01 02 03 04", "01 81 01 02 03 04 40 82 80 05 06 07 08",
                 "89 fe 00 00 01 02 03 04", "81 fe 00 01 01 02 03 04 40",
                 "83 80 01 02 03 04", "8b 80 01 02 03 04", "a2 80 01 02 03 04",
                 "91 80 01 02 03 04", "81 ff 80 00 00 00 00 00 00 00",
                 "82 ff 00 00 00 00 00 00 00 01 01 02 03 04 40"]
        for wire in wires:
            with self.subTest(wire=wire):
                self.invalid(bytes.fromhex(wire))

    def test_ws15_invalid_complete_text_is_not_repaired(self):
        self.invalid(bytes.fromhex("81 82 01 02 03 04 c1 ad"), 1007)
        self.invalid(frame(1, b"\xe2", fin=False) + frame(0, b"A"), 1007)

    def test_ws17_reference_exact_frame_and_advertised_one_over(self):
        p = self.start()
        c = Peer(p)
        body = b"Z" * MAX_PAYLOAD
        c.send(frame(2, body))
        self.message(p, c, "binary", body)
        # Advertised length only: no one-over body is sent or generated by the probe.
        c.send(bytes.fromhex("82 ff 00 00 00 00 00 10 00 01 01 02 03 04"))
        self.assertEqual(c.receive(), (8, close_body(1009)))
        self.closed(p, c)

    def test_fragmented_assembled_message_one_over(self):
        p = self.start()
        c = Peer(p)
        c.send(frame(2, b"Z" * MAX_PAYLOAD, fin=False))
        c.send(frame(0, b"!"))
        self.assertEqual(c.receive(), (8, close_body(1009)))
        self.closed(p, c)
        p.assert_no_data()

    def test_ws10_server_then_peer_close_and_no_second_reply(self):
        p = self.start()
        c = Peer(p)
        result = p.control("native_close", connection=c.id, code=1000, reason="")
        self.assertEqual(result["status"], "public_native_call")
        self.assertEqual(c.receive(), (8, close_body(1000)))
        c.send(frame(8, close_body(1001, b"bye")))
        self.closed(p, c)
        p.assert_no_data()

    def test_ws18_actual_empty_close_differs_from_abrupt_eof(self):
        # Tagged native ownership is a profile obligation; raw EOF cannot invent1000.
        p = self.start(profile=True)
        a = Peer(p)
        a.socket.shutdown(socket.SHUT_RDWR)
        closed = p.wait("closed", connection=a.id)
        inspection = closed["inspection"]
        self.assertEqual(inspection["status"], "ok")
        self.assertEqual(inspection["snapshot"]["first_peer_close"], {"kind": "no_close_frame"})
        b = Peer(p)
        b.send(frame(8))
        self.assertEqual(b.receive(), (8, b""))
        closed = self.closed(p, b)
        self.assertEqual(closed["inspection"]["snapshot"]["first_peer_close"],
                         {"kind": "close_without_status"})

    def test_ws11_ws19_ping_repeated_close_and_output_refusal_preserve_anchor(self):
        p = self.start(profile=True, limits=SMALL)
        c = Peer(p)
        self.assertEqual(p.control("close", connection=c.id, code=1000)["disposition"], "started")
        self.assertEqual(c.receive(), (8, close_body(1000)))
        deadline = p.inspect(c)["close_deadline"]
        self.assertEqual(deadline, 1010000000)
        p.set_time(1009000000)
        ping = frame(9, b"x")
        c.send(ping)
        self.assertEqual(c.receive(), (10, b"x"))
        s = p.progress(c, consumed=len(ping))
        self.assertEqual(s["close_deadline"], deadline)
        repeated = p.control("close", connection=c.id, code=1001, reason="different")
        self.assertEqual(repeated["disposition"], "already_closing")
        refused = p.control("send", connection=c.id, length=1, octet=65)
        self.assertEqual((refused["status"], refused["reason"]), ("unavailable", "closed"))
        s = p.inspect(c)
        self.assertFalse(s["transport_closed"], "output refusal is not TCP teardown")
        self.assertEqual(s["close_deadline"], deadline)
        c.send(frame(9, b"y"))
        # An extra Close/body from the repeated call would precede this Pong.
        self.assertEqual(c.receive(), (10, b"y"))
        self.assertEqual(p.progress(c, consumed=2 * len(ping))["close_deadline"], deadline)
        p.assert_no_data()
        p.set_time(deadline)
        p.control("recheck", connection=c.id)
        self.closed(p, c)

    def test_late_text_after_local_close_has_no_delivery_and_bounded_teardown(self):
        p = self.start(profile=True, limits=SMALL)
        c = Peer(p)
        p.control("close", connection=c.id, code=1000)
        self.assertEqual(c.receive(), (8, close_body(1000)))
        c.send(frame(1, b"must not enter") + frame(9, b"marker"))
        try:
            # Pong positively proves progress past ignored late data. Immediate native
            # teardown is also allowed; no unpromised open/drop policy is imposed.
            self.assertEqual(c.receive(), (10, b"marker"))
        except (EOFError, ConnectionResetError):
            c.close_socket()
            p.wait("closed", connection=c.id)
        else:
            p.set_time(1010000000)
            p.control("recheck", connection=c.id)
            self.closed(p, c)
        p.assert_no_data()

    def test_simultaneous_close_and_peer_observation_remain_distinct(self):
        p = self.start(profile=True)
        c = Peer(p)
        p.control("close", connection=c.id, code=1000)
        c.send(frame(8, close_body(1001, b"bye")))
        self.assertEqual(c.receive(), (8, close_body(1000)))
        e = self.closed(p, c)
        self.assertEqual(e["inspection"]["snapshot"]["first_peer_close"],
                         {"kind": "close_with_status", "code": 1001, "reason_hex": "627965"})

    def test_ws21_first_empty_close_is_immutable_while_actual_output_pending(self):
        p = self.start(profile=True, small_send_buffer=True)
        c = Peer(p, tiny_receive=True)
        self.assertEqual(p.control("send", connection=c.id, length=MAX_PAYLOAD,
                                   octet=65, binary=True)["status"], "ok")
        s = p.inspect(c)
        self.assertGreater(s["current"]["application_output"], 0,
                           "not-reading alone does not establish native pending output")
        c.send(bytes.fromhex("88 80 01 02 03 04"))
        s = p.progress(c, consumed=6)
        self.assertEqual(s["first_peer_close"], {"kind": "close_without_status"})
        deadline = s["close_deadline"]
        self.assertEqual(deadline, 2000000000)
        self.assertFalse(s["transport_closed"])
        self.assertGreater(s["current"]["application_output"], 0)
        p.set_time(1001000000)
        c.send(bytes.fromhex("88 82 05 06 07 08 06 ee") + frame(9, b"x") + frame(1, b"late"))
        active = p.inspect(c)
        self.assertEqual(active["first_peer_close"], {"kind": "close_without_status"})
        if not active["transport_closed"] or active["close_deadline"] is not None:
            self.assertEqual(active["close_deadline"], deadline)
        opcode, body = c.receive_message()
        self.assertEqual((opcode, len(body), hashlib.sha256(body).hexdigest()),
                         (2, MAX_PAYLOAD, hashlib.sha256(b"A" * MAX_PAYLOAD).hexdigest()))
        self.assertEqual(c.receive(), (8, b""))
        closed = self.closed(p, c)
        terminal = closed["inspection"]["snapshot"]
        self.assertEqual(terminal["first_peer_close"], {"kind": "close_without_status"})
        if terminal["close_deadline"] is not None:
            self.assertEqual(terminal["close_deadline"], deadline)
        p.assert_no_data()

    def test_frame_message_and_io_turn_bounds_use_real_snapshots(self):
        p = self.start(profile=True, limits=SMALL)
        c = Peer(p)
        body = b"a" * 64
        c.send(frame(2, body))
        self.message(p, c, "binary", body)
        burst = frame(1) * 32  #192 wire bytes exceeds the actual128-byte raw-capacity limit.
        c.send(burst)
        for _ in range(32):
            self.message(p, c, "text", b"")
        s = p.progress(c, consumed=len(frame(2, body)) + len(burst))
        for group in (s["current"], s["peak"]):
            self.assertLessEqual(group["raw_capacity"], 128)
            self.assertLessEqual(group["assembly_capacity"], 128)
            self.assertLessEqual(group["application_output"], 128)
            self.assertLessEqual(group["control_output"], 254)
            self.assertLessEqual(group["callback_payload_connection"], 128)
            self.assertLessEqual(group["callback_payload_global"], 256)
        self.assertLessEqual(s["frames_in_last_turn"], 2)
        self.assertIsInstance(s["continuation_pending"], bool)
        c.send(frame(2, b"b" * 65))
        self.assertEqual(c.receive(), (8, close_body(1009)))
        self.closed(p, c)

    def test_empty_fragment_count_is_bounded(self):
        p = self.start(profile=True, limits=SMALL)
        c = Peer(p)
        c.send(frame(1, fin=False) + frame(0, fin=False) * 2 + frame(0))
        self.message(p, c, "text", b"")  # Exactly4 data frames remains eligible.
        c.send(frame(1, fin=False) + frame(0, fin=False) * 3 + frame(0))
        self.assertEqual(c.receive(), (8, close_body(1009)))
        self.closed(p, c)
        p.assert_no_data()

    def test_b04_current_native_delivery_claims_once(self):
        p = self.start(profile=True, limits=SMALL, hold=True, claim_twice=True)
        c = Peer(p)
        c.send(frame(1, b"A"))
        m = self.message(p, c, "text", b"A")
        self.assertEqual(m["second_claim"], {"status": "unavailable", "reason": "delivery_claimed"})
        self.assertEqual(p.inspect(c)["callback_leases_connection"], 1)
        r = p.control("claim", connection=c.id)
        self.assertEqual((r["status"], r["reason"]), ("unavailable", "outside_delivery"))
        self.assertEqual(p.inspect(c)["callback_leases_connection"], 1)

    def test_b05_other_connection_cannot_claim_current_native_payload(self):
        p = self.start(profile=True, limits=SMALL, claim_other=2)
        a, b = Peer(p), Peer(p)
        a.send(frame(1, b"A"))
        r = p.wait("claim_other", connection=a.id)["result"]
        self.assertEqual(r, {"status": "unavailable", "reason": "delivery_mismatch"})
        self.message(p, a, "text", b"A")
        self.assertEqual(p.inspect(b)["callback_leases_connection"], 0)

    def test_b07_safe_null_foreign_wrong_owner_checks_do_not_sample(self):
        p = self.start(profile=True, limits=SMALL)
        c = Peer(p)
        for fields, reason in (({"target": "null"}, "null_connection"),
                               ({"target": "foreign"}, "not_owned"),
                               ({"connection": c.id, "wrong_owner": True}, "wrong_owner")):
            with self.subTest(reason=reason):
                r = p.control("inspect", **fields)
                self.assertEqual((r["status"], r["reason"]), ("unavailable", reason))
                self.assertEqual(r["clock_calls_before"], r["clock_calls_after"])
                self.assertEqual(r["foreign_virtual_calls"], 0)

    def test_move_copy_release_and_zero_length_slot_limits(self):
        p = self.start(profile=True, limits=SMALL, hold=True)
        c = Peer(p)
        c.send(frame(1))
        first = self.message(p, c, "text", b"")["lease"]
        r = p.control("lease_move", connection=c.id, lease=first)
        self.assertFalse(r["moved_from_present"])
        self.assertEqual(r["moved_from_reserved"], 0)
        moved = r["lease"]
        self.assertEqual(p.inspect(c)["callback_leases_connection"], 1)
        consumer = p.control("lease_consumer", connection=c.id, lease=moved)
        self.assertTrue(consumer["off_owner"])
        self.assertTrue(consumer["original_present"])
        self.assertEqual(consumer["copy"], {"status": "ok"})
        self.assertTrue(consumer["copy_reset_absent"])
        self.assertEqual(consumer["copy_reset_reserved"], 0)
        self.assertTrue(consumer["original_reset_absent"])
        self.assertEqual(consumer["original_reset_reserved"], 0)
        self.assertEqual(p.inspect(c)["callback_leases_connection"], 0)
        c.send(frame(1))
        moved = self.message(p, c, "text", b"")["lease"]
        r = p.control("lease_copy", connection=c.id, lease=moved)
        self.assertEqual(r["status"], "ok")
        copy = r["lease"]
        r = p.control("lease_copy", connection=c.id, lease=moved)
        self.assertEqual((r["status"], r["reason"]), ("unavailable", "resource_limit"))
        self.assertEqual(p.inspect(c)["callback_leases_connection"], 2)
        p.control("lease_release", connection=c.id, lease=copy)
        p.control("lease_release", connection=c.id, lease=moved)
        self.assertEqual(p.inspect(c)["callback_leases_connection"], 0)
        c.send(frame(1, b"resumed"))
        self.message(p, c, "text", b"resumed")
        c.send(frame(1))
        self.message(p, c, "text", b"")
        c.send(frame(1))  # Third retained callback is forbidden even with zero payload bytes.
        self.assertEqual(c.receive(), (8, close_body(1009)))
        self.closed(p, c)
        p.assert_no_data()

    def test_b01_b02_empty_retained_peer_does_not_release_owner_slot(self):
        p = self.start(profile=True, limits=SMALL, hold=True)
        a, b = Peer(p), Peer(p)
        a.send(frame(1))
        lease = self.message(p, a, "text", b"")["lease"]
        a.send(frame(8))
        self.assertEqual(a.receive(), (8, b""))
        self.closed(p, a)
        s = p.inspect(a)
        self.assertTrue(s["transport_closed"])
        self.assertEqual(s["callback_leases_connection"], 1)
        self.assertEqual(p.global_snapshot()["owners_live_or_retiring"], 2)
        refused = Peer(p, upgrade=False)
        refused.send(b"GET /native-profile HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                     b"Connection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                     b"Sec-WebSocket-Version: 13\r\n\r\n")
        refused.refused_upgrade()
        self.assertLessEqual(p.global_snapshot()["owners_peak"], 2)
        r = p.control("claim", connection=a.id)
        self.assertEqual((r["status"], r["reason"]), ("unavailable", "closed"))
        v = p.control("lease_view", connection=a.id, lease=lease)
        self.assertTrue(v["present"])
        self.assertEqual(v["payload"]["length"], 0)
        r = p.control("lease_copy", connection=a.id, lease=lease)
        self.assertEqual((r["status"], r["reason"]), ("unavailable", "closed"))
        p.control("lease_release", connection=a.id, lease=lease)
        self.assertEqual(p.retire(1)["callback_leases"], 0)  # B02's second peer remains live.
        r = p.control("inspect", connection=a.id)
        self.assertEqual((r["status"], r["reason"]), ("unavailable", "not_owned"))
        b.send(frame(8))
        self.assertEqual(b.receive(), (8, b""))
        self.closed(p, b)
        self.assertEqual(p.retire(0)["callback_payload_bytes"], 0)

    def test_global_callback_capacity_is_actual_and_releases(self):
        limits = dict(SMALL, frame_bytes=65, message_bytes=65, assembly_capacity_bytes=128,
                      callback_bytes_global=128)
        p = self.start(profile=True, limits=limits, hold=True)
        a, b = Peer(p), Peer(p)
        a.send(frame(2, b"A" * 65))
        m = self.message(p, a, "binary", b"A" * 65)
        self.assertGreaterEqual(m["reserved_payload_bytes"], 65)
        self.assertLessEqual(m["reserved_payload_bytes"], 128)
        b.send(frame(2, b"B" * 65))
        self.assertEqual(b.receive(), (8, close_body(1009)))
        self.closed(p, b)
        g = p.global_snapshot()
        self.assertEqual(g["callback_leases"], 1)
        self.assertLessEqual(g["callback_payload_bytes"], 128)
        p.control("lease_release", connection=a.id, lease=m["lease"])
        self.assertEqual(p.global_snapshot()["callback_payload_bytes"], 0)
        a.send(frame(2, b"C" * 65))
        self.message(p, a, "binary", b"C" * 65)

    def test_b03_copy_refuses_before_more_than_available_payload(self):
        # Any actual capacity for65 bytes is at least65: a second copy cannot fit128.
        limits = dict(SMALL, frame_bytes=65, message_bytes=65, assembly_capacity_bytes=128)
        p = self.start(profile=True, limits=limits, hold=True)
        c = Peer(p)
        c.send(frame(2, b"A" * 65))
        m = self.message(p, c, "binary", b"A" * 65)
        before = p.inspect(c)
        r = p.control("lease_copy", connection=c.id, lease=m["lease"])
        self.assertEqual((r["status"], r["reason"]), ("unavailable", "resource_limit"))
        after = p.inspect(c)
        self.assertEqual(after["callback_leases_connection"], 1)
        self.assertEqual(after["current"]["callback_payload_connection"],
                         before["current"]["callback_payload_connection"])
        # The no-preallocation and unknown-bound refusal proof also requires private source review.

    def test_local_output_budget_leaves_control_reserve_under_ping_pressure(self):
        p = self.start(profile=True, small_send_buffer=True)
        c = Peer(p, tiny_receive=True)
        first = p.control("send", connection=c.id, length=MAX_PAYLOAD, octet=65, binary=True)
        self.assertEqual(first["status"], "ok")
        before = p.inspect(c)
        self.assertGreater(before["current"]["application_output"], 0)
        second = p.control("send", connection=c.id, length=MAX_PAYLOAD, octet=66, binary=True)
        self.assertEqual((second["status"], second["reason"]), ("unavailable", "resource_limit"))
        pings = b"".join(frame(9, bytes((n,))) for n in range(32))
        c.send(pings)
        pressure = p.progress(c, consumed=len(pings))
        self.assertGreater(pressure["current"]["application_output"], 0)
        self.assertGreater(pressure["peak"]["control_output"], 0)
        self.assertLessEqual(pressure["peak"]["application_output"], 1048590)
        self.assertLessEqual(pressure["peak"]["control_output"], 254)
        self.assertEqual(p.control("close", connection=c.id)["status"], "ok")
        data = bytearray()
        data_started = data_done = close_seen = False
        pongs = []
        for _ in range(4096 + 32 + 1):
            opcode, body = c.receive()
            if opcode == 10:
                self.assertEqual(len(body), 1)
                self.assertIn(body[0], range(32))
                if pongs:
                    self.assertGreater(body[0], pongs[-1])
                pongs.append(body[0])
            elif opcode in (0, 2):
                self.assertFalse(close_seen)
                self.assertEqual(opcode, 0 if data_started else 2)
                self.assertFalse(data_done)
                data_started = True
                self.assertLessEqual(len(data) + len(body), MAX_PAYLOAD)
                data.extend(body)
                data_done = c.last_fin
            elif opcode == 8:
                self.assertFalse(close_seen)
                self.assertTrue(data_done)
                self.assertEqual(body, b"")
                close_seen = True
            else:
                self.fail(f"unexpected output/control opcode {opcode}")
            if data_done and close_seen and pongs and pongs[-1] == 31:
                break
        else:
            self.fail("bounded admitted data/Pong/Close drain did not finish")
        self.assertEqual(len(data), MAX_PAYLOAD)
        self.assertEqual(hashlib.sha256(data).hexdigest(), hashlib.sha256(b"A" * MAX_PAYLOAD).hexdigest())
        # Coalescing is permitted; legal Pongs may interleave data fragments or follow local Close.
        # Source/resource review additionally proves at most one retained pending Pong.
        c.send(frame(8))
        self.closed(p, c)

    def test_local_close_validation_never_transmits_forbidden_code(self):
        p = self.start(profile=True)
        c = Peer(p)
        for code in (1005, 1006, 1010, 1015, 1016, 5000):
            with self.subTest(code=code):
                r = p.control("close", connection=c.id, code=code)
                self.assertEqual((r["status"], r["reason"]), ("unavailable", "invalid_input"))
                self.assertFalse(p.inspect(c)["close_sent"])
        c.send(frame(1, b"still admitted"))
        self.message(p, c, "text", b"still admitted")

    def test_partial_frame_equality_entry_guard_and_snapshot_is_pure(self):
        p = self.start(profile=True, limits=SMALL)
        c = Peer(p)
        c.send(b"\x81")
        s = p.progress(c, read=1)
        self.assertEqual(s["frame_deadline"], 1010000000)
        p.set_time(1009999999)
        r = p.control("inspect", connection=c.id)
        self.assertEqual(r["clock_calls_before"], r["clock_calls_after"])
        self.assertFalse(r["snapshot"]["transport_closed"])
        p.set_time(1010000000)
        # Calls the actual native entry guard; source review must verify every quota continuation
        # invokes this same guard. inspect itself never polls/repairs/extends it.
        p.control("recheck", connection=c.id)
        self.closed(p, c)

    def test_ws22_actual_quota_continuation_expires_before_further_parse(self):
        p = self.start(profile=True, limits=dict(SMALL, frames_per_turn=1), quota_observer=True)
        c = Peer(p)
        # One write is only input. Genuine native read/consumed/backlog/quota/anchor state
        # in the queued observer must establish the barrier BEFORE any fake-time advance.
        c.send(frame(9) + b"\x81")
        observed = p.wait("quota_observed", connection=c.id)
        self.assertEqual(observed["status"], "observed_and_advanced",
                         "SETUP/BARRIER: not native feature red; no time advance without proof")
        s = observed["snapshot"]
        self.assertTrue(s["continuation_pending"])
        self.assertEqual(s["frames_in_last_turn"], 1)
        self.assertEqual(s["frames_consumed"], 1)
        self.assertGreaterEqual(s["ws_bytes_read"], 7)
        self.assertEqual(s["ws_bytes_consumed"], 6)
        self.assertEqual(s["frame_deadline"], 1010000000)
        self.assertEqual(observed["advanced_to"], 1010000000)
        # Account for the already-admitted empty Pong; it is not late application data.
        try:
            self.assertEqual(c.receive(), (10, b""))
        except (EOFError, ConnectionResetError):
            pass  # Force-close may beat local control handoff; never infer peer receipt.
        c.eof()
        c.close_socket()
        closed = p.wait("closed", connection=c.id)
        self.assertEqual(closed["inspection"]["status"], "ok")
        terminal = closed["inspection"]["snapshot"]
        self.assertTrue(terminal["transport_closed"])
        self.assertEqual(terminal["frames_consumed"], 1)
        self.assertEqual(terminal["ws_bytes_consumed"], 6)
        if terminal["frame_deadline"] is not None:
            self.assertEqual(terminal["frame_deadline"], 1010000000)
        p.assert_no_data()
        # Native FIFO/timer/source review is a separate causality gate: no fixture recheck
        # or close is invoked, and timer-only closure cannot substitute for this continuation.

    def test_fragment_message_deadline_not_reset_by_control_or_trickle(self):
        p = self.start(profile=True, limits=SMALL)
        c = Peer(p)
        first = frame(1, b"A", fin=False)
        c.send(first)
        s = p.progress(c, consumed=len(first))
        self.assertEqual(s["message_deadline"], 1030000000)
        p.set_time(1020000000)
        ping = frame(9, b"x")
        c.send(ping)
        self.assertEqual(c.receive(), (10, b"x"))
        self.assertEqual(p.progress(c, consumed=len(first + ping))["message_deadline"], 1030000000)
        p.set_time(1030000000)
        p.control("recheck", connection=c.id)
        self.closed(p, c)
        p.assert_no_data()

    def test_unavailable_regressing_and_overflowing_elapsed_refuse(self):
        for condition in ("unavailable", "regression", "overflow"):
            with self.subTest(condition=condition):
                p = self.start(profile=True, limits=SMALL, now=1000000000)
                c = Peer(p)
                if condition == "overflow":
                    p.set_time(18446744073709551610)
                    c.send(b"\x81")
                else:
                    c.send(b"\x81")
                    p.progress(c, read=1)
                    p.set_time(0 if condition == "regression" else 1000000000,
                               unavailable=condition == "unavailable")
                    p.control("recheck", connection=c.id)
                self.closed(p, c)
                p.assert_no_data()

    def test_actual_monotonic_uncooperative_peer_teardown(self):
        p = self.start(profile=True, clock="native", limits=dict(SMALL, close_ms=100))
        c = Peer(p)
        self.assertEqual(p.control("close", connection=c.id, code=1000)["status"], "ok")
        self.assertEqual(c.receive(), (8, close_body(1000)))
        # Never answer Close. Require actual server teardown BEFORE fixture cleanup/kill.
        c.eof()
        p.wait("closed", connection=c.id)

    def test_incomplete_upgrade_quota_and_actual_timeout_release(self):
        p = self.start(profile=True, clock="native", limits=dict(SMALL, upgrade_ms=300))
        a, b = Peer(p, upgrade=False), Peer(p, upgrade=False)
        partial = b"GET /native-profile HTTP/1.1\r\nUpgrade: websocket\r\n"
        a.send(partial)
        b.send(partial)
        for _ in range(64):
            if p.global_snapshot()["upgrade_owners"] == 2:
                break
        else:
            self.fail("actual incomplete-upgrade ownership was not observed")
        c = Peer(p, upgrade=False)
        c.send(partial)
        c.refused_upgrade()
        self.assertLessEqual(p.global_snapshot()["upgrade_owners_peak"], 2)
        a.refused_upgrade()
        b.refused_upgrade()
        for _ in range(64):
            if p.global_snapshot()["upgrade_owners"] == 0:
                break
        else:
            self.fail("actual incomplete-upgrade retirement not observed")

    def test_factory_copies_limits_and_owns_readonly_source(self):
        p = self.start(profile=True, limits=SMALL, mutate_limits=True)
        c = Peer(p)
        dropped = p.control("drop_clock", connection=c.id)
        self.assertEqual(dropped["source_destroyed"], 0)
        c.send(frame(1, b"A"))
        self.message(p, c, "text", b"A")
        observed = p.control("inspect", connection=c.id)
        self.assertEqual(observed["status"], "ok")
        self.assertGreater(observed["clock_calls_after"], dropped["clock_calls_after"])
        self.assertEqual(observed["source_destroyed"], 0)

    def test_factory_configuration_failures_are_exact(self):
        cases = [({"frame_bytes": 0}, "zero_limit"),
                 ({"frame_bytes": 65}, "inconsistent_limits"),
                 ({"raw_capacity_bytes": 77}, "inconsistent_limits"),
                 ({"control_output_bytes": 253}, "inconsistent_limits"),
                 ({"callback_bytes_global": 127}, "inconsistent_limits"),
                 ({"close_ms": 18446744073709551615}, "arithmetic_overflow"),
                 ({"connections": 18446744073709551615}, "arithmetic_overflow")]
        for changes, reason in cases:
            with self.subTest(changes=changes):
                p = Probe(self, factory=True, limits=dict(SMALL, **changes))
                self.assertEqual((p.factory["status"], p.factory.get("reason")),
                                 ("invalid_configuration", reason),
                                 "MISSING_PROFILE_BEHAVIOR; stock Unimplemented is not native red")
        p = Probe(self, factory=True, null_source=True, limits=SMALL)
        self.assertEqual((p.factory["status"], p.factory.get("reason")),
                         ("invalid_configuration", "missing_elapsed_source"))

    def test_b06_factory_existing_install_and_native_start_have_distinct_reasons(self):
        p = Probe(self, factory=True, factory_action="repeat", limits=SMALL)
        self.assertEqual(p.factory["status"], "created", "MISSING_PROFILE_BEHAVIOR")
        self.assertEqual(p.wait("factory", repeat=True),
                         {"event": "factory", "repeat": True, "status": "invalid_configuration",
                          "reason": "already_installed", "clock_calls": p.factory["clock_calls"]})
        both = Probe(self, factory=True, factory_action="repeat_after_start", limits=SMALL)
        self.assertEqual(both.factory["status"], "created", "MISSING_PROFILE_BEHAVIOR")
        self.assertEqual(both.wait("factory", repeat=True).get("reason"), "already_installed")
        p = Probe(self, factory=True, factory_action="after_start", limits=SMALL)
        result = p.wait("factory", repeat=False)
        self.assertEqual((result["status"], result.get("reason")),
                         ("invalid_configuration", "native_started"), "MISSING_PROFILE_BEHAVIOR")


def test_ids():
    # Passive registration inventory only: no child, fixture body or socket.
    return [f"{NativeProfile.__name__}.{name}" for name in
            unittest.defaultTestLoader.getTestCaseNames(NativeProfile)]


def main():
    global PROBE
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path)
    parser.add_argument("--list", action="store_true")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()
    if args.list:
        print("\n".join(test_ids()))
        return 0
    if args.probe is None:
        parser.error("--probe is required for native execution")
    PROBE = args.probe.resolve(strict=True)
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(NativeProfile)
    result = unittest.TextTestRunner(verbosity=2 if args.verbose else 1).run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())
