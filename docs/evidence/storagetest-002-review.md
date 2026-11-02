# STORAGETEST-002 independent concrete-contract review

Reviewer `/root/contribution_reviewer`, 2026-11-02. **APPROVE the actual logical
and physical DOCUMENT contracts below.** This does not approve a concrete C++
interface, migration source, fixture/test baseline, implementation, future CI or
merge. No such new executable artifact exists in this reviewed scope.

## Actual artifacts and independence

Normally integrated frozen root `4956f56a7965fdf4659599db5bbac95039baee4f`, then
the actual separately authored clarification through root
`035268a242c1809ffff6d56b83756edaf950226f`, into the dedicated clean
`task/STORAGETEST-002-contract-review` worktree. Author revisions were
`404de065f69626324b92ee7c31d5175fe2bd3c88` and
`eba8dc5712b79f17f566daf1992cfe62364d38c5`. Read both entire contracts, their
author evidence, specification §§3.3–3.5/4/8/16 and DB-01/NUM-01–04, the accepted
logical bundle, actual core values and existing writer/retention/receipt/
contribution/plan boundaries. The coordinator retains shared integration ownership;
this reviewer owns only this new evidence file.

| Actual accepted artifact | Git blob |
| --- | --- |
| [Concrete logical/API proposal](../contracts/sqlite-application-bundle-v1.md) | `cf4624bf113b375c8614b4163b83245be67d9f0a` |
| [Exact physical DOCUMENT](../contracts/sqlite-application-bundle-schema-v1.md) | `f13fca6d11e57410a31a7d307a8b12eaa21a68f9` |
| [Author evidence, checked for scope/attribution](storagetest-002-api-author.md) | `b737a1ab6cc594739a8534d98e25e357bf6e521d` |

Accepted logical predecessor `3c8cbd869800f6aa4cd117016ef568f789d3ace9`, plan
logical `3af256cdf82a4737c2fc71a67c88b8b332d6ba71`, physical
`99bf45b46f6c9d361b899fdb975dc2db51da4554`, plan header
`4634d7b57dccc5c559f2ab05a968833b4b925feb` and writer header
`afc9f483da7d8e61a5e73715ca33819216e9e668` remain unchanged. These reviewed
documents introduce no dependency, executable schema/source, build or workflow.

## Findings resolved in actual documents

Early actual-draft review identified the one-basis-per-request restriction and
missing direct-component equality. Final SQL permits MANY original component
links for ONE immutable current occurrence; SB13 retains increments -20/-10 for
the one captured request -30, without synthetic request splitting. Direct role0
now requires its sole score/count component exactly equal requested units. SB14
therefore rejects component -1000 for request/link -20 even if a matching after
balance was supplied. Exact arithmetic and basis bounds remain explicit storage
conservation checks; rounded entitlement and financial correlation stay upstream.

During full review of original logical blob
`6d7e1d5ff3759550fbfb4f1dccc1e0c21eea5e69`, the phrase original-kind “complete
application” conflicted with the underspecified targeted source projection. The
actual replacement above resolves that finding. It enumerates actual sibling
COUNT/key/order/group/kind metadata for each snapshot, effects, components, jobs,
transitions and absent original-operation reversal children. It fully checks the
selected original effect's complete component set, including refundable-Main
debt-repayment0, and its actual selected plan/source. It explicitly does not
certify unselected amounts/rationals/job bodies or whole-operation numeric deltas.
Metadata traverses64-row pages/65 lookahead without applying aggregate read caps
as lifetime quotas. Full lookup retains its separate full selected-record scope.
Direct comparison confirms all14 SB rows and the entire physical document remain
byte-identical through this clarification; earlier diagnostics still apply.

## Boundaries assessed

- One inert borrowed-session port; all reads, duplicates, claims and stage returns
  are provisional. Prior intake/capture COMMIT is an honest caller precondition.
  No owning13/14 wrapper, hidden COMMIT, destructor cleanup or inferred durability.
  Known B08 rollback, unknown acknowledgement/cleanup and post-COMMIT body failure
  remain distinct; publication requires actual normal committed completion.
- Immutable head/advanced-before/after snapshots preserve CAS coverage and forbid
  token rebind/reuse/ABA. Clock-only advancement preserves debt/counters/config/
  format. Exact actual-component deltas apply from advanced-before to after.
  Complete CURRENT contribution comparison is separate; original application
  patches only application and preserves unrelated current dimensions.
- Full/targeted/absent scopes, typed physical source closure, complete plan-history
  structural/quote/dependency audit, selected-body provenance, CID0 versus absence,
  positive full-range receipt/revision fields and historical-state freshness are
  explicit. No support/event receipt equality or authority from semantic strings.
- Intent differs from computed outcome; original subject, source receipt and scoped
  reversal identities cannot be bypassed with another key. Immutable request/
  component identity preserves duplicate action occurrences. Actual positive-credit
  bases, cumulative bounds, ordered per-basis progress and both head paths require
  runtime equivalence; SQL FK existence alone is not that proof.
- Signed64 extrema, canonical arbitrary-precision VALUE components, independent
  byte/count/total bounds, n+1 allocation/65-row continuation, pure-input priority,
  selected stored-error priority and cumulative writer budgets are specified.
  All allocations/bindings must precede own writes where possible; every later
  failure needs guarded whole-unit rollback, with actual uncertainty/capability/
  protocol errors preserved. These are future implementation obligations.

## Own independent checks

Host Python3.14.8 / stdlib SQLite3.54.0. Scratch
`python3 /tmp/storage002-review-independent.py` derives expectations directly from
the exact14 document SQL literals, manually enumerated new table/index inventory,
the framing definition and already-frozen prefix fixtures. It does not call a new
producer/manifest/codec or reuse author/root diagnostic scripts as its oracle.

| Actual check | Observed outcome |
| --- | --- |
| Four prefix catalog/framing comparisons | Exact7/13/20/30 objects and2000/10535/18126/29015 bytes; each frozen payload and digest reproduced |
| Proposed step5 expected versus actual catalog |14 tables +22 SQLNULL autoindexes; full66; exact columns/SQL byte lengths and unchanged prefix30 |
| Own canonical step5 framing | F62465 / SHA256 `c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb` |
| Independently supplied raw data | Actual prefix FK targets, CID0/absent subjects, two original Main bases100/50, ONE current request -30 and TWO links -20/-10; foreign_key_check empty, integrity_check OK |
| Disposable constraint probes |28 expected SQL refusals covering types/NULL/domains/groups/source targets/component ownership/reversal positions/head targets and uniqueness |
| Explicit SQL-admitted cases |6 runtime-only obligations: head cumulative equality, claim subject equivalence, application-after role, declared balance count, canonical rational reduction and component conservation |
| Independent Fraction arithmetic | SB03 partitions, SB05 cumulative residuals, SB11 half-even residual literals, SB13 sum and SB14 direct equality pass |
| `python3 tools/check_project.py` | PASS391 tasks/122 decisions/current links/presets/HTML, both original candidate and actual replacement |
| `python3 tools/progress.py render --check` | Current, both revisions; no shared edits |
| `python3 -m unittest discover -s tests/tooling -v` | PASS19,0.282s at4956; unchanged tooling not rerun for prose-only replacement |
| Direct old/new comparison and `git diff --check` |14 SB rows/physical/predecessor pins unchanged; PASS |

Scratch source SHA256
`d88c0f4737a472b35908ec0a6ea78f53afdd0ff88c8f4f3b6d9c0b04068793e1`;
summary `/tmp/storage002-review-independent.json`, SHA256
`585e6c40e7103296928f7d3fb05b4d4bfb6cbc954738069134c19fdac2e64dc5`.
Initial scratch setup omitted existing catalog/history bootstrap tables; those
frozen prefix tables were added before the successful complete diagnostic. This
was diagnostic setup, not feature red. Raw rows establish SQL representability,
not complete current support-anchor/semantic application validity or transaction
behavior. SQL-admitted corruptions are explicitly required runtime rejections.

## Remaining gates

Next comes an actual minimal owned C++ API/immediate named stubs and separate
API/build/config review, then independent literal fixture/old-writer environment
and genuine feature tests/full baseline review, selected official environment
proof and FIRST intended missing-behavior red. STORAGE-002 production must wait
for that baseline, then pass acceptance, separate source/config review and actual
automatic official CI. No native suite, new facade, old red or live action ran
for this review. Root retains latest published-head CI/discussions/rules and normal
merge checks; no future revision or merge is approved here.

Actual authority, numeric policy, full state/job/wire/recovery/delivery meaning,
provider correlation and refund allocation across prior debt remain owning gates.
ACCT-004/STORAGE-002 consume this contract, not prerequisites creating a cycle.
All seven integrations/three FX and full application/UI/platform/package/security/
backup/recovery/live release requirements remain open and unchanged.
