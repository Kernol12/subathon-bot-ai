# Current test strategy

Tests establish bounded component behavior. They do not establish full v0.45 product
or release readiness. [Progress state](../progress/state.json) records task status;
[the evidence summary](evidence/README.md) records proof levels and remaining gates.
Detailed commands, reviewed revisions and historical red runs belong in PRs and the
[published archive](https://github.com/Kernol12/subathon-bot-ai/tree/evidence-archive-2026-11/docs/evidence).
This document describes current coverage and how to extend it, without a task ledger.

## Framework ownership

| Framework | Primary responsibility and current coverage |
| --- | --- |
| GoogleTest | Pure arithmetic, lifecycle/policy/progression proposals, framework-free accounting services with fake stores, permissions and selected-libsodium components |
| GoogleMock | Meaningful adapter/service interactions; dedicated coverage remains planned |
| Qt Test | Event-loop/thread ownership and disposable real SQLite stores/application composition; selected Qt build/catalog checks |
| Qt Quick Test | QML component behavior; planned |
| Python unittest | Development dashboard/consistency, independent fixture reproduction and translation-generator behavior; tooling checks do not count as server/product acceptance |
| CTest | Runs native, Qt and explicitly registered Python suites; runner success says only which selected checks passed |
| Vitest | Browser unit/component behavior; planned |
| Playwright | Browser workflows; planned, with actual OBS/compositor checks still separate |

## Passing component suites

Counts below describe the current owning suite, not unique release requirements or
coverage percentages. The [acceptance map](../tests/acceptance-map.json) tracks
specification decisions and scenarios. Related presets may include earlier suites. A passing fake-store
service is simulation evidence; disposable Qt SQL checks exercise real SQLite while
still using synthetic data and supplied policy/authority facts.

| Preset | Focused CTest selection | Current behavior/proof |
| --- | --- | --- |
| `acceptance-debug` | `-L '^acceptance$'` | 23 cap-limited credit cases in the owning suite: pure numeric planning, not full product proof |
| `exact-numeric-debug` | `-L '^exact-numeric$'` | 33 exact decimal/half-even conversion cases |
| `timer-lifecycle-debug` | `-L '^timer-lifecycle$'` | 39 Main/Grace transition proposals |
| `pause-policy-debug` | `-L '^pause-policy$'` | 37 independent pause-reason/delay proposals |
| `time-debt-debug` | `-L '^time-debt$'` | 32 debt/reversal and debt-first addition proposals |
| `timer-editorial-debug` | `-L '^timer-editorial$'` | 27 methods/44 examples for correction, cap, deadline and first-Start proposals |
| `milestone-progression-debug` | `-L '^milestone-progression$'` | 53 pure sequential milestone progression cases |
| `receipt-intake-debug` | `-L '^receipt-intake$'` | 15 receipt identity/retry cases using primitive-store simulation |
| `contribution-evidence-debug` | `-L '^contribution-evidence$'` | 21 eligibility/linkage cases using primitive-store simulation |
| `application-plan-debug` | `-L '^application-plan$'` | 22 owned captured-plan transformations |
| `effective-permissions-debug` | `-L '^effective-permissions$'` | 32 pure evaluator cases; supplied facts convey no execution authority |
| `password-hashing-debug` | `-L '^password-hashing$'` | 37 methods against selected libsodium, including secret-lifetime/failure checks |
| `session-resolution-debug` | `-L '^session-resolution$'` | 39 token preparation/current-fact resolution cases; no actual login/session issuance |
| `sqlite-writer-debug` | `-L '^sqlite-writer$'` | 25 controlled writer/ownership cases using real Qt SQL |
| `sqlite-work-debug` | `-L '^sqlite-work$'` | 24 caller-owned synchronous transaction/work cases |
| `sqlite-migrations-debug` | `-L '^sqlite-migrations$'` | 24 catalog/identity/prefix-transaction cases |
| `sqlite-retained-debug` | `-L '^sqlite-retained$'` | 16 immutable configuration/context cases |
| `sqlite-receipt-debug` | `-L '^sqlite-receipt$'` | 22 identified-receipt storage cases |
| `sqlite-contribution-debug` | `-L '^sqlite-contribution$'` | 24 contribution evidence storage cases |
| `sqlite-plan-debug` | `-L '^sqlite-plan$'` | 29 captured-plan storage cases |
| `sqlite-bundle-debug` | `-L '^sqlite-bundle$'` | 35 atomic bundle storage cases |
| `sqlite-original-claim-debug` | `-L '^sqlite-original-claim$'` | 18 typed original-claim query cases |
| `sqlite-hold-debug` | `-L '^sqlite-hold$'` | 35 methods/270 scenarios for durable application-review holds |
| `application-service-debug` | `-L '^application-service$'` | 43 methods/233 scenarios composing actual SQLite bundles/holds with supplied ports |
| `sqlite-reference-capture-debug` | `-L '^reference-capture-values$'` / `-L '^sqlite-reference-capture$'` | 28 structural codec cases and 38 real-SQL methods/67 scenarios |
| `translation-catalog-debug` | `-L '^translation-catalog$'` | 43 generator methods with literal goldens; build also produces/validates real Qt QM |
| `schema-validation-debug` | `-L '^schema-validation$'` | 8 Python methods: actual OpenAPI/17 components, 23 existing positive examples, 77 independent component cases, and 11 structural progress cases plus actual state |

Source/resource review remains necessary for allocation bounds, owned capabilities,
secret handling, process cleanup and SQL projection/ownership; output assertions alone
do not measure those properties. Security wrappers retain nonvacuous positive observations,
zero-work guards and required source/clock/login-gate ordering. Private exact-positive
native totals may be simplified only through separately approved cleanup; erasure,
locking, lifetime and fault observations remain. Standard GoogleTest skip recognition
is runner metadata; actual acceptance still requires zero skipped cases. Schema checks validate real OpenAPI/JSON-schema
packages and examples, without establishing authorization or runtime transport safety.

## Repository-schema validation

The [current contract](contracts/schema-validation-tool-v1.md) requires the actual
OpenAPI 3.1.1 document, all 17 component targets with independent valid/invalid cases,
all 23 existing manifest-bound positive examples, and the separate Draft 2020-12
progress editor schema. The `schema-validation-debug` Python suite runs real selected standards validators;
its local passing checks do not replace selected-host CI or source review. The retired generic API/process harness remains historical
evidence. No runtime authority, raw ingress or provider/platform support follows
from schema shape, and editor hints do not replace runtime progress invariants.

## Reviewed baselines still awaiting implementation

| Preset / focused selection | Current state |
| --- | --- |
| `rate-snapshot-debug` / `-L '^rate-snapshot$'` | 53 reviewed feature cases have intended stub-red evidence; rate behavior remains unimplemented. The suite has `intentional-red` labels. |
| `sqlite-account-sessions-debug` / `-L '^sqlite-account-sessions$'` | 44 reviewed feature cases have intended missing-implementation red evidence; production storage remains unimplemented. The suite has `intentional-red` labels. |

Their passing CI checks compile/discover feature registrations, enforce selected
dependency configuration and execute existing stable component/infrastructure suites. They do not turn the red feature bodies into
passing product coverage. Keep their intended-red runs separate from passing CI;
never use `WILL_FAIL`, blanket continue-on-error, disabled/skipped bodies or a failed
build as a substitute. Preserve original red evidence rather than replaying it for a
new receipt. A documented baseline revision requires a separate reviewer.

## Running relevant checks

Use the selected preset for the changed component and its dependent regression scope:

```sh
cmake --preset exact-numeric-debug
cmake --build --preset exact-numeric-debug
ctest --preset exact-numeric-debug -L '^exact-numeric$' --no-tests=error
```

Change the preset/selection using the tables above. Inspect discovery with
`ctest --preset <preset> --show-only=json-v1` when registration changes. Run the full
preset when affected composition warrants it, accounting for any explicit red baseline.
CI retains native discovery, dependency configuration and ordinary stable acceptance. Environment-only fixture/library programs were removed under the separately
reviewed cleanup. Schema checks retain isolated dependency admission and actual
artifact/example validation under their separately reviewed contract.

All native/translation acceptance presets use one Boolean,
`SUBATHON_BUILD_ACCEPTANCE_TESTS`, and an explicit `SUBATHON_ACCEPTANCE_SUITES`
semicolon list. Selector names match the preset without `-debug`, except
`acceptance-debug` selects `time-credit`. Named presets retain their existing suite
and SDK closure: SQLite presets include earlier storage stages; milestone,
permissions and rate include numeric checks; editorial includes debt and lifecycle.
Bare acceptance ON defaults to credit. An explicit empty/unknown selection fails;
`all` explicitly includes every native/translation suite, all required SDKs and both
known red baselines. Schema validation and configure-only Qt probing remain separate
controls. To combine SQL stable checks with passive account discovery:

```sh
cmake --preset sqlite-reference-capture-debug -DSUBATHON_ACCEPTANCE_SUITES="sqlite-reference-capture;sqlite-account-sessions"
cmake --build --preset sqlite-reference-capture-debug
ctest --preset sqlite-reference-capture-debug -LE '^intentional-red$' --no-tests=error
```

Retired per-feature cache switches are refused when enabled; migrate an existing
build directory with `cmake --fresh --preset <name>`. Disabled retired entries are
removed during configuration. Keep personal SDK paths in user presets as before.

`subathon_add_qt_suite` in `cmake/AcceptanceRegistration.cmake` registers one
`${name}.suite` CTest node per existing executable. Qt's unfiltered `qExec` runs every
slot/data row and its normal lifecycle; add Qt suites through this helper instead
of maintaining slot lists. Test-only fixture paths are shared through its interface
target. GoogleTest retains built-in per-case discovery and process isolation,
including sodium initialization/fault cases; `subathon_discover_gtests` applies
complete labels to that discovered list. Credit, exact-numeric and permission
regressions compile as auxiliary sources of their owning acceptance targets. Their
semantic owner labels cover every case; the former GTest regression/draft tags are
retired. Each case still runs in its own process, including the permissions allocator
fault scope. The separate Qt writer-ownership runner keeps its existing boundary and
labels. Translation uses ordinary unittest
discovery for its whole module, retaining the real Qt verifier environment. Counts
above describe methods/scenarios rather than CTest nodes. Source assertions and
fixtures remain the acceptance baseline.

Shared SQLite test support lives in `tests/qt/support/sqlite_test_support.hpp`.
Use ordinary `QVERIFY`/`QCOMPARE` in test slots. Inside writer callbacks, use the
throwing `require` helper so a failed assertion unwinds instead of returning from
only a helper and allowing a later commit. It includes the assertion's source
location. The common completion helper waits on a condition and returns the actual
writer result; suites retain their own typed error assertions and timeouts.

The 13 existing SQL warning policies are installed in each suite's `init()` as
well as setup. Qt clears warning patterns after each test function, so installing
them only in `initTestCase` does not guard later slots. Keep expected warnings
explicit and investigate unexpected output; do not add blanket suppression.

Use C++20/CMake/Ninja and the existing selected dependencies: GoogleTest, Boost >=1.83
for numeric-dependent lanes, Qt >=6.12 with its real QSQLITE plugin for SQL/Qt lanes,
and the selected libsodium SDK for password/session lanes. Keep personal prefixes in
untracked `CMakeUserPresets.json`; do not substitute older Qt to obtain a green result.
Repository-schema checks use their isolated, hash-pinned developer environment;
follow [their setup](schema-validation.md), not global installs. The current contract
replaces the generic package/native-owner surface with actual schema/example checks.
Independent test/oracle review precedes execution, and the dependency-only metadata
bootstrap precedes candidate imports. No native artifact/kernel admission is required.
Translation acceptance requires the selected Qt tools as well as Python.

Independent fixture reproduction currently supports `reference_capture`,
`account_sessions`, `bundle`, `holds`, `original_claims` and `application_service`,
with fixed script-relative
seed inputs and explicit literal recipes. Check committed bytes without writing:

```sh
python3 tools/generate_fixtures.py --check
```

Use `--output-dir <directory>` to generate selected files elsewhere; `--fixture <name>`
restricts selection. No mode writes in place by default. The [fixture contract](contracts/fixture-generation-v1.md)
defines raw seed pins, byte-exact checks and independent inputs. Reproducing a proposed
account fixture adds no account-storage capability. Claim selectors use six reviewed
names and explicit corruption-priority roles; all earlier typed values remain frozen.
Service output uses the common JSON layout while preserving its entire frozen typed
content. The existing SQL workflow runs the all-six `--check` immediately after checkout,
before dependency preparation or native use. [Fixture documentation](../tests/fixtures/README.md)
records source/output coverage, provenance, modes and selector roles.

Infrastructure checks remain distinct from component acceptance:

```sh
python3 tools/progress.py validate
python3 tools/progress.py render --check
python3 tools/check_project.py
python3 -m unittest discover -s tests/tooling -v
cmake --preset scaffold-debug
cmake --build --preset scaffold-debug
ctest --preset scaffold-debug
```

`scaffold-debug` checks infrastructure only. `qt-probe-debug` configures Qt dependency
discovery and scaffold checks only; `sqlite-writer-debug` checks actual Qt behavior. Follow [CI guidance](CI.md), inspect actual latest-head check results
and repository merge rules, and retain separate independent review before merging.

## Test design and remaining coverage

Follow [the agent workflow](AGENT_WORKFLOW.md): contract/examples → authored tests →
independent test review → observed intended red → implementation → independent code
review and relevant checks. Derive expected values independently from the specification;
compilation errors, missing dependencies and skips are not valid red evidence.

Prefer small named behavior scenarios, synthetic/redacted fixtures, fake adapters and
controllable clocks. Keep one primary home for shared mechanics. Avoid duplicate writer
plumbing and private SQL/layout/message pins unless they protect an explicit persisted
or public contract. Preserve specified accounting, rollback, idempotency, security and
recovery assertions; remove/revise a baseline only under the scoped cleanup decision
and separate review, recording the precise revised baseline in its PR.

Keep each shipped migration's canonical framing/digest freeze in its owning suite.
Every later factory still compares its complete prefix against independent frozen
versions, IDs, ordered SQL and projected schema; a later digest does not freeze
earlier SQL. Exact public formats, fixed security error messages, required service
ordering and published byte-budget formulas remain contract assertions.

Required gaps include crash/reopen and populated-database upgrades, corruption/disk-full
handling, authorization expiry/current permissions, committed-reader visibility,
end-to-end effects, exact conservation/refunds, arrival-time rules/modifiers and bounded
automation. Real current authority/clock/policy producers, full timer/recovery/action
orchestration, API/client workflows and packaging remain separate acceptance work.

All seven integrations and three exchange-rate providers remain required. Unknown
provider behavior stays unresolved. English/German, Light/Dark/System, separate native
and browser renderers, the desktop editor, and Linux/Windows/OBS output remain in scope.
Live provider, supported hardware/platform, browser/compositor, deployment/restore,
security and performance evidence must be recorded separately; component CI proves
none of those by itself. Ordinary tests must not trigger live chat/payment effects.
