# SCHEMATOOL-001 native foundations — independent source/build review

Reviewer C (`session_plan_review`), 6 November 2026. This review is limited to
existing PR77, which the owner asked us to finish and then stop. A authored the
foundation source at `8e20a7682d26290e6d9f46cf5fc4bf3e1c5cf8b7`; root integrated
it as `0f54777` and authored the build/CI packet at
`29902d4`. I reviewed an isolated branch from that immutable integrated commit.
I authored neither the foundation source nor this build configuration.

## Verdict and exact scope

**Configure/build and static artifact inspection only are approved for the
reviewed opt-in build. Runtime source approval is withheld for the cleanup findings
below.** CMake `DISCOVERY_MODE PRE_TEST` leaves candidate discovery out of the
build. Root may configure/build and inspect generated objects, linked ELF files,
relocations, disassembly, symbols and artifact hashes without executing them.
This permission includes neither `ctest --show-only` nor native case listing:
PRE_TEST discovery executes the test executable on its first CTest invocation.
It also excludes loading the DSO, executing either Python test program, child or
kernel probes, behavior red, and publishing the current executable workflow.

The current workflow requires a separately reviewed, fail-closed generated-code
audit **before** its first inventory/discovery or probe. Root acknowledged that
gap and chose to author a deterministic per-host static audit; that future source
and its actual compiler artifacts are not approved by this note. Both selected
hosts must subsequently observe genuine green foundations before the separate
12-method/22-observation direct-native behavior baseline can reach its intended
red. No transport owner, complete native negative baseline, future binding,
production behavior, kernel capability or task completion is approved here.

## Reviewed identities

| Path | Exact Git blob at `29902d4` |
| --- | --- |
| `tests/schema_validation/native_foundation.cpp` | `43d71fbe8a5fa6097bfae4ca1784cd3a47445e93` |
| `tests/schema_validation/native_foundation_child.h` | `f1082aa839ab3bb2c22b408d4def63a54f4fc815` |
| `tests/schema_validation/native_foundation_child.cpp` | `c46e4d6639dce626ab4eea0f9456f9083604c648` |
| `tests/schema_validation/native_loader_foundation.py` | `b16a20cc09c83d82bb9ae8896d5973b573808620` |
| `tests/schema_validation/CMakeLists.txt` | `5b256e27b05e047f124356f6f8a643cb8a531200` |
| `tools/schema_validation/native/CMakeLists.txt` | `008fb80546bce06c9bfc9741b40347a781da0401` |
| `tools/schema_validation/native/transport_v1.map` | `a8c5c1f6cf1e428e47c67109998b0c95e85afefe` |
| `.github/workflows/schema-native-foundation.yml` | `48e65479070592b5489108180820d43530459bbc` |
| `CMakeLists.txt` | `5b9f5c66e2fd85426bc6d9698d86fa620c1d2a7c` |
| `CMakePresets.json` | `ac1b190e2ceac7b18f7d7da8ce6ce05b1d289b8b` |
| `tests/CMakeLists.txt` | `0db020840f5ac70da265ed039dacdd10cddca64a` |

The ABI header/stub remain the independently reviewed `04e8a7fe`/`e04d4c08`
literal-31 interface, not an owner implementation. Authority is specification
v0.45 §§17/18, API-01/02/03 and TEST-04; accepted public API `973961c2`, logical
contract `abb426a9`, context `32ec87a2`, native contract semantic revision
`aec16d6e`, policy `877c41e` with C44/C48 document reviews, and the unchanged
[74-method oracle map](schematool-001-native-test-mapping.md). The author
[source handoff](schematool-001-native-foundation-author.md) and root
[build handoff](schematool-001-native-build-author.md) were read in full.

## Changes required before execution

1. **Fallback does not establish reap before returning.** In the reviewed
   `native_foundation.cpp` lines 316–327, fallback sends a kill then makes one
   unchecked blocking `waitid`/`wait4`. A raw `-EINTR` can leave the child unreaped
   when lines 329–336 close the pidfd and return. The kill-only alarm cannot reap.
   The controller's line 166 and the negative-control unexpected-positive waits
   at lines 593/603 are likewise one-shot. Retry interrupted terminal cleanup and
   require an actual terminal consume or identity-supported ECHILD proof before
   the fixture returns; record unexpected cleanup failures without turning them
   green. Retain every existing status/FD/signal assertion and `fallback` failure.
   This is a fixture correction before execution, not a changed product oracle.

2. **Unexpected short-size success commits an unclosed pidfd.** At lines
   587–594, the deliberately invalid short-size call is expected to fail EINVAL.
   Its defensive positive-PID branch really reaps the unexpected child but never
   closes the actual successfully committed `pidfd_cell`. Only a positive
   successful return may adopt that output and close it after cleanup. Failed
   calls' output cells remain unowned, including after EFAULT; never guess an FD
   from a failed output write. An impossible/malformed positive output needs an
   explicit fixture-failure path rather than a claim of complete cleanup.

3. **CI omits the required generated-code gate.** The reviewed workflow builds,
   records hashes/`nm`, then immediately invokes CTest inventory and probes.
   Hashes and sole-export output do not prove the copied-stack child and executed
   reporter call graphs. Source comments expressly require this proof. Add and
   separately review the actual static checker and inspect actual host-generated
   paths before the first candidate execution. Reject unknown/indirect/runtime
   callbacks or allocation paths; do not infer the hosted compiler output from a
   local compiler. The current workflow must remain unpublished until this gate
   and corrected-source runtime release are concrete.

Findings were sent to root and A before this note. They are not evidence that any
candidate command ran, and no future correction is prospectively approved.

## Source, oracle and build assessment

The six ordinary GoogleTests and two ordinary loader methods use fixed, genuine
operations. The loader holds four builtin NUL-terminated arrays for the complete
CDLL call, fixes eight builtin pointer/uint arguments and c_int, and calls the
actual sole symbol. Its trusted artifact path is the exact preset output; no load
occurs during import/help. Literal31 is a temporary inert-stage control requiring
a later separately reviewed migration, never validation success.

The native child uses copied address space/stack with no CLONE_VM/FILES/THREAD,
zero stack fields, CLONE_PIDFD/CLEAR_SIGHAND and SIGCHLD. The child branch enters a
noreturn, fixed-report raw-syscall path before parent C++ work. The 16 int64 report
fields are all assigned, including failure sentinels; 128 bytes fit one atomic
blocking pipe write. Raw negative errno, eight-byte kernel signal masks and the
four-word x86-64 sigaction layout are explicit. The pre-exec report independently
checks caught-handler clearing, inherited ignore/mask, then explicit reset and
unmask; the second report uses a real exec of the fixed nonvalidating reporter.
Only one live extra FD closure is proved. The second close_range observes an
already-closed number; it is not claimed as the full future all-FD closure proof.

Main-thread birth owns the real positive PID/pidfd commit. The competing thread is
constructed before resource acquisition; release/acquire phases publish its
inputs/results, futex waits have fixed deadlines/attempt bounds, and join precedes
GoogleTest assertions. Live wait si_pid zero, real kill/reap identity, foreign
__WNOTHREAD refusal, competing real reap and pinned stale-handle ESRCH/ECHILD are
distinct oracles. No numeric-PID signal is used. Poll/protocol failure and the
independent five-second child alarm cannot make an observation green. The checked
getdents parser admits only complete bounded records, numeric names or dot entries,
sorted at most128 identities, and requires observed EOF; truncation fails. The
controlled EINVAL/EFAULT rollback checks complete before/after FD inventories and
own-thread ECHILD, not an unchanged output sentinel. Cleanup findings above still
prevent execution approval of these otherwise coherent observations.

I independently checked current primary [Linux v6.12 fork code](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/fork.c)
for signal-copy clearing, pidfd preparation/output-before-publication and rollback,
and [x86 signal UAPI](https://raw.githubusercontent.com/torvalds/linux/v6.12/arch/x86/include/uapi/asm/signal.h)
for the raw layout. The [glibc 2.35 x86-64 syscall source](https://raw.githubusercontent.com/bminor/glibc/release/2.35/master/sysdeps/unix/sysv/linux/x86_64/syscall.S)
has a success return on the copied stack, supporting this narrow no-CLONE_VM
recipe at source level. Actual linked-wrapper/generated-call inspection remains
mandatory. A web fetch of Linux exit.c timed out; it supplied no verification.
The [wait documentation](https://man7.org/linux/man-pages/man2/waitpid.2.html)
confirms interruption and the distinct terminal/ECHILD cases behind finding1.

The option defaults OFF; only the reviewed preset enables it. C++20 and existing
warnings are reused, with hidden visibility, sole-symbol version map, eager/RELRO
DSO binding and no RTTI/exceptions in the stub/reporter. No Qt/server/client or new
third-party process dependency is added. Existing GoogleTest selection admits a
system/local >=1.17 or explicitly fetches pinned `063de7e9578f82b369302001269680b4b1553359`;
actual selected source/version must be recorded, not assumed from the pin.
All six cases plus the one two-method loader registration are selected by an
exact inventory, explicit foundation label, finite timeout and unconditional
execution. Direct-native behavior red has a separate label and is excluded from
the green foundation command. A full preset CTest run would include that red and
must not be described as passing. No skip, failure inversion, provider/live data,
old stub-red replay or production control certificate is introduced.

Comparing actual accepted policy/native revisions against this packet shows only
factual acceptance headers/README/tool-policy wording, with normative bodies
preserved. Comparing `671fdf4` against this head shows exact blobs retained for
the Python parent, all six worker modules, public64, all four focused regressions,
SDK14 source, requirements lock and wheel identities. Earlier functional CI and
all first-red artifacts/reviews remain historical evidence, not native approval.

## Checks actually performed

- Read-only Git source/diff/blob inspection and primary-source web reads above.
- A stdlib here-document parsed the loader with `ast.parse(feature_version=(3,10))`
  and `compile` without `exec`; independently enumerated its two methods and six
  unique C++ TEST names, checked literal lengths13/16/20/14 and preset values,
  ran `bash -n` on all six workflow run blocks, and compared the frozen blobs
  listed above: PASS. An initial reviewer script incorrectly expected seven
  workflow blocks and stopped at that assertion; corrected six-block enumeration
  passed without executing any candidate or changing candidate source.
- `python3 tools/check_project.py`: PASS426 tasks/122 decisions/local links,
  presets/current HTML; explicitly does not compile/run/approve draft cases.
- `python3 tools/progress.py render --check`: current.
- No configure, C++ compilation, discovery, candidate import/load, native/syscall
  probe, child/thread creation, SDK install or behavior/test execution occurred.

Only this review note is owned/changed. Root retains integration, shared metadata,
CI publication, actual runtime release and final PR77 merge/stop responsibilities.

## Actual cleanup correction and generated stack-protector finding

I independently reviewed A's actual correction
`264c522fad3067aa95c2d42f8338d0e749909601` from `29902d4`, on a fresh isolated
review branch. Its only authored changes are foundation .cpp
`83c2255edc7281be46032486e14a73088c55afd8` and author note
`2e59eeb30a2aa7f633ab0488361d8698b8c82537`. **The two cleanup source findings
above are resolved for these actual fixture paths.** Configure/build/static
inspection permission remains; runtime and workflow publication remain withheld
pending the generated-code and source gates below.

Both raw wait helpers retry EINTR. `cleanup_child` retains the known successful
identity through real signal/wait, and returns only after actual native-PID
terminal consume, or pinned exit readiness plus a fresh birth-thread ECHILD under
the checked default/no-auto-reap SIGCHLD profile. The saved native PID fallback is
a wait, never a signal; this still-live birth thread makes no other birth or
reparenting, and __WNOTHREAD prevents claiming another thread's reused-PID child.
Unexpected failures cannot become a normal unproved child-gone return. The
nonreturning raw119 fixture failure is nonzero, not a cleanup or success
certificate. The foreign reaper retries interruption and records a nonmatching
consume as failure before allowing birth-thread disposal.

The writable short-size control now adopts/closes its pidfd only on a real
positive successful clone return. Negative returns still never adopt the cell.
An impossible positive PROT_NONE result reaps its actual child, then raw-exits121
rather than reading inaccessible output or guessing an FD. Missing successful
output/failed known-FD release are likewise fatal fixture errors, not passing
inventory observations. `fallback=true` still fails the original test. Explicit
braces preserve the inventory comparison and remove the reported dangling-else
macro ambiguity. No new product requirement or fixture weakening is involved.

I checked all **89 EXPECT invocations** against the original packet with an
independent balanced-parenthesis/string-aware source extractor: exact order and
arguments are preserved. All six TEST names/order, the three ancillary source
files and two loader registrations are unchanged. Python3.10 AST/compile without
exec, `clang-format --dry-run --Werror` on the changed .cpp, project426/122,
render--check and diff--check pass. No correction compilation or execution was
performed by me.

Root reported a permitted old-299 compile using GNU16.2.1, CPython3.14.9 and actual
system GoogleTest1.18.0: 12 build steps, exit0, one dangling-else warning corrected
by this revision. That is not a build receipt for the new source. I independently
used `objdump -dC` only on those existing ELF files, without executing/loading
either. Old foundation executable SHA256
`5905cd3241dbc8a9f301da01b1ce4a4c8a07db1ff5fd4517bf74ebc10b3db06a`
contains `%fs:0x28` canary access/check and `__stack_chk_fail@plt` in
`schema_foundation::observe`. Old reporter SHA256
`5615377668e308cfbde4029ef51752c5d25371702a2803b0c47fe065ecf43a70`
contains canary TLS access in main/observe and the external failure call in
observe. Thus raw-only generated paths are presently **not** established even
though the source uses no explicit runtime helper.

The narrow correction is a function-local `no_stack_protector` attribute on the
already bounded raw child helper/entry call graph and the fixed reporter's
main/literal/descriptor helpers where needed. [GCC documents the function-local
attribute](https://gcc.gnu.org/onlinedocs/gcc/Common-Attributes.html), and
[Clang documents the same supported behavior](https://clang.llvm.org/docs/AttributeReference.html#no-stack-protector-safebuffers).
These functions use fixed arrays/reports, bounded decimal/literal parsing and
raw syscall results; their bounds still need source review. Removing one generated
helper does not prove absence of all compiler instrumentation. This route honors
the frozen raw-only path more directly than claiming the external failure branch
unreachable or permitting an extra PLT call.

This assessment releases **annotation source authoring only** for those bounded
functions, with a written rationale and exact different source review before
compilation. Keep parent GoogleTest/probe/disposal hardening; no target/global
`-fno-stack-protector`, expanded raw graph, accepted PLT exception, altered oracle,
ABI, public limit or normative scope relaxation is approved. The actual future
annotation diff is not prospectively approved. A fail-closed, differently reviewed
static checker and actual per-host generated/linked graph, TLS/indirect/external
call inspection still precede discovery/probes and CI publication. No native
owner, runtime facility or full test-baseline approval follows from this cleanup
verdict or authoring release.

## Actual raw-function attributes and first static-checker review

Different source review at actual root
`7b212654c183709dfb97b85d2308b4d7f56926fc`, on a fresh isolated branch. A's
annotation commit `66cd1ac6357ec6049575ba80c2182a71f3bdca3d` is integrated as
`8aa2847`; its header is `d002f9dd03ebe8ed721c1fb9051c96b5c92d5121` and reporter
is `f13e886bc5b622d1f71dcfe5e0e68bc634887c46`. Root separately authored the
202-line static checker `tools/schema_validation/audit_native_foundation.py`,
blob `c6245e60f74533237f9487ee0db0ee61a73f32ee`. I authored neither source.

**The actual attribute source is approved for rebuild and static artifact
inspection. The actual checker requires changes and is not approved for execution
or admission. Runtime/discovery/loading and workflow publication remain held.**
Root may now rebuild the corrected/annotated targets, then read objdump/readelf/nm
and artifact hashes. Neither this source approval nor a future checker result is
by itself a host capability or native-owner approval.

I independently counted exactly seven header function-local attributes on
raw/address/finish/observe/publish/hold/child, and three on reporter
literal/descriptor/main. Removing only those attributes, comments and whitespace
leaves exact previous bodies/types/literals. Parent probe/cleanup source (including
all89 EXPECT calls), loader and direct-native22 source are byte-identical to the
approved previous revision. No target/global hardening option changed. The actual
annotation therefore matches the narrow GCC/Clang route approved above.

The first checker invokes only fixed objdump/nm/readelf subprocesses with input
paths as arguments, reads ELF/disassembly/maps, and never loads or executes input
artifacts. Its command timeouts and trusted build-artifact scope are appropriate
for a small build harness; it is not the product package validator or a general
binary verifier. The ELF header, transitive roots, TLS/indirect transfer checks,
sole-export/eager-binding inspection and libc success-wrapper intent are useful,
but these concrete gaps prevent its claimed fail-closed admission:

1. `disassembly` lines62–74 silently ignores any address-bearing instruction
   line that fails its mnemonic regular expression, such as `(bad)` or `.byte`.
   Unknown decoded bytes can therefore disappear. Reject every unparsed
   address-bearing record. Local branches must also land on a known instruction
   start, rather than any byte numerically between first and last addresses
   (lines101–102).
2. `syscall_return` lines125–157 checks a **set** of seven moves plus a syscall,
   compare, forward jae and return. It does not require their exact order, exclude
   arbitrary extra instructions/TLS/stack mutations, or require the threshold
   compare immediately after the syscall. Use the actual narrow ordered reviewed
   wrapper shape, allowing only explicitly justified padding/CET instructions;
   unknown transforms must fail, not satisfy a subset check.
3. `raw_helper` lines31–32 admits any symbol in the entire schema_foundation
   namespace. That does not enforce the claimed rejection of unknown generated
   helpers. Admit the actual fixed reviewed helper-name families, including
   concrete address-template instantiations, and reject other names.
4. The graph root begins **inside** `child`. It omits generated instructions
   between successful clone return and the zero-PID branch's child entry; compiler
   helpers/TLS there are outside its graph. Reading the audit Python process's
   mapped libc also does not establish the executable's actual syscall PLT
   binding. Add narrowly reviewed callsite/relocation and same minimal-loader
   binding proof, or explicitly retain different actual **per-host** inspection
   of those edges before execution. Do not describe this checker alone as complete
   generated-path proof or infer hosted output from a local compiler.

Findings and the exact rebuild-only release were sent to root before this note.
No prospective correction or checker fixture is approved. A small literal
positive/negative parser/wrapper fixture set would be useful to establish that
malformed or unreviewed disassembly cannot silently pass; that too needs actual
source review before execution. The existing source/contract/oracle approvals
remain valid within their stated scopes.

Actual checks: independent stdlib attribute/body/preserved-file comparison;
checker Python3.10 AST parse and `compile` without exec, with only argparse,
hashlib, pathlib, re, struct and subprocess imports; clang-format dry-run/Werror
on both changed C++ files; project426/122; render--check; diff--check: all PASS.
I did not import/run the checker, configure/build, execute binutils in this review
turn, discover/list cases, load a DSO, run a probe/test, install SDKs or create a
candidate child/thread. Prior old-artifact objdump observations above remain
separately attributed and are not results for this new source.

## Actual four-finding static checker correction

Reviewer C reviewed root's actual `258ac989bd0d19fdd7097f664dc7b395e5286454`,
checker blob `0eacdc88f1e91b26b648eff11b71f07ef3fd0fe3`, against preserved old
`c6245e6`. This follows independently reviewed controls `ee13b687` and their
actual FIRST static-only8/10 assertion red, independently attributed in
[the control review](schematool-001-native-audit-controls-review.md). No test or
native source changed. **The actual bounded checker correction is approved for
the unchanged static-control rerun and static artifact audit only.**

Every address-bearing decoded record must now parse and belong to a function;
unparsed/bad/byte records cannot silently disappear. A local transfer must land
on an actual instruction start, and duplicate starts fail. The raw helper set
contains exactly twelve reviewed names, including concrete address-template
instantiations, rather than accepting the namespace. The libc success prefix
requires seven exact ordered register moves, syscall, the immediate exact errno
comparison, one unsigned forward error edge to an actual later instruction and
an exact ret, with only known initial NOP/CET decorations. Reordered/extra
instructions, TLS or stack changes no longer satisfy a subset-of-moves rule.
Fixed actual binutils decoding and the reviewed source remain the trust scope;
this is not an adversarial general binary verifier or a complete semantic proof.

Root may now run the unchanged eight-control module and use this checker to read
trusted compiled artifacts via fixed binutils commands, recording exact results
and hashes. A failed audit is not a green capability or permission to broaden its
allowlist silently. The checker does not execute or load inputs. No actual green
or corrected-artifact audit result was observed by me in this review.

The clone3 success-return to child-entry edge and the actual executable's libc
binding are **still outside this checker**. Root retains different concrete
per-host generated-path/binding review before runtime unless a separately
reviewed actual automatic/bootstrap proof replaces it. Reading Python's maps is
not by itself executable binding proof. A proposed no-child bootstrap source is
not approved prospectively. Discovery, case listing, DSO loading, facility/child
probes, native behavior red and CI runtime publication remain held. Both selected
host foundation receipts and all future native-owner source/test/red gates remain.

Actual checks: source/diff inspection; Python3.10 AST and compile without exec;
exact twelve helper literals; preserved control/foundation/public/SDK blobs;
project426/122, render--check and diff--check PASS. I did not import or execute the
corrected checker, controls, binutils or any candidate artifact in this review.

## Actual no-child binding source and staged build review

Reviewer C, actual root `3c3ad5fea87bc4465168b9906c19b4ae22850376`, fresh isolated
review branch. A's actual source `473448c7186d16df84ea4a9ee9133105ac87a730` is
integrated as `73ecce5`; root separately authored the CMake/CI additions. Reviewed
identities: foundation .cpp `96eefa80d17553494c63dd8df99581181cacc241`, author
note `a6324e66ab74eb5a7cf06eb7f72868c45d23a9c2`, scoped test CMake
`f722d7bde1a3a28427f20de8ddef383b7f097031`, workflow
`4eb2a350350296cabbc6e125c236799fe1acebbe`. I authored neither source nor build.

**The actual additive source/build configuration is approved for rebuild and
static inspection.** Root may run the reviewed one-case NO-CHILD bootstrap only
after the corrected static graph gate and different actual PLT/GOT/clone-entry
inspection of the resulting local artifact. Those generated observations and
the bootstrap's future result are not supplied by this source verdict. No kernel
or child probe, CTest inventory, loader, behavior31 red, native owner or workflow
runtime publication is released here.

The seventh case rejects nine loader-override keys by presence, including empty
values, in its disposable process. Its actual `&::syscall` address must resolve
through dladdr to the exact symbol start/name in one of six fixed standard libc
spellings; an executable PLT address fails rather than pretending to be libc.
The path copy is bounded4096+NUL. Its trusted already-loaded base supplies only
the fixed64-byte ELF header, checked for ELF64/little-endian/current version,
ordinary SYSV/GNU ABI, ET_DYN and x86-64. Address subtraction checks nonnegative
offset below64MiB before recording it. This is a selected trusted system-loader
fixture, not authentication of arbitrary package addresses or a modified host.

Only that resolved resident path is opened with NOLOAD/NOW/LOCAL. The two actual
lookups clear/check dlerror independently; both unversioned and GLIBC_2.2.5
addresses must equal the original, and a second dladdr must match base/path/start.
No GTest assertion, formatting or throwing C++ operation occurs between acquiring
that reference and its one checked dlclose. Null/lookup/close failures remain hard
test failures. There is no generic syscall invocation, transport-owner call,
new FD acquisition, child/thread, signal mutation or probe helper in this case.
The later metadata records actual loader values, never a fabricated hash or
runtime control certificate.

I independently read [dladdr's PLT warning](https://man7.org/linux/man-pages/man3/dladdr.3.html),
[NOLOAD/NOW and reference semantics](https://man7.org/linux/man-pages/man3/dlopen.3.html),
[versioned lookup/error semantics](https://man7.org/linux/man-pages/man3/dlsym.3.html)
and the [glibc x86-64 ABI list](https://raw.githubusercontent.com/bminor/glibc/master/sysdeps/unix/sysv/linux/x86_64/64/libc.abilist).
They support these literal checks; they do not observe selected hosts or prove a
different call site's binding. A tagged ABI-list fetch failed; the successful
primary master list is the source used for the stable GLIBC_2.2.5 symbol fact.

Root adds only CMAKE_DL_LIBS to the test executable. Static8 controls and the
corrected graph audit precede native startup. The explicit filtered bootstrap
uses the exact executable, minimal environment, finite30s and checked native
exit. Its XML guard requires exactly one actual named run/completed case, zero
failures/errors/disabled/skips and one testcase, so an empty filter cannot pass.
The reviewed registration inventory is now seven native cases plus the unchanged
two-method loader program. All old six case/helper tokens and89 EXPECT calls,
264 cleanup and66 raw attributes remain preserved.

**The full candidate workflow remains unpublished.** It still proceeds from the
bootstrap into discovery/full kernel foundations without enforcing the omitted
current-artifact clone-entry/binding review. Its ordering/guards are sound as
source, but cannot stand in for that missing execution gate. A compile/static/
no-child-only hosted stage needs its actual separately reviewed configuration,
or a small reviewed exact generated-edge gate, before publication. Concrete
per-host artifacts/metadata must then be differently admitted before enabling
unconditional child probes. A local compiler or lookup pass is not hosted proof;
neither a general ELF dependency resolver nor an unsupported capability claim is
needed to preserve this staged boundary.

Root's earlier old-source compile/rebuild logs are read as compile-only evidence:
the latter has four steps/no compiler warning and no execution command. Root
also reports corrected static8 green and static graph exit0; this source review
does not replace separate raw/output attribution of those observations. None is
a result for the newly added bootstrap source.

Actual checks: independently remove only the additive case/five new includes and
compare all previous tokens with264; enumerate seven cases; verify89 old EXPECTs,
close-before-assert and no forbidden helper call; syntax-only Python3.10 AST/
compile for controls/checker/loader; bash-n on all nine workflow run blocks;
clang-format dry-run/Werror; project426/122; render--check; diff--check, all PASS.
No CMake configure/build, binutils invocation, candidate import/loading, native
program, case-list/discovery, bootstrap or child/kernel probe ran during this
review. Root retains actual artifact/release/integration and PR77 finish/stop.
