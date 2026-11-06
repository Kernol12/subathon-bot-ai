# Scoped clone-entry and same-GOT automation

Root, 6 November2026. Source authoring only; no new audit/control function or
candidate native program executed. This is nonvalidating build-harness admission,
not production owner implementation. Existing owner31/red/review gates remain.

Actual fixed hosted static run42263771664 at586e18b supplies both GNU debug
instruction patterns: clone3 number0x1b3; direct RAX save; compare that exact
64-bit slot with0; JNE skips exactly LEA/MOV/direct raw child call. No TLS, loader,
errno or callback operation occurs on the successful zero branch. Syscall PLT
is one optional-CET/NOP-decorated JMP (host22 BND-prefixed) through the exact
GLIBC2.2.5 GLOB_DAT slot. The binding test takes its address from that same slot.
Existing scoped transitive child and actual libc-success audits remain required;
the no-child bootstrap is still necessary to bind to the actual loaded libc.

`probe_entry_binding` automates ONLY those closed instruction shapes. It checks
real instruction boundaries, actual function target, one versioned relocation,
no extra PLT operation and exactly one matching bootstrap reference. Changed
compiler output fails rather than broadening a namespace or inferring safety.
The seven literal controls independently transcribe known shapes and reject
wrong syscall/register width/comparison/branch/interior/callback/GOT/version/
relocation/PLT/reference. No ELF/subprocess/native operation occurs in controls.
The workflow invokes these unconditionally before any native discovery.

Different actual source/control review precedes first static execution. Root will
record exact outcomes on actual local ELFs and host evidence; no claim of a
completed first run, facility green or owner acceptance appears here. New helper
raw-child entries must receive their own generated graph/edge checks before use.
