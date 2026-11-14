#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""MIGRATE-003 opt-in real admitted SDK integration. Review before execution.

Run with admitted literal venv path and -I -S -B. Setup/admission errors are never
intended red. No installs, global SDK fallback, tokens, board or network access.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/support'))
from publication_fixture import ENV
from proof_publication_fixture import ProofPublicationFixture, SELECTED

WITNESS=b'{"tests":1,"failures":0,"errors":0,"skipped":0}\n'
NI=b'PROOF_PUBLICATION_NOT_IMPLEMENTED'
PROBE=r'''
import importlib.util
import sys
import unittest
spec=importlib.util.spec_from_file_location('independent_proof_probe',sys.argv[1])
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
suite=unittest.TestLoader().loadTestsFromName('ProofSchemas.test_proof_and_policy_contract',module)
result=unittest.TextTestRunner(verbosity=2).run(suite)
if (result.testsRun!=1 or result.failures or result.errors or result.skipped
    or result.expectedFailures or result.unexpectedSuccesses):
    raise SystemExit(1)
sys.stdout.write('{"tests":1,"failures":0,"errors":0,"skipped":0}\n')
'''


class ProofPublicationSDKTests(unittest.TestCase):
    def probe(self,f):
        before=f.snapshot()
        result=subprocess.run([os.path.abspath(sys.executable),'-I','-S','-B','-c',PROBE,
                               'tests/schema_validation/test_proof_schemas.py'],cwd=f.root,
                              env=ENV,capture_output=True,timeout=120)
        self.assertEqual(f.snapshot(),before)
        return result

    def checker(self,f,executable=None):
        before=f.snapshot()
        result=subprocess.run([os.path.abspath(sys.executable),'-E','-s','-B','tools/check_publication.py',
            *[a for a in f.arguments(schema_python=os.path.abspath(sys.executable) if executable is None else executable)
              if a != '--proofs']],
            cwd=f.root,env=ENV,capture_output=True,timeout=120)
        self.assertEqual(f.snapshot(),before)
        return result

    def positive(self,f):
        probe=self.probe(f)
        self.assertEqual((probe.returncode,probe.stdout),(0,WITNESS),
                         'SDK/admission/setup failure is NOT meaningful red: '+probe.stderr.decode(errors='replace'))
        result=self.checker(f)
        self.assertEqual(result.returncode,0,result.stderr.decode(errors='replace'))
        summary=json.loads(result.stdout)
        self.assertEqual(summary['checks'],dict.fromkeys(
            ('recorded_paths','outgoing_paths','proofs','policy_history','project','proof_schema'),'passed'))
        self.assertEqual((summary['head'],summary['commit_count']),(f.head,1))

    def python_controls(self,f):
        for command in (['tools/proofs.py','validate'], ['tools/check_project.py']):
            before=f.snapshot()
            result=subprocess.run([os.path.abspath(sys.executable),'-E','-s','-B',*command],
                                  cwd=f.root,env=ENV,capture_output=True,timeout=120)
            self.assertEqual(f.snapshot(),before)
            self.assertEqual((result.returncode,result.stderr),(0,b''),result.stderr)

    def refuse(self,result):
        self.assertEqual((result.returncode,result.stdout),(1,b''))
        self.assertTrue(result.stderr.strip())
        self.assertNotIn(NI,result.stderr)
        self.assertNotIn(b'CUTOVER_',result.stderr)

    def test_actual_canonical_inputs_refuse_after_real_sdk_passing_control(self):
        for mutation in ('missing-policy','missing-record','malformed-unselected','bad-policy'):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as directory:
                f=ProofPublicationFixture(Path(directory)/'candidate'); self.positive(f)
                if mutation=='missing-policy': (f.root/'progress/proof-policy.json').unlink()
                elif mutation=='missing-record': (f.root/'progress/proofs/OLD-902.json').unlink()
                elif mutation=='malformed-unselected': f.write('progress/proofs/OLD-902.json',b'{bad JSON')
                else:
                    policy_path=f.root/'progress/proof-policy.json'
                    value=json.loads(policy_path.read_text()); value['version']=True
                    policy_path.write_text(json.dumps(value))
                probe=self.probe(f)
                self.assertEqual((probe.returncode,probe.stdout),(1,b''),probe.stderr)
                self.assertTrue(probe.stderr.strip())
                self.assertNotIn(b'CUTOVER_',probe.stderr)

    def test_aggregate_and_every_actual_canonical_record_both_required(self):
        for identity in (SELECTED,'OLD-902'):
            with self.subTest(identity=identity), tempfile.TemporaryDirectory() as directory:
                f=ProofPublicationFixture(Path(directory)/'candidate'); self.positive(f)
                path=f.root/'progress/proof.schema.json'; schema=json.loads(path.read_text())
                # Synthetic examples stay valid; the aggregate's actual-record arm refuses.
                schema.setdefault('allOf',[]).append({'not':{'properties':{'task_id':{'const':identity}},'required':['task_id']}})
                path.write_text(json.dumps(schema)); f.head=f.commit('Synthetic actual-record-only schema refusal')
                self.python_controls(f)
                probe=self.probe(f)
                self.assertEqual((probe.returncode,probe.stdout),(1,b''),probe.stderr)
                self.assertIn(b'ValidationError',probe.stderr)
                self.assertIn(identity.encode(),probe.stderr)
                result=self.checker(f); self.refuse(result)
                self.assertIn(b'ValidationError',result.stderr)
                self.assertIn(identity.encode(),result.stderr)

    def test_real_aggregate_skip_or_failure_never_counts_as_witness(self):
        for mutation in ('skip','assertion','extra-output','offline-ref'):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as directory:
                f=ProofPublicationFixture(Path(directory)/'candidate'); self.positive(f)
                if mutation=='offline-ref':
                    path=f.root/'progress/proof.schema.json'; schema=json.loads(path.read_text())
                    schema['$ref']='https://example.invalid/unavailable'; path.write_text(json.dumps(schema))
                else:
                    path=f.root/'tests/schema_validation/test_proof_schemas.py'; source=path.read_text()
                    if mutation=='extra-output': source += '\nprint("extra witness byte")\n'
                    else:
                        needle='    def test_proof_and_policy_contract(self):'
                        source=source.replace(needle,needle+'\n        '+(
                            'self.skipTest("synthetic skip")' if mutation=='skip' else 'self.fail("synthetic aggregate failure")'))
                    path.write_text(source)
                f.head=f.commit('Synthetic actual schema child mutation')
                self.refuse(self.checker(f))

    def test_missing_empty_and_non_sdk_interpreters_are_setup_refusals(self):
        with tempfile.TemporaryDirectory() as directory:
            f=ProofPublicationFixture(Path(directory)/'candidate'); self.positive(f)
            for executable in ('/bin/true',str(f.root/'missing-interpreter'),'/usr/bin/python3'):
                with self.subTest(executable=executable):
                    self.refuse(self.checker(f,executable))


if __name__=='__main__':
    unittest.main(verbosity=2)
