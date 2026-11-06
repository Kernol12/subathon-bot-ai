# Module agent instructions

Read the root AGENTS.md first.

Python >=3.10 standard-library developer tools. Keep HTML offline/self-contained, escape all editable text, validate local relative links and task dependency DAGs. Progress data must remain truthful. Tooling tests do not count as product acceptance or live-provider verification.

The dedicated schema-validation tool has an explicit exception under ADR-0008:
reviewed, fully pinned standards libraries may be used in its isolated developer
environment. Keep existing progress/repository tools standard-library-only. Review
all direct/transitive versions, hashes, licenses and real offline foundations before
admitting executable validator behavior; tests/red and implementation gates remain.
Do not install these dependencies globally or add them to the server/client runtime.
