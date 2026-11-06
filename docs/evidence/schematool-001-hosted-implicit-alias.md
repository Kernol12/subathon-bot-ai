# Actual hosted implicit constructor and destructor aliases

Exact0b6a2c5 native run42306061441 reports the same three missing GNU graph identities on BOTH selected hosts, after successful build/controls and before any native execution:

```
_ZN8subathon17schema_validation6native6RawOpsC1ERKNS1_7RequestE
_ZN8subathon17schema_validation6native6detail5OwnerINS1_6RawOpsEEC1ERS4_RNS2_7ScratchEl
_ZN8subathon17schema_validation6native6detail5OwnerINS1_6RawOpsEED1Ev
```

These identify C1 constructors/D1 destructor; the previously admitted GNU16 graph explicitly records triangle aliases, whereas this diagnostic proves missing-node references on GNU11/13. Their apparent category is not sufficient proof. A correction may defer ONLY a missing callee with a single nested Itanium C1E/D1E token to a present, real-frame C2E/D2E counterpart. Binding must require both mangled symbols at the SAME actual ELF address, matching real CI/SU frame/source, never a synthesized node, guessed frame, frame0, arbitrary external endpoint or dropped edge. Missing callers, absent/unframed counterpart, missing/different-address symbols and unrelated missing targets must still refuse. Existing explicit triangle alias proof remains intact.

A authors additive literal controls; C separately reviews those actual tests before new-only intended red. Root production correction waits for that reached red, then actual different source review, same controls and both combined local static gates before fresh hosted execution. This is compiler representation support under the same actual-address/frame proof, not a native behavior/flags/stack budget change. Both latest full66+SDK14 hosted acceptance and all merge gates remain open. The rejected broad metadata export was not applied; this diagnosis used only approved bounded public-code identities.
