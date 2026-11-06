# SCHEMATOOL-001 — actual local bootstrap artifact, static evidence

Root,6 November2026. Actual C91bec03 source/build approval (root ee3ebde) precedes
this rebuild and nonexecuting inspection. New seventh binding source is96eefa80;
raw child annotation/cleanup bodies are unchanged from their exact approvals.
Native executable/bootstrap/--list/probe has NOT run. Runtime is still held.

`cmake --build --preset schema-transport-debug --verbose` exited0 with zero
warnings, regenerating the CMake configuration and rebuilding/linking the changed
test executable with test-only -ldl. [Exact build log](schematool-001-native-bootstrap-build.log.txt).
The reviewed static audit then exited0 against these ACTUAL artifacts:

- Foundation:c618c4d6abedfc82661955cdee56c9ad8c9c9ac72cd66ac00ca5766fdbb9c90e.
- Reporter:bd3e5ea0c6af3de7e1abcbd80d4a74f91cf8f23755dee1a85c497798084c22c1.
- Inert DSO:bf797690cc4f7d6341b10b4e22bb581f47572d418eea0e956af17b18e54fcac2.
- Actual audit-process mapped libc:02c8c3d06180beee9c7d0392aa85736c7d19f274a81e7f8f0b3d9e28e0641524.

[Full scoped graph/hash output](schematool-001-native-bootstrap-static-graph.txt).
The unchanged static controls also passed8methods/.005s/exit0 before this rebuild;
their historical FIRST10-failure raw was not replayed or modified.

Actual GNU objdump/readelf supplemental inspection closes the local callsite
scope omitted by the generic graph check, subject to DIFFERENT actual review:

- [Clone return edge](schematool-001-native-bootstrap-entry-edge.txt): call435
  through syscall@plt at5912; store returned RAX, compare with0, branch on NONZERO
  to5937; zero falls through only LEA/MOV then direct raw-child call at5932.
  The errno/parent paths start after that noreturn child call. No TLS, helper,
  allocator or external callback lies between successful child return and entry.
- [Relocations](schematool-001-native-bootstrap-relocations.txt): syscall has
  R_X86_64_GLOB_DAT slot23fa8 with GLIBC_2.2.5. The [PLT stub](schematool-001-native-bootstrap-plt.txt)
  at4440 jumps through exactly23fa8. This is the actual probe call target.
- [Address taken by the binding test](schematool-001-native-bootstrap-got-pointer.txt)
  loads that SAME23fa8 slot at8c23. Therefore the source-reviewed bootstrap's
  dladdr/dlsym/dlvsym comparisons observe the same actual callable address used by
  this binary's clone callsite; they are not an unrelated second lookup.

The initial mixed demangle/mangled --disassemble selector produced no matching
output; non-demangling objdump with the actual nm symbol then produced the exact
preserved edge above. No failure was treated as a passing observation.

This artifact proof is LOCAL GNU16.2.1/GTest1.18.0 only and is not self-approval
for execution. C must independently inspect the actual bytes/relocations/hashes,
then explicitly release NO-CHILD bootstrap. Its actual result and library binding
still precede any kernel-probe admission. Both selected CI host artifacts need
matching concrete proof before their probe phase; the current full workflow is
unpublished. No native owner, SDK/worker execution,31 behavior red or product
acceptance follows. Finish PR77 then stop.
