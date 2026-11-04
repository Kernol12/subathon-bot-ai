# Architecture and dependency boundaries

The specification is authoritative. This map describes intended modules, not built features.

| Path | Owns | May depend on / may not do |
| --- | --- | --- |
| core | Exact domain values, calculations, event/effect contracts, clock abstractions | C++ standard library and selected exact arithmetic; no UI, network or provider SDK |
| server | Qt application services, authoritative queue, persistence, permissions, API bridge | Core and explicit adapters; all durable state follows one writer path |
| integrations | Provider-specific auth, transport, validation and capability descriptors | Intake contracts; cannot write timer/database state directly |
| desktop | Qt Quick/QML management/editor/native renderer | Versioned server API and shared schemas; no service credentials/authoritative state |
| web | React management/viewer and independent TypeScript overlay | Versioned server API/schemas; no direct DB access or full scene editor |
| schemas | OpenAPI 3.1, WS/scene/normalized-event contracts | Shared ownership; exact numeric JSON strings and compatibility policy |
| tests | Acceptance oracles, fakes, fixtures, reference scenes, platform/live evidence | Public interfaces; explicit suite ownership |
| deploy | Docker Compose, Podman Quadlet, native/systemd/Caddy and packaging | Agreed x86-64 support; no project-operated authorization service |

## Thread and transaction boundary

Drogon handles public transport and bridges commands to the authoritative Qt queue.
The application validates permissions/revisions and commits receipts, effects/state
and delivery jobs through the controlled Qt SQL/SQLite writer. External delivery happens
through persisted jobs; uncertain outcomes are not blindly retried. Clients render
server state and never own the authoritative timer or accounting.

## Contracts before dependent implementation

The first small C++ time-credit boundary is revision 1 in docs/contracts/time-credit-v0.md;
its reviewed original red baseline is recorded in
[evidence](evidence/time-credit-baseline-handoff.md). The bounded numeric helper is
implemented; [IMPL-001 evidence](evidence/impl-001-handoff.md) records passing
acceptance/regression checks and separate code review. It does not apply contributions
or commit effects.
CORE-001 also implements [exact decimal/final conversion revision1](contracts/exact-decimal-v1.md)
as an explicit core target;32 reviewed cases plus one regression pass with separate
code review and observed numeric CI. It supplies no application transaction,
aggregate policy or provider interpretation; see [handoff](evidence/core-001-handoff.md).
The internal [record, clock and writer boundary contract](contracts/event-state-boundaries-v1.md)
(revision 1, CONTRACT-002) separates durable intake from atomic application and
post-commit delivery. It defines shared invariants and specification-derived examples;
concrete C++/SQL layouts, bounded domain baselines and behavior remain downstream work.
No OpenAPI, normalized-event wire schema, effect ledger schema or complete timer
schema is provided as final. CONTRACT-003 defines transport contracts before clients;
implementation still requires each slice's reviewed tests and observed intended red.
Keep UI, provider payloads, normalized events, contributions, plans, effects and delivery
jobs distinct. Request/actual credit, arrival context, originals and reversals must retain
provenance. Two scene renderers share data/behavior contracts, not a renderer implementation.

The [identified receipt service](contracts/receipt-intake-v1.md) implements one
repository-port transaction with immutable originals and traceable retry/conflict
attempts. Fifteen reviewed cases pass in a primitive-store simulation with separate
code review/automatic CI; [handoff](evidence/acct-001-handoff.md). This is no SQL/Qt
writer, actual-provider or contribution-application verification.

## Early feasibility work

Check Drogon/Qt lifecycle/queue/shutdown, Socket.IO protocol compatibility, Twitch
independent authorization coverage and Linux/Windows OBS/media behavior early. Isolated
spikes are allowed before production implementation; document results and unresolved
capabilities, and discard or explicitly review spike code before using it as production.


SPIKE-001 now has [bounded Linux loopback evidence](evidence/spike-001-bridge-validation.md)
for pinned Drogon/Trantor, real HTTP/WebSocket queue/writer operations and cooperative
shutdown. This approves no production reuse. The observed WebSocket close behavior
and remaining lifecycle cases are carried into APITEST-001/API-001 and
APITEST-003/API-003; persistent connections and platform/release support remain open.

The [Main/Grace reducer](contracts/timer-lifecycle-v1.md) implements pure countdown
proposals and ordered transitions with39 reviewed passing cases, separate code
review and automatic CI; [handoff](evidence/core-002-handoff.md). UTC and elapsed
inputs remain separate, and accepted actual credit can resume confirmation wait.
The caller still owns segmentation, pause reasons, permissions, actual-credit
calculation and atomic commit; no Qt timer/queue, durable state or recovery is supplied.

The [eligibility/linkage slice](contracts/contribution-eligibility-v1.md) implements
independent planning holds and committed-receipt association through a transaction
port. Its21 cases pass with separate code review and automatic simulation CI;
[handoff](evidence/acct-002-handoff.md). Original context/conditions remain anchored
to support while explicit confirmation only promotes evidence. This supplies no
provider proof, Qt/SQL adapter, application transaction, job or refund reconciliation.

The [pause policy](contracts/pause-policy-v1.md) implements independent reasons and
normalized monotonic signal delays with37 passing reviewed cases, separate code
review and automatic CI; [handoff](evidence/core-003-handoff.md). It returns a final
proposal, not within-interval countdown history. Callers must segment at signal/delay
boundaries and commit under the authoritative writer; no actual provider/state/timer
integration or delay-reconfiguration behavior is supplied by this helper.

The [captured-plan helper](contracts/application-plans-v1.md) implements owned
arrival-time request/provenance histories, audited override proposals and preview
freshness.22 preserved cases pass with separate code review and pinned automatic CI;
[handoff](evidence/acct-003-handoff.md). Sources supply already-resolved selections
and requests; the helper never evaluates rules/modifiers, validates real permission,
applies current caps/effects, or persists state. Those remain API/accounting/writer gates.

The [captured-plan SQLite store](contracts/sqlite-application-plans-v1.md)
implements complete owned captures and immutable history on caller-owned writer
sessions, exact rational/uint64 storage and distinct complete/selected read scopes.
Its29 reviewed cases pass with separate source/config review and automatic selected
Qt acceptance; [handoff](evidence/storage-015-handoff.md). Caller commit/cleanup
remain actual; semantic authority/evaluation, effects/jobs/full app transactions,
provider valuation and operational/platform verification remain separate gates.

The [application transaction bundle contract](contracts/application-transaction-bundle-v1.md)
defines logical row/port obligations and21 reviewed examples, not a concrete API or
repository implementation; [handoff](evidence/storagecontract-001-handoff.md).
One caller owns commit/cleanup; active-session reads/duplicates are provisional,
and known B08 rollback differs from unknown or later publication failure. Concrete
state/job/wire/semantic gates and the real STORAGETEST-002 baseline remain required.

The [SQLite writer boundary](contracts/sqlite-writer-v1.md) implements owned queued
transactions/private connections on the authoritative Qt command/state thread and
separate actual read-only readers.22 unchanged reviewed cases plus3 independent
ownership regressions pass with separate code review and automatic UbuntuQt6.12 acceptance;
[implementation handoff](evidence/storage-001-handoff.md). Original stub-red remains
in its baseline evidence. This supplies no running server application or repository
adapter. Application schemas/migrations, durable app effects/aggregate/job atomicity,
recovery and backup remain later storage/accounting gates. SQL batch/payload limits
are internal engineering choices, not product contribution limits or a hard
arbitrary-SQL runtime deadline.

The [time-debt numeric helper](contracts/time-debt-v1.md) implements pure signed
reversal policies and debt-first additions with32 unchanged reviewed passing cases,
separate code review and actual automatic Ubuntu acceptance;
[implementation](evidence/core-005-handoff.md). It preserves full requests and reports
Main/debt/unapplied/cap effects separately, with checked signedMIN/overflow handling.
This new API leaves the older credit helper's narrower precondition unchanged.
Existing CORE-002 owns lifecycle/Grace/end-mode composition; the original32stub-red
remains retained. This grants no contribution status/permission, proportional refund
entitlement, ledger allocation, persisted review/audit or authoritative transaction.

[Writer transaction work revision1](contracts/writer-transactions-v1.md) defines the
additive internal bridge needed by synchronous repository service ports: real
commit before service success, revocable work/session capabilities, explicit
transaction/work outcomes and cumulative SQL budgets on the same private writer.
[Independent contract evidence](evidence/contract-004-handoff.md) and
[storage decomposition](decisions/0002-storage-executable-slices.md) preserve every
original storage requirement through real baseline-gated slices. STORAGE-010 now
implements the additive mechanism with24 unchanged separately reviewed real Qt SQL
cases, original writer25 passing, independent code/config approval and actual
ordinary Ubuntu acceptance. [Implementation handoff](evidence/storage-010-handoff.md)
records exact artifacts and retained reviewed/red baseline. Existing wire/provider/
application schema boundaries remain unchanged: no domain repository adapter,
migration, application bundle, recovery authorization or running server is supplied.

[Migration catalog revision1](contracts/migration-catalog-v1.md) now defines strict
internal identity/history/content/schema/UTF8, whole-prefix transactions and
explicit-update caller boundaries. STORAGETEST-011 supplies22 independently reviewed
Qt SQL cases/ten literal vectors, selected officialQt green7 diagnostic support
and actual intended stub-red; [baseline](evidence/storagetest-011-handoff.md).
STORAGE-011 now implements this internal codec/catalog/runner with24 passing
migration cases (original22 plus two independently reviewed Unicode regressions),
unchanged writer49, separate source/config approval and actual automatic selected
Ubuntu Qt6.12 acceptance. [Implementation](evidence/storage-011-handoff.md) records
exact revisions and the review-found initial-U+FEFF defect, reviewed regression
red and correction. Existing private writer API/allowlist are unchanged. Explicit
advancement commits the whole missing prefix in one transaction; opening a writer
does not initiate it. Metadata DDL supplies no domain store, application bundle,
backup/update authority or recovery mechanism. Caller OPS-02 obligations remain.

[Retained configuration/context revision1](contracts/retained-context-v1.md) fixes
immutable opaque byte identities/documents and optional typed configuration links
composed on a caller-owned existing session. STORAGE-012 now implements its pure
manifest and synchronous find/retain primitives, with16 unchanged reviewed cases,
independent code/config approval, author/root/reviewer full95 pass and actual
automatic selected Ubuntu Qt6.12 acceptance. [Implementation](evidence/storage-012-handoff.md)
retains original reviewed/red evidence and exact source/interface/test revisions.
Root migration1 retain-config-context-v1 has only two retained tables/two generated
indexes plus catalog objects. The caller still owns transaction/commit/publication,
startup inspection and actual update/backup/maintenance obligations. No current-
settings pointer, core port change, semantic config/evaluator or wire layout is
supplied. Typed uint64 domain codecs remain explicit013/014/015 gates; opaque
byte retention does not certify those, domain application bundle or recovery.


The [identified-receipt SQL contract](contracts/sqlite-receipts-v1.md) and
[additive schema](contracts/sqlite-receipts-schema-v1.md) now have an implemented
STORAGE-013 adapter/uint64 codec/manifest, with22 unchanged reviewed real-SQL
cases/full118 passing, separate source/config approval and actual automatic
selected UbuntuQt6.12/SQLite3.53.4 receipt22 acceptance. [Handoff](evidence/storage-013-handoff.md)
retains the original reviewed/compiled/red baseline chronology and exact artifacts.
The unchanged service owns classification; one existing session owns real atomic
commit, immutable originals/every attempt/retained targets and truthful uncertainty.
No new writer/core/provider/wire API is added. This supplies no running server,
contribution/plan/effect/job/application bundle or operational/platform release
capability; downstream typed codecs/rates and all full-release scope remain open.

The [contribution SQL adapter](contracts/sqlite-contributions-v1.md) now implements
the unchanged primitive repository ports with full CId0..MAX/NULL distinction,
immutable original-receipt/observation provenance and independent current state.
24 preserved real-SQL cases/full143 pass with separate source/config review and
actual automatic selected Qt6.12/SQLite3.53.4 acceptance. [Handoff](evidence/storage-014-handoff.md)
records source615, exact additive migration3 and prior reviewed/red baseline.
Existing service owns policy. One controlled session owns real commit/RAII rollback
and uncertainty; receipt prior commitment and semantic/provider authority remain
upstream preconditions. No app bundle/effect/job, plan/rate history, permission,
recovery/operational or platform/provider release capability follows.


The [application-bundle SQLite store](contracts/sqlite-application-bundle-v1.md)
now implements the caller-session atomic storage boundary: immutable three-state
snapshots, actual effect components, contribution application-only patch, identity
claims, queued job/result/audit envelopes and per-basis reversal history. Full and
selected-source readers have distinct closure/materialization scopes; metadata
uses checked64/65 pages without lifetime quotas. All returns remain provisional
until real caller COMMIT/normal completion. [Handoff](evidence/storage-002-handoff.md)
records35/full209 local component passes and separate source/config approval;
promoted official CI/publication gates are recorded separately. This does not
implement the authoritative application orchestrator or interpret permission,
clock/cap policies, financial refund allocation, delivery or recovery documents.

The [durable application-review hold store](contracts/sqlite-application-holds-v1.md)
implements immutable causes, detection/resolution history, stable tagged-subject
active gating, complete-set resolution and finite replacement results on that same
caller session. The exact additive migration6 preserves prefix1–5 and supplies
full89 catalog objects. Strict reference readers reuse private bundle decoding;
connectors/clients acquire no authoritative writer. All results remain provisional
until actual caller commit and normal work completion. [Handoff](evidence/storage-016-handoff.md)
records270 reviewed scenarios/full264 native acceptance and selected official CI,
with final review/publication gates recorded separately. Storage does not decide
authority, numeric policy, mixed timer outcomes or review resolution permission;
the application service and actual policy/clock/action adapters remain required.


The [original application service](contracts/application-service-api-v1.md) now
composes those nonowning stores through one actual queued writer unit. It checks
current access before protected retry/claim/hold lookup, resolves complete exact
proposals and records either one atomic bundle or a durable whole-input hold.
The completion bridge publishes only the actual committed normal typed work value;
unknown outcomes and post-commit body exceptions never synthesize success.
[Implementation](evidence/acct-004-handoff.md) retains43 reviewed native/real-SQL
cases, separate source/allocation review and actual promoted selected-Qt CI. The
trusted finite read-only ports still need real authority/policy/clock/action/reference
producers. Financial refunds, explicit review/continuation, running API/server,
recovery, delivery and release/platform/provider scope remain separate gates.


The shared [translation catalog contract](contracts/translation-catalog-v1.md)
and [build API](contracts/translation-catalog-api-v1.md) now have an implemented
standard-library Python generator. One authoritative English/German JSON source
produces canonical Qt TS/browser text and real selected Qt6.12.0 QM. Native
consumers require generated declaration metadata and raw QTranslator lookup before
their later named formatter. The build verifies tracked text without overwriting
it; an explicit update target regenerates that text. Source files are not cleanable
build byproducts. [LOCALE-001 evidence](evidence/locale-001-handoff.md) records43
preserved passing cases, independent source/process/ownership audit and actual
promoted selected CI. Runtime formatting, preferences, live switching, renderers,
packaging and supported-platform release checks remain separate requirements.
