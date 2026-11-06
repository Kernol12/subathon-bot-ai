# SCHEMATOOL-001 — native foundation build authoring

Integrator/root, 6 November 2026. **Authored configuration, pending different
actual build/test/CI review; no configure, compile, load or native execution.**
This work is limited to finishing existing PR77, then stopping under the owner's
latest instruction. It does not resume the broader standing task-selection loop.

Authority is actual native contract aec16d6, independent C44adb36c document approval,
root policy877c41e and its different C48c07527 approval. B's exact ABI/stub22e0df4
is integrated481dc5c and independently source-approved Cc0b1f619 (root52ee9b7).
Those approvals do not approve this new build or A's future foundation source.

Root owns CMakeLists.txt, CMakePresets.json, tests/CMakeLists.txt, both new scoped
CMakeLists files, the sole-symbol version map and the native-foundation workflow.
A separately owns actual native/loader foundation tests and nonvalidating child
source. B separately authors a small direct-native behavior baseline, with its own
different test review and later intended red. No production owner is authorized.

The new opt-in SUBATHON_BUILD_SCHEMA_TRANSPORT defaults OFF. Preset
schema-transport-debug selects only this developer foundation plus existing default
scaffold; no Qt/server/client, standards-engine or third-party addition. Platform
and ELF-toolchain admission fails explicitly outside the selected Linux x86-64
GNU/Clang profile. Existing GoogleTest >=1.17 discovery/local-source/explicit pinned
fetch selection is reused unchanged; actual selected version must be recorded.

Target subathon_schema_transport_v1 builds the explicit inert stub, no exceptions
or RTTI, hidden visibility and eager/RELRO binding. A version map exports only the
fixed ABI symbol. Its artifact lands at the exact reviewed module-relative path
build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so. Building is
explicit; importing/help never triggers a build or load. Actual symbols, ELF
initializers/constructors and artifact hashes remain observed-review gates.

The independent GoogleTest executable links only the actual ABI stub, selected
GoogleTest and Threads, plus a bounded nonvalidating child target for actual exec
observations. The child target has eager binding and no C++ exceptions/RTTI.
Compile-time target paths are trusted build outputs, not package/public inputs.
Native case discovery uses the normal GoogleTest route; every foundation has the
explicit schema-native-foundation label and finite30-second timeout. A separate
stdlib loader script receives only --artifact ABS_TARGET_PATH under -I -S -B and
the exact minimal environment. Actual tests/oracles and inventories still require
different review before any command executes these programs.

The new workflow runs both Ubuntu22.04/CPython3.10 and Ubuntu24.04/3.12. It records
actual uname/compiler/CMake/Ninja/libc/Python, explicitly allows the existing pinned
GoogleTest fetch if needed, builds the exact target, records SHA256/exports and
unconditionally executes the reviewed foundation label. No missing syscall, skip,
readiness failure, watchdog or compile/dependency failure may be treated as green
capability or an intended feature red. Existing Python SDK/public/focused workflows
remain byte-identical and ordinary; this new label supplies no product acceptance.

Before publication/execution: integrate actual authored A source, add exact compiled
registration/no-disable/no-inversion guards, then obtain different whole-source/
build/CI review. Only after actual green foundations on BOTH selected hosts may
the reviewed direct-native missing-behavior baseline execute. No old stub red or
functional suite is replayed to substitute for that gate. Ported74 oracles, new
negative ownership fixtures, actual implementation/source audit/full integration,
latest-head CI/reviews/discussions and normal merge requirements remain.
