# ACCESSTEST-002 independent partial logical review

Reviewer `/root/codec_author`, 2026-11-04, separate from author `service_author`.
Actual clean starting revision: `d7040673828fa07d3a761574a2d8d060646fdeab` on
`task/ACCESSTEST-002-contract-review`. Reviewed author `6bc3d51f`, imported as
`c88bcf5`; no contract or shared file was edited by this reviewer.

**APPROVED as an explicitly PARTIAL document checkpoint for its settled logical
requirements and examples.** No additional requirement conflict or correction was
found. This is not approval of a complete permission algorithm or PR merge.
**D1 remains unanswered and blocks complete logical approval and every dependent
API/test/production stage.** No alternative is selected, including the recommended
option in the existing question. An actual answer and actual revised review are
still required.

## Actual artifacts and authority

| Artifact | Reviewed Git blob |
| --- | --- |
| [Partial logical contract](../contracts/effective-permissions-v1.md) | `5dfc6a1e6cff3877a4d2874a26d29d0e6f7f44ed` |
| [Author checkpoint](accesstest-002-contract-author.md) | `66edd21fa135309c1cafc113eec6c9c9e5b3538c` |
| [Specification v0.45](../specification.md), §10/AUTH-03/06/07 | `3e008efaef3181dd8b06a038f6309347ec31619d` |
| [ACCESS-002 brief](../tasks/EPIC-006/ACCESS-002.md) | `af0eb841abe59e6389acda66ff5aba0de372ea22` |
| [Common value contract](../contracts/wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| [Event/state boundaries](../contracts/event-state-boundaries-v1.md) | `8b60da43f1362c46c54730e8ca029b858964ea59` |

Read root/core/server instructions, workflow/test-first boundaries, the adjacent
ACCESS-003/004 briefs and the [existing D1 question](accesstest-002-open-decisions.md).
The repository specification supplies product authority; the author's external
security citation does not decide this product's precedence or open policy.

## Findings

The unambiguous individual decision takes precedence; otherwise applicable role
deny defeats role allow, then default deny. EP06–09 explicitly avoid a mismatching
constrained individual override. §4, D1-A and D1-B correctly retain all three
different applicability alternatives and leave constrained-individual deny/shape
consequences open. They are not executable or frozen oracles. No elapsed time,
preview, profile or conservative-sounding label substitutes for the owner's answer.

Whole-grant matching preserves AUTH-06. EP01–05/10/11 require one entire resource/
quantity tuple, including when alternative grants share a role or source; neither
cross-role nor within-source fragments manufacture authority. Requests retain their
requested quantities despite smaller actual cap-limited effects. Zero still needs
authorization. The exact fraction, signed-minimum magnitude, 2^64 and long-component
examples EP12–14/27 preserve the owning domains without rounding, signed narrowing
or a borrowed parser/finalizer cap. Arbitrary-precision capacity does not widen an
operation's declared quantity domain.

EP15–19 keep current ownership separate from role labels and editable permissions.
Only the narrow protected access-management/recovery source survives editable denial;
ordinary owner operations still use ordinary grants. Trusted operation classification
is a required catalog integration seam, not a caller's owner/protected flag. The
owner-target helper-reset refusal cannot create a credential or bypass dedicated
recovery, fresh authentication, recipient acceptance or ownership transfer. No
unrestricted owner/administrator authority or authentication success is inferred.

EP20–21 and §6 require authoritative current session/owner/policy/revision checks at
execution, including queued/review work. Captured facts/results remain history;
they do not authorize a later command. Explanations retain complete fitting witnesses,
losing sources and decisive denials in reproducible order rather than selecting
constraint fragments. EP22–26 preserve opaque unsigned-byte identities, NUL versus
empty, unknown versus known-empty facts, invalid complete shapes and resource failure
instead of successful truncation. Disclosure still needs its own current rights.

The configurable 4096-reference-byte/16384-component-byte/4096-entry/16MiB-input/
32MiB-result proposals are finite local evaluation profiles, not global domain caps
or permission fallbacks. All supplied and losing sources count. Complete output is
projected before ownership; oversize explanations cannot leave a successful permit.
The conservative 131072-bit scratch bound follows from two 16384-decimal-byte
components, not a new final-value limit. Exact B/I formulas, shape/duplicate/error
order and before-allocation/arithmetic checks remain required concrete-API work.
These declared engineering details are not an additional owner policy question.

Use-versus-grant authority, self-affecting edits, invitations and automation remain
ACCESS-003. Actual account/session/owner loading, protected operation catalog and
recovery/transfer remain their owning tasks. Chat categories and optional Twitch
identity are not application roles or authentication proof. No current provider,
transport, session adapter or enforcement capability is claimed.

## Actual checks and remaining gates

Read-only Git status/log/diff/hash-object and full document/source-authority reads
confirmed the exact artifacts above. Inline Python stdlib Fraction/integer/byte
checks passed 15 independent literal facts, all 27 unique EP rows, both pending D1
rows, local links and balanced fences. Facts include 600001/2 > 300000,
abs(INT64_MIN) = 9223372036854775808, ASCII 4f before byte 80, 4096/4097-byte
references, and 10^5000 requiring 5001 decimal bytes/16610 magnitude bits. A maximum
positive 16384-digit component squared needs 108853 magnitude bits, within the
proposed conservative scratch bound. These are arithmetic/document checks, not an
authorization evaluator or SUT-derived oracle.

`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` passed: 406 tasks, 122 decisions, local links/presets/current
HTML and whitespace. Staged inventory/whitespace were checked before committing.
No schema/API/stub, fixture/test, auth/SDK/native/runtime, compiler or feature
execution was authored or run. Only this review note changes. Root owns PR/state/
HTML/build/CI/publication and all subsequent releases. D1, revised logical review,
separate API/source/test reviews, selected environment, FIRST intended red and
implementation/code/integration reviews remain pending; existing CI cannot replace
them. This review neither completes ACCESSTEST-002 nor makes ACCESS-002 ready.
