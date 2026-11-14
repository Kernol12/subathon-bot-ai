# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only proof CLI fixture. Authored only; independent review precedes use."""
from contextlib import redirect_stdout, redirect_stderr
from io import StringIO
import hashlib
import json
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import urllib.request
from unittest.mock import patch
import proofs


def invoke_context(case, bundle, command):
    """Preserve the former invoke129 bytes guard and workflow process/network guards."""
    with tempfile.TemporaryDirectory() as directory:
        root=Path(directory)
        policy=root/'progress/proof-policy.json'; records=root/'progress/proofs'; records.mkdir(parents=True)
        policy.write_text(json.dumps(bundle['policy']),encoding='utf-8')
        for identity,value in bundle['records'].items():
            (records/(identity+'.json')).write_text(json.dumps(value,ensure_ascii=False),encoding='utf-8')
        (root/'PROGRESS.html').write_bytes(b'Preserve this HTML exactly.\n')
        (root/'coordinator-checkpoint.md').write_bytes(b'Original authorization checkpoint')
        def snapshot():
            return {p.relative_to(root):p.read_bytes() if p.is_file() else None for p in root.rglob('*')}
        before=snapshot()
        source=Path(__file__).resolve().parents[2]
        real_paths=[source/p for p in ('tools/proofs.py','docs/specification.md','docs/STATUS.md')]
        real_before=[hashlib.sha256(p.read_bytes()).digest() for p in real_paths]
        out,err=StringIO(),StringIO()
        args=list(command)
        with patch.object(proofs,'ROOT',root), redirect_stdout(out),redirect_stderr(err), \
             patch.object(subprocess,'run',side_effect=AssertionError('unexpected process')), \
             patch.object(subprocess,'Popen',side_effect=AssertionError('unexpected process')), \
             patch.object(socket,'socket',side_effect=AssertionError('unexpected network')), \
             patch.object(urllib.request,'urlopen',side_effect=AssertionError('unexpected network')):
            try:
                code=proofs.main(args)
            except SystemExit as error:
                code=error.code
        after=snapshot()
        case.assertEqual(after,before)
        case.assertEqual(real_before,[hashlib.sha256(p.read_bytes()).digest() for p in real_paths])
        return code,out.getvalue(),err.getvalue(),before,after
