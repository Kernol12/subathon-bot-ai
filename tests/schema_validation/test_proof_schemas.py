#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""MIGRATE-002 actual-schema oracles, authored only until independent test review.

Run directly with the selected venv spelling and -I -S -B. Admission precedes SDK
imports. This separate aggregate leaves RepositorySchemas and its witness intact.
"""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ProofSchemas(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        location = ROOT/'tools/schema_validation/check_dependencies.py'
        spec = importlib.util.spec_from_file_location('proof_schema_dependencies', location)
        admission = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(admission)
        sdk = admission.check_sdk_metadata(ROOT)
        sys.path.append(str(sdk))
        from jsonschema import Draft202012Validator
        from jsonschema.exceptions import ValidationError
        from referencing import Registry
        from referencing.exceptions import NoSuchResource

        def deny(uri):
            raise NoSuchResource(ref=uri)

        cls.Draft = Draft202012Validator
        cls.Invalid = ValidationError
        cls.registry = Registry(retrieve=deny)
        # Share authored literal input mechanics, never production algorithms or a
        # candidate schema generator. No test bodies execute during module import.
        sys.path.insert(0, str(ROOT/'tests/tooling'))
        spec = importlib.util.spec_from_file_location('proof_oracles', ROOT/'tests/tooling/test_proofs.py')
        cls.examples = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.examples)

    def test_proof_and_policy_contract(self):
        """One real aggregate, suitable for 003's later exact one-test witness."""
        examples = self.examples
        schemas = [json.loads((ROOT/'progress'/name).read_text())
                   for name in ('proof.schema.json', 'proof-policy.schema.json')]
        for schema in schemas:
            self.assertEqual(schema['$schema'], 'https://json-schema.org/draft/2020-12/schema')
            self.Draft.check_schema(schema)
        proof_validator, policy_validator = [self.Draft(s, registry=self.registry) for s in schemas]
        positives = [examples.record(), examples.reviewed_record()]
        historical = examples.record('APITEST-016')
        historical['supplemental_workflow_evidence'] = examples.supplemental()
        positives.append(historical)
        observed = examples.record()
        observed.update(verification={'ci': 'failed', 'baseline': 'not_run'},
                        review={'author': 'same', 'reviewer': 'same'},
                        publication=examples.publication())
        observed['evidence'].append(observed['publication']['pr'])
        positives.append(observed)
        for value in positives:
            with self.subTest(positive=value['task_id']):
                proof_validator.validate(value)  # NI baseline: real ValidationError
        policy_validator.validate(examples.policy('CONVERT-901'))
        policy_validator.validate(examples.policy())
        # Shape/pattern cases are single edits of a passing positive; do not ask a
        # schema to perform workflow grammar, checksum, URL decoding or time order.
        for key, value in [('version', True), ('version', 2), ('task_id', 'CONVERT-901\n'),
                ('task_id', 'convert-901'), ('owner', 'invented'), ('kind', 'epic'),
                ('brief', '../escape'), ('brief', 'a//b'), ('brief', 'a\\b'),
                ('limitations', None), ('evidence', [''])]:
            bad = examples.record(); bad[key] = value
            with self.subTest(field=key, value=value), self.assertRaises(self.Invalid):
                proof_validator.validate(bad)
        for key in examples.record():
            bad = examples.record(); del bad[key]
            with self.subTest(missing=key), self.assertRaises(self.Invalid):
                proof_validator.validate(bad)
        for issue in ({'repository': 'other/project', 'number': 9901},
                      {'repository': 'Kernol12/subathon-bot-ai', 'number': True},
                      {'repository': 'Kernol12/subathon-bot-ai', 'number': 0},
                      dict(examples.record()['issue'], url='invented')):
            bad = examples.record(); bad['issue'] = issue
            with self.subTest(issue=issue), self.assertRaises(self.Invalid):
                proof_validator.validate(bad)
        for key, value in [('version', True), ('archive_commit', 'short'),
                           ('legacy_done_sha256', 'x'*64), ('unknown', 'field')]:
            bad = examples.policy(); bad[key] = value
            with self.subTest(policy=key), self.assertRaises(self.Invalid):
                policy_validator.validate(bad)
        for key in examples.policy():
            bad = examples.policy(); del bad[key]
            with self.subTest(policy_missing=key), self.assertRaises(self.Invalid):
                policy_validator.validate(bad)
        for mutation in examples.supplemental_mutations():
            bad = copy.deepcopy(historical); bad['supplemental_workflow_evidence'] = mutation
            with self.subTest(supplemental='manual mutation'), self.assertRaises(self.Invalid):
                proof_validator.validate(bad)
        bad = copy.deepcopy(historical); bad['task_id'] = 'CONVERT-901'
        with self.assertRaises(self.Invalid):
            proof_validator.validate(bad)
        # Draft integer accepts integral floats; the stdlib validator is narrower.
        value = examples.record(); value['version'] = 1.0; value['issue']['number'] = 9901.0
        proof_validator.validate(value)
        # A schema instant is nonempty text; Python owns aware syntax and order.
        value = copy.deepcopy(observed); value['publication']['observed_at'] = 'not-an-instant'
        proof_validator.validate(value)
        # Schema-valid does not mean semantically valid/current/complete.
        stale = examples.reviewed_record()
        for receipt in stale['workflow']['stages']['contract']['reviews'][-2:]:
            receipt['revision'] = 'a'*40
        proof_validator.validate(stale)
        self_review = examples.reviewed_record()
        self_review['workflow']['stages']['contract']['reviews'][0]['reviewer'] = 'codex:synthetic-author'
        proof_validator.validate(self_review)


if __name__ == '__main__':
    unittest.main()
