# Conditional native/helper runtime CI promotion

Root,6November2026; AUTHORING ONLY, unpublished/unexecuted pending different
actual source/configuration review. This replaces the diagnostic static-only
stage for the same existing two-host matrix, not an additional project task.

Both original and new static controls/audits run before ANY native discovery or
execution. They inspect each actual host's generated raw child graphs, exact
clone-zero entry edges, same versioned syscall GOT binding and both artifact
export/init profiles. Unknown generated code fails the job; no ignored status,
continue-on-error, platform skip or inverted expectation exists. Artifact hashes
and actual entry/relocation evidence remain recorded.

Then the existing exact-one NO-CHILD SyscallBindingToActualLibc test is executed
under minimal environment with a30s outer limit. XML must contain exactly one
completed/run passing case, zero failure/error/disabled/skipped. Only after that
actual guard passes may CTest discovery/inventory or kernel/helper tests execute.
Inventory freezes55 native registrations (7original+24helper+24fixedchild), one
two-method loader program, enabled labels and no WILL_FAIL. NO_PRETTY_VALUES
makes the explicit6/4/2/24 parameter indices stable. All56 registrations then run
unconditionally with verbose output and --no-tests=error.

The owner direct12/57 negatives are deliberately separate intentional red; this
job establishes real facilities only. It supplies no owner/schema acceptance,
SDK-green or merge claim. BOTH-host actual green and reviewed FIRST31 still
precede common owner implementation. Local285 static passes/independent actual
artifact review and exact binding result remain separate evidence; a prior15b
or old hosted hash cannot be reused for the changed native binary.

C actual binding-result/runtime review found an environment continuity gap:
bootstrap used the exact3-key environment, but the later CTest inventory/probes
inherited runner variables. Root corrects inventory subprocess and final CTest to
that SAME exact3-key environment. This excludes LD_*/GLIBC_TUNABLES/GTEST_FILTER
changes between admitted binding and discovery/probes; no assertion/name/count/
platform/code changes. Actual different C delta review precedes publication.
