# CONTRACT-005 handoff

Coordinator /root, 2026-11-04. Branch task/CONTRACT-005-wire-primitives from
main dd6a141107e4e950f3c6c3f6bd3a74b5ba1f7f09, claimc1e9859.

The [common primitive contract](../contracts/wire-primitives-v1.md) defines lossless
source numeric text, canonical derived exact integers/finite decimals/rationals,
hex opaque-byte references, finite owning profiles and version/capability/control/
nonsecret-error vocabulary. It preserves larger existing domain ranges rather than
applying core parsing/finalization ceilings globally. This is document-only common
contract acceptance; no schema, parser, encoder, client or provider behavior exists.
Concrete HTTP/WS envelopes, profiles, domain validation and scene packages remain
CONTRACT006–008 and the full CONTRACT003 integration audit. No feature gate removed.

Author codec_author commit1404de537497d437fdf7edc1ad8ff3720ff6d472/import378da1b.
Contract blob11eeb5f422a174e042143df6b476ae1d717eeacf, author note7cebb719aca46a3e124a996ba7f674958eef9bf1.
Different [review](contract-005-review.md) commitf01bd28fa320d607b00e50621187ad77f0191b13
approves those exact artifacts:323 lines/27WP examples,23 independently checked
arithmetic/size/order facts and existing storage/application profile compatibility.
No blocker or owner decision. No production source/assertion/dependency/workflow change.

Actual `python3 tools/check_project.py`, `python3 tools/progress.py render --check`
and `git diff --check` pass with401 tasks/122 decisions. Existing scaffold CI covers
document/graph/link consistency; all unchanged component checks still required on
final PR head. No new executable suite is warranted and no product/native/provider/
renderer/platform tests ran for this document. No fake acceptance or red run claimed.

Local task acceptance complete; publication/CI/rules/threads/normal merge remain.
After actual merge record it on the next task branch, refresh main and claim a
ready consumer/scene contract with explicit shared schema ownership. Do not treat
a planned domain/profile/schema as implemented capability. Preserve active password
API work, both reviewed baseline access blockers and required exact valuation bridge.
All original release/provider/platform requirements remain.
