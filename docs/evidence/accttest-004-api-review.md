# ACCTTEST-004 concrete service document review

**APPROVED — DOCUMENT ONLY**, 2026-11-03, independent reviewer
bundle_reviewer. This approves the actual final proposed boundary and meanings;
it does not approve a future header, stub, codec, fixture, test or implementation.

Reviewed author revisions d421471b927636cd159794adea223d5b5d49b3c2,
bd9b354751e314222b71779f04884ad5e4b775f0,
6160e352613b2c1e5e0776099be9c648fd7d6a95 and
ee05694510dbe3892d97a56ce52c65441a3a08b4, including the complete original
document, both actual correction diffs and final author evidence:

| Artifact | Reviewed Git blob |
| --- | --- |
| [Concrete API/codec document](../contracts/application-service-api-v1.md) | 8fa0c76ad90ad046712463dfc95206e2a99b4e40 |
| [Author evidence](accttest-004-api-author.md) | 608acc129c48f8ee5ed97fd09db1ade541511c56 |
| [Accepted logical service](../contracts/application-service-v1.md) | 6d0cd67f487dae92a17d41eb07dda5b407c78a56 |
| [ADR-0003](../decisions/0003-application-service-prerequisites.md) | e971a47e64b7cabffa19e325cee698378a2d0a8a |

The source comparison used integrated main
efe1c7e87ae2a7e8a92c3f5e7ede88e68e72b089 and its actual writer, retained,
bundle/early-claim, hold, captured-plan, contribution, timer and time-debt APIs.
Their unchanged header pins are recorded in the author evidence. Review work used
the dedicated task/ACCTTEST-004-service-api-review tree after claim5236d88;
only this review file is reviewer-authored.

## Findings resolved in the actual document

- A historical detection retry cannot establish a current hold. Only freshly
  recorded creation with the exact mutation/hold/version outcome can produce
  held_numeric/held_policy. Already_recorded maps to typed stage_conflict; all
  actual detection dispositions and impossible outputs are accounted for. CS16
  correctly distinguishes resolved-current history from its old active version.
- Dependency lifetime now covers every outstanding prepared pair, including an
  unsubmitted pair. The matching real Work/WorkCompletion pair preserves the
  acknowledged-COMMIT/body-tail-exception case without fabricated completion.
- The action proposal has finite token-free members, issuer success echoes the
  complete binding, and construction order is calculation, jobs, required refs,
  then token-dependent codecs/bundle. No placeholder token or regeneration loop
  is required.
- Callback-view expiry/foreign access is distinct from a real writer ticket
  failure. Pure retained-key errors have not aborted SQL; selected retained/query
  errors have. Escaped local view errors map to port_contract/invalid_output
  after one active cleanup; actual uncertainty dominates. Missing Completion and
  foreign preparation/admission now have explicit, representable error paths.
- Public encode/decode, intermediate values and final response have separate
  exact B/I targets and error contexts. Opaque documents do not add fictional
  parsed vector items. Bounded scanning/projection precedes materialization.
  CORE002 consumes the current pre-occurrence timer, including preceding effects;
  CS18 preserves 58→60→60 rather than resetting or adding twice.

## Coverage and actual checks

Read every section/member/format/example against specification §§3–5, NUM-01–04,
B08, the logical AS01–21 obligations and integrated storage contracts. The final
surface preserves access before historical disclosure; exact complete command
comparison; global early claims and all active holds before new calculation;
independent current plan head/all15 contribution dimensions; segmented hard end
before final admission; cap/debt/Grace/wait/signed-MIN behavior; whole-input hold
atomicity; honest stale/missing/claim dispositions; and actual COMMIT plus normal
completion before response/publication. Known malformed retained meanings remain
distinct from healthy unsupported formats. Opaque documents grant no authority.

Independent scratch literal/integer/Fraction checks, without repository imports,
SQL or any feature call:

- `python3 /tmp/acct004-review-literals.py` — PASS. Script SHA256
  528438a21e7205d8c0ff002642442ad3b51cf3f5cecd5c0f507a790f10dc2b2a.
  Reproduced command58/66/62/73, Clock39, Effect88, SAS1=90, SAR1=182,
  SAA1=178; logical command25/33/32 and returned Envelope98. Checked exact
  conservation/ordered balances in CS01–03/13/18 and CS17's distinct 25→98/98→25
  ownership boundaries. These are document examples, not healthy SQL fixtures.
- `python3 tools/check_project.py` — PASS396 tasks/122 decisions/local links/
  presets/current HTML; `python3 tools/progress.py render --check` — PASS.
- `git diff --check` — PASS. Diff from claim to imported final candidate contains
  only the two author-owned documents. Environment: Linux/Python3.14.8.

No CMake/native baseline, codec/service entry point, stub marker, provider or live
operation ran for this review. Separate bounded codec review is an additional
gate, not replaced by this record. Minimal actual API/stubs/build review, independent
literal fixtures/tests, full test review, own selected foundation green and the
first genuine same-clean intended red remain required before implementation.
Authentication/disclosure, current policy, clock lineage, reference uniqueness,
real jobs/delivery, manual derivation, failed-capture producers, mixed/post-finish/
paused/recovery/continuation/refund commands and all product/platform/live release
obligations remain mandatory owning-task gates; none is enabled or waived here.
