# MILESTONE-006 Stage 1 slice five

[#238](https://github.com/Kernol12/subathon-bot-ai/issues/238) frozen r2
`89925f45567d1b0dcc8eec58a800aef9d99c99fd5fcf8592476f69aa501c6104`.
Protected `71a1c2e5f661f8d144bac6bdf93a6aaec07d5302`. This slice starts
from credited `844bc4684662005692bfd9b00b2a04e031dad2ca`, tree
`2bb0ab4091da62e9d3cdfac4b477a3185e57e019`, with receipt
`58245a4e7c3c2c1cdd5f9162e074bb419313c830836e5c897d5cafac771df957`,
manifest `8ac47c068f3d950517703c7e3ff377978c25517b133aa0f052df1a018299338e`,
and closing review
`4754c1c524a9b478474902f9911f7e937234b18f5e6276ed81e1650b987872d2`.
All four credited slices, including the slice-four F2/F3/F4/N5 claim limits,
remain unchanged. 162 A and 184 A still apply. This slice uses existing v21
storage and fixture-only current authority inputs.

## Coverage delta

- The protected STORAGE-027 reader already has
  `stoppedOccurrenceExcludesNewRefundPolicy:application-replace-defer-with-real-refund-review`.
  It proves stale refusal and fresh eligibility. Slice three adds unchanged
  Defer, Continue-and-release, Stop, and configuration revision controls.
  Slice five extends the real refund lane with exact before/after hold history,
  stale no-write, selected slot, release and replay/attempt checks.
- Slice three already covers mixed-kind application shape and an application
  A/B wrong-target token. Protected STORAGE-027 covers single-origin edit
  shape and typed mismatch without independent application automation at that
  slot. Slice five enrolls real edit A/1 and application B/1 with the same
  action and version refs in **one** track, then proves isolation across both.
  The application empty/duplicate/nonexistent/no-change rows cover the
  application side. The edit mixed-kind rows use a valid same-kind edit
  patch beside the invalid patch and check both origins for whole-unit
  refusal.
- Slice two covers missing versus denied facts at **preview**. Slice four
  covers preview disclosure and stale bound grants. This slice adds exact
  **confirmation** under missing, revoked and current authority on both
  typed origins. The **retained application result** has a separate authorized
  retry and revoked disclosure/retry control. It does not repeat the preview
  failure or compatibility rows.

## Selector floor

The default-OFF `SUBATHON_BUILD_MILESTONE_006_SLICE3` gate also registers
`milestone-006-slice5-replace.suite`. The [sealed prediction](expected-selectors.tsv)
is nine passing controls and six intended first stops, plus ordinary Qt
`initTestCase` and `cleanupTestCase` hooks. None has been executed on this
candidate.

| Selector | Rows | Oracle and evidence boundary |
| --- | ---: | --- |
| `r5ApplicationRefundReviewInvalidatesDeferPreview` | 1 pass | Real application Defer then exact #236 Continue resolution. Live refund changes score 1,020 to 995 and gives only A a Review at saved target 1,000 and deficit 5. Old token refuses with original selection and exact history unchanged. Fresh under-Review token selects one unstarted bonus, produces one command/version and no attempt, bonus receipt or release. Exact replay produces no duplicate. |
| `r7SharedRefDoesNotAuthorizeCrossOriginOrChangedBytes` | 1 pass | Both origins independently accept a preview with the shared ref. Copying the bound edit token to otherwise-eligible B, substituting payload bytes or route in the selected capture facts, and asserting an unowned actor leave both origins untouched. A newly bound authorized token changes only A/1. |
| `r7ShapeRefusalsPreserveBothOrigins` | 6 pass | Application empty set, duplicate slot, nonexistent slot with a supplied candidate, and exact no-change values lack a preview. On edit A, both opposite kind changes reject the complete patch that also contains valid slot 3. The valid counterpart can preview. Normal exact selection and capture reads and durable snapshots verify no writes or starts on either origin. |
| `r7ApplicationRetryAndDisclosureCheckRevocation` | 1 pass | Real authorized application confirm selects only B. Exact authorized retry returns the stored result. Current disclosure revocation hides both exact result and selection and denies retry, leaving both origins unchanged. |
| `r7ConfirmationAuthorityParity` | 6 first stops | Both origins have real enrolled capture, normally bound preview, and independently retained sibling. Known revocation also proves protected denial before the seam. Then the owner confirmation stub is the first stop for current, revoked and absent facts. After that stop, expect one durable typed selection only for current, denied for known revocation, unavailable for absent facts, and no writes on either origin for both negative rows. |

The confirmation stub maps to the milestone interruption command owner's
`confirm_milestone_replacement(session, authority, exact_command, preview)`.
It may only become one mechanical forwarding call using the existing session,
command, authority, preview and decision types. Protected `stage_command`
collapses absent current facts to denied. The owner must distinguish those
facts before forwarding a granted case to the real storage confirmation.
Closure review must ensure the fixture adds no owner logic. The grants are
synthetic port answers, with no simulated persistence of authentication state.

The protected request cannot carry caller-supplied route, sender or payload
as independent execution authority. The parity control substitutes selected
authority bytes and route against a bound token, and separately supplies
an unowned client actor. It does not invent a client route field or a host
permission producer. Combined edit S1 and warm Hold/replay with media
remain later scope. There is no schema, codec, #294 or #816 change.
