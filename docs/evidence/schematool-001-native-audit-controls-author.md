# SCHEMATOOL-001 static-audit control source

SOURCE ONLY, no execution. Author A, isolated
`task/SCHEMATOOL-001-native-audit-controls`, base
`7b21265` (root's actual `audit_native_foundation.py` build-harness source).
Only this note and `tests/schema_validation/native_audit_controls.py` are owned.
The user scope remains finish PR77 then stop. Root owns the audit implementation,
build/artifacts/CI/state; C owns different actual control-source review. Existing
tooling19, foundation6+2, behavior/public74, source reviews and red receipts stay.

## Exact natural inventory and independent literals

Eight ordinary `NativeAuditControls` methods, 38 selected control observations:

| `test_` suffix | Rows | Fixed oracle |
| --- | ---: | --- |
| `internal_raw_graph_has_exact_reachable_names` | 1 | Literal admitted child/raw closure and exact sorted names; internal branch lands on an actual instruction boundary. |
| `plt_and_unknown_external_transfers_refuse` | 4 | PLT call, missing external target, PLT tail jump, and unreviewed function despite `schema_foundation::` prefix all raise RuntimeError. |
| `indirect_or_unresolved_transfers_refuse` | 4 | Register call, memory jump, unresolved numeric operand and local jump into the middle of a mov instruction refuse. |
| `tls_unreviewed_instructions_and_traps_refuse` | 6 | FS/GS access, int3, sysenter, ud2 and unreviewed xsave refuse. |
| `malformed_text_and_missing_or_ambiguous_roots_refuse` | 8 | Empty text, duplicate function address, address-bearing `(bad)`/`.byte`/unparsed line, missing root, empty root and duplicate matching roots refuse. |
| `literal_objdump_text_preserves_graph` | 1 | Fixed GNU-style two-function text yields exactly the literal parsed dictionary, objdump argv and reachable names. |
| `literal_libc_success_prefix` | 1 | Fixed seven-move/syscall/negative-errno compare/forward jae/ret prefix admits the exact symbol; TLS errno block beyond that first ret is outside the success prefix. |
| `corrupted_libc_prefixes_refuse` | 13 | Wrong register, wrong errno threshold, wrong branch condition/direction, external call, missing syscall, missing return, ambiguous symbol, reordered moves, extra RAX clobber, TLS, stack change and delayed compare refuse. |

The literal libc prefix is transcribed independently from the reviewed
[x86-64 glibc generic syscall source](https://raw.githubusercontent.com/bminor/glibc/master/sysdeps/unix/sysv/linux/x86_64/syscall.S)
and the native-foundation contract's copied-stack return requirement. The seven
register moves are an ordered data dependency, not an unordered set: overwriting
RDI before copying the original syscall number into RAX changes the call. The
Linux negative-errno threshold is the fixed `0xfffffffffffff001` literal; the
forward unsigned branch must skip the bounded success return. Extra TLS/stack/
RAX-changing instructions and a delayed comparison are not an admitted prefix.
Corruption fixtures change independent literal tuples and adjust their own fixed
addresses; no audit output generates fixture data or expected results. These
snippets/dictionaries are data, never assembled or executed programs.

## Actual source findings and staged observation

C's actual source review of the current7b audit found concrete fail-closed gaps:
address-bearing unparsed lines can be silently dropped; local branch range does
not establish instruction-boundary membership; a namespace prefix is not a
reviewed-function allowlist; and the libc unordered-move/loose-prefix checks can
admit reordered moves, extra operations, TLS, changed stack or delayed comparison.
Root explicitly required fixed regression controls **before** correcting those
implementation paths. The listed negative expectations derive from the approved
raw-child/generated-ABI gate, not from what7b currently happens to accept. They
are intended static-gate regressions; none has yet been observed red or green.

The test module imports only stdlib and defines helpers/tests. During a future
authorized test run, setUpClass imports only the actual audit SCRIPT, whose main
guard remains inactive. No test calls its `main`, `elf`, `libc_path`, reads process
maps or discovers/loads native artifacts. Tests call only `reachable`,
`disassembly` with the binutils-text `command` mocked, and `syscall_return` with
literal `disassembly` mocked. There is no binutils process, native library call,
worker, validator, platform capability, accepted package result or success seam.
The script-under-test stays byte-identical to the assigned base.

Different actual control-source review and explicit static-only execution release
must precede their FIRST control run. Preserve that run's source/raw bytes/status
and distinguish genuine assertion failures from fixture/import failure. Root may
then correct the audit logic without weakening these controls. A future control
green is narrow harness evidence, not a general binary-verifier proof, native
feature red, native-owner coverage or host admission. C's entry-edge/binding audit
findings and all real per-host generated-code/artifact/ABI/runtime gates remain.
This assignment changes no product behavior, frozen native assertion or build
policy and authorizes no candidate/native execution.

## Author verification

Actual stdlib source-only checks: Python3.10 `ast.parse` and `compile` without
exec PASS; exact eight method names match the table, literal AST tuple sizes
independently establish1+4+(3+1)+6+(5+3)+1+1+(6+7)=38; no top-level candidate call
or test calls to audit main/elf/libc_path. `git ls-tree HEAD` records unchanged
audit blob `c6245e60f74533237f9487ee0db0ee61a73f32ee`.
`python3 tools/check_project.py` PASS426/122;
`python3 tools/progress.py render --check` current;
`git diff --cached --check` PASS before commit. Only the two owned files change.
No test module/audit script import, test body, main/discovery, binutils, candidate
compile/load/probe, native thread/child/signal or installation was executed here.
