# SCHEMATOOL-001 — first static-audit control red

Root, 6 November2026. Actual FIRST run at CLEAN
2832f736ffddc28efb1ad36e8639010523715fa3, after different source/oracle approval
C e19b38af6c90cd5350e82654ae02866fff449849. Script ee13b687 contains eight methods /
38 literal controls; checker c6245e6 remained unmodified before this observation.

Exact command: `/tmp/subathon-schematest001-py314/bin/python -I -S -B
tests/schema_validation/native_audit_controls.py`, cwd `/tmp/subathon-schematool001`,
environment exactly PATH=/usr/bin:/bin, LC_ALL=C.UTF-8, LANG=C.UTF-8.
Started16:05:49.166958Z. Native unittest result: eight methods,0.006s, exit1,
TEN assertion failures, zero errors/skips; outer elapsed0.059447s and30-second
watchdog did not fire. All expected-positive controls passed.

Failures precisely reach C's reported logic: five altered libc prefixes accepted;
one branch into an instruction interior accepted; three malformed/address-bearing
instruction texts dropped; one unreviewed namespace function admitted. Each fails
`RuntimeError not raised`; there is no import/setup/dependency failure.

[Exact stderr](schematool-001-native-audit-first-red.stderr.txt):8411 bytes,
SHA25698bf17727ae168001b9dcd252e3b997672c91add859c469a26f56cc1db6786d3.
[Stdout](schematool-001-native-audit-first-red.stdout.txt) is empty.
[Receipt](schematool-001-native-audit-first-red.receipt.json) records head, review,
command, environment, native status, wall time and both hashes. Preserve unchanged.

Scope is STATIC mocked-text build-harness logic only: audit definitions import,
actual parser/graph/wrapper functions and fixed mocked external text. No native
artifact load/entry, ELF/proc discovery, binutils, clone3 probe, SDK or worker path
ran. This is not the native owner31 baseline or host capability evidence. Original
FIRST/public/focused artifacts remain unchanged; no historical red was replayed.

The observed/reviewed failures permit the bounded checker corrections, followed
by different actual source/receipt review and unchanged-control passing run.
Clone-success entry edge, actual libc binding, per-host generated artifacts,
bootstrap/probe execution, helper/owner baselines,74 ports and PR77 merge gates
remain separate open work. Finish PR77 then stop remains the instruction.
