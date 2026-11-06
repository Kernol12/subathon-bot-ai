# SCHEMATOOL-001 — actual native compilation, no probe execution

Root, 6 November2026. This is COMPILE/STATIC evidence only. No native test binary,
case-list/discovery, loader test, kernel probe, worker or native transport ran.
The only new behavioral execution is the separately reviewed mocked-text static
audit control red, not a kernel/owner observation.

Actual C ac1638f approval (rootdc25669) allowed configure/build/static inspection
of packet299 while holding runtime for fixture cleanup and generated-code gates.
Ran `cmake --preset schema-transport-debug
-DPython3_EXECUTABLE=/tmp/subathon-schematest001-py314/bin/python`: exit0,
GNU16.2.1, Python3.14.9, Linux x86-64, Ninja. Actual link selects system GoogleTest
1.18.0 under the unchanged >=1.17 infrastructure; no pinned1.17 observation is
invented. No dependency was installed/fetched or global setting changed.

Then `cmake --build --preset schema-transport-debug --verbose`: exit0,12 steps,
ONE actual ambiguous-else warning at native_foundation.cpp:623. The
[first exact build output](schematool-001-native-first-build.log.txt) preserves it.
No runtime/discovery command followed. A corrected the guard with explicit braces
and reported cleanup defects without changing any of89 EXPECT invocations.

C a4a10d0 approves actual fixture264 cleanup; C4037668 approves actual annotation66
for rebuild/static inspection only. The latter follows independent actual ELF
inspection: host compiler had emitted TLS/canary plus __stack_chk_fail@plt in raw
helpers. Ten function-local no_stack_protector attributes remove that unsupported
path only from bounded child/reporter functions; parent hardening is retained.

Rebuilt with the SAME build command: exit0,4 steps, zero warnings. The
[corrected exact output](schematool-001-native-corrected-build.log.txt) preserves
commands/flags/source paths. Actual artifact SHA256 after this rebuild:

- Native foundation:19aad31605c2a75b5a6c18632e486bb49b73d41e4f1b36b464e7ae7aaffc0a07.
- Fixed reporter:bd3e5ea0c6af3de7e1abcbd80d4a74f91cf8f23755dee1a85c497798084c22c1.
- Inert production DSO:bf797690cc4f7d6341b10b4e22bb581f47572d418eea0e956af17b18e54fcac2.

Static nm/readelf/objdump were inspected, without executing their input artifacts.
The corrected reporter call graph has raw helpers/direct calls, no TLS/canary or
stack-fail call; normal ELF startup is a separate path. Production DSO has the
reviewed sole public ABI, eager BIND_NOW/FLAGS_1 NOW and standard C++/C runtime
NEEDED entries. These observations are not complete admission of the native
owner, bootstrap binding, clone-success edge or every generated path.

Runtime and CI publication remain HELD. Actual corrected checker/source controls,
per-host generated graph, successful-clone entry edge and actual libc binding
still need separate evidence/review before probes. A is authoring a nonvalidating
no-child binding case; it has not run. Both-host actual positive native facilities,
helper controls, intended actual-owner31 red, implementation/source audit,74 ports,
full CI and normal PR77 merge gates remain. Finish PR77 then stop.
