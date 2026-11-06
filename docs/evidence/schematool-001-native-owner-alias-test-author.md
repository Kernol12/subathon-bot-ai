# SCHEMATOOL-001 — literal missing-node ABI alias test source

Author A,6November2026. Root released TEST SOURCE ONLY from integration
0b6a2c5 after actual selected GNU11/13 static gates refused missing compiler
alias nodes. Fresh isolated branch
task/SCHEMATOOL-001-native-owner-alias-control, worktree
/tmp/subathon-schematool001-native-owner-alias-control. Only the existing
literal owner-audit control file and this note change. Production auditor, native
owner, build/CI/pins and all other tests remain unchanged. PR77 only, then stop.

Read the actual hosted static failure log
/tmp/schematool001-0b6a2c5-hosted-static-fail.log. Both lanes stop before native
execution at compiler edge/no-node refusal for RawOps C1, Owner<RawOps> C1 and
Owner<RawOps> D1 names. This is an observed static metadata/profile problem,
not a validation/owner failure or product red.

The contract still requires real certified CI/SU frames bound to actual ELF
identities and the whole inherited32768-byte limit. A complete constructor C1
or destructor D1 identity may borrow only its ABI-counterpart C2/D2 frame when
BOTH actual ELF symbols exist at the SAME address and the counterpart carries
the matching real CI/SU record. No missing-node frame0, namespace wildcard,
generic name replacement or invented frame is acceptable. The proposed narrow
identity rule is _ZN-prefixed and exactly one C1E→C2E or D1E→D2E token; root
owns the actual implementation after separate test review and intended red.
[Primary ABI constructor/destructor grammar](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-special-ctor-dtor)
distinguishes complete/base constructor C1/C2 and destructor D1/D2 variants;
their name alone is not proof of equal code/frame. The actual same-address
ELF and matched compiler metadata requirements supply that additional evidence.

ONE additive natural method:
test_missing_abi_alias_node_requires_same_elf_counterpart_frame.
Constructor positive removes ONLY the existing triangle C1 node, retaining its
incoming and alias edges and the real24-byte C2 node/SU frame. A parallel
independent destructor literal has only the D2 node and D1 incoming/alias edges.
For each, literal expected frames are entry16/base24, graph entry→base and
whole48 (16+8 return+24). Alias edges at one verified address must not invent a
second call/frame. The two positive contexts explicitly require complete exact
frames and graph, not merely a nonthrowing parse.

Each family also refuses ten finite controls: different alias/counterpart ELF
addresses, either missing ELF symbol, missing counterpart CI node, missing SU
frame, unmatched SU frame, missing ordinary caller, unknown missing target,
non-_ZN prefix and ambiguous multiple ABI tokens. The ordinary caller remains
required; only the positively proved constructor/destructor alias is eligible
for the narrow missing-node case. No expected values come from the auditor.
All six previous method ASTs/statements/assertions, fixtures/imports and module
body remain exact. Seven natural methods total; no new helper or module-load
operation.

Source-only checks PASS: Python3.10 AST parse/compile-without-execution; removing
the ONE added method restores the entire old module AST (all six original
methods/imports/fixtures exact); literal missing-node string edits and48-byte
arithmetic inspected without invoking the auditor. Production auditor is byte
exact. Project426/122, progress render--check and diff--check PASS.
No new test/checker import or execution, compiler,
binutils, native ELF load/call, SDK/worker/child/thread/probe occurs here.
Actual different C test-source review and a genuine reached literal red must
precede production parser changes; later matching artifact and hosted gates
remain separate.
