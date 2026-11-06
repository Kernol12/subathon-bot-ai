# SCHEMATOOL-001 — independent Python native binding source review

Reviewer C, fresh isolated review from actual A
`4476be37f0fa94f28db767198d4a34625d8e3c55`, source
`f6dc5c2f2a6c3b07ab4731159720d418d691e36d`, author note
`f1f64d0846a239357b29e4e7e5c04d59efc74248`. ONLY this note changes.

**APPROVED: exact corrected Python binding/routing source. Released: the focused
reviewed loader-cleanup regression ONLY, with synthetic header/CDLL tripwire and
no foreign entry.** Root must integrate this source with the actual regression.
No new-owner ELF load, binding9 run against unaudited new artifacts, public/native
integration, hosted capability, implementation completion or merge is approved.

Read full a8 source/author note, actual4476 correction, accepted native/process/
installation/public contracts, binding9/public74 source reviews and actual reds.
Binding9 and corrected route13 are separately attributed in Cec3; native69's
actual-return31 attribution is C746. Real loader regression/root source review
and actual reached red precede this correction. Historical baselines stay intact.

Public18 enum, main/CLI/exit mapping, exact-string path/URI/4096 UTF8 admission
remain unchanged. The three override-presence guards run before sysconfig;
ordinary CPython/ABI/venv/root admission precedes native loading. Original
sys.executable spelling is preserved. Only native owns pyvenv.cfg and child/
pipe/clock/read/poll/kill/reap resources; old Python process/thread/recovery/raw-FD/
configuration-reader code is removed. Python retains trusted installation metadata
admission, not package/SDK validation or a positive result cache.

Loader admits only the fixed trusted repository artifact: bounded path, nonfollow
regular-file stat, bounded64-byte unbuffered FileIO read and identity snapshot
before/after; ELF class/endian/type/machine/version/header size checks before
LOCAL|NOW CDLL and sole symbol setup. FileIO closes before foreign loading.
None/mapped loader failures are quiet; original MemoryError/cancellation propagate
to public resource_limit/unchanged exception handling. These checks bind the
admitted trusted build; they do not authenticate an administrator replacing its
path after preflight. Standard CDLL mapping lifetime is not claimed reclaimed,
and no unsafe custom unload/destructor is added.

The genuine builtin function has four pointer/uint pairs and c_int, no errcheck,
custom converter or callback. Four private immutable NUL-terminated buffers,
pointers and eight argument values exist before the ONE actual C call and stay
alive across it. Only afterward runs the immutable18-entry exact-int decoder:
0/2..17 map literally, reserved1/interrupted18/forbidden31/unknowns refuse as
validation_unavailable. It does no IO or validation and creates no fake signal.
Public catches only known MemoryError; original other BaseExceptions propagate.
The actual native source must separately prove quiescence before C return.

## Actual acquisition defect and corrected ownership boundary

My initial a8 audit found source=open OUTSIDE the header cleanup try. A negative
event after binding could retain that FileIO through the original traceback.
Different root-reviewed new regression then genuinely observed BOTH prebuilt
KeyboardInterrupt/MemoryError cases fail ONLY real fstat[0] versus EBADF[9] at
clean78; raw2470 bytes/SHA256
`c702a6b9801791b882c15f29c0ee69b40b29b35a2bd6000c8c14fae498ca3c64`.
Trigger/type/identity/traceback/quiet/no-CDLL oracles already passed; fixture close
followed raw observation. C544 separately accepts that reached source bug red.

Unimported aebe's with-only proposal was withheld because close/__exit__ could
mask the original exception. Actual4476 instead initializes source=None, makes
the SAME open/assignment the first protected-try statement, and guards the SAME
existing exception-preserving close/bare-raise with source-is-not-None. Ordinary
close and all protected read/header/body behavior remain. Source before first
binding owns no FileIO; after binding the negative trace event is protected.
Failed initialization is handled by FileIO's native owner. All other module ASTs
are identical to a8. This fixes the observed boundary while preserving original
exception priority; it is not approval of the rejected intermediate proposal.

Read primary FileIO sources for
[CPython3.10.12](https://raw.githubusercontent.com/python/cpython/v3.10.12/Modules/_io/fileio.c),
[3.12.3](https://raw.githubusercontent.com/python/cpython/v3.12.3/Modules/_io/fileio.c),
and [3.14.9](https://raw.githubusercontent.com/python/cpython/v3.14.9/Modules/_io/fileio.c):
native open ownership resides in the existing object, initialization errors close
it, and close attempts internal_close even if base close raises. No raw returned
numeric FD needs Python boxing before an owner exists. This is selected-source
support, not an arbitrary global-OOM or async-cleanup certificate.

Actual reviewer operations: full source/diff/contract/primary-source/lifetime/
bound/order analysis; Python3.10 AST and compile-without-execution, exact enum/
main/path and correction-only AST comparisons; project426/122/render/diff PASS.
No candidate module/helper/test/trace/native/SDK/DSO, compiler, subprocess, thread,
signal or probe ran from this reviewer. Focused actual PASS and then applicable
source/generated/full acceptance/latest-head CI/normal PR77 merge remain.
