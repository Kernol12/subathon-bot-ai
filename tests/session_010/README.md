# SESSION-010 ordinary edge capture producer

These two GoogleTest bodies are test-only EDGE-02 controls. Run exactly one
selector per fresh process, after independent admission of that executable.
The coordinator owns the client, stock edge, namespaces, process/socket
correlation and runtime matrix. No CTest registration is provided here.

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

## Coordinator synchronization

- Read `SESSION010_READY <selector> http=127.0.0.1:<port> native=127.0.0.1:<port>`
  from stdout. These endpoints come from the SDK listener readback in
  registration order and the report follows the IO0 setup callback. Set the
  fixed edge upstream to the ordinary HTTP port. Do not use a backend
  liveness connection, since this control consumes exactly one peer.
- Supply one frozen positive through the stock edge, using a fresh backend
  connection. Read `SESSION010_CAPTURE <selector> listener=<id> connection=<id>
  sequence=<n> peer=127.0.0.1:<port> fields=<n> projection=<n> ordered=1`.
  The lease supplies all three IDs and the backend peer IP and ephemeral port.
  The report follows assignment of the one peer context, exact synchronous
  capture, reply commit and raw release request. It carries no header values
  or prepared credential material. The selector and tuple correlate without
  adding a correlation header to the frozen packet.
- Independently match the backend socket and namespace/UID/process custody to
  the actual Caddy connection using that tuple. Match the client's actual TLS
  exchange and fixed HTTP 200 body `diag`. Once the response and observations
  are complete, write the single byte `S` to the producer's stdin pipe.
  The ordinary reply keeps the backend connection open for observation.
  Require `SESSION010_FINISHED <selector>`, normal exit and one passing test.
  The fixture checks one drained request, one raw release, one wire release
  and the complete input byte count after the drain/join/finish sequence.
- The listener, capture and completion waits are respectively bounded by
  7, 12 and 5 seconds. Native shutdown waits at most 7 seconds for the
  drain and framework return, within the unconditional 30-second fixture
  watchdog. Any timeout, additional peer/dispatch, report mismatch or abnormal
  exit is a fixture failure.

The native listener ID belongs to one successful Foundation registration. It
is not a global identity, a socket proof of Caddy ownership, or a policy
generation. The fixture checks the native artifact/profile identity against
configured data. Independent passive and loaded-object admission still binds
the binary and actual SDK. The existing default logger scanner checks its
own bounded INFO control channel only. It does not observe pre-main or
direct-write sinks, the synthetic credential on every sink, or Caddy logs.

This endpoint supplies a diagnostic response. It has no live API host or
installation, 94A readiness acknowledgment, digest preparation, policy pin,
session lookup or authenticated route. Stock edge configuration and actual
TLS are coordinator evidence. These two positives do not qualify refusal,
logging, resource, reload or later scope-to-queue gates.
