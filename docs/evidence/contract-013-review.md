# CONTRACT-013 independent contract/package review

Author: fresh `/root/reference_contract_author` session; author commit `5819a297`,
integrated packet `cebf164`. Coordinator `/root` owns the CI addition and the
subsequent dialect correction, attributed in the author note.
Reviewer: different `/root/reference_contract_review` session, branch
`review/CONTRACT-013-schema`, worktree `/tmp/subathon-contract013-review`.
Date: 7 November 2026. Environment actually used: Python 3.14.9,
Linux 7.2.14-arch1-1 x86-64.

Reviewed the actual complete authored packet at `cebf164`, including
[reference HTTP R1](../contracts/reference-http-v1.md),
[OpenAPI document](../../schemas/http/reference-v1/api.json),
[manifest and examples](../../schemas/http/reference-v1/manifest.json),
[author evidence](contract-013-author.md) and the coordinator's five-line
[workflow addition](../../.github/workflows/schema-native-foundation.yml).
Then reviewed the exact two-file correction at
`5c67366ba76587c7e94aaf1f914db587309c3128`. The latter changes only the document
dialect string and adds correctly attributed correction evidence; assertions,
examples, bindings, limits and the HTTP contract are unchanged. Completion
metadata and later PR heads are not certified by this review.

Authority: specification v0.45 §§2/3/10/17, API-01/02/03 and NUM-04;
[task brief](../tasks/CONTRACT-013.md), root/schema instructions and workflow.
Verified unchanged import blobs: reference-path R1
`4b705cba2ab999543689b3e181fdd6a84ce1f33b`, common wire v1
`11eeb5f422a174e042143df6b476ae1d717eeacf`, session supplier
`c5f5a6e71664042e18475068d3caf577b0c8d009`, account transport
`2c33c2bbe9052455107c3056783d6bb605a5fef2`. Also read the accepted schema-tool
logical/API/context contracts and its actual document-dialect admission code.

## Scope and substantive assessment

| Requirement | Actual packet assessment |
| --- | --- |
| Bounded reference scope | Exactly public POST negotiate and protected GET timer. No login, mutation, contribution, scene, WS, provider or generic-v1 alias is registered or advertised. Full HTTP006/CONTRACT003 and release scope remain. |
| Negotiation/RP13 | Server preference, unknown offered identifiers, version409 before required-capability422, empty capabilities and implementation-gated timer advertisement are explicit. Fixed upgrade guidance is bounded and nonsecret. Public incidental credentials grant no authority and are never resolved. |
| Session/CSRF import | Exact cookie/Bearer forms, multiplicity/mutual exclusion, browser GET marker and optional exact Origin, desktop prohibition of Origin/marker, lossless ingress and log/TLS/proxy prerequisites are retained. HTTP envelope errors may precede auth; imported credential/Origin/projection/token ordering remains intact. |
| Current disclosure | Original digest and current ingress revision/session/epochs/clock/permission are rechecked on the Qt queue. Ordinary `timer.snapshot.read` binds the actual `timer` resource with no quantities or freshness requirement. Denial precedes private state/readiness lookup; no owner/page/reference shortcut appears. |
| Status/error framing | Imported431/400/401-with-challenge/403/503 distinctions remain. The read never emits a freshness challenge. CommonFailure stays closed and non-enumerating; stateless/read routes do not invent conflict/write-unknown responses, redirects, cookies or guessed Retry-After. |
| Exact closed representations | Required/null cap semantics, opaque revision, fixed active/debt0/pausedfalse literals and exact signed64 string endpoints match the imports. Failure specializations remain closed through their CommonFailure allOf. No float/coercion or scene import appears. |
| Finite resources | Outer and decoded semantic4096-byte admission, imported helper limits, JSON scan/depth/value/token/collection limits, scratch/decoded/result ownership, finite queue and complete publication limits are explicit. The116992-byte stated application payload sum is correct; separately excluded framework/native/session/connection budgets remain actual runtime obligations. |
| Examples and local references | Inspected all23 external positive examples, their actual schema associations, all94 local reference occurrences and target kinds, including unused definitions. No inline example/default inventory or remote target is concealed. H01–26 retain malformed/raw/resource/semantic cases outside the positive manifest. Schema-only endpoint snapshots do not assert ready producer state. |
| CI | The final unconditional CLI step uses the existing admitted venv and exact manifest on both Ubuntu22.04/Python3.10 and Ubuntu24.04/Python3.12. It follows the existing ordered native/static/bootstrap/facility/full-acceptance gates, introduces no dependency or failure inversion, and retains the check names and unfiltered PR coverage. Actual run success and branch enforcement require separate evidence. |

Reviewed method/path/query/body/content-type/encoding/Accept/correlation policy,
fixed cache/nosniff/challenge/Allow/upgrade headers and their first-failure ordering.
The permission operation/binding and public-route rules are bounded owning-contract
choices consistent with the imported granular-operation and session boundaries;
they do not implement the missing real producers.

## Finding and resolution

**R1 — blocking at `cebf164`; resolved at `5c67366`.** `api.json` originally set
document-level `jsonSchemaDialect` to Draft2020-12. Accepted
[schema-tool logical §4](../contracts/schema-validation-tool-v1.md#4-dialects-references-and-complete-example-inventory)
and the actual `tools/schema_validation/_schema_index.py` lines257–258 admit only
the OAS base dialect at that document position; individual Schema Objects may
select Draft2020-12. The original package would be refused at dialect admission,
before semantic/example validation. This was source/contract inspection, not a
claimed observed CLI result. Sent the finding promptly, then consolidated the
full packet findings before correction.

The coordinator changed only that string to
`https://spec.openapis.org/oas/3.1/dialect/base` and recorded attribution. Inspected
the actual delta: it satisfies the existing admission rule without broadening the
validator, changing assertions/examples, weakening a baseline or changing product
behavior. No other blocking finding remains from this review.

## Checks actually observed

At `cebf164`, `python3 tools/check_project.py` exited0:427 task records,
122 decisions, links/presets/current HTML pass. Its19 draft GoogleTests were not
run or approved. `git diff --check` exited0 there and on the corrected delta.
`git show HEAD:<import> | git hash-object --stdin` produced the four exact import
blobs recorded above. `git diff cebf164 HEAD -- schemas/http/reference-v1/api.json
docs/evidence/contract-013-author.md` plus the complete delta stat established
the narrow correction. `python3 --version` and `uname -srm` produced the environment
above.

Two bounded standard-library `python3 - <<'PY'` inspections exited0 on the original
packet. They are literal/property audits, not an alternative standards validator:

- Strict UTF-8 JSON reads with an object-pairs uniqueness assertion; closed manifest
  names/profile/declaration inspection; exact25-file/23-binding inventory, regular
  nonsymlink package entries and actual named component targets; exact two routes
  and operation methods. Original package raw bytes:43999.
- Compiled the actual primitive regexes and compared their matches with independent
  Python integer inequalities across10670 values. The finite family contained
  MIN/MAX and adjacent values, U64 maximum, every first-divergence decimal prefix
  against both signed endpoints with suffixes0/9, and10000 deterministic values
  from `random.Random(13013).randrange(-(1<<65),1<<65)`. Nonnegative expected
  membership was `0 <= n <= (1<<63)-1`; negative membership was
  `-(1<<63) <= n < 0`. Canonical spelling refusals included plus/leading-zero/-0,
  fractional/exponent/whitespace/newline/NUL, fullwidth/Arabic digits and empty.
- Checked revision/correlation odd/even/empty/maximum/maximum+1 boundaries,
  uppercase/nonhex/newline/NUL refusals, identifier64/65 and account-token64
  shape/refusals. Independently compared the complete RP01 literal; inspected
  8192-character revision,512-character correlation,64-character synthetic token,
  both16-item offer arrays and a64-character identifier. The payload sum
  `sum((4096,4096,4096,4096,16384,16384,67840))` equals116992.
- Mechanically inspected94 literal local `$ref` pointers and existing object
  targets:34 schema,37 header,17 response,6 parameter occurrences. Read their
  structural uses separately; this mechanical pointer check alone is not a typed
  standards closure proof. No inline example/examples/default, format,
  externalValue, discriminator or dynamic-reference fields were present.
- Printed all23 example values with long strings summarized by length and inspected
  the actual request/response/snapshot/failure associations and literal outcomes.

The correction affects no audited regex, assertion, literal, binding or resource
number; those completed audits were retained rather than repeated for new receipts.
The corrected package's new byte count is not claimed from the old43999-byte run.

No admitted local schema environment/native artifact survived the prior foundation
work. This reviewer did not build/load an unadmitted artifact, invoke the public
validator, run product acceptance or claim JSON Schema/OpenAPI native acceptance.

## Verdict and remaining gates

**Approved for integration of the actual contract/package and five-line CI change
at `5c67366`, with R1 resolved.** This is an independent review of written artifacts
and their requirements, not a report that the standards validator has passed.
Observed admitted hosted validation of the complete corrected package/examples and
latest-head required CI remain mandatory before CONTRACT-013 completion/merge;
draft [PR79](https://github.com/Kernol12/subathon-bot-ai/pull/79) was still pending
that evidence when this record was written.

APITEST-010 and every existing SESSION/access/API/runtime prerequisite, separate
test review, intended red, implementation/code review and real committed HTTP read
acceptance remain. Actual secure lossless/log-safe ingress, connection/framework
budgets, current producers, browser/native/platform and provider verification are
not established by schema shape or this review. No endpoint/capability is thereby
supported; all seven integrations/three rate providers and full release scope remain.
