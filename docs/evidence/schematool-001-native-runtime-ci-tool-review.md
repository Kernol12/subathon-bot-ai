# SCHEMATOOL-001 — CTest lookup correction and hosted attribution

Reviewer C, 6 November 2026. Fresh isolated review from actual root
`348686555fcf8091ce2a1abc2c56e8917852be79`; ONLY this note changes.

**APPROVED: exact tool-lookup configuration correction for ordinary conditional
PR77 publication.** Native environment remains exact3; no test/assertion/count
or generated-code guard is weakened. BOTH-host facility green and owner intended
red remain outstanding. This review executes no candidate or test.

Reviewed workflow blob `810622fd3df288141f350467405d9967046a0a58` and author
note `b3299b13400703e94e46d65b12845589601d19aa`. Inventory selects CTest using
stdlib shutil.which under the configured runner tool environment, rejects absent
tools, resolves the path strictly and invokes that absolute executable with the
same exact PATH=/usr/bin:/bin, LC_ALL=C.UTF-8, LANG=C.UTF-8 dictionary. Final CTest
captures the configured command path before env -i and quotes it as one command
argument. GitHub's fail-fast shell fails missing lookup/command status. The job
also records CTest version and selected inventory path. This avoids changing the
native PATH to locate a build tool; actual later path/version/results must still
be observed. No new dependency or public option is introduced.

All previous static gates, exact-one no-child XML guard, exact55-native-plus-one-
loader enabled inventory and unconditional verbose all56 remain. Earlier B/C
reviews apply to unchanged source. No hosted hash, kernel facility, test pass or
owner behavior is inferred from this source correction.

I independently queried/downloaded full actual hosted run
[42277775666](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42277775666)
at head `9bce17d34b9858bf535dada608aa8763b9da464f` and both job/step records.
Own complete log `/tmp/schematool001-native-helper-hosted-tool-c-review.log`
is993754 bytes, SHA256
`5fef485cd21a4f466c3c353aa3ae3fea04202e9054fe2965bac225b6c8125075`.
Both checkout logs identify synthetic mergee7a15e526a47759598f75b2a7e15aa067710dd55.
GitHub commit API confirms its tree93d68dd971903249c11f0e17c20f5e5ebea50e21
equals actual9b's tree. This run used pre-correction workflow4f92fd2.

- Ubuntu22 job128098504535: GCC11.4.0/Python3.10.12/glibc2.35; control8/8/5
  pass.011/.003/.002s; both actual ELF graphs pass. Strict no-child binding passes
  at19:14:14Z, GLIBC_2.2.5 syscall offset0x11e9c0, kernel6.8.0-1070-azure,
  actual seccomp0.
- Ubuntu24 job128098505037: GCC13.3.0/Python3.12.3/glibc2.39; control8/8/5
  pass.006/.002/.001s; both actual ELF graphs pass. Strict no-child binding passes
  at19:14:13Z, GLIBC_2.2.5 syscall offset0x127510, kernel7.0.0-1017-azure,
  actual seccomp0.

Both build/static/export steps succeed before the only native invocation, the
reviewed no-child case. Raw output has one actual passed binding case per host
and the XML guard's post-check completion line. The next inventory process fails
with exact FileNotFoundError ENOENT for `ctest` under env3. Neither inventory nor
kernel/helper/loader execution succeeds; final runtime steps are dependency-
skipped. This is a harness tool lookup failure, neither facility denial nor owner
red. Do not waive it or reuse local56 as hosted success. Actual corrected runs
must establish all56 on BOTH hosts before the next behavior gate.

Actual reviewer checks: complete source/diff/author-note and hosted log/step/tree
analysis; four Python3.10 inline AST parses, exact env3/inventory-command checks,
all12 literal/folded shell blocks bash-n. Project426/122, render--check and
diff--check PASS. No workflow body, candidate import, audit/control replay,
configure/build, CTest, SDK/native/DSO load, thread, child, signal or probe ran
from this review. Historical FIRST evidence and all preserved assertions remain.
