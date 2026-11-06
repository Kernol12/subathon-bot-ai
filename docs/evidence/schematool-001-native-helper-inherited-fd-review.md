# SCHEMATOOL-001 — hosted inherited-FD failure attribution

Reviewer C, 6 November 2026. Fresh isolated review from actual published
`6a415b2017afd7bcda412fbc2a9ea74e1ae8b92f`; ONLY this note changes.
No candidate execution or historical FIRST replay.

**Actual BOTH-host prerequisite remains FAILED: each executes56, passes55 and
fails only producer report28.** Preserve all successes and the genuine failed
assertions. This is test-launcher setup, not native-owner red or permission to
weaken packet/descriptor expectations or begin owner implementation.

I independently downloaded COMPLETE run
[42279145549](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42279145549)
plus both job/step records. Own raw log
`/tmp/schematool001-native-helper-inherited-fd-c-review.log` is1463958 bytes,
SHA256 `ce348043945a7c14b7bff07fb3bc29679d24f4f632d2c68c78ed4e2525a0825e`.
Both checkout logs identify381a003d3e39b91fe01a79f9ff0db3782d9fdc09; GitHub commit
API confirms tree4128f887df6da0e9d528bcac933e366dfa64bc8e equals reviewed6a.
Workflow810622fd, producer launcher edc26111 and conditional-dispatch child
7a876c03 match their admitted source revisions.

Both jobs pass original/fault static graphs and their8/8/5 controls before strict
one-case XML no-child binding, then strict exact56 inventory. Both actually select
`/usr/local/bin/ctest` under unchanged native env3. Final all56 runs unconditionally:

- Ubuntu24 job128103212092:55PASS/oneFAIL, total.19s; actual foundation SHA256
  `6e4156f84dacb9c247e6922446ae1511fe8d3bd326a81e4bd8e850b1ad52953e`,
  negative child `0ce4ccafcf46adce7c83472dde787456b33aaba6ba329d8609be4b9a7b2c624e`.
- Ubuntu22 job128103212363:55PASS/oneFAIL, total.24s; actual foundation SHA256
  `e278e6dabe58b66e69599df9d0ec703832df79c2974f162be4a52b83943fb22e`,
  negative child `03ca5ff6c4fd9dcf716b2da0cca82d43d2e881339a6a68d00b84571ee77f46fd`.

Independent complete raw parsing verifies the exact reviewed56-name set,55 native
RUN starts,54 native OK results plus the one failed native case, and both loader
methods OK. No skipped/disabled/missing/timeout case appears. Raw output repeats
the failed diagnostics for CTest reporting; this is not a second execution.
The sole failed registration is
`ClosedBytes/NativeFaultChildBytes.ActualFixedExecBytesAndStatus/23`, mode28.
Both jobs report the same THREE genuine assertion failures: count8 rather than6,
and actual descriptors142/145 rather than UINT32_MAX in the otherwise-unused
packet slots. Packet length, child identity/status, EOF, signal observations and
the other existing producer assertions pass. No125/setup/ABI/readiness/tool cause
substitutes for this failure. CTest exits8. Existing kernel/helper controls passing
do not make the complete prerequisite green.

Source inspection explains a concrete setup gap: the parent posix_spawn fixture
remaps3/4 and closes only its four known pipe sources; it never closes other
inherited non-CLOEXEC child descriptors. Raw packet confirms those extras survive.
Their original runner purpose is unknown and irrelevant to the exact child-table
contract. Parent runner descriptors must remain untouched; expanding tolerated
counts or ignoring particular FD numbers would weaken the required oracle.

Privacy review: content is public CI/toolchain/source metadata and synthetic
fixtures, including transient process addresses/FDs, not supporter/live-provider
data. Checkout basic authorization values are masked. Independent known GitHub/
AWS token-prefix and private-key-header scans find no unmasked matches; this is
bounded log inspection, not a security certification. Root may preserve these
exact failed raw bytes/metadata with the source identities above.

## Controlled sentinel regression source

I also read actual A3fbd47db1cac7746ab3ee31c767adf61a4a4398a, producer blob
`7ceb2d778690391e870972e8ff64dc17d35db81b` and written author reason
`42729929e086b7939fdbf22cf6406e66ea475d74`.
**APPROVED: bounded regression source, before any launcher fix.** Actual build/
static/binding gates and root's separately controlled focused report28 first-red
observation still precede correction; no native-owner behavior is approved.

The insertion-only delta creates a real F_DUPFD>=64 read alias before spawn,
requires non-CLOEXEC flags0 and deliberately omits that unrelated alias from the
unchanged four known-source child closes. It cannot keep protocol writers open
or collide with3/4. Parent adopts only actual successful FD returns, closes its
alias once and checks immediate close0/EBADF before any later FD acquisition;
peak ownership is6 including actual pidfd. Partial/setup failures still close
adopted aliases and are fixture errors. All existing24 literal rows/assertions
remain exact; four additional sentinel assertions must pass in the reached red.
The same unchanged report set/count oracle should now expose incomplete child
closure deterministically. No close-from implementation is present yet.

A later child-only posix_spawn close-from action after remapping/known closes is
a coherent narrow fixture correction after that actual red, subject to separate
actual source review/generated gates and real full BOTH-host56. It supplies no
production atomic-birth fallback. No prospective implementation approval follows.

Actual reviewer operations: complete log/receipt/step/tree/source analysis,
stdlib hash/count/name checks, privacy scans and source-diff ownership reasoning.
Project426/122, render--check and diff--check PASS. No candidate import, audit/
control replay, configure/build, CTest/listing, native/DSO/SDK load, thread,
signal, child or probe occurred from this review. Owner69 FIRST, binding/public
ports, full acceptance/source/latest-head CI and normal PR77 merge remain gated.
