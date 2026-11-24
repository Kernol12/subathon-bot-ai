# API-023 Host fatal consumer source candidate

[#684](https://github.com/Kernol12/subathon-bot-ai/issues/684) · Tier A tests stage.
The coordinator supplied the [night claim](https://github.com/Kernol12/subathon-bot-ai/issues/684#issuecomment-7091053084)
and accepted standalone source scope. The protected source is
`aa04f15238987192c5338d7d63cf289033d59e1f` under the
[advisor re-point](https://github.com/Kernol12/subathon-bot-ai/issues/684#issuecomment-7126776377) from the
original `71a1c2e5f661f8d144bac6bdf93a6aaec07d5302`. None of the 52 commits
between them changes the Foundation, Host, Drogon transport, native profile,
SDK patches, manifest or `tests/qt/support/api_host_fixture.hpp`.

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

## Coordinator-authorized passive assembly

This section records the assembly at the original `71a1c2e5` basis. Its
runner cannot be admitted for a run on the current basis. Since then main
changed `server/include/subathon/server/sqlite_writer.hpp` in the test's
include closure (`bc891c3d`: a private member and friend declarations, no
layout change) and the linked writer and migration sources. A fresh assembly
and header-dependency readback on the current basis supersede this one.

The follow-up assembled the exact test source at `439cc6ef9f71a267f16c9fca56d2a3d8433a6dd0`
(tree `1e4df0c638e1f025495c9f894d1b523f2adf0fc9`). The coordinator
authorized the retained SDK as a link-time input for the unchanged normal
`subathon_api_host`, its v10 catalog and the separate native profile.
`SUBATHON_BUILD_API_023_HOST_CONSUMER_TESTS` defaults OFF and is scoped to
the existing Host-test selection. It adds five distinct CTest commands with
one selector per process, the existing 180 s bound and SDK path policy,
and ordinary failure semantics under the intentional-red label.

The `api-host-debug` preset configured the fresh retained
`build/API-023/test-assembly` directory. Ninja compiled and linked only
`server_api_023_host_consumer` and its prerequisites, in 54 steps. The
compiler reported the protected `Options::limits` shadow warning. CMake
reported the redundant disconnected-fetch flag as unused. The authored test
and prior fixtures are byte-identical. Static readback matched all five new
CTest commands and all 157 existing Host registrations in frozen order.
Generated MOC contains four methods and the `fatal_stop_order_data` hook,
with no fixture-defined init or cleanup hook. Both row tags and the private
forward marker are present in the ELF. This is generated-file and symbol
readback. The one control and four stub outcomes remain forecasts.

The coordinator's `roots.txt` selected the historical successor SDK.
All 176 entries in `sdk-176.sha256` matched before and after build.
The identity digest is
`d89b7744b4f97c02d9eafc5de12bc9ea0772b58814d4d5fbc30b3adacf1ca294`.
The protected installed manifest, 16 Release options, upstream pins, ordered
patches, and original and derived native sources matched their recorded
identities. Compiler dependency readback names the sealed native build copy
and its SDK headers. The explicit libsodium root passed configuration, but
the runner link graph contains no libsodium archive.

The retained `evidence/` directory contains the configure/build commands and
logs, compiler and CTest command readback, 23 link archives with members and
hashes, and 33 prospective ELF objects including the interpreter, Qt and the
separate QSQLITE plugin root. The protected passive ELF method checked
SONAMEs, search paths, version requirements, strong imports for the runner
and both SDK DSOs, seven patched exports and the successor marker. Both the
runner and Drogon define default-visible Session RTTI in `.dynsym` and
`.symtab`. The scoped link/symbol inventory contains the real Host and
native profile, with no Drogon transport adapter or Foundation archive.

| Retained evidence | SHA-256 |
| --- | --- |
| Runner ELF | `e42e7598394a41b5f659b5302c75fd5104d3ba6ba5687c284f22aefdad9d5d8c` |
| `evidence.sha256` | `5560ed846918a3f34d5c7c3b0f62e39436135127d0096447c223f43cf1b3ab06` |
| `elf-closure.sha256` | `aeb620907678b251f1f3bf861300e376e9ed5e5008081dda0d8845ea06ba679e` |
| `passive-abi.json` | `a97227ffe7f1be97e436af818bdde49f60bbf53f975de8f4626d0ccafd8d17f0` |
| `registration.cmake` | `3cbefd98e1527c72219a54bf38cfc86515086f566386b5c39662bb92b769b0d8` |

The fixture supplies only fake transport and synthetic in-process peers. The
SDK-linked ELF establishes no service, socket, provider, lookup or vendor
behavior. No candidate was loaded, no discovery or Qt hook ran and no
SQLite work executed. Any separately authorized synthetic Host-only run
still requires fresh exact admission, including the controlled environment,
reached loader and plugin maps and reconciled pre/post hashes. Tier A test
review, credited intended red, implementation review and frozen native,
composed and Release gates remain pending.
