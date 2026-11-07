# HTTP schemas

[Reference v1](reference-v1/manifest.json) defines only public negotiation and a
protected committed timer snapshot. Its [HTTP contract](../../docs/contracts/reference-http-v1.md)
and [review/validation evidence](../../docs/evidence/contract-013-handoff.md) distinguish
artifact acceptance from the required runtime/authentication/HTTP/SQLite gates.
The complete release HTTP API remains CONTRACT-006; no runtime capability is
announced by these files. Later contracts import this reviewed package.
