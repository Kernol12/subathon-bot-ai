# SCHEMATOOL-001 — actual local no-child binding bootstrap

Root,6 November2026. FIRST local bootstrap at CLEAN67f3e54305d6f9dc3563839653b1bdb8740fadf0,
after actual different artifact admission C17bf81c6fb12776bfe72da63734d803fac79b593.
The executable hash remained exactc618c4d6; no rebuild/source change intervened.

Ran only SchemaNativeFoundation.SyscallBindingToActualLibc with the exact filter,
--gtest_color=no and a fresh temporary XML destination. Environment exactly
PATH=/usr/bin:/bin, LC_ALL=C.UTF-8, LANG=C.UTF-8;30-second watchdog did not fire.
Native exit0: ONE test actually ran/passed,0ms; XML independently requires tests1,
failures0/errors0/disabled0, exact case name/statusrun/resultcompleted and no skip.
Started16:28:44.408171Z; outer elapsed0.003941s. No other GTest case/discovery ran.

Actual result: /usr/lib/libc.so.6, GLIBC_2.2.5 syscall offset0x11e450; unversioned/
versioned lookup and same-object checks pass. This matches the independently
reviewed actual file prefix/relocation/GOT-slot proof. Diagnostics report kernel
7.2.14-arch1-1, x86_64, libc2.44, seccomp mode2. These are observations, not proof
that clone3 or any other blocked/available facility works in this environment.

- [Stdout](schematool-001-native-bootstrap-first.stdout.txt):624 bytes,
  SHA256440274b654b8f86fe7ffa72af347546ea2e13832cdc807d9a2a58d44eaf4c161.
- [Stderr](schematool-001-native-bootstrap-first.stderr.txt):208 bytes,
  SHA2566367028db22bf63eef01971d96d439af974182fbc837ab0f1a6b70c5e89cf72b.
- [XML](schematool-001-native-bootstrap-first.xml):1210 bytes,
  SHA256a7f9c4fe1d6bfa8dff531ae20e1215464994fff13c70d24173b1a85d18d99d2a.
- [Receipt](schematool-001-native-bootstrap-first.receipt.json) preserves actual
  argv/head/review/environment/status/time/hashes and exact-one-pass result.

This is the first native executable run and is deliberately NO-CHILD: no generic
syscall/clone probe, thread/signal fixture, loader test, CTest inventory, native
owner/worker or domain validation ran. Source-approved metadata uname/prctl and
same-loaded-libc lookup are the only bootstrap-specific native operations.

Different actual attribution/admission precedes local kernel foundation execution.
If the tool's seccomp profile denies a required syscall, that is unavailable
facility evidence, not intended owner red; a separately approved target-host test
environment may be needed and must be recorded. No unsupported facility may be
silently skipped or greened. Both selected hosted artifacts/facilities, helper
foundations, intended owner31 red and all source/public74/CI/merge gates remain.
Finish PR77 then stop remains the instruction.
