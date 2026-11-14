# SPDX-License-Identifier: GPL-3.0-or-later
"""MIGRATE-003 independent repository/tracker split. Authored, not executed."""
from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
sys.path.insert(0,str(ROOT/'tests/support'))
import check_project
from publication_fixture import PublicationFixture, ENV

NI=b'PROOF_PUBLICATION_NOT_IMPLEMENTED'


class RepositoryChecks(unittest.TestCase):
    def cli(self,f,*options):
        return subprocess.run([sys.executable,'-E','-s','-B','tools/check_project.py',*options],
                              cwd=f.root,env=ENV,capture_output=True,timeout=120)

    def positive(self,f):
        result=self.cli(f)
        self.assertEqual((result.returncode,result.stdout,result.stderr),(0,b'PASS: repository checks\n',b''))
        self.assertEqual(check_project.check_repository(f.root),dict(decisions=122,time_credit_declarations=19))

    def refusal(self,result):
        self.assertEqual((result.returncode,result.stdout),(1,b''))
        self.assertIn(b'Project check failed:',result.stderr)
        self.assertNotIn(NI,result.stderr)
        self.assertNotIn(b'CUTOVER_',result.stderr)

    def test_status_version_declaration(self):
        for declaration in (None, 'Specification version: 0.44\n',
                            'Specification version: 0.45\nSpecification version: 0.45\n',
                            'Specification version: 0.45 trailing prose\n'):
            with self.subTest(declaration=declaration), tempfile.TemporaryDirectory() as directory:
                f=PublicationFixture(Path(directory)/'candidate')
                status=f.root/'docs/STATUS.md'
                original=status.read_text()
                lines=[line for line in original.splitlines() if not line.startswith('Specification version:')]
                status.write_text('\n'.join(lines)+'\nSpecification version: 0.45\n')
                good=self.cli(f)
                self.assertEqual(good.returncode,0,good.stderr)
                # Version capture excludes trailing specification prose.
                spec=f.root/'docs/specification.md'
                spec.write_text(spec.read_text().replace('Version: 0.45','Version: 0.45 explanatory prose',1))
                good=self.cli(f); self.assertEqual(good.returncode,0,good.stderr)
                status.write_text('\n'.join(lines)+'\n'+(declaration or ''))
                before=f.snapshot()
                self.refusal(self.cli(f))
                self.assertEqual(f.snapshot(),before)


    def test_repository_only_never_reads_tracker_or_render(self):
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as directory:
            f=PublicationFixture(Path(directory)/'candidate')
            self.positive(f)
            f.write('progress/state.json',b'broken JSON\n'); f.write('PROGRESS.html',b'broken HTML\n')
            before=f.snapshot()
            with patch.object(check_project, 'ROOT', f.root):
                self.positive(f)
            self.assertEqual(f.snapshot(),before)
            usage=self.cli(f,'--repository-only')
            self.assertEqual((usage.returncode,usage.stdout),(2,b''))
            self.assertIn(b'usage:',usage.stderr)

    def test_repository_checks_preserve_each_artifact_refusal(self):
        for mutation in ('decision','map','test-path','evidence-link','evidence-layout','preset','markdown'):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as directory:
                f=PublicationFixture(Path(directory)/'candidate'); self.positive(f)
                good=self.cli(f); self.assertEqual(good.returncode,0,good.stderr)
                if mutation=='decision':
                    path=f.root/'docs/specification.md'; path.write_text(path.read_text()+'\n| DUP-01 | a |\n| DUP-01 | b |\n')
                elif mutation in ('map','test-path','evidence-link'):
                    path=f.root/'tests/acceptance-map.json'; value=json.loads(path.read_text())
                    if mutation=='map': value['specification_version']='0.44'
                    elif mutation=='test-path': value['decisions'][0]['tests']=['tests/missing.cpp']
                    else: value['decisions'][0]['evidence']=['docs/evidence/missing.md']
                    path.write_text(json.dumps(value))
                elif mutation=='evidence-layout': f.write('docs/evidence/extra.md')
                elif mutation=='preset':
                    path=f.root/'CMakePresets.json'; value=json.loads(path.read_text())
                    value['buildPresets'][0]['configurePreset']='missing-preset'; path.write_text(json.dumps(value))
                else: f.write('docs/broken.md',b'[missing](missing-target.md)\n')
                result=self.cli(f); self.refusal(result)
                diagnostics={'decision':b'Duplicate specification decision IDs',
                    'map':b'Acceptance map must track every current decision exactly once',
                    'test-path':b'Missing acceptance-map test artifact:',
                    'evidence-link':b'Missing acceptance-map evidence:',
                    'evidence-layout':b'docs/evidence must contain only README.md',
                    'preset':b'Preset references an unknown configuration',
                    'markdown':b'Broken Markdown link'}
                self.assertIn(diagnostics[mutation],result.stderr)



if __name__=='__main__':
    unittest.main()
