# SCHEMATOOL-001 — actual local owner artifact admission

Reviewer C independently reviewed immutable root
`37f0ecde29977005f935d91dd7e8236dd1bf1999` on a fresh note-only branch.
**ACCEPTED: actual matching local static proofs.** This admits the ordered,
conditional local execution stages below; it does not approve the known
normalization behavior or the complete implementation.

The27764-byte proof archive has SHA256
`57190ab57d77757e2756f671aa9186464e9cf2ce8637860323df1f226936e410`.
I read all11 receipt/raw sets, independently verified their stream sizes and
SHA256 values, and matched all six archived CI/SU files byte-for-byte to the
current build. Both combined static invocations at clean
`a1458f407f48f8453b2777c7309f75029968928c` exit0 with empty stderr, at
UTC21:46:28.620139/21:46:28.905980. Their outputs retain the legacy helper,
probe/GOT/libc-success, primitive and negative-child checks AND matching owner
proof, under the independently approved f028 integration.

| Actual local artifact | SHA256 |
| --- | --- |
| production DSO | `e5579584ecab74295219ae1ca10c52424517d969b14fbca3b809c5baf57ff0b6` |
| fault DSO | `e1bb573d0ca3a6d771a2644838113c343b3600bd468a97edad36436483a64f76` |
| foundation executable | `f831be2f68c05754b35a26b3c5561ec1a4668629016d71bc80eb97d353f9efc1` |
| reporter child | `bd3e5ea0c6af3de7e1abcbd80d4a74f91cf8f23755dee1a85c497798084c22c1` |
| negative child | `bc122f84ffc00aa79005de58f8fa38ba6e43855581d8d9767874f3f5ba54102b` |
| actual audit libc | `02c8c3d06180beee9c7d0392aa85736c7d19f274a81e7f8f0b3d9e28e0641524` |

I independently hashed those current files; each matches both relevant receipts
and archive output. Production is unchanged from its earlier full proof. The
four-step observer rebuild at clean4e8659d exits0/no warnings/empty stderr and
rebuilds only the scoped fixture translation units and dependent links. Its
fault proof uses the new e1bb artifact and fresh matching metadata, not the
rejected prior-canary ELF. No generated/runtime claim rests on an old hash.

I independently parsed archived compiler node/frame/edge records and SU records,
joined their mangled names to actual ELF nm addresses, added actual disassembled
inter-function call/transfer edges, and traversed the resulting ABI-rooted graphs.
This separate stdlib analysis confirms production71 functions/26736 bytes and
fault124 functions/27712 bytes, including8 bytes per call edge and inherited
ABI/parent frames. Neither graph has a reachable external/missing frame or cycle;
all frame records agree across CI/SU and totals remain below32768. This is the
selected fixed build profile, not a general binary/stack verifier or OS-memory
certificate. Descriptor8/generation12 and lifetime semantics remain separately
source-reviewed and dynamically unproven until their real acceptance runs.

Reviewed source bindings are exact core058d9d12, RawOpsf96e3ae3,
entryb2bb1493, FaultOps2f772f7b, fixturee669ee35, checker9e9855aa and its
independently root-reviewed controls791f7002. The original whole75 source audit,
reviewed codegen/R3/scalar/observer deltas and f028 integration cover this source.
The new regression remains08e3b3da. All three normalizer work contexts remain
false; approval of their eventual behavior fix is expressly withheld.

The preserved old default controls have actual8/8/5 passes; owner controls have
actual5 passes after the separately reviewed literal-corruption corrections.
The earlier positive-PUSH error and initial1807-byte CWTL refusal are preserved
in the separate1393 archive native-owner-static-current.tar.gz,148554 bytes/
SHA256a8e48bd60830f0ec9ba33f973f0c760ab893ccde7ffdf772991f6fc2fae86c4c.
I checked those members as well. The later frame/canary refusals are in37f's
archive. No failed attempt is projected into a runtime pass or hidden by a gate.

Root may now perform ONLY this ordered local sequence on these exact artifacts,
ordinary selected dedicated CPython3.14 environment and env3
(PATH=/usr/bin:/bin, LC_ALL=C.UTF-8, LANG=C.UTF-8):

1. Run the existing exact one-case no-child binding bootstrap under its finite
   watchdog. Require exit0 and XML exactly1 named run/completed case, zero
   failures/errors/disabled/skips and matching actual libc binding metadata.
2. Only if that succeeds, perform strict discovery/inventory and the full existing
   foundation56 registrations, all enabled/no WILL_FAIL or inversion, under the
   same environment and reviewed finite bounds. Require every test/loader method
   to run and pass without skip, helper poison or watchdog assistance.
3. Only if that succeeds, execute ONLY the new normalization-close regression
   program's one method once, against the matching fault DSO. Preserve full raw,
   command/source/artifact receipts and pre-fallback facts. The intended red must
   reach genuine close0/release ACK/AFTER41 and physical cleanup, then fail the
   literal18 assertion because active interruption was ignored. Fixture/setup/
   dependency/missing-marker/timeout causes do not establish the behavior red.

A failed prerequisite halts dependent stages. No full69, binding9, public74 or
other behavior suite is released before this new red, the separate work=true
fix and its actual source/generated review. Different actual result attribution
still follows; this conditional release is not a claim that any stage has run.
It gives no hosted capability, final implementation, task completion or merge
approval. PR77 remains the only task, then stop.

Actually performed: full archive/receipt/source/ancestry/hash analysis, six-file
metadata matching and independent CI/SU+ELF graph/stack arithmetic using only
stdlib and read-only nm/objdump. Project426/122/render/diff PASS. I did not import
or execute a candidate auditor/test/SDK/module, load an input ELF, compile,
create a native thread/child, replay a baseline or run a probe.
