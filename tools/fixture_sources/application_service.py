# SPDX-License-Identifier: GPL-3.0-or-later
"""Named independent service DATA and public framing; no policy or SQL evaluation.

Expected arithmetic, budgets, query/snapshot values and historical audit/hold bytes
are independently supplied literals. Packers only spell accepted public bytes of
already supplied commands, snapshots, effects and synthetic jobs.
"""
import struct

from .bundle import insert, statement
from .common import blob, i64, integer, load_seeds, null, sized, text, u64


COMMAND_FORMAT = "subathon.internal.application-command.v1"
STATE_FORMAT = "subathon.internal.application-state.v1"
RESULT_FORMAT = "subathon.internal.application-result.v1"
JOB_FORMAT = "subathon.internal.application-job-synthetic.v1"


def _byte(value):
    return bytes((int(value),))


def _bytes(value):
    return value if isinstance(value, bytes) else value.encode("utf-8")


def _b(value):
    return sized(_bytes(value))


def _signed(value):
    return i64(int(value))


def _unsigned(value):
    return u64(int(value))


def _optional(value, pack):
    return b"\x00" if value is None else b"\x01" + pack(value)


def _vector(rows, pack):
    return struct.pack(">I", len(rows)) + b"".join(pack(row) for row in rows)


def _env(format_ref, document):
    return {"format": format_ref, "document_hex": document.hex()}


def _env_bytes(value):
    return _b(value["format"]) + _b(bytes.fromhex(value["document_hex"]))


def _sac(command):
    """Accepted SAC1 field order; no field is omitted as metadata."""
    principal = command["principal"]
    return (b"SAC1" + _b(command["operation"][0]) + _b(command["operation"][1])
            + _byte(command["subject"]) + _unsigned(command["receipt"])
            + _optional(command["cid"], _unsigned)
            + _b(command["plan"][0]) + _unsigned(command["plan"][1])
            + _byte(principal["kind"]) + _b(principal["installation"])
            + _b(principal["principal"]) + _byte(command["purpose"])
            + _byte(command["preview_mode"]) + _optional(command["preview"], _b)
            + _optional(command["actor"], _b) + _optional(command["decision"], _b)
            + _optional(command["reason"], _b) + _vector(command["choices"], _env_bytes))


def _clock_bytes(clock):
    def segment(row):
        return _signed(row["elapsed"]) + _signed(row["utc"]) + _byte(row["paused"])

    return (_b(clock["lineage"]) + _b(clock["observation"]) + _signed(clock["utc"])
            + _vector(clock["segments"], segment))


def _timer_bytes(timer):
    return (_byte(timer["lifecycle"]) + _byte(timer["phase"])
            + _signed(timer["main"]) + _signed(timer["grace"]) + _byte(timer["wait"]))


def _balance_bytes(row):
    return _byte(row["kind"]) + _b(row["target"]) + _signed(row["units"])


def _sas(state):
    return (b"SAS1" + _b(state["configuration"]) + _optional(state["basis"], _b)
            + _clock_bytes(state["observation"]) + _b(state["policy"])
            + _timer_bytes(state["timer"]) + _signed(state["debt"])
            + _vector(state["balances"], _balance_bytes))


def _component_bytes(row):
    return _byte(row["kind"]) + _signed(row["units"]) + _optional(row["target"], _b)


def _effect_bytes(effect):
    return (_signed(effect["ordinal"]) + _byte(effect["role"]) + _byte(effect["unit"])
            + _b(effect["exact"][0]) + _b(effect["exact"][1]) + _signed(effect["final"])
            + _b(effect["residual"][0]) + _b(effect["residual"][1])
            + _optional(effect["cap"], _signed)
            + _optional(effect["deficit_policy"], _byte)
            + _optional(effect["deficit_disposition"], _byte)
            + _b(effect["explanation"]) + _vector(effect["components"], _component_bytes))


def _job_bytes(job):
    return (_b(job["ref"]) + _optional(job["source"], _signed)
            + _optional(job["group"], _b) + _optional(job["group_ordinal"], _signed)
            + _env_bytes(job["envelope"]))


def _sar(command, head, before, after, revalidated, effects, jobs, transitions):
    return (b"SAR1" + _b(_sac(command)) + _b(head) + _b(before) + _b(after)
            + _byte(revalidated) + _vector(effects, _effect_bytes)
            + _vector(jobs, _job_bytes) + _vector(transitions, _byte))


def _saj(ordinal, request, components, synthetic_action, payload):
    """Non-deliverable SAJ1 DATA; requested units/components are literal inputs."""
    return (b"SAJ1" + _signed(ordinal) + _b(request["action"]) + _b(request["version"])
            + _b(request["target"]) + _byte(request["unit"]) + _signed(request["final"])
            + _vector(components, _component_bytes) + _b(synthetic_action) + _b(payload))


def _command(operation, *, subject=0, receipt="7", cid="0", plan=("PA", "1"),
             principal_kind=0, installation="fixture-installation", principal="fixture-user",
             purpose=0, preview_mode=0, preview="a", actor=None, decision=None, reason=None,
             choices=None):
    if choices is None:
        choices = [{"format": "synthetic.choice.v1", "document_hex": "efbbbf00ff41"}]
    return {"operation": list(operation), "subject": subject, "receipt": receipt, "cid": cid,
            "plan": list(plan), "principal": {"kind": principal_kind,
            "installation": installation, "principal": principal}, "purpose": purpose,
            "preview_mode": preview_mode, "preview": preview, "actor": actor,
            "decision": decision, "reason": reason, "choices": list(choices)}


def _timer(main, *, lifecycle=1, phase=0, grace="90", wait=False):
    return {"lifecycle": lifecycle, "phase": phase, "main": main, "grace": grace, "wait": wait}


def _clock(*, observation="observation", utc="1000", segments=None,
           policy="policy", head="a", configuration="cfg", lineage="lineage"):
    if segments is None:
        segments = [{"elapsed": "0", "utc": "1000", "paused": False}]
    return {"lineage": lineage, "observation": observation, "configuration": configuration,
            "policy": policy, "head": head, "utc": utc, "segments": list(segments)}


def _state(token, timer, debt, balances, observation, policy, basis, configuration="cfg"):
    row = {"token": token, "configuration": configuration, "timer": timer, "debt": debt,
           "balances": balances, "observation": observation, "policy": policy, "basis": basis}
    row["envelope"] = _env(STATE_FORMAT, _sas(row))
    return row


def _record(command, head, before, after, advance, contribution, effects, jobs,
            transitions, audit, oracles, revalidated):
    """Assemble supplied outcomes; there is no calculation from requested inputs."""
    return {"command": command, "command_envelope": _env(COMMAND_FORMAT, _sac(command)),
            "head": head, "before": before, "after": after, "advance_explanation": advance,
            "expected_contribution": contribution, "effects": effects, "jobs": jobs,
            "transitions": transitions, "result": _env(RESULT_FORMAT, _sar(
                command, head["token"], before["token"], after["token"], revalidated,
                effects, jobs, transitions)), "audit": audit, "oracles": oracles}


def build(seed_root):
    raise ValueError("FeatureNotImplemented: application_service fixture recipe")
