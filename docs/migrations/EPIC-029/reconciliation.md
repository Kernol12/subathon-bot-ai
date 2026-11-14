# EPIC-029 one-time task reconciliation

Observed 2026-11-14 against GitHub native issues and relationships. The source is
`epic-029-old-tracker-2026-11-14` at `355ffdfe`; this report is an immutable
migration observation, not a planning authority or synchronization tool.

503 tracker IDs map one-to-one to 503 GitHub issues. No missing or duplicate
IDs, parent mismatches or prerequisite mismatches were observed. Native containment
is distinct from blocked-by prerequisites. Readable GitHub titles are retained;
496 titles differ from the old tracker names, which remain in the tagged tracker
and the existing original-name issue history. Titles below record both observations.

Historical proof: **historical, as recorded, not re-verified**. This report does
not repeat the stopped A/B/reliance/seal/privacy audit. Private artifacts stay
private. Existing proposal manifest SHA-256 references only:

- A: `41522ad50f10d7ae26e1216b591ca02ee48cedecd2472271c44f219703e0761c`. Note: the archived manifest itself is unreviewed.
- B: `a574c4e673d975d16169aa7db8543a311966387d3477c5bc2ff818759ae8cf36`.
- Reliance: `094e1b37447c3e29625121a7e8d9a1c6f2a08c973c50e9eb23cc1797cf066748`.
- Privacy: `f0589f81fbaee1fd39b343078e7de171b265759518eda47b9b8c7669398250ec`.

| Task ID | Issue | Recorded old name | GitHub readable title | Native parent | Native prerequisites |
| --- | --- | --- | --- | --- | --- |
| EPIC-028 | #127 | Simplify agent workflow and focus test coverage | Simplify workflow, evidence and test maintenance | — | — |
| EVIDENCE-001 | #128 | Archive detailed receipts and publish a readable summary | Archive detailed evidence and publish readable summaries | #127 | — |
| EVIDENCE-002 | #129 | Enforce concise task notes and current suite summaries | Keep task notes and test-suite summaries concise | #127 | #128 |
| TESTCLEAN-001 | #130 | Replace schema harness tests with real schema checks | Validate actual schemas instead of schema test harnesses | #127 | — |
| TESTCLEAN-002 | #131 | Remove environment foundation programs | Remove obsolete environment probe programs | #127 | — |
| TESTCLEAN-003 | #132 | Simplify test registration and labels | Simplify test registration and suite labels | #127 | #131 |
| TESTCLEAN-004 | #133 | Share Qt test support and deduplicate writer plumbing | Share Qt test helpers and SQLite writer setup | #127 | #132 |
| TESTCLEAN-005 | #134 | Remove repeated implementation-detail assertions | Remove redundant assertions about implementation details | #127 | #133 |
| FIXTURE-001 | #135 | Independent catalog, reference and account fixture recipes | Add independent catalog, reference and account fixture recipes | #127 | #134 |
| FIXTURE-002 | #136 | Storage fixture recipes and named selector roles | Add reproducible storage fixtures and named selectors | #127 | #135 |
| FIXTURE-003 | #137 | Independent application-service fixture recipes | Add independent application-service fixture recipes | #127 | #136 |
| TESTCLEAN-006 | #138 | Replace giant unreproducible fixtures | Replace oversized fixtures with reproducible recipes | #127 | #134, #135, #136, #137 |
| TESTCLEAN-007 | #139 | Trim libsodium call-counting tests | Focus password tests on behavior instead of library call counts | #127 | #132 |
| TESTCLEAN-008 | #140 | Merge regression executables and simplify test labels | Consolidate regression executables and test labels | #127 | #133, #139 |
| BOOT-001 | #141 | Create the packaged agent foundation | Set up the initial agent-ready project scaffold | — | — |
| BOOT-002 | #142 | Validate CLion and the local CMake toolchain | Verify CLion and the local CMake toolchain | — | #141 |
| BOOT-004 | #143 | Establish task branches and gated PR merging | Establish task branches and protected PR merging | — | #141 |
| BOOT-005 | #144 | Decompose all 27 release epics | Break the release roadmap into executable tasks | — | #143 |
| BOOT-006 | #145 | Show execution order and dependency readiness | Display task dependencies and execution order | — | #144 |
| CONTRACT-001 | #146 | Review the draft time-credit boundary | Review the initial time-credit contract | — | #141 |
| TEST-001 | #147 | Review and expand the draft credit tests | Expand the initial time-credit acceptance tests | — | #146 |
| REVIEW-001 | #148 | Independently review the acceptance baseline | Independently review the initial acceptance tests | — | #147 |
| TEST-002 | #149 | Demonstrate the intended failing baseline | Establish the intended failing time-credit baseline | — | #142, #148 |
| IMPL-001 | #150 | Implement reviewed cap-limited credit | Implement time credits limited by the timer cap | — | #149 |
| BOOT-003 | #151 | Validate the Qt 6.12 environment probe | Verify Qt 6.12 dependency discovery | — | #142 |
| CONTRACT-002 | #152 | Define event, effect, clock and persistence boundaries | Define event, effect, clock and persistence interfaces | — | #146 |
| CONTRACT-003 | #153 | Define HTTP, WebSocket and scene contracts | Define the initial HTTP, WebSocket and scene interfaces | — | #152, #157, #158, #159, #160, #555 |
| CONTRACT-004 | #154 | Define writer-owned synchronous repository transaction integration | Define repository transactions owned by the database writer | — | #152, #188, #190, #297 |
| CONTRACTPLAN-001 | #155 | Split the product wire-contract prerequisites | Split wire-protocol prerequisites into bounded contracts | — | #152 |
| CONTRACTPLAN-002 | #156 | Split declarative scene contract prerequisites | Split scene-format prerequisites into bounded contracts | — | #155, #157 |
| CONTRACT-005 | #157 | Common wire primitives and compatibility | Define shared wire values and compatibility rules | — | #152, #155 |
| CONTRACT-006 | #158 | HTTP/OpenAPI 3.1 contract | Define the HTTP API and OpenAPI schema | — | #155, #157, #160, #555 |
| CONTRACT-007 | #159 | Persistent WebSocket synchronization contract | Define persistent WebSocket synchronization | — | #155, #157, #158, #165 |
| CONTRACT-008 | #160 | Declarative scene/schema contract | Define the declarative scene format and schema | — | #155, #156, #157, #161, #162, #163, #164 |
| CONTRACT-009 | #161 | Protocol-neutral scene document model | Define the protocol-independent scene document model | — | #156, #157 |
| CONTRACT-010 | #162 | Typed scene bindings conditions and presentation timelines | Define typed scene bindings, conditions and timelines | — | #156, #161, #173, #175, #179, #190, #214, #216, #227, #233, #246 |
| CONTRACT-011 | #163 | Scene assets fonts and portable package contracts | Define scene assets, fonts and portable packages | — | #156, #161 |
| CONTRACT-012 | #164 | Complete versioned scene schema package | Complete the versioned scene schema package | — | #156, #161, #162, #163 |
| SPIKE-001 | #165 | Validate Drogon and Qt lifecycle/thread bridge | Check feasibility of the Qt/Drogon lifecycle and thread bridge | — | #141 |
| SPIKE-002 | #166 | Validate independent Twitch authorization | Check independent Twitch authorization feasibility | — | #141 |
| SPIKE-003 | #167 | Validate Socket.IO protocol compatibility | Check Socket.IO compatibility for payment integrations | — | #141 |
| SPIKE-004 | #168 | Validate Qt/OBS capture and media on reference platforms | Check Qt media and OBS capture on target platforms | — | #151 |
| EPIC-001 | #169 | Core test plan: exact arithmetic and full timer lifecycle | Timer lifecycle and exact calculations | — | #150, #152 |
| CORETEST-001 | #170 | Review and demonstrate red: Bounded exact decimal parsing and final-effect rounding | Test baseline: exact decimal parsing and final-effect rounding | #169 | #152 |
| CORE-001 | #171 | Bounded exact decimal parsing and final-effect rounding | Implement exact decimal parsing and final-effect rounding | #169 | #152, #170 |
| CORETEST-002 | #172 | Review and demonstrate red: Main countdown and ending-mode transitions | Test baseline: countdown and ending-mode transitions | #169 | #150, #152 |
| CORE-002 | #173 | Main countdown and ending-mode transitions | Implement countdown and ending-mode transitions | #169 | #150, #152, #172 |
| CORETEST-003 | #174 | Review and demonstrate red: Independent pause reasons and delayed status signals | Test baseline: independent pause reasons and delayed status signals | #169 | #173 |
| CORE-003 | #175 | Independent pause reasons and delayed status signals | Handle independent pause reasons and delayed status signals | #169 | #173, #174 |
| CORETEST-004 | #176 | Review and demonstrate red: Recurring schedules and timezone occurrence previews | Test baseline: recurring schedules and timezone-aware previews | #169 | #175 |
| CORE-004 | #177 | Recurring schedules and timezone occurrence previews | Add recurring schedules and timezone-aware previews | #169 | #175, #176 |
| CORETEST-005 | #178 | Review and demonstrate red: Refund deficit policies and separate time debt | Test baseline: refund deficits and separate time debt | #169 | #171, #173 |
| CORE-005 | #179 | Refund deficit policies and separate time debt | Track refund deficits and separate time debt | #169 | #171, #173, #178 |
| CORETEST-006 | #180 | Review and demonstrate red: Downtime reconstruction and recovery-review decisions | Test baseline: downtime reconstruction and recovery decisions | #169 | #175, #177 |
| CORE-006 | #181 | Downtime reconstruction and recovery-review decisions | Reconstruct downtime and support recovery decisions | #169 | #175, #177, #180 |
| CORETEST-007 | #182 | Review and demonstrate red: Finished-input and audited reopening policies | Test baseline: finished-timer input and audited reopening policies | #169 | #179, #181 |
| CORE-007 | #183 | Finished-input and audited reopening policies | Enforce finished-timer input and audited reopening policies | #169 | #179, #181, #182 |
| CORETEST-008 | #184 | Review and demonstrate red: Manual adjustments cap edits and hard-deadline commands | Test baseline: audited timer adjustments, caps and hard deadlines | #169 | #179 |
| CORE-008 | #185 | Manual adjustments cap edits and hard-deadline commands | Add audited timer adjustments, caps and hard deadlines | #169 | #179, #184 |
| EPIC-002 | #186 | Contribution ledger, review, partial/full reversals and deduplication | Contribution accounting, review and refunds | — | #152, #193 |
| ACCTTEST-001 | #187 | Review and demonstrate red: Normalized receipt identity and duplicate-safe ingestion | Test baseline: normalized receipts without duplicates | #186 | #152, #171 |
| ACCT-001 | #188 | Normalized receipt identity and duplicate-safe ingestion | Ingest normalized receipts without duplicates | #186 | #152, #171, #187 |
| ACCTTEST-002 | #189 | Review and demonstrate red: Independent eligibility and confirmation linkage | Test baseline: contribution eligibility and payment confirmation | #186 | #188 |
| ACCT-002 | #190 | Independent eligibility and confirmation linkage | Link contribution eligibility and payment confirmation | #186 | #188, #189 |
| ACCTTEST-003 | #191 | Review and demonstrate red: Captured application plans and calculation provenance | Test baseline: contribution plans and calculation provenance | #186 | #171, #190 |
| ACCT-003 | #192 | Captured application plans and calculation provenance | Capture contribution plans and calculation provenance | #186 | #171, #190, #191 |
| ACCTCONTRACT-001 | #193 | Define application-service and durable-hold logical contract | Define application-service operations and durable review holds | — | #179, #192, #312 |
| ACCTTEST-004 | #194 | Review and demonstrate red: Atomic effect application and operation idempotency | Test baseline: atomic effects and idempotent operations | #186 | #179, #192, #193, #312, #314, #316 |
| ACCT-004 | #195 | Atomic effect application and operation idempotency | Apply effects atomically with idempotent operations | #186 | #179, #192, #194, #312 |
| ACCTTEST-005 | #196 | Review and demonstrate red: Individual batch and silent contribution review | Test baseline: individual, batch and silent contribution review | #186 | #195 |
| ACCT-005 | #197 | Individual batch and silent contribution review | Review contributions individually, in batches or silently | #186 | #195, #196 |
| ACCTTEST-006 | #198 | Review and demonstrate red: Cumulative partial and full reversal entitlements | Test baseline: cumulative partial and full refund entitlements | #186 | #171, #195 |
| ACCT-006 | #199 | Cumulative partial and full reversal entitlements | Calculate cumulative partial and full refund entitlements | #186 | #171, #195, #198 |
| ACCTTEST-007 | #200 | Review and demonstrate red: Apply reversals with debt and preserved progression | Test baseline: reversals while preserving debt and progression rules | #186 | #179, #199, #228 |
| ACCT-007 | #201 | Apply reversals with debt and preserved progression | Apply reversals while preserving debt and progression rules | #186 | #179, #199, #200, #228 |
| EPIC-003 | #202 | Rule builder, exclusions, generated events and simulator | Automation rules and event simulation | — | #152 |
| RULESTEST-001 | #203 | Review and demonstrate red: Versioned rule definitions and typed conditions | Test baseline: versioned rules and typed conditions | #202 | #152, #171 |
| RULES-001 | #204 | Versioned rule definitions and typed conditions | Add versioned rules and typed conditions | #202 | #152, #171, #203 |
| RULESTEST-002 | #205 | Review and demonstrate red: Exclusive groups and directional exclusions | Test baseline: exclusive rule groups and directional exclusions | #202 | #204 |
| RULES-002 | #206 | Exclusive groups and directional exclusions | Enforce exclusive rule groups and directional exclusions | #202 | #204, #205 |
| RULESTEST-003 | #207 | Review and demonstrate red: Ordered action planning and delegated capabilities | Test baseline: ordered actions with delegated permissions | #202 | #206, #249 |
| RULES-003 | #208 | Ordered action planning and delegated capabilities | Plan ordered actions with delegated permissions | #202 | #206, #207, #249 |
| RULESTEST-004 | #209 | Review and demonstrate red: Bounded generated-event processing and parentage | Test baseline: bounded generated events and preserved parentage | #202 | #208 |
| RULES-004 | #210 | Bounded generated-event processing and parentage | Bound generated events and preserve their parentage | #202 | #208, #209 |
| RULESTEST-005 | #211 | Review and demonstrate red: Reusable fake adapters and inspectable simulator | Test baseline: an inspectable simulator with reusable fake adapters | #202 | #192, #210, #219, #228 |
| RULES-005 | #212 | Reusable fake adapters and inspectable simulator | Add an inspectable simulator with reusable fake adapters | #202 | #192, #210, #211, #219, #228 |
| EPIC-004 | #213 | Timed modifiers, stacking and arrival-time context | Timed modifiers and stacking rules | — | #152 |
| MODIFIERTEST-001 | #214 | Review and demonstrate red: Persistent modifier definitions and activation instances | Test baseline: modifier definitions and activation instances | #213 | #152, #171 |
| MODIFIER-001 | #215 | Persistent modifier definitions and activation instances | Store modifier definitions and activation instances | #213 | #152, #171, #214 |
| MODIFIERTEST-002 | #216 | Review and demonstrate red: Independent modifier clocks and lifecycle behavior | Test baseline: independent modifier clocks and lifecycle transitions | #213 | #181, #215 |
| MODIFIER-002 | #217 | Independent modifier clocks and lifecycle behavior | Run independent modifier clocks and lifecycle transitions | #213 | #181, #215, #216 |
| MODIFIERTEST-003 | #218 | Review and demonstrate red: Per-group and per-target stacking operators | Test baseline: stacking rules by modifier group and target | #213 | #171, #215 |
| MODIFIER-003 | #219 | Per-group and per-target stacking operators | Apply stacking rules by modifier group and target | #213 | #171, #215, #218 |
| MODIFIERTEST-004 | #220 | Review and demonstrate red: Previewed edits to active instances | Test baseline: changes to active modifiers | #213 | #217, #219, #247 |
| MODIFIER-004 | #221 | Previewed edits to active instances | Preview changes to active modifiers | #213 | #217, #219, #220, #247 |
| MODIFIERTEST-005 | #222 | Review and demonstrate red: Captured modifier context in pending contribution plans | Test baseline: modifier context in pending contribution plans | #213 | #192, #219 |
| MODIFIER-005 | #223 | Captured modifier context in pending contribution plans | Capture modifier context in pending contribution plans | #213 | #192, #219, #222 |
| MODIFIERTEST-006 | #224 | Review and demonstrate red: Persist and recover modifier clocks with input holds | Test baseline: restore persisted modifier clocks while respecting input holds | #213 | #217, #223, #318 |
| MODIFIER-006 | #225 | Persist and recover modifier clocks with input holds | Restore persisted modifier clocks while respecting input holds | #213 | #217, #223, #224, #318 |
| EPIC-005 | #226 | Sequential milestones and persisted action delivery | Milestones and reliable action delivery | — | #152 |
| MILESTONETEST-001 | #227 | Review and demonstrate red: Sequential achievement and deficit calculation | Test baseline: sequential milestone progress and deficits | #226 | #152, #171 |
| MILESTONE-001 | #228 | Sequential achievement and deficit calculation | Calculate sequential milestone progress and deficits | #226 | #152, #171, #227 |
| MILESTONETEST-002 | #229 | Review and demonstrate red: Previewed target edits and completed-target corrections | Test baseline: target-edit previews and completed-target corrections | #226 | #228, #247 |
| MILESTONE-002 | #230 | Previewed target edits and completed-target corrections | Preview target edits and correct completed milestones | #226 | #228, #229, #247 |
| MILESTONETEST-003 | #231 | Review and demonstrate red: Durable delivery-job states and uncertain outcomes | Test baseline: action-delivery states and uncertain outcomes | #226 | #152, #312 |
| MILESTONE-003 | #232 | Durable delivery-job states and uncertain outcomes | Persist action-delivery states and uncertain outcomes | #226 | #152, #231, #312 |
| MILESTONETEST-004 | #233 | Review and demonstrate red: Sequential action groups and server presentation timeline | Test baseline: sequential action groups on the server timeline | #226 | #228, #232 |
| MILESTONE-004 | #234 | Sequential action groups and server presentation timeline | Execute sequential action groups on the server timeline | #226 | #228, #232, #233 |
| MILESTONETEST-005 | #235 | Review and demonstrate red: Deferred skipped and refund-held action groups | Test baseline: deferred, skipped and refund-held action groups | #226 | #234 |
| MILESTONE-005 | #236 | Deferred skipped and refund-held action groups | Handle deferred, skipped and refund-held action groups | #226 | #234, #235 |
| MILESTONETEST-006 | #237 | Review and demonstrate red: Replace unstarted actions and interrupt groups | Test baseline: unstarted-action replacement and group interruption | #226 | #230, #236 |
| MILESTONE-006 | #238 | Replace unstarted actions and interrupt groups | Replace unstarted actions and interrupt action groups | #226 | #230, #236, #237 |
| MILESTONETEST-007 | #239 | Review and demonstrate red: Manual sequential completion without score changes | Test baseline: manual milestone completion without score changes | #226 | #230, #236 |
| MILESTONE-007 | #240 | Manual sequential completion without score changes | Complete milestones manually without changing score | #226 | #230, #236, #239 |
| MILESTONETEST-008 | #241 | Review and demonstrate red: Audited completion reset and replay safeguards | Test baseline: audited milestone resets and replay safeguards | #226 | #238, #240 |
| MILESTONE-008 | #242 | Audited completion reset and replay safeguards | Audit milestone resets and prevent unsafe replay | #226 | #238, #240, #241 |
| EPIC-006 | #243 | Invites, permissions, local/Twitch identities and chat roles | Accounts, permissions, invitations and chat commands | — | #153 |
| ACCESSTEST-001 | #244 | Review and demonstrate red: Initial owner setup and invitation registration | Test baseline: first-owner setup and invitation registration | #243 | #155, #249, #260, #312, #579, #600, #603, #606 |
| ACCESS-001 | #245 | Initial owner setup and invitation registration | Add first-owner setup and invitation registration | #243 | #155, #244, #249, #260, #312, #579, #600, #603, #606 |
| ACCESSTEST-002 | #246 | Review and demonstrate red: Effective permissions and complete-grant constraints | Test baseline: effective permissions and complete-grant validation | #243 | #155 |
| ACCESS-002 | #247 | Effective permissions and complete-grant constraints | Calculate effective permissions and validate complete grants | #243 | #155, #246 |
| ACCESSTEST-003 | #248 | Review and demonstrate red: Delegation and authorized automation capabilities | Test baseline: delegated access and automation permissions | #243 | #247 |
| ACCESS-003 | #249 | Delegation and authorized automation capabilities | Enforce delegated access and automation permissions | #243 | #247, #248 |
| ACCESSTEST-004 | #250 | Review and demonstrate red: Account login and optional Twitch identity linking | Test baseline: account login and optional Twitch identity linking | #243 | #245, #262, #333, #585 |
| ACCESS-004 | #251 | Account login and optional Twitch identity linking | Add account login and optional Twitch identity linking | #243 | #245, #250, #262, #333, #585 |
| ACCESSTEST-005 | #252 | Review and demonstrate red: Recovery codes and administrative recovery | Test baseline: recovery codes and administrator-assisted recovery | #243 | #245, #249, #262, #579, #582, #600, #603, #606 |
| ACCESS-005 | #253 | Recovery codes and administrative recovery | Add recovery codes and administrator-assisted recovery | #243 | #245, #249, #252, #262, #579, #582, #600, #603, #606 |
| ACCESSTEST-006 | #254 | Review and demonstrate red: Two-party ownership transfer and fresh authentication | Test baseline: two-party ownership transfer and fresh authentication | #243 | #251, #253 |
| ACCESS-006 | #255 | Two-party ownership transfer and fresh authentication | Require two-party approval and fresh login for ownership transfer | #243 | #251, #253, #254 |
| ACCESSTEST-007 | #256 | Review and demonstrate red: Chat command categories arguments and cooldowns | Test baseline: chat parsing, command categories and cooldowns | #243 | #247 |
| ACCESS-007 | #257 | Chat command categories arguments and cooldowns | Parse chat commands and enforce categories and cooldowns | #243 | #247, #256 |
| EPIC-007 | #258 | Session/credential encryption, keys and remembered login | Secure sessions, credentials and remembered login | — | #153 |
| SESSIONTEST-001 | #259 | Review and demonstrate red: Password hashing and parameter upgrades | Test baseline: password hashing and parameter upgrades | #258 | #155 |
| SESSION-001 | #260 | Password hashing and parameter upgrades | Hash passwords and upgrade hashing parameters | #258 | #155, #259 |
| SESSIONTEST-002 | #261 | Review and demonstrate red: Opaque sessions expiry revocation and freshness | Test baseline: opaque sessions, expiry, revocation and freshness | #258 | #260, #312, #567, #568, #575, #576, #579, #582, #585 |
| SESSION-002 | #262 | Opaque sessions expiry revocation and freshness | Manage opaque sessions, expiry, revocation and freshness | #258 | #260, #261, #312, #575, #576, #579, #582, #585 |
| SESSIONTEST-003 | #263 | Review and demonstrate red: Browser cookie CSRF and desktop header transport | Test baseline: browser cookies, CSRF and desktop session headers | #258 | #262, #567, #568, #588, #593 |
| SESSION-003 | #264 | Browser cookie CSRF and desktop header transport | Secure browser cookies, CSRF and desktop session headers | #258 | #262, #263, #588, #593 |
| SESSIONTEST-004 | #265 | Review and demonstrate red: Windows DPAPI remembered-login storage | Test baseline: remembered login with Windows DPAPI | #258 | #151, #262 |
| SESSION-004 | #266 | Windows DPAPI remembered-login storage | Store remembered login with Windows DPAPI | #258 | #151, #262, #265 |
| SESSIONTEST-005 | #267 | Review and demonstrate red: Linux QtKeychain remembered-login fallback | Test baseline: remembered login with Linux QtKeychain fallback | #258 | #151, #262 |
| SESSION-005 | #268 | Linux QtKeychain remembered-login fallback | Store remembered login with Linux QtKeychain fallback | #258 | #151, #262, #267 |
| SESSIONTEST-006 | #269 | Review and demonstrate red: Encrypted provider credentials and protected key provisioning | Test baseline: encrypted provider credentials and protected key provisioning | #258 | #155, #312 |
| SESSION-006 | #270 | Encrypted provider credentials and protected key provisioning | Encrypt provider credentials and provision protected keys | #258 | #155, #269, #312 |
| SESSIONTEST-007 | #271 | Review and demonstrate red: Crash-safe credential encryption-key rotation | Test baseline: credential-key rotation and crash safety | #258 | #270 |
| SESSION-007 | #272 | Crash-safe credential encryption-key rotation | Rotate credential-encryption keys safely across crashes | #258 | #270, #271 |
| EPIC-008 | #273 | Qt/Drogon API server, durable intake and live snapshots | Server API, event intake and live synchronization | — | #153, #165, #555, #556 |
| APICONTRACT-001 | #274 | Production host lifecycle and persistent WebSocket profile contract | Define production host lifecycle and persistent WebSockets | #273 | #151, #157, #165, #579, #580 |
| APITEST-012 | #275 | Review and demonstrate red: native persistent WebSocket profile | Test baseline: the admitted native persistent WebSocket profile | #273 | #151, #165, #274 |
| API-012 | #276 | Admitted native persistent WebSocket control profile | Implement the admitted native persistent WebSocket profile | #273 | #274, #275 |
| APITEST-001 | #277 | Review and demonstrate red: Production Qt and Drogon lifecycle bridge | Test baseline: the production Qt/Drogon server lifecycle and threads | #273 | #151, #157, #165, #274, #276, #579, #580 |
| API-001 | #278 | Production Qt and Drogon lifecycle bridge | Bridge the production Qt/Drogon server lifecycle and threads | #273 | #151, #157, #165, #274, #276, #277, #579, #580, #612, #613, #616 |
| APITEST-002 | #279 | Review and demonstrate red: HTTP command schemas exact values and revision checks | Test baseline: HTTP commands, exact values and state revisions | #273 | #158, #247, #264, #278 |
| API-002 | #280 | HTTP command schemas exact values and revision checks | Validate HTTP commands, exact values and state revisions | #273 | #158, #247, #264, #278, #279 |
| APITEST-003 | #281 | Review and demonstrate red: Authorized WebSocket snapshots and resynchronization | Test baseline: authorized WebSocket synchronization and missed-state recovery | #273 | #159, #280 |
| API-003 | #282 | Authorized WebSocket snapshots and resynchronization | Synchronize authorized WebSocket clients and recover missed state | #273 | #159, #280, #281 |
| APITEST-004 | #283 | Review and demonstrate red: Capability handshake and matching web assets | Test baseline: capability negotiation and matching browser assets | #273 | #280 |
| API-004 | #284 | Capability handshake and matching web assets | Negotiate capabilities and serve matching browser assets | #273 | #280, #283 |
| APITEST-005 | #285 | Review and demonstrate red: Common connector lifecycle and capability contracts | Test baseline: shared connector lifecycle and capability interfaces | #273 | #152, #157 |
| API-005 | #286 | Common connector lifecycle and capability contracts | Implement shared connector lifecycle and capability interfaces | #273 | #152, #157, #285 |
| APITEST-006 | #287 | Review and demonstrate red: Shared catch-up baseline import and overlap policies | Test baseline: catch-up, first imports and overlap handling | #273 | #190, #286 |
| API-006 | #288 | Shared catch-up baseline import and overlap policies | Coordinate catch-up, first imports and overlap handling | #273 | #190, #286, #287 |
| APITEST-007 | #289 | Review and demonstrate red: Persisted resumable setup and isolated trial runs | Test baseline: resumable setup and isolated trial runs | #273 | #212, #280 |
| API-007 | #290 | Persisted resumable setup and isolated trial runs | Persist resumable setup and isolate trial runs | #273 | #212, #280, #289 |
| APITEST-008 | #291 | Review and demonstrate red: Early fake-event to committed timer and API vertical slice | Test baseline: the fake-event-to-committed-timer API path | #273 | #173, #179, #195, #312, #554, #555, #556, #558, #560, #562 |
| API-008 | #292 | Early fake-event to committed timer and API vertical slice | Connect a fake event to committed timer state and the API | #273 | #173, #179, #195, #291, #312, #554, #555, #556, #558, #560, #562 |
| APITEST-009 | #293 | Review and demonstrate red: Integrate review modifiers reversals milestones and recovery | Test baseline: review, modifiers, refunds, milestones and recovery | #273 | #183, #185, #201, #208, #210, #225, #242, #282, #286, #288, #292, #318 |
| API-009 | #294 | Integrate review modifiers reversals milestones and recovery | Integrate review, modifiers, refunds, milestones and recovery | #273 | #183, #185, #201, #208, #210, #225, #242, #282, #286, #288, #292, #293, #318 |
| EPIC-009 | #295 | Qt SQL migrations, backups, recovery and rollback | Database storage, migrations, backup and recovery | — | #152, #154, #310, #577 |
| STORAGETEST-001 | #296 | Review and demonstrate red: Qt SQL ownership and single SQLite writer | Test baseline: Qt SQL thread ownership and a single SQLite writer | #295 | #151, #152, #165 |
| STORAGE-001 | #297 | Qt SQL ownership and single SQLite writer | Enforce Qt SQL thread ownership and a single SQLite writer | #295 | #151, #152, #165, #296 |
| STORAGETEST-010 | #298 | Review and demonstrate red: Writer-owned dynamic repository transactions | Test baseline: writer-owned dynamic repository transactions | #295 | #152, #154, #188, #190, #297 |
| STORAGE-010 | #299 | Writer-owned dynamic repository transactions | Add writer-owned dynamic repository transactions | #295 | #152, #154, #188, #190, #297, #298 |
| STORAGETEST-011 | #300 | Review and demonstrate red: Version catalog and transactional migration harness | Test baseline: a versioned catalog and transactional migration runner | #295 | #299 |
| STORAGE-011 | #301 | Version catalog and transactional migration harness | Add a versioned catalog and transactional migration runner | #295 | #299, #300 |
| STORAGETEST-012 | #302 | Review and demonstrate red: Immutable configuration and arrival-context version retention | Test baseline: immutable configuration and event-arrival context | #295 | #152, #301 |
| STORAGE-012 | #303 | Immutable configuration and arrival-context version retention | Retain immutable configuration and event-arrival context | #295 | #152, #301, #302 |
| STORAGETEST-013 | #304 | Review and demonstrate red: Identified-receipt SQLite repository adapter | Test baseline: identified receipts through a SQLite adapter | #295 | #188, #303 |
| STORAGE-013 | #305 | Identified-receipt SQLite repository adapter | Store identified receipts through a SQLite adapter | #295 | #188, #303, #304 |
| STORAGETEST-014 | #306 | Review and demonstrate red: Contribution evidence and receipt-association SQLite adapter | Test baseline: contribution evidence and receipt associations | #295 | #190, #305 |
| STORAGE-014 | #307 | Contribution evidence and receipt-association SQLite adapter | Store contribution evidence and receipt associations | #295 | #190, #305, #306 |
| STORAGETEST-015 | #308 | Review and demonstrate red: Captured application-plan and revision-history persistence | Test baseline: captured application plans and revision history | #295 | #192, #303, #307 |
| STORAGE-015 | #309 | Captured application-plan and revision-history persistence | Persist captured application plans and revision history | #295 | #192, #303, #307, #308 |
| STORAGECONTRACT-001 | #310 | Define internal application bundle row/port contract | Define application-bundle storage rows and repository ports | — | #152, #179, #192, #309 |
| STORAGETEST-002 | #311 | Review and demonstrate red: application bundle repositories and storage integration | Test baseline: application bundles in writer-owned transactions | #295 | #297, #309, #310 |
| STORAGE-002 | #312 | Application bundle repositories and transactional storage integration | Store application bundles in writer-owned transactions | #295 | #297, #299, #301, #303, #305, #307, #309, #310, #311 |
| STORAGETEST-016 | #313 | Review and demonstrate red: nonowning durable application-review holds | Test baseline: application-review holds without taking ownership | #295 | #193, #312 |
| STORAGE-016 | #314 | Nonowning durable application-review hold repository | Persist application-review holds without taking ownership | #295 | #193, #312, #313 |
| STORAGETEST-017 | #315 | Review and demonstrate red: typed early application-claim projection | Test baseline: typed claims for the early application path | #295 | #193, #312 |
| STORAGE-017 | #316 | Typed early application-claim projection | Project typed claims for the early application path | #295 | #193, #312, #315 |
| STORAGETEST-003 | #317 | Review and demonstrate red: Durable clock checkpoints and restart integration | Test baseline: durable clock checkpoints and restart recovery | #295 | #181, #312 |
| STORAGE-003 | #318 | Durable clock checkpoints and restart integration | Persist clock checkpoints and restore them after restart | #295 | #181, #312, #317 |
| STORAGETEST-004 | #319 | Review and demonstrate red: Consistent database configuration and asset snapshot | Test baseline: consistent database, configuration and asset snapshots | #295 | #270, #312 |
| STORAGE-004 | #320 | Consistent database configuration and asset snapshot | Snapshot database, configuration and assets consistently | #295 | #270, #312, #319 |
| STORAGETEST-005 | #321 | Review and demonstrate red: Versioned encrypted backup envelope and recovery keys | Test baseline: encrypted backup envelopes and recovery keys | #295 | #320 |
| STORAGE-005 | #322 | Versioned encrypted backup envelope and recovery keys | Encrypt versioned backups with recovery keys | #295 | #320, #321 |
| STORAGETEST-006 | #323 | Review and demonstrate red: Automatic manual and pre-upgrade backup retention | Test baseline: automatic, manual and pre-upgrade backups | #295 | #322 |
| STORAGE-006 | #324 | Automatic manual and pre-upgrade backup retention | Retain automatic, manual and pre-upgrade backups | #295 | #322, #323 |
| STORAGETEST-007 | #325 | Review and demonstrate red: Stopped-server staged restore and recovery holds | Test baseline: restore stopped servers in stages with recovery holds | #295 | #232, #262, #318, #322, #579, #582 |
| STORAGE-007 | #326 | Stopped-server staged restore and recovery holds | Restore stopped servers in stages with recovery holds | #295 | #232, #262, #318, #322, #325, #579, #582 |
| STORAGETEST-008 | #327 | Review and demonstrate red: Explicit upgrade and matching-state rollback | Test baseline: explicit upgrades and matching-state rollback | #295 | #284, #324, #326 |
| STORAGE-008 | #328 | Explicit upgrade and matching-state rollback | Support explicit upgrades and matching-state rollback | #295 | #284, #324, #326, #327 |
| STORAGETEST-009 | #329 | Review and demonstrate red: Backup recipient-key rotation and old-archive recovery | Test baseline: backup-key rotation and old-archive recovery | #295 | #247, #322 |
| STORAGE-009 | #330 | Backup recipient-key rotation and old-archive recovery | Rotate backup recipient keys while retaining old-archive recovery | #295 | #247, #322, #329 |
| EPIC-010 | #331 | Twitch API, EventSub and both chat identities | Twitch events, authorization and chat identities | — | #152, #166 |
| TWITCHTEST-001 | #332 | Review and demonstrate red: Installation authorization broadcaster bot and user grants | Test baseline: installation, broadcaster, bot and user access | #331 | #166, #270, #286 |
| TWITCH-001 | #333 | Installation authorization broadcaster bot and user grants | Authorize installation, broadcaster, bot and user access | #331 | #166, #270, #286, #332 |
| TWITCHTEST-002 | #334 | Review and demonstrate red: EventSub subscriptions gifts and Bits normalization | Test baseline: Twitch subscriptions, gifts and Bits from EventSub | #331 | #288, #333 |
| TWITCH-002 | #335 | EventSub subscriptions gifts and Bits normalization | Normalize Twitch subscriptions, gifts and Bits from EventSub | #331 | #288, #333, #334 |
| TWITCHTEST-003 | #336 | Review and demonstrate red: Chat ingress and stream-state signals | Test baseline: Twitch chat and stream-state signals | #331 | #175, #257, #333 |
| TWITCH-003 | #337 | Chat ingress and stream-state signals | Receive Twitch chat and stream-state signals | #331 | #175, #257, #333, #336 |
| TWITCHTEST-004 | #338 | Review and demonstrate red: Explicit bot and broadcaster message delivery | Test baseline: Twitch messages with explicit bot or broadcaster identity | #331 | #232, #249, #333 |
| TWITCH-004 | #339 | Explicit bot and broadcaster message delivery | Send Twitch messages with explicit bot or broadcaster identity | #331 | #232, #249, #333, #338 |
| TWITCHTEST-005 | #340 | Review and demonstrate red: Connection loss capability health and unrecoverable gaps | Test baseline: Twitch connection health and unrecoverable gaps | #331 | #288, #335, #337, #339 |
| TWITCH-005 | #341 | Connection loss capability health and unrecoverable gaps | Report Twitch connection health and unrecoverable gaps | #331 | #288, #335, #337, #339, #340 |
| TWITCH-090 | #342 | Verify Twitch grants events senders and gap limitations live | Verify Twitch permissions, events and chat identities live | #331 | #294, #341 |
| EPIC-011 | #343 | betterplace public integration and aggregate visibility | betterplace polling and public donation visibility | — | #152 |
| BETTERPLACETEST-001 | #344 | Review and demonstrate red: Public polling and new-support baseline | Test baseline: betterplace polling and the new-support baseline | #343 | #286, #288 |
| BETTERPLACE-001 | #345 | Public polling and new-support baseline | Poll betterplace and establish the new-support baseline | #343 | #286, #288, #344 |
| BETTERPLACETEST-002 | #346 | Review and demonstrate red: Separate aggregate observations and private donation fields | Test baseline: separate betterplace public totals and private fields | #343 | #190, #345 |
| BETTERPLACE-002 | #347 | Separate aggregate observations and private donation fields | Separate betterplace public totals from private donation fields | #343 | #190, #345, #346 |
| BETTERPLACETEST-003 | #348 | Review and demonstrate red: Polling gap reconciliation and capability descriptors | Test baseline: betterplace gap reconciliation and capability reporting | #343 | #347 |
| BETTERPLACE-003 | #349 | Polling gap reconciliation and capability descriptors | Reconcile betterplace polling gaps and report capabilities | #343 | #347, #348 |
| BETTERPLACE-090 | #350 | Verify public fields polling identities and visibility live | Verify betterplace public fields, identities and polling live | #343 | #294, #349 |
| EPIC-012 | #351 | Patreon v2 payment evidence, webhooks and reconciliation | Patreon payments, webhooks and reconciliation | — | #152 |
| PATREONTEST-001 | #352 | Review and demonstrate red: Server-side authorization and verified webhook intake | Test baseline: Patreon authorization and webhook verification | #351 | #270, #286 |
| PATREON-001 | #353 | Server-side authorization and verified webhook intake | Authorize Patreon server access and verify incoming webhooks | #351 | #270, #286, #352 |
| PATREONTEST-002 | #354 | Review and demonstrate red: Payment evidence confirmation and refund mapping | Test baseline: Patreon payment confirmation and refund evidence | #351 | #190, #199, #288, #353 |
| PATREON-002 | #355 | Payment evidence confirmation and refund mapping | Map Patreon payment confirmation and refund evidence | #351 | #190, #199, #288, #353, #354 |
| PATREONTEST-003 | #356 | Review and demonstrate red: History reconciliation and reconnect policy | Test baseline: Patreon history reconciliation and reconnect policy | #351 | #355 |
| PATREON-003 | #357 | History reconciliation and reconnect policy | Reconcile Patreon history and reconnect safely | #351 | #355, #356 |
| PATREON-090 | #358 | Verify v2 payment identifiers charging refunds and history live | Verify Patreon payment IDs, charges, refunds and history live | #351 | #294, #357 |
| EPIC-013 | #359 | Streamlabs socket, verified payments and history | Streamlabs live payments and payment history | — | #152, #167 |
| STREAMLABSTEST-001 | #360 | Review and demonstrate red: Socket.IO authentication and live receipt normalization | Test baseline: Streamlabs Socket.IO authorization and normalized receipts | #359 | #167, #270, #286 |
| STREAMLABS-001 | #361 | Socket.IO authentication and live receipt normalization | Authenticate Streamlabs Socket.IO and normalize live receipts | #359 | #167, #270, #286, #360 |
| STREAMLABSTEST-002 | #362 | Review and demonstrate red: Paginated verified payment history and reconciliation | Test baseline: paginated, verified Streamlabs payment history | #359 | #190, #288, #361 |
| STREAMLABS-002 | #363 | Paginated verified payment history and reconciliation | Reconcile paginated, verified Streamlabs payment history | #359 | #190, #288, #361, #362 |
| STREAMLABSTEST-003 | #364 | Review and demonstrate red: Gap catch-up first import and capability reporting | Test baseline: Streamlabs catch-up, first imports and capabilities | #359 | #363 |
| STREAMLABS-003 | #365 | Gap catch-up first import and capability reporting | Handle Streamlabs catch-up, first imports and capabilities | #359 | #363, #364 |
| STREAMLABS-090 | #366 | Verify Socket.IO live-to-REST payment evidence live | Verify Streamlabs live-to-history payment correlation | #359 | #294, #365 |
| EPIC-014 | #367 | StreamElements Astro tips and recovery limits | StreamElements tips and recovery boundaries | — | #152 |
| STREAMELEMENTSTEST-001 | #368 | Review and demonstrate red: Astro connection and completed-tip normalization | Test baseline: StreamElements Astro connections and completed tips | #367 | #270, #286 |
| STREAMELEMENTS-001 | #369 | Astro connection and completed-tip normalization | Connect StreamElements Astro and normalize completed tips | #367 | #270, #286, #368 |
| STREAMELEMENTSTEST-002 | #370 | Review and demonstrate red: Payment identity source authority and eligibility | Test baseline: StreamElements payment identity, authority and eligibility | #367 | #190, #288, #369 |
| STREAMELEMENTS-002 | #371 | Payment identity source authority and eligibility | Preserve StreamElements payment identity, authority and eligibility | #367 | #190, #288, #369, #370 |
| STREAMELEMENTS-003 | #372 | Establish refund and gap-recovery capability boundaries | Establish StreamElements refund and gap-recovery limits | #367 | #371 |
| STREAMELEMENTSTEST-004 | #373 | Review and demonstrate red: Capability-aware reconnect and visible gap handling | Test baseline: StreamElements reconnect and visible gap handling | #367 | #372 |
| STREAMELEMENTS-004 | #374 | Capability-aware reconnect and visible gap handling | Reconnect StreamElements and make unsupported gaps visible | #367 | #372, #373 |
| STREAMELEMENTS-090 | #375 | Verify Astro events evidence and gap behavior live | Verify StreamElements Astro events and recovery behavior live | #367 | #294, #374 |
| EPIC-015 | #376 | Ko-fi payment webhooks and deduplication | Ko-fi payments and duplicate protection | — | #152 |
| KOFITEST-001 | #377 | Review and demonstrate red: Authenticated payment webhook receipt and deduplication | Test baseline: authenticated Ko-fi webhooks and payment deduplication | #376 | #188, #270, #286 |
| KOFI-001 | #378 | Authenticated payment webhook receipt and deduplication | Authenticate Ko-fi webhooks and reject duplicate payments | #376 | #188, #270, #286, #377 |
| KOFITEST-002 | #379 | Review and demonstrate red: Tip membership and other payment eligibility mapping | Test baseline: Ko-fi tips, memberships and other payment eligibility | #376 | #190, #288, #378 |
| KOFI-002 | #380 | Tip membership and other payment eligibility mapping | Map Ko-fi tips, memberships and other payment eligibility | #376 | #190, #288, #378, #379 |
| KOFI-003 | #381 | Establish payment refund and history capability evidence | Establish Ko-fi payment, refund and history capabilities | #376 | #380 |
| KOFI-090 | #382 | Verify installation webhook eligibility and duplicate handling live | Verify Ko-fi webhook eligibility and deduplication live | #376 | #294, #381 |
| EPIC-016 | #383 | TipeeeStream socket/history and evidence | TipeeeStream events, history and recovery | — | #152, #167 |
| TIPEEESTREAM-001 | #384 | Establish current protocol payload and payment semantics | Establish TipeeeStream protocol and payment semantics | #383 | #167, #286 |
| TIPEEESTREAMTEST-002 | #385 | Review and demonstrate red: Compatible socket authorization and live intake | Test baseline: TipeeeStream socket authorization and live intake | #383 | #270, #384 |
| TIPEEESTREAM-002 | #386 | Compatible socket authorization and live intake | Authorize TipeeeStream sockets and receive live events | #383 | #270, #384, #385 |
| TIPEEESTREAMTEST-003 | #387 | Review and demonstrate red: Filtered event history and live correlation | Test baseline: filtered TipeeeStream history and live-event correlation | #383 | #190, #288, #386 |
| TIPEEESTREAM-003 | #388 | Filtered event history and live correlation | Filter TipeeeStream history and correlate live events | #383 | #190, #288, #386, #387 |
| TIPEEESTREAMTEST-004 | #389 | Review and demonstrate red: Reconnection catch-up and capability health | Test baseline: TipeeeStream reconnect and capability health | #383 | #388 |
| TIPEEESTREAM-004 | #390 | Reconnection catch-up and capability health | Recover TipeeeStream connections and report capability health | #383 | #388, #389 |
| TIPEEESTREAM-090 | #391 | Verify socket history identity and confirmation behavior live | Verify TipeeeStream sockets, history and payment confirmation live | #383 | #294, #390 |
| EPIC-017 | #392 | Frankfurter, ExchangeRate-API and Open Exchange Rates | Currency conversion with all three rate providers | — | #152 |
| RATESTEST-001 | #393 | Review and demonstrate red: Lossless fetched and effective rate snapshot model | Test baseline: lossless fetched and effective exchange-rate snapshots | #392 | #152, #171 |
| RATES-001 | #394 | Lossless fetched and effective rate snapshot model | Preserve fetched and effective exchange-rate snapshots exactly | #392 | #152, #171, #393 |
| RATESTEST-002 | #395 | Review and demonstrate red: Frankfurter provider adapter and freshness | Test baseline: Frankfurter rates and freshness checks | #392 | #286, #394 |
| RATES-002 | #396 | Frankfurter provider adapter and freshness | Integrate Frankfurter rates and freshness checks | #392 | #286, #394, #395 |
| RATESTEST-003 | #397 | Review and demonstrate red: ExchangeRate-API plan and quota-aware adapter | Test baseline: ExchangeRate-API with plan and quota handling | #392 | #270, #286, #394 |
| RATES-003 | #398 | ExchangeRate-API plan and quota-aware adapter | Integrate ExchangeRate-API with plan and quota handling | #392 | #270, #286, #394, #397 |
| RATESTEST-004 | #399 | Review and demonstrate red: Open Exchange Rates plan and usage adapter | Test baseline: Open Exchange Rates with plan and usage handling | #392 | #270, #286, #394 |
| RATES-004 | #400 | Open Exchange Rates plan and usage adapter | Integrate Open Exchange Rates with plan and usage handling | #392 | #270, #286, #394, #399 |
| RATESTEST-005 | #401 | Review and demonstrate red: Persistent sync schedule retries and request accounting | Test baseline: rate-sync schedules, retries and request accounting | #392 | #312, #396, #398, #400 |
| RATES-005 | #402 | Persistent sync schedule retries and request accounting | Persist rate-sync schedules, retries and request accounting | #392 | #312, #396, #398, #400, #401 |
| RATESTEST-006 | #403 | Review and demonstrate red: Ordered fallback and source provenance | Test baseline: ordered rate-provider fallback with source provenance | #392 | #402 |
| RATES-006 | #404 | Ordered fallback and source provenance | Apply ordered rate-provider fallback with source provenance | #392 | #402, #403 |
| RATESTEST-007 | #405 | Review and demonstrate red: Consistent manual inverse cross-rate and pin overrides | Test baseline: consistent manual, inverse, cross-rate and pinned overrides | #392 | #247, #312, #394 |
| RATES-007 | #406 | Consistent manual inverse cross-rate and pin overrides | Support consistent manual, inverse, cross-rate and pinned overrides | #392 | #247, #312, #394, #405 |
| RATESTEST-008 | #407 | Review and demonstrate red: Historical display conversion and missing valuation review | Test baseline: historical display conversion and missing-valuation review | #392 | #190, #404, #406 |
| RATES-008 | #408 | Historical display conversion and missing valuation review | Convert historical display values and review missing valuations | #392 | #190, #404, #406, #407 |
| RATES-090 | #409 | Verify all three rate provider plan profiles | Verify all three exchange-rate providers and plan profiles | #392 | #408 |
| EPIC-018 | #410 | Qt Quick desktop management and permissions | Desktop management application | — | #151, #153 |
| DESKTOPTEST-001 | #411 | Review and demonstrate red: Qt Quick shell login and capability-aware connection | Test baseline: the Qt Quick shell, login and capability-aware connection | #410 | #151, #264, #284 |
| DESKTOP-001 | #412 | Qt Quick shell login and capability-aware connection | Build the Qt Quick shell, login and capability-aware connection | #410 | #151, #264, #284, #411 |
| DESKTOPTEST-002 | #413 | Review and demonstrate red: Live timer grace pause and reopening controls | Test baseline: desktop timer, grace, pause and reopening controls | #410 | #183, #282, #412 |
| DESKTOP-002 | #414 | Live timer grace pause and reopening controls | Add desktop timer, grace, pause and reopening controls | #410 | #183, #282, #412, #413 |
| DESKTOPTEST-003 | #415 | Review and demonstrate red: Contribution review and reversal management panels | Test baseline: desktop contribution review and reversal panels | #410 | #197, #201, #412 |
| DESKTOP-003 | #416 | Contribution review and reversal management panels | Add desktop contribution review and reversal panels | #410 | #197, #201, #412, #415 |
| DESKTOPTEST-004 | #417 | Review and demonstrate red: Structured rule editor and simulation inspection | Test baseline: the desktop rule editor and simulation inspector | #410 | #212, #412 |
| DESKTOP-004 | #418 | Structured rule editor and simulation inspection | Add the desktop rule editor and simulation inspector | #410 | #212, #412, #417 |
| DESKTOPTEST-005 | #419 | Review and demonstrate red: Modifier configuration and active-instance controls | Test baseline: desktop modifier setup and active-instance controls | #410 | #225, #412 |
| DESKTOP-005 | #420 | Modifier configuration and active-instance controls | Add desktop modifier setup and active-instance controls | #410 | #225, #412, #419 |
| DESKTOPTEST-006 | #421 | Review and demonstrate red: Milestone editor and action queue controls | Test baseline: the desktop milestone editor and action-queue controls | #410 | #242, #412 |
| DESKTOP-006 | #422 | Milestone editor and action queue controls | Add the desktop milestone editor and action-queue controls | #410 | #242, #412, #421 |
| DESKTOPTEST-007 | #423 | Review and demonstrate red: Accounts roles invitations sessions and audit management | Test baseline: desktop account, role, invitation, session and audit management | #410 | #255, #264, #412 |
| DESKTOP-007 | #424 | Accounts roles invitations sessions and audit management | Add desktop account, role, invitation, session and audit management | #410 | #255, #264, #412, #423 |
| DESKTOPTEST-008 | #425 | Review and demonstrate red: Provider connection setup health and capability panels | Test baseline: desktop integration setup and capability-health panels | #410 | #286, #412 |
| DESKTOP-008 | #426 | Provider connection setup health and capability panels | Add desktop integration setup and capability-health panels | #410 | #286, #412, #425 |
| DESKTOPTEST-009 | #427 | Review and demonstrate red: Rate providers reporting currency and override panels | Test baseline: desktop rate-provider, reporting-currency and override panels | #410 | #408, #412 |
| DESKTOP-009 | #428 | Rate providers reporting currency and override panels | Add desktop rate-provider, reporting-currency and override panels | #410 | #408, #412, #427 |
| DESKTOPTEST-010 | #429 | Review and demonstrate red: Resumable guided desktop setup and trial events | Test baseline: resumable guided desktop setup and trial events | #410 | #290, #414, #418, #420, #422, #426, #428, #445 |
| DESKTOP-010 | #430 | Resumable guided desktop setup and trial events | Add resumable guided desktop setup and trial events | #410 | #290, #414, #418, #420, #422, #426, #428, #429, #445 |
| EPIC-019 | #431 | Desktop-only scene editor, themes and media assets | Desktop scene editor, themes and assets | — | #151, #153 |
| SCENETEST-001 | #432 | Review and demonstrate red: Versioned scene document drafts and server asset references | Test baseline: versioned scene drafts and server asset references | #431 | #160, #247 |
| SCENE-001 | #433 | Versioned scene document drafts and server asset references | Store versioned scene drafts and server asset references | #431 | #160, #247, #432 |
| SCENETEST-002 | #434 | Review and demonstrate red: Native canvas selection geometry grouping and layers | Test baseline: canvas selection, geometry, groups and layers | #431 | #412, #433 |
| SCENE-002 | #435 | Native canvas selection geometry grouping and layers | Add canvas selection, geometry, groups and layers | #431 | #412, #433, #434 |
| SCENETEST-003 | #436 | Review and demonstrate red: Binding visibility and lifecycle preview editors | Test baseline: scene binding, visibility and lifecycle preview editors | #431 | #435 |
| SCENE-003 | #437 | Binding visibility and lifecycle preview editors | Add scene binding, visibility and lifecycle preview editors | #431 | #435, #436 |
| SCENETEST-004 | #438 | Review and demonstrate red: Undo redo and scene draft edit transactions | Test baseline: transactional scene edits with undo and redo | #431 | #437 |
| SCENE-004 | #439 | Undo redo and scene draft edit transactions | Add transactional scene edits with undo and redo | #431 | #437, #438 |
| SCENETEST-005 | #440 | Review and demonstrate red: Asset upload budgets restricted SVG and font validation | Test baseline: asset uploads, restricted SVGs and fonts within budgets | #431 | #433 |
| SCENE-005 | #441 | Asset upload budgets restricted SVG and font validation | Validate asset uploads, restricted SVGs and fonts within budgets | #431 | #433, #440 |
| SCENETEST-006 | #442 | Review and demonstrate red: Portable themes and asset font import export | Test baseline: portable themes, assets and fonts | #431 | #439, #441 |
| SCENE-006 | #443 | Portable themes and asset font import export | Import and export portable themes, assets and fonts | #431 | #439, #441, #442 |
| SCENETEST-007 | #444 | Review and demonstrate red: Permissioned live publishing restore and compatibility checks | Test baseline: live scenes with permission and compatibility checks | #431 | #443, #447 |
| SCENE-007 | #445 | Permissioned live publishing restore and compatibility checks | Publish and restore live scenes with permission and compatibility checks | #431 | #443, #444, #447 |
| EPIC-020 | #446 | Independent native/browser renderers and OBS outputs | Native and browser overlays with OBS output | — | #153, #168 |
| RENDER-001 | #447 | Author shared scene reference fixtures and conformance oracles | Author shared scene fixtures and independent conformance oracles | #446 | #160 |
| RENDERTEST-002 | #448 | Review and demonstrate red: Native QML scene elements and synchronized bindings | Test baseline: native QML scene elements with synchronized bindings | #446 | #151, #159, #433, #447 |
| RENDER-002 | #449 | Native QML scene elements and synchronized bindings | Render native QML scene elements with synchronized bindings | #446 | #151, #159, #433, #447, #448 |
| RENDERTEST-003 | #450 | Review and demonstrate red: Independent browser scene renderer and state projection | Test baseline: the independent browser renderer and state projection | #446 | #159, #433, #447 |
| RENDER-003 | #451 | Independent browser scene renderer and state projection | Implement the independent browser renderer and state projection | #446 | #159, #433, #447, #450 |
| RENDERTEST-004 | #452 | Review and demonstrate red: Elapsed-time animations media and font profiles | Test baseline: animation, media and fonts from elapsed-time profiles | #446 | #441, #449, #451 |
| RENDER-004 | #453 | Elapsed-time animations media and font profiles | Drive animation, media and fonts from elapsed-time profiles | #446 | #441, #449, #451, #452 |
| RENDERTEST-005 | #454 | Review and demonstrate red: Revocable overlay credentials and live scene revisions | Test baseline: overlay credential revocation and live scene revisions | #446 | #282, #445, #449, #451, #484 |
| RENDER-005 | #455 | Revocable overlay credentials and live scene revisions | Revoke overlay credentials and synchronize live scene revisions | #446 | #282, #445, #449, #451, #454, #484 |
| RENDER-090 | #456 | Verify native OBS capture on KDE Wayland | Check native overlay capture in OBS on KDE Wayland | #446 | #168, #455 |
| RENDER-091 | #457 | Verify native and browser OBS outputs on Windows | Verify native and browser OBS output on Windows | #446 | #168, #455 |
| RENDER-092 | #458 | Verify Linux OBS browser-source output and media | Check browser overlays and media in Linux OBS | #446 | #168, #455 |
| EPIC-021 | #459 | React/TypeScript/Vite management foundation and workflows | Browser management application | — | #153 |
| WEBAPPTEST-001 | #460 | Review and demonstrate red: React TypeScript Vite shell and static server delivery | Test baseline: the React/TypeScript/Vite shell and static asset delivery | #459 | #158, #284 |
| WEBAPP-001 | #461 | React TypeScript Vite shell and static server delivery | Build the React/TypeScript/Vite shell and serve static assets | #459 | #158, #284, #460 |
| WEBAPPTEST-002 | #462 | Review and demonstrate red: Browser login sessions and permission-aware live state | Test baseline: browser login, sessions and permission-aware live state | #459 | #251, #264, #282, #461 |
| WEBAPP-002 | #463 | Browser login sessions and permission-aware live state | Add browser login, sessions and permission-aware live state | #459 | #251, #264, #282, #461, #462 |
| WEBAPPTEST-003 | #464 | Review and demonstrate red: Browser timer lifecycle and recovery operations | Test baseline: browser timer lifecycle and recovery controls | #459 | #183, #463 |
| WEBAPP-003 | #465 | Browser timer lifecycle and recovery operations | Add browser timer lifecycle and recovery controls | #459 | #183, #463, #464 |
| WEBAPPTEST-004 | #466 | Review and demonstrate red: Browser contribution review reversals and silent processing | Test baseline: browser contribution review, reversals and silent processing | #459 | #197, #201, #463 |
| WEBAPP-004 | #467 | Browser contribution review reversals and silent processing | Add browser contribution review, reversals and silent processing | #459 | #197, #201, #463, #466 |
| WEBAPPTEST-005 | #468 | Review and demonstrate red: Browser rules and simulator workflows | Test baseline: browser rule management and simulation | #459 | #212, #463 |
| WEBAPP-005 | #469 | Browser rules and simulator workflows | Add browser rule management and simulation | #459 | #212, #463, #468 |
| WEBAPPTEST-006 | #470 | Review and demonstrate red: Browser modifier definition and active-instance workflows | Test baseline: browser modifier definitions and active-instance controls | #459 | #225, #463 |
| WEBAPP-006 | #471 | Browser modifier definition and active-instance workflows | Add browser modifier definitions and active-instance controls | #459 | #225, #463, #470 |
| WEBAPPTEST-007 | #472 | Review and demonstrate red: Browser accounts delegation and session management | Test baseline: browser account, delegation and session management | #459 | #255, #463 |
| WEBAPP-007 | #473 | Browser accounts delegation and session management | Add browser account, delegation and session management | #459 | #255, #463, #472 |
| WEBAPPTEST-008 | #474 | Review and demonstrate red: Browser provider setup and capability health | Test baseline: browser integration setup and capability health | #459 | #286, #463 |
| WEBAPP-008 | #475 | Browser provider setup and capability health | Add browser integration setup and capability health | #459 | #286, #463, #474 |
| WEBAPPTEST-009 | #476 | Review and demonstrate red: Browser milestone editing completion and action queue | Test baseline: browser milestone editing, completion and action queues | #459 | #242, #463 |
| WEBAPP-009 | #477 | Browser milestone editing completion and action queue | Add browser milestone editing, completion and action queues | #459 | #242, #463, #476 |
| WEBAPPTEST-010 | #478 | Review and demonstrate red: Browser rate configuration and historical conversion review | Test baseline: browser rate configuration and historical-conversion review | #459 | #408, #463 |
| WEBAPP-010 | #479 | Browser rate configuration and historical conversion review | Add browser rate configuration and historical-conversion review | #459 | #408, #463, #478 |
| WEBAPPTEST-011 | #480 | Review and demonstrate red: Browser resumable setup simulation and start | Test baseline: resumable browser setup, simulation and start | #459 | #290, #445, #465, #469, #471, #475, #477, #479 |
| WEBAPP-011 | #481 | Browser resumable setup simulation and start | Add resumable browser setup, simulation and start | #459 | #290, #445, #465, #469, #471, #475, #477, #479, #480 |
| EPIC-022 | #482 | Viewer access, milestones, supporter privacy and statistics | Viewer pages, supporter privacy and statistics | — | #153 |
| VIEWERTEST-001 | #483 | Review and demonstrate red: Viewer access modes and separately scoped links | Test baseline: viewer access modes and separately scoped links | #482 | #158, #159, #247, #264 |
| VIEWER-001 | #484 | Viewer access modes and separately scoped links | Add viewer access modes and separately scoped links | #482 | #158, #159, #247, #264, #483 |
| VIEWERTEST-002 | #485 | Review and demonstrate red: Source-respecting supporter privacy projections | Test baseline: source privacy when displaying supporter information | #482 | #190, #484 |
| VIEWER-002 | #486 | Source-respecting supporter privacy projections | Respect source privacy when displaying supporter information | #482 | #190, #484, #485 |
| VIEWERTEST-003 | #487 | Review and demonstrate red: Exact metrics and historical refund aggregation | Test baseline: exact viewer metrics and historical refund totals | #482 | #201, #408, #486 |
| VIEWER-003 | #488 | Exact metrics and historical refund aggregation | Calculate exact viewer metrics and historical refund totals | #482 | #201, #408, #486, #487 |
| VIEWERTEST-004 | #489 | Review and demonstrate red: Graph bucketing archive corrections and export query contract | Test baseline: graph buckets, archive corrections and export queries | #482 | #488 |
| VIEWER-004 | #490 | Graph bucketing archive corrections and export query contract | Define graph buckets, archive corrections and export queries | #482 | #488, #489 |
| VIEWERTEST-005 | #491 | Review and demonstrate red: React viewer page milestone sections and recent support | Test baseline: viewer milestone sections and recent-support displays | #482 | #228, #461, #490 |
| VIEWER-005 | #492 | React viewer page milestone sections and recent support | Build viewer milestone sections and recent-support displays | #482 | #228, #461, #490, #491 |
| VIEWERTEST-006 | #493 | Review and demonstrate red: Permissioned CSV and JSON export without secrets | Test baseline: authorized CSV and JSON without exposing secrets | #482 | #247, #490 |
| VIEWER-006 | #494 | Permissioned CSV and JSON export without secrets | Export authorized CSV and JSON without exposing secrets | #482 | #247, #490, #493 |
| VIEWERTEST-007 | #495 | Review and demonstrate red: Retention deletion and archive access safeguards | Test baseline: retention, deletion and archive-access safeguards | #482 | #312, #494 |
| VIEWER-007 | #496 | Retention deletion and archive access safeguards | Enforce retention, deletion and archive-access safeguards | #482 | #312, #494, #495 |
| EPIC-023 | #497 | English/German catalogs and Light/Dark/System | English/German localization and appearance settings | — | #153 |
| LOCALETEST-001 | #498 | Review and demonstrate red: Shared translation catalog schema and build generator | Test baseline: the shared translation schema and build generator | #497 | #141 |
| LOCALE-001 | #499 | Shared translation catalog schema and build generator | Add the shared translation schema and build generator | #497 | #141, #498 |
| LOCALETEST-002 | #500 | Review and demonstrate red: Native runtime language locale and saved preferences | Test baseline: desktop language and locale with saved preferences | #497 | #412, #499 |
| LOCALE-002 | #501 | Native runtime language locale and saved preferences | Switch desktop language and locale with saved preferences | #497 | #412, #499, #500 |
| LOCALETEST-003 | #502 | Review and demonstrate red: Web management and viewer runtime translation | Test baseline: runtime translation of browser management and viewer pages | #497 | #461, #492, #499 |
| LOCALE-003 | #503 | Web management and viewer runtime translation | Translate browser management and viewer pages at runtime | #497 | #461, #492, #499, #502 |
| LOCALETEST-004 | #504 | Review and demonstrate red: Independent Light Dark and System appearance | Test baseline: independent Light, Dark and System appearance settings | #497 | #412, #461 |
| LOCALE-004 | #505 | Independent Light Dark and System appearance | Add independent Light, Dark and System appearance settings | #497 | #412, #461, #504 |
| LOCALE-090 | #506 | Verify English German and theme layouts on native and browser UIs | Verify English/German text and theme layouts across both UIs | #497 | #430, #475, #481, #501, #503, #505 |
| EPIC-024 | #507 | Docker Compose, Podman Quadlet, native Linux and Caddy | Linux server installation and deployment | — | #153 |
| DEPLOYMENTTEST-001 | #508 | Review and demonstrate red: Common application image storage and secret layout | Test baseline: the shared server image and storage/secret layout | #507 | #270, #294, #312 |
| DEPLOYMENT-001 | #509 | Common application image storage and secret layout | Build the shared server image and storage/secret layout | #507 | #270, #294, #312, #508 |
| DEPLOYMENTTEST-002 | #510 | Review and demonstrate red: Caddy defaults and existing reverse proxy contract | Test baseline: bundled Caddy and existing reverse proxies | #507 | #282 |
| DEPLOYMENT-002 | #511 | Caddy defaults and existing reverse proxy contract | Support bundled Caddy and existing reverse proxies | #507 | #282, #510 |
| DEPLOYMENTTEST-003 | #512 | Review and demonstrate red: Docker Compose lifecycle and persistent services | Test baseline: persistent services with Docker Compose | #507 | #328, #509, #511, #593 |
| DEPLOYMENT-003 | #513 | Docker Compose lifecycle and persistent services | Deploy persistent services with Docker Compose | #507 | #328, #509, #511, #512, #593 |
| DEPLOYMENTTEST-004 | #514 | Review and demonstrate red: Rootless Podman Quadlet default deployment | Test baseline: the default rootless Podman Quadlet setup | #507 | #328, #509, #511, #593 |
| DEPLOYMENT-004 | #515 | Rootless Podman Quadlet default deployment | Deploy the default rootless Podman Quadlet setup | #507 | #328, #509, #511, #514, #593 |
| DEPLOYMENTTEST-005 | #516 | Review and demonstrate red: Rootful Podman with containerized Caddy alternative | Test baseline: rootful Podman with containerized Caddy | #507 | #515 |
| DEPLOYMENT-005 | #517 | Rootful Podman with containerized Caddy alternative | Support rootful Podman with containerized Caddy | #507 | #515, #516 |
| DEPLOYMENTTEST-006 | #518 | Review and demonstrate red: Native Linux server archive and systemd installation | Test baseline: native Linux server archives and systemd installation | #507 | #294, #328, #511, #595 |
| DEPLOYMENT-006 | #519 | Native Linux server archive and systemd installation | Package native Linux server archives and systemd installation | #507 | #294, #328, #511, #518, #595 |
| DEPLOYMENT-090 | #520 | Verify Docker persistence HTTPS upgrade and recovery | Verify Docker persistence, HTTPS, upgrades and recovery | #507 | #513 |
| DEPLOYMENT-091 | #521 | Verify both Podman Quadlet deployment variants | Check rootless and rootful Podman Quadlet deployments | #507 | #515, #517 |
| DEPLOYMENT-092 | #522 | Verify native Ubuntu service install update and restore | Verify native Ubuntu installation, updates and restore | #507 | #519 |
| EPIC-025 | #523 | Windows portable ZIP, Linux archives and Arch package | Windows and Linux desktop distribution | — | #151 |
| PACKAGETEST-001 | #524 | Review and demonstrate red: Pinned native build and runtime dependency manifests | Test baseline: native build and runtime dependencies in manifests | #523 | #151 |
| PACKAGE-001 | #525 | Pinned native build and runtime dependency manifests | Pin native build and runtime dependencies in manifests | #523 | #151, #524 |
| PACKAGETEST-002 | #526 | Review and demonstrate red: Windows installation-free portable ZIP | Test baseline: the Windows desktop as a portable ZIP | #523 | #266, #430, #455, #501, #525 |
| PACKAGE-002 | #527 | Windows installation-free portable ZIP | Package the Windows desktop as a portable ZIP | #523 | #266, #430, #455, #501, #525, #526 |
| PACKAGETEST-003 | #528 | Review and demonstrate red: Bundled Linux desktop archive and relocation policy | Test baseline: a relocatable Linux desktop archive | #523 | #268, #430, #455, #501, #525 |
| PACKAGE-003 | #529 | Bundled Linux desktop archive and relocation policy | Package a relocatable Linux desktop archive | #523 | #268, #430, #455, #501, #525, #528 |
| PACKAGETEST-004 | #530 | Review and demonstrate red: Arch PKGBUILD against system dependencies | Test baseline: an Arch PKGBUILD using system dependencies | #523 | #268, #430, #455, #501, #525 |
| PACKAGE-004 | #531 | Arch PKGBUILD against system dependencies | Add an Arch PKGBUILD using system dependencies | #523 | #268, #430, #455, #501, #525, #530 |
| PACKAGE-005 | #532 | Verify clean Windows portable startup media and moved login | Verify portable Windows startup, media and moved-login behavior | #523 | #457, #527 |
| PACKAGE-006 | #533 | Verify Linux archives Arch package and secure keyrings | Verify Linux archives, the Arch package and secure keyrings | #523 | #456, #529, #531 |
| EPIC-026 | #534 | Target platform/browser/OBS matrix and performance evidence | Platform compatibility and performance verification | — | #446, #507, #523 |
| VALIDATE-001 | #535 | Maintain complete requirement and integration evidence matrix | Maintain the complete requirements and integration evidence matrix | #534 | #155 |
| VALIDATE-002 | #536 | Run combined core accounting recovery and security scenarios | Verify combined timer, accounting, recovery and security scenarios | #534 | #183, #185, #201, #225, #238, #242, #255, #272, #294 |
| VALIDATE-003 | #537 | Run supported browser and client compatibility matrix | Verify supported browsers and client compatibility | #534 | #284, #475, #477, #479, #481, #496, #506 |
| VALIDATE-004 | #538 | Benchmark server eligible-input latency and viewer load | Benchmark server input latency and viewer load | #534 | #238, #294, #492, #520 |
| VALIDATE-005 | #539 | Benchmark both scene renderers and missed-frame timing | Benchmark both renderers and missed-frame timing | #534 | #456, #457, #458 |
| VALIDATE-006 | #540 | Consolidate platform package capture and restore release gates | Consolidate platform, packaging, capture and restore release evidence | #534 | #520, #521, #522, #532, #533, #535, #536, #537, #538, #539 |
| EPIC-027 | #541 | Release license/source manifests, diagnostics and documentation | Release documentation, licenses and diagnostics | — | #534 |
| RELEASETEST-001 | #542 | Review and demonstrate red: Dependency font license and corresponding-source manifests | Test baseline: dependency/font license and corresponding-source manifests | #541 | #141 |
| RELEASE-001 | #543 | Dependency font license and corresponding-source manifests | Generate dependency/font license and corresponding-source manifests | #541 | #141, #542 |
| RELEASETEST-002 | #544 | Review and demonstrate red: Operational log retention and redacted diagnostic export | Test baseline: operational log retention and redacted diagnostic export | #541 | #157, #247, #286 |
| RELEASE-002 | #545 | Operational log retention and redacted diagnostic export | Retain operational logs and export redacted diagnostics | #541 | #157, #247, #286, #544 |
| RELEASE-003 | #546 | Write operator setup recovery and client workflow guides | Write operator setup, recovery and client guides | #541 | #290, #328, #330, #430, #475, #481, #519 |
| RELEASE-004 | #547 | Review final artifacts notices diagnostics and release checklist | Review release artifacts, notices, diagnostics and the final checklist | #541 | #169, #186, #202, #213, #226, #243, #258, #273, #295, #331, #343, #351, #359, #367, #376, #383, #392, #410, #431, #446, #459, #482, #497, #507, #523, #534, #540, #543, #545, #546 |
| BOOT-007 | #548 | Add a resumable coordinator prompt | Add a coordinator handoff and resume prompt | — | #143 |
| BOOT-008 | #549 | Grow CI with implemented behavior | Extend CI alongside implemented behavior | — | #150, #548 |
| RATECONTRACT-001 | #550 | Exact retained valuation bridge contract | Define exact rates for retained historical valuations | #392 | #192, #195, #309, #394 |
| RATECONTRACT-002 | #551 | Exact valuation API, codec and additive storage contract | Define exact valuation APIs, codecs and additive storage | #392 | #550, #564, #565 |
| RATESTEST-009 | #552 | Review and demonstrate red: Exact rate snapshots in retained historical valuations | Test baseline: exact rate snapshots in historical valuations | #392 | #192, #195, #309, #394, #550, #551, #609 |
| RATES-009 | #553 | Exact rate snapshots in retained historical valuations | Retain exact rate snapshots in historical valuations | #392 | #192, #195, #309, #394, #550, #551, #552, #609 |
| CONTRACTPLAN-003 | #554 | Realign the early end-to-end reference path | Realign the first end-to-end reference path | — | #156, #157, #195 |
| CONTRACT-013 | #555 | Bounded reference HTTP contract and schema | Define the bounded reference HTTP interface and schema | — | #157, #554, #567, #568, #571 |
| CONTRACT-014 | #556 | Single-rule reference path and adapter contract | Define the single-rule reference path and adapters | — | #152, #171, #173, #179, #195, #554 |
| RULESTEST-006 | #557 | Review and demonstrate red: Single-rule reference evaluator | Test baseline: the single-rule reference evaluator | #202 | #171, #249, #556 |
| RULES-006 | #558 | Single-rule reference evaluator | Implement the single-rule reference evaluator | #202 | #171, #249, #556, #557 |
| APITEST-010 | #559 | Review and demonstrate red: Reference HTTP snapshot | Test baseline: reference HTTP negotiation and committed snapshots | #273 | #247, #264, #278, #312, #555, #589 |
| API-010 | #560 | Reference HTTP negotiation and committed snapshot | Negotiate the reference HTTP API and return committed snapshots | #273 | #247, #264, #278, #312, #555, #559, #589 |
| APITEST-011 | #561 | Review and demonstrate red: Reference intake and application adapters | Test baseline: reference intake and current application adapters | #273 | #173, #179, #188, #190, #192, #195, #249, #305, #307, #309, #312, #556, #558, #564 |
| API-011 | #562 | Reference intake and current application adapters | Connect reference event intake to current application adapters | #273 | #173, #179, #188, #190, #192, #195, #249, #305, #307, #309, #312, #556, #558, #561, #564 |
| STORAGETEST-018 | #563 | Review and demonstrate red: durable initial source/capture binding | Test baseline: initial source and capture bindings | #295 | #303, #305, #307, #309, #556 |
| STORAGE-018 | #564 | Durable initial source/capture binding | Persist initial source and capture bindings | #295 | #303, #305, #307, #309, #556, #563, #565 |
| STORAGE-019 | #565 | Bounded reference capture structural codecs | Add bounded structural codecs for reference captures | #295 | #563 |
| CONTRACTPLAN-004 | #566 | Explicit session supplier contract prerequisite | Separate the current-session supplier prerequisite | — | #157, #260, #312 |
| SESSIONCONTRACT-001 | #567 | Current-session supplier contract | Define current-session lookup and supplied facts | #258 | #157, #260, #312, #566 |
| SESSIONCONTRACT-002 | #568 | Browser and desktop credential transport contract | Define browser and desktop credential transport | #258 | #567 |
| CONTRACTPLAN-005 | #569 | Explicit schema-validation foundation prerequisite | Separate the schema-validation tooling prerequisite | — | #157 |
| SCHEMATEST-001 | #570 | Reviewed schema-validation wrapper baseline | Establish the reviewed failing schema-wrapper baseline | — | #157, #569 |
| SCHEMATOOL-001 | #571 | Offline schema-package validation wrapper | Validate schema packages offline with the selected SDK | — | #570 |
| BOOT-009 | #572 | Keep agent assignments and review overhead bounded | Bound agent assignments and review overhead | — | #548 |
| CONTRACTPLAN-006 | #573 | Split current-session and lifecycle architectural prerequisites | Separate session lookup from session lifecycle work | — | #260, #312, #567, #568 |
| SESSIONTEST-008 | #574 | Review and demonstrate red: bounded account-session supplier | Test baseline: token preparation and current-session resolution | #258 | #260, #567, #573 |
| SESSION-008 | #575 | Bounded token preparation and current-session resolution | Prepare tokens and resolve current sessions | #258 | #260, #567, #573, #574 |
| SESSIONCONTRACT-003 | #576 | Concrete session lifecycle mutation and inspection contract | Define session lifecycle changes and inspection | #258 | #247, #260, #299, #567, #568, #573 |
| STORAGECONTRACT-002 | #577 | Account, credential and current-session storage contract | Define account, credential and current-session storage | — | #260, #299, #301, #573, #576 |
| STORAGETEST-020 | #578 | Review and demonstrate red: physical account/session store | Test baseline: SQLite account/session storage and current fact sources | #295 | #299, #301, #577 |
| STORAGE-020 | #579 | Physical account/session store and current fact producers | Implement SQLite account/session storage and current fact sources | #295 | #299, #301, #577, #578 |
| SESSIONCONTRACT-004 | #580 | Trustworthy current session-clock producer contract | Define trustworthy current session-clock facts | #258 | #152, #573, #576, #577 |
| SESSIONTEST-009 | #581 | Review and demonstrate red: actual current session clock | Test baseline: trustworthy current session-clock facts | #258 | #151, #278, #579, #580 |
| SESSION-009 | #582 | Actual current session-clock producer | Produce trustworthy current session-clock facts | #258 | #151, #278, #579, #580, #581 |
| ACCESSCONTRACT-001 | #583 | Actual accepted-local-login authority contract | Define the authority for accepted local login | #243 | #260, #573, #576, #577, #580 |
| ACCESSTEST-008 | #584 | Review and demonstrate red: accepted local-login authority | Test baseline: accepted local login and current password facts | #243 | #245, #260, #579, #582, #583, #588, #600, #606 |
| ACCESS-008 | #585 | Accepted local login and current password-source integration | Connect accepted local login to current password facts | #243 | #245, #260, #579, #582, #583, #584, #588, #600, #606 |
| SESSIONCONTRACT-005 | #586 | Concrete lossless and secret-safe ingress solution | Define lossless account ingress and secret handling | #258 | #157, #568, #573, #589 |
| SESSIONTEST-010 | #587 | Review and demonstrate red: actual lossless account ingress | Test baseline: lossless account ingress and secret handling | #258 | #151, #278, #586, #591 |
| SESSION-010 | #588 | Actual lossless and secret-safe account ingress source | Preserve account ingress exactly without leaking secrets | #258 | #151, #278, #586, #587, #591 |
| SESSIONCONTRACT-006 | #589 | Reconcile edge and application ingress boundaries | Align edge and application account-ingress responsibilities | #258 | #157, #555, #568, #573 |
| SESSIONTEST-011 | #590 | Review and demonstrate red: selected native ingress artifact | Test baseline: the selected native account-ingress implementation | #258 | #151, #165, #586, #616 |
| SESSION-011 | #591 | Selected native ingress artifact correction | Correct the selected native account-ingress implementation | #258 | #151, #165, #586, #590, #616 |
| DEPLOYMENTTEST-007 | #592 | Review and demonstrate red: bundled proxy account ingress admission | Test baseline: account ingress through the bundled proxy | #507 | #278, #588, #589 |
| DEPLOYMENT-007 | #593 | Bundled proxy account ingress admission | Validate account ingress through the bundled proxy | #507 | #278, #588, #589, #592 |
| DEPLOYMENTTEST-008 | #594 | Review and demonstrate red: alternative proxy account ingress admission | Test baseline: account ingress through alternative proxies | #507 | #278, #588, #589 |
| DEPLOYMENT-008 | #595 | Alternative proxy account ingress admission | Validate account ingress through alternative proxies | #507 | #278, #588, #589, #594 |
| WORKFLOW-001 | #596 | T3 coordination and derived task context | Add T3 coordination and derived task context | — | #129, #275 |
| WORKFLOW-002 | #597 | Coordinator-selected reviews with Codex correction cycles | Add coordinator-selected reviews and Codex correction cycles | — | #596 |
| APICONTRACT-002 | #598 | Bounded host authentication continuation contract | Define bounded host authentication continuation | #273 | #274, #579, #583, #586 |
| APITEST-013 | #599 | Review and demonstrate red: Host authentication continuation extension | Test baseline: host authentication continuation | #273 | #278, #579, #588, #598 |
| API-013 | #600 | Host authentication continuation extension | Extend the host with authentication continuation | #273 | #278, #579, #588, #598, #599 |
| APICONTRACT-003 | #601 | Guarded enrollment and recovery host contract | Define guarded host enrollment and recovery | #273 | #579, #586, #598 |
| APITEST-014 | #602 | Review and demonstrate red: guarded enrollment and recovery host extension | Test baseline: guarded host enrollment and recovery | #273 | #600, #601 |
| API-014 | #603 | Guarded enrollment and recovery host extension | Extend the host with guarded enrollment and recovery | #273 | #600, #601, #602 |
| SESSIONCONTRACT-007 | #604 | AUTH-route transport budget and edge-anchor contract | Define authentication-route transport budgets and edge anchors | #258 | #588, #593, #595, #598 |
| SESSIONTEST-012 | #605 | Review and demonstrate red: AUTH-route transport budget producer | Test baseline: authentication-route transport budgets at the edge | #258 | #588, #593, #595, #600, #604 |
| SESSION-012 | #606 | AUTH-route transport budget producer and edge admission | Enforce authentication-route transport budgets at the edge | #258 | #588, #593, #595, #600, #604, #605 |
| STORAGECONTRACT-003 | #607 | Controlled parent-table rebuild migration contract | Define controlled parent-table rebuild migrations | #295 | #299, #301, #551, #579 |
| STORAGETEST-021 | #608 | Review and demonstrate red: controlled parent-table rebuild migration | Test baseline: controlled parent-table rebuild migrations | #295 | #607 |
| STORAGE-021 | #609 | Controlled parent-table rebuild migration capability | Support controlled parent-table rebuild migrations | #295 | #607, #608 |
| CI-001 | #610 | Reliable Ubuntu package acquisition on GitHub runners | Make Ubuntu package acquisition reliable on GitHub runners | — | #597 |
| WORKFLOW-003 | #611 | Structured review gates, bounded handoffs and publication reconciliation | Track review stages, bounded handoffs and PR publication | — | #551, #597, #610 |
| APICONTRACT-004 | #612 | Isolated HTTP and native WebSocket host listener contract | Define isolated HTTP and native WebSocket listeners | #273 | #274, #276, #277 |
| APITEST-015 | #613 | Review and demonstrate red: corrected dual-listener host baseline | Test baseline: corrected HTTP and native WebSocket listener isolation | #273 | #276, #277, #612, #616 |
| APICONTRACT-005 | #614 | Bounded selected HTTP transport foundation contract | Define the selected bounded HTTP transport foundation | #273 | #276, #586, #612 |
| APITEST-016 | #615 | Review and demonstrate red: bounded HTTP transport foundation | Test baseline: the admitted bounded HTTP transport foundation | #273 | #151, #276, #614 |
| API-016 | #616 | Admitted bounded HTTP transport foundation | Implement the admitted bounded HTTP transport foundation | #273 | #276, #614, #615 |
| WORKFLOW-004 | #617 | Bounded coordination policy and focused implementation plan | Define bounded coordination and a focused implementation plan | — | #611 |
| WORKFLOW-005 | #618 | Compact context and publication preflight | Add compact task context and publication preflight checks | — | #617 |
| WORKFLOW-006 | #619 | Artifact-aware approval and results-only records | Bind approvals to artifacts and record execution results only | — | #618, #630 |
| WORKFLOW-007 | #620 | Reusable bounded synthetic execution runner | Add a reusable runner for bounded synthetic checks | — | #619 |
| CI-002 | #621 | Actionable acquisition diagnostics and enforced merge checks | Explain acquisition failures and enforce required merge checks | — | #610 |
| CI-003 | #624 | Validate the Noble split security source profile | Validate Ubuntu Noble archive and security sources | — | #621 |
| EPIC-029 | #625 | Move task management to GitHub | Move task management to GitHub | — | — |
| MIGRATE-001 | #626 | Define the task-tracking migration contract | Define the task-tracking migration contract | #625 | — |
| MIGRATE-002 | #627 | Separate review evidence from task tracking | Separate review evidence from task tracking | #625 | #626 |
| MIGRATE-003 | #628 | Prepare publication checks and CI for evidence records | Prepare publication checks and CI for evidence records | #625 | #627 |
| MIGRATE-004 | #629 | Switch task authority to GitHub and retire the tracker | Switch task authority to GitHub and retire the tracker | #625 | #628 |
| MIGRATE-005 | #630 | Verify and close the task-tracking migration | Verify and close the task-tracking migration | #625 | #631 |
| MIGRATE-006 | #631 | Compact active task briefs and archive completed briefs | Compact active task briefs and archive completed briefs | #625 | #629 |
