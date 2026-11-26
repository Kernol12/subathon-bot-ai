#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""SESSION-010 (#588) edge capture driver: one selector of session_010_edge_capture
behind the pinned stock Caddy edge, on plain host loopback.

The driver starts the producer, reads its SDK listener report, starts the pinned
Caddy with a generated TLS edge configuration in front of the producer's HTTP
listener, sends the one frozen account-ingress request over verified TLS 1.3 /
HTTP/1.1 and checks the three report lines. The backend peer the producer reports
must be Caddy's actual upstream socket, found through /proc; a mismatch fails.

Under ADR-0014 and advisor 588.3 the run uses ephemeral loopback ports and no
namespaces. Nothing here prints request bytes, child output or key material, and
the working directory (disposable PKI, configuration, child logs) is deleted.
"""
import argparse
import hashlib
import http.client
import json
import os
from pathlib import Path
import queue
import re
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
import time

CADDY_SHA256 = '4ef1f68c70219536b2711fd16547a79841a2dec2d6b4e56b1e3e5e9da76028e6'  # 2.10.2
ADDRESS = '127.0.0.1'
HEX_LOOPBACK = '0100007F'
HOST = 'timer.example'
# Frozen external request bytes, native leased field count, native projection.
PROJECTION = {'Desktop252': (149, 6, 252), 'Browser282': (179, 7, 282)}
LINE_LIMIT = 4096  # producer stdout lines kept for matching; never printed


class Failure(Exception):
    pass


def require(condition, message):
    if not condition:
        raise Failure(message)


def frozen_request(case):
    """The R2 desktop149 / browser179 positives with an all-zero synthetic credential."""
    prefix = b'GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n'
    if case == 'Desktop252':
        packet = prefix + b'Authorization: Bearer ' + b'0' * 64 + b'\r\n\r\n'
    else:
        packet = (prefix + b'Cookie: __Host-subathon_session=' + b'0' * 64 +
                  b'\r\nX-Subathon-CSRF: 1\r\n\r\n')
    require(len(packet) == PROJECTION[case][0], 'frozen request size')
    return packet


class Child:
    """A child process whose stdout lines are queued; stderr goes to a private file."""

    def __init__(self, command, env, work, name, stdin=subprocess.DEVNULL):
        self.stderr = open(work / (name + '.stderr'), 'wb')
        self.proc = subprocess.Popen(command, env=env, cwd=work, stdin=stdin,
                                     stdout=subprocess.PIPE, stderr=self.stderr)
        self.lines = queue.Queue()
        self.seen = []
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

    def _read(self):
        for raw in self.proc.stdout:
            line = raw.decode('ascii', 'replace').rstrip('\n')
            if len(self.seen) < LINE_LIMIT:
                self.seen.append(line)
            self.lines.put(line)
        self.lines.put(None)

    def until(self, prefix, seconds):
        deadline = time.monotonic() + seconds
        while True:
            left = deadline - time.monotonic()
            require(left > 0, prefix + ' not reported in time')
            try:
                line = self.lines.get(timeout=left)
            except queue.Empty:
                continue
            require(line is not None, prefix + ' not reported before exit')
            if line.startswith(prefix + ' '):
                return line
            # Report lines come in exactly this order; any other one is a mismatch.
            require(not line.startswith('SESSION010_'), prefix + ' expected next')

    def stop(self):
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=5)
        self.reader.join(timeout=2)
        self.stderr.close()


def run(command, env, work, name, seconds=5):
    with open(work / (name + '.log'), 'wb') as log:
        status = subprocess.run(command, env=env, cwd=work, stdin=subprocess.DEVNULL,
                                stdout=log, stderr=subprocess.STDOUT, timeout=seconds).returncode
    require(status == 0, name + ' failed')


def private_pki(work):
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'OPENSSL_CONF': '/dev/null'}
    openssl = '/usr/bin/openssl'
    run([openssl, 'req', '-x509', '-newkey', 'ed25519', '-noenc', '-subj',
         '/CN=Session 010 fixture CA', '-days', '2', '-addext', 'basicConstraints=critical,CA:TRUE',
         '-addext', 'keyUsage=critical,keyCertSign,cRLSign', '-keyout', 'ca.key.pem',
         '-out', 'ca.crt.pem'], env, work, 'ca')
    run([openssl, 'req', '-new', '-newkey', 'ed25519', '-noenc', '-subj', '/CN=' + HOST,
         '-keyout', 'leaf.key.pem', '-out', 'leaf.csr.pem'], env, work, 'csr')
    (work / 'leaf.ext').write_text('subjectAltName=DNS:' + HOST + '\nextendedKeyUsage=serverAuth\n')
    run([openssl, 'x509', '-req', '-in', 'leaf.csr.pem', '-CA', 'ca.crt.pem', '-CAkey',
         'ca.key.pem', '-set_serial', '1', '-days', '1', '-extfile', 'leaf.ext',
         '-out', 'leaf.crt.pem'], env, work, 'leaf')
    run([openssl, 'verify', '-CAfile', 'ca.crt.pem', 'leaf.crt.pem'], env, work, 'verify')


def edge_config(work, listen_port, upstream_port):
    """The reviewed candidate stock profile (h1-only TLS edge), on an ephemeral port."""
    def refuse(match):
        return {'match': match, 'handle': [{'handler': 'static_response', 'status_code': 400,
                'headers': {'Cache-Control': ['no-store']}, 'body': '', 'close': True}],
                'terminal': True}

    def expression(source):
        return {'expression': source}

    canonical_target = ("{http.request.orig_uri}.startsWith('/') && "
                        "{http.request.orig_uri} == {http.request.uri}")
    canonical_authority = (r"{http.request.hostport}.matches(r'^[\x21-\x7e]+$') && "
                           r"{http.request.hostport}.matches(r'(?i)^timer\.example(:443)?$')")
    ordinary_connection = (r"{http.request.header.Connection}.matches(r'(?i)^[ \t]*(?:(?:close|"
                           r"keep-alive)[ \t]*(?:,[ \t]*(?:close|keep-alive)[ \t]*)*)?$')")
    routes = [
        refuse([expression("{http.request.proto} != 'HTTP/1.1' || {http.request.scheme} != 'https'")]),
        refuse([expression(f'!({canonical_target})')]),
        refuse([expression(f'!({canonical_authority})')]),
        refuse([{'header_regexp': {name: {'pattern': '(?s).*'}}}
                for name in ('Transfer-Encoding', 'Upgrade', 'Trailer', 'TE', 'Proxy-Connection',
                             'Expect')]),
        refuse([expression(f'!({ordinary_connection})')]),
        {'handle': [
            {'handler': 'headers', 'request': {'delete': ['Forwarded', 'X-Forwarded-*']}},
            {'handler': 'request_body', 'max_size': 4096},
            {'handler': 'reverse_proxy', 'upstreams': [{'dial': f'{ADDRESS}:{upstream_port}'}],
             'transport': {'protocol': 'http', 'versions': ['1.1'], 'compression': False,
                           'dial_timeout': '3s', 'response_header_timeout': '15s',
                           'read_buffer_size': 4096, 'write_buffer_size': 4096,
                           'max_conns_per_host': 1, 'read_timeout': '15s', 'write_timeout': '15s',
                           'max_response_header_size': 2048}}],
         'terminal': True},
    ]
    discard = {'writer': {'output': 'discard'}}
    return {
        'admin': {'disabled': True, 'config': {'persist': False}},
        'logging': {'sink': discard, 'logs': {'default': discard}},
        'apps': {
            'http': {'servers': {'reference': {
                'listen': [f'{ADDRESS}:{listen_port}'], 'protocols': ['h1'],
                'automatic_https': {'disable': True}, 'listen_protocols': [['h1', 'h2']],
                'read_header_timeout': '10s', 'read_timeout': '15s', 'write_timeout': '15s',
                'idle_timeout': '30s', 'max_header_bytes': 32768,
                'tls_connection_policies': [{'alpn': ['http/1.1']}], 'routes': routes}}},
            'tls': {'certificates': {'load_files': [{'certificate': str(work / 'leaf.crt.pem'),
                                                     'key': str(work / 'leaf.key.pem')}]},
                    'disable_ocsp_stapling': True, 'disable_storage_clean': True},
        },
    }


def free_port():
    with socket.socket() as probe:
        probe.bind((ADDRESS, 0))
        return probe.getsockname()[1]


def owned_sockets(pid, state, local, remote=None):
    """TCP sockets of one process in `state` with the given loopback endpoints."""
    base = Path('/proc') / str(pid)
    inodes = set()
    for fd in (base / 'fd').iterdir():
        try:
            match = re.fullmatch(r'socket:\[(\d+)\]', os.readlink(fd))
        except FileNotFoundError:
            continue
        if match:
            inodes.add(match[1])
    found = []
    for line in (base / 'net/tcp').read_text().splitlines()[1:]:
        fields = line.split()
        if fields[9] not in inodes or fields[3] != state:
            continue
        if fields[1] != f'{HEX_LOOPBACK}:{local:04X}':
            continue
        if remote is not None and fields[2] != f'{HEX_LOOPBACK}:{remote:04X}':
            continue
        found.append(int(fields[9]))
    return found


def observe(child, state, local, remote=None, seconds=2):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline and child.proc.poll() is None:
        found = owned_sockets(child.proc.pid, state, local, remote)
        if found:
            require(len(found) == 1, 'ambiguous owned socket')
            return found[0]
        time.sleep(0.025)
    raise Failure('expected owned socket was not observed')


def exchange(work, case, edge_port):
    """One verified TLS 1.3 / HTTP/1.1 exchange of the frozen request; returns the open socket."""
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    context.load_verify_locations(cafile=work / 'ca.crt.pem')
    context.set_alpn_protocols(['http/1.1'])
    connection = socket.create_connection((ADDRESS, edge_port), timeout=5)
    peer = context.wrap_socket(connection, server_hostname=HOST)
    leaf = ssl.PEM_cert_to_DER_cert((work / 'leaf.crt.pem').read_text())
    require(peer.getpeercert(binary_form=True) == leaf, 'edge certificate')
    require(peer.version() == 'TLSv1.3' and peer.selected_alpn_protocol() == 'http/1.1',
            'TLS 1.3 with HTTP/1.1 ALPN')
    peer.sendall(frozen_request(case))
    response = http.client.HTTPResponse(peer)
    response.begin()
    require(response.status == 200 and response.version == 11 and response.read(4097) == b'diag',
            'edge response 200 diag')
    return peer


def drive(producer_path, caddy_path, case, work):
    selector = 'Session010EdgeCapture.' + case
    with open(caddy_path, 'rb') as binary:
        require(hashlib.sha256(binary.read()).hexdigest() == CADDY_SHA256, 'pinned Caddy 2.10.2')
    private_pki(work)
    for directory in ('tmp', 'home', 'data', 'config'):
        (work / directory).mkdir(mode=0o700)
    base = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(work / 'tmp')}
    if 'LD_LIBRARY_PATH' in os.environ:
        base['LD_LIBRARY_PATH'] = os.environ['LD_LIBRARY_PATH']
    producer = Child([producer_path, '--gtest_filter=' + selector, '--gtest_color=no'], base, work,
                     'producer', stdin=subprocess.PIPE)
    caddy = None
    client = None
    try:
        ready = re.fullmatch(r'SESSION010_READY (\S+) http=127\.0\.0\.1:(\d+) native=127\.0\.0\.1:(\d+)',
                             producer.until('SESSION010_READY', 7))
        require(ready and ready[1] == selector, 'listener report')
        http_port = int(ready[2])
        observe(producer, '0A', http_port)

        edge_port = free_port()
        config = work / 'caddy.json'
        config.write_text(json.dumps(edge_config(work, edge_port, http_port)))
        profile = dict(base, HOME=str(work / 'home'), XDG_DATA_HOME=str(work / 'data'),
                       XDG_CONFIG_HOME=str(work / 'config'))
        run([caddy_path, 'validate', '--config', str(config)], profile, work, 'validate')
        caddy = Child([caddy_path, 'run', '--config', str(config)], profile, work, 'caddy')
        observe(caddy, '0A', edge_port, seconds=5)

        client = exchange(work, case, edge_port)
        capture = re.fullmatch(r'SESSION010_CAPTURE (\S+) listener=(\d+) connection=(\d+) '
                               r'sequence=(\d+) peer=127\.0\.0\.1:(\d+) fields=(\d+) '
                               r'projection=(\d+) ordered=1', producer.until('SESSION010_CAPTURE', 12))
        require(capture and capture[1] == selector, 'capture report')
        listener, connection, _, peer_port, fields, projection = map(int, capture.groups()[1:])
        require(listener > 0 and connection > 0, 'lease identities')
        require((fields, projection) == PROJECTION[case][1:], 'exact native capture')
        # The reported backend peer must be Caddy's own upstream connection to the producer.
        observe(caddy, '01', peer_port, http_port)
        observe(producer, '01', http_port, peer_port)

        producer.proc.stdin.write(b'S')
        producer.proc.stdin.close()
        finished = producer.until('SESSION010_FINISHED', 7).split()
        require(finished[1:] == [selector], 'completion report')
        require(producer.proc.wait(timeout=10) == 0, 'producer exit')
        producer.reader.join(timeout=2)
        require(sum(line.startswith('[  PASSED  ] 1 test.') for line in producer.seen) == 1,
                'one passing test')
    finally:
        if client is not None:
            client.close()
        if caddy is not None:
            caddy.stop()
        producer.stop()
    require(caddy.proc.returncode == 0, 'edge exit')
    leftovers = [path for directory in ('tmp', 'home', 'data', 'config')
                 for path in (work / directory).rglob('*') if path.is_file()]
    require(not leftovers, 'profile and temporary storage stay empty')
    return fields, projection


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--producer', required=True)
    parser.add_argument('--caddy', required=True)
    parser.add_argument('--selector', required=True, choices=sorted(PROJECTION))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='session010-edge-') as directory:
        work = Path(directory)
        work.chmod(0o700)
        try:
            fields, projection = drive(args.producer, args.caddy, args.selector, work)
        except Exception as error:  # one line only: no values, traceback or child output
            reason = str(error) if isinstance(error, Failure) else type(error).__name__
            print(f'SESSION010_DRIVER FAIL {args.selector}: {reason}')
            return 1
    print(f'SESSION010_DRIVER PASS {args.selector} fields={fields} projection={projection}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
