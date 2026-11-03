# STORAGE-016 — read-only source review preparation

2026-11-03, bundle_reviewer. **PREPARATION ONLY; no implementation approval.**
New isolated task/STORAGE-016-review worktree at claim4622646, integrated main017d40f.
Old baseline review tree and its configuration/API verification copies are preserved.
Read task brief, server instructions, concrete701c710f/physical83e96362/API46c70297/
headerfe50fb10 and the accepted baseline/full/correction/final reviews. Valid red598
is265 named markers/70 init-cleanup PASS/0setup/skip; it was not replayed.

Actual unchanged code read for this map:

| Artifact | Git blob |
| --- | --- |
| sqlite_application_bundle.cpp |834336d34f532508cf35d9eabb58f8ed8f0e71bf |
| sqlite_application_bundle_values.hpp |b0ca921cb27068ecc33228d136b82506fcb6e0a3 |
| sqlite_writer.cpp |023451987e40a9330fef91c6c0955f0e527f14e6 |

Concrete reuse/cautions sent directly to both actual authors:

- Private Access::plan(key,false) audits contiguous scalar revision metadata and
  loads the selected full body; state(token,false) resolves historical snapshot/
  configuration/balances. Public read_inputs/current impose other freshness scopes
  and are unsuitable substitutes. Internal plan/state/original decoders do not all
  perform their public caller's final result_bytes/result_size pass: apply the
  declared reference-body caps explicitly without fictitious hold return charges.
- original/contribution_by_id/observation provide strict same-session source pieces.
  resolve(arrival) checks retained-row existence, not the hold path's complete
  context.configuration_ref equality. Compare actual receipt/config/context fields,
  current event observation CID and immutable ORIGINAL support anchor separately;
  do not compare mutable current contribution conditions to historical capture.
- Access::fail performs actual rollback; execute maps writer failures already
  aborted by the writer. RetentionFailure may already be terminal. OLD guarded
  recognizes bundle/work exceptions, not new hold errors; a new wrapper must own
  its cleanup and wrote_ flag so a hold-local failure cannot cause second rollback.
  Actual rollback uncertainty overrides a known local reason; original wrong-thread/
  stale/closed/protocol/uncertain WorkFailure remains original and sticky.
- Preserve old namespace/options/queries/byte walks/result charging through factoring.
  OLD bundle options must remain unchanged; new pure code translates option error
  tags deliberately. Historical stored shape checking must not use submitted
  input_items/owned_bytes quotas. Every fixed hold query still has its own input
  byte charge, unlike the distinct017 selector contract.
- Hold metadata requires actual unfiltered counts/type/domain/min/max and complete
  visited chains, first page without a hiding lower bound, all inactive anchors
  before filtering, finite64/65 buffers and no lifetime full-body cap. Actual full
  occurrence/expected/entry/replacement counts must precede allocation; actual
  source review is required because baseline tests do not observe allocations.
- B/I charges actual owning copies: Cause once, firstDetectionData, both Version
  members and actual current event; finite historical resolutions own replacements
  and produced versions. Optional engagement adds0, explicit enum/variant tags1.
  Returned objects do not absorb unreturned selected reference projections.

No C++/test/fixture/shared-file edits, native runs or prospective verdict. Actual
pure helper, runtime/factoring, final root promotion,35/full264 acceptance and own
selected ordinary CI still require independent review of their concrete revisions.
