# SESSION-010 ordinary edge capture producer

These two GoogleTest bodies are test-only EDGE-02 controls in
`session_010_edge_capture`. Each runs as one selector per fresh process, started
by the committed driver `edge_capture_driver.py`. The driver also starts the pinned
stock edge and the TLS client. With `SUBATHON_SESSION010_CADDY` set to the pinned
Caddy 2.10.2 binary, the account-capture build registers
`session-010.edge-capture.{Desktop252,Browser282}` under the `session-010` label.

| Exact selector | Frozen external request | Exact capture |
| --- | --- | --- |
| `Session010EdgeCapture.Desktop252` | R2 desktop149 | 6 fields, 252 projected bytes |
| `Session010EdgeCapture.Browser282` | R2 browser179 | 7 fields, 282 projected bytes |

Both send one bodyless reference GET each. The browser control carries its
single Cookie and marker with no Origin. The oracle checks method, escaped
target, route index, empty body and the complete ordered occurrence vector
within the real native lease. It compares even field-name casing against this
specific stock wire projection. Cross-name order is a fixture observation.
The contract permits cross-name reordering in general. The oracle assumes
the stock proxy's forwarding and Via generation and the Go HTTP/1.1 writer.
[Pinned Caddy source](https://raw.githubusercontent.com/caddyserver/caddy/551f793700fe1550845c824470b623fd1aa03d36/modules/caddyhttp/reverseproxy/reverseproxy.go) ·
[Pinned Go request writer](https://raw.githubusercontent.com/golang/go/6e676ab2b809d46623acb5988248d95d1eb7939c/src/net/http/request.go)

## Driver

The driver keeps only what the oracle needs (ADR-0014; advisor 588.3). It uses plain
host loopback with ephemeral ports and no namespaces.

1. Check the Caddy binary against its pinned SHA-256.
2. Make a disposable Ed25519 CA and `timer.example` leaf with `openssl`.
3. Start the producer and read
   `SESSION010_READY <selector> http=127.0.0.1:<port> native=127.0.0.1:<port>`.
   These endpoints come from the SDK listener readback in registration order,
   and the report follows the IO0 setup callback. The driver checks that the
   producer owns the HTTP listener. It opens no backend liveness connection,
   since this control consumes exactly one peer.
4. Start Caddy with the reviewed candidate stock profile (h1-only TLS listener,
   `http/1.1` ALPN, the stock reverse proxy to the producer's HTTP port) on an
   ephemeral loopback port. The profile is validated first.
5. Send the frozen positive over verified TLS 1.3 and HTTP/1.1, and require
   HTTP 200 with body `diag`.
6. Read `SESSION010_CAPTURE <selector> listener=<id> connection=<id>
   sequence=<n> peer=127.0.0.1:<port> fields=<n> projection=<n> ordered=1`.
   The lease supplies all three IDs, plus the backend peer IP and ephemeral
   port. The report follows the exact synchronous capture, the reply commit and
   the raw release request. It carries no header values or prepared credential
   material. The reported peer must be Caddy's own upstream connection to the
   producer, matched in `/proc` on both ends. A mismatch fails the test.
7. Write `S` to the producer's stdin. Require `SESSION010_FINISHED <selector>`,
   a normal exit and one passing test. The fixture checks one drained request,
   one raw release, one wire release and the complete input byte count after
   the drain, join and finish sequence. Caddy must exit normally and leave its
   profile storage empty.

The listener, capture and completion waits are bounded by 7, 12 and 7 seconds.
The producer allows 5 seconds for the completion byte and keeps its own
unconditional 30-second watchdog. Any timeout, additional peer or dispatch,
report mismatch or abnormal exit fails the test.

The driver prints only its PASS or FAIL line. It never prints request bytes,
child output or key material, and it deletes its working directory. The
dropped namespace and UID custody was evidence for the retired local procedure.

The native listener ID belongs to one successful Foundation registration. It
is not a global identity, a socket proof of Caddy ownership, or a policy
generation. The fixture checks the native artifact/profile identity against
configured data. The existing default logger scanner checks its
own bounded INFO control channel only. It does not observe pre-main or
direct-write sinks, the synthetic credential on every sink, or Caddy logs.

This endpoint supplies a diagnostic response. It has no live API host or
installation, 94A readiness acknowledgment, digest preparation, policy pin,
session lookup or authenticated route. These two positives do not qualify refusal,
logging, resource, reload or later scope-to-queue gates.
