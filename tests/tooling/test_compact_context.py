# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent literal oracles for context-publication-v1 (WORKFLOW-005).

Authored only: full independent test review must close before import or execution.
"""
import copy
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests/support"))
import proofs
from context_fixture import invoke_context

FULL_LIMIT = ('Recorded information only; Git contents, live GitHub/CI, review identity '
              'and authorization are not verified.')
COMPACT_LIMIT = FULL_LIMIT + (' Compact view omits complete workflow, other stages, supplemental history, '
    'non-blocking finding details and closing-only finding metrics; use full mode or review links.')
BRIEF = "docs/tasks/WORKFLOW-001.md"
OLD, CURRENT = "a" * 40, "b" * 40
# These are invented fixture receipts, never claims of actual independent review.
REFS = [
    {"phase": "initial", "provider": "claude", "model": "synthetic",
     "reviewer": "fixture-initial", "revision": OLD, "outcome": "changes_requested",
     "evidence": "https://example.test/initial"},
    {"phase": "verification", "provider": "codex", "model": "synthetic",
     "reviewer": "fixture-verification-one", "revision": CURRENT, "outcome": "approved",
     "evidence": "https://example.test/verification-one"},
    {"phase": "closing", "provider": "claude", "model": "synthetic",
     "reviewer": "fixture-closing-one", "revision": CURRENT, "outcome": "changes_requested",
     "evidence": "https://example.test/closing-one"},
    {"phase": "verification", "provider": "codex", "model": "synthetic",
     "reviewer": "fixture-verification-two", "revision": CURRENT, "outcome": "approved",
     "evidence": "https://example.test/verification-two"},
    {"phase": "closing", "provider": "claude", "model": "synthetic",
     "reviewer": "fixture-closing-two", "revision": CURRENT, "outcome": "approved",
     "evidence": "https://example.test/closing-two"},
]


def literal_bundle():
    """Canonical input, independent literal values; no state/status projection.

    The retired global activity sentinel maps to an UNSELECTED record limitation.
    Supplemental history uses only APITEST-016's frozen shape and a string leaf.
    """
    from test_proofs import record, policy, supplemental
    reviews = copy.deepcopy(REFS)
    for review in reviews:
        review.update(findings=[], blocking_findings=[])
    reviews[0].update(findings=['B2','advisory','B1'], blocking_findings=['B2','B1'],
        started_at='2026-11-12T12:00:00Z', finished_at='2026-11-12T12:00:01.250Z')
    reviews[1].update(started_at='2026-11-12T13:00:00+01:00',finished_at='2026-11-12T12:00:02.500Z')
    reviews[2].update(findings=['B3','B2','closing-advisory'],blocking_findings=['B3','B2'])
    reviews[3].update(started_at='2026-11-12T12:00:03Z',finished_at='2026-11-12T12:00:03Z')
    selected = record('APITEST-016')
    selected.update(kind='implementation',brief=BRIEF,
        evidence=['https://example.test/selected-evidence'], limitations=['Kompakt ä'],
        supplemental_workflow_evidence=copy.deepcopy(supplemental()),
        workflow=dict(required_stages=['tests','implementation'],stages={
            'tests':dict(authors=['fixture-author'],revision=CURRENT,paths=[BRIEF],reviews=reviews),
            'implementation':dict(authors=['other-stage-author'],revision=OLD,paths=[BRIEF],reviews=[])}))
    selected['supplemental_workflow_evidence']['format']='supplemental sentinel'
    unrelated=record('CTX-999',9902)
    unrelated['limitations']=['unrelated sentinel','activity sentinel',
                             'Unrelated full record sentinel','Unrelated historical note sentinel',
                             'Historical activity sentinel']
    return dict(policy=policy('APITEST-016','CTX-999'),
                records={'APITEST-016':selected,'CTX-999':unrelated})


def expected_compact():
    # Independent declaration, corroborated by baseline ProofTests449–477.
    return dict(proof=dict(version=1,task_id='APITEST-016',
        issue=dict(repository='Kernol12/subathon-bot-ai',number=9901),kind='implementation',
        brief=BRIEF,evidence=['https://example.test/selected-evidence'],limitations=['Kompakt ä']),
        required_stages=['tests','implementation'],limitations=COMPACT_LIMIT)


class CompactContextTests(unittest.TestCase):
    def invoke(self, data, *options):
        return invoke_context(self, data, ['context', *options])[:3]

    def success(self, result):
        code, stdout, stderr = result
        self.assertEqual(code, 0, stderr)
        self.assertEqual(stderr, '')
        return stdout

    def test_compact_cli_literal_shape_order_serialization_and_no_stage(self):
        data = literal_bundle()
        before = copy.deepcopy(data)
        expected = expected_compact()
        output = self.success(self.invoke(data, "APITEST-016", "--compact"))
        self.assertEqual(output, json.dumps(expected, indent=2, ensure_ascii=False) + "\n")
        self.assertEqual(proofs.context(data, "APITEST-016", compact=True), expected)
        self.assertEqual(data, before)

    def test_selected_stage_keeps_all_refs_historical_blockers_and_exact_metrics(self):
        expected = expected_compact()
        expected["stage_context"] = {
            "name": "tests", "complete": True, "next_gate": "complete",
            "packet": {"authors": ["fixture-author"], "revision": CURRENT, "paths": [BRIEF]},
            "review_refs": REFS, "historical_blocking_findings": ["B2", "B1", "B3"],
            "metrics": {"review_count": 5, "elapsed_seconds": 3.75, "timed_reviews": 3},
        }
        data=literal_bundle(); before=copy.deepcopy(data)
        self.assertIsNone(proofs.validate(data['policy'],list(data['records'].values())))
        output = self.success(self.invoke(data, 'APITEST-016', '--compact', '--stage', 'tests'))
        self.assertEqual(output, json.dumps(expected, indent=2, ensure_ascii=False) + "\n")
        for excluded in ("supplemental sentinel", "activity sentinel", "other-stage-author",
                         "closing-advisory", "unrelated sentinel"):
            self.assertNotIn(excluded, output)
            self.assertIn(excluded, json.dumps(data))
        for compact in (False, True):
            view=proofs.context(data,'APITEST-016',stage='tests',compact=compact)
            self.assertEqual(view['proof']['evidence'], ['https://example.test/selected-evidence'])
        self.assertEqual(data,before)

    def test_unstarted_and_untimed_stages_and_existing_next_gates(self):
        for reviews, complete, gate, count in ((None, False, "authoring", 0),
                                               ([], False, "initial", 0),
                                               (REFS[:1], False, "verification", 1),
                                               (REFS[:2], False, "closing", 2)):
            with self.subTest(gate=gate):
                data = literal_bundle()
                stages = data["records"]["APITEST-016"]["workflow"]["stages"]
                if reviews is None:
                    del stages["tests"]
                else:
                    stages["tests"]["reviews"] = [dict(r, findings=["B"] if i == 0 else [],
                        blocking_findings=["B"] if i == 0 else []) for i, r in enumerate(reviews)]
                view = json.loads(self.success(self.invoke(data, "APITEST-016", "--stage", "tests", "--compact")))
                self.assertEqual(view["stage_context"], {
                    "name": "tests", "complete": complete, "next_gate": gate,
                    "packet": None if reviews is None else {
                        "authors": ["fixture-author"], "revision": CURRENT, "paths": [BRIEF]},
                    "review_refs": [] if reviews is None else reviews,
                    "historical_blocking_findings": ["B"] if count else [],
                    "metrics": {"review_count": count, "elapsed_seconds": None, "timed_reviews": 0}})

    def test_optional_publication_empty_required_stages_and_availability(self):
        data=literal_bundle()
        publication = {"state":"open", "pr":"https://example.test/selected-evidence", "head":CURRENT,
            "merge_commit":None,"merged_at":None,"observed_at":"2026-11-12T12:00:00Z"}
        data['records']['APITEST-016']['publication']=publication
        expected=expected_compact(); expected['proof']['publication']=publication
        self.assertEqual(json.loads(self.success(self.invoke(data,'APITEST-016','--compact'))),expected)
        leaf=json.loads(self.success(self.invoke(data,'CTX-999','--compact')))
        self.assertEqual(leaf['required_stages'],[])
        self.assertNotIn('publication',leaf['proof'])

    def test_default_context_bytes_and_full_selected_history_are_unchanged(self):
        data=literal_bundle(); before=copy.deepcopy(data)
        expected=dict(proof=copy.deepcopy(data['records']['APITEST-016']),
                      required_stages=['tests','implementation'],limitations=FULL_LIMIT)
        observed=self.success(self.invoke(data,'APITEST-016'))
        actual=json.loads(observed)
        self.assertEqual(observed,json.dumps(expected,indent=2,ensure_ascii=False)+'\n')
        self.assertEqual(proofs.context(data,'APITEST-016'),expected)
        self.assertEqual(proofs.context(data,'APITEST-016',compact=False),expected)
        # Moved ContextCliTests372–396: exact selected record, bounded history, warning concepts.
        for concept in ('recorded','github','ci','not verified'):
            self.assertIn(concept,actual['limitations'].lower())
        for sentinel in ('Unrelated full record sentinel','Unrelated historical note sentinel','Historical activity sentinel'):
            self.assertIn(sentinel,json.dumps(data))
            self.assertNotIn(sentinel,observed)
        expected['stage_context']=dict(name='tests',packet=copy.deepcopy(data['records']['APITEST-016']['workflow']['stages']['tests']),
            complete=True,next_gate='complete',metrics=dict(review_count=5,blocking_findings=['B2','B1','B3'],
                closing_only_findings=['B3','closing-advisory'],elapsed_seconds=3.75,timed_reviews=3))
        self.assertEqual(self.success(self.invoke(data,'APITEST-016','--stage','tests')),json.dumps(expected,indent=2,ensure_ascii=False)+'\n')
        self.assertEqual(data,before)

    def test_lookup_validation_and_argparse_errors_precede_compact_ni(self):
        for options,message in ((('CTX-404','--compact'),'Unknown task: CTX-404'),
                (('APITEST-016','--stage','unknown','--compact'),'APITEST-016: unknown workflow stage: unknown')):
            with self.subTest(options=options):
                self.assertEqual(self.invoke(literal_bundle(),*options),(1,'',f'Proof error: {message}\n'))
        data=literal_bundle()
        self.success(self.invoke(data,'APITEST-016'))
        data['records']['CTX-999']['version']=True
        for identity in ('APITEST-016','CTX-404'):
            result=self.invoke(data,identity,'--compact')
            self.assertEqual(result[:2],(1,'')); self.assertIn('Proof error:',result[2])
            self.assertNotIn('Unknown task',result[2]); self.assertNotIn('CUTOVER_',result[2])
        for options in (('--compact',),('APITEST-016','--not-an-option')):
            code,out,err=self.invoke(literal_bundle(),*options)
            self.assertEqual((code,out),(2,'')); self.assertIn('usage:',err)
        with self.assertRaisesRegex(ValueError,'Unknown task: CTX-404'):
            proofs.context(literal_bundle(),'CTX-404',compact=True)


if __name__ == "__main__":
    unittest.main()
