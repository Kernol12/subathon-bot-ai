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
