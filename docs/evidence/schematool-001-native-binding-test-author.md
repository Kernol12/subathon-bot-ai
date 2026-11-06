# SCHEMATOOL-001 — Python native-binding admission test source

Author A (`/root/session_supplier_review`), 6 November 2026. New isolated branch
`task/SCHEMATOOL-001-native-binding-tests`, tree
`/tmp/subathon-schematool001-native-binding-tests`, starts at root `9bce17d`.
Own only [native_binding_acceptance.py](../../tests/schema_validation/native_binding_acceptance.py),
this note and the explicitly authorized minimal declarations in
[schema_validate.py](../../tools/schema_validate.py). Finish PR77 then stop.
No candidate import, test execution, native load/call, compiler, subprocess,
worker, signal or SDK probe is authorized or performed by this source assignment.

Authority: [native process §§1/2](../contracts/schema-validation-native-process-v1.md),
[installation revision2](../contracts/schema-validation-installation-v1.md),
[public API](../contracts/schema-validation-tool-api-v1.md), and the independently
reviewed [74-method mapping](schematool-001-native-test-mapping.md). The approved
public74 port/correction source4b/926 and C26e verdict remain separate artifacts;
this9b base still preserves the old Python parent3438. Their matching integration
and all prior valid source/runtime receipts remain root's responsibility.

## Exact private surface and inert declarations

Root froze `_load_native_transport()` to return the actual builtin ctypes C
function on admitted success, or `None` for recognized missing/unloadable/
wrong-profile/missing-symbol installation. Public validation must map that None
to `environment_unavailable` before entry. Known MemoryError escapes loading for
public `resource_limit`; KeyboardInterrupt/BaseException identity must not be
swallowed. No new private loader exception or public option is introduced.
`_decode_native_result(result: int) -> SchemaValidationOutcome` is the pure
post-quiescence mapping surface, not a worker, validator or resource owner.

Because neither declaration existed at9b, the ONLY production-source additions
are those two functions with an immediate single
`raise SchemaNativeBindingNotImplemented()` body, and that distinct exception's
literal message `Native schema binding is intentionally unimplemented`.
No native/import/path/mapping/routing behavior exists in those stubs. All old
imports, class/function bodies and module statements are unchanged. The existing
public calls still use the old parent until separately reviewed replacement.
This distinct Python-binding marker must never be reported as actual native31,
`SchemaTransportNotImplemented` or the historical original wrapper marker.

## Focused inventory and independent oracles

Nine natural methods, with the following selected cases/rows:

| Method suffix | Cases | Independent requirement/oracle |
| --- | ---: | --- |
| definition_only_import_has_no_native_or_file_side_effect | 1 | Actual trusted module definition load; CDLL, builtin/io/os open and Popen guards all remain uncalled, including post-call assertions. Trusted source import itself is the test mechanism. |
| actual_trusted_function_has_builtin_eight_argument_abi_and_local_now | 1 | Real production DSO loaded at the exact repository-derived path with LOCAL/NOW; returned object is that library's genuine builtin symbol, eight exact pointer/uint types, c_int, no errcheck or Python-API/GIL-held flag. It is NEVER invoked. |
| missing_empty_and_short_fixed_artifacts_refuse_without_cdll | 3 | Missing artifact, empty file and four-byte ELF prefix all return None; synthetic bytes never reach CDLL. |
| wrong_elf_profiles_are_read_then_refused_without_cdll | 7 | Independent64-byte ELF64 ET_DYN/x86-64/current header mutations: magic, class32, big endian, ident version, ET_REL, AArch64 and header-size63. Actual header bytes must be inspected; each returns None and CDLL remains uncalled. |
| loader_failure_classification_preserves_known_python_exceptions | 3 | At the specific loader call, ENOEXEC→None/quiet, but the original MemoryError and KeyboardInterrupt objects propagate. Every CDLL attempt has exact trusted path/local-now arguments. |
| missing_versioned_symbol_refuses_after_real_trusted_load | 1 | Actual CDLL loads the trusted artifact; only symbol lookup is denied with AttributeError. Require that exact versioned lookup once, None refusal and exact load call. |
| public_loader_none_refuses_environment_before_worker_entry | 1 | First invoke the actual loader so the binding-stub marker precedes any synthetic refusal. Then a negative-only None seam must be called once and public environment refusal must precede the guarded old worker path. |
| quiescent_integer_mapping_is_literal_and_does_not_validate | 18 | Explicit literal0..17 mapping; reserved1 refuses as validation_unavailable. Six independent loader/open/spawn guards forbid decoder resource work or swallowed attempts. |
| quiescent_interrupted_and_unknown_integers_refuse_without_fake_signal | 7 | Literal signed32-bit extremes, -1,18,19,30,32 all map to validation_unavailable; same pure-mapping guards. No manufactured KeyboardInterrupt. |

The mapping methods are unit assertions about closed integer conversion ONLY.
Calling the pure decoder with integer0 does not inject a successful native result,
replace a real validator or claim a schema-package pass. No test synthesizes31;
that marker remains reserved for an ACTUAL reviewed C return31. No C function is
invoked by this module, including the genuine function inspected by the ABI case.
No arbitrary foreign pointers, converters, errcheck, callbacks, alternate public
artifact path, new dependency or fake successful engine exists.

The loader-specific three-way exception case closes a distinct broad-catch risk
inside admission; it does not duplicate public74's pre-path allocation or actual
post-return identity/MemoryError complements. Existing exact help, no-callback
argv/path checks, six environment-presence guards, real package positives,
native25/26 cleanup and all other public74 assertions remain unchanged and required.

## Synthetic path/header and negative lookup discipline

Root explicitly authorized TEST-only patching of the existing module `__file__`
to an actual temporary `tools/schema_validate.py` beneath a disposable synthetic
repository. The sole candidate fixture path is still exactly its derived
`build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so`. No production
getter, loader parameter or path override is added. Missing/short fixtures have
no callable code. Each malformed64-byte header is independently constructed from
literal bytes and a fixed field mutation, never produced by the loader under test.

The header read recorder delegates real builtin/io file operations and os
open/read/pread. It records actual bytes and binds tracked raw FD numbers to their
actual device/inode, including fdopen paths. A no-read/static-None shortcut cannot
pass the seven complete-header cases. Setup ownership closes real files/FDs if
constructing their recording view fails. CDLL is independently guarded and its
post-call no-call assertion prevents a swallowed loader attempt from going green.
The recorder is solely test I/O observation, not an ELF parser or feature engine.

The real positive ABI and missing-symbol cases load ONLY the actual trusted,
separately admitted production DSO. The latter replaces only ctypes symbol lookup
with a negative AttributeError; it does not supply a successful callable, integer,
semantic validator or library. Recognized loader OSError/known Python exceptions
are negative-only injection at the pre-entry loader boundary. Actual binary/source/
provenance and both-host foundation gates must pass before those cases execute.

## Checks, intended baseline and remaining gates

Actual source-only checks PASS: Python3.10 AST parsing and compile-without-exec
for the new tests and parent; complete old parent AST equality after removing
ONLY the three new declarations; two immediate single-raise function bodies;
nine unique natural methods; independent64-byte ELF header field/length and
18-row literal mapping checks; no foreign function invocation or fabricated31.
`python3 tools/check_project.py` PASS426/122/current HTML;
`python3 tools/progress.py render --check` current; `git diff --check` PASS.
Only the three assigned paths differ.
Different review must inspect the exact nine test bodies/helpers/literal rows and
the exact16 inert declaration lines before root releases any run. The intended
new FIRST is the real `SchemaNativeBindingNotImplemented` raised by those stubs,
never missing attributes, a missing artifact/dependency, setup error or simulated
native31. The passive-import case can pass independently; its truth is not a
validator implementation. For public-None routing, the actual loader pre-call
ensures its stub marker is reached before a prospective unimplemented route.

Root must integrate the reviewed public74 revision and source routing gates, bind
the selected ordinary venv/interpreter and actual built artifact, and select a
finite unconditional CI registration before any execution. Existing native69
and original wrapper FIRST receipts are retained separately; no replay or new
attribution is authorized here. Loader/decoder implementation follows the new
different test/stub review and actual intended binding red. The C++ owner still
requires actual both-host helper GREEN and reviewed native69 FIRST independently.
All public74, direct12, negative57, worker/SDK, offline/budget/source and latest-head
normal PR77 merge gates remain. No component completion or runtime capability is
claimed by this source packet.

Prospective root-owned registration: the admitted ordinary venv interpreter with
`-I -B` and the absolute `native_binding_acceptance.py` path, standard unittest
main selecting all9 methods, finite timeout and explicit nonzero inventory.
No `-S` parent or arbitrary artifact argument is introduced: the positive
public-None observation needs the ordinary installation binding, and the library
location is fixed by the trusted entrypoint. This command was NOT executed.

## Source-test correction: genuine lookup return without cache assumptions

Author A, 6 November2026. Separate branch
`task/SCHEMATOOL-001-native-binding-lookup-identity`, tree
`/tmp/subathon-schematool001-native-binding-lookup-identity`, starts at preserved
`c38c24846de39343645e347c76cb8aea7ac2430d`. ONLY the binding test and this note
change. B's different actual source review3165 found one overconstraint in the
positive ABI case: comparing the returned entry to a subsequent attribute lookup
requires ctypes attribute-cache identity that native contract§2 never required.

[Official ctypes documentation](https://docs.python.org/3.10/library/ctypes.html#ctypes.CDLL)
describes cached attribute access and fresh wrapper objects for indexed access.
Both are valid ways to retrieve the same genuine exported function. The revised
test transparently records the actual objects returned by REAL
`ctypes.CDLL.__getitem__` DURING the loader call; ctypes attribute lookup also uses
that method. The recorder invokes the saved original method, records its actual
library/symbol/function-object tuple, and returns that object unchanged. It never
calls the foreign function, supplies a fake C callable/result, or requires one
particular lookup syntax/count/cache policy.

The returned entry must now be identical to one of those genuinely returned
objects for the exact loaded production library and versioned symbol. The real
CDLL trusted-path/local-now call, exact library count, builtin `_CFuncPtr`, all
eight builtin argument types, c_int result, absent errcheck and non-PythonAPI/GIL
flag assertions remain unchanged. This corrects a fixture overconstraint while
preserving the native ABI requirement; it does not reduce product behavior or
weaken the prohibition on successful substitutes.

All9 method names, eight other complete method ASTs, header/admission helpers,
module statements, literal25 integer mappings, negative header rows and all16
inert declaration lines remain exact. The only test-function change is the
positive ABI recorder/identity assertion. Source-only checks PASS: Python3.10
AST/compile-without-exec, exact equality of every other definition and unchanged
production source/stubs,9 names/25 mapping rows, transparent real lookup forwarder;
project426/122/current HTML, render--check current and git diff--check.
No test/candidate import, native artifact
load, function invocation, child/thread, compiler or SDK execution occurred.
Actual different delta review is required before the intended named Python-binding
FIRST; actual native31/owner69 and both-host facility gates remain separate.
