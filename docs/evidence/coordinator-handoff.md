# Coordinator checkpoint — wire prerequisite review and milestone interfaces

Coordinator `/root`, 2026-11-04. Continue AGENTS.md and
[the coordinator prompt](../COORDINATOR_PROMPT.md). The full product is incomplete.
Continue useful authorized work while access and product questions wait.

## Integrated work and actual repository state

Main is `3217c21233bfbe1e4e2732c7c7819bdffe00ef1c`, rechecked on resume.
[PR55](https://github.com/Kernol12/subathon-bot-ai/pull/55) merged normally at
10:01:43Z from `4b008d28`, after all 12 exact-head checks and review `acaeb41`.
Merge/head trees equal `3b3dd794`; all 11 post-main workflows passed. PR53 baseline
and PR51 ACCT-004 also completed. See [generator handoff](locale-001-handoff.md),
[review](locale-001-code-review.md), [CI](locale-001-ci.md),
[historical red](localetest-001-red-review.md) and
[accounting handoff](acct-004-handoff.md). Preserve these completed gates; they do
not establish runtime UI, Windows, packaging, providers or full-release acceptance.

Root has two isolated integrator branches. This tree is
`task/CONTRACTPLAN-001-wire-prerequisites` at planning candidate `76c7b6e`. The
milestone integrator remains `task/MILESTONETEST-001-progression-baseline` in the
primary tree, concrete interface/harness at `d8b3d6a`. Open drafts PR52/PR54/PR56 remain at
`3e05b48`/`512828c`/`3fc99ce` (milestone publication to `d8b3d6a` in progress). Existing checks passed at those heads; future local
revisions do not inherit those results. Inspection preserved dirty files in eight
older trees: acct004-service, accttest004-service-api-review, storage002-review,
storage017-review, storage017-validation, storagetest016-api-author,
storagetest016-review and storagetest017-review. Missing historical registrations
remain untouched. Never reset, prune or delete them. No Git/build/CTest/publisher
child was running at inspection. The ordinary milestone API publisher is currently being polled; inspect it on resume.

## Milestone baseline and actual API review

[Draft PR56](https://github.com/Kernol12/subathon-bot-ai/pull/56) last confirmed published
`3fc99cee810d9ee5c91ad6313a01f62d5957ab05`; all 12 actual check-runs passed at that SHA.
Root owns state/HTML/checkpoint and shared schemas/build/CI/pins.

The [logical model](../contracts/milestone-progression-v1.md), blob
`0115a5cc3cdc4fb1ebb8c9739716a27cb3379933`, was authored `4362fe29`, imported
`af7e2be`, and independently approved `8338981`/import `4724e10`. See
[author](milestonetest-001-contract-author.md) and
[review](milestonetest-001-contract-review.md). Full signed64 scores/targets,
U64 distances, immutable captured prefix, ordered new completions, current-definition
projection, no repeats and absent first interval are explicit. MP01–22 and 23
independent arithmetic facts were checked.

Actual [API candidate](../contracts/milestone-progression-api-v1.md), blob
`58b8e129815c0525176cc6c1b6830a7e6f0a9fdb`, was authored `c89776c5c07252339e1cd85f8c627c3ffaee7b24`
and imported `9d8ba9e`. [API author evidence](milestonetest-001-api-author.md), blob
`9a9f3e5905565e3d51947e00abfcebf6ba5716d4`, records five literal byte sums.
One raw-input advancement operation returns owned prefix/new range/projection.

Actual API review `b8932cd`/import `21ba96d`, note blob `99a3dfc0`, APPROVES
that exact document. Sixteen independent arithmetic/field/count facts passed;
prior logical review remains unchanged. Minimal typed source author `2e36bc8` was
imported as `d481d08`: header `9a167813`, TU `4e3291f0`, note `5ddb9991`. One immediate
distinct throw is the entire feature body; neither feature nor marker was invoked.
Root harness `d8b3d6a` adds only option/target/preset24, no new workflow/test. Local
14-step build is warning-free; 33 old numeric plus three infrastructure checks pass.
Initial guessed GoogleTest source directory failed configure, then corrected to
installed GTest1.17; this is not red. See milestone interface-harness evidence on PR56.
service_reviewer now independently reviews ACTUAL source/harness in
`local-evidence/worktrees/milestonetest001-minimal-review`, branch
`task/MILESTONETEST-001-minimal-review`. Author idle. No feature tests or red released.
Dedicated selected CI, independent foundation and future test review remain required.

## Wire-contract prerequisite planning

`task/CONTRACTPLAN-001-wire-prerequisites` in
`local-evidence/worktrees/contractplan001-integrator`: claim `a9decd7`, actual plan
`76c7b6e`. [ADR-0004](../decisions/0004-wire-contract-prerequisites.md) and
[author evidence](contractplan-001-author.md) retain complete CONTRACT-003 scope.
Four planned children are common005, HTTP006, WS007 and scene008; all wait for actual
planning review. Exact 24 non-epic coarse edges are narrowed; six explicit WS edges
are added; all ten epic parent gates and every unrelated dependency are preserved.
Root authored the artifacts; service_reviewer supplied read-only recommendations.
codec_author now reviews ACTUAL plan in
`local-evidence/worktrees/contractplan001-review`, branch
`task/CONTRACTPLAN-001-independent-review`, owning only a new review note. No schemas,
source, fixture/assertion or workflow changed. 401-record graph/link/HTML checks pass;
no contract or product behavior is completed. Next: resolve actual findings, mark only
planning task done after approval, publish focused draft/CI, normal merge, then claim
common005 or newly ready internal baseline from integrated main.

## Rate baseline: locally reviewed, publication access blocked

[Draft PR54](https://github.com/Kernol12/subathon-bot-ai/pull/54), integrator tree
`local-evidence/worktrees/ratestest001-integrator`, branch
`task/RATESTEST-001-snapshot-baseline`: remote `512828c`, local `79c8512`.
Model `76d2d9b3`, API `556840c2`, header `3a06ff18`, TU `4cf1c1ed`, minimal review
`6150962` remain frozen. Corrected 53 tests `64fd60b2`, literal fixture `8936f4fa`,
Boost-only foundation `28a829aa` and author note `60d84fb2` received final actual
review `dcf5dca`/import `7d6b55f`. Mixed ASCII O/high-byte 80/ff ordering correction
preserves every previous assertion and all 53 names. No new feature test/function/
marker body has ever run. RATES-001 remains planned.

Independent foundation passed 14 facts locally after narrow review `4e9c615`/import
`7124003`; raw log SHA starts `419c41ee`. Old interface CI 41805994785 at `512828c`
compiled Boost1.83/ran old 36 checks; all 13 check-runs passed. It cannot substitute
for current CI with all 53 compiled/discovered tests and the new foundation. Current
CI publication is blocked before the first feature red.

Current gh scopes repo/read:org/gist/admin:public_key lack workflow, rechecked on
resume. SSH_AUTH_SOCK was absent and existing GPG SSH agent had no usable identity;
authenticated HTTPS Git returned 500. Ordinary API publication succeeded, but new
rate workflow CreateTree returned 404. Do not bypass workflow permission through
another path/ref route. Owner request remains pending: restore SSH for this CLion
session or complete `gh auth refresh -h github.com -s workflow`. A selected option
is not proof of restored credentials. Reinspect real auth and remote state first.

After access returns: resume in_progress, publish exact reviewed commits, observe
current Boost1.83 CI/foundation, then FIRST plain 53-case feature CTest red. Save
raw/JUnit/clean-head evidence, obtain independent failure attribution, update final
metadata, observe latest checks and merge normally. Do not replay completed reviews.
Required RATESTEST-009/RATES-009 exact valuation bridge and EPIC-017 dependencies
live in the 398-task rate branch; root has 396 tasks. Preserve them and newer locale/
milestone statuses when integrating.

## Parallel preparation and parked decisions

Agent roles above are current, not assumed from old sessions. Existing completed
reviews remain valid. Reuse idle agents with followup_task, not send_message alone.
No future product wire schema or implementation is approved by planning reviews.

CORETEST-008 [draft PR52](https://github.com/Kernol12/subathon-bot-ai/pull/52)
remains blocked at `3e05b48`; partial doc `5446a91`/review `c33a4b1` covers 16
literals only. Three owner choices remain unanswered: cap below Main (retain/reject/
audited clamp), Ready Start after deadline (reject/finish), enable/move deadline to
now/past (accept and finish/reject). Do not default, reask settled policies or release
new API/tests/red/production.

## Transport, integration and release boundaries

`/tmp/subathon-publish-task-ref.py` verifies exact blobs/trees/commits and only
nonforced task refs; never main/history/credential scopes. Resume identical objects
after transient 5xx/truncated JSON only; do not ignore 403/404. Earlier locale-only
publisher is not generic. Partial rate commit `03124ab` exists as an API object,
but no rate ref advanced. Inspect actual mutation state after ambiguous errors.

Last observed ruleset26170263: PR/resolved threads/zero GH approvals/no bypass.
Recheck before each merge; written workflow also requires ALL latest CI and actual
independent reviews. Normal exact-head merge only. All seven integrations, three
FX providers, both renderers, localization/themes/platform/license/package/live
release scope remain. No real chat/payment/provider side effects are authorized.
Local GNU16.2/Python3.14/Boost1.92 differ from selected GNU13.3/Python3.12/Boost1.83.
Verified Qt6.12 tools remain `/tmp/subathon-locale-qttools-6.12.0/prefix`; Windows
and Python3.10 runtime gates remain unperformed.
