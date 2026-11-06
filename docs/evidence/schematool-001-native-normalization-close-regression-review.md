# SCHEMATOOL-001 — normalization close hook and regression review

Reviewer C independently reviewed author B's exact
`e1fabcca75b0733ccac997c3a125b5ad36a29636` on a fresh isolated note-only
branch. **APPROVED SOURCE for integration and compile/static inspection.**
The one new test may execute only after actual matching owner/fixture generated
code, stack/resource and artifact admission; this is not a runtime release or
an approval of the still-present normalization interruption defect.

Reviewed blobs:

| File | Blob |
| --- | --- |
| private typed header | `f986622e5633e14a24adafbbe91ec126950ae2f5` |
| common owner implementation | `058d9d127330a81c2139225fcc95678247772faa` |
| negative compile-time policy | `875fd5312c721102443ed9bf61f09631baa58ceb` |
| nonvalidating fixture | `1cda48749a1e17ab8fbc3b7c6fa24ef546bcd23f` |
| existing closed57 source | `b223e74fd76076981239c96ec007bcbc0562bb18` |
| new regression | `08e3b3dac9281eee855e89d370508fa52878a534` |

I checked the actual delta against native revision3 `d63fa44c`, its independent
C99d document review, native section6 work-EINTR18 requirement, prior owner75
source audit and unchanged public/API/process guarantees. PhaseBase20 is
appended; old1..19, public ABI, child modes, exports and capacities stay intact.
Only the closed case24/phase41/EINTR tuple is newly admitted. Phase40 is a real
observational BEFORE, not a newly injectable tuple. FaultOps accepts the new
close boundary while retaining its actual-return/ownership-ACK checks.

The owner hook preserves the reviewed source-duplicate AFTER boundary. It then
observes BEFORE40, performs the genuine close once, invalidates the slot, emits
its actual release ACK and only then calls AFTER41 with the unchanged raw result.
No kernel close0 is relabelled EINTR, and no descriptor is retried. Returning the
actual result from close_bare leaves ordinary cleanup callers' behavior intact;
unowned cleanup slots perform no operation/event. All three normalization work
contexts explicitly remain false. Thus this packet exposes, rather than fixes,
the confirmed active-work classification mistake. The later work=true behavior
change still requires the new reviewed test's actual intended red.

The one undecorated method makes one real builtin C entry call using retained
four-buffer arguments and the existing real FaultSession. It writes a small
conforming integer package; neither validation nor a successful integer is faked.
The real fallback is registered before arm. Literal31/type/poison/setup causes
cannot count as the intended red. Before phase or expected-value assertions and
before fallback, the method saves all observed owner FD fstat states and fresh
nonconsuming own-thread/group P_ALL facts. It requires exactly one kind2/41/raw0,
exactly one earlier kind4/40/raw0 release, preceding genuine acquisition and
EBADF for that original and every observed owner descriptor. Only after these
reached-close/physical-cleanup facts does it require literal18. Full shared
assert_evidence then requires no historical birth, balanced generations, actual
final39/result18, caller state and empty logs. The current bug may run the real
worker and return0; that actual wrong result, rather than an unreached hook,
fixture error, missing dependency or leak, is the prospective intended red.

I independently proved the entire old57 file is byte-identical after removing
ONLY the explicit `or encoded in (40, 41)` representation clause. Its57 literals,
12 methods, other assertions and shared helpers are preserved. This is an
explicit reviewed record-grammar extension, not a claim of unchanged whole AST.
No row is added to or removed from that frozen registry; the new program requires
exactly one discovered/executed test and ordinary unconditional unittest status.

Actually performed: full six-source/author-note/diff/contract and oracle audit;
stdlib AST3.10 and compile-without-execution of the new test; one-method/one-entry
call and exact old57 byte-preservation checks; four C++ files clang-format
`--dry-run --Werror`; project426/122, render check and diff check all PASS.
No candidate import, native load/call, test, SDK, compiler, thread, child, signal
or probe executed. Original69/FIRST and public/binding evidence remain intact.
This review releases no behavior fix, full acceptance, hosted capability, task
completion or PR77 merge. Finish this existing PR through gates, then stop.
