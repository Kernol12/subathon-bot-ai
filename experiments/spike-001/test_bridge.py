# SPDX-License-Identifier: GPL-3.0-or-later
"""External loopback experiments; independent review/red evidence is recorded separately."""
import argparse
import base64
from concurrent.futures import ThreadPoolExecutor
import hashlib
import http.client
import json
import os
from pathlib import Path
import queue
import resource
import signal
import socket
import sqlite3
import struct
import subprocess
import sys
import tempfile
import threading
import time
import unittest

PROBE = None
DEADLINE = 10


class Probe:
    """Own a disposable process/database and wait for explicit JSON barriers."""

    def __init__(self, hold=False, port=0):
        self.directory = tempfile.TemporaryDirectory(prefix="spike-001-")
        self.database = Path(self.directory.name) / "commands.sqlite"
        command = [PROBE, "--port", str(port), "--database", str(self.database)]
        if hold:
            command.append("--hold")
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.events = queue.Queue()
        self.pending = []
        self.output = []
        self.reader = threading.Thread(target=self._read_output, daemon=True)
        self.reader.start()
        self.port = None
        self.finished = False

    def _read_output(self):
        for line in self.process.stdout:
            self.output.append(line.rstrip())
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue
            if isinstance(event, dict):
                self.events.put(event)
        self.events.put(None)

    def wait_event(self, name, **fields):
        def matches(event):
            return event.get("event") == name and all(event.get(k) == v for k, v in fields.items())

        for index, event in enumerate(self.pending):
            if matches(event):
                return self.pending.pop(index)
        limit = time.monotonic() + DEADLINE
        while True:
            remaining = limit - time.monotonic()
            if remaining <= 0:
                raise AssertionError(f"Missing {name} {fields}; output: {self.output[-15:]}")
            try:
                event = self.events.get(timeout=remaining)
            except queue.Empty as error:
                raise AssertionError(f"Missing {name} {fields}; output: {self.output[-15:]}") from error
            if event is None:
                raise AssertionError(f"Exited before {name} {fields}; output: {self.output[-15:]}")
            if matches(event):
                return event
            self.pending.append(event)

    def ready(self):
        event = self.wait_event("ready")
        self.port = event["port"]
        if not isinstance(self.port, int) or not 0 < self.port < 65536:
            raise AssertionError(f"Invalid bound port: {event}")
        return event

    def control(self, command):
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def finish(self, send_stop=True):
        if send_stop:
            self.control("stop")
        stopped = self.wait_event("stopped")
        result = self.process.wait(timeout=DEADLINE)
        self.reader.join(timeout=DEADLINE)
        if result != 0:
            raise AssertionError(f"Probe exit {result}; output: {self.output[-15:]}")
        self.finished = True
        return stopped

    def cleanup(self):
        # Cleanup preserves a primary test failure and does not pretend a killed process passed.
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait(timeout=DEADLINE)
        self.reader.join(timeout=DEADLINE)
        self.process.stdin.close()
        self.process.stdout.close()
        self.directory.cleanup()


def http_command(port, command_id):
    connection = http.client.HTTPConnection("127.0.0.1", port, timeout=DEADLINE)
    try:
        connection.request("POST", "/command", json.dumps({"id": command_id}),
                           {"Content-Type": "application/json", "Connection": "close"})
        response = connection.getresponse()
        return response.status, json.loads(response.read())
    finally:
        connection.close()


class WebSocket:
    """Small RFC6455 client for bounded text/close frames in this loopback probe."""

    def __init__(self, port):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=DEADLINE)
        self.buffer = bytearray()
        key = base64.b64encode(os.urandom(16)).decode("ascii")
        request = (f"GET /ws HTTP/1.1\r\nHost: 127.0.0.1:{port}\r\n"
                   f"Upgrade: websocket\r\nConnection: Upgrade\r\n"
                   f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n")
        try:
            self.socket.sendall(request.encode("ascii"))
            while b"\r\n\r\n" not in self.buffer:
                block = self.socket.recv(4096)
                if not block:
                    raise AssertionError("WebSocket upgrade closed before HTTP headers")
                self.buffer.extend(block)
                if len(self.buffer) > 16384:
                    raise AssertionError("Oversized WebSocket upgrade headers")
            header, rest = self.buffer.split(b"\r\n\r\n", 1)
            self.buffer = bytearray(rest)
            lines = header.decode("ascii").split("\r\n")
            if lines[0].split()[1] != "101":
                raise AssertionError(f"WebSocket upgrade failed: {lines[0]}")
            headers = {name.lower(): value.strip() for name, value in
                       (line.split(":", 1) for line in lines[1:])}
            accept = base64.b64encode(hashlib.sha1(
                (key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode("ascii")).digest()).decode("ascii")
            if headers.get("sec-websocket-accept", "") != accept:
                raise AssertionError("WebSocket upgrade accept mismatch")
        except BaseException:
            self.socket.close()
            raise

    def _exact(self, length):
        while len(self.buffer) < length:
            block = self.socket.recv(max(4096, length - len(self.buffer)))
            if not block:
                raise AssertionError("WebSocket closed before complete frame")
            self.buffer.extend(block)
        data = bytes(self.buffer[:length])
        del self.buffer[:length]
        return data

    def send_frame(self, opcode, payload):
        if len(payload) >= 126:
            raise AssertionError("Probe client payload exceeds its bounded short-frame contract")
        mask = os.urandom(4)
        masked = bytes(value ^ mask[index % 4] for index, value in enumerate(payload))
        self.socket.sendall(bytes([0x80 | opcode, 0x80 | len(payload)]) + mask + masked)

    def send(self, command_id):
        self.send_frame(1, json.dumps({"id": command_id}).encode("utf-8"))

    def receive_frame(self):
        first, second = self._exact(2)
        if not first & 0x80 or second & 0x80:
            raise AssertionError("Expected complete, unmasked server frame")
        length = second & 0x7f
        if length == 126:
            length = struct.unpack("!H", self._exact(2))[0]
        elif length == 127:
            length = struct.unpack("!Q", self._exact(8))[0]
        if length > 65536:
            raise AssertionError("Oversized probe WebSocket frame")
        return first & 0xf, self._exact(length)

    def receive(self):
        while True:
            opcode, payload = self.receive_frame()
            if opcode == 9:
                self.send_frame(10, payload)
                continue
            if opcode != 1:
                raise AssertionError(f"Expected text acknowledgement, received opcode {opcode}")
            return json.loads(payload)

    def close(self):
        try:
            self.send_frame(8, struct.pack("!H", 1000))
            while True:
                opcode, payload = self.receive_frame()
                if opcode == 8:
                    break
                if opcode == 9:
                    self.send_frame(10, payload)
                else:
                    raise AssertionError(f"Unexpected frame during close: {opcode}")
        finally:
            self.socket.close()

    def abort(self):
        self.socket.close()


class BridgeExperiments(unittest.TestCase):
    def start(self, hold=False):
        probe = Probe(hold=hold)
        self.addCleanup(probe.cleanup)
        ready = probe.ready()
        self.assertEqual(ready["journal_mode"], "wal")
        self.assertEqual(ready["synchronous"], 2)
        self.assertTrue(ready["qt_version"])
        self.assertTrue(ready["drogon_version"])
        return probe

    def assert_ack(self, reply, command_id):
        self.assertEqual(reply["id"], command_id, reply)
        for field in ("qt_owner", "sql_owner", "request_off_owner"):
            self.assertIs(reply[field], True, reply)

    def assert_rows(self, probe, expected_ids, expected_count, expected_total):
        with sqlite3.connect(probe.database.as_uri() + "?mode=ro", uri=True) as connection:
            rows = connection.execute("SELECT id FROM commands ORDER BY id").fetchall()
            count, total = connection.execute("SELECT COUNT(*), COALESCE(SUM(id),0) FROM commands").fetchone()
        self.assertEqual([row[0] for row in rows], expected_ids)
        self.assertEqual(count, expected_count)
        self.assertEqual(total, expected_total)

    def assert_stopped(self, probe, expected_ids, expected_count, expected_total, send_stop=True):
        event = probe.finish(send_stop=send_stop)
        self.assertIs(event["writer_closed"], True)
        self.assertIs(event["drogon_joined"], True)
        self.assertEqual(event["owner_violations"], 0)
        self.assert_rows(probe, expected_ids, expected_count, expected_total)

    def test_mixed_http_websocket_commands_commit_on_qt_owner(self):
        probe = self.start()
        barrier = threading.Barrier(8)

        def command(command_id):
            barrier.wait(timeout=DEADLINE)
            if command_id < 4:
                status, reply = http_command(probe.port, command_id)
                self.assertEqual(status, 200, reply)
            else:
                client = WebSocket(probe.port)
                try:
                    client.send(command_id)
                    reply = client.receive()
                    client.close()
                finally:
                    client.abort()
            self.assert_ack(reply, command_id)

        with ThreadPoolExecutor(max_workers=8) as pool:
            list(pool.map(command, range(8)))
        self.assert_stopped(probe, [0, 1, 2, 3, 4, 5, 6, 7], 8, 28)

    def test_disconnected_admitted_http_command_commits_once(self):
        probe = self.start(hold=True)
        client = socket.create_connection(("127.0.0.1", probe.port), timeout=DEADLINE)
        self.addCleanup(client.close)
        body = b'{"id":41}'
        request = (b"POST /command HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\n"
                   b"Connection: close\r\nContent-Length: " + str(len(body)).encode("ascii") + b"\r\n\r\n" + body)
        client.sendall(request)
        probe.wait_event("admitted", id=41, transport="http")
        self.assert_rows(probe, [], 0, 0)
        client.shutdown(socket.SHUT_RDWR)
        client.close()
        probe.control("release")
        probe.wait_event("released")
        self.assert_stopped(probe, [41], 1, 41)

    def test_closed_admission_rejects_late_transports_and_preserves_queued_work(self):
        probe = self.start(hold=True)
        with ThreadPoolExecutor(max_workers=1) as pool:
            pending = pool.submit(http_command, probe.port, 7)
            probe.wait_event("admitted", id=7, transport="http")
            self.assert_rows(probe, [], 0, 0)
            probe.control("close")
            probe.wait_event("admission_closed")
            status, reply = http_command(probe.port, 8)
            self.assertEqual(status, 503, reply)
            self.assertEqual(reply, {"error": "admission_closed"})
            client = WebSocket(probe.port)
            try:
                client.send(9)
                self.assertEqual(client.receive(), {"error": "admission_closed"})
                client.close()
            finally:
                client.abort()
            probe.control("release")
            probe.wait_event("released")
            status, reply = pending.result(timeout=DEADLINE)
            self.assertEqual(status, 200, reply)
            self.assert_ack(reply, 7)
        self.assert_stopped(probe, [7], 1, 7)

    def test_stop_drains_admitted_http_and_websocket_replies(self):
        probe = self.start(hold=True)
        client = WebSocket(probe.port)
        self.addCleanup(client.abort)
        with ThreadPoolExecutor(max_workers=1) as pool:
            pending = pool.submit(http_command, probe.port, 11)
            client.send(12)
            probe.wait_event("admitted", id=11, transport="http")
            probe.wait_event("admitted", id=12, transport="websocket")
            self.assert_rows(probe, [], 0, 0)
            probe.control("stop")
            probe.wait_event("stopping")
            status, reply = pending.result(timeout=DEADLINE)
            self.assertEqual(status, 200, reply)
            self.assert_ack(reply, 11)
            self.assert_ack(client.receive(), 12)
            client.close()
        self.assert_stopped(probe, [11, 12], 2, 23, send_stop=False)

    def test_occupied_listener_port_fails_without_false_readiness(self):
        with socket.socket() as occupied:
            occupied.bind(("127.0.0.1", 0))
            occupied.listen()
            probe = Probe(port=occupied.getsockname()[1])
            self.addCleanup(probe.cleanup)
            result = probe.process.wait(timeout=DEADLINE)
            probe.reader.join(timeout=DEADLINE)
            self.assertNotEqual(result, 0, probe.output)
            ready = [event for event in probe.pending if event.get("event") == "ready"]
            while not probe.events.empty():
                event = probe.events.get_nowait()
                if event and event.get("event") == "ready":
                    ready.append(event)
            self.assertEqual(ready, [], probe.output)

    def test_sigterm_drains_admitted_command_and_exits_cleanly(self):
        probe = self.start(hold=True)
        with ThreadPoolExecutor(max_workers=1) as pool:
            pending = pool.submit(http_command, probe.port, 21)
            probe.wait_event("admitted", id=21, transport="http")
            self.assert_rows(probe, [], 0, 0)
            probe.process.send_signal(signal.SIGTERM)
            probe.wait_event("stopping")
            status, reply = pending.result(timeout=DEADLINE)
            self.assertEqual(status, 200, reply)
            self.assert_ack(reply, 21)
        self.assert_stopped(probe, [21], 1, 21, send_stop=False)


if __name__ == "__main__":
    # The occupied-port case deliberately exercises upstream fatal startup failure.
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, help="Path to the real-framework probe executable")
    arguments, unittest_arguments = parser.parse_known_args()
    PROBE = str(Path(arguments.probe).resolve())
    unittest.main(argv=[sys.argv[0], *unittest_arguments])
