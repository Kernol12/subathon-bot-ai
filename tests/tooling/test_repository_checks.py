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
        result=self.cli(f,'--repository-only')
        self.assertEqual((result.returncode,result.stdout,result.stderr),(0,b'PASS: repository checks\n',b''))
        self.assertEqual(check_project.check_repository(f.root),dict(decisions=122,time_credit_declarations=19))

    def refusal(self,result):
        self.assertEqual((result.returncode,result.stdout),(1,b''))
        self.assertIn(b'Project check failed:',result.stderr)
        self.assertNotIn(NI,result.stderr)

    def test_repository_only_never_reads_tracker_or_render(self):
        with tempfile.TemporaryDirectory() as directory:
            f=PublicationFixture(Path(directory)/'candidate')
            self.positive(f)
            self.assertEqual(check_project.check_tracker(f.data,f.root),dict(tasks=1))
            good=self.cli(f); self.assertEqual(good.returncode,0,good.stderr)
            f.write('progress/state.json',b'broken JSON\n'); f.write('PROGRESS.html',b'broken HTML\n')
            self.positive(f)
            self.refusal(self.cli(f))
            with self.assertRaises(ValueError) as error:
                check_project.check_tracker(f.data,f.root)
            self.assertNotIn(NI.decode(),str(error.exception))

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
                self.refusal(self.cli(f,'--repository-only')); self.refusal(self.cli(f))

    def test_tracker_checks_preserve_spec_refs_version_render_assets_anchors_links(self):
        for mutation in ('reference','version','render','asset','anchor','link'):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as directory:
                f=PublicationFixture(Path(directory)/'candidate')
                self.assertEqual(check_project.check_tracker(f.data,f.root),dict(tasks=1))
                data=deepcopy(f.data)
                if mutation=='reference': data['tasks'][0]['spec_refs']=['MISSING-01']
                elif mutation=='version': data['specification_version']='0.44'
                elif mutation=='render': f.write('PROGRESS.html',b'stale\n')
                else:
                    # Keep currency exact so malformed generated HTML reaches its own checks.
                    from unittest.mock import patch
                    html={'asset':'<img src="https://example.test/image">',
                          'anchor':'<a href="#missing">x</a>',
                          'link':'<a href="missing-target.md">x</a>'}[mutation]
                    f.write('PROGRESS.html',html.encode())
                    with patch.object(check_project,'render',return_value=html):
                        with self.assertRaises(ValueError) as error:
                            check_project.check_tracker(data,f.root)
                    self.assertNotIn(NI.decode(),str(error.exception)); continue
                with self.assertRaises(ValueError) as error:
                    check_project.check_tracker(data,f.root)
                self.assertNotIn(NI.decode(),str(error.exception))


if __name__=='__main__':
    unittest.main()
