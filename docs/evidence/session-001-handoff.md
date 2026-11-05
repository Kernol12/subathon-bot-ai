# SESSION-001 password primitive implementation handoff

Coordinator /root,2026-11-05. [PR65](https://github.com/Kernol12/subathon-bot-ai/pull/65),
reused task/SESSION-001-password-hashing. Preserved original claimffdef49 was found
without active agent/worktree or production work; refreshed currentmain520cc6e,
then explicitly reassigned source to access_reviewer. Different reviewer selected_boost.
Actual main685b17f (normal PR64) now integrated16444ce without changing this source.

The three real selected libsodium1.0.22 entrypoints implement raw-byte Argon2id
creation/verification and source-bound conditional upgrade proposals. Borrowed
option/password/PHC/cost validation precedes crypto/source calls. One guarded,
continuously explicitly locked plaintext allocation remains alive across native
work/source checks, wiped/freed on every path. Native output is length/profile/
parameter validated before ownership. Upgrade verifies the actual stored source,
requires current accepted-login/source checks, preserves stronger dimensions and
returns an owned compare-and-save proposal. It writes nothing and authenticates no
account; current login/conditional-store producer integration is still required.

## Frozen baseline, source and independent review

[SESSIONTEST001 handoff](sessiontest-001-handoff.md) retains actual logical/API/
test/native-input reviews and genuine FIRST37red d560cbda/attributionc5b10c59.
No FIRSTred/native/input-generator proof was replayed. Original test1d0d3775,
fixture025d4de9, private header7e87bbe9/observer4e1f049a and contracts remain exact.
Public header changes ONLY its obsolete all-three-stubs comment, no declaration.

Author6b870c27b52610928533f3a13e7c20b80d570bce imported920cfaa, sourceblob
e56442511cf51ec4329f7d4a3cf94a3a093f1bb6. [Actual different review](session-001-review.md)
0817f48ad45b136789b8b0a92a37c5c03cc42671/import5ff3fa2 inspects source, selected
SDK free/unlock/clear behavior, strict PHC grammar/base64/cost priorities, continuous
secret lifetime/exception partition, private source-bound proof/two checks, native
output and proposal ownership beyond passing assertions. No blocking defect found.
Allocation handling follows the frozen API; failure construction's documented
runtime_error allocation limitation is not presented as hardware-OOM survival.

## Actual acceptance and CI

[Exact restored SDK](session-001-environment.md): official previously GPG-authenticated
source pin,2008529bytes/SHAadbdd8f..., fresh disposable static build; all3 artifacts
hash-match preserved prior reviewed SDK, archive836744bytes/SHA959cdbe8... . No global
install/new dependency, no fixture generation/vendor suite/benchmark. Host Linux
x86-64/GNU16.2.1/GTest1.18.0/CMake4.4.5/Python3.14.9.

Author fresh41/41 PASS0skip3.57s; root13-step build/full41 PASS3.59s; different
reviewer fresh13-step build/full41 PASS3.51s,0fail/disabled/skip. Original37 plus
independent real SDK environment and3 infrastructure. Exact current37 enabled/
60stimeout/default-skip/no-inversion discovery guard and static/private symbol
isolation pass: twelve GNU wraps only in feature executable; no wrapper symbols
in product library/foundation/generator. Generator only compiled, never invoked.
Actual commands/JUnit/hash receipts are in [author](session-001-author.md),
[review](session-001-review.md) and [CI integration](session-001-ci.md).

[Ordinary selected Ubuntu run42069351732](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42069351732)
atf1042f2d229328e0a268456b03ef22d088133f9f completedSUCCESS. Root read actual
metadata/full logs: authenticated exact source/static SDK, exact37 enabled guard,
real native environment1PASS0.53s, UNCONDITIONAL37PASS5.41s, old3PASS1.27s.
Rootfaef49f promotes labels/execution preserving stable job/pins/source/auth checks;
no skip, failure inversion or continue-on-error. [CI note](session-001-ci.md).
Final metadata/combined-main review and all latest-head applicable checks remain
publication gates; old CI is not assigned to an unseen final head.

## Remaining gates

Component accepted; done does not mean merged or actual account/login security
complete. Real accepted-login/current-source/atomic conditional-store authority,
aggregate admission/queue behavior, supported-hardware100sample cost benchmark,
platforms/packages/licenses/live verification and full release remain their owner
tasks. No actual account/credential/provider/chat/payment/deployment side effect.
Final independent metadata review/currenthead CI/rules/resolved discussions and
GitHub-permitted normal merge remain at authoring. Original CORE008 claim3f81224
and parked asset work remain preserved, no old agent assumed live. Root owns all
shared build/CI/state/map/HTML/integration and checks exact current PR before merge.
