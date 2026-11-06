# Actual hosted implicit constructor and destructor aliases

Exact0b6a2c5 native run42306061441 reports the same three missing GNU graph identities on BOTH selected hosts, after successful build/controls and before any native execution:

```
_ZN8subathon17schema_validation6native6RawOpsC1ERKNS1_7RequestE
_ZN8subathon17schema_validation6native6detail5OwnerINS1_6RawOpsEEC1ERS4_RNS2_7ScratchEl
_ZN8subathon17schema_validation6native6detail5OwnerINS1_6RawOpsEED1Ev
```

These identify C1 constructors/D1 destructor; the previously admitted GNU16 graph explicitly records triangle aliases, whereas this diagnostic proves missing-node references on GNU11/13. Their apparent category is not sufficient proof. A correction may defer ONLY a missing callee with a single nested Itanium C1E/D1E token to a present, real-frame C2E/D2E counterpart. Binding must require both mangled symbols at the SAME actual ELF address, matching real CI/SU frame/source, never a synthesized node, guessed frame, frame0, arbitrary external endpoint or dropped edge. Missing callers, absent/unframed counterpart, missing/different-address symbols and unrelated missing targets must still refuse. Existing explicit triangle alias proof remains intact.

A authors additive literal controls; C separately reviews those actual tests before new-only intended red. Root production correction waits for that reached red, then actual different source review, same controls and both combined local static gates before fresh hosted execution. This is compiler representation support under the same actual-address/frame proof, not a native behavior/flags/stack budget change. Both latest full66+SDK14 hosted acceptance and all merge gates remain open. The rejected broad metadata export was not applied; this diagnosis used only approved bounded public-code identities.

Clarification for A’s actual additive control: a missing alias MAY occur as the source of its own exact C1→C2 or D1→D2 identity bridge, as in the existing explicit-alias fixture with only the triangle node omitted. It still requires the SAME actual address and real counterpart frame; the bridge represents one body, not an additional recursive call. Every ordinary missing caller and every other missing-alias outgoing edge must refuse. This refines the earlier “missing callers” shorthand; it does not permit a generic orphan or drop an unproved edge. The [Itanium ABI constructor/destructor naming specification](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-special-ctor-dtor) identifies these variants, but does not itself prove any two implementations share code: actual symbol-address/frame equality remains mandatory.

Aab7 tests imported7c53; Ce25 actual test/clarification approval imported5ab9304. FIRST ONLY named new method at clean5ab ran1/.008s/exit1 with TWO reached positive subtest RuntimeErrors, exact missing `_ZN1AC1Ev` and `_ZN1AD1Ev`; all20 independently literal refusal checks passed. Raw `/tmp/schematool001-implicit-alias-first-red`. Root correction now defers only the exact single-token framed counterpart, requires every missing source to be its own exact identity bridge, then requires real CI/SU definitions plus BOTH actual ELF symbols at SAME address before binding. No synthetic records/zero frames/unknown edges, and all old explicit-alias/SU/source/address/cycle/stack checks remain. Different actual source review precedes any checker execution/publication.

[Immutable actual alias-control red receipts](schematool-001-implicit-alias-first-red.tar.gz): 946 bytes SHA256 `d535b6358ec2ae166373dc66aacece753ffadc4ba0fa8143ea56c662afbde461`. These are reached auditor representation failures, not a new native-feature red or a hosted success claim.
