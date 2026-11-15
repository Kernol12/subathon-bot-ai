# Module agent instructions

Read the root AGENTS.md first.

Qt application services, authoritative command queue, identity/permissions and Qt SQL persistence. Drogon threads bridge to the Qt queue; they never become independent state writers. Version interfaces and migrations before dependent agents. The development walking skeleton in `dev/` hosts the reference path; the production server host remains unimplemented.
