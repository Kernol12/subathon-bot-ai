# SPDX-License-Identifier: GPL-3.0-or-later
"""APITEST-016 authored literals. Execution requires a NEW reviewed consumer admission.

No import/discovery/identity/body execution is authorized by this source packet.
NI is an ordinary failing availability assertion, never a pass or expected failure.
"""
from __future__ import annotations

import argparse
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


def sha(path):
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for _ in range(32768): # <=2 GiB admitted file, bounded read chunk
            b=f.read(65536)
            if not b: return h.hexdigest()
            h.update(b)
    raise AssertionError("ADMISSION: file hash size cap")


class Child:
    """One actual framework/profile lifetime; no daemon readers or hidden retry children."""
    def __init__(self, owner, **config):
        self.cleaned=False; self.owner=owner; self.process=None; self.sockets=[]; self.events=[]
        self.parts={}; self.bytes=0; self.count=0; self.commands=0; self.closed=False
        self.end=time.monotonic()+CHILD_SECONDS
        self.root=Path(tempfile.mkdtemp(prefix="apitest016-"))
        self.root.chmod(0o700)
        assert len(os.fsencode(self.root))<=512,"FIXTURE: scratch path bound"
        (self.root/"empty").mkdir(); (self.root/"upload").mkdir()
        # Four synthetic 16-byte canaries, including the SDK upload leaf.
        (self.root/"upload/tmp/00").mkdir(parents=True)
        for p in ("empty/index.html","empty/secret.txt","secret.txt","upload/tmp/00/secret.txt"):
            (self.root/p).write_bytes(b"PRIVATE-SENTINEL\n")
        owner.addCleanup(self.cleanup)
        assert ADMISSION is not None and PROBE is not None, "ADMISSION required"
        self.seal={str(p):sha(p) for p in ADMISSION["files"]}
        assert self.seal==ADMISSION["files"], "ADMISSION: seal changed before launch"
        args=json.dumps(config,separators=(",",":"))
        assert len(args.encode())<=512
        self.process=subprocess.Popen([str(PROBE),args],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE,cwd=self.root,env=ADMISSION["environment"],
                                      bufsize=0,close_fds=True)
        for f in (self.process.stdout,self.process.stderr):
            os.set_blocking(f.fileno(),False); self.parts[f.fileno()]=bytearray()
        self.wait("maps_end",phase="initial")
        self.check_maps("initial")

    def remaining(self):
        n=min(STEP_SECONDS,self.end-time.monotonic())
        if n<=0: raise AssertionError("FIXTURE: child watchdog, not behavioral red")
        return n

    def pump(self):
        fds=list(self.parts)
        readable,_,_=select.select(fds,[],[],min(.05,self.remaining()))
        for fd in readable:
            try: b=os.read(fd,4096)
            except (BlockingIOError,InterruptedError): continue
            if not b:
                if self.parts[fd]: raise AssertionError("FIXTURE: truncated event")
                del self.parts[fd]; continue
            self.bytes+=len(b)
            assert self.bytes<=MAX_LOG,"FIXTURE: output budget"
            if fd==self.process.stderr.fileno():
                raise AssertionError(f"FIXTURE/SDK diagnostics: {b[:512]!r}")
            part=self.parts[fd];part.extend(b)
            for _ in range(4096):
                if b"\n" not in part: break
                at=part.index(10)+1;line=bytes(part[:at]);del part[:at]
                assert len(line)<=MAX_LINE,"FIXTURE: line budget"
                v=json.loads(line);self.count+=1
                assert isinstance(v,dict) and self.count<=MAX_EVENTS,"FIXTURE: event inventory"
                if v["event"]=="missing_producer":
                    raise AssertionError(f"MISSING_PRODUCER {v['boundary']}; wire bodies UNEXECUTED")
                if v["event"]=="assertion": raise AssertionError(f"PRODUCT assertion: {v}")
                self.events.append(v)
            assert len(part)<=MAX_LINE,"FIXTURE: partial event bound"

    def wait(self, name, **fields):
        deadline=time.monotonic()+self.remaining()
        for _ in range(MAX_TURNS):
            for i,e in enumerate(self.events):
                if e["event"]==name and all(e.get(k)==v for k,v in fields.items()):
                    return self.events.pop(i)
            if time.monotonic()>=deadline: break
            if not self.parts: raise AssertionError(f"SETUP/FIXTURE: child exited {self.process.poll()} before {name}")
            self.pump()
        raise AssertionError(f"FIXTURE: missing {name}; timeout is not red")

    def check_maps(self,phase):
        maps=[e for e in self.events if e["event"]=="map" and e["phase"]==phase]
        assert 0<len(maps)<=256,"ADMISSION: complete nonempty reached-phase maps"
        seen=set()
        for e in maps:
            words=e["line"].split(maxsplit=5)
            if len(words)<6 or words[5].startswith("["): continue
            p=words[5]
            assert p.startswith("/") and " (deleted)" not in p,"ADMISSION: unresolved loaded map"
            assert p in self.seal,"ADMISSION: unadmitted loaded file "+p
            seen.add(p)
        for p in seen: assert sha(p)==self.seal[p],"ADMISSION: mapped seal changed"
        self.events[:]=[e for e in self.events if not (e["event"]=="map" and e["phase"]==phase)]

    def start(self):
        e=self.wait("listeners");self.check_maps("serving")
        assert 0<e["http"]<65536
        if e["native"]: assert e["native"]!=e["http"]
        self.ports=e;return self

    def connect(self,native=False):
        assert len(self.sockets)<MAX_PEERS,"FIXTURE: peer budget"
        s=socket.create_connection(("127.0.0.1",self.ports["native" if native else "http"]),timeout=self.remaining())
        s.settimeout(self.remaining());self.sockets.append(s);return s

    def send(self,s,data):
        assert len(data)<=MAX_WIRE,"FIXTURE: payload bound"
        # Bounded writes; no assumption about TCP packetization follows.
        for at in range(0,len(data),8192): s.sendall(data[at:at+8192])

    def read(self,s,n):
        assert n<=MAX_WIRE
        out=bytearray()
        for _ in range(MAX_TURNS):
            if len(out)==n: return bytes(out)
            s.settimeout(self.remaining());b=s.recv(min(8192,n-len(out)))
            if not b: raise AssertionError(f"PRODUCT: early EOF {len(out)}/{n}")
            out.extend(b)
        raise AssertionError("FIXTURE: read iterations")

    def eof(self,s):
        s.settimeout(self.remaining())
        try: b=s.recv(1)
        except ConnectionResetError: b=b""
        assert b==b"","PRODUCT: expected EOF with no additional response"

    def command(self,op,**fields):
        self.commands+=1; assert self.commands<=MAX_COMMANDS
        data=json.dumps(dict(op=op,**fields),separators=(",",":")).encode()+b"\n"
        assert len(data)<=513
        self.process.stdin.write(data)
        if op!="stop": self.wait("ack",op=op)

    def inspect(self):
        self.command("inspect");return self.wait("inspection")

    def stop(self):
        for s in self.sockets: s.close()
        self.sockets.clear()
        self.command("stop",ns=6000000000)
        self.wait("stop_drained")
        self.wait("framework_joined");self.wait("maps_end",phase="cleanup");self.check_maps("cleanup")
        self.wait("cleanup_complete")
        assert self.process.wait(timeout=REAP_SECONDS)==0
        self.closed=True
        self.cleanup()

    def cleanup(self):
        if self.cleaned: return
        self.cleaned=True
        failure=None
        if self.process:
            if self.process.poll() is None:
                self.process.terminate()
                try: self.process.wait(timeout=REAP_SECONDS)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    try: self.process.wait(timeout=REAP_SECONDS)
                    except subprocess.TimeoutExpired: failure="FIXTURE: unreaped child"
            for s in self.sockets: s.close()
            for f in (self.process.stdin,self.process.stdout,self.process.stderr): f.close()
        # Never traverse arbitrary /tmp: only this exclusively created owned root.
        entries=list(self.root.rglob("*"))
        assert len(entries)<=264,"FIXTURE: scratch inventory exceeds 260 dirs +4 canaries"
        assert all(not p.is_symlink() for p in entries),"FIXTURE: unexpected scratch symlink"
        assert sum(p.stat().st_size for p in entries if p.is_file())<=64,"FIXTURE: persistent output/spill"
        shutil.rmtree(self.root)
        if ADMISSION is not None:
            assert {p:sha(p) for p in self.seal}==self.seal,"ADMISSION: seal changed after child"
        self.events.clear();self.parts.clear();self.sockets.clear();self.seal.clear()
        if failure: raise AssertionError(failure)


class FoundationTests(unittest.TestCase):
    def child(self,**kw): return Child(self,mode="feature",**kw).start()

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
        p=self.child(action="full-receive-block");s=p.connect();p.wait("connected_held")
        p.send(s,F01+b"x"*65484);p.command("release-connected")
        self.assertEqual(p.wait("latch",slot=0)["returned"],65536,"FIXTURE: actual full receive not witnessed")
        p.wait("request");p.command("suspend");x=p.inspect();self.assertEqual(x["requests"],1)
        self.assertEqual(x["stock_batches"],0);p.command("close");p.eof(s);p.stop()

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
                               ("drop-reply",False),("cancel-before",False),("throw-request",False),("throw-read",False)):
            p=self.child(action=action);s=p.connect();p.send(s,F01)
            if success: self.assertEqual(p.read(s,66),DIAG)
            else: p.eof(s)
            x=p.inspect();self.assertEqual(x["requests"],0 if action=="throw-read" else 1);p.stop()

    def test_wrong_thread_void_terminates(self):
        for action in ("wrong-thread-raw","wrong-thread-cancel"):
            p=self.child(action=action);s=p.connect();p.send(s,F01)
            code=p.process.wait(timeout=STEP_SECONDS)
            self.assertIn(code,(-6,134),"void wrong-thread must terminate, setup exit is not red")
            p.closed=True;p.cleanup()

    def test_truncated_body_eof(self):
        p=self.child();s=p.connect();p.send(s,F01[:-1]);s.shutdown(socket.SHUT_WR);p.eof(s)
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
        p.send(s,F01[:32]);p.command("clock",ns=6099000000);p.send(s,F01[32:40])
        p.command("clock",ns=6100000000);p.eof(s);self.assertEqual(p.inspect()["requests"],0);p.stop()

    def test_stop_first_deadline(self):
        for repeat in (6500000000,5500000000):
            p=self.child(action="large-output");s=p.connect()
            p.command("clock",ns=2000000000);p.send(s,F01);p.wait("request")
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
            p.wait("cleanup_complete");self.assertEqual(p.process.wait(timeout=REAP_SECONDS),0);p.closed=True;p.cleanup()

    def test_native_retention_blocks_stop_drain(self):
        for action in ("strong","weak","lease"):
            p=self.child(profile="p2",action=action);s=p.connect(True);p.send(s,UPGRADE)
            self.assertEqual(p.read(s,129),SWITCH);p.wait("opened")
            if action=="lease": p.send(s,BINARY_A);p.wait("message")
            p.command("close");p.wait("closed");p.command("drop")
            p.command("stop",ns=6000000000);p.command("clock",ns=6000000000)
            x=p.inspect();self.assertEqual(x["sdk_retained_peers"],1)
            self.assertFalse(any(e["event"]=="stop_drained" for e in p.events))
            p.command("foreign-release");p.wait("removed");p.wait("stop_drained")
            p.wait("framework_joined");p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
            p.wait("cleanup_complete");self.assertEqual(p.process.wait(timeout=REAP_SECONDS),0);p.closed=True;p.cleanup()

    def test_native_lifetime_strong_weak_lease(self):
        for action in ("strong","weak","lease","facade-negative"):
            p=self.child(profile="p2",action=action);s=p.connect(True);p.send(s,UPGRADE)
            self.assertEqual(p.read(s,129),SWITCH);p.wait("opened")
            if action=="lease": p.send(s,BINARY_A);p.wait("message")
            if action=="facade-negative": p.command("facade-negative")
            p.command("close");p.wait("closed")
            p.command("drop") # native peers only receive Peer+Wake; no fabricated Request/Reply
            x=p.inspect();self.assertEqual(x["sdk_retained_peers"],1)
            self.assertEqual(x["peers"],1)
            # Backlog connect can succeed; no extra native construction is promised.
            spare=p.connect(True);self.assertEqual(p.inspect()["constructed"],x["constructed"])
            if action=="facade-negative": p.command("facade-negative")
            p.command("foreign-release");p.wait("removed");spare.close();p.stop()

    def test_native_read_ahead_assignment(self):
        for profile,accepted in (("reference",True),("tiny",False),("p2",False)):
            # Up to eight complete child attempts; only a witnessed same-read is a valid case.
            witnessed=False
            for attempt in range(8):
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
        c=p.connect();y=p.inspect();self.assertEqual(y["constructed"],2)
        self.assertEqual(y["accept_attempts"],x["accept_attempts"])
        self.assertEqual(y["transient_fds"],0);c.close();a.close();b.close();p.stop()

    def test_reference_http_quota80(self):
        p=self.child()
        for i in range(80): p.connect();p.wait("connected",peer=i)
        x=p.inspect();p.connect();y=p.inspect()
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
        p.send(peers[8],F01);x=p.inspect()
        self.assertEqual((x["input_slots"],x["input_capacity"],x["requests"]),(8,8388608,8))
        self.assertEqual(x["bytes_read"],416)
        p.command("close",peer=0);p.wait("request",peer=8)
        self.assertLessEqual(p.inspect()["peak_transient_fds"],1);p.stop()

    def test_gate_cancel_releases_node_once(self):
        p=self.child(action="gate-cancel");s=p.connect();p.send(s,F01);p.wait("request")
        self.assertEqual(p.inspect()["held_application_wire"],1)
        p.command("pressure-cancel");p.eof(s);p.wait("ended",reason=2)
        x=p.inspect();self.assertEqual((x["output_slots"],x["wire_releases"]),(0,1));p.stop()
        p=self.child(action="gate-stop");s=p.connect();p.send(s,F01);p.wait("request")
        self.assertEqual(p.inspect()["held_application_wire"],1)
        p.command("stop",ns=6000000000);p.eof(s);p.wait("ended",reason=4)
        p.wait("stop_drained");p.wait("framework_joined")
        p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
        p.wait("cleanup_complete");self.assertEqual(p.process.wait(timeout=REAP_SECONDS),0);p.closed=True;p.cleanup()

    def test_zero_and_close_reply(self):
        for action,expected in (("zero-reply",b"HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n"),("reply-close",DIAG_CLOSE)):
            p=self.child(action=action);s=p.connect();p.send(s,F01)
            self.assertEqual(p.read(s,len(expected)),expected)
            if action=="reply-close": p.eof(s)
            p.stop()

    def test_configuration_checked_before_registration(self):
        for name in ("zero-duration","roles","output-count","duration-over","epoll-narrow","missing-source","duration-bound"):
            p=Child(self,mode="config",case=name)
            p.wait("config_pass");p.wait("maps_end",phase="cleanup");p.check_maps("cleanup")
            self.assertEqual(p.process.wait(timeout=REAP_SECONDS),0);p.closed=True;p.cleanup()

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
        self.assertEqual(p.process.wait(timeout=REAP_SECONDS),0);p.closed=True;p.cleanup()


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
        p.wait("cleanup_complete");self.assertEqual(p.process.wait(timeout=REAP_SECONDS),0);p.closed=True;p.cleanup()


def main():
    global PROBE,ADMISSION
    parser=argparse.ArgumentParser()
    parser.add_argument("--probe",type=Path,required=True)
    parser.add_argument("--admission",type=Path,required=True)
    parser.add_argument("selectors",nargs="*")
    args=parser.parse_args();PROBE=args.probe.resolve()
    assert args.admission.stat().st_size<=65536,"ADMISSION: manifest bound"
    ADMISSION=json.loads(args.admission.read_text())
    # This consumes coordinator-reviewed source/passive ELF/ABI/controlled-identity
    # admission. It cannot authenticate that review or manufacture it from booleans.
    assert ADMISSION["source_commit"] and ADMISSION["review_receipt"].startswith("https://")
    assert len(ADMISSION["files"])<=128 and str(PROBE) in ADMISSION["files"]
    assert set(ADMISSION["environment"])<=set(("PATH","LD_LIBRARY_PATH","LANG","LC_ALL","TZ"))
    suite=unittest.TestSuite()
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(LegacyControl))
    if args.selectors:
        assert len(args.selectors)==len(set(args.selectors))<=64
        for selector in args.selectors:
            assert selector.startswith("test_") and hasattr(FoundationTests,selector)
            suite.addTest(FoundationTests(selector))
    else: suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(FoundationTests))
    # A legacy failure is setup failure, not proof from a suite of refusals.
    result=unittest.TextTestRunner(verbosity=2,failfast=True).run(suite)
    raise SystemExit(not result.wasSuccessful())


if __name__=="__main__": main()
