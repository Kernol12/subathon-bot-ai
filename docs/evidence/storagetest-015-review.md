# STORAGETEST-015 independent contract review

Reviewer: contribution_reviewer, 2026-11-02. **APPROVED: actual logical and
physical proposal only.** No blocking finding remains for the next separately
reviewed minimal API/stub proposal. This does not approve an absent API, fixture,
test baseline, codec, schema factory or persistence implementation.

Reviewed integrated revision `93136966f61a85f82e996d3e586500a4adbd8bcc`:

| Artifact | Git blob |
| --- | --- |
| [Logical proposal](../contracts/sqlite-application-plans-v1.md) | `1b204cbc61b065079e1792a2bea6356b11482c79` |
| [Physical proposal](../contracts/sqlite-application-plans-schema-v1.md) | `99bf45b46f6c9d361b899fdb975dc2db51da4554` |

The logical author's original blob `6e37b3720788db15c32af674adad67aee58292c8`
differs only by the four-line link to this actual physical companion. I read both
complete documents, the actual core application-plan model/source and original
22 tests, Exact type/contract, and the applicable event-state, writer, retention,
receipt and contribution contracts. Existing core/server/tests/schema/build/CI
paths are unchanged from merged main `71139958465e7dde17a661a93b288fd825d1c09e`.

## Scope and model consistency

- First revision is 1; later complete histories are contiguous with immediate
  parent and authorized override audit. A pure codec may represent 0..UINT64_MAX;
  persisted revision/head/parent fields are 1..UINT64_MAX. NULL alone means absent
  parent. ReceiptId excludes zero; optional ContributionId preserves present zero.
- Every owned capture, ordered rule/modifier/request vector, override audit and
  original/effective/fetched valuation spelling is retained. All eleven historical
  SQL cells map to the actual HistoricalValuation members, including original
  money's two fields, source, both timestamps and manual decision. Optional groups
  remain distinct from present empty strings and signed timestamp/unit extrema.
- Exact is the current arbitrary-precision rational VALUE. Canonical reduced
  numerator/positive denominator and zero 0/1 preserve that value; they do not
  invent an original rational expression. Independent 16384-byte component bounds
  do not reuse the numeric parser/finalizer's unrelated limits. SQL byte checks
  alone do not establish canonical grammar, gcd or successful Boost decoding.
- Full reads validate/materialize a bounded complete history. Targeted reads
  validate their declared selected body, local immediate parent and initial
  anchor. Sparse 1,MAX-1,MAX with intermediary parent 1 is globally INVALID;
  selecting MAX can demonstrate exact local fields without certifying that
  intermediary's chain/body. Full find and append must refuse its count/head gap.
- Append's paged structural audit checks every immediate parent/audit and required
  core capture/valuation/dependency-flag metadata. It does not certify unread old
  rational/string payloads. A full-read materialization cap is not a write quota;
  paged work still consumes cumulative existing session budgets.
- The store is inert over a caller-owned active session. Staged results are not
  commits. Healthy no-own-write domain refusals preserve unrelated staged work;
  known SQL failure, rollback and unknown outcomes retain writer semantics. Trusted
  prior-commit/source normalization, semantic authority and authorized upstream
  decisions remain preconditions, not new permission/evaluator/provider claims.
- Physical references use actual receipt/contribution/configuration/context
  namespaces. Receipt support is not incorrectly required to equal the event's
  original receipt. Opaque semantic refs gain no invented configuration FK.

## Independent SQL and framing observations

I independently extracted the five exact SQL strings from the actual Markdown,
used only the frozen retained/receipt/contribution prefix fixtures, and derived
the expected new table/NULL-index tuples and catalog F framing. I did not read
root's scratch JSON/script as expected data or call a future producer.

Command: `python3 /tmp/storagetest015-contract-review.py` (scratch diagnostic,
SHA256 `72d6fa61eec1ddd1fc7909d76feb67859e6eddab8329b2df951ca85982914a30`).
Actual environment: Python 3.14.8, stdlib SQLite 3.54.0. Exit 0:

- Exact 30 schema objects, including five new generated indexes with SQL NULL,
  and byte-exact unchanged prefixes 20/13/7; five initially empty tables with
  7/34/7/10/14 columns. Canonical full payload is **29015 bytes**, SHA256
  `bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50`.
- **314 focused engine cases**, including **217 expected constraint refusals**.
  These exercise type/NULL truth, every own-money/decision required-presence
  combination, all 64 required and 32 optional quote-presence combinations,
  unresolved quote absence, enum/bool bounds, initial/later parent/audit roles,
  same-plan parent/child membership, duplicate primary keys and ordered children.
- Duplicate content at different ordinals and multiple plans per receipt/event
  remain legal. Missing CID cannot borrow a receipt identity. Present CID zero
  and present empty parent-event BLOB remain distinct from SQL NULL. Raw selected
  MAX/parent, signed unit MIN/MAX and large rational component BLOBs retain their
  actual SQLite class/bytes/value; foreign-key checks return no violations.
- Sparse MAX data remains explicitly INVALID as a complete history. SQL permits
  noncanonical rational bytes by design; runtime canonical decoding remains a
  separate required gate. These disposable direct rows do not establish a valid
  product capture, prior committed anchors, aggregate validity or valid exhaustion.

I also read the complete root Qt proposal consumer and
[root diagnostic evidence](storagetest-015-probe.md). Root records an actual
formatted -Werror build and old-writer transaction/projection/data pass on
Qt 6.12.0 / SQLite 3.54.0. Its source uses existing SqliteWriter/Qt only, preserves
the same deliberately invalid sparse-history distinction, and makes no new-store
or codec claim. This is attributed root evidence; I did not repeat that native
run. Neither host diagnostic proves selected official bundled SQLite behavior.

## Checks and remaining gates

After review, these actual commands passed:

```sh
git diff --check
python3 tools/check_project.py
python3 tools/progress.py render --check
```

Project check reported 391 tasks/122 decisions and current HTML; its 19 draft
GoogleTest cases are not a product-baseline approval. No old red or unchanged
native feature suite was repeated. No existing assertion, fixture or review was
altered.

Next gates remain concrete API/result/error-priority review, explicit field-bound
and overflow-safe count mapping, exact targeted scope and paged-audit behavior,
lossless codec and factory review, separately authored/reviewed literal fixtures
and tests, successful compile and actual official green, then the first genuine
missing-behavior red before STORAGE-015 implementation. Later source review and
passing acceptance are separate. Latest published revision CI, discussions and
merge rules remain coordinator gates; this approval cannot preapprove later edits.
Full application/effects, authority/providers/FX, recovery/platform/UI and all
seven integrations/three rate providers remain in release scope.

## Actual minimal API, stub and build review

2026-11-02, contribution_reviewer. **APPROVED: actual minimal API/stub/build
precision stage.** Reviewed root candidate
`f24ac08ac182b5c749b407d7323334b452912bee`, normally merged into the isolated
review tree at `c6d7566247dc21f8b94bff1e6fec3847dc31d25e`. No blocking finding
remains for separately authored tests, literal fixtures and green diagnostics.
This is not their approval and does not authorize production implementation.

| Actual artifact | Git blob |
| --- | --- |
| sqlite_application_plans.hpp | `4634d7b57dccc5c559f2ab05a968833b4b925feb` |
| sqlite_application_plans.cpp | `e989d70f5021a9285db0dbc79c32d1153b4e7700` |
| Logical contract plus Concrete API precision | `3af256cdf82a4737c2fc71a67c88b8b332d6ba71` |
| Root CMake | `58e443305f4c5fbc5834d0946e3a7facdf416e42` |
| Presets | `16cdf68c05f10907a6b6de24fd2032bcf05bb37d` |
| Core CMake | `90de2e1e3add9063066c58fb843b2eb2d648b58d` |
| Server CMake | `5f4a3179d1e16a77d7da421d91799cbb51037d0b` |
| SQL workflow | `81ba8204cfc8d10abec8ec3f8b7a87c710d132ac` |

I read the complete 132-line header, 41-line source, all 234 appended precision
lines and five build/workflow diffs. Independently verified the previous approved
logical bytes are an exact prefix; physical blob `99bf45b46f6c9d361b899fdb975dc2db51da4554`
is unchanged. Source has exactly nine immediate named NotImplemented throws plus
the inert session/options constructor. No validation, serializer, Exact/numeric
codec, schema factory, target resolver, audit, SQL statement or writer behavior is
hidden in the minimal source.

The concrete signatures and precision preserve these reviewed boundaries:

- Copyable const/repeatable store facade owns no transaction, cleanup or destructor
  promise. Valid calls rely on actual ticket owner/phase checks after pure input
  validation. Staged insertion/append is distinct from the caller's real commit
  and publication; failures of that caller commit remain original WorkFailure.
- Expected head zero is a valid stale token; a targeted stored revision zero is
  invalid input. Candidate-local immediate-parent shape is checked separately
  from matching the stored head. Healthy stored audit precedes stale, revision
  conflict and then complete-event conflict; these make no own writes and do not
  resolve proposed replacement targets or discard earlier caller work.
- Absent header means absent/missing plan without a global orphan audit. Targeted
  lookup checks the declared header/head/max and actual initial-anchor scalar,
  quote-original-money and request-dependency metadata before returning an absent
  revision. Found snapshots validate their full body and local parent identity,
  not the unselected parent's history. Full-find summary precedes read caps;
  over-cap refusal does not certify unread payloads. Existing initial retention
  and append perform the declared paged structural audit independent of read caps.
- The private audit page is 64, with ordered strict-key progress and checked
  termination/lookahead. Initial-anchor and old revision metadata include the
  actual missing-historical dependency flags; unselected child payloads/Exact or
  initial ordinals are not incorrectly certified by a metadata-only check.
- Present CID selects actual ID, original ReceiptId and its matching receipt
  identity target. That limited projection neither validates nor imposes equality
  on unselected current contribution state, source facts/context or support anchor.
  Full contribution validation and trusted relationship/authorization semantics
  remain existing upstream obligations. Typed zero CID and NULL remain distinct.
- All actual HistoricalValuation fields and independent contribution dimensions
  retain their types/presence. The full field-to-bound table covers refs, value
  spellings/documents/reasons and independent Exact components. Count predicates
  reject nonpositive/unrepresentable values before conversion and reserve n+1
  against signed/Qt/vector limits; checked full-find totals are not a fictitious
  combined vector or an append/history quota.
- Pure Exact functions handle canonical rational VALUE, not source expressions,
  parsing or rounding policy. Their explicit null/size/grammar/gcd ordering is
  distinct from selected SQL class/shape/size checks. Large fitting values above
  4096 bits remain required; wrongly typed/noncanonical versus correctly typed
  over-limit stored components have distinct specified failure precedence.
- Known port failures require actual rollback, including mapped retention errors;
  inactive cleanup alone is not proof. Capability/protocol/invalid-statement and
  actual unknown outcomes remain original writer failures. Partial-own-write
  allocation guard requirements cover the whole caller unit; bad_alloc is not
  automatically relabelled resource_limit. No-own-write refusals do no cleanup.

Build review confirmed the existing Boost dependency is propagated through the
core plan target without introducing Qt into core or enabling GoogleTest baseline
flags/fetching. The new plan profile inherits all old SQL flags. Independently
compared the complete workflow body from the environment-probe step onward:
byte-identical after only replacing the profile path. All old registration checks,
unconditional suites, action/archive pins, permissions and check name remain.
Pinned Boost 1.83 headers/package/config checks are explicit; actual workflow
execution is attributed separately below.

Actual independent commands, all successful:

```sh
cmake --preset sqlite-plan-debug
cmake --build --preset sqlite-plan-debug
ctest --preset sqlite-plan-debug
ctest --preset sqlite-plan-debug --show-only=json-v1
/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Fresh configure/build: Linux x86-64, GNU 16.2.1, CMake/CTest 4.4.5, host Qt 6.12.0
and Boost 1.92.0; **74 steps, zero warnings**, including the new source/library.
Cache confirms APPLICATION_PLAN_TESTS, EXACT_NUMERIC_TESTS and FETCH_GOOGLETEST
remain OFF, SQLITE_PLAN_TESTS ON. Exact registration count is **143 inherited
cases**, with no plan feature registration or invocation; all passed in **3.25s**.
Logs/registration JSON are `/tmp/storagetest015-review-{configure,build,ctest}.log`
and `/tmp/storagetest015-review-registered.json`. Project/render checks remain
current. These are minimal-build/inherited regression observations, not new plan
green, red, official bundled Qt or product behavior evidence.

Coordinator reports actual official run `41488687802` SUCCESS at exact published
`f24ac08ac182b5c749b407d7323334b452912bee`. I read its downloaded full log at
`local-evidence/coordinator/storagetest015-interface-ci-f24ac08.log` in the root
checkout: both selected Qt archives OK, installed Boost 1.83.0-2.1ubuntu3, actual
header macro 108300/configured Boost 1.83.0, GNU 13.3 and 74-step new-profile build
including new source/library. All existing automatic 135 SQL cases and required
old 7/3/3/3 diagnostic functions passed. This is root-observed CI with independently
read log, not my own remote invocation or a future-head result. No new plan
fixture/diagnostic/feature existed or ran, so it supplies no such behavior proof.

Separate actual fixture/test/green-helper review, literal independent expected
data, compilation, selected official diagnostic execution and first genuine
missing-behavior red remain required before STORAGE-015. No previous red was
repeated, no old assertion/source/fixture weakened, and no unprovided test or
implementation is approved. Latest-head CI, discussions, rules and publication
remain coordinator gates. The earlier full-release limitations remain unchanged.
