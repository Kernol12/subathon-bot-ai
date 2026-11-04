# SESSIONTEST-001 independent local-red sequencing review

Reviewer: `/root/codec_author`, different from proposal author `/root`.
Date: 2026-11-04. Dedicated `task/SESSIONTEST-001-environment-review`, isolated
`local-evidence/worktrees/sessiontest001-environment-review`, initially clean.
Owns only this new note. This reviews root's sequencing proposal, not this
reviewer's earlier R1/R2 findings or their still-unapproved correction.

## Actual proposal and verdict

Reviewed root `25506e021937d51beb5d2f5e3d83b8fe303ee873`, containing only the new
[local sequencing proposal](sessiontest-001-local-red-sequencing.md), Git blob
`1bb429b5eed292c6419a23e964cda286585ba472`.

**APPROVE the conditional sequencing clarification for this exact password SDK
lane.** After actual corrected full-test approval, a valid FIRST LOCAL plain marker
red may precede remote Ubuntu CI. The user instructions do not require the remote
job before that local observation. They still require valid environment/test-first
evidence, applicable CI/publication/review/rules/discussion gates before integration,
and all production prerequisites. This decision satisfies none of those future
gates and releases no current feature execution or production.

R1/R2 remain CHANGES REQUIRED on candidate `2c0c9ab`. The tests/wrappers in this
proposal's tree are still the original `52f0e911` / `2782be41` blobs. Only the actual
corrected candidate, independently re-reviewed and approved, can satisfy that gate.
This conditional sequencing approval is not prospective approval of its bytes.

## User authority and the limited supersession

Read the actual root instructions and workflow:

| Authority | Git blob |
| --- | --- |
| AGENTS.md | `24c6f074fbce1f263247bc9dd82ce53412af86cd` |
| FIRST_AGENT_PROMPT.md | `a4ad6f46c1c45fbfdb87589cc6f740ab8b529e51` |
| [AGENT_WORKFLOW.md](../AGENT_WORKFLOW.md) | `6eabf987b00ad6bf0e341ecf9bf3382883967360` |

AGENTS.md's first gate requires environment validation, public-contract review,
independently reviewed specification-linked tests and observed intended failure
before production; missing dependencies, compilation failure and skips do not
qualify. FIRST_AGENT_PROMPT steps 3–4 likewise place independent review before
building/running intended red, without a GitHub prerequisite. AGENT_WORKFLOW's
behavior sequence places red before implementation. Its merge/CI sections require
current checks, actual review of changed workflows, resolved discussions and the
applicable repository rules before merge, and keep intentional red separate and
visible. A local failing baseline is not a green CI/feature result.

The stronger CI-before-FIRST-local-red order was agent-added. If root adopts the
clarification, it supersedes ONLY that ordering in:

- [CI preparation](sessiontest-001-ci-preparation.md), the sentence requiring
  current selected CI before either first feature red or baseline merge;
- [Full harness evidence](sessiontest-001-test-harness.md), the selected Ubuntu
  execution-before-first-red-or-merge sentence;
- [Full test review](sessiontest-001-test-review.md), lines 204–205, where this
  reviewer repeated the root harness ordering.

Those earlier sentences are not retrospectively satisfied or erased. The new
sequence replaces their before-local-red condition, retaining their CI-before-
baseline-integration/merge condition. The full test review's actual R1/R2 findings,
required correction/re-review and every unchanged quality gate remain intact.
Lists requiring selected CI and valid red before production still hold: production
also waits for its prerequisite integration/task gates. No user requirement,
assertion, fixture, CI job, permission or branch rule is weakened. No new owner
product decision or permission bypass is needed for this validation order.

## Actual host evidence, not a package-version assumption

Read the committed source, SDK and first-native receipts and their actual raw
stdout/stderr; no native program, foundation, generator or test was replayed.

| Evidence | Actual Git blob |
| --- | --- |
| [Source authentication](sessiontest-001-source-authentication.json) | `e99b9d9d5136d9a40347ffc903e3d12d1bc40076` |
| [SDK build](sessiontest-001-sdk-build.json) | `8609f0c8cc44ea5f0a96f300f8ccb1aa65e1c5e7` |
| [Different native source/harness review](sessiontest-001-native-review.md) | `b6ff16a7f01340bb1670475494f5fc92f5f2a2ff` |
| [FIRST native results](sessiontest-001-native-results.json) | `c43f982c86b1b7891bfd6f08c8f9575c98f003db` |
| [Exact synthetic input records](sessiontest-001-fixture-inputs.json) | `c083d9f89007770f539122b5759176180e9935a1` |
| [Existing full test findings](sessiontest-001-test-review.md) | `56ad3b1c1589b2d601389acb96c36689b2c7710e` |

The official immutable 1.0.22 source is authenticated by the recorded GPG trust
path and exact archive SHA256
`adbdd8f16149e81ac6078a03aca6fc03b592b89ef7b5ed83841c086191be3349`.
The controlled host SDK build/static prefix and explicit importer have no host
package/PATH fallback. Current read-only sizes/hashes match all three recorded
archive/version-header/pkg-config artifacts; the archive remains
`959cdbe83cd0c80264e080efdd1e861b20b57ded2a85bd9a924297b8a10f1935`, matching
the receipt. Header metadata alone would be insufficient, but the actual native
proof also exists.

Different approval `9b92c671` / import `c0646b4` preceded the FIRST foundation on
clean `c0646b4` at 14:42:52Z: exit 0, one test/zero failures, real runtime 1.0.22/
ABI26/4, published independent vectors and guarded-memory/raw-byte facts. The
subsequent generator supplied two INPUT credentials only. Raw foundation stdout/
stderr hashes and raw generator stdout/stderr hashes match the committed receipt;
generator stdout is byte-identical to the committed input JSON. Both children had
core limit 0. These read-only checks support the recorded observations; they are
not new crypto executions or supported-hardware measurements.

Foundation/generator source blobs remain respectively
`9f688e3d9353311913af9dac8bab4043e1f5b51e` and
`f705227eb4460c551fdabd14cf6460b2b6849299`; immediate-three-throw source remains
`eaf56f41bafd502b8e4cb38c0de3082180bffcd5`. The exact selected dependency is
therefore supported for this recorded host lane, not merely inferred from Arch's
installed label or replaced by another version. This reasoning does NOT transfer
to local Boost1.92 versus selected1.83 or any different dependency/environment.
Arch/GCC16 host evidence does not establish Ubuntu/toolchain/platform equivalence.

## Conditions for the later local operation

Root's four-step proposal preserves these mandatory boundaries:

1. Import actual R1/R2 corrections and obtain the same different reviewer's actual
   full approval. Preserve all 37 enabled names, independent literal inputs,
   unaffected assertions, real/default SDK behavior and three immediate marker
   bodies. Recheck complete 41-case discovery/build/link on the actual candidate;
   do not reuse an earlier source review for changed bytes.
2. On the actual clean exact execution revision, establish that the selected SDK,
   fixture/proof sources and native-link semantics still match the applicable
   recorded host foundation. A changed or inapplicable environment needs its own
   resolved prerequisite; a version string or a fake successful wrapper is not
   enough. Existing valid foundation evidence need not be replayed just to repeat
   a completed gate when its applicability is unchanged.
3. Root may then perform only the explicit ordinary local 37-feature selection,
   with core limit 0 and raw stdout/stderr/JUnit, exact command/time, source/binary/
   SDK hashes and clean-head evidence. Actual separate attribution must show each
   case fails for the intended PasswordHashingNotImplemented behavior. Setup,
   dependency, compile, skip or unrelated failures are not red; no inversion or
   suppression may manufacture a passing result. No red result exists yet.
4. Keep remote selected Ubuntu CI, workflow publication/access, current-head
   checks, reviews, discussions and repository merge rules OPEN until actually
   satisfied. The access request remains genuinely needed for publication. No
   alternate credential route or bypass is supplied. This note does not complete
   SESSIONTEST-001, mark SESSION-001 dependency-ready or release production.

Actual account/current-login/conditional-store/admission, complete secret cleanup,
cost benchmarking, licensing/package and platform gates are unchanged. Local red
is evidence of missing implementation in one validated environment, not a passing
product, remote CI result or authority to merge.

## Actual checks and ownership

Executed read-only `git status/log/show/rev-parse`, source/receipt reads and Python
stdlib file/hash comparisons described above. Documentation commands:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed: 406 task records, 122 decisions, valid links/presets/current HTML.
Only this new review note is changed; staged sole-path/whitespace checks accompany
commit. No compile, SDK authentication/build, GTest LIST, CTest/test body, native API,
foundation, generator, wrapper injection or feature/marker operation was run in this
sequencing review. Root owns actual adoption/status/checkpoint cross-links, later
execution, CI and publication. No push, PR or main merge was performed.
