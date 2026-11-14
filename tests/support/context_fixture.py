# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only proof CLI fixture. Authored only; independent review precedes use."""
from contextlib import redirect_stdout, redirect_stderr
from io import StringIO
import json
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import urllib.request
from unittest.mock import patch
import proofs


def invoke_context(case, bundle, command, *, prepare=None):
    """Preserve the former invoke129 bytes guard and workflow process/network guards."""
    with tempfile.TemporaryDirectory() as directory:
        root=Path(directory)
        policy=root/'progress/proof-policy.json'; records=root/'progress/proofs'; records.mkdir(parents=True)
        policy.write_text(json.dumps(bundle['policy']),encoding='utf-8')
        for identity,value in bundle['records'].items():
            (records/(identity+'.json')).write_text(json.dumps(value,ensure_ascii=False),encoding='utf-8')
        (root/'PROGRESS.html').write_bytes(b'Preserve this HTML exactly.\n')
        (root/'progress/coordinator-checkpoint.md').write_bytes(b'Original authorization checkpoint')
        if prepare is not None:
            prepare(root)
        def snapshot():
            return {p.relative_to(root):p.read_bytes() if p.is_file() else None for p in root.rglob('*')}
        before=snapshot()
        source=Path(__file__).resolve().parents[2]
        def real_snapshot():
            watched=[source/'progress/proof-policy.json',
                     source/'progress/proofs', source/'progress/coordinator-checkpoint.md']
            records=source/'progress/proofs'
            if records.is_dir() and not records.is_symlink():
                watched.extend(records.rglob('*'))
            result={}
            for path in watched:
                if path.is_symlink(): value=('symlink',str(path.readlink()))
                elif not path.exists(): value=('absent',)
                elif path.is_dir(): value=('directory',)
                else: value=('file',path.read_bytes())
                result[path.relative_to(source)]=value
            return result
        real_before=real_snapshot()
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
        case.assertEqual(real_before,real_snapshot())
        return code,out.getvalue(),err.getvalue(),before,after
