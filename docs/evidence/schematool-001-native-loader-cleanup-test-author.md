# SCHEMATOOL-001 — real loader-header cleanup regression source

Author A (`/root/session_supplier_review`),6November2026. New isolated branch
`task/SCHEMATOOL-001-native-loader-cleanup-regression`, tree
`/tmp/subathon-schematool001-native-loader-cleanup-regression`, starts at actual
Python implementation `a8ae7779d39f9bb72674490cca1b4a321fa7d58d`.
Own ONLY [native_loader_cleanup_regression.py](../../tests/schema_validation/native_loader_cleanup_regression.py)
and this note. Root/tests instructions read. Production loader/binding/public
source, existing9 tests, native owner, worker, pins, build/CI/state remain exact.
This is PR77 only; finish that open PR and stop.

C's actual source review found `source = open(...)` precedes the loader's protected
try. An exception at the next Python line can escape without closing the genuine
FileIO; the original exception traceback can retain that still-open file. This
violates the [native binding](../contracts/schema-validation-native-process-v1.md)
and [public lifetime/exception contract](../contracts/schema-validation-tool-api-v1.md):
known allocation/cancellation must not expose leaked handles or replace the
original cancellation identity. This is a new implementation defect, not a
requirement change or reason to weaken any original baseline assertion.

## One method, two independent prebuilt negative exceptions

`NativeLoaderCleanupRegression.test_prebody_fault_closes_real_header_before_original_exception_escapes`
has two literal subcases: original KeyboardInterrupt and original MemoryError.
Neither is an actual OOM/stress event, fake native result or successful validator.
The expected kernel errno is independently literal `errno.EBADF`; the original
exception object and its real traceback must remain intact through that oracle.

Setup writes one actual64-byte synthetic ELF header beneath an actual temporary
`tools/schema_validate.py` location and the EXACT fixed derived artifact path.
The real target module is loaded BEFORE the fault/patch scope. The TEST-only
`__file__` convention already admitted for binding9 is reused; no production
artifact getter/argument/environment knob is added. All fixture creation is
before injection. The only open replacement transparently calls saved builtin
open, retains the REAL unbuffered FileIO/native FD, and returns that unchanged
object. It closes only on an independent recorder-setup failure; such a failure
cannot satisfy the required prebuilt-exception/one-trigger assertions.

The [documented sys.settrace line-event mechanism](https://docs.python.org/3.10/library/sys.html#sys.settrace)
is scoped to the actual loader's exact code object. It raises the prebuilt
exception ONCE at the first line event where that frame's `source` local is the
saved actual file. There is no source line number, timing/sleep, bytecode offset,
injected fake file, filename-derived success or callback while a native owner is
active. Other Python frames are not traced. The prior trace function is restored
in finally and its actual identity is asserted. Raising from a trace function
can disable tracing; cleanup/restoration does not rely on it remaining active.

The caller retains the original caught exception AND traceback strongly. Before
any new FD opens, it directly fstats the saved numeric descriptor and records the
actual errno/live state. Only after collecting that ownership fact does it assert
one genuine FileIO, one trigger, exact original exception identity, a live retained
traceback, restored trace identity, zero CDLL calls, quiet Python streams and EBADF.
The CDLL guard also has a post-call no-call assertion; swallowed attempts cannot
pass. Synthetic ELF bytes are never loaded/called, and no SDK/worker/child exists.

Explicit fixture disposal closes the saved actual file ONLY in the outer finally
AFTER raw facts/oracles; the old live-FD observation cannot be rescued green by
cleanup or GC. Only after disposal is the original traceback cleared. A correct
guarded acquisition must close even though the test/traceback keep the real file
object alive. The new regression exposes lifecycle behavior, not an implementation
generated expected outcome. The target private local name identifies the concrete
review finding, without requiring a particular source line or future body layout.

## Gates and checks

Actual source-only checks PASS: Python3.10 AST/compile-without-exec, one natural
method/two prebuilt exception rows, actual-code/local-object selection without
line constants, byte-exact production/binding9/public-fixture preservation;
project426/122/current HTML, render--check current and git diff--check.
The actual one-method/two-subcase source
needs different review before root may run it against unchanged a8. A reached
raw live-FD/EBADF assertion, with all trigger/identity/setup checks passing, is
the intended red; missing loader attributes/platform/setup/recording failures
are not. Production correction is explicitly NOT authored here and remains
gated on that actual focused red, then separate code review.

No target/test module import, tracing installation, file fixture/control execution,
native load/call, worker/SDK/child, compiler or test run occurs in this assignment.
All existing nine binding tests and public74/native69 source/oracles remain
unchanged, and their earlier valid receipts remain scoped evidence. Different
source review, focused red/fix attribution, passing full acceptance, actual native
generated/source/resource audits and latest-head both-host CI/normal merge remain.
