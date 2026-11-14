# Server module

Qt application services, authoritative command queue, identity/permissions and Qt SQL persistence. Drogon threads bridge to the Qt queue; they never become independent state writers. Version interfaces and migrations before dependent agents. No server application is implemented yet.

ACCT-001 implements the framework-free identified-receipt service behind an atomic
repository port. Its15 reviewed cases pass with a primitive disposable store and
separate code review/automatic simulation CI; see [handoff](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/acct-001-handoff.md).
No SQL adapter, live connector, Qt writer ownership or server application is supplied
by this helper. Follow GitHub Issues/Projects (after the final protected MIGRATE-004 cutover merge) and the full specification for those gates.
