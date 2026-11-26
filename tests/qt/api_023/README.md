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

## Stage 3 selectors

r4 §2 "Delivery and wake" and Correction 3 (advisor 684.6: `ResponseLease::take`
and `try_copy` refuse a published fatal with `Failure::stopping`, changing
nothing). All 21 selectors pass against the Stage 3 implementation. The
credited intended red at `13636dfd` is kept per row: four controls passed,
eight failed at their first assertion and nine aborted at the escaping throw.

| Qt selector | Independent witness | Credited red at `13636dfd` | Outcome |
| --- | --- | --- | --- |
| `lease_take_under_fatal:fatal` | Fatal published on IO before `take`/`try_copy` of a retained lease: both refuse `stopping`, no charge, association or handoff changes; later `take` still refuses; the normal owner releases the association; `failed` after both receipts. | Fails: `take` succeeds. | Passes. |
| `lease_take_under_fatal:ordinary` | Control: after an ordinary stop `take` still succeeds (output window kept); `stopped`, no diagnostic. | Passes. | Passes. |
| `post_failure_is_contained:stop_refuse_fatal` | Control: a refused stop post cannot suppress a published fatal. | Passes. | Passes. |
| `post_failure_is_contained:{stop,disconnect,lease}_raise_fatal` | The site's owner-turn post throws: the throw stays inside the boundary; disconnect retires its peer charge, or the last lease release retires its association; the owner timer drives one fence, one report and `failed` after both receipts. | Aborts at the escaping throw. | Passes. |
| `post_failure_is_contained:{stop,disconnect,lease}_{refuse,raise}_ordinary` | The site's post is refused or throws, no fatal; for disconnect and lease a follow-up foreign stop's post is refused too, so only the owner timer and the first failure's obligation can drive the turn: one fence, `stopped`, no diagnostic. | Fails: stop never consumed (refuse), or aborts at the escaping throw (raise). | Passes. |
| `cold_stop_without_start:{refuse,raise}` | Cold Host, running dispatcher, no `start()`: a nonowner stop whose post fails still gets one fence and real cleanup to `stopped`, with no writer open, transport start, receipt or report; repeats are inert; later `start()` is `already_started`. | Fails (refuse) or aborts (raise). | Passes. |
| `stop_before_writer_open:{refuse,raise}` | Stop latched after startup preparation; the initial post and the early-branch re-wake both fail: `StartAccepted`, one fence, cleanup to `stopped`, no startup work, report or receipt. | Fails (refuse) or aborts (raise). | Passes. |
| `timer_registration_failure_refuses_start` | Control: abstract registration failure gives the existing prelaunch refusal and cleanup, with no launch or report. | Passes. | Passes. |
| `deferred_bootstrap:stop_latched` | Host built before its thread has a dispatcher, stop latched with its post refused: the one bootstrap event consumes the stop first; no registration, startup work or receipt. | Fails: stop never consumed. | Passes. |
| `deferred_bootstrap:stop_latched_raise` | As above with the latching post throwing. | Aborts at the escaping throw. | Passes. |
| `deferred_bootstrap:arms_once` | Same construction, no stop: the bootstrap arms the timer exactly once. | Fails: never armed. | Passes. |
| `deferred_bootstrap:start_before_bootstrap` | Control: live `start()` reaches prelaunch before the bootstrap is delivered; a late bootstrap cannot register twice. | Passes. | Passes. |

The seams are test-only `testing::HostAccess` members: `inject_post_faults`
replaces the owner-turn post in `Engine::wake` (refuse returns false, raise
throws `InjectedPostFault`); `refuse_timer_registration`; `before_writer_open`;
and `owner_timer` (registered state and count). Production never sets them.
`pending_post_faults` lets each selector check that the injected fault met
the stimulus's own post. In the disconnect and lease ordinary rows the
follow-up stop's obligation suffices to drive the turn, so the site's own
obligation is not separately observable. `report_first` records the first
assertion before `Rig` teardown, whose watchdog ends a process whose stop
never completes.
These selectors carry the required `api-023-host-consumer` label with the
Stage 2 rows.

r4 fixes no terminal state after a registration refusal, so that control
accepts `stopped` or `failed`. r4 places the timer disarm "after both barriers
and relay quiescence, or on unstarted owner destruction"; for a never-started
Host the selectors therefore assert one registration, not a disarm point.

## Opt-in build

`SUBATHON_BUILD_API_023_HOST_CONSUMER_TESTS` defaults OFF and exists only with
`SUBATHON_BUILD_API_HOST_TESTS`. It builds `server_api_023_host_consumer` and
registers all 26 selectors above (five Stage 2, 21 Stage 3) as their own CTest
commands (`api-023-host-consumer.<selector>`), one Host per process, with the
existing 180 s bound under the `api-023-host-consumer` label. The `native` CI job builds
them in its host lane and runs that label as required success. They stay out of
the existing host inventories.

## Catalog

The Host uses its compiled **v10** catalog. `Host::start` inspects
`reference_authority_repository_manifest()` and requires that compiled head
before activation; `Rig("current")` builds the disposable v10 database with no
substitution. Existing v21 fixtures cannot reach this entry point. No catalog
substitutions, schema changes or borrowed #238 reservation.

## Declared gaps

The Stage 2 fatal selectors need a Qt owner turn without a test-driven wake,
so they depend on the Host's 10 ms owner timer. Stage 3 covers its setup,
bootstrap and post-failure variants except these: an idle serving Host with
zero peers (the fatal control keeps one peer and lease), a coalesced rather
than refused ordinary wake under fatal, and the synchronized-claim races in
which a stale delivery or failure cleanup must not erase a newer claim or
obligation. Those need a scheduling boundary beyond this fixture.

The network receipt represents only the in-process owner's barrier handoff.
The tests hold that receipt until the real writer finishes. Demonstrating
network-first while an accepted Qt writer body or completion is held requires
a reviewed scheduling boundary beyond this fixture. The startup selector
checks the Host side of queued readiness. Transport `ListenersObserver`,
post-read activation-only ordering and the full set of final authority gates
need separate oracles. The Drogon listener-observer fatal gate (I2b) needs live
listener discovery and waits for H1 #837. The fake records the first stop argument. Transport
call cardinality and cancellation of deferred Close stay with their owner.

These selectors do not cover elapsed fault induction, the producer,
socket/Close retirement, broader startup or composed Release runtime. Post
failures are injected only through the test seam above.
