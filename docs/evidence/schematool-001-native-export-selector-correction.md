# SCHEMATOOL-001 — exact evidence-export selector correction

Root,6 November2026. Actual hosted staged run42262357346 at c7f3977 failed BOTH
export steps with "Actual native probe symbol is missing/ambiguous". API step
conclusions independently show configure/build/hashes/static8/generated-helper
audit all SUCCESS on both hosts; no native executable step exists in this stage.
This failed export is not a native facility failure, passing CI or owner red.

The broad startswith(probe) selector also matches the function's lambda operator.
Actual local disassembly independently reproduces TWO matches: the exact probe
and its lambda. Requiring equality to the exact source-defined function signature
produces ONE. The count==1/unknown-shape refusal is retained, not relaxed by picking
the first arbitrary result. No fixture/assertion/ABI/source/binary body changes.

After this narrow configuration correction, ran the EXACT extracted export
here-document locally under Python -I -S/minimalenv, with only runpy definition
loading/objdump/readelf text inspection. Exit0, full static evidence exported;
no candidate ELF/discovery/loader/kernel executed. The real broad-match regression
is the hosted failure plus observed local2→exact1; no implementation-shaped smoke
suite was added for this reversible configuration defect.

Different actual delta review precedes publication. Both hosted export jobs must
then actually succeed and supply complete per-host binary evidence before any
runtime admission. Original static/red/source/ABI assertions remain unchanged.
Finish PR77 then stop.
