# API-023 Host fatal consumer source candidate

[#684](https://github.com/Kernol12/subathon-bot-ai/issues/684) · Tier A tests stage.
The coordinator supplied the [night claim](https://github.com/Kernol12/subathon-bot-ai/issues/684#issuecomment-7091053084)
and accepted standalone source scope. The protected source is
`71a1c2e5f661f8d144bac6bdf93a6aaec07d5302`, tree `d752321ed39b064dba90ef494232805855550521`.

| Frozen input | SHA-256 |
| --- | --- |
| 176 A · `contract-fix3.md` | `abb683401068112e6a565392fd5126af9b794c45b9faa9214de4f12464d56601` |
| Contract closing | `3cf15d79bd61fbe6b62128e7b90087410017d45e2e0b13ebb91cd1889f4b40cb` |
| Standalone source handoff | `6d368ebc461aa84f3069e07f5dc2b9bc1e3407792e378f48a46c01e44dde89c1` |
| Standalone source closing | `f53196574c7c04442bbb966dcdeb57c8faa62709862bf937d9bf466bbbf14fd7` |
| Coordinator acceptance | `661d851ebeb1df5127029f47aaba9bc53ffa01bf43e62811f56f96d497c362c1` |

## Boundary and forecast

Sections 2 and 3 of the frozen contract supply the expectations. Each fatal
selector hands one fixture-owned immutable fact to the private Host consumer
from the existing fake IO owner. No producer, listener, provider, socket, SDK
failure stimulus or clock fault is involved. The ordinary-stop rows use
constant fixture elapsed time of 2 s and the default 5 s drain. The fixture
leaves the actual writer in charge of work admission and completion.

| Planned Qt selector | Independent witness after the handoff | Predicted first missing checkpoint |
| --- | --- | --- |
| `ordinary_stop_control` | Serving positive. D = 7,000,000,000 ns, one ordinary fence, no fatal report, writer receipt plus held fake network receipt before `stopped`. | None. Protected control forecast only. |
| `fatal_stop_order:fatal_first` | No ordinary stop request before the Host consumes the fact. Deadline absent, first fake transport argument zero, no new elapsed read. Exact typed inspection and one local line survive later ordinary stop and both receipts, ending in `failed`. | Private forward after serving and before any stop. |
| `fatal_stop_order:ordinary_first_sink_refuses` | Ordinary fence, D and the writer receipt exist before the fact. D and the original transport argument survive. Sink refusal preserves inspection and there is no retry. Held network receipt still blocks owner cleanup. | Private forward while ordinarily stopping. |
| `published_before_queued_host_readiness` | Fake startup has delivered readiness to the Host callback, with its Qt work pending. No readiness success, installation-read enqueue or activation after publication. Accepted catalog inspection completes normally. | Private forward after fake callback delivery and before the Qt queue runs. |
| `accepted_units_drain_and_later_command_stays_out` | Two accepted writer units retain committed and rolled-back outcomes in FIFO. Waiting wrapper has no entry or writer result. A third submission is refused before the Qt turn and the finishing reply is withheld. Pending units give no writer receipt. After drain, held fake network receipt still blocks owner cleanup. | Private forward after two writer admissions inside the first dispatcher callback. |

`forward_published_fact` is a single test-local throwing stub. Its intended
destination is the first-party transport-to-Host binding that delivers
Foundation's already-published `bounded_http::FatalElapsed` to the Host Engine's
shared record. The eventual fake should forward to that owning boundary with
a typed conversion only, without calling `request_stop()` or running a Qt turn.
Protected `detail::TransportComposition` has no relay binding, and
`Host::Impl` currently connects only ingress, command and reply ownership.
The stub therefore has no existing private function to name. The source
adapts future observation through the frozen `Inspection::fatal_elapsed` and
optional sink (the fixture assumes the member spelling `Options::fatal_diagnostic`
and aggregate order context then callback). Those probes remain passive and the
fatal selectors demand real Host observation after the forward. The local sink
only captures bytes and delivery count in fixed-size storage without logging,
blocking or reentering Host.

## Passive assembly handoff

- Translation unit `tests/qt/api_023/host_fatal_consumer_tests.cpp`,
  Qt guiless main with AUTOMOC, using the existing
  `tests/qt/support/api_host_fixture.hpp`. Register each literal method or data
  selector above in a separate process if this exact target is later admitted.
  Keep the new selectors out of existing inventories and ordinary passing CI.
- The existing Host test target dependencies are `subathon_api_host`,
  `Qt6::Test` and `subathon_test_fixtures`. That is a **source dependency
  reference**. Protected `subathon_api_host` transitively selects
  `subathon_native_websocket_profile`, which links `Drogon::Drogon`.
  `Host::start` calls `native_ws::create_profile` even for a fake transport.
  A standalone assembly with no actual SDK therefore still needs an authorized,
  reviewed SDK-free closure. Merely swapping the transport does not establish
  one. Shared registration, closure and recipe decisions remain with the
  coordinator.
- The reused disposable fixture matches this Host's compiled **v10** catalog.
  `Host::start` inspects `reference_authority_repository_manifest()` and
  requires that compiled head before activation. A disposable v21 database
  cannot reach the active-command oracles through that entry point at this pin.
  No catalog substitutions, schema changes or borrowed #238 reservation.

## Pending gates

The network receipt represents only the in-process owner's barrier handoff.
The tests hold that receipt until the real writer finishes. Demonstrating
network-first while an accepted Qt writer body or completion is held requires
a reviewed scheduling boundary beyond this fixture. The startup selector
checks the Host side of queued readiness. Transport `ListenersObserver`,
post-read activation-only ordering and the full set of final authority gates
need separate oracles. The fake records the first stop argument. Transport
call cardinality and cancellation of deferred Close stay with their owner.

Preserve the frozen Foundation 37, LegacyControl, native44, API-001 Qt87/native30
and API-027 157 selector receipts, with R4 Release scope and candidate4/NI
custody intact. The source here supplies no frozen composed/native H-case
credit. H1 remains separate startup-lifetime triage, with endpoint discovery
before the first fault in frozen row 279. H2 stays with #685's shared
registration seal and one Producer slot. N1 retains the cold no-start
combined-failure exclusion and the separate early `StartAccepted` direct
branch and prelaunch refusal. Those wake/bootstrap variants are outside this
slice. N2 applies to diagnostic and log files. Fault induction, missing or
throwing Qt post validation, producer, socket/Close retirement, broader startup
and composed Release runtime remain pending.

Still due are assembly and its source/resource review, exact binary closure and
review admission, independent Tier A test review and fresh admission for
observed intended red. The implementation and its Tier A cycle, R4 §8 Release
ABI/layout/resource attestation and matched consumer admission, exact-head CI
and PR/proof receipts are future gates. Wider Host/exit composition (#294),
operational logging (#545), #685 composition and #812 Debug qualification
retain their owners. This is authored source with a forecast only. No compile,
configure, artifact load, test discovery or runtime evidence.
