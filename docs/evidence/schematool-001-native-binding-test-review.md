# SCHEMATOOL-001 — independent Python native-binding test/interface review

Reviewer B, 6 November 2026. Fresh isolated
`review/SCHEMATOOL-001-native-binding-tests` from actual
`c38c24846de39343645e347c76cb8aea7ac2430d`. ONLY this note changes. A authored
the reviewed tests/declarations; B approves no own fixture/Ops/child source here.
Scope remains PR77 then stop. Actual reviewed blobs:

| Path | Blob |
| --- | --- |
| `tests/schema_validation/native_binding_acceptance.py` | `e0485509b4c36d4fa723d14d3d0d295fbcb7f170` |
| `tools/schema_validate.py` | `3fc42fee391a95dcde2b7e761cda78416e5cae87` |
| binding test author note | `8c21f200367943483573eae155a16e25ff11d970` |

**ONE FINDING: complete test-source approval pending correction.** Positive ABI
test line159 asserts `entry is getattr(libraries[0], SYMBOL)`. Native§§1/2 require
the actual builtin CDLL symbol/function with the exact ABI, not attribute-cache
object identity. Official [CPython3.10 ctypes documentation](https://docs.python.org/3.10/library/ctypes.html#ctypes.CDLL)
permits attribute OR index lookup: attribute access caches its wrapper, index
access produces a new wrapper. Thus a genuine correctly configured
`library[SYMBOL]` implementation fails the current assertion even for the same
real symbol. Record the genuine return of transparent saved-real `__getitem__`
calls during loading and assert the returned entry IS that actual lookup object,
or otherwise prove genuine symbol provenance without requiring this unfrozen
cache detail. Keep exact library/path/mode/ABI/no-callback assertions and never
invoke a function or substitute a fake successful library/function. Root/A were
notified; no test execution established this source-based finding.

The remaining reviewed scope is coherent. Nine natural methods cover passive
import, real trusted DSO/function configuration without invocation,3 missing/
empty/short artifacts,7 independently malformed ELF headers,3 negative loader
exceptions, actual trusted missing-symbol lookup, public None refusal,18 closed
integer mappings and7 interrupted/unknown integer refusals. Last25 are ONLY pure
post-quiescence decoder observations; raw0 mapping is not fabricated validation
success. Native1 and18/unknown map to validation_unavailable; actual C31 marker is
deliberately not invented by passing31 into a private decoder.

Synthetic repos patch only trusted `__file__`; their artifacts are real files at
the fixed derived path. The seven complete malformed headers must actually be
read through the delegated file/raw I/O recorder before None is accepted. Recorded
bytes reach the independently mutated field; CDLL is both denied and checked
uncalled afterward, so swallowing a guard cannot make a load attempt green. The
recorder performs real reads and tracks FD device/inode, with cleanup if wrapping
fails; it is not a loader/parser or successful validator. Missing/empty/short
inputs cannot supply callable code. The positive and missing-symbol cases load
ONLY the actual trusted admitted DSO; missing-symbol injection returns no callable.
No C entry is invoked anywhere in the authored tests.

Known MemoryError and KeyboardInterrupt loader failures must propagate the SAME
exception instance; ENOEXEC produces quiet None. Public None refusal first calls
the actual stub to preserve its named missing-binding FIRST, then independently
checks one loader call, environment_unavailable, no old worker and no output.
Pure mapping guards have post-call no-call assertions, not only swallowed raising
mocks. Public wrong-type/poison/no-eager behavior remains in the independently
reviewed74 ports: the private decoder receives actual builtin c_int results, and
these25 integer cases do not invent a general public object-conversion surface.

The ONLY16 additive production lines define distinct
SchemaNativeBindingNotImplemented and two single immediate-raise functions.
There is no working loader, decoder, routing, native call, native31 observation,
SDK/config read or resource operation. The named binding marker differs from
both historical wrapper and actual native31 exceptions. Both private attributes
exist, so intended red need not rely on missing attributes/signatures. Passive
import may pass as infrastructure; future FIRST eligibility still needs matching
admitted environment/artifact/build/source gates and separate actual attribution.

Read root/tools/tests instructions, actual native§§1/2, installation2, public API,
74 oracle map and author note. Actual STATIC checks: stdlib ast.parse PASS;9
undecorated unique methods; all18 literal mappings and7 literal unknowns match
the frozen table; no `entry(...)` call. Removing ONLY the3 new declarations
reproduces the entire old parent AST exactly; both new functions have ONLY the
immediate binding-marker raise. No candidate/test/ctypes/SDK import, compiler,
load, native function, child, signal, audit or candidate executable ran in this review.
Project/render/staged diff PASS426 tasks/122 decisions/current HTML. Actual
corrected-source rereview and explicitly released FIRST still precede bodies;
native69, helper/reporting/current both-host gates, public74/SDK/source/security,
latest-head CI and normal PR merge remain separate. No future body is approved.

## Actual58a correction — source approved

Reviewer B, fresh isolated
`review/SCHEMATOOL-001-native-binding-test-correction` from actual A
`58a3243ee3ebcf8d065a92c7eaee209b25dcad93`, retaining this prior findings note.
**APPROVED: exact binding9 tests and minimal16 declaration lines**, corrected test
blob`42ab9cd058b37e4cbdabe54772769ebd272f3679`; production declaration blob
`3fc42fee391a95dcde2b7e761cda78416e5cae87` remains byte-identical.

The positive ABI case now transparently instruments the actual CDLL class's
`__getitem__` with a normal bound method. It invokes the SAVED REAL method on
the actual library/symbol first, records that genuine return and returns the
same object unchanged. The returned entry must be identical to one actual lookup
return for the exact loaded library/versioned symbol. Both documented attribute
and index forms traverse this seam; neither a cache identity nor lookup count is
imposed. No native entry is invoked and no callable/result substitutes for the
genuine lookup return. Symbol provenance resolves the sole finding above.

All existing exact trusted-path/CDLL LOCAL|NOW/library-count/builtin eight-argument
types/c_int/no-errcheck/non-PythonAPI assertions remain. Independent stdlib AST
comparison removes ONLY the positive ABI method and reproduces the entire old
c38 test module exactly: other8 methods, helper lifecycle/recorders, all25 mapping
integers/ELF cases/module definitions remain unchanged. Nine methods remain; no
`entry(...)` invocation exists. Current two immediate binding-marker stubs and
their distinct class are unchanged; the old public-parent AST is still preserved.

Actual AST preservation, project/render/staged diff PASS426 tasks/122 decisions/
current HTML. No candidate/module/ctypes/SDK import, compiler, load, native/test/
audit/helper/thread/child/signal execution. Approval is SOURCE only for the later
explicitly released binding-specific FIRST under matching admitted environment/
artifact/build gates; it is not actual native31 or a release of owner69. Future
loader/decoder/public routing bodies, native owner, source/security/74-port and
latest hosted/normal-merge gates retain their separate requirements.
