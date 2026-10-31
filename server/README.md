# Server module

Qt application services, authoritative command queue, identity/permissions and Qt SQL persistence. Drogon threads bridge to the Qt queue; they never become independent state writers. Version interfaces and migrations before dependent agents. No server application is implemented yet.

ACCT-001 implements the framework-free identified-receipt service behind an atomic
repository port. Its15 reviewed cases pass with a primitive disposable store and
separate code review/automatic simulation CI; see [handoff](../docs/evidence/acct-001-handoff.md).
No SQL adapter, live connector, Qt writer ownership or server application is supplied
by this helper. Follow progress/state.json and the full specification for those gates.
