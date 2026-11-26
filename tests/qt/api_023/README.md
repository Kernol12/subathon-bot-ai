# API-023 Host fatal consumer tests

Tier A tests for [#684](https://github.com/Kernol12/subathon-bot-ai/issues/684):
the private Host consumer of a fatal elapsed-time fact that Foundation has
already published. Sections 2 and 3 of the frozen contract supply the
expectations.

## Boundary

Each fatal selector hands one fixture-owned immutable fact to the private Host
consumer from the existing fake IO owner. No producer, listener, provider,
socket, SDK failure stimulus or clock fault is involved. The ordinary-stop rows
use constant fixture elapsed time of 2 s and the default 5 s drain. The fixture
leaves the actual writer in charge of work admission and completion.

## Selectors

All five selectors pass against the Stage 2 Host consumer. Before it, the
control passed and the other four failed at a `forward_published_fact` stub
(credited intended red at `0a20f6ff`).

| Qt selector | Independent witness after the handoff | Expected outcome |
| --- | --- | --- |
| `ordinary_stop_control` | Serving positive. D = 7,000,000,000 ns, one ordinary fence, no fatal report, writer receipt plus held fake network receipt before `stopped`. | Passes. |
| `fatal_stop_order:fatal_first` | No ordinary stop request before the Host consumes the fact. Deadline absent, first fake transport argument zero, no new elapsed read. Exact typed inspection and one local line survive later ordinary stop and both receipts, ending in `failed`. | Passes. |
| `fatal_stop_order:ordinary_first_sink_refuses` | Ordinary fence, D and the writer receipt exist before the fact. D and the original transport argument survive. Sink refusal preserves inspection and there is no retry. Held network receipt still blocks owner cleanup. | Passes. |
| `published_before_queued_host_readiness` | Fake startup has delivered readiness to the Host callback, with its Qt work pending. No readiness success, installation-read enqueue or activation after publication. Accepted catalog inspection completes normally. | Passes. |
| `accepted_units_drain_and_later_command_stays_out` | Two accepted writer units retain committed and rolled-back outcomes in FIFO. Waiting wrapper has no entry or writer result. A third submission is refused before the Qt turn and the finishing reply is withheld. Pending units give no writer receipt. After drain, held fake network receipt still blocks owner cleanup. | Passes. |

## Forward seam and observer probes

`forward_published_fact` maps the supplied example to a typed
`bounded_http::FatalElapsed` and passes it, on the fixture IO owner, to
`testing::HostAccess::publish_fatal`. That calls the Host's private fatal relay
binding, the same one `detail::TransportComposition::bind_fatal` receives. The
fixture's fake transport is a plain `Transport`, so the test reaches the binding
through `HostAccess` rather than through a composition.

`unavailable_at_scan` maps to `{sample_unavailable, scan}` and
`overflow_at_reply_deadline` to `{deadline_overflow, reply_deadline}`. The body
makes no `request_stop()`, Qt post or invoke, wake, wait or catch, and writes no
elapsed, sink or fake-transport field. The Host's own owner turn must consume
the fact.

The source observes through `Inspection::fatal_elapsed` and the optional
`Options::fatal_diagnostic` sink (aggregate order: context, then callback).
Those probes stay passive and the fatal selectors demand real Host observation
after the forward. A `static_assert` requires both members or neither, so the
control's no-fatal checks cannot go silently vacuous through one missing or
misspelled member. The local sink only captures bytes and delivery count in
fixed-size storage without logging, blocking or reentering Host.

## Opt-in build

`SUBATHON_BUILD_API_023_HOST_CONSUMER_TESTS` defaults OFF and exists only with
`SUBATHON_BUILD_API_HOST_TESTS`. It builds `server_api_023_host_consumer` and
registers each selector above as its own CTest command
(`api-023-host-consumer.<selector>`), one Host per process, with the existing
180 s bound under the `api-023-host-consumer` label. The `native` CI job builds
them in its host lane and runs that label as required success. They stay out of
the existing host inventories.

## Catalog

The Host uses its compiled **v10** catalog. `Host::start` inspects
`reference_authority_repository_manifest()` and requires that compiled head
before activation; `Rig("current")` builds the disposable v10 database with no
substitution. Existing v21 fixtures cannot reach this entry point. No catalog
substitutions, schema changes or borrowed #238 reservation.

## Declared gaps

The fatal selectors need a Qt owner turn without a test-driven wake, so they
depend on the Host's 10 ms owner timer but cover none of its setup, bootstrap or
wake-failure variants.

The network receipt represents only the in-process owner's barrier handoff.
The tests hold that receipt until the real writer finishes. Demonstrating
network-first while an accepted Qt writer body or completion is held requires
a reviewed scheduling boundary beyond this fixture. The startup selector
checks the Host side of queued readiness. Transport `ListenersObserver`,
post-read activation-only ordering and the full set of final authority gates
need separate oracles. The fake records the first stop argument. Transport
call cardinality and cancellation of deferred Close stay with their owner.

These selectors do not cover fault induction, missing or throwing Qt post
validation, the producer, socket/Close retirement, broader startup or composed
Release runtime.
