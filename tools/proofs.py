#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reviewed MIGRATE-002 interface seams; deliberately no proof implementation."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys


def read_json(path: Path) -> object:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def discover_records(directory: Path) -> list[Path]:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def validate_policy(policy: object) -> None:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def validate_record(record: object) -> None:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def validate(policy: object, records: list[dict]) -> None:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def require_complete(bundle: dict, task_id: str) -> None:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def load(policy_path: Path, records_dir: Path) -> dict:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def context(bundle: dict, task_id: str, *, stage: str | None = None,
            compact: bool = False) -> dict:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def convert(source: dict, identities: dict, archive_commit: str) -> dict:
    raise ValueError('PROOF_RECORDS_NOT_IMPLEMENTED')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    for command in ('validate', 'context'):
        sub = commands.add_parser(command)
        if command == 'context':
            sub.add_argument('task_id')
            sub.add_argument('--stage')
            sub.add_argument('--compact', action='store_true')
        sub.add_argument('--policy', type=Path, required=True)
        sub.add_argument('--records-dir', type=Path, required=True)
    sub = commands.add_parser('convert')
    for name in ('source', 'source-root', 'identities', 'output-dir'):
        sub.add_argument('--' + name, type=Path, required=True)
    sub.add_argument('--archive-commit', required=True)
    parser.parse_args()
    print('Proof error: PROOF_RECORDS_NOT_IMPLEMENTED', file=sys.stderr)
    return 1


if __name__ == '__main__':
    sys.exit(main())
