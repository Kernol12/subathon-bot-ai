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
import progress
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

    def test_default_repository_is_independent_of_tracker_and_retired_flag_is_usage(self):
        with tempfile.TemporaryDirectory() as directory:
            f=PublicationFixture(Path(directory)/'candidate')
            status=f.root/'docs/STATUS.md'
            self.assertEqual([line for line in status.read_text().splitlines()
                              if line.startswith('Specification version:')],
                             ['Specification version: 0.45'])
            good=self.cli(f); self.assertEqual(good.returncode,0,good.stderr)
            # Unreadable obsolete inputs are sentinels for accidental fallback, not
            # assertions about whether deleted source files or buttons exist.
            f.write('progress/state.json',b'not JSON\n'); f.write('PROGRESS.html',b'not HTML\n')
            before=f.snapshot()
            good=self.cli(f)
            self.assertEqual(good.returncode,0,good.stderr)
            self.assertEqual(f.snapshot(),before)
            usage=self.cli(f,'--repository-only')
            self.assertEqual((usage.returncode,usage.stdout),(2,b''))
            self.assertIn(b'usage:',usage.stderr)

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
                messages={'reference':'unknown specification reference',
                          'version':'Progress/specification version mismatch',
                          'render':'Stale progress snapshot', 'asset':'must not need external assets',
                          'anchor':'Missing HTML anchor', 'link':'Missing HTML link'}
                if mutation in ('reference','version'):
                    if mutation=='reference': data['tasks'][0]['spec_refs']=['MISSING-01']
                    else: data['specification_version']='0.44'
                    f.write('PROGRESS.html',progress.render(data).encode())
                elif mutation=='render': f.write('PROGRESS.html',b'stale\n')
                else:
                    # Keep currency exact so malformed generated HTML reaches its own checks.
                    from unittest.mock import patch
                    html={'asset':'<img src="https://example.test/image">',
                          'anchor':'<a href="#missing">x</a>',
                          'link':'<a href="missing-target.md">x</a>'}[mutation]
                    f.write('PROGRESS.html',html.encode())
                    with patch.object(check_project,'render',return_value=html), \
                         patch.object(progress,'render',return_value=html):
                        with self.assertRaises(ValueError) as error:
                            check_project.check_tracker(data,f.root)
                    self.assertNotIn(NI.decode(),str(error.exception))
                    self.assertIn(messages[mutation],str(error.exception)); continue
                with self.assertRaises(ValueError) as error:
                    check_project.check_tracker(data,f.root)
                self.assertNotIn(NI.decode(),str(error.exception))
                self.assertIn(messages[mutation],str(error.exception))


if __name__=='__main__':
    unittest.main()
