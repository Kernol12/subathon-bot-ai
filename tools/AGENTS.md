# Module agent instructions

Read the root AGENTS.md first.

Python >=3.10 standard-library developer tools. Keep HTML offline/self-contained, escape all editable text, validate local relative links and task dependency DAGs. Progress data must remain truthful. Tooling tests do not count as product acceptance or live-provider verification.

The dedicated schema-validation tool has an explicit exception under ADR-0008:
reviewed, fully pinned standards libraries may be used in its isolated developer
environment. Keep existing progress/repository tools standard-library-only. Review
all direct/transitive versions, hashes, licenses and real offline foundations before
admitting executable validator behavior; tests/red and implementation gates remain.
Do not install these dependencies globally or add them to the server/client runtime.

For the same offline schema tool only, independently reviewed
[ADR-0009](../docs/decisions/0009-schema-native-process-owner.md) defines a narrow
first-party C++20 transport exception, using existing CMake/Ninja/GoogleTest and
Python stdlib ctypes. It adds no third-party process library or Qt/server/client
runtime dependency. The actual ADR, native/process-test/installation contracts
and oracle map require different review before native interface/test/probe/code
work; actual C48c07527/C44adb36c document reviews establish that authoring gate,
not implementation or execution approval. Preserve test review → intended
red → implementation → separate source review, real both-host facility foundations,
all public guarantees and every existing assertion. Other repository/progress
tools remain Python standard-library-only. Native build/loading is explicit and
must not occur during import/help. Owned resources must end inside one callback-free
native call before Python result conversion or unwinding.
