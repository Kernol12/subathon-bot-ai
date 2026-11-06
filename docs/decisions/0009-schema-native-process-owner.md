# ADR-0009 — Own the schema worker lifetime inside one native call

Date: 6 November 2026. Status: proposed; different actual policy/contract review
required before dependent interfaces, tests, probes or implementation.
Authority: specification v0.45 §§17/18, API-01/02/03 and TEST-04;
[ADR-0008](0008-schema-validation-foundation.md), SCHEMATOOL-001.

## Problem and evidence

The existing Python implementation passes the unchanged public 64 methods and
ten focused methods locally and on both selected CI Python versions. This proves
functional behavior, not complete process ownership. Different source review
still finds post-birth/pre-PID-publication allocation, constructor deadline,
helper-thread start and before-close ownership gaps. The actual selected CPython
sources create a child before allocating its Python PID result; Python pidfd
creation has a similar result-allocation boundary. More traceback recovery does
not establish the required lifetime guarantee. Preserve these source findings,
valid worker reviews and all original red/pass artifacts.

[Allocation findings](../evidence/schematool-001-native-allocation-findings.md),
[planning review](../evidence/schematool-001-native-boundary-review.md) and the
[74-method migration map](../evidence/schematool-001-native-test-mapping.md)
establish the concrete reason for this private architecture change. They do not
already approve native code or the actual policy revision here.

## Proposed decision

Use one first-party C++20 process-transport library, called through Python stdlib
ctypes, solely for this offline developer tool. Reuse selected CMake/Ninja and
GoogleTest infrastructure. Add no third-party library, Qt link, application
runtime dependency, service, CLI option or validator backend. The real pinned
Python standards libraries and worker remain responsible for validation.

The [native process contract](../contracts/schema-validation-native-process-v1.md)
defines the sole fixed C ABI, artifact, integer mapping, path/configuration
admission, syscall ownership, signal/descriptor setup, protocol, deadlines and
negative-only test seam. Its reviewed revision, the process-test revision4 and
installation revision2 must agree before dependent work starts. The tools policy
exception is limited to this library and its reviewed tests; all other repository
Python tooling remains standard-library-only.

Prepare Python buffers and load/resolve the trusted artifact before native entry.
The callback-free native interval owns every configuration/pipe/null/PID/pidfd
resource without heap allocation and closes/reaps all of them before returning a
closed integer. Python result conversion, allocation or interruption can then
unwind only after resources have gone. No Python helper thread, Popen constructor,
traceback recovery or Python signal/mask change owns a worker in this design.

Select atomic clone3(CLONE_PIDFD | CLONE_CLEAR_SIGHAND), with no fork/pidfd_open
fallback. Actual kernel rollback, syscall/signal/ABI semantics and capability on
both positive CI hosts are admission gates. A kernel version string is not proof;
unsupported facilities or an absent native library are environment_unavailable,
never an intended feature red, skipped selected-host case or accepted validation.
The fixed trusted artifact is built explicitly; import/help never build or load it.

The ten-second absolute work deadline starts before the first native resource.
Only a result committed after full cleanup and before that deadline can be
accepted ordinarily. Expiry causes resource_limit and mandatory termination/reap;
non-real-time kernel delays may extend cleanup, never permit late acceptance or
return with a survivor. This explicitly reconciles the old private clock wording
with the existing public cleanup requirement; no public timeout option or limit
is changed. Genuine conflicts found in actual implementation must be reported.

## Alternatives

Retain the current Python owner: rejected for the concrete unresolved source
intervals above; existing scoped reviews and functional receipts remain valid.
Python proc/pidfd recovery still has allocation, identity, completeness and
deadline gates. It is not an approved fallback.

Qt QProcess: an existing selected library worth considering, but it would add
QtCore to the otherwise isolated developer entrypoint and still need actual
lifetime/allocation/signal proof. No Qt failure or measured superiority is claimed.
The narrow native transport reuses the selected C++ stack without Qt or a new
process library. It implements process ownership, not a standards engine.

## Review and execution gates

Keep this work inside focused draft SCHEMATOOL-001 PR77. The task is the bounded
offline wrapper implementation, with these explicit support assignments rather
than unrelated product features or a new epic:

1. Root authors policy, installation and process-test reconciliation; B authors
   the native contract; C independently reviews their actual revisions. No native
   executable work is released before all these documents agree and are approved.
2. A authors the bounded native foundation and behavior test contracts/source;
   B may author only separately assigned minimal interfaces, explicit inert stub
   and nonvalidating harness. Root owns root CMake/presets/CI/shared state. Different
   actual interface/test review precedes execution. Real loader/ABI/kernel/build
   foundations must pass before intended native behavior red; a dependency,
   compile, facility, readiness or fixture failure is not red.
3. Only after reviewed tests and intended red may B implement the native owner and
   Python binding. C separately reviews actual source/call graph/resource bounds;
   root integrates and runs the full preserved/migrated acceptance and regression
   checks and unconditional CI on both selected hosts before normal merge gates.

Each assignment records exact revisions, owned paths and its gate. Support branches
and independent notes integrate into the one task PR; none is a duplicate merge
request or permission to mix unrelated work. Existing task IDs/dependencies remain.
If a support assignment cannot remain executable and bounded, split it explicitly
before authoring rather than treating this ADR as permission for an oversized task.

All 74 methods and every literal protocol/setup/deadline/cleanup assertion remain
accounted. Mechanism ports need their own different source/oracle review and new
intended red; original artifacts are immutable. Negative fixtures run the same
native algorithm and real fault children, never fake successful validation. Ordinary
schema/domain/SDK cases run the real production transport, worker and pinned SDK.

## Consequences and verification

A local native build becomes an explicit developer installation prerequisite.
Its missing-artifact refusal must not be mistaken for dependency_unavailable or a
passing foundation. Production exports only the versioned ABI; no resource-owning
global constructor or successful injected validator is admitted. Exact artifact
provenance, no-allocation call graph, child async-safety and actual host observations
remain separate evidence requirements beyond green functional outcomes.

No public call/enum/CLI, package/ref/example semantics, 512 MiB worker ceiling,
offline/metadata guarantees, supported product platform or release requirement is
reduced. All seven integrations, three exchange providers, both renderers,
locales/themes, security/recovery/packaging/live/platform gates remain. CONTRACT013
still waits for the complete reviewed and merged SCHEMATOOL-001 implementation.

This revision authors documents only. Current Python production source and tests
stay frozen. Independent review status and later observed commands/results belong
in task evidence and progress, not inferred from the proposal's wording.
