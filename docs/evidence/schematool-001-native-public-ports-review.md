# SCHEMATOOL-001 — independent public74 native test-port review

Reviewer C, fresh isolated review from actual A
`4b7d5dcf0a38dad4208224b9b8375404b2e3ba86`. ONLY this note changes.
This is PR77-only source/oracle review; no candidate execution or future binding
or owner implementation approval.

## Initial actual source findings

Initial helper blob `bfa9c1f5cb69a53aa6f10e9fa930b11060a390c1` is not yet
approved for execution. Root and A were notified of these bounded source gaps:

1. public_fault relies on FaultSession's raw FD capture without Python stream
   capture. Buffered Python stderr without newline can survive the raw file-size
   check, flush after descriptor restoration and disappear into the driver's
   otherwise permitted unittest stderr. Retain StringIO stdout/stderr capture
   around the actual public call, then require both empty after raw evidence.
2. postreturn_negative has Python StringIO capture but no raw FD capture. A raw
   C write to stderr can likewise enter permitted driver stderr. Capture actual
   descriptors as well; finish capture setup before the independent FD inventory
   baseline and actual call, then require both raw files empty before restoration.
   Keep raw0, exact prebuilt negative exception, FD/no-child/handler/mask and
   MemoryError outcome assertions intact.
3. Its actual31 hook raises the historical SchemaValidationNotImplemented.
   Native contract section2 requires a distinct named native-unimplemented
   marker, without reattributing this new boundary to the historical FIRST.
   Preserve actual31 using the matching native marker declaration.

These are source-derived counterexamples/contract findings, not executed leak
or marker observations. No assertion is weakened and no production fix is
authorized. The initial candidate and all prior raw/review evidence are retained.

## Preserved semantics and remaining execution gates

I read all six changed files, original helpers/callers, accepted processr4,
nativeR2, public/logical/context contracts and the complete74 migration map.
Independent Python3.10 AST/compile-without-exec checks preserve all74 ordinary
names: wrapper64, cleanup2, spawn2, context3 and unchanged lexical3. Exactly61
wrapper test bodies plus package/outcome/cli/script_result/setUpClass/tearDown
are AST-identical. Its three changed test bodies are help, environment guards
and native ENOSYS. Both spawn caller bodies and all context/lexical bodies are
identical; direct-worker observations precede the seven new real public parities.

Restricted literal/loop AST extraction independently reproduces all23 original
wire inputs, order and expected outcomes (3 activation,7 outcome,2 status,10
setup,1 deadline) and the new helper's30 unique closed rows. No production result
generates an expected value. Each fault row launches one real fresh test process,
uses actual builtin CDLL entry via only the private loader replacement, and
checks actual exit0 plus an exact completion receipt after all inner oracles.
The receipt is test-framework completion, not native/public validation success.
No stderr text is interpreted as31 or accepted success.

Written processr4 reasons coherently replace unobservable Popen/clock/close
patches with real native25/26 and separate actual production0 post-quiescence
negative complements. No Python callback may run in C ownership. The native
fault path must retain actual raw cleanup/status/phase proof before fallback;
the complements must require actual0 before throwing, rather than using a
resource result that could mask incorrect conversion. G loader/CDLL tripwires
have explicit no-call assertions, preserving prior input/help/setup guarantees.

The matching native57 helper is a prerequisite: actual stable-PID978 plus
3a055ac stronger progress/acquisition corrections, source35cff4b7 and different
Bdbd64d1 review. This candidate's inherited earlier helper alone is insufficient.
Root must integrate that actual dependency and all source/build/admission gates
before execution. BOTH selected-host real facilities must pass; current2ff CI
fails closed at generated notrack, so no FIRST permission follows this source
assessment. Old FIRST63/261, new64th and regression receipts remain historical.

Against the old Python binding, create=True prevents a missing new private
attribute from becoming a setup error. An unused native loader/decoder instead
fails an explicit actual-call/evidence assertion; it cannot vacuously pass. A
separately reviewed inert private route to actual stub31 may establish precise
new missing-native provenance before the new intended baseline. Any missing
artifact/import/ABI/arm/readiness/fixture/timeout error is invalid red. Binding
behavior implementation still waits actual reviewed new intended red.

## Actual correction rereview — approved test source

Reviewer C, fresh isolated review from actual A
`92636d9d0f4ca8a3bc96793cdbb5dea63ecf3053`.
**APPROVED: exact public74 test-port source/oracles for integration and later
new baseline, subject to the explicit prerequisites above.** No immediate
runtime, binding behavior, native owner or capability approval follows.

Reviewed corrected helper `978178009a8653d0769b245404e862bc52b9668c` and
author note `0c3140c43cb5728f3ad8f0fa802f9e197a14ff50`. The five other public
sources are byte-identical to4b: wrapperbe346c5b, cleanupf894c474, spawnb7b799b1,
context2896177d and unchanged lexicalfc826843. Their preserved74 inventory,
all61 ordinary wrapper bodies/six shared methods,23 literal tuples/order/
classifications,30 closed helper selectors and all context/public parities retain
the findings-free assessment above. Old G bodies reproduce exactly after
removing only the additive native-loader/CDLL guards and their no-call checks.

All three findings are resolved in actual source. public_fault redirects both
Python streams through raw session.assert_evidence, then requires them empty.
Thus neither buffered Python stderr nor raw descriptor output can disappear
into allowed framework diagnostics. postreturn_negative now sets up both raw
temporary files and saved stdio duplicates before its FD inventory baseline and
real call. Capture remains live through raw0, exact exception/MemoryError outcome,
FD/no-child/handler/mask and Python/raw-empty assertions. No flush or new capture
FD is introduced after return. ExitStack restores each original descriptor and
inheritable flag before closing its saved duplicate; setup and exceptional exits
retain restoration/closure. This is fixture cleanup, not assistance to native
ownership or a successful decoder substitute.

The actual31 branch raises matching native.SchemaTransportNotImplemented with
the fixed native-unimplemented message before unreached raw0/identity assertions.
It does not relabel the old FIRST or synthesize a return. Every previous helper
assertion remains: public_fault8→10 assertion calls, postreturn_negative14→16.
Only those two functions and one new raw-capture contextmanager change; all
other helper definitions, literals/imports and five old files remain exact.

For execution root must bind this approved source to actual35cff native57
helpers/different B review, current admitted artifacts and real BOTH-host green,
then review the exact minimal private declarations/inert31 route before assigning
the new baseline. Record actual cause and new source identities; do not count
setup/missing-file/library/helper failures or project an old receipt. Native-owner
57/direct12 and public74 observations remain distinct. Implementation, security/
source review, real full acceptance/latest-head CI and normal PR77 merge still
follow their gates; finish77 then stop.

Actually performed: complete original/corrected source/diff/contract/mapping
review; stdlib restricted literal extraction, Python3.10 AST and compile-without-
execution, byte/AST preservation and assertion-superset checks. Project426/122,
render--check and diff--check PASS. No authored module/target/SDK import, test/
helper/audit/native execution, DSO loading, compiler, subprocess, thread, child,
signal or probe occurred from this review.

## Actual identical-wire fixture correction after first observation

Reviewer C, fresh isolated review from actual A
`42ddf02f27ef23c81b5b65b53b018f2091cc54df`.
**APPROVED: corrected resolver source and only the two affected wrapper methods'
new reached-boundary observation after integration.** Do not rerun the full74
before implementation just to replace historical output. Actual revised red
attribution remains a separate gate; no route body is approved by this note.

Root's actual new public FIRST exposed a fixture defect I missed in C26:
preserving all23 literal observations did not imply unique tuple lookup. a1/e21
are identical empty-A/ok+LF/normal0/no-deadline/no-death conditions, both expecting
validation_unavailable. The prior len(selected)==1 rejected that legitimate
condition before the public observation. Those fixture AssertionErrors are
invalid route-red evidence. Preserve other valid actual missing-route failures
and original FIRST bytes; this is a written test defect correction, not behavior
change or a weakened expected result.

Reviewed helper source064258ad38533dae987121d7f5e6a5e531f3951f and author's
written reason. Only protocol_row changes. It now requires a nonempty exact
five-field tuple match AND agreement of every matching independently literal
expected value; unknown conditions or inconsistent/wrong expected values still
refuse. The first equivalent private name is deterministic a1. Actual fixed
producer1/21 emit the same activation/outcome/status bytes. Both original outer
calls remain distinct fresh test processes with their unchanged expected enum;
no call is combined, cached, skipped or converted to success. Direct57 retains
its independent case21, and e21 remains a selectable closed helper ID.

Independent literal/AST checks find23 rows/22 distinct conditions, the sole
duplicate pair a1/e21 and common expected value. Every other helper definition,
all74 method sources and native57 bytes are identical; Python3.10 AST/compile-
without-execution PASS. Root may run ONLY
test_worker_activation_missing_extra_and_malformed_refuse_forged_ok and
test_worker_setup_refusal_before_activation (their3+10 original observations),
against the reviewed old route/current admitted artifacts, to replace the
affected fixture causes with genuine boundary assertions. No synthetic31 or
extra inert relay is needed. Missing setup/artifact/arm/import/timeout still is
not valid red; actual new causes must be attributed before routing behavior.

Project426/122, render--check and diff--check PASS. No candidate helper/module/
target import, test/audit/native/SDK execution, compiler, thread, child, signal
or probe occurred from this reviewer. Earlier C26 approval is corrected only
for this real fixture defect; all unaffected valid reviews remain preserved.
