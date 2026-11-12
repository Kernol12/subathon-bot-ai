# SPDX-License-Identifier: GPL-3.0-or-later
"""APITEST-016 authored literals. Execution requires a NEW reviewed consumer admission.

No import/discovery/identity/body execution is authorized by this source packet.
NI is an ordinary failing availability assertion, never a pass or expected failure.
"""
from __future__ import annotations

import argparse
import fcntl
import struct
import stat
import sys
import hashlib
import json
import os
from pathlib import Path
import select
import shutil
import socket
import subprocess
import tempfile
import time
import unittest

PROBE: Path | None = None
ADMISSION: dict | None = None
CHILD_SECONDS = 30
STEP_SECONDS = 7
REAP_SECONDS = 2
MAX_EVENTS = 2048
MAX_LINE = 65536
MAX_LOG = 2097152
MAX_PEERS = 128
MAX_COMMANDS = 256
MAX_WIRE = 1048576 + 40961 + 128
MAX_TURNS = 8192

F01 = b"POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ndata"
DIAG = b"HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: keep-alive\r\n\r\ndiag"
DIAG_CLOSE = b"HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: close\r\n\r\ndiag"
CONTROL = {
    400: b"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
    413: b"HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
    414: b"HTTP/1.1 414 URI Too Long\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
    431: b"HTTP/1.1 431 Request Header Fields Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
}
UPGRADE = (b"GET /foundation/ws HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\n"
           b"Connection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
           b"Sec-WebSocket-Version: 13\r\n\r\n")
SWITCH = (b"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
          b"Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n")
BINARY_A = bytes.fromhex("82 81 01 02 03 04 40")
# Literal fixtures repeat ASCII, not production scanner/rendering algorithms.
RAW_BASE = b"GET /f HTTP/1.1\r\nHost: x\r\n" + (b"X: " + b" " * 4092 + b"a\r\n") * 8
RAW_EXACT = RAW_BASE + b"Y: " + b" " * 8142 + b"a\r\n\r\n"
RAW_OVER = RAW_BASE + b"Y: " + b" " * 8143 + b"a\r\n\r\n"
SEM_EXACT = (b"GET /f HTTP/1.1\r\nHost: x\r\n" + (b"X: " + b"a"*8191 + b"\r\n")*3
             + b"Y: " + b"a"*8164 + b"\r\n\r\n")
BUDGET = b"POST /p HTTP/1.1\r\nHost: x\r\nContent-Length: 4097\r\nBad Header: x\r\n\r\n"
HOST256 = b"a"*63 + b"." + b"b"*63 + b"." + b"c"*63 + b"." + b"d"*58 + b":12345"

# Each row is a finite, independently specified wire oracle. No expected result is
# derived by calling the SUT. (name, profile, bytes, status, route, native)
WIRE = (
    ("basic", "tiny", F01, 200, 0, False),
    ("get", "reference", b"GET /f HTTP/1.1\r\nHost: x\r\n\r\n", 200, 1, False),
    ("line2048", "reference", b"GET /"+b"a"*2032+b" HTTP/1.1\r\nHost: x\r\n\r\n", 200, 2, False),
    ("line2049", "reference", b"GET /"+b"a"*2033+b" HTTP/1.1\r\nHost: x\r\n\r\n", 414, None, False),
    ("method16", "method", b"ABCDEFGHIJKLMNOP /f HTTP/1.1\r\nHost: x\r\n\r\n", 200, 1, False),
    ("method17", "method", b"ABCDEFGHIJKLMNOPQ /f HTTP/1.1\r\nHost: x\r\n\r\n", 400, None, False),
    ("method_unknown", "reference", b"FROB /f HTTP/1.1\r\nHost: x\r\n\r\n", 400, None, False),
    ("raw40960", "reference", RAW_EXACT, 200, 1, False),
    ("raw40961", "reference", RAW_OVER, 431, None, False),
    ("fields64", "reference", b"GET /f HTTP/1.1\r\nHost: x\r\n"+b"X:\r\n"*63+b"\r\n", 200, 1, False),
    ("fields65", "reference", b"GET /f HTTP/1.1\r\nHost: x\r\n"+b"X:\r\n"*64+b"\r\n", 431, None, False),
    ("field8192", "reference", b"GET /f HTTP/1.1\r\nHost: x\r\nX: "+b"a"*8191+b"\r\n\r\n", 200, 1, False),
    ("field8193", "reference", b"GET /f HTTP/1.1\r\nHost: x\r\nX: "+b"a"*8192+b"\r\n\r\n", 431, None, False),
    ("semantic32768", "reference", SEM_EXACT, 200, 1, False),
    ("semantic32769", "reference", SEM_EXACT[:-4]+b"a\r\n\r\n", 431, None, False),
    ("body5", "tiny", F01.replace(b"Length: 4", b"Length: 5"), 413, None, False),
    ("cl_zero", "tiny", b"POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 0\r\n\r\n", 200, 0, False),
    ("cl_zeros", "tiny", b"POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 0000\r\n\r\n", 200, 0, False),
    ("priority_body", "budget", BUDGET, 413, None, False),
    ("priority_header", "budget", BUDGET[:-2]+b"X-Pad: "+b"a"*4022+b"\r\n\r\n", 431, None, False),
    ("priority_total", "budget", BUDGET.replace(b"4097",b"4096"), 431, None, False),
    ("priority_syntax", "budget", BUDGET.replace(b"4097",b"1"), 400, None, False),
    ("priority_positive", "budget", BUDGET.replace(b"4097",b"1").replace(b"Bad Header",b"Bad-Header")+b"a", 200, 3, False),
    ("native", "tiny", UPGRADE, 101, None, True),
    ("native_host256", "reference", UPGRADE.replace(b"Host: x",b"Host: "+HOST256), 101, None, True),
    ("native_host257", "reference", UPGRADE.replace(b"Host: x",b"Host: "+HOST256.replace(b":12345",b"d:12345")), 431, None, True),
    ("native_cookie", "reference", UPGRADE[:-2]+b"Cookie: synthetic=1\r\n\r\n", 101, None, True),
    ("native_cl0", "tiny", UPGRADE[:-2]+b"Content-Length: 0\r\n\r\n", 101, None, True),
    ("native_cl00", "p2", UPGRADE[:-2]+b"Content-Length: 00\r\n\r\n", 101, None, True),
    ("native_http", "tiny", b"GET /f HTTP/1.1\r\nHost: x\r\n\r\n", 400, None, True),
    ("ordinary_upgrade", "reference", UPGRADE, 400, None, False),
)
# Exact occurrence syntax variants; these add rows without hiding data-dependent loops.
BAD_CL = (b"+4", b"4x", b"", b"18446744073709551616", b"-1", b"4,4")
BAD_FIELDS = (
    b"Content-Length: 0\r\nContent-Length: 00\r\n", b"Content-Length: 0\r\nTransfer-Encoding: chunked\r\n",
    b"X-Host-Correlation: 44\r\nX-Host-Correlation: 44\r\n", b"TE:\r\n", b"Trailer: x\r\n",
    b"Expect:\r\n", b"Expect: 100-continue\r\n", b"Proxy-Connection: close\r\n",
    b"Connection: X-Host-Correlation\r\n", b" folded\r\n", b"Colonless\r\n", b"Bad : x\r\n",
    b"X: \x00\r\n", b"X: \x7f\r\n", b"Transfer-Encoding:\r\n")
BAD_NATIVE = (
    UPGRADE.replace(b"Host: x\r\n",b""), UPGRADE.replace(b"Host: x",b"Host: x\r\nHost: x"),
    UPGRADE.replace(b"Host: x",b"Host: x y"), UPGRADE.replace(b"Host: x",b"Host: [::1%lo]"),
    UPGRADE.replace(b"dGhlIHNhbXBsZSBub25jZQ==", b"dGhlIHNhbXBsZSBub25jZQ="),
    UPGRADE.replace(b"dGhlIHNhbXBsZSBub25jZQ==", b"dGhlIHNhbXBsZSBub25jZQ==="),
    UPGRADE.replace(b"dGhlIHNhbXBsZSBub25jZQ==", b"dGhlIHNhbXBsZSBub25jZR=="),
    UPGRADE.replace(b"Version: 13",b"Version: 12"), UPGRADE.replace(b"Sec-WebSocket-Version: 13\r\n",b""),
    UPGRADE.replace(b"Version: 13",b"Version: 13\r\nSec-WebSocket-Version: 13"),
    UPGRADE[:-2]+b"Sec-WebSocket-Protocol:\r\n\r\n", UPGRADE[:-2]+b"Sec-WebSocket-Extensions:\r\n\r\n",
    UPGRADE[:-2]+b"Content-Length: 1\r\n\r\n", UPGRADE.replace(b"Upgrade: websocket",b"Upgrade: h2c"),
    UPGRADE.replace(b"Connection: Upgrade",b"Connection: close"),
    UPGRADE.replace(b"Sec-WebSocket-Key: ",b"Sec-WebSocket-Key: bad\r\nSec-WebSocket-Key: "),
)
WIRE += tuple((f"badcl{i}","tiny",F01.replace(b"Length: 4",b"Length: "+cl),400,None,False) for i,cl in enumerate(BAD_CL))
WIRE += tuple((f"badfield{i}","reference",b"GET /f HTTP/1.1\r\nHost: x\r\n"+f+b"\r\n",400,None,False) for i,f in enumerate(BAD_FIELDS))
WIRE += tuple((f"badnative{i}","reference",f,400,None,True) for i,f in enumerate(BAD_NATIVE))
BAD_LINES = (
    b"GET /f HTTP/1.0\r\nHost: x\r\n\r\n", b"GET /f HTTP/2.0\r\nHost: x\r\n\r\n",
    b"GET /f HTTP/1.1\r\n\r\n", b"GET /f HTTP/1.1\r\nHost: x\r\nHost: x\r\n\r\n",
    b"GET /f HTTP/1.1\r\nHost: x y\r\n\r\n", b"GET /f HTTP/1.1\nHost: x\n\n",
    b"PUT /f HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\n", # no refused body supplied
    b"POST /missing HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\n",
    b"GET /f? HTTP/1.1\r\nHost: x\r\n\r\n", b"GET /%66 HTTP/1.1\r\nHost: x\r\n\r\n",
    b"GET /f#frag HTTP/1.1\r\nHost: x\r\n\r\n", b"GET http://x/f HTTP/1.1\r\nHost: x\r\n\r\n",
)
WIRE += tuple((f"badline{i}","reference",f,400,None,False) for i,f in enumerate(BAD_LINES))

WIRE += (
    ("priority_raw_over_line", "reference", RAW_OVER.replace(b"GET /f",b"GET /"+b"a"*2033),431,None,False),
    ("priority_count_over_syntax", "reference", b"GET /f HTTP/1.1\r\nHost: x\r\n"+b"Colonless\r\n"*64+b"\r\n",431,None,False),
    ("native_query", "reference", UPGRADE.replace(b"/foundation/ws",b"/foundation/ws?"),400,None,True),
    ("native_key_absent", "reference", UPGRADE.replace(b"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n",b""),400,None,True),
    ("native_upgrade_absent", "reference", UPGRADE.replace(b"Upgrade: websocket\r\n",b""),400,None,True),
    ("native_connection_absent", "reference", UPGRADE.replace(b"Connection: Upgrade\r\n",b""),400,None,True),
    ("native_upgrade_duplicate", "reference", UPGRADE.replace(b"Upgrade: websocket",b"Upgrade: websocket\r\nUpgrade: websocket"),400,None,True),
    ("native_connection_duplicate", "reference", UPGRADE.replace(b"Connection: Upgrade",b"Connection: Upgrade\r\nConnection: Upgrade"),400,None,True),
    ("native_negotiation_nonempty", "reference", UPGRADE[:-2]+b"Sec-WebSocket-Protocol: synthetic\r\n\r\n",400,None,True),
)

WIRE += (
    ("host_ipv6", "reference", b"GET /f HTTP/1.1\r\nHost: [::1]:12345\r\n\r\n",200,1,False),
    ("cl_leading_zeros4", "tiny", F01.replace(b"Length: 4",b"Length: 0004"),200,0,False),
    ("connection_keepalive", "reference", F01.replace(b"Host: x",b"Host: x\r\nConnection: keep-alive"),200,0,False),
    ("native_connection_tokens", "reference", UPGRADE.replace(b"Connection: Upgrade",b"Connection: keep-alive, Upgrade"),101,None,True),
)


WIRE += (
    ("ordinary_upgrade_known_route", "reference", b"GET /f HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n\r\n",400,None,False),
    ("host_userinfo", "reference", b"GET /f HTTP/1.1\r\nHost: user@x\r\n\r\n",400,None,False),
    ("host_bad_bracket", "reference", b"GET /f HTTP/1.1\r\nHost: [::1\r\n\r\n",400,None,False),
    ("host_ambiguous_port", "reference", b"GET /f HTTP/1.1\r\nHost: x:1:2\r\n\r\n",400,None,False),
)


def sha(path):
    # Bounded regular files only; nonblocking open also refuses FIFO/device input
    # before any read. This consumes an external seal, never creates an admission.
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_CLOEXEC)
    with os.fdopen(fd, "rb") as f:
        before = os.fstat(f.fileno())
        require(stat.S_ISREG(before.st_mode) and before.st_size <= 2147483648,
                "ADMISSION: regular-file/2GiB hash bound")
        h = hashlib.sha256()
        for _ in range(32769):  # 32768 chunks plus one EOF observation at exact2GiB.
            b = f.read(65536)
            if not b:
                after = os.fstat(f.fileno())
                fingerprint = lambda v: (v.st_dev,v.st_ino,v.st_size,v.st_mtime_ns,v.st_ctime_ns)
                require(fingerprint(before)==fingerprint(after),"ADMISSION: file changed during hashing")
                return h.hexdigest()
            h.update(b)
    raise AssertionError("ADMISSION: file hash iteration cap")


def require(condition, message):
    """Fail closed even when this file is parsed by an optimized interpreter."""
    if not condition:
        raise AssertionError(message)


SOURCE_PATHS = (
    "server/include/subathon/server/bounded_http.hpp",
    "server/include/subathon/server/testing/bounded_http_observers.hpp",
    "server/src/bounded_http.cpp",
    "tests/bounded_http_foundation/bounded_http_probe.cpp",
    "tests/bounded_http_foundation/test_bounded_http.py",
    "tests/bounded_http_foundation/inventory.json",
    "tests/bounded_http_foundation/RESOURCE_REVIEW.md",
    "tests/bounded_http_foundation/CMakeLists.txt",
    "cmake/AcceptanceSuites.cmake", "tests/CMakeLists.txt", "CMakePresets.json",
    "docs/TEST_STRATEGY.md", "tests/tooling/test_acceptance_selection.py",
)
REPOSITORY = Path(__file__).resolve().parents[2]


def closed_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "ADMISSION/FIXTURE: duplicate JSON key")
        result[key] = value
    return result


def admission(document, probe):
    require(type(document) is dict and set(document) ==
            {"source_commit", "review_receipt", "files", "sources", "environment"},
            "ADMISSION: exact document schema required")
    def hex_string(value, size):
        return type(value) is str and len(value) == size and all(c in "0123456789abcdef" for c in value)
    require(hex_string(document["source_commit"], 40), "ADMISSION: full source commit")
    url = document["review_receipt"]
    require(type(url) is str and 8 < len(url) <= 2048 and url.startswith("https://") and
            not any(c.isspace() or ord(c) < 32 for c in url), "ADMISSION: review receipt syntax")
    files, sources, environment = (document[k] for k in ("files", "sources", "environment"))
    require(type(files) is dict and 0 < len(files) <= 128 and str(probe) in files,
            "ADMISSION: exact binary closure must contain this probe")
    require(all(type(p) is str and p.startswith("/") and not any(ord(c)<32 for c in p) and len(os.fsencode(p)) <= 4096 and
                hex_string(h, 64) for p, h in files.items()), "ADMISSION: file paths/hex digests")
    require(type(sources) is dict and set(sources) == set(SOURCE_PATHS) and
            all(hex_string(h, 64) for h in sources.values()), "ADMISSION: exact reviewed source set")
    require(type(environment) is dict and set(environment) <=
            {"PATH", "LD_LIBRARY_PATH", "LANG", "LC_ALL", "TZ"} and
            all(type(v) is str and "\x00" not in v and len(v.encode()) <= 4096
                for v in environment.values()), "ADMISSION: scrubbed string environment")
    # The coordinator authenticates actual review and the commit-to-source receipt.
    # A URL cannot do that. These hashes bind the code actually used by this runner.
    require({p: sha(REPOSITORY / p) for p in SOURCE_PATHS} == sources,
            "ADMISSION: reviewed test/probe/NI/header/inventory/integration source seal")
    return document


class Child:
    """One actual framework/profile lifetime; no early NI attribution past admission."""
    def __init__(self, owner, **config):
        require(sys.flags.optimize==0,"ADMISSION: optimized Python is forbidden")
        self.cleaned = False
        self.owner, self.config, self.process = owner, config, None
        self.sockets, self.events, self.parts = [], [], {}
        self.bytes = self.count = self.commands = 0
        self.closed, self.terminal = False, None
        self.stderr = bytearray()
        self.map_rows, self.map_ends, self.admitted = {}, {}, set()
        self.seal = {}
        self.end = time.monotonic() + CHILD_SECONDS
        self.root = None
        try:
            self.root = Path(tempfile.mkdtemp(prefix="apitest016-")).resolve()
            owner.addCleanup(self.cleanup)
            self.root.chmod(0o700)
            require(len(os.fsencode(self.root)) <= 512, "FIXTURE: scratch path bound")
            (self.root / "empty").mkdir()
            (self.root / "upload/tmp/00").mkdir(parents=True)
            for p in ("empty/index.html", "empty/secret.txt", "secret.txt", "upload/tmp/00/secret.txt"):
                (self.root / p).write_bytes(b"PRIVATE-SENTINEL")
            require(ADMISSION is not None and PROBE is not None, "ADMISSION required")
            self.seal = {p: sha(p) for p in ADMISSION["files"]}
            require(self.seal == ADMISSION["files"], "ADMISSION: binary seal changed before launch")
            require({p: sha(REPOSITORY / p) for p in SOURCE_PATHS} == ADMISSION["sources"],
                    "ADMISSION: source seal changed before launch")
            args = json.dumps(config, separators=(",", ":"))
            require(len(args.encode()) <= 512, "FIXTURE: argv cap")
            self.process = subprocess.Popen([str(PROBE), args], stdin=subprocess.PIPE,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.root,
                env=ADMISSION["environment"], bufsize=0, close_fds=True)
            for f in (self.process.stdout, self.process.stderr):
                os.set_blocking(f.fileno(), False)
                self.parts[f.fileno()] = bytearray()
            self.wait("maps_end", phase="initial")
            self.check_maps("initial")
        except BaseException:
            self.cleanup()
            raise

    def remaining(self):
        n = min(STEP_SECONDS, self.end - time.monotonic())
        require(n > 0, "FIXTURE: child watchdog, not behavioral red")
        return n

    def pump(self):
        readable, _, _ = select.select(list(self.parts), [], [], min(.05, self.remaining()))
        for fd in readable:
            try:
                b = os.read(fd, 4096)
            except (BlockingIOError, InterruptedError):
                continue
            if not b:
                require(not self.parts[fd], "FIXTURE: truncated event")
                del self.parts[fd]
                continue
            self.bytes += len(b)
            require(self.bytes <= MAX_LOG, "FIXTURE: output budget")
            if fd == self.process.stderr.fileno():
                self.stderr.extend(b)
                continue
            part = self.parts[fd]
            part.extend(b)
            for _ in range(4096):
                if b"\n" not in part:
                    break
                at = part.index(10) + 1
                line = bytes(part[:at])
                del part[:at]
                require(len(line) <= MAX_LINE, "FIXTURE: line budget")
                try:
                    v = json.loads(line,object_pairs_hook=closed_object)
                except (ValueError, UnicodeError) as e:
                    raise AssertionError("FIXTURE: invalid event JSON") from e
                self.count += 1
                require(type(v) is dict and type(v.get("event")) is str and
                        self.count <= MAX_EVENTS, "FIXTURE: event inventory")
                name = v["event"]
                if name in ("missing_producer", "assertion"):
                    require(self.terminal is None, "FIXTURE: duplicate terminal event")
                    self.terminal = v  # Defer attribution until maps + exact reap + stderr.
                elif name == "map":
                    phase = v.get("phase")
                    require(phase in ("initial", "serving", "cleanup") and
                            phase not in self.map_ends and type(v.get("line")) is str and
                            len(v["line"].encode()) <= 4096, "FIXTURE: map record/order")
                    rows = self.map_rows.setdefault(phase, [])
                    rows.append(v["line"])
                    require(len(rows) <= 256, "FIXTURE: maps count")
                else:
                    if name == "maps_end":
                        phase = v.get("phase")
                        require(phase in ("initial", "serving", "cleanup") and
                                phase not in self.map_ends and type(v.get("count")) is int,
                                "FIXTURE: maps terminator")
                        self.map_ends[phase] = v["count"]
                    self.events.append(v)
            require(len(part) <= MAX_LINE, "FIXTURE: partial event bound")
        # Admit complete reached phases even when terminal shares their pipe read.
        for phase in self.map_ends:
            self.check_maps(phase)

    def check_maps(self, phase):
        if phase in self.admitted:
            return
        require(phase in self.map_ends, "ADMISSION: missing reached-phase terminator")
        rows = self.map_rows.get(phase, [])
        require(0 < len(rows) <= 256 and len(rows) == self.map_ends[phase],
                "ADMISSION: complete nonempty reached-phase maps")
        seen = set()
        for line in rows:
            words = line.split(maxsplit=5)
            require(len(words) >= 5, "ADMISSION: malformed map line")
            if len(words) < 6 or words[5].startswith("["):
                continue
            p = words[5]
            require(p.startswith("/") and " (deleted)" not in p and p in self.seal,
                    "ADMISSION: unresolved or unadmitted loaded file " + p)
            seen.add(p)
        require(str(PROBE) in seen, "ADMISSION: this executable absent from actual maps")
        require(all(sha(p) == self.seal[p] for p in seen), "ADMISSION: mapped seal changed")
        self.admitted.add(phase)

    def reap(self, expected, *, abort=False):
        deadline = time.monotonic() + self.remaining()
        for _ in range(MAX_TURNS):
            if not self.parts and self.process.poll() is not None:
                break
            require(time.monotonic() < deadline, "FIXTURE: terminal pipe/reap timeout")
            self.pump()
        else:
            raise AssertionError("FIXTURE: terminal drain iteration bound")
        code = self.process.wait(timeout=REAP_SECONDS)
        require(code == expected, f"FIXTURE: expected exact exit {expected}, got {code}")
        require(set(self.map_rows) == set(self.map_ends) == self.admitted and
                "initial" in self.admitted, "ADMISSION: every reached map must be admitted")
        if not abort:
            require(not self.stderr, f"FIXTURE: nonempty stderr {self.stderr[:512]!r}")
        self.closed = True
        return code

    def terminal_failure(self):
        e = self.terminal
        if e is None:
            return
        is_ni = e["event"] == "missing_producer"
        self.reap(40 if is_ni else 42)
        if is_ni:
            mode = self.config.get("mode")
            boundary = ("project_storage" if mode == "arithmetic" else
                "foundation.create.duration_bound" if mode == "config" and
                self.config.get("case") == "duration-bound" else
                "foundation.create.config_before_listeners_run" if mode == "config" else
                "foundation.create.before_listeners_run" if mode == "feature" else None)
            require(boundary is not None and e.get("boundary") == boundary,
                    "FIXTURE: unexpected NI mode/boundary")
            require("serving" not in self.admitted, "FIXTURE: first NI occurred after serving")
            self.cleanup()
            raise AssertionError(f"MISSING_PRODUCER {boundary}; admitted initial maps; exact exit40; wire bodies UNEXECUTED")
        require(type(e.get("oracle", e.get("boundary"))) is str,
                "FIXTURE: unnamed assertion exit42")
        self.cleanup()
        raise AssertionError(f"PRODUCT assertion; admitted reached maps; exact exit42: {e}")

    def wait(self, name, **fields):
        deadline = time.monotonic() + self.remaining()
        for _ in range(MAX_TURNS):
            self.terminal_failure()
            for i, e in enumerate(self.events):
                if e["event"] == name and all(e.get(k) == v for k, v in fields.items()):
                    return self.events.pop(i)
            if time.monotonic() >= deadline:
                break
            if not self.parts:
                # Never call an unnamed exit a product red.
                raise AssertionError(f"SETUP/FIXTURE: child exited {self.process.poll()} before {name}; stderr={self.stderr[:512]!r}")
            self.pump()
        product = {"request", "ended", "removed", "opened", "closed", "message", "ready",
                   "stop_drained", "pressure_held", "pressure_refused", "pressure_released"}
        label = "PRODUCT" if name in product else "FIXTURE"
        raise AssertionError(f"{label}: missing {name}; a timeout alone is not intended red")

    def start(self):
        e = self.wait("listeners")
        self.check_maps("serving")
        require(type(e.get("http")) is int and 0 < e["http"] < 65536, "FIXTURE: actual HTTP port")
        require(type(e.get("native")) is int and (e["native"] == 0 or
                0 < e["native"] < 65536 and e["native"] != e["http"]), "FIXTURE: actual native port")
        configuration = self.wait("configuration")
        require(configuration.get("upload") == str(self.root / "upload") and
                configuration.get("document") == str(self.root / "empty"), "FIXTURE: actual scratch configuration")
        self.ports = e
        return self

    def connect(self, native=False, *, small_receive=False):
        require(len(self.sockets) < MAX_PEERS, "FIXTURE: peer budget")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sockets.append(s)
        s.settimeout(self.remaining())
        if small_receive:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
            require(s.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF) >= 4096,
                    "FIXTURE: client receive-buffer configuration")
        s.connect(("127.0.0.1", self.ports["native" if native else "http"]))
        return s

    def send(self, s, data):
        require(len(data) <= MAX_WIRE, "FIXTURE: payload bound")
        for at in range(0, len(data), 8192):
            s.sendall(data[at:at+8192])

    def delivered(self, s):
        """Linux TCP output-queue observation, not an assumption about packetization."""
        deadline = time.monotonic() + self.remaining()
        for _ in range(700):
            outstanding = struct.unpack("I", fcntl.ioctl(s.fileno(), 0x5411, struct.pack("I", 0)))[0]
            if outstanding == 0:
                return
            require(time.monotonic() < deadline, "FIXTURE: TCP delivery witness timeout")
            select.select([], [], [], .01)
        raise AssertionError("FIXTURE: SIOCOUTQ iteration bound")

    def barrier(self):
        # A future real timer readiness plus its end-of-active-batch marker
        # establishes IO0 progress after client delivery, not just queue submission.
        self.command("barrier")
        self.wait("io_barrier")

    def read(self, s, n):
        require(n <= MAX_WIRE, "FIXTURE: read cap")
        out = bytearray()
        for _ in range(MAX_TURNS):
            if len(out) == n:
                return bytes(out)
            s.settimeout(self.remaining())
            b = s.recv(min(8192, n-len(out)))
            require(bool(b), f"PRODUCT: early EOF {len(out)}/{n}")
            out.extend(b)
        raise AssertionError("FIXTURE: read iterations")

    def eof(self, s):
        s.settimeout(self.remaining())
        try:
            b = s.recv(1)
        except ConnectionResetError:
            b = b""
        require(b == b"", "PRODUCT: expected EOF with no additional response")

    def command(self, op, **fields):
        self.commands += 1
        require(self.commands <= MAX_COMMANDS, "FIXTURE: command budget")
        data = json.dumps(dict(op=op, **fields), separators=(",", ":")).encode() + b"\n"
        require(len(data) <= 513, "FIXTURE: command cap")
        self.process.stdin.write(data)
        self.wait("ack", op=op)

    def inspect(self):
        self.command("inspect")
        return self.wait("inspection")

    def stop(self):
        for s in self.sockets:
            s.close()
        self.sockets.clear()
        self.command("stop", ns=6000000000)
        self.wait("stop_drained")
        self.wait("framework_joined")
        self.wait("maps_end", phase="cleanup")
        self.check_maps("cleanup")
        self.wait("cleanup_complete")
        self.reap(0)
        self.cleanup()

    def cleanup(self):
        if self.cleaned:
            return
        self.cleaned = True
        failure = None
        if self.process:
            if self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=REAP_SECONDS)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    try:
                        self.process.wait(timeout=REAP_SECONDS)
                    except subprocess.TimeoutExpired:
                        failure = "FIXTURE: unreaped child"
            for s in self.sockets:
                s.close()
            for f in (self.process.stdin, self.process.stdout, self.process.stderr):
                f.close()
        require(failure is None, failure or "FIXTURE: reap")
        if self.root is not None:
            entries = list(self.root.rglob("*"))
            require(len(entries) <= 263 and all(not p.is_symlink() for p in entries),
                    "FIXTURE: scratch entries/symlink bound")
            directories = {str(p.relative_to(self.root)) for p in entries if p.is_dir()}
            allowed_dirs = {"empty", "upload", "upload/tmp"} | {f"upload/tmp/{i:02X}" for i in range(256)}
            files = {str(p.relative_to(self.root)): p for p in entries if p.is_file()}
            canaries = {"empty/index.html", "empty/secret.txt", "secret.txt", "upload/tmp/00/secret.txt"}
            require(directories <= allowed_dirs and set(files) == canaries and
                    all(p.read_bytes() == b"PRIVATE-SENTINEL" for p in files.values()),
                    "FIXTURE: exact actual-tree four16-byte canaries/64bytes; no spill")
            if "serving" in self.admitted:
                require(directories == allowed_dirs and len(entries) == 263,
                        "FIXTURE: actual SDK upload leaves/263 entries")
            shutil.rmtree(self.root)
        if ADMISSION is not None:
            require({p: sha(p) for p in self.seal} == self.seal, "ADMISSION: binary seal changed after child")
            require({p: sha(REPOSITORY/p) for p in SOURCE_PATHS} == ADMISSION["sources"],
                    "ADMISSION: source seal changed after child")
        self.events.clear()
        self.map_rows.clear()
        self.parts.clear()
        self.sockets.clear()
        self.seal.clear()


class FoundationTests(unittest.TestCase):
    def child(self,**kw): return Child(self,mode="feature",**kw).start()

    def retained_native(self, p, *, lease=False):
        # Native Context retirement and SDK disposal are independent. Real IO
        # progress may complete the former while the caller still pins the latter.
        for _ in range(32):
            p.barrier()
            x = p.inspect()
            self.assertFalse(any(e["event"] == "removed" and e.get("peer") == 0
                                 for e in p.events), "PRODUCT: removed while native owner retained")
            self.assertEqual(x["sdk_retained_peers"], 1)
            if lease:
                self.assertEqual(x["native_retires"], 0, "live lease must prevent native retirement")
                return x
            self.assertIn(x["native_retires"], (0, 1))
            if x["native_retires"] == 1:
                return x
        self.fail("PRODUCT: native retirement missing independently of SDK disposal; timeout alone is not red")

    def wire(self,row,split=None):
        name,profile,request,status,route,native=row
        p=self.child(profile=profile)
        s=p.connect(native)
        if split is None: p.send(s,request)
        else:
            p.send(s,request[:split])
            if split<len(request):
                for _ in range(32):
                    if p.inspect()["bytes_read"]==split: break
                else: self.fail("FIXTURE: split not observed before second write")
            p.send(s,request[split:])
        expected=SWITCH if status==101 else DIAG if status==200 else CONTROL[status]
        self.assertEqual(p.read(s,len(expected)),expected,name)
        if status==200:
            e=p.wait("request");self.assertEqual(e["route"],route)
            self.assertEqual(e["ordinal"],1)
            self.assertEqual(e["method"],request.split(b" ",2)[0].decode("ascii"))
            self.assertEqual(e["target"],request.split(b" ",2)[1].decode("ascii"))
            body=request.split(b"\r\n\r\n",1)[1]
            self.assertEqual(e["body_size"],len(body));self.assertEqual(e["body_sha256"],hashlib.sha256(body).hexdigest())
        elif status==101: p.wait("opened")
        else: p.eof(s)
        x=p.inspect();self.assertEqual(x["stock_batches"],0);self.assertEqual(x["stock_output_appends"],0)
        self.assertEqual(x["requests"],int(status==200))
        if status==101:
            self.assertEqual(x["native_projected_fields"],5);self.assertEqual(x["native_projected_cookies"],0)
            self.assertEqual(x["native_handoff_capacity"],0);self.assertEqual(x["native_handoff_accepted"],1)
        p.stop()

    def test_literal_wire_inventory(self):
        for row in WIRE:
            with self.subTest(case=row[0]): self.wire(row)

    def test_split_priority_and_line_envelopes(self):
        # Every CR/LF boundary of these complete heads, one fresh lifetime per split.
        names=("line2048","line2049","raw40960","raw40961","priority_body","priority_header","priority_total","priority_syntax","priority_positive")
        for row in WIRE:
            if row[0] not in names: continue
            for at in tuple(i+1 for i,b in enumerate(row[2]) if b in (10,13)):
                with self.subTest(case=row[0],split=at): self.wire(row,at)

    def test_field_occurrence_views(self):
        p=self.child();s=p.connect()
        data=b"GET /f HTTP/1.1\r\nHost: x\r\nx-ONE: \tfirst\t \r\nX-one:\r\nX-one: second\r\n\r\n"
        p.send(s,data);self.assertEqual(p.read(s,66),DIAG)
        e=p.wait("request");self.assertEqual(e["fields"],[["Host","x"],["x-ONE","first"],["X-one",""],["X-one","second"]]);p.stop()

    def test_reference_body_stream(self):
        p=self.child();s=p.connect();body=b"z"*1048576
        p.send(s,b"POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 1048576\r\n\r\n"+body)
        self.assertEqual(p.read(s,66),DIAG);e=p.wait("request")
        self.assertEqual((e["body_size"],e["body_sha256"]),(1048576,hashlib.sha256(body).hexdigest()))
        x=p.inspect();self.assertEqual(x["reserved_storage"],33101824);self.assertEqual(x["mapped_storage"],29888512);p.stop()

    def test_read_witness_two_phases(self):
        for cuts in ((32,20),(16,16,10,10)):
            p=self.child(profile="reference",action="hold");s=p.connect();at=0
            for size in cuts:
                p.send(s,F01[at:at+size]);at+=size
                # Observe cumulative producer state, not a sleep/last_read_* inference.
                for _ in range(32):
                    if p.inspect()["bytes_read"]==at: break
                else: self.fail("FIXTURE: phase cumulative witness absent")
            a=p.wait("latch",slot=0);b=p.wait("latch",slot=1)
            self.assertEqual(tuple(a[k] for k in ("head","tail","writable","offered","iovcnt")),(8,8,65536,65536,1),"FIXTURE: contradictory first latch")
            self.assertEqual(tuple(b[k] for k in ("head","tail","writable","offered","iovcnt")),(8,40,65504,65504,1),"FIXTURE: contradictory second latch")
            p.wait("request");p.command("reply");self.assertEqual(p.read(s,66),DIAG);p.stop()

    def test_second_coalesced_request_after_release(self):
        p=self.child(action="hold");s=p.connect();p.send(s,F01+F01)
        p.wait("request",ordinal=1);p.command("suspend")
        self.assertEqual(p.inspect()["requests"],1)
        p.command("reply");self.assertEqual(p.read(s,66),DIAG);p.wait("ended")
        self.assertEqual(p.inspect()["requests"],1)
        p.command("admit");p.command("admit");p.wait("request",ordinal=2)
        p.command("reply");self.assertEqual(p.read(s,66),DIAG)
        x=p.inspect();self.assertEqual(x["requests"],2);self.assertEqual(x["stock_batches"],0);p.stop()

    def test_full_receive_held_successor(self):
        p=self.child(action="full-receive");s=p.connect();p.wait("connected")
        configured=p.wait("listen_socket")
        require(configured["receive_buffer"]>=131072,"FIXTURE: actual receive buffer too small")
        p.send(s,F01+b"x"*65484);p.delivered(s);p.barrier()
        self.assertEqual((p.inspect()["reads"],p.inspect()["bytes_read"]),(0,0))
        p.command("admit")
        self.assertEqual(p.wait("latch",slot=0)["returned"],65536,"FIXTURE: actual full receive not witnessed")
        p.wait("request");x=p.inspect()
        self.assertEqual((x["requests"],x["reads"],x["bytes_read"],x["max_iovcnt"]),(1,1,65536,1))
        self.assertEqual((x["reserved_storage"],x["mapped_storage"]),(33101824,29888512))
        p.send(s,b"y"*8192);p.delivered(s);p.barrier();y=p.inspect()
        for key in ("requests","reads","bytes_read","reserved_storage","mapped_storage"):
            self.assertEqual(y[key],x[key],key)
        # One-current-request suspension may occur BEFORE another before_read guard;
        # zero guard-pauses is conforming. No-extra-syscall/no-growth discriminates.
        self.assertEqual(y["stock_batches"],0);p.command("close");p.eof(s);p.stop()

    def test_output_pressure_actual_node(self):
        # Child's second request drives the complete sequence locally on IO0.
        p=self.child(profile="pressure",action="pressure");a=p.connect();b=p.connect()
        p.send(a,F01);p.wait("request",peer=0);p.send(b,F01)
        p.wait("pressure_held");p.wait("pressure_refused");p.wait("pressure_released")
        self.assertEqual(p.read(a,66),DIAG);self.assertEqual(p.read(b,66),DIAG)
        x=p.inspect();self.assertEqual(x["held_application_wire"],0);p.stop()

    def test_once_drop_thread_reentry_faults(self):
        for action,success in (("once",True),("wrong-thread-result",True),("reentry",True),("revoked",True),
                               ("null-context",True),("null-read",True),("no-wake",True),("moves",True),("retain-inert",True),("bad-fill",False),("drop-request",False),
                               ("drop-reply",False),("cancel-before",False),("foreign-drop-request",False),("throw-request",False),("throw-read",False)):
            p=self.child(action=action);s=p.connect();p.send(s,F01)
            if success: self.assertEqual(p.read(s,66),DIAG)
            else: p.eof(s)
            x=p.inspect();self.assertEqual(x["requests"],0 if action=="throw-read" else 1)
            if action!="throw-read":
                self.assertEqual(p.wait("ended")["reason"],0 if success else 6 if action=="throw-request" else 2)
            p.stop()

    def test_wrong_thread_void_terminates(self):
        for action in ("wrong-thread-raw","wrong-thread-cancel"):
            p=self.child(action=action);s=p.connect();p.send(s,F01)
            p.wait("request")
            p.wait("before_wrong_thread_void",operation="release_raw" if action=="wrong-thread-raw" else "cancel")
            code=p.reap(-6,abort=True)
            self.assertEqual(code,-6,"attributed void wrong-thread call must SIGABRT")
            p.closed=True;p.cleanup()

    def test_truncated_body_eof(self):
        p=self.child();s=p.connect();p.send(s,F01[:-1]);s.shutdown(socket.SHUT_WR);p.eof(s)
        e=p.wait("read_result",returned=0);self.assertEqual(e["bytes_read"],51)
        self.assertIs(type(e["error"]),int)
        self.assertEqual(p.inspect()["requests"],0);p.stop()

    def test_request_connection_close_wins(self):
        p=self.child();s=p.connect();p.send(s,F01.replace(b"Host: x",b"Host: x\r\nConnection: close")+F01)
        self.assertEqual(p.read(s,len(DIAG_CLOSE)),DIAG_CLOSE);p.eof(s)
        self.assertEqual(p.inspect()["requests"],1);p.stop()

    def test_accept_time_equality_and_failure(self):
        for ns,available in ((5999000000,True),(6000000000,True),(999999999,True),(1000000000,False),(18446744073709551615,True)):
            p=self.child();s=p.connect();p.wait("connected")
            p.command("clock",ns=ns,available=available)
            if ns==5999000000:
                p.send(s,F01);self.assertEqual(p.read(s,66),DIAG)
            else: p.eof(s)
            p.stop()

    def test_request_no_trickle_extension_commit_equality(self):
        p=self.child(action="commit-equality");s=p.connect();p.command("clock",ns=1100000000)
        p.send(s,F01);p.eof(s);e=p.wait("ended");self.assertEqual(e["reason"],3);p.stop()
        p=self.child(action="hold");s=p.connect();p.command("clock",ns=1100000000)
        p.send(s,F01[:32])
        p.delivered(s)
        for _ in range(32):
            if p.inspect()["bytes_read"]==32: break
        else: self.fail("PRODUCT: first 32 bytes not observed before advancing request clock")
        p.command("clock",ns=6099000000);p.send(s,F01[32:40]);p.delivered(s);p.barrier()
        p.command("clock",ns=6100000000);p.eof(s);self.assertEqual(p.inspect()["requests"],0);p.stop()

    def test_stop_first_deadline(self):
        for repeat in (6500000000,5500000000):
            p=self.child(action="large-output");s=p.connect(small_receive=True)
            p.command("clock",ns=2000000000);p.send(s,F01);p.wait("request")
            actual=p.wait("accepted_socket");require(actual["send_buffer"]>=4096,"FIXTURE: actual SO_SNDBUF witness")
            # No deliberate SDK hold: witness the actual outstanding application slot.
            x=p.inspect();self.assertEqual(x["output_slots"],1,"FIXTURE: no actual retained output witness")
            p.command("stop",ns=6000000000);p.command("clock",ns=5999000000)
            p.command("stop",ns=repeat)
            self.assertEqual(p.inspect()["output_slots"],1)
            p.command("clock",ns=6000000000);p.wait("ended",reason=4)
            wire=bytearray()
            for _ in range(MAX_TURNS):
                data=s.recv(8192)
                if not data: break
                wire.extend(data);self.assertLessEqual(len(wire),1048704)
            else: self.fail("FIXTURE: stopped output read bound")
            expected=b"HTTP/1.1 200 OK\r\nContent-Length: 1048576\r\nConnection: keep-alive\r\n\r\n"+b"z"*1048576
            self.assertTrue(expected.startswith(wire));self.assertLess(len(wire),len(expected))
            p.wait("stop_drained");p.wait("framework_joined")
            p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
            p.wait("cleanup_complete");self.assertEqual(p.reap(0),0);p.closed=True;p.cleanup()

    def test_native_retention_blocks_stop_drain(self):
        for action in ("strong","weak","lease"):
            p=self.child(profile="p2",action=action);s=p.connect(True);p.send(s,UPGRADE)
            self.assertEqual(p.read(s,129),SWITCH);p.wait("opened")
            if action=="lease": p.send(s,BINARY_A);p.wait("message")
            p.command("close");p.wait("closed");p.command("drop")
            p.command("stop",ns=6000000000);p.command("clock",ns=6000000000)
            self.retained_native(p,lease=action=="lease")
            self.assertFalse(any(e["event"]=="stop_drained" for e in p.events))
            p.command("foreign-release");self.assertEqual(p.wait("removed",peer=0)["native_retires"],1)
            p.wait("stop_drained")
            p.wait("framework_joined");p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
            p.wait("cleanup_complete");self.assertEqual(p.reap(0),0);p.closed=True;p.cleanup()

    def test_native_lifetime_strong_weak_lease(self):
        for action in ("strong","weak","lease","facade-negative"):
            p=self.child(profile="p2",action=action);s=p.connect(True);p.send(s,UPGRADE)
            self.assertEqual(p.read(s,129),SWITCH);p.wait("opened")
            if action=="lease": p.send(s,BINARY_A);p.wait("message")
            if action=="facade-negative": p.command("facade-negative")
            p.command("close");p.wait("closed")
            p.command("drop") # native peers only receive Peer+Wake; no fabricated Request/Reply
            x=self.retained_native(p,lease=action=="lease")
            self.assertEqual(x["peers"],1)
            # Backlog connect can succeed; no extra native construction is promised.
            spare=p.connect(True);p.send(spare,b"G");p.delivered(spare);p.barrier()
            self.assertEqual(p.inspect()["constructed"],x["constructed"])
            self.assertFalse(any(e["event"]=="removed" and e.get("peer")==0 for e in p.events))
            if action=="facade-negative": p.command("facade-negative")
            p.command("foreign-release");self.assertEqual(p.wait("removed",peer=0)["native_retires"],1)
            spare.close();p.stop()

    def test_native_read_ahead_assignment(self):
        for profile,accepted in (("reference",True),("tiny",False),("p2",False)):
            # Contract F13 permits three fresh peers per profile; require a real same-read witness.
            witnessed=False
            for attempt in range(3):
                p=self.child(profile=profile);s=p.connect(True);p.send(s,UPGRADE+BINARY_A)
                latch=p.wait("latch",slot=0)
                if latch["returned"]!=160:
                    p.cleanup();continue
                witnessed=True
                if accepted:
                    self.assertEqual(p.read(s,129),SWITCH);p.wait("message")
                else: p.eof(s)
                x=p.inspect();self.assertEqual(x["native_handoff_capacity"],65544)
                self.assertEqual(x["native_handoff_accepted"],int(accepted));p.stop();break
            self.assertTrue(witnessed,"FIXTURE: no actual same-read Upgrade witness; not red")

    def test_role_quota_retained_handles(self):
        p=self.child(profile="p2",action="hold");a=p.connect();p.wait("connected",peer=0)
        b=p.connect(True);p.wait("connected",peer=1);x=p.inspect()
        self.assertEqual((x["peers"],x["http_peers"],x["native_peers"]),(2,1,1))
        c=p.connect();p.send(c,b"G");p.delivered(c);p.barrier();y=p.inspect();self.assertEqual(y["constructed"],2)
        self.assertEqual(y["accept_attempts"],x["accept_attempts"])
        self.assertEqual(y["transient_fds"],0);c.close();a.close();b.close();p.stop()

    def test_reference_http_quota80(self):
        p=self.child()
        for i in range(80): p.connect();p.wait("connected",peer=i)
        x=p.inspect();extra=p.connect();p.send(extra,b"G");p.delivered(extra);p.barrier();y=p.inspect()
        self.assertEqual((x["http_peers"],x["constructed"]),(80,80))
        self.assertEqual(y["accept_attempts"],x["accept_attempts"])
        p.connect(True);p.wait("connected",peer=80);self.assertEqual(p.inspect()["native_peers"],1);p.stop()

    def test_post_ready_and_moves(self):
        p=self.child(action="hold");s=p.connect();p.send(s,F01);p.wait("request")
        p.command("post");post=p.wait("post");self.assertIn(post["result"],(0,1,2))
        p.wait("ready");self.assertEqual(p.inspect()["requests"],1)
        p.command("reply");self.assertEqual(p.read(s,66),DIAG);p.stop()

    def test_input_pool_before_first_read(self):
        p=self.child(action="hold");peers=[]
        for i in range(9): peers.append(p.connect());p.wait("connected",peer=i)
        for i in range(8): p.send(peers[i],F01);p.wait("request",peer=i)
        p.send(peers[8],F01);p.delivered(peers[8]);p.barrier();x=p.inspect()
        self.assertEqual((x["input_slots"],x["input_capacity"],x["requests"]),(8,8388608,8))
        self.assertEqual(x["bytes_read"],416)
        p.command("close",peer=0);p.wait("request",peer=8)
        self.assertEqual(p.inspect()["peak_transient_fds"],1);p.stop()

    def test_gate_cancel_releases_node_once(self):
        p=self.child(action="gate-cancel");s=p.connect();p.send(s,F01);p.wait("request")
        self.assertEqual(p.inspect()["held_application_wire"],1)
        p.command("pressure-cancel");p.eof(s);p.wait("ended",reason=2)
        x=p.inspect();self.assertEqual((x["output_slots"],x["wire_releases"]),(0,1));p.stop()
        p=self.child(action="gate-stop");s=p.connect();p.send(s,F01);p.wait("request")
        self.assertEqual(p.inspect()["held_application_wire"],1)
        # Stop at 5s precedes the application's 6s output bound. Earlier drain is
        # allowed; advance before requiring terminal output rather than demanding it now.
        p.command("stop-at-deadline",ns=5000000000)
        wire=bytearray()
        for _ in range(128):
            block=s.recv(66)
            if not block: break
            wire.extend(block);self.assertLessEqual(len(wire),66)
        else: self.fail("FIXTURE: gate-stop EOF bound")
        end=p.wait("ended")["reason"];self.assertIn(end,(0,4))
        self.assertTrue(DIAG.startswith(wire))
        if end==0: self.assertEqual(wire,DIAG)
        p.wait("stop_drained");p.wait("framework_joined")
        p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
        p.wait("cleanup_complete");self.assertEqual(p.reap(0),0);p.closed=True;p.cleanup()

    def test_http_retained_handles_pin_role(self):
        p=self.child(profile="p2",action="retain-http");a=p.connect();p.wait("connected",peer=0)
        a.close();p.wait("removed",peer=0)
        x=p.inspect();b=p.connect();p.send(b,b"G");p.delivered(b);p.barrier()
        self.assertEqual(p.inspect()["constructed"],x["constructed"])
        p.command("drop-http-handles",peer=0);p.wait("connected",peer=1);p.barrier()
        self.assertEqual(p.inspect()["constructed"],x["constructed"]+1);p.stop()

    def test_established_native_ignores_http_idle_and_read_observer(self):
        p=self.child(action="lease");s=p.connect(True);p.send(s,UPGRADE)
        self.assertEqual(p.read(s,129),SWITCH);p.wait("opened");p.barrier();x=p.inspect()
        p.command("clock",ns=7000000000);p.barrier()
        p.send(s,BINARY_A);p.wait("message");p.barrier();y=p.inspect()
        self.assertEqual((y["reads"],y["bytes_read"]),(x["reads"],x["bytes_read"]))
        self.assertEqual(y["fixture_read_callbacks"],x["fixture_read_callbacks"])
        self.assertEqual(y["control_capacity"],0)
        p.command("foreign-release");p.stop()

    def test_gate_excludes_control_and_native101(self):
        p=self.child(action="gate-exclusions");bad=p.connect()
        p.send(bad,b"GET /missing HTTP/1.1\r\nHost: x\r\n\r\n")
        self.assertEqual(p.read(bad,66),CONTROL[400]);p.eof(bad);p.wait("removed",peer=0)
        native=p.connect(True);p.send(native,UPGRADE)
        self.assertEqual(p.read(native,129),SWITCH);p.wait("opened");p.barrier()
        x=p.inspect();self.assertEqual((x["held_application_wire"],x["control_capacity"]),(0,0))
        ordinary=p.connect();p.send(ordinary,F01);p.wait("request")
        self.assertEqual(p.inspect()["held_application_wire"],1)
        p.command("clock",ns=6000000000);p.eof(ordinary);p.wait("ended",reason=3)
        p.stop()

    def test_post_after_stop_is_refused(self):
        p=self.child(action="strong");s=p.connect(True);p.send(s,UPGRADE)
        self.assertEqual(p.read(s,129),SWITCH);p.wait("opened")
        p.command("stop",ns=5000000000);p.command("post-stopping")
        p.command("clock",ns=5000000000);p.wait("closed")
        self.retained_native(p)
        self.assertFalse(any(e["event"]=="stop_drained" for e in p.events))
        p.command("foreign-release");self.assertEqual(p.wait("removed",peer=0)["native_retires"],1)
        p.wait("stop_drained")
        p.wait("framework_joined");p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
        p.wait("cleanup_complete");p.reap(0);p.cleanup()

    def test_zero_and_close_reply(self):
        for action,expected in (("zero-reply",b"HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n"),("reply-close",DIAG_CLOSE)):
            p=self.child(action=action);s=p.connect();p.send(s,F01)
            self.assertEqual(p.read(s,len(expected)),expected)
            if action=="reply-close": p.eof(s)
            p.stop()

    def test_configuration_checked_before_registration(self):
        for name in ("zero-duration","roles","output-count","duration-over","epoll-narrow","overflow-before-native","allocation","missing-source","duration-bound"):
            p=Child(self,mode="config",case=name)
            p.wait("config_pass");p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
            self.assertEqual(p.reap(0),0);p.closed=True;p.cleanup()

    def test_idle_anchor_after_terminal_and_output_expiry(self):
        p=self.child();s=p.connect();p.send(s,F01);self.assertEqual(p.read(s,66),DIAG)
        p.wait("ended");p.command("clock",ns=5999000000)
        p.send(s,F01);self.assertEqual(p.read(s,66),DIAG)
        p.wait("ended");p.command("clock",ns=10999000000);p.eof(s);p.stop()
        p=self.child(action="output-expiry");s=p.connect();p.command("clock",ns=2000000000)
        p.send(s,F01);p.wait("request")
        x=p.inspect();self.assertEqual((x["output_slots"],x["held_application_wire"]),(1,1))
        p.command("clock",ns=6999000000);self.assertEqual(p.inspect()["output_slots"],1)
        p.command("clock",ns=7000000000);p.eof(s);p.wait("ended",reason=3)
        self.assertEqual(p.inspect()["output_slots"],0);p.stop()

    def test_owner_samples_are_not_globally_ordered(self):
        p=self.child(action="owner-nonregression");s=p.connect();p.send(s,F01)
        self.assertEqual(p.read(s,66),DIAG);p.stop()

    def test_mailbox_coalescing_during_ready_and_close(self):
        for action in ("hold","ready-repost"):
            p=self.child(action=action);s=p.connect();p.send(s,F01);p.wait("request")
            if action=="ready-repost": self.assertEqual(p.read(s,66),DIAG)
            p.command("post-pair");p.wait("ready")
            if action=="ready-repost": p.wait("ready")
            p.command("post-close");p.eof(s);p.wait("removed");p.stop()

    def test_raw_wipe_hooks_quarantine(self):
        p=self.child(action="wipe-failure");s=p.connect();p.send(s,F01);p.eof(s)
        x=p.inspect();self.assertGreater(x["fixture_wipes"],0)
        self.assertEqual(x["output_slots"],0);p.stop()

    def test_storage_and_guard_literals(self):
        p=Child(self,mode="arithmetic");p.wait("arithmetic_pass")
        p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
        self.assertEqual(p.reap(0),0);p.closed=True;p.cleanup()


class LegacyControl(unittest.TestCase):
    def test_actual_legacy_positive(self):
        p=Child(self,mode="legacy").start();s=p.connect()
        p.send(s,b"GET /legacy-positive HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
        out=bytearray()
        for _ in range(64):
            b=s.recv(4096)
            if not b: break
            out.extend(b);self.assertLessEqual(len(out),4096)
        else: self.fail("FIXTURE: legacy read cap")
        self.assertTrue(out.startswith(b"HTTP/1.1 200 OK\r\n"));self.assertTrue(out.endswith(b"\r\n\r\nlegacy-positive"))
        p.wait("legacy_request");p.command("stop",ns=6000000000)
        p.wait("framework_joined");p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
        p.wait("cleanup_complete");self.assertEqual(p.reap(0),0);p.closed=True;p.cleanup()


def main():
    global PROBE, ADMISSION
    require(sys.flags.optimize == 0, "ADMISSION: optimized Python is forbidden")
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--admission", type=Path, required=True)
    parser.add_argument("selectors", nargs="*")
    args = parser.parse_args()
    PROBE = args.probe.resolve()
    require(args.admission.stat().st_size <= 65536, "ADMISSION: manifest bound")
    ADMISSION = admission(json.loads(args.admission.read_text(),object_pairs_hook=closed_object), PROBE)
    loader = unittest.defaultTestLoader
    legacy = unittest.TextTestRunner(verbosity=2).run(loader.loadTestsFromTestCase(LegacyControl))
    if not legacy.wasSuccessful():
        raise SystemExit(1)  # No foundation selector is meaningful without real setup.
    if args.selectors:
        available = set(loader.getTestCaseNames(FoundationTests))
        require(len(args.selectors) == len(set(args.selectors)) <= 64 and
                set(args.selectors) <= available, "FIXTURE: exact selected method inventory")
        suite = unittest.TestSuite(FoundationTests(name) for name in args.selectors)
    else:
        suite = loader.loadTestsFromTestCase(FoundationTests)
    # Every selector executes after the genuine positive gate, with ordinary failure semantics.
    result = unittest.TextTestRunner(verbosity=2, failfast=False).run(suite)
    raise SystemExit(not result.wasSuccessful())


if __name__ == "__main__":
    main()
