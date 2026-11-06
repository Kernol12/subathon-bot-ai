# SCHEMATOOL-001 — native boundary proposal and oracle-map review

Reviewer C (`/root/session_plan_review`), different from proposal author B
(`/root/session_supplier_author`) and mapping author A
(`/root/session_supplier_review`),6 November2026. **APPROVED DOCUMENT PLANNING
ONLY**, for the next actual policy/private-contract authoring gate. No native
interface/stub, probe/test, fixture port, implementation or supported capability is
approved by this verdict. Current Python source/lifetime gates remain OPEN.

Actual proposal46a464aa1621f6eee3fceceb5cb3312a746dcdb4, blob
`4240b17b5cd66244773e14e2df584856840b00f8`; actual mappingbe753ee, blob
`96130822fef8d47f884ed43800cca3c3a41678b5`. Root imports89a2799/d3d6c94 integrate
both documents. NEW isolated branch `task/SCHEMATOOL-001-native-boundary-review`,
worktree `/tmp/subathon-schematool001-native-boundary-review`, starts at
`d3d6c94be2c3fa2e586d6eab24b3e6a442066c55`. ONLY this evidence note changes.

## Authority and scope reconciliation

Read actual root/tools AGENTS, ADR0008, SCHEMATOOL001 brief, accepted logical/API/
context/installation/private-r3 documents, root CMake/presets/options/GoogleTest
selection and workflow boundaries. tools/AGENTS currently selects Python>=3.10
stdlib developer tools with a narrow isolated standards-library exception. That
text does NOT already admit a project native process library. Private-r3 expressly
requires production module-Popen/os.pipe/Python-clock mechanisms. Moving them is a
REAL written architecture/private-test change, not a conforming edit under r3.

No owner-authored product requirement found mandates Python-only process ownership
or Popen. The user's standing execution/coordination authority, selected first-party
C++20/CMake/POSIX stack and routine implementation discretion support a narrow
reviewable developer-only decision without an automatic owner question. Permission
is not inferred from an exception locally written by an agent. The actual root-
owned tools/ADR/private/API-installation/test-plan revisions MUST explicitly resolve
the conflicts and obtain different review before dependent code/tests. An actual
public-behavior/limit/support/dependency-policy conflict would still need resolution;
this verdict supplies no blanket permission to rewrite instructions or the product.

The proposal preserves exact public18 outcomes/calls/CLI/no-callback inputs, worker
512MiB/no-site/ABI/SDK admissions, all schema/context/default/example semantics and
release scope. It adds a local native developer artifact/capability prerequisite,
not Qt/server/client linkage or an unreviewed third-party selection. Existing
CMake>=3.25/C++20/Ninja and selected GoogleTest route are available infrastructure;
their existence is not a new native build/host observation. Import/help without
native loading/building and truthful missing-artifact refusal remain required.

## Feasible candidate, with concrete unproved gates

The callback-free single C ABI interval is a coherent way to move the problematic
resource lifetime: bounded path buffers/prototype/loader allocations occur BEFORE
entry, native integer/PID/FD slots own resources without Python boxing, and ALL
resources end before integer return conversion/Python unwinding. The fixed enum
mapping distinguishes ABI values from CLI exits, refuses unknown values, reserves
private18 for interruption without inventing KeyboardInterrupt, and gates any named
31 stub marker separately. Fixed32KiB scratch/noexcept/RAII are requirements to
prove, not evidence merely from C++ names.

Primary [ctypes documentation](https://docs.python.org/3.10/library/ctypes.html#loading-shared-libraries)
supports CDLL's GIL release and PyDLL distinction. Actual
[selected ctypes call source](https://raw.githubusercontent.com/python/cpython/v3.14.9/Modules/_ctypes/callproc.c)
places argument conversion before ffi_call and result conversion afterward. No
Python API, custom converter/restype, errcheck, callback or Python-backed fault
driver may run while native resources are owned. Pending SIGINT must propagate
only after cleanup; native EINTR/deadline/wait behavior and actual selected3.10/
3.12/3.14 boundaries remain required observations, not a promise from ctypes alone.

Atomic clone3(CLONE_PIDFD), with fork-like private address/FD state and SIGCHLD,
is a source-backed candidate for eliminating post-birth Python PID allocation.
[clone3/CLONE_PIDFD documentation](https://man7.org/linux/man-pages/man2/clone.2.html)
defines the parent pidfd output; [kernel6.12 fork source](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/fork.c)
contains explicit pidfd reservation/unwind machinery. This does NOT establish
behavior on either selected CI kernel/seccomp policy. The proposal correctly
selects no silent fork+pidfd_open fallback and keeps positive-host capability proof
mandatory. Another raw atomic primitive would require its own actual review.

Fixed child descriptor3/4/stdio remapping, unrelated-FD closure and an exec-only
child path are coherent with the frozen worker interface. The
[async-signal-safe restriction](https://man7.org/linux/man-pages/man7/signal-safety.7.html)
and actual generated call graph must govern every pre-exec operation; no Python,
C++ allocation/logging/destructor/loader callback is allowed there. Eager binding
is necessary but is not a complete async-safety proof. The stated10s work deadline
must reject late ordinary success while still completing mandatory termination/
reap; non-real-time kernel delays cannot justify premature return or a hidden
longer acceptance interval. This distinction needs exact private wording.

Before any code/test release, actual contract authoring must freeze:

- Exact ABI/artifact/version/path admission, no resource-owning ELF/global
  constructors before entry, fixed arguments/statuses and no retained request state.
- Atomic failure cleanup and native handle slots, including every pipe, pidfd and
  null/config descriptor, not merely the original four protocol endpoints.
- Child signal dispositions/mask/exec inheritance, syscall-only safety and closed
  0/1/2/unrelated-FD cases, with no parent-global handler/mask mutation.
- SIGCHLD/no-autoreap admission; same-birth-thread waitid/P_PIDFD options; pinned
  exit PLUS definitive own/other-waiter reap proof. ECHILD/ESRCH alone cannot prove
  cleanup, and numeric PID discovery must never authorize unsafe reused-PID kill.
- Absolute work clock before birth, EINTR/cancellation/timeout/protocol cleanup,
  mandatory reap/close and precise known-resource/setup/unclassified taxonomy.
- Actual both-host facility/ABI/build/loader foundations, negative-only SAME-core
  instrumentation and meaningful source/allocator/call-graph audits.

The proposal already records these as candidate/future proof areas; this list is
their concrete review boundary, not approval of unspecified implementation details.
The Qt QProcess alternative is considered explicitly without inventing a Qt failure
or claiming measured superiority. No standards engine or native validator is added.

## Exact existing-oracle preservation

Independent stdlib AST/table comparison verifies EXACTLY64 unique public method
rows plus cleanup2/context3/lexical3/spawn2: ALL74 existing methods are present once,
with no omitted name. Source blobs64f4911/acde40d/b34a564/fc826843/777d907 match.
The source-counted327 observations are not represented as327 registered/native
tests. U/G/P separates untouched literal behavior, strengthened no-call guards and
mechanisms needing actual port review. It grants no right to skip/retire assertions.

All protocol rows, narrow setup refusal, unknown-signal classification, actual
readiness/FD/reap-before-fallback, deadline, known allocation and cancellation
oracles remain explicit. Existing Python patches becoming unreachable must not
produce vacuous green. Native fault workers stay real negative children; ordinary
domain/SDK checks use the real production owner/worker/backends. Actual native
ledgers, zombie control and allocator boundaries must prove resources, not return
a claimed clean boolean. Python exception identity is preserved when observable;
any changed native signal boundary needs written reason and complementary actual
coverage, not a silently dropped assertion.

Mapping correctly retains outer real-script launches and genuine missing/corrupted
SDK clone controls, adding native-artifact admission instead of misclassifying a
missing library as dependency failure. Private context fixtures must also gain real
public integration. All stable worker/context/lexical assertions and SDK14 remain;
the current Python full64+10 functional pass retains its exact separate scope.

Proposed task split is bounded: actual contracts/policy → reviewed inert native
interface/harness/tests → actual green build/facilities → intended new native red
→ native owner/binding implementation and independent review → complete matching
integration/CI. Root still owns real task IDs/claims/shared build/dependency/state.
Missing library/kernel/GTest/readiness or skipped cases are not intended red.
Original FIRST63/261/new1/10 and focused reds remain historical unchanged evidence.

Actual checks: immutable document/source reads, exact74-name AST/table equality,
source/CI equality to functional checkpoint0f8, project426/122/current render and
whitespace. No native/Python candidate/test/helper/worker/SDK import or execution,
compile/build, facility probe, child/thread/signal, install, historical replay,
live operation or GitHub write occurred. Different actual concrete contract/test/
source reviews and all real native/hosted gates remain before completion/merge.
