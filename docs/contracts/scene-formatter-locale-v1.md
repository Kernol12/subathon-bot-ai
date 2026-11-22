# Scene formatter locale v1

[CONTRACT-008 #160, owner decision 160](https://github.com/Kernol12/subathon-bot-ai/issues/160) ·
Tier B review candidate against protected base `c905cc840e11a33224d09fb5bc94ae4d6c57724d`.
[Specification v0.45 §12](../specification.md#12-localization-and-appearance)
gives the streamer control over Scene language and formatting. This contract
supplies the registry for [CONTRACT-010 §2](scene-bindings-conditions-timeline-v1.md#2-values-destinations-and-formatting)
and the semantic checks of [CONTRACT-012](scene-schema-validation-v1.md).

## 1. Closed registry and Scene selection

| Exact `localeId` | Decimal separator |
| --- | --- |
| `en` | U+002E full stop |
| `de` | U+002C comma |

The versioned registry owns formatter rules and membership. Adding a language
requires a new registry version. Identifiers match case-sensitively without
normalization, detection, or region fallback.

Resolve the immutable content revision's exact selected 010 definitions first.
Every applicable [`Formatting` definition](../../schemas/scene/v1/bindings.schema.json)
in the selected closure must resolve in one registry version and name the same
registered locale. Include transitive layout-slot definitions and selections
whose values are currently unavailable, hidden by a condition, or in an
unselected runtime layout branch. Unreferenced library definitions are outside
this closure. A revision with no applicable formatter needs no locale selection.
Missing exact definitions, unregistered IDs and disagreement are separate
semantic refusals.

The streamer chooses the locale in SCENE-003 #437's binding/preview editor.
SCENE-001 #433 and SCENE-004 #439 own selection and edits that save fresh
immutable content with any changed formatter definitions. Preview evaluates
the explicit candidate and supplied observations without affecting live state.
[CONTRACT-009 §§6–7](scene-document-v1.md#6-drafts-publication-and-restore)
and SCENE-007 #445 own authorized publication and restore. Restore selects the
historical content and its formatter selection under the existing head,
authorization, closure and compatibility checks. LOCALE-002 #501 and LOCALE-003
#503 own personal language/preferences, including detection, saved choice and
English fallback. Those choices, the host locale and personal number, date
and currency preferences have no effect on Scene selection.

**Exact version and catalog selection.** The supported 010 owner identity
`subathon.scene-bindings-conditions-timeline.v1` selects formatter registry v1.
The reference catalog is [localization/messages.json](../../localization/messages.json)
with source SHA-256 `341aa5c139e2a51d25183d5747b33c87cdfcff1659225783f4392bee155a87f4`.
This pin records the exact JSON source for both generated representations.

For that selection, freeze all dispatched Scene message IDs, parameter names
and types, plural selectors, locale/form names, and exact decoded form text.
A later catalog build qualifies for the same selection only if its generated
Qt and browser forms on every intended output match those pinned records,
including the text and placeholders. Unrelated catalog entries and translator
descriptions may change without changing the selected Scene forms. Presence of
a form alone is insufficient.

A change to a selected Scene record's text or interpretation requires a new
reviewed catalog pin and explicit compatibility and upgrade review under 012
before any Scene can select it. The original selection retains its exact
resources. A deliberate locale or formatter change creates fresh immutable
content. An app upgrade preserves each existing Scene's selection. A missing
exact supported version or dependency fails compatibility. Existing versioned
references carry the initial selection without an additional wire field or
package entry.

## 2. Built-in text

[The JSON catalog](../../localization/messages.json) is the single source of
built-in Scene translations, descriptions, parameters and forms. Both
renderers use the same closed dispatch and generated resources under
[LOCALE-001 §§2–6](translation-catalog-v1.md#2-authoritative-source-shape).
The `catalog-label` formatter selects a parameterless ID for a known enum or
Boolean in a matching typed destination. Count formatters select their own
parameterized IDs. Each suffix comes from the closed alternatives below.
Scenes supply typed selectors and formatters. They have no arbitrary message-ID
or template field.

| Typed selection | Stable catalog identity |
| --- | --- |
| `timer.lifecycle` | `scene.timer.lifecycle.` followed by `ready`, `active` or `finished` for Ready, Active or Finished. |
| `timer.phase` | `scene.timer.phase.` followed by `main` or `grace` for Main or Grace. |
| `timer.awaitingConfirmation` | `scene.timer.awaiting_confirmation.false` or `scene.timer.awaiting_confirmation.true`. |
| `pause.active` | `scene.pause.active.false` or `scene.pause.active.true`. |
| `pause.reason(reason)` | `scene.pause.reason.false` or `scene.pause.reason.true` for the Boolean membership of the selected reason. The same pair serves Manual, Schedule, StreamOffline, DesignatedClientDisconnected, RecoveryReview and StreamStatusUnknown. |
| `pause.streamUnknownNotice` | `scene.pause.stream_unknown_notice.false` or `scene.pause.stream_unknown_notice.true` for the known notice flag. |
| `modifier.active` | `scene.modifier.active.false` or `scene.modifier.active.true` for the selected instance. |
| `modifier.mode` | `scene.modifier.mode.active_subathon` or `scene.modifier.mode.real_elapsed` for ActiveSubathon or RealElapsed. |
| `track.position` | `scene.track.position.` followed by the exact enum tag `empty`, `before_first`, `between_targets` or `all_completed`. |
| Recent-support kind | `scene.support.kind.` followed by `subscription`, `gift`, `bits` or `donation` for the already-selected Subscription, Gift, Bits or Donation classification. |
| Matching recent-support count slot | `scene.support.count.gift` for `gift-count` on Gift and `scene.support.count.bits` for `bits-count` on Bits. |

For either count, `quantity` is a text argument with the exact unsigned64
quantity written in ASCII decimal digits, including `0`. The separate
`selector` is a canonical signed64 integer string, `1` when the quantity is
exactly one and `0` otherwise. Both forms display `quantity` once and omit
`selector`. LOCALE-001 selects `one` or `other` with Qt lookup witnesses 1 or 2
respectively. The visible count can exceed signed64. Keep the displayed digits
through selection and substitution without narrowing or converting the count to
binary float. The browser applies the same category rule to the selector string.

RENDER-002 #449 uses explicit Scene-selected QM resources, the mandatory
English QM, and generated `web/en.json` declaration metadata. Look up the raw
form on the selected `QTranslator` with empty context, the stable ID and the
contracted witness (−1 for an ordinary entry). RENDER-003 #451 uses the
Scene-selected browser descriptor and forms with the mandatory English base.
Both apply one-pass named substitution and emit plain text. Require the exact
selected locale and every reachable form for every intended output, including
values that can appear after publication. LOCALE-001's adapter fallback still
applies at its own boundary. A fallback for a missing selected resource or form
cannot satisfy exact Scene support, and an unavailable English base fails the
adapter contract. The publication path checks compatibility before selecting the
revision, and a renderer that loses exact support reports that failure.

Apply 010 availability and disclosure before selecting a label. A known false
or zero is displayable. Absent, unknown, undisclosed and explicit null follow the
authored placeholder or omit rule for the slot. Inactive timer phase and
confirmation are inapplicable, and incomplete evidence for membership supplies
no false label. A catalog failure is a compatibility failure. Authored text,
unavailable placeholders, progress-state text, admitted names, milestone
descriptions and announcements remain literal data. Whole-row withholding
removes the row upstream. Timer duration uses its selected formatter directly.

## 3. Numeric display

Use ASCII digits, ASCII minus for negative rounded values, and no grouping.
The registry supplies only the display radix for these numeric rules. Use the
admitted exact inputs and 010 display semantics.

| Formatter | Display |
| --- | --- |
| Duration | Nonnegative milliseconds as `H:MM:SS` or its fractional form. Hours do not wrap and minutes and seconds use two digits each. The whole-second form truncates milliseconds. The fractional form adds the locale radix and exactly three millisecond digits. |
| Fixed points | Divide exact micro-points by 1,000,000. Display the selected 0–6 places, rounding half to even and padding to the selected precision. A result rounded to zero uses zero without a sign. |
| Percent | Multiply the exact progress ratio by 100. Apply the same fixed-precision rule and append U+0025 percent sign directly. |
| Exact factor | Use the admitted original decimal spelling for display, substituting only the mantissa radix if present. Preserve all other spelling components, including sign, zeros and exponent if present. Append the optional authored multiplier marker verbatim, with any authored spacing. |
| Exact money | Use the original amount's currency code, one ASCII space and its admitted source decimal spelling with only the radix localized. Preserve source precision and all other spelling components. Never convert, round or substitute valuation or another currency. |

Display leaves the retained source lexeme, exact value and provenance intact.
Conditions, threshold comparisons, capture and ledger operations keep their
own exact inputs.

## 4. Focus examples for independent review

These logical oracles assume an admitted observation and the required exact
resources unless the case removes one. Compare catalog IDs, forms and
arguments against the single source in §2.

| Case | Input and expected observation |
| --- | --- |
| L01 · Interface isolation | A fully covered de Scene shows a known Active lifecycle with the `scene.timer.lifecycle.active` de text form and a 1251 ms fractional duration as `0:00:01,251`. Switch personal interface and numeric preferences. Both Scene outputs retain the same form and digits. Select fresh all-en content and both use the en text form and `0:00:01.251`. |
| L02 · Membership | Over seven applicable formatting sites, use `en-US` at all seven. The IDs agree and fail registry membership. No ambient `en` is selected. |
| L03 · Agreement | Use `en` at six applicable sites and `de` at a transitive slot currently without a value. Every ID passes membership and the Scene fails agreement. A revision with zero formatter references requires no locale. |
| L04 · Exact selected form | Use all-de content and remove just the de form for lifecycle Active from one intended output, with English and every other dependency present. Membership and agreement pass. Exact support fails for that output even if its generic catalog lookup falls back, so compatible publication targeting both outputs fails. Include Active in coverage even while the admitted lifecycle is Ready. |
| L05 · Known false and unavailable | A known false membership of Manual selects `scene.pause.reason.false`. With the same selector and insufficient evidence, use the authored placeholder or omit the slot. No Boolean label is selected for that observation. |
| L06 · Display and provenance | At two places, 125000 μpt displays `0.12` in en or `0,12` in de and still compares greater than 120000 μpt. Exact progress 1/3 displays `33.33%` or `33,33%`. Original `EUR 9007199254740993.250` displays that spelling in en and `EUR 9007199254740993,250` in de. Factor `1.2300E-02` and authored marker ` ×` display `1.2300E-02 ×` or `1,2300E-02 ×`. The retained spellings and exact values are unchanged. |
| L07 · Full unsigned range | For each matching count kind, 0, 1, 2, 9223372036854775808 and 18446744073709551615 choose `other`, `one`, `other`, `other`, `other`. The Qt witnesses are 2, 1, 2, 2, 2. Named substitution preserves every visible digit. The two count IDs remain distinct. An unavailable quantity selects no plural form. A gift-count slot cannot format Bits quantity. |
| L08 · Restore and version support | Publish an en revision, then a newly authored de revision. With valid head, rights and exact original resources, restore the first and observe the en selection. A client missing the original Scene catalog selection fails exact support even when it has a newer one. An unrecognized registry version with locale `en` also fails exact support. |
| L09 · Unwrapped duration | At 90061251 ms, the whole-second formatter emits `25:01:01` in either locale and the fractional formatter emits `25:01:01.251` or `25:01:01,251`. |
| L10 · Pinned forms | A later build changes a non-Scene catalog label only and both generated outputs still match every selected Scene record. The v1 selection remains eligible. Change the de Active form under the same selection in either output. That output fails exact support even when the ID and form remain present. |

## 5. Audit boundaries

- The independently reviewed synthetic selected-closure fix1 packet stays at
  its recorded 13-file checksum-list SHA-256
  `47211d553e3bb07db3a3dd92c12b70aa8fd44909297431f9babfd373845eadf4`
  with 159 protected and 23 packet structural expectations. B00 is the
  synthetic positive, S15 is unknown membership and S16 is disagreement
  within the packet's synthetic premise. The packet assigns no production
  locale. R9 follows the later owner decision and this separate review.
  [R8 and R9 record](../../schemas/scene/README.md#consumer-gates)
- R8 stays in renderer conformance. Independently assert progress track, fill
  or unavailable paint, state text and ordered slots. Assert supplied support
  row and slot order, and local text clipping/opacity before node
  clipping/opacity. RENDER-001 #447 owns the shared oracles, #449 and #451 the
  comparisons, and RENDER-004 #453 the execution gate. L01–L09 propose
  additional logical oracles under those owners.
- C008-SC-R4 remains with CONTRACT-011. Section 5 and AP06 still describe 010
  as unfinalized. Missing exact selected closure still refuses. Trusted
  production feature-to-content/output coverage, projection and rights,
  package and archive checks, asset/font inspection, renderer execution,
  platform and OBS evidence retain their respective owners.
