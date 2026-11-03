# STORAGE-016 — bounded private-value source review

2026-11-03, bundle_reviewer. **NOT APPROVED: two cross-stage priority defects OPEN.**
Actual helper commit1cf31dfe, blob `a18a0bd7705f8043daac75acd365471cc59e49c5`
(868 lines), [author evidence](storage-016-values-author.md)3011508/blob
`df9aa41264d5052a39ea03a7bee0d1f158247eaa`, and unchanged old option helperb0ca921c
were inspected against concrete701/physical83e/API46c/headerfe50 and frozen fixtures.
This is a bounded helper review, never prospective runtime/factoring approval.

Confirmed actual findings, sent directly to both authors and coordinator:

1. shape_detection reaches shape_cause's matrix/Exact component checks before
   shape_detection_data validates the required request.audit document. Valid keys/
   counts plus null audit and oversized Exact returns exact_component_limit11,
   although the earlier required-envelope stage requires invalid_detection8.
2. shape_resolution validates expected/entry order while later entry/replacement
   required bodies remain unchecked. Valid keys/counts plus unsorted expected set
   and a replacement's null audit returns invalid_resolution9 instead of the
   earlier required-envelope invalid_detection8.

API46c explicitly orders required scalar/envelope/whole-group shape BEFORE matrix/
Exact and BEFORE local ordering. These findings do not invent a total order among
unrelated same-stage defects. A direct native pure diagnostic confirmed both
actual errors; no Store/session/public factory or baseline was invoked. The helper
author confirmed the findings and holds source correction for the coordinator's
independent additive-regression review and observed reached-red gate.

The rest of the actual bounded audit found no further confirmed blocker:

- Exact old bundle option validation is reused with only invalid_options translated.
  Positive/n+1/count/actual vector maxima and2*entries+1 output capacity use checked
  ranges. All keys/subjects precede nested counts; actual DATA permits empty versus
  unknown. Required branches distinguish CapturedRevision/FailedCapture, numeric/
  timer/policy causes, explicit composition and absent versus known final values.
- Shape helpers omit submitted aggregate and per-vector materialization ceilings;
  actual runtime must prove selected unfiltered counts BEFORE allocation. Source
  existence/current conditions/freshness/active CAS are not fabricated pure checks.
- Every public owning member participates in B/I/equality; repeated first/current
  versions and finite resolution/replacement/event ownership charge actual copies.
  Optional engagement adds0, explicit enum/variant1, fixed signed/unsigned64/count8.
  Checked size_t overflow maps at the appropriate input aggregate tier; nullable
  QByteArray equality preserves null versus non-null empty.
- Independent native count diagnostic matched ALL35 previously frozen B/I oracles.
  Private pure manifest matched all SIX complete canonical payloads/SHA values;
  all12 step6DDL strings also byte-match the approved fixture/physical literals,
  preserving old prefix1–5/full66 and exact new full89/F89890/SHA9f5b.

Scratch C++20/-fPIC/Qt6.12 diagnostics use the new helper, independently reviewed
literal DTO fixtures and old libraries only. Initial scratch linker setup was
corrected; it is not feature red. Completed outputs are source-level diagnostics,
not full runtime acceptance or selected official compiler proof. Actual root/native
full acceptance, old factoring, SQL classes/counts/closure/allocation/rollback and
final config promotion remain unreviewed until concrete candidates are supplied.

| Actual diagnostic artifact | SHA256 |
| --- | --- |
| /tmp/hold016-priority-review.cpp |a5c50b46191402df4ddb65e256065dd7f9645a84b5bbded2a72bea3a07858603 |
| local-evidence/storage016-review/priority-review-run.txt |4e8c031aeb4f5512112aff0e9f80c951bd20781b175b6648a4f4263271e18262 |
| /tmp/hold016-count-review.cpp |740ab554a5b164aeed3a199bd08128d88cc892ae3e080ec167d66b36405eb7b4 |
| local-evidence/storage016-review/count-review-run.txt |aec0b88b1cb4bc46594b200f83a6a2f661d8159a3f742404dc53f7e3969a3f55 |

No C++/test/fixture/shared-file edits by the reviewer; valid first red598 and old
reviews/verification copies remain preserved. Approval requires the actual reviewed
regression/correction artifacts and a new helper pin, not this early candidate.
