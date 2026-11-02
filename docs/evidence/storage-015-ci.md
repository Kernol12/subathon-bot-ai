# STORAGE-015 local acceptance and CI promotion candidate

Coordinator `/root`,2026-11-02. Actual implementation candidate is under separate
source/config review; official automatic29 execution remains pending. This is not
implementation-done or full-release evidence.

## Exact inputs and reviewed baseline correction

Source author7dd67bb integrated89daa8a; source blob
`9b0b01f1f6cc8862ed4abac80a43ab0b54211a47`. Evidence-only author8b565969 was
integrated3898199 after actual passing runs. Public header/logical/physical and all
old producer sources/fixtures remain unchanged.

The initial tested source was b6978430 (28/29 and172/173 pass); sole failure was an
unsupported Qt Invalid-QVariant assumption about raw SQL NULL. Later source hashes
were untested intermediates until the final9b0 build/runs; they are distinguished
in the [author evidence](storage-015-implementation.md).

Separate correction author61f4242/root6d09bee and actual different review90eceb0c/
rootbddd847 revise only the affected history slot:
original0f8e625ff7c238776602fd1be102959f8c43b073 →
`a805674e014c0c7ae325474460bc64d6b10de770`. Every outside-slot byte and all original
history/audit/parent/revision values remain; explicit typeof/nullness/exact2 checks
are stronger than incidental Qt metatype equality. Independent raw Qt6.12/SQLite3.54
proof and specification-based reason are in the
[correction](storage-015-null-baseline-correction.md) and
[actual review](storage-015-null-baseline-review.md). No requirement was changed,
no production normalization added, and original redae3 remains untouched. This
corrected assertion does not invent a new missing-behavior baseline or repeat it.

## Actual local passing observations

After actual correction approval/integration, current source9b0/testa805 passed:

| Actor | Actual29 | Actual full173 |
| --- | --- | --- |
| Author | PASS1.15s | PASS4.65s |
| Root | PASS1.11s | PASS4.45s |

Author parsed Qt totals for zero failures/skips and preserved exact logs. Root
commands were actual configure/build sqlite-plan-debug, then targeted
`ctest --preset sqlite-plan-debug -L '^sqlite-plan$'`, then full
`ctest --preset sqlite-plan-debug`; both exit0. Root initial source build and
corrected-test incremental build completed without warnings. Host GNU16.2.1,
CMake4.4.5, Qt6.12.0/SQLite3.54.0 and Boost1.92. Logs are ignored
`local-evidence/coordinator/storage015-{source-*,corrected-build.txt,root29.txt,root173.txt}`.
No unchanged stub-red or historical suite baseline was repeated.

## Concrete shared configuration for review

Only after actual passing29/full173, root proposes:

| Artifact | Blob |
| --- | --- |
| Root CMake | `16381eb09dedb572f2158a76bbaa3b03d1287af1` |
| Presets | `43c39c05d11c4a4a429960a378225c9f08426b8e` |
| Qt registration | `08bc2a4e22b5fc76acb19f0126f1132e1ef4227d` |
| SQL workflow | `ce4b2dd26734375a6a63b1fe9e6103b77b558daa` |

Changes remove only draft text/label and manual plan-feature input/condition;
all29 now run unconditionally after exact compiled discovery and required green4.
All previous135 automatic SQL cases, old7/3/3/3 diagnostics, action/archive/Boost
pins, permissions, check name and every case/assertion remain. No continue-on-error,
WILL_FAIL, skip or failure inversion. Reconfigure/build after label promotion passes;
exact173 registrations/29 feature cases remain and no feature draft label survives.
Actionlint1.7.12/diff/project/render checks pass. Source/config approval and actual
changed automatic official workflow are still required before completion/merge.
All semantic/app/provider/operational/UI/platform release gates stay open.

## Actual source/config approval and automatic official acceptance

Different actual [source review](storage-015-code-review.md)
`d15450bef3c1ed8025d3a5caf2bf3e689c27eb3a` integrated `6192617` accepts exact
candidate `1bf47f0e6e72bac154413e0c7f290e36ec2024fc`: source9b0/revised testa805 and
four shared config blobs above. Reviewer read all1176 source lines, traced the
scope/ownership/rollback/allocation paths beyond assertions, independently matched
five DDL to physical99bf and preserved old config steps. Its actual fresh82-step
build had0 warnings;29/full173 PASS1.11s/4.45s, zero Qt failures/skips. No absent
future revision, CI result or full-release claim was approved.

Actual automatic [run41503096872](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41503096872)
at SAME source/config candidate1bf ran2026-11-02T18:42:23Z–18:43:35Z and concluded
SUCCESS. CI and code review proceeded in parallel; this is attributed source-head
CI, not a claim that a later evidence commit was in that job. Source/config/fixture
bytes remain unchanged after1bf. Root inspected actual JSON/full log and all-checks
list under ignored `local-evidence/coordinator/storage015-source-*`.

Observed actual job:

- Both selected Qt archive hashes OK; project GNU13.3.0, pinned Boost1.83 package/
  selected header108300/configs, Qt6.12.0 and bundled SQLite3.53.4.
- Fresh82-step build, including actual new implementation and both Qt suites,
  zero build warnings; complete compiled discovery matches all29.
- All old135 automatic SQL cases/old required green7+3+3+3 pass. New green4 reports
  actual6PASS0fail/0skip26ms with selected Qt/SQLite identity and INVALID sparseMAX.
- **Run reviewed captured-plan acceptance** executes unconditionally and reports
  **100% tests passed,0 failed out of29,1.70s**. It is not a dispatch-only discovery
  or suppressed failure. All11 source-head checks completedSUCCESS.

These satisfy bounded implementation acceptance plus different source/config
review. Final completion metadata/CI-attribution review and future latest-head
checks/discussions/rules/normal merge remain separate. The historical baseline
red is unchanged; no unsupported full-app/provider/platform capability follows.
