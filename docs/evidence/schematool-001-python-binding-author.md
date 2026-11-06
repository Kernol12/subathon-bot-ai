# SCHEMATOOL-001 — Python native binding implementation

Author A (`/root/session_supplier_review`),6November2026. New isolated branch
`task/SCHEMATOOL-001-native-python-binding`, tree
`/tmp/subathon-schematool001-native-python-binding`, starts at root `9a70ec0`.
Own ONLY [schema_validate.py](../../tools/schema_validate.py) and this note.
Root owns tests/build/CI/progress; B separately owns the native C++ owner. Finish
the existing PR77 and stop. No candidate import/test/native/worker/compiler or
runtime execution is authorized by this source-authoring assignment.

Authority: [native process §§1/2](../contracts/schema-validation-native-process-v1.md),
[installation revision2](../contracts/schema-validation-installation-v1.md),
[public API](../contracts/schema-validation-tool-api-v1.md),
[process revision4](../contracts/schema-validation-process-test-v1.md) and the
[74-method migration map](schematool-001-native-test-mapping.md).
Read actual [binding FIRST evidence](schematool-001-native-binding-first-red.md):
at clean2a153677, root ran the different-reviewed9 methods after both-host56
foundations/native69 FIRST. Passive import passed; the remaining observations
gave41 real `SchemaNativeBindingNotImplemented` errors, no other cause or native
entry. That gate releases loader/decoder implementation, independently of public
routing. Its retained declaration is historical; these implemented helpers no
longer raise it and never manufacture an actual native31 marker.

## Loader and pure decoder source stage

The loader derives ONLY the fixed production DSO path from the trusted entrypoint,
bounded to4096 UTF8 bytes. Linux x86-64/little-endian/builtin ctypes widths are
checked before load. A non-following stat rejects missing/nonregular/short
artifacts; a genuine unbuffered builtin FileIO reads only64 header bytes and
compares device/inode/mode/size/mtime/ctime before/after against the path snapshot.
The admitted header is ELF64 little-endian/current, SYSV/GNU OSABI, ET_DYN/x86-64,
current ELF version and ehsize64. All malformed/changed/unloadable/missing-symbol
cases refuse with None before any foreign invocation or print.

The FileIO object owns its native handle, rather than obtaining a raw numeric
os.open result that could lose ownership during Python allocation. No opener
callback is supplied. The header handle closes BEFORE dlopen. If reading/stat
raises, explicit cleanup preserves that original exception object even if closing
also raises; on ordinary completion a close error is a controlled loader refusal.
The selected CPython3.10 primary source stores the open result directly in
FileIO's existing C fd field and closes it on initialization error; its fileno
boxes only an already-owned descriptor. See
[FileIO source](https://raw.githubusercontent.com/python/cpython/v3.10.12/Modules/_io/fileio.c)
and [unbuffered open/error cleanup](https://raw.githubusercontent.com/python/cpython/v3.10.12/Modules/_io/_iomodule.c).
This is a source-backed lifetime recipe, not a newly observed fault-injection pass.

No package/config file is read by these helpers. The native artifact remains a
trusted, source/build/hash-admitted local artifact: snapshot checks do not claim
to authenticate against an administrator replacing a path between preflight and
dlopen. Artifact symlinks/nonregular paths are refused before opening; concurrent
hostile administrator replacement is outside that stated trust boundary. No
alternative artifact, environment lookup, automatic build/download or search path
exists. Loader OSError/ImportError/AttributeError/ValueError return None without
error text; MemoryError and BaseException are not caught by that refusal mapping.

Actual `ctypes.CDLL` uses LOCAL|NOW and the sole versioned symbol. Its returned
genuine builtin function receives exactly four repetitions of builtin unsigned
byte-pointer/unsigned-int and builtin c_int return. No errcheck, custom converter,
PyDLL, callback, fake successful result or C entrypoint invocation occurs in the
loader. Each loader call is fresh; no validation result/cache is introduced.

The decoder is an immutable literal18-entry enum tuple with exact-type integer
admission. Native0 and2..17 map literally; reserved1 maps validation_unavailable.
Interrupted18, forbidden completed-native31, negative/unknown/non-int input all
refuse as validation_unavailable, without manufacturing a signal or baseline
marker. It opens/loads/calls nothing and owns no resource. Actual Python pending
SIGINT/negative decode-hook exceptions must propagate from the eventual route
after native quiescence.

## Public route after the separately recorded affected-condition red

The loader/decoder stage began while public routing was explicitly held. Root's
first mixed public74 baseline excluded two duplicate-identity fixture errors;
test-only correction42dd/C997 preserved the identical a1/e21 conditions and both
fresh executions. Root then explicitly released route authoring after the actual
clean945a469 run at20:05:43Z: the two affected methods reached all13 literal
observations with real public/session PIDcount0!=1, exit1/3.463s/stdoutempty,
stderr30458B/SHA`45499f9185c1ef7aa015c91f8b81e1c1cc54b8ebb63931b15db8631c511a658e`.
Read the actual receipt and `schematool-001-native-public-wire-first-red.md` at
root5a866bd. Previously valid unaffected route failures remain in the separate
archive; no tidy full replay occurred and no old FIRST is reattributed.

The public route now admits the manifest with the unchanged exact-builtin string/
URI/4096 UTF8 checks, then the unchanged three environment-presence guards BEFORE
sysconfig, ordinary CPython/ABI/venv/one-SDK-root installation admission, and only
then loads the native entry. Loader None returns environment_unavailable before
worker entry. Installation retains the original absolute sys.executable spelling;
normalization is used only to derive the directory binding, never to resolve the
venv interpreter symlink. Python no longer opens pyvenv.cfg: its bounded nofollow
snapshot/read/close belongs to the single native owner before birth.

All legacy Python pipe/fcntl/Popen/thread/traceback recovery/read/poll/kill/wait/
reap/configuration-FD code and its unused imports/constants/classes are removed.
The private `_worker_result` NAME remains for the reviewed None-before-worker
guard, but its body is now only the native buffer/call/decoder helper. It prepares
four bounded UTF8 payloads, private immutable bytes with explicit terminal NUL,
four builtin byte pointers and the eight argument values BEFORE C entry. The
byte/pointer/argument tuples remain live across exactly ONE actual builtin C call.
Lengths exclude the terminal NUL and public/trusted path admission already
ensures1..4096 payload bytes/no embedded NUL. No other thread receives/mutates the
private buffers; native source must copy them before acquiring its own resources.

Only after that call returns does `_decode_native_result` run. Known MemoryError
anywhere in public admission/buffers/loading/result/decode maps to resource_limit.
Original BaseException/KeyboardInterrupt objects propagate without manufacture,
translation or Python process cleanup; actual native cleanup is mandatory before
result conversion/unwinding. Public main, the18-value enum, exit/diagnostic mapping,
`_bounded_path`, exact help and all CLI/signatures/input priority remain unchanged.
There is no successful provider/validator substitute or public test knob.

The loader owns only its short-lived FileIO metadata read before native entry;
standard CDLL mapping lifetime is separate from per-call native PID/FD ownership.
This source makes no dlclose/loader-reference-count reclamation claim and no
successful validation cache. No arbitrary library destructor/unload is added.

Actual source-only checks PASS: Python3.10 AST/compile-without-exec; exact enum18,
main/CLI, path admission, exit mapping and environment-presence statements;
independent literal18-native-code table; exactly one C call before decoder; no
legacy process/thread/raw-FD/config-reader code/import; project426/122/current
HTML; render--check current; git diff--check. Only the two assigned paths change.
No candidate module, loader, decoder, public call, SDK or native code was executed.
Different actual code review, matching source integration, generated
native gates and passing binding/public/direct/native/SDK suites plus unconditional
latest-head both-host CI/normal merge remain. This source stage is not completion
or a supported native validation claim; no earlier FIRST is replayed.
