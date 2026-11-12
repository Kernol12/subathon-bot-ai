# APICONTRACT-005 — Bounded selected HTTP transport foundation

Kind: contract. Parent EPIC-008; specification v0.45 §2, SERVER-01, API-01,
AUTH-12 and §17–18. Status/ownership/dependencies: progress/state.json.

API-001 review B1 and APICONTRACT-004 initial review B1–B3 expose absent generic
ordinary-HTTP producer guarantees. Define the concrete selected-source foundation
BEFORE its tests or implementation. Use a NEW additive artifact built on reviewed
candidate4, preserving the exact historical API-012 artifact/patch/pins and all44
methods. New patches/pins/ABI/identity/admission require their own review; immutable
history does not prohibit a separately admitted successor. No alternate library,
parser/server/proxy or scope waiver is authorized.

Own generic per-listener pre-retention line/header/map/request-batch limits,
bounded receive/read suspension, request admission, separately reserved output,
pre-admission transient connection ownership and shutdown. Align receive/suspension
with accepted SESSIONCONTRACT-005: fixed65536 receive and at least8192 free before
read; one current raw request/connection, later bytes unparsed and bounded until
admission/release. Explicitly choose/review any deviation, head/line inclusion,
body streaming and complete per-association/global copy/metadata arithmetic.
Do not buffer a complete1MiB body merely to set a raw receive ceiling. Native
pre-Upgrade parsing must be explicitly unchanged or separately bounded and checked
against frozen native-profile semantics; no global parser regression by assumption.

Define transport-level suspension/admission APIs testable with a small real-stack
fixture independent of API-001, with finite listener/connection/callback ownership,
shared elapsed-source identity and separate accept/idle/request/output-drain
anchors. No host/generation/authority setter. Select concrete existing-source
patch seams, exact SDK compatibility/ABI/license/build inputs and literal oracles.
Import the listener contract's selected public probe-bypass/pre-listen socket
policy and scratch/log/default-route restrictions; do not create a second owner
for that choice. Any remaining source correction is explicit. No unsafe lock,
implicit filesystem/HTTP/default-body or allocation capability is inferred.

Closing review CC71487-A2/A3 asks this contract to assess role fairness/reserved
capacity rather than treating finite HTTP deadlines as a no-starvation proof.
The stock timing wheel is only a backstop to the shared elapsed source. Name the
generic fixture's second-coalesced-request-after-release case and the separate
APITEST-015 host-composition case; preserve the one-current-request rule.

SESSION-011 subsequently layers account occurrence capture/erasure/pre-log removal
on this foundation. It retains every accepted credential/body/field/lease policy,
gets an explicit dependency/rebase amendment and must not depend on API-001 for
this foundation's evidence. Do not silently expand SESSION-011 or duplicate its
account semantics. API-001 retains actual lifecycle/current-source composition.

Contract stage only: scoped contract/examples and relevant dependency/brief
amendments, no header/test/production/SDK edits. Coordinator owns shared build,
schemas, state/HTML/checkpoint. Independent Claude initial → Codex fixes/separate
verification → Claude closing, focused PR/current CI/protected merge. Approval
supplies no selected artifact, parser safety or runtime capability.
