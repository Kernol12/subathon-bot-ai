# SCHEMATOOL-001 static-audit controls — independent source review

Reviewer C (`session_plan_review`), 6 November 2026, existing PR77 only. A authored
actual `0ced7a3969f74e198b1b00afc8f720806eca0145` from `7b21265`; I reviewed that
immutable source on a fresh isolated branch. Only this review note is mine.

**APPROVED: the actual eight-method/38-control source and independent literal
oracles. Root may perform the FIRST static-only control run before fixing the
checker. No native artifact, discovery, loader, probe, kernel capability, full
generated-profile admission, production owner or CI publication is approved.**
The run must use this reviewed source against unchanged checker `c6245e6`, preserve
actual command/environment/head/raw stdout/stderr/status and distinguish genuine
algorithm assertion failures from fixture/import errors. Missing modules,
binutils, compilation or skipped tests cannot establish this static red.

Reviewed test source is `tests/schema_validation/native_audit_controls.py`, blob
`ee13b68706333cd1e4c131125b7a404462f5d81b`; author evidence is
`a1519ba6223362b6d1bf019c90c032449df64e12`. The checker remains exactly
`c6245e60f74533237f9487ee0db0ee61a73f32ee` and is intentionally unfixed/unapproved
for artifact admission. Authority is native contract semantic revision `aec16d6`,
ADR-0009/C48 scope and the actual fail-closed findings in
[the foundation review](schematool-001-native-foundation-review.md), especially
the actual7b checker review. These are test-build harness controls, not the package
validator or a substitute for its unchanged public74/fault/SDK oracles.

The exact ordinary methods and independent observations are:

| Method suffix | Controls | Assessment |
| --- | ---: | --- |
| `internal_raw_graph_has_exact_reachable_names` | 1 | Two literal function records; branch lands at a real boundary; exact sorted closure. |
| `plt_and_unknown_external_transfers_refuse` | 4 | PLT call, unknown external, PLT tail jump and unknown namespace helper each refuse. |
| `indirect_or_unresolved_transfers_refuse` | 4 | Register/memory indirect transfer, unresolved target and instruction-middle target refuse. |
| `tls_unreviewed_instructions_and_traps_refuse` | 6 | FS/GS and int3/sysenter/ud2/xsave refuse. |
| `malformed_text_and_missing_or_ambiguous_roots_refuse` | 8 | Empty/duplicate/unparsed address text and missing/empty/ambiguous roots refuse. |
| `literal_objdump_text_preserves_graph` | 1 | Exact literal dictionary, objdump argument list and closure are asserted. |
| `literal_libc_success_prefix` | 1 | Exact seven ordered moves, syscall, errno compare, forward jae, ret; later errno TLS is outside the success path. |
| `corrupted_libc_prefixes_refuse` | 13 | Six direct corruptions plus missing return, duplicate symbol, reordered moves, RAX/TLS/stack mutation and delayed comparison refuse. |

The libc expected sequence is independently transcribed from the reviewed
[glibc x86-64 syscall source](https://raw.githubusercontent.com/bminor/glibc/release/2.35/master/sysdeps/unix/sysv/linux/x86_64/syscall.S).
Its first register moves are ordered data dependencies; moving RSI into RDI first
destroys the original syscall number before the RAX copy. The extra RAX operation
changes the call, and stack mutation changes the sixth argument/return premise.
TLS, an unreviewed intermediate operation or a non-immediate compare is outside
the deliberately narrow wrapper profile. The fixture's address rebasing is local
arithmetic on fixed tuples; it neither queries checker output nor assembles or
executes those bytes. Negative cases retain the relevant surrounding valid graph
so they do not depend on an unrelated missing-root error. Positive exact parser
and wrapper controls prevent a blanket reject implementation from satisfying
the negative expectations.

Import defines stdlib helpers/tests. At run time `setUpClass` loads only the actual
audit module definitions using a non-main name. I read that exact audit's top level:
stdlib imports, literal/regex constants and function definitions; its main guard
is inactive. Test bodies call only `reachable`, `disassembly` with `command`
mocked, and `syscall_return` with `disassembly` mocked. The latter is the real
wrapper-checking function; only its external text dependency is replaced with
literal data. No test calls main/elf/libc_path, accesses process maps or native
artifacts, launches binutils, loads a DSO, creates native workers or submits a
fake successful validation. Synthetic graph ret instructions test closure parsing,
not a claim that the actual noreturn child returns.

This source review precedes the first static run. Root may correct the checker
only after actual known-path assertion failures are recorded, without weakening
these controls; actual corrected source and observed green still need different
review. Neither controls nor future green establish the omitted clone-success to
child-entry edge or actual executable/libc symbol binding. Root explicitly keeps
different concrete **per-host** disassembly/binding review before runtime, unless
an actual reviewed automatic proof replaces it. All native execution/publication
holds and both-host real foundation gates remain.

Actually performed: Python3.10 `ast.parse` and `compile` without exec; exact eight
ordinary undecorated method names, literal tuple counts `1+4+4+6+8+1+1+13=38`,
allowed direct target-function calls and unchanged checker/main guard inspection;
`python3 tools/check_project.py` PASS426/122;
`python3 tools/progress.py render --check` current; `git diff --check` PASS.
I did not import/run either candidate Python module, execute test bodies, invoke
binutils, access ELF/proc artifacts, compile/load native code, create candidate
children/threads, probe the kernel or install dependencies.

## Independent attribution of actual FIRST static red

Different receipt/source review at immutable root
`258ac989bd0d19fdd7097f664dc7b395e5286454`, using a fresh isolated branch.
I read and parsed the complete committed
[stderr](schematool-001-native-audit-first-red.stderr.txt),
[empty stdout](schematool-001-native-audit-first-red.stdout.txt),
[receipt](schematool-001-native-audit-first-red.receipt.json) and
[observer note](schematool-001-native-audit-first-red.md), without rerunning FIRST.
**The actual static-only reached-failure attribution is accepted.** This is
separate from my earlier test-source approval and does not attribute native
execution, host capability or native-owner behavior.

The receipt identifies clean root `2832f736ffddc28efb1ad36e8639010523715fa3`,
selected `/tmp/subathon-schematest001-py314/bin/python -I -S -B
tests/schema_validation/native_audit_controls.py`, cwd `/tmp/subathon-schematool001`,
with only PATH=/usr/bin:/bin, LC_ALL=C.UTF-8 and LANG=C.UTF-8. Start is
2026-11-06T16:05:49.166958Z, after actual source review `e19b38a` (16:05:00Z)
and its integration at the observed head. Parent/observer records Python3.14.9;
the receipt itself records the selected interpreter path rather than a separate
version output. Exit1, native8 methods/0.006s, outer0.059447s and watchdog=false
are consistent across full raw and receipt.

Independent full-byte hashing confirms stderr8411 bytes, SHA256
`98bf17727ae168001b9dcd252e3b997672c91add859c469a26f56cc1db6786d3`, and stdout0
bytes with the standard empty SHA256. All eight actual method names appear once
in the top-level progress inventory. All ten failure records are exactly
`AssertionError: RuntimeError not raised`, at the reviewed local assertRaises
sites, grouped as follows:

- Five named altered libc prefixes: reordered moves, extra RAX clobber, TLS,
  stack mutation and delayed compare (line228).
- One instruction-middle branch accepted (line104).
- Three address-bearing bad/byte/unparsed text rows accepted (line128).
- One unreviewed namespace helper accepted (line88).

No ERROR, setup/import/dependency cause, skip, watchdog or native path appears.
All three expected-positive parser/closure/libc methods pass. The raw footer is
`Ran 8 tests in 0.006s` / `FAILED (failures=10)`. Reviewed test `ee13b687` and old
checker `c6245e6` are exact at the recorded head. The subsequent commit changes
only checker logic plus these raw/receipt/note files; foundation/public/SDK source
is unchanged. The actual source limits execution to definitions and mocked text,
so those reached assertions do not require ELF/proc/binutils/native candidates.
The receipt is the preserved first observation; I have not replayed it or inferred
any unrecorded execution from source alone.

I separately reviewed corrected checker `0eacdc88` in
[the foundation review](schematool-001-native-foundation-review.md). Its approval
releases the unchanged **static** eight-control green run and static-only artifact
inspection, while actual clone-entry/binding/per-host/runtime gates stay open.
The original first red must remain unchanged. Actual future green/output attribution
is separate work, not proved by this note.

Checks actually performed: stdlib full raw/receipt/hash/ten-frame-group/footer
analysis; Git review/head/source chronology and frozen-byte comparisons;
corrected checker AST/compile without exec; project426/122; render--check;
diff--check, all PASS. No test/checker module import, replay, binutils, native load,
child/probe, SDK install or candidate execution occurred in this review.
