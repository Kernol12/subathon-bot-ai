# SPDX-License-Identifier: GPL-3.0-or-later
"""Small independent byte/framing primitives; no product or SQL-engine calls."""
import hashlib
import json
from pathlib import Path
import struct


SEEDS = {
    "retained_context_vectors.json":
        "631260c6050c6e47c36ef69fbf6fecf67016407b7f9cf93d6b3246b9438f1dcf",
    "sqlite_receipts_vectors.json":
        "350e2f22db39661c7966996f8d7ae5da12fad91b929605280ff7930d1bec7e8a",
    "sqlite_contributions_vectors.json":
        "86eadfdf53367e7b3d7ae27f1885483aad9e8183a6b759e40ad526ae1d33ff23",
    "sqlite_application_plans_vectors.json":
        "9a603917d75019b3fe18c8dd6e7dbb05386d48f64b90ee340f5d2d66d21b968a",
}


def load_seeds(directory):
    """Pin the four approved inputs, then expose only their literal manifest tuples."""
    raw = {}
    for name, expected in SEEDS.items():
        path = Path(directory) / name
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError(f"Seed fingerprint mismatch: {path}")
        raw[name] = data
    steps = []
    for data in raw.values():
        source = json.loads(data)["manifest"]
        steps.append({key: source[key]
                      for key in ("version", "id", "sql", "expected_schema")})
    return steps


def schema_key(row):
    fields = [row[key].encode("utf-8") for key in ("type", "name", "table_name")]
    fields.append(b"0" if row["sql"] is None else b"1")
    if row["sql"] is not None:
        fields.append(row["sql"].encode("utf-8"))
    return tuple(fields)


def migration_payload(step):
    """Migration-catalog R1 F-framing over independently declared literal tuples."""
    def field(value):
        data = str(value).encode("utf-8")
        return str(len(data)).encode("ascii") + b":" + data

    parts = [field("migration-catalog-v1"), field(step["version"]), field(step["id"]),
             field(len(step["sql"]))]
    parts.extend(field(sql) for sql in step["sql"])
    parts.append(field(len(step["expected_schema"])))
    for row in step["expected_schema"]:
        parts.extend(field(row[key]) for key in ("type", "name", "table_name"))
        parts.append(field("0" if row["sql"] is None else "1"))
        if row["sql"] is not None:
            parts.append(field(row["sql"]))
    return b"".join(parts)


def manifest(step):
    payload = migration_payload(step)
    return {**step, "payload_bytes": len(payload),
            "sha256_hex": hashlib.sha256(payload).hexdigest()}


def u64(value):
    return struct.pack(">Q", value)


def i64(value):
    return struct.pack(">q", value)


def sized(data):
    return struct.pack(">I", len(data)) + data


def blob(value):
    return {"type": "blob", "hex": value.hex()}


def integer(value):
    return {"type": "integer", "decimal": str(value)}


def null():
    return {"type": "null"}


def text(value):
    return {"type": "text", "value": value}


def render(value):
    def reject_floats(item):
        if isinstance(item, float):
            raise ValueError("Fixture source must not contain floating-point values")
        if isinstance(item, dict):
            for key, member in item.items():
                reject_floats(key)
                reject_floats(member)
        elif isinstance(item, (list, tuple)):
            for member in item:
                reject_floats(member)

    reject_floats(value)
    return (json.dumps(value, ensure_ascii=False, allow_nan=False, indent=2) + "\n").encode("utf-8")
