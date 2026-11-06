# SCHEMATOOL-001 — conditional native runtime CI configuration review

Reviewer B (`/root/session_supplier_author`), 6 November 2026. Fresh isolated
`review/SCHEMATOOL-001-native-runtime-ci` from actual root
`7ef0a16c12efb3bb4ed45d27c76ca3203bc4837e`. ONLY this note changes. Review is
the root-authored workflow delta and its runtime-CI author note, not B's Ops/
negative-child source or independent C's new artifact/runtime review. PR77 only,
then stop; earlier actual source/static-gate approvals remain preserved.

**APPROVED: exact conditional CI source/configuration.** No source finding blocks
publication after root's separately required actual artifact/source admission
and local green. This verdict does not assert either hosted kernel supports the
selected backend or that any hosted/native case has run/passed.

Reviewed exact blobs:

- `.github/workflows/schema-native-foundation.yml`:
  `54eb26f67078e049ea16012981908d0506fc534b`.
- `docs/evidence/schematool-001-native-runtime-ci-author.md`:
  `c963944afabc32666df855b61498d449e82bd8e9`.

Read actual diff, root/module instructions, existing root/test presets and native
CMake registration, loader source, binding case and all three control inventories.
Same two runners/minor guards (Ubuntu22.04/3.10 and24.04/3.12), read-only token,
pinned checkout/no persisted credentials, timeout, build/dependency selection and
native source assertions stay intact. New workflow/job names describe facility
acceptance; current required checks/latest publication receipts remain root's
normal merge gate, not supplied by this review.

Before ANY candidate-native bootstrap/discovery, the workflow runs8 original and
8 entry controls plus the original actual ELF gate, then5 fault literal controls
and the new actual ELF gate.
The original gate remains mandatory and the new fault gate is also mandatory;
both precede runtime. Actual artifact identities and static clone/GOT evidence
are recorded. Unknown host/compiler code fails the job. Configure/build use
existing PRE_TEST discovery, so they do not silently execute the candidate first.

The first candidate invocation filters exactly the NO-CHILD
`SchemaNativeFoundation.SyscallBindingToActualLibc` under the three-key minimal
environment with check=True/30s limit. XML requires exactly one testcase and
tests=1, zero failures/errors/disabled, no skipped descendants, exact case/class,
status=run/result=completed. Missing XML, zero tests, failed filter/status and
process failure cannot pass. The actual existing binding test performs no generic
syscall call, child/thread creation or signal mutation; this does not approve a
different future body. C actual285 artifact/admission and observed binding results
remain separate evidence.

ONLY after that bootstrap may CTest --show-only run PRE_TEST discovery. The guard
requires the exact set AND count:7 original native +12 ordinary helpers +6 invalid
plans +4 capture plans +2 signal edges +24 fixed-child cases=55, plus loader1.
Existing NO_PRETTY_VALUES fixes parameter names0..5/0..3/0..1/0..23. Every selected
registration must have the exact facility label and neither DISABLED nor
WILL_FAIL. Loader AST independently requires exactly its two named undecorated
methods. Then unconditional verbose CTest executes all56 facility registrations
with --no-tests=error; no platform skip, inversion, conditional continuation or
continue-on-error is added. Matrix fail-fast=false permits both actual lanes to
report independently. Direct native12/negative57 behavior remains outside this
facility label; no assertion is deleted, reclassified as facility success or
replayed here.

Actual STATIC reviewer checks only: yaml.safe_load (PyYAML6.0.3) parsed the file,
and independent assertions checked the exact matrix, token permissions, absence
of conditional/continue-on-error steps and static→bootstrap→inventory→CTest
ordering. Stdlib ast.parse checked all four inline Python heredocs without
executing them and confirmed control inventories8/8/5 from source. No workflow
body, new audit/control, candidate import, CMake/CTest, artifact load, native,
SDK, helper, thread, child or signal was executed by this reviewer.

`python3 tools/check_project.py`, `python3 tools/progress.py render --check`,
`git diff --check` and final `git diff --cached --check` PASS:426 tasks,
122 decisions/local links/presets/current HTML. Actual both-host CI receipts,
helper/facility green, independent reviewed behavior baseline/intended31 red,
owner implementation/source review, full preserved ports/public acceptance and
latest-head normal PR merge gates remain distinct outstanding work.
