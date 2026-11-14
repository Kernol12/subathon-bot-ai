# SPDX-License-Identifier: GPL-3.0-or-later
"""Literal WORKFLOW-003 record/CLI examples, independent of implementation helpers."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0,str(ROOT / "tests/support"))
import proofs
import workflow
from test_proofs import policy
from context_fixture import invoke_context

OLD_REV = "a" * 40
REV = "b" * 40
MERGE = "c" * 40
PR = "https://example.test/project/pull/42"


def receipt(phase, revision=REV, **changes):
    result = {
        "phase": phase, "provider": "codex" if phase == "verification" else "claude",
        "model": "synthetic-model", "reviewer": f"independent-{phase}",
        "revision": revision, "outcome": "approved",
        "evidence": f"https://example.test/reviews/{phase}",
        "findings": [], "blocking_findings": [],
    }
    result.update(changes)
    return result


def stage(reviews=None):
    return {
        "authors": ["author-session", "coauthor-session"], "revision": REV,
        "paths": ["docs/tasks/WORKFLOW-001.md"],
        "reviews": ([receipt("initial", OLD_REV), receipt("verification"), receipt("closing")]
                    if reviews is None else reviews),
    }


def publication(state="merged"):
    return {
        "state": state, "pr": PR, "head": REV,
        "merge_commit": MERGE if state == "merged" else None,
        "merged_at": "2026-11-11T12:00:00Z" if state == "merged" else None,
        "observed_at": "2026-11-11T12:01:00+00:00",
    }


def task(task_id='WFLOW-999', kind='contract'):
    names = ['contract'] if kind=='contract' else ['tests'] if kind.startswith('test_') else ['tests','implementation'] if kind=='implementation' else ['implementation']
    return dict(version=1,task_id=task_id,issue=dict(repository='Kernol12/subathon-bot-ai',number=9901),
        kind=kind,brief='docs/tasks/WORKFLOW-001.md',limitations=['Original task note'],evidence=[PR],
        verification={'acceptance':'passed','baseline':'observed_red'},
        review={'author':'author-session','reviewer':'independent-session','outcome':'approved'},
        workflow=dict(required_stages=names,stages={name:stage() for name in names}))


def bundle(value):
    return dict(policy=policy(value['task_id']),records={value['task_id']:value})


class WorkflowRecordTests(unittest.TestCase):
    def test_policy_requires_new_claim_records_without_fabricating_legacy_reviews(self):
        historical=task('ACCESS-002'); del historical['workflow']; historical['review']=None
        self.assertIsNone(proofs.validate(policy('ACCESS-002'),[historical]))
        self.assertIsNone(historical['review'])
        missing=task(); del missing['workflow']
        with self.assertRaises(ValueError):
            proofs.require_complete(bundle(missing),'WFLOW-999')
        # Keep the full frozen boundary; a matched checksum cannot admit a duplicate
        # or newly grandfather an unknown/current non-exempt task.
        for addition,diagnostic in (('ACCESS-002','duplicates'),('UNKNOWN-999','frozen'),('WFLOW-999','frozen')):
            bad=policy('ACCESS-002'); bad['legacy_done'].append(addition)
            bad['legacy_done_sha256']=hashlib.sha256(
                ('\n'.join(sorted(bad['legacy_done']))+'\n').encode('utf-8')).hexdigest()
            with self.subTest(addition=addition),self.assertRaisesRegex(ValueError,diagnostic):
                proofs.validate_policy(bad)
        # Whole-list substitution is independently owned by ProofTests214–216.
        for key,value in (('version',2),('legacy_done_sha256','0'*64)):
            bad=policy(); bad[key]=value
            with self.assertRaises(ValueError): proofs.validate_policy(bad)
        self.assertIsNone(proofs.validate_record(historical))
        self.assertIsNone(proofs.validate_record(task('ACCESS-002')))
        supplied=task('ACCESS-002'); supplied['workflow']['stages']['contract']['authors']=[]
        with self.assertRaises(ValueError): proofs.validate_record(supplied)
        self.assertEqual(workflow.kind_minimum('epic'),set())

    def test_nonexempt_contract_done_requires_separate_approved_review(self):
        # Historical legacy review objects do not replace current stage approval.
        for kind in ('contract','cleanup'):
            good=task(kind=kind)
            self.assertIsNone(proofs.require_complete(bundle(good),'WFLOW-999'))
            name=good['workflow']['required_stages'][0]
            missing=copy.deepcopy(good); missing['workflow']['stages'][name]['reviews']=[]
            with self.assertRaises(ValueError):
                proofs.require_complete(bundle(missing),'WFLOW-999')
            self_review=copy.deepcopy(good)
            self_review['workflow']['stages'][name]['reviews'][-1]['reviewer']='author-session'
            with self.assertRaisesRegex(ValueError,'(?i)(author|independen)'):
                proofs.require_complete(bundle(self_review),'WFLOW-999')
            rejected=copy.deepcopy(good)
            rejected['workflow']['stages'][name]['reviews'][-1].update(
                outcome='changes_requested',findings=['B1'],blocking_findings=['B1'])
            self.assertIsNone(proofs.validate_record(rejected))
            with self.assertRaises(ValueError):
                proofs.require_complete(bundle(rejected),'WFLOW-999')

    def test_kind_minimum_stages_and_packet_shape_are_enforced(self):
        for kind, names in (("contract", ["contract"]), ("test_draft", ["tests"]),
                            ("test_review", ["tests"]), ("test_baseline", ["tests"]),
                            ("implementation", ["tests", "implementation"]), ("cleanup", ["implementation"])):
            good = task(kind=kind)
            proofs.validate_record(good)
            with self.subTest(kind=kind):
                good["workflow"] = {"required_stages": ["tests"] if names == ["contract"] else ["contract"],
                                    "stages": {"tests" if names == ["contract"] else "contract": stage()}}
                with self.assertRaises(ValueError):
                    proofs.validate_record(good)
        for key, value in (("authors", []), ("authors", ["author", "author"]),
                           ("revision", "B" * 40), ("revision", "b" * 39),
                           ("paths", []), ("paths", ["../outside"]),
                           ("paths", ["docs/tasks/WORKFLOW-001.md"] * 2)):
            with self.subTest(field=key, value=value):
                candidate = task()
                candidate["workflow"]["stages"]["contract"][key] = value
                with self.assertRaises(ValueError):
                    proofs.validate_record(candidate)
        for workflow_value in ({"required_stages": [], "stages": {}},
                         {"required_stages": ["contract", "contract"], "stages": {"contract": stage()}},
                         {"required_stages": ["contract"], "stages": {"implementation": stage()}},
                         {"required_stages": ["contract", "unknown"], "stages": {"contract": stage(), "unknown": stage()}}):
            with self.subTest(workflow=workflow_value), self.assertRaises(ValueError):
                candidate = task()
                candidate["workflow"] = workflow_value
                proofs.validate_record(candidate)
        candidate = task(kind="implementation")
        candidate["workflow"] = {"required_stages": ["implementation"], "stages": {"implementation": stage()}}
        with self.assertRaises(ValueError):
            proofs.validate_record(candidate)
        historical=task(); historical['workflow']['stages']['contract']['paths']=['docs/no-such-workflow-file.md']
        self.assertIsNone(proofs.validate_record(historical))
        extra = task()
        extra["workflow"]["stages"]["tests"] = stage()
        with self.assertRaises(ValueError):
            proofs.validate_record(extra)
        extra["workflow"]["required_stages"].append("tests")
        proofs.validate_record(extra)  # Additional declared stages are allowed.
        unstarted = task(kind="implementation")
        unstarted["workflow"]["stages"] = {}
        proofs.validate_record(unstarted)  # Future authors/paths/revisions are not invented.

    def test_review_order_requires_initial_verification_and_fresh_closing_cycles(self):
        rejected = receipt("closing", outcome="changes_requested", findings=["B1"], blocking_findings=["B1"])
        unapproved = receipt("verification", outcome="changes_requested", findings=["B1"], blocking_findings=["B1"])
        invalid = [
            [receipt("verification")], [receipt("initial"), receipt("closing")],
            [receipt("initial"), receipt("initial")],
            [receipt("initial"), unapproved, receipt("closing")],
            [receipt("initial"), receipt("verification"), unapproved, receipt("closing")],
            [receipt("initial"), receipt("verification", OLD_REV), receipt("closing")],
            [receipt("initial"), receipt("verification"), rejected, receipt("closing")],
            [receipt("initial"), receipt("verification"), receipt("closing"), receipt("closing")],
        ]
        for reviews in invalid:
            with self.subTest(phases=[r["phase"] for r in reviews]):
                candidate = task()
                candidate["workflow"]["stages"]["contract"] = stage(reviews)
                with self.assertRaises(ValueError):
                    proofs.validate_record(candidate)
        for reviews in ([receipt("initial"), receipt("verification"), rejected,
                         receipt("verification"), receipt("closing")],
                        [receipt("initial"), receipt("verification"), receipt("closing"),
                         receipt("verification"), receipt("closing")],
                        [receipt("initial"), unapproved, receipt("verification"), receipt("closing")]):
            candidate = task()
            candidate["publication"] = publication()
            candidate["workflow"]["stages"]["contract"] = stage(reviews)
            proofs.require_complete(bundle(candidate),"WFLOW-999")

    def test_receipt_independence_evidence_findings_and_provider_are_validated(self):
        bad_receipts = [
            receipt("initial", provider="codex"), receipt("verification", provider="claude"),
            receipt("closing", provider="codex"), receipt("initial", reviewer="coauthor-session"),
            receipt("initial", model=""), receipt("initial", reviewer=""),
            receipt("initial", revision="short"), receipt("initial", evidence="http://example.test/review"),
            receipt("initial", findings=["B1", "B1"]),
            receipt("initial", blocking_findings=["B1"]),
            receipt("initial", outcome="changes_requested", blocking_findings=["B1"]),
            receipt("initial", outcome="changes_requested", findings=["B1"], blocking_findings=["B1", "B1"]),
            receipt("initial", findings=[1]),
            receipt("initial", findings=["B1"], blocking_findings=["B1"]),
            receipt("initial", outcome="changes_requested"),
            receipt("verification", reviewer="author-session"),
            receipt("closing", reviewer="coauthor-session"), receipt("initial", reviewer="author-session"),
        ]
        self.assertIsNone(proofs.validate_record(task()))  # Moved self-review passing control.
        for invalid in bad_receipts:
            with self.subTest(receipt=invalid):
                reviews = [receipt("initial"), receipt("verification"), receipt("closing")]
                reviews[{"initial": 0, "verification": 1, "closing": 2}[invalid["phase"]]] = invalid
                candidate = task()
                candidate["workflow"]["stages"]["contract"] = stage(reviews)
                with self.assertRaises(ValueError):
                    proofs.validate_record(candidate)

    def test_review_times_require_observed_ordered_timezone_pairs(self):
        for times in ({"started_at": "2026-11-11T12:00:00Z"},
                      {"finished_at": "2026-11-11T12:00:00Z"},
                      {"started_at": "2026-11-11T12:00:00", "finished_at": "2026-11-11T12:01:00"},
                      {"started_at": "2026-11-11T12:02:00Z", "finished_at": "2026-11-11T12:01:00Z"},
                      {"started_at": "not-a-time", "finished_at": "not-a-time"}):
            with self.subTest(times=times):
                candidate = task()
                candidate["workflow"]["stages"]["contract"] = stage([receipt("initial", **times)])
                with self.assertRaises(ValueError):
                    proofs.validate_record(candidate)
        candidate = task()
        candidate["workflow"]["stages"]["contract"] = stage([receipt("initial",
            started_at="2026-11-11T13:00:00+01:00", finished_at="2026-11-11T12:01:00Z")])
        proofs.validate_record(candidate)

    def test_done_requires_complete_current_stages_and_publication(self):
        for reviews in ([],[receipt('initial')],[receipt('initial'),receipt('verification')],
                [receipt('initial'),receipt('verification',OLD_REV),receipt('closing',OLD_REV)],
                [receipt('initial'),receipt('verification'),receipt('closing'),receipt('verification')]):
            with self.subTest(reviews=reviews):
                candidate=task(); candidate['publication']=publication()
                candidate['workflow']['stages']['contract']=stage(reviews)
                with self.assertRaises(ValueError): proofs.require_complete(bundle(candidate),'WFLOW-999')
        missing=task(); del missing['workflow']
        with self.assertRaises(ValueError): proofs.require_complete(bundle(missing),'WFLOW-999')
        done=task(); done['publication']=publication('open')
        self.assertIsNone(proofs.require_complete(bundle(done),'WFLOW-999'))
        self.assertEqual(done['publication']['state'],'open')
        # Moved ProgressTests152–161: passing acceptance recorded, independent approval closes.
        implementation=task(kind='implementation')
        self.assertEqual(proofs.context(bundle(implementation),'WFLOW-999')['proof']['verification']['acceptance'],'passed')
        self.assertIsNone(proofs.require_complete(bundle(implementation),'WFLOW-999'))
        implementation['workflow']['stages']['implementation']=stage([])
        with self.assertRaises(ValueError): proofs.require_complete(bundle(implementation),'WFLOW-999')
        del implementation['workflow']['stages']['implementation']
        with self.assertRaises(ValueError): proofs.require_complete(bundle(implementation),'WFLOW-999')

    def test_publication_shape_and_dependency_readiness_match_context_and_html(self):
        for key, value in (("state", "closed"), ("pr", "https://example.test/other/pull/1"),
                           ("head", "short"), ("merge_commit", None), ("merged_at", None),
                           ("observed_at", "2026-11-11T11:59:59Z"),
                           ("observed_at", "2026-11-11T12:01:00"),
                           ("pr", "http://example.test/project/pull/42"),
                           ("merged_at", "2026-11-11T12:00:00")):
            with self.subTest(field=key, value=value):
                candidate = task()
                candidate["publication"] = publication()
                candidate["publication"][key] = value
                with self.assertRaises(ValueError):
                    proofs.validate_record(candidate)
        equal = task()
        equal["publication"] = publication()
        equal["publication"]["observed_at"] = equal["publication"]["merged_at"]
        proofs.validate_record(equal)
        for key, value in (("merge_commit", MERGE), ("merged_at", "2026-11-11T12:00:00Z")):
            candidate = task()
            candidate["publication"] = publication("open")
            candidate["publication"][key] = value
            with self.subTest(open_field=key), self.assertRaises(ValueError):
                proofs.validate_record(candidate)


class WorkflowCliTests(unittest.TestCase):
    def invoke(self, data, command):
        return invoke_context(self,data,command)

    def context(self, reviews, revision=REV):
        selected = task()
        selected["workflow"]["stages"]["contract"] = stage(reviews)
        selected["workflow"]["stages"]["contract"]["revision"] = revision
        result, stdout, stderr, before, after = self.invoke(bundle(selected), ["context", "WFLOW-999", "--stage", "contract"])
        self.assertEqual(result, 0, stderr)
        self.assertEqual(stderr, "")
        self.assertEqual(before, after, "Stage context must remain read-only")
        return json.loads(stdout), selected

    def test_stage_context_derives_packet_next_gate_and_observed_metrics(self):
        reviews = [
            receipt("initial", OLD_REV, outcome="changes_requested", findings=["B1"], blocking_findings=["B1"],
                    started_at="2026-11-11T12:00:00Z", finished_at="2026-11-11T12:00:10Z"),
            receipt("verification", findings=["N1"], started_at="2026-11-11T12:00:15Z", finished_at="2026-11-11T12:00:20Z"),
            receipt("closing", outcome="changes_requested", findings=["B1", "B2"], blocking_findings=["B2"]),
            receipt("verification", findings=["N2"]), receipt("closing", findings=["B2", "N3"]),
        ]
        output, selected = self.context(reviews)
        view = output["stage_context"]
        self.assertEqual({key: view[key] for key in ("name", "packet", "complete", "next_gate")},
                         {"name": "contract", "packet": selected["workflow"]["stages"]["contract"],
                          "complete": True, "next_gate": "complete"})
        metrics = view["metrics"]
        self.assertEqual(metrics["review_count"], 5)
        self.assertEqual(metrics["blocking_findings"], ["B1", "B2"])
        self.assertEqual(metrics["closing_only_findings"], ["B2", "N3"])
        self.assertEqual(metrics["elapsed_seconds"], 15)
        self.assertEqual(metrics["timed_reviews"], 2)
        self.assertIn("recorded", output["limitations"].lower())
        self.assertIn("not verified", output["limitations"].lower())
        for history, revision, expected in (([], REV, "initial"), ([receipt("initial")], REV, "verification"),
                ([receipt("initial"), receipt("verification")], REV, "closing"),
                ([receipt("initial"), receipt("verification", outcome="changes_requested", findings=["C1"], blocking_findings=["C1"])], REV, "verification"),
                (reviews[:3], REV, "verification"),
                ([receipt("initial"), receipt("verification"), receipt("closing"), receipt("verification")], REV, "closing"),
                ([receipt("initial"), receipt("verification"), receipt("closing")], OLD_REV, "verification")):
            with self.subTest(expected=expected):
                output, _ = self.context(history, revision)
                view = output["stage_context"]
                self.assertEqual(view["next_gate"], expected)
                self.assertFalse(view["complete"])
                if history != reviews[:3]:
                    self.assertEqual(view["metrics"]["timed_reviews"], 0)
                    self.assertIsNone(view["metrics"]["elapsed_seconds"])
        pending = task()
        pending["workflow"]["stages"] = {}
        result, stdout, stderr, before, after = self.invoke(bundle(pending), ["context", "WFLOW-999", "--stage", "contract"])
        self.assertEqual(result, 0, stderr)
        self.assertEqual(before, after)
        view = json.loads(stdout)["stage_context"]
        self.assertEqual((view["packet"], view["complete"], view["next_gate"]), (None, False, "authoring"))

    def test_default_context_keeps_existing_shape_and_unknown_stage_refuses_read_only(self):
        data = bundle(task())
        result, stdout, stderr, before, after = self.invoke(data, ["context", "WFLOW-999"])
        self.assertEqual(result, 0, stderr)
        self.assertEqual(before, after)
        output = json.loads(stdout)
        self.assertEqual(set(output), {"proof", "required_stages", "limitations"})
        self.assertEqual(output["proof"], data["records"]["WFLOW-999"])
        result, stdout, stderr, before, after = self.invoke(data, ["context", "WFLOW-999", "--stage", "tests"])
        self.assertEqual(result, 1)
        self.assertEqual(stdout, "")
        self.assertTrue(stderr.strip())
        self.assertNotIn("CUTOVER_",stderr,"NI is not an input refusal")
        self.assertEqual(before, after)




if __name__ == "__main__":
    unittest.main()
