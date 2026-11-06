# Independent parent cleanup/allocation regression source review

Reviewer `/root/session_plan_review`, DIFFERENT from author
`/root/session_supplier_author` and integrator `/root`,6 November2026.
**APPROVED exact TWO regression source/oracles FOR reached-bug execution**, actual
candidate `8460c85ddac1c03f3c7267deda2e23600e2963d3`, test blob
`acde40d001a7932f68d2c8ebe5aa25b3a983ef82`.

NEW isolated branch `task/SCHEMATOOL-001-parent-cleanup-test-review`, worktree
`/tmp/subathon-schematool001-parent-cleanup-test-review`, from that candidate.
ONLY this note changes. Read full actual211-line source/author note, root/tests
instructions, public API§4/known-resource outcomes and exact unfixed parenta25.
No test/helper/target/SDK import or execution, child/timer/probe/clone/install ran.

## Independent contract oracles and fault isolation

The two findings originated in different source review: cancellation while Python
can unwind must finish owned-resource cleanup without fabricating a result; known
initial admission MemoryError must return resource_limit. Root explicitly assigned
these narrow private failure-injection seams before any production correction.
They add no public callback/factory/options or successful standards validator.

The cancellation case records only the two REAL parent os.pipe pairs BEFORE the
Popen interposer is entered. Native Popen's own error pipe and separate test-ready/
blocking pipes are excluded. Normal private argv/options/FD and DEVNULL/shell/close
settings are checked before substituting a REAL isolated child and returning its
REAL Popen/PID. Its code writes malformed activation B, acknowledges readiness and
blocks while keeping both protocol write ends open. It emits no validation outcome.
The syntactically valid public path is sufficient for this pre-package protocol
fault; no file-existence or package-validity claim is supplied by the fixture.

The root-authorized close seam calls the saved REAL Linux close FIRST on the first
recorded parent READ descriptor, THEN records one trigger and raises the SAME
KeyboardInterrupt object. It models an exception after an actual close, rather
than inventing a leaked triggering descriptor. The child is still blocked/alive
when malformed B forces parent cleanup; a closed activation stream is not mistaken
for the stimulus. Only that one parent close can inject cancellation.

After public unwinding, before fallback cleanup/poll/wait, it captures real
waitpid(WNOHANG) and ALL4 original parent fstat states. Only ChildProcessError means
the parent already reaped; running or waitable-unreaped states FAIL. A waitable child
that the test probe itself reaps is explicitly marked test-probe-reaped, not success.
The same cancellation identity, one trigger, all4 closed FDs and empty public logs
are required. Capturing diagnostics before assertions preserves both child and FD
evidence when unfixed source fails; test cleanup cannot manufacture success.

Real select readiness is bounded3s. A real5s watchdog can kill ONLY and sets a flag
that must remain false; it never reaps or supplies the oracle. Timer tracking is
assigned after successful start. Nested finally cleanup subsequently cancels/joins,
closes real test/remaining parent descriptors, and kills/waits a surviving child.
There is no sleep/stress loop, SDK import or fake validating response.

The allocation case passes exact builtin `manifest.json`, and ONLY the existing
private _bounded_path failure point raises the specific MemoryError. It requires
the exact resource_limit enum, one initial call and no public output. Installation,
Popen and builtin/io/os opens are raising guards WITH independent assert_not_called
checks, preventing swallowed guard attempts from becoming false green. This tests
the public catch boundary; it does not claim real allocator exhaustion, child
RLIMIT or package validation. Expected outcome comes from the frozen known-resource
rule, not a successful helper or implementation-generated result.

## Exact stage and allowed first observation

Production remains EXACT unfixed blob `a25d531ea70271a8434f2a4206b5bff25f444d45`.
Frozen original64 source remains `64f4911edae1e6ec1546777661f199252d06131c`.
Both new methods are ordinary undecorated unittest registrations; target loading
is lazy setUpClass, module definitions are inert and no baseline helper is imported.
The command selects ONLY this separate two-method module, not the old64/red suite.

Root/author may now execute the exact reviewed command from the author note in the
admitted selected environment, retaining original raw bytes/status/source hashes.
An intended reached-bug result must show actual cleanup failure after the SAME
recorded cancellation and the specific initial MemoryError escaping the public
catch boundary. Readiness/import/setup/dependency/other errors do NOT substitute.
Actual result/count/diagnostics remain FUTURE, not inferred from this approval.
Only after observed attributable red may B fix parent source, preserving these
assertions and all frozen64 tests. Actual corrected source review and passing new
regressions remain required. This is not parent implementation approval.

Root must integrate these new regression checks into appropriate ordinary CI once
reviewed fixes pass; existing64 registration alone does not execute this new file.
Worker/source/integration, full64 real acceptance and resource/ABI/no-site/secure-
read/offline/allocation/platform/live/full-release gates remain. No original FIRST
is replayed and all7 integrations/3 FX providers remain in scope.

Actual checks: stdlib ast.parse(feature_version=(3,10))/compile WITHOUT exec, exact
two names/no decorators, real-close-before-raise AST and parent/frozen64 hash guards
PASS. `python3 tools/check_project.py` PASS426/122/current links/presets/HTML;
`python3 tools/progress.py render --check` and exact commit diff check PASS.
No candidate/regression/SDK import, feature/native-child/clone/install execution or
GitHub write occurred; only this independent evidence note changes.
