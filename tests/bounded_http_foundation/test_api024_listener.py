# SPDX-License-Identifier: GPL-3.0-or-later
"""API-024 Stage 1: listener accept-error hold, recovery and stop (frozen r3, packet r2).

Opt-in intentional red (SUBATHON_BUILD_API_024_STAGE1), outside passing CI. It reuses the
frozen APITEST-016 harness and probe; the probe's API-024 code is insertion-only and runs
only for argv carrying "api024". Every expected value below comes from contract r3
(``contract-fix2.md``) and packet r2, never from the implementation.

Until the implementation lands, the observer arm returns NI. Each red case then ends with
exactly one ``missing_producer`` event whose boundary is STUB, after its earlier checkpoints.
"""
from __future__ import annotations

import argparse
import json
import sys
import unittest
from pathlib import Path

import test_bounded_http as tb

STUB = "API-024 stub: arm_listener_observer returned NI"
HTTP, NATIVE = 0, 1
CLEAR, HELD, TIME_FENCED, STOPPED = 0, 1, 2, 3
DRAINED, STOPPED_END, ELAPSED_FAILED = 0, 4, 5  # bh::End values
NO_PENDING, OPERATIONAL, PASS = 1, 2, 3          # outcome script codes
MAX = 2**64 - 1


def ms(value):
    """r3 milliseconds on the fixture's nanosecond clock."""
    return value * 1_000_000


class Child(tb.Child):
    """The frozen harness, with the API-024 stub boundary as the only added NI attribution."""

    def terminal_failure(self):
        e = self.terminal
        if (e is not None and e["event"] == "missing_producer" and "api024" in self.config
                and e.get("boundary") == STUB):
            self.reap(40)
            self.cleanup()
            raise AssertionError(f"MISSING_PRODUCER {STUB}; admitted reached maps; exact exit40")
        super().terminal_failure()


class ListenerTests(unittest.TestCase):
    # -- fixture helpers ------------------------------------------------------------
    def child(self, **config):
        return Child(self, mode="feature", **config).start()

    def cold(self, **config):
        return Child(self, mode="feature", **config)

    @staticmethod
    def observe(p):
        p.command("api024-observe")
        return p.wait("api024_record")

    @staticmethod
    def turn(p, ns, role=HTTP, available=True):
        """Owner turn at ns: facts read in the IO0 functor that sees the witness rise."""
        p.command("api024-owner-turn", ns=ns, role=role, available=available)
        return p.wait("api024_turn")

    @staticmethod
    def until(p, field, value, role=HTTP):
        p.command("api024-wait", role=role, field=field, value=value)
        return p.wait("api024_waited")

    def record(self, snapshot, role, **expected):
        actual = snapshot["roles"][role]
        self.assertEqual({k: actual[k] for k in expected}, expected, f"role {role}")

    def inspect(self, snapshot, **expected):
        actual = snapshot["inspect"]
        self.assertEqual({k: actual[k] for k in expected}, expected, "inspection")

    def same(self, a, b):
        """Every record fact except the witness itself is unchanged."""
        for role in (HTTP, NATIVE):
            left = {k: v for k, v in a["roles"][role].items() if k != "owner_evaluations"}
            right = {k: v for k, v in b["roles"][role].items() if k != "owner_evaluations"}
            self.assertEqual(left, right, f"role {role} changed")
        self.assertEqual(a["inspect"], b["inspect"])

    def checkpoint0(self, p):
        """Correction 2: exactly one enable and one registration per role after startup."""
        x = self.observe(p)
        for role in (HTTP, NATIVE):
            self.record(x, role, interest_enables=1, poller_registrations=1, final_removals=0,
                        phase=CLEAR, interest=1, no_pending=0, operational=0, hold_deadline=0,
                        acceptor_retirements=0)
        self.inspect(x, accept_attempts=0, peers=0, transient_fds=0)
        return x

    def finish_stopped(self, p):
        """After a stop the test drove itself: drain, joins, final record, exact exit 0."""
        p.wait("stop_drained")
        drained = p.wait("api024_drained")
        p.wait("framework_joined")
        final = p.wait("api024_final")
        p.wait("maps_end", phase="cleanup")
        p.check_maps("cleanup")
        p.wait("cleanup_complete")
        self.assertEqual(p.reap(0), 0)
        p.closed = True
        p.cleanup()
        return drained, final

    def window(self, mode):
        p = self.cold(profile="tiny", api024=mode)
        p.wait("api024_window_pass")
        p.wait("maps_end", phase="cleanup")
        p.check_maps("cleanup")
        p.wait("cleanup_complete")
        self.assertEqual(p.reap(0), 0)
        p.closed = True
        p.cleanup()

    # -- control --------------------------------------------------------------------
    def test_control_existing_accept_and_stop(self):
        """No observer: the shipped accept, request and stop path is unchanged."""
        p = self.child(profile="tiny", api024="control")
        s = p.connect()
        p.wait("connected", peer=0)
        p.send(s, tb.F01)
        self.assertEqual(p.read(s, len(tb.DIAG)), tb.DIAG)
        p.wait("request", peer=0)
        p.wait("ended", peer=0, reason=DRAINED)
        x = p.inspect()
        self.assertEqual((x["accept_attempts"], x["constructed"], x["http_peers"]), (1, 1, 1))
        s.close()
        p.sockets.remove(s)
        p.wait("removed", peer=0)
        p.stop()  # stop_drained exactly once is the probe's own must()

    # -- r3 rows --------------------------------------------------------------------
    def test_a1_operational_hold_recovery(self):
        p = self.child(profile="tiny", api024="row", sh=[OPERATIONAL])
        self.checkpoint0(p)
        p.command("clock", ns=ms(1020))
        p.connect()
        held = self.until(p, "operational", 1)
        self.record(held, HTTP, phase=HELD, hold_deadline=ms(1070), interest=0,
                    operational=1, no_pending=0, script_cursor=1)
        self.record(held, NATIVE, phase=CLEAR, interest=1, operational=0, no_pending=0)
        self.inspect(held, accept_attempts=1, peers=0, constructed=0, transient_fds=0)
        self.same(held, self.turn(p, ms(1069)))
        p.connect()  # stale readiness while held: kernel backlog, no callback, no attempt
        self.same(held, self.turn(p, ms(1069)))
        x = self.turn(p, ms(1070))
        self.record(x, HTTP, phase=CLEAR, hold_deadline=0, interest=1, operational=1)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        # Recovery alone does no peer work. The retained backlog connection may already be
        # accepted by the next readiness (advisor V5a); the later peer is stated explicitly.
        self.assertIn((x["inspect"]["accept_attempts"], x["inspect"]["http_peers"]),
                      ((1, 0), (2, 1)))
        p.wait("connected", peer=0)
        x = self.until(p, "accept_attempts", 2)
        self.inspect(x, http_peers=1, constructed=1, transient_fds=0)
        self.record(x, HTTP, phase=CLEAR, interest=1, operational=1, no_pending=0,
                    script_cursor=1)
        p.stop()

    def test_a2_no_pending_then_operational_jump(self):
        p = self.child(profile="tiny", api024="row", sh=[NO_PENDING, OPERATIONAL])
        self.checkpoint0(p)
        p.command("clock", ns=ms(1020))
        p.connect()
        x = self.until(p, "no_pending", 1)
        self.record(x, HTTP, phase=HELD, hold_deadline=ms(1070), interest=0, operational=0)
        x = self.turn(p, ms(1070))
        self.record(x, HTTP, phase=CLEAR, interest=1)
        self.inspect(x, accept_attempts=1)
        p.connect()
        x = self.until(p, "operational", 1)
        self.record(x, HTTP, phase=HELD, hold_deadline=ms(1120), interest=0, no_pending=1)
        x = self.turn(p, ms(1220))
        self.record(x, HTTP, phase=CLEAR, hold_deadline=0, interest=1)
        self.inspect(x, accept_attempts=2, peers=0)
        p.connect()
        p.wait("connected", peer=0)
        x = self.until(p, "accept_attempts", 3)
        self.inspect(x, http_peers=1, constructed=1, transient_fds=0)
        self.record(x, HTTP, phase=CLEAR, interest=1, no_pending=1, operational=1,
                    script_cursor=2)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        p.stop()

    def test_a3_held_keeps_peer_service(self):
        p = self.child(profile="tiny", api024="row", sh=[PASS, OPERATIONAL], wire_gate=True)
        self.checkpoint0(p)
        h = p.connect()
        p.wait("connected", peer=0)
        p.command("clock", ns=ms(1001))
        n = p.connect(True)
        p.wait("connected", peer=1)
        p.send(n, tb.UPGRADE)
        self.assertEqual(p.read(n, len(tb.SWITCH)), tb.SWITCH)
        p.wait("opened", peer=1)
        p.command("clock", ns=ms(1020))
        p.connect()
        x = self.until(p, "operational", 1)
        self.record(x, HTTP, phase=HELD, hold_deadline=ms(1070), interest=0, script_cursor=2)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        self.inspect(x, accept_attempts=3, http_peers=1, native_peers=1, constructed=2,
                     transient_fds=0)
        p.command("clock", ns=ms(1021))
        p.command("post", peer=0)
        p.wait("post")
        p.wait("ready", peer=0)
        p.send(h, tb.F01)
        p.wait("request", peer=0)
        x = self.until(p, "held_application_wire", 1)
        self.inspect(x, output_slots=1)
        p.command("clock", ns=ms(1022))
        p.command("api024-release-wire")
        self.assertEqual(p.read(h, len(tb.DIAG)), tb.DIAG)
        p.wait("ended", peer=0, reason=DRAINED)
        p.send(n, tb.BINARY_A)
        p.wait("message", peer=1)
        x = self.observe(p)
        self.record(x, HTTP, phase=HELD, hold_deadline=ms(1070), interest=0)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        self.inspect(x, http_peers=1, native_peers=1, transient_fds=0, accept_attempts=3,
                     wire_releases=1)
        self.assertFalse(any(e["event"] == "removed" for e in p.events), "removal before 1072")
        # Idle deadline 1072 from raw release at 1022: still open after an owner turn at 1071.
        self.turn(p, ms(1071))
        self.assertFalse(any(e["event"] == "removed" for e in p.events), "idle before 1072")
        p.command("clock", ns=ms(1072))
        p.wait("removed", peer=0)
        p.stop()

    def test_a4_p2_capacity_and_hold(self):
        p = self.child(profile="p2", api024="row", sh=[OPERATIONAL])
        self.checkpoint0(p)
        p.command("clock", ns=ms(1020))
        p.connect()
        x = self.until(p, "operational", 1)
        self.record(x, HTTP, phase=HELD, interest=0)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        p.command("clock", ns=ms(1021))
        n = p.connect(True)
        p.wait("connected", peer=0)
        x = self.observe(p)
        self.record(x, NATIVE, interest=0)
        self.record(x, HTTP, phase=HELD, interest=0)
        p.command("clock", ns=ms(1022))
        p.send(n, tb.UPGRADE)
        self.assertEqual(p.read(n, len(tb.SWITCH)), tb.SWITCH)
        p.wait("opened", peer=0)
        x = self.observe(p)
        self.record(x, NATIVE, interest=0)
        self.inspect(x, native_peers=1)
        x = self.turn(p, ms(1070))
        self.record(x, HTTP, phase=CLEAR, interest=1)
        self.record(x, NATIVE, interest=0)
        p.connect()
        p.wait("connected", peer=1)
        x = self.until(p, "accept_attempts", 3)
        self.record(x, HTTP, interest=0)
        self.record(x, NATIVE, interest=0)
        self.inspect(x, http_peers=1, native_peers=1, transient_fds=0, constructed=2)
        p.connect()  # readiness after promotion: both interests are off
        x = self.turn(p, ms(1070))
        self.inspect(x, accept_attempts=3, http_peers=1, native_peers=1)
        self.record(x, HTTP, script_cursor=1, interest=0)
        p.stop()

    def test_a5_stop_beats_hold(self):
        p = self.child(profile="tiny", api024="row", sh=[PASS, OPERATIONAL], wire_gate=True)
        self.checkpoint0(p)
        h = p.connect()
        p.wait("connected", peer=0)
        p.command("clock", ns=ms(1020))
        p.connect()
        self.until(p, "operational", 1)
        p.command("clock", ns=ms(1030))
        p.send(h, tb.F01)
        p.wait("request", peer=0)
        self.until(p, "held_application_wire", 1)
        p.command("api024-stop-pair", first=ms(1075), second=ms(1085), ns=ms(1070))
        self.until(p, "phase", STOPPED, role=HTTP)
        x = self.until(p, "phase", STOPPED, role=NATIVE)
        for role in (HTTP, NATIVE):
            self.record(x, role, phase=STOPPED, interest=0)
        self.record(x, HTTP, script_cursor=2, operational=1)
        self.inspect(x, accept_attempts=2, output_slots=1, held_application_wire=1)
        self.assertFalse(any(e["event"] == "ended" for e in p.events), "ended before 1075")
        p.command("clock", ns=ms(1074))
        p.barrier()
        x = self.observe(p)
        self.inspect(x, output_slots=1, held_application_wire=1)
        self.assertFalse(any(e["event"] == "ended" for e in p.events), "ended before 1075")
        p.command("clock", ns=ms(1075))
        p.wait("ended", peer=0, reason=STOPPED_END)
        p.wait("removed", peer=0)
        self.assertFalse(any(e["event"] == "stop_drained" for e in p.events),
                         "stop_drained before peer retirement")
        drained, final = self.finish_stopped(p)
        self.assertEqual(drained["inspect"]["accept_attempts"], 2)
        for role in (HTTP, NATIVE):
            self.record(final, role, phase=STOPPED, interest=0, acceptor_retirements=1,
                        listener_closes=1, idle_closes=1, final_removals=1)

    def source_fault(self, available, ns):
        p = self.child(profile="tiny", action="hold", api024="row", sh=[PASS, OPERATIONAL])
        self.checkpoint0(p)
        h = p.connect()
        p.wait("connected", peer=0)
        p.send(h, tb.F01)
        p.wait("request", peer=0)
        p.command("clock", ns=ms(1020))
        p.connect()
        x = self.until(p, "operational", 1)
        self.record(x, HTTP, phase=HELD, hold_deadline=ms(1070))
        self.inspect(x, accept_attempts=2)
        p.command("clock", ns=ns, available=available)
        p.wait("ended", peer=0, reason=ELAPSED_FAILED)
        x = self.observe(p)
        for role in (HTTP, NATIVE):
            self.record(x, role, interest=0)
        self.inspect(x, accept_attempts=2)
        x = self.turn(p, ms(1070))
        for role in (HTTP, NATIVE):
            self.record(x, role, interest=0, phase=CLEAR, hold_deadline=0)
        self.inspect(x, accept_attempts=2)
        p.stop()  # explicit stop keeps its drain authority

    def test_a6_source_fault_unavailable(self):
        self.source_fault(False, ms(1020))

    def test_a6_source_fault_regressed(self):
        self.source_fault(True, ms(1019))

    def test_a7_overflow_time_fence(self):
        p = self.child(profile="tiny", api024="row", sh=[OPERATIONAL])
        self.checkpoint0(p)
        p.command("clock", ns=MAX - 49_999_999)
        p.connect()
        x = self.until(p, "operational", 1)
        self.record(x, HTTP, phase=TIME_FENCED, hold_deadline=0, interest=0)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        self.inspect(x, accept_attempts=1, peers=0, constructed=0, transient_fds=0)
        x = self.turn(p, MAX)
        self.record(x, HTTP, phase=TIME_FENCED, interest=0)
        self.record(x, NATIVE, phase=CLEAR, interest=1)  # no global source latch
        p.stop()

    def test_a7b_max_deadline_valid(self):
        p = self.child(profile="tiny", api024="row", sh=[OPERATIONAL])
        self.checkpoint0(p)
        p.command("clock", ns=MAX - 50_000_000)
        p.connect()
        x = self.until(p, "operational", 1)
        self.record(x, HTTP, phase=HELD, hold_deadline=MAX, interest=0)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        x = self.turn(p, MAX - 1)
        self.record(x, HTTP, phase=HELD, hold_deadline=MAX, interest=0)
        x = self.turn(p, MAX)
        self.record(x, HTTP, phase=CLEAR, interest=1)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        self.inspect(x, accept_attempts=1, peers=0, transient_fds=0)
        p.stop()

    def test_a8_stop_before_startup_entry(self):
        p = self.cold(profile="tiny", api024="a8")
        p.wait("api024_a8_stopped")
        drained, final = self.finish_stopped(p)
        self.assertEqual((drained["inspect"]["accept_attempts"], drained["inspect"]["peers"]),
                         (0, 0))
        for role in (HTTP, NATIVE):
            self.record(final, role, interest_enables=0, poller_registrations=0,
                        final_removals=0, listener_closes=1, idle_closes=1,
                        acceptor_retirements=1, phase=STOPPED, interest=0)

    def test_a9_release_no_early_rearm(self):
        p = self.child(profile="tiny", action="retain-http", api024="row",
                       sh=[PASS, OPERATIONAL])
        self.checkpoint0(p)
        h = p.connect()
        p.wait("connected", peer=0)
        p.command("clock", ns=ms(1020))
        p.connect()
        self.until(p, "operational", 1)
        p.command("clock", ns=ms(1030))
        h.close()
        p.sockets.remove(h)
        p.wait("removed", peer=0)
        x = self.observe(p)
        self.inspect(x, http_peers=1, accept_attempts=2)
        self.assertGreater(x["inspect"]["handles"], 0)
        self.record(x, HTTP, phase=HELD, hold_deadline=ms(1070), interest=0, script_cursor=2)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        p.command("clock", ns=ms(1031))
        p.command("drop-http-handles", peer=0)
        x = self.until(p, "http_peers", 0)
        self.record(x, HTTP, phase=HELD, hold_deadline=ms(1070), interest=0)
        x = self.turn(p, ms(1031))
        self.record(x, HTTP, phase=HELD, interest=0)
        self.record(x, NATIVE, phase=CLEAR, interest=1)
        self.inspect(x, accept_attempts=2, http_peers=0)
        p.stop()

    # -- accepted R3 arm window and Correction 1b -----------------------------------
    def test_r3_arm_window_cold_then_second(self):
        self.window("window-cold")

    def test_r3_arm_window_seal_refuses_both(self):
        self.window("window-seal")

    def test_r3_arm_window_stop_refuses(self):
        self.window("window-stop")

    def test_r3_arm_window_io0_wrong_thread(self):
        p = self.child(profile="tiny", api024="window-io0")
        p.command("api024-arm-from-io0")
        p.stop()


def main():
    tb.require(sys.flags.optimize == 0, "ADMISSION: optimized Python is forbidden")
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--admission", type=Path, required=True)
    parser.add_argument("selectors", nargs="*")
    args = parser.parse_args()
    tb.PROBE = args.probe.resolve()
    tb.require(args.admission.stat().st_size <= 65536, "ADMISSION: manifest bound")
    tb.ADMISSION = tb.admission(
        json.loads(args.admission.read_text(), object_pairs_hook=tb.closed_object), tb.PROBE)
    loader = unittest.defaultTestLoader
    if args.selectors:
        available = set(loader.getTestCaseNames(ListenerTests))
        tb.require(len(args.selectors) == len(set(args.selectors)) <= 16 and
                   set(args.selectors) <= available, "FIXTURE: exact selected method inventory")
        suite = unittest.TestSuite(ListenerTests(name) for name in args.selectors)
    else:
        suite = loader.loadTestsFromTestCase(ListenerTests)
    result = unittest.TextTestRunner(verbosity=2, failfast=False).run(suite)
    raise SystemExit(not result.wasSuccessful())


if __name__ == "__main__":
    main()
