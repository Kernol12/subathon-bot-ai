# SPDX-License-Identifier: GPL-3.0-or-later
"""Publication binding/history tests use real disposable Git, never mock Git proof.

Host/schema orchestration is mocked here; actual admitted schema proof belongs only
in test_publication_integration.py. Review must close before importing these tests.
"""
from contextlib import contextmanager, redirect_stderr, redirect_stdout
from io import StringIO
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests/support"))
import check_publication as publication
import progress
from publication_fixture import PublicationFixture

WITNESS = b'{"tests":1,"failures":0,"errors":0,"skipped":0}\n'
LIMITATIONS = [
    "Recorded bindings are not authenticated approvals.",
    "Forbidden-path checks are not complete secret scanning.",
    "Current-head hosted, repository and owner gates are separate.",
    "Filesystem checks can observe ignored local files.",
]
ENV = {"PATH": "/usr/bin:/bin", "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8"}


class PublicationTests(unittest.TestCase):
    @contextmanager
    def fixture(self):
        with tempfile.TemporaryDirectory() as directory:
            yield PublicationFixture(Path(directory) / "candidate")

    def invoke(self, fixture, arguments=None, *, responses=None, before_step=None):
        """Only substitute host/schema children; real Git remains on run_step."""
        real_step = publication.run_step
        children, git_calls = [], []

        def step(argv, *, git=False):
            if git:
                git_calls.append(list(argv))
                return real_step(argv, git=True)
            index = len(children)
            children.append(list(argv))
            if before_step is not None:
                before_step(index, argv)
            if responses is not None and index in responses:
                response = responses[index]
                if isinstance(response, Exception):
                    raise response
                code, stdout, stderr = response
                return subprocess.CompletedProcess(argv, code, stdout, stderr)
            return subprocess.CompletedProcess(argv, 0, WITNESS if index == 3 else b"local diagnostic\n", b"")

        before = fixture.snapshot()
        stdout, stderr = StringIO(), StringIO()
        with patch.object(publication, "ROOT", fixture.root), patch.object(publication, "run_step", side_effect=step), \
             redirect_stdout(stdout), redirect_stderr(stderr):
            try:
                code = publication.main(fixture.arguments() if arguments is None else arguments)
            except SystemExit as error:
                code = error.code
        self.assertEqual(fixture.snapshot(), before, "Publication must not write files/index/refs")
        return code, stdout.getvalue(), stderr.getvalue(), children, git_calls

    def success(self, result):
        code, stdout, stderr, children, git_calls = result
        diagnostic = ("intended assertion red: PUBLICATION_CHECK_NOT_IMPLEMENTED"
                      if code == 1 and "PUBLICATION_CHECK_NOT_IMPLEMENTED" in stderr
                      else "unexpected setup/interface failure is NOT intended red: " + stderr)
        self.assertEqual(code, 0, diagnostic)
        self.assertTrue(git_calls, "A mock host witness cannot replace Git verification")
        self.assertEqual(len(children), 4)
        return json.loads(stdout)

    def refusal(self, result, *, children=0):
        code, stdout, stderr, calls, _ = result
        self.assertEqual((code, stdout), (1, ""))
        self.assertTrue(stderr.strip())
        self.assertNotIn("PUBLICATION_CHECK_NOT_IMPLEMENTED", stderr,
                         "NI is not evidence for a negative safety case")
        self.assertEqual(len(calls), children)

    def test_clean_range_cli_success_has_exact_public_summary(self):
        with self.fixture() as fixture:
            result = self.invoke(fixture)
            self.success(result)
            expected = {
                "task": "WFIX-001", "base": fixture.base, "head": fixture.head, "commit_count": 1,
                "stages": [{"name": "contract", "revision": fixture.base, "path_count": 1, "complete": False}],
                "checks": {"recorded_paths": "passed", "outgoing_paths": "passed", "progress": "passed",
                           "render": "passed", "project": "passed", "schema": "passed"},
                "limitations": LIMITATIONS,
            }
            self.assertEqual(result[1], json.dumps(expected, indent=2, ensure_ascii=False) + "\n")
            self.assertNotIn(str(fixture.root), result[1])
            self.assertNotIn("local diagnostic", result[1])
            # Includes working-tree authored files, with its own source SHA and no invented approval.
            self.assertEqual((fixture.root / "tools/check_publication.py").read_bytes(),
                             (ROOT / "tools/check_publication.py").read_bytes())
            self.assertEqual(fixture.task["workflow"]["stages"]["contract"]["reviews"], [])

    def test_receipt_only_change_and_empty_range_keep_source_binding(self):
        with self.fixture() as fixture:
            fixture.data["activity"].append({"date": "2026-11-12", "task": "WFIX-001",
                                            "note": "Synthetic observation only, no review claim"})
            fixture.save()
            fixture.head = fixture.commit("Synthetic receipt-only metadata observation")
            summary = self.success(self.invoke(fixture))
            self.assertEqual(summary["commit_count"], 2)
            self.assertEqual(summary["stages"][0]["revision"], fixture.base)
            summary = self.success(self.invoke(fixture, fixture.arguments(base="HEAD")))
            self.assertEqual((summary["base"], summary["head"], summary["commit_count"]),
                             (fixture.head, fixture.head, 0))

    def test_repeated_distinct_stages_preserve_cli_order_and_exclude_unselected_binding(self):
        with self.fixture() as fixture:
            # Alter the tests source; selecting only contract must still pass.
            with (fixture.root / "tests/tooling/test_publication.py").open("ab") as stream:
                stream.write(b"\n# Synthetic source change\n")
            fixture.head = fixture.commit("Change unselected packet")
            self.success(self.invoke(fixture))
            self.refusal(self.invoke(fixture, fixture.arguments(stages=("tests", "contract"))))
        with self.fixture() as fixture:
            summary = self.success(self.invoke(fixture, fixture.arguments(stages=("tests", "contract"))))
            self.assertEqual(summary["stages"], [
                {"name": "tests", "revision": fixture.base, "path_count": 1, "complete": False},
                {"name": "contract", "revision": fixture.base, "path_count": 1, "complete": False}])

    def test_stale_content_mode_missing_path_and_nonregular_git_files_refuse(self):
        name = "docs/contracts/context-publication-v1.md"
        for change in ("content", "mode", "missing", "symlink", "gitlink"):
            with self.subTest(change=change), self.fixture() as fixture:
                path = fixture.root / name
                if change == "content":
                    path.write_bytes(path.read_bytes() + b"\nSynthetic change\n")
                elif change == "mode":
                    path.chmod(0o755)
                elif change == "missing":
                    fixture.task["workflow"]["stages"]["contract"]["paths"] = ["packet-added-later.txt"]
                    fixture.write("packet-added-later.txt")
                    fixture.save()
                elif change == "symlink":
                    path.unlink()
                    path.symlink_to("../tasks/WORKFLOW-005.md")
                else:
                    path.unlink()
                    fixture.git("update-index", "--add", "--cacheinfo", f"160000,{fixture.base},{name}")
                    fixture.git("commit", "--quiet", "-m", "Synthetic gitlink candidate")
                    self.refusal(self.invoke(fixture))
                    continue
                fixture.head = fixture.commit("Synthetic stale or nonregular binding")
                self.refusal(self.invoke(fixture))
        # Matching nonregular modes at BOTH recorded revision and candidate also refuse.
        for mode in ("symlink", "gitlink"):
            with self.subTest(recorded_mode=mode), self.fixture() as fixture:
                name = "packet-link"
                if mode == "symlink":
                    (fixture.root / name).symlink_to("docs/tasks/WORKFLOW-005.md")
                    revision = fixture.commit("Synthetic recorded symlink")
                else:
                    fixture.git("update-index", "--add", "--cacheinfo", f"160000,{fixture.base},{name}")
                    fixture.git("commit", "--quiet", "-m", "Synthetic recorded gitlink")
                    revision = fixture.git("rev-parse", "HEAD").stdout.decode().strip()
                fixture.task["workflow"]["stages"]["contract"].update(revision=revision, paths=[name])
                if mode == "gitlink":
                    # A gitlink cannot satisfy the semantic regular-file precondition.
                    # Record the negative input directly, preserving its Git mode.
                    (fixture.root / "progress/state.json").write_text(json.dumps(fixture.data), encoding="utf-8")
                    fixture.git("add", "progress/state.json")
                    fixture.git("commit", "--quiet", "-m", "Record synthetic gitlink binding")
                else:
                    fixture.save()
                    fixture.commit("Record synthetic link binding")
                self.refusal(self.invoke(fixture))

    def test_literal_unusual_recorded_paths_and_escaping_or_unresolvable_bindings(self):
        with self.fixture() as fixture:
            names = ["packet [one]*?.txt", "packet with space.txt", "-packet.txt", "packet\nline.txt"]
            for name in names:
                fixture.write(name)
            (fixture.root / "-packet.txt").chmod(0o755)  # Both regular Git modes are allowed.
            revision = fixture.commit("Synthetic literal path packet")
            fixture.task["workflow"]["stages"]["contract"].update(revision=revision, paths=names)
            fixture.save()
            fixture.head = fixture.commit("Record literal bindings")
            self.assertEqual(self.success(self.invoke(fixture))["stages"][0]["path_count"], 4)
            fixture.write("packet [one]*?.txt", b"Changed exact literal\n")
            fixture.commit("Change literal wildcard filename")
            self.refusal(self.invoke(fixture))
        for path, revision in (("../outside.txt", None), ("/outside.txt", None),
                               (":(glob)**", None), (None, "f" * 40)):
            with self.subTest(path=path, revision=revision), self.fixture() as fixture:
                if path is not None:
                    fixture.task["workflow"]["stages"]["contract"]["paths"] = [path]
                if revision is not None:
                    fixture.task["workflow"]["stages"]["contract"]["revision"] = revision
                # Deliberately invalid state: do not run the existing renderer/validator to prepare it.
                (fixture.root / "progress/state.json").write_text(json.dumps(fixture.data), encoding="utf-8")
                fixture.commit("Synthetic invalid recorded binding")
                self.refusal(self.invoke(fixture))

    def test_forbidden_components_added_then_deleted_or_renamed_are_rejected(self):
        forbidden = ["local-evidence/private.txt", "local-secrets/private.txt", "a/.venv/x",
                     "a/node_modules/x", ".env", "nested/.env.production", "docs/evidence/private.md"]
        for index, name in enumerate(forbidden):
            with self.subTest(path=name), self.fixture() as fixture:
                # Force-add ignored names only inside the disposable fixture.
                path = fixture.write(name)
                fixture.git("add", "--force", "--", name)
                fixture.git("commit", "--quiet", "-m", "Synthetic forbidden outgoing tree")
                if index % 2:
                    fixture.git("mv", "--", name, "renamed-safe.txt")
                else:
                    path.unlink()
                fixture.head = fixture.commit("Remove forbidden final-tree name")
                self.refusal(self.invoke(fixture))

    def test_allowed_component_boundaries_case_and_base_only_private_history(self):
        with self.fixture() as fixture:
            for name in ("docs/local-evidence-guide.md", "nested/local-evidence/file.txt", ".env.example",
                         "nested/.env.example", "local-secrets-guide.txt", ".ENV", "a/Node_Modules/x"):
                fixture.write(name)
                fixture.git("add", "--force", "--", name)
            fixture.head = fixture.commit("Synthetic allowed fixed-rule names")
            self.success(self.invoke(fixture))
        with self.fixture() as fixture:
            fixture.write("local-secrets/base-only.txt")
            fixture.git("add", "--force", "local-secrets/base-only.txt")
            boundary = fixture.commit("Synthetic private path before outgoing boundary")
            fixture.git("rm", "--", "local-secrets/base-only.txt")
            fixture.head = fixture.commit("Remove before future outgoing work")
            # Removal commit's tree is clean; base itself must not be rescanned.
            self.assertEqual(self.success(self.invoke(fixture, fixture.arguments(base=boundary)))["commit_count"], 1)

    def test_merge_side_history_is_scanned_even_when_final_merge_tree_is_clean(self):
        with self.fixture() as fixture:
            fixture.git("checkout", "-b", "side")
            fixture.write("local-secrets/side.txt")
            fixture.git("add", "--force", "local-secrets/side.txt")
            fixture.commit("Synthetic side private tree")
            fixture.git("rm", "--", "local-secrets/side.txt")
            fixture.commit("Delete private side path")
            fixture.git("checkout", "fixture-main")
            fixture.write("main-only.txt")
            fixture.commit("Synthetic main branch")
            fixture.git("merge", "--no-ff", "--no-edit", "side")
            self.refusal(self.invoke(fixture))
        with self.fixture() as fixture:
            fixture.git("checkout", "-b", "side")
            fixture.write("side-only.txt")
            fixture.commit("Synthetic allowed side tree")
            fixture.git("checkout", "fixture-main")
            fixture.write("main-only.txt")
            fixture.commit("Synthetic allowed main tree")
            fixture.git("merge", "--no-ff", "--no-edit", "side")
            self.assertEqual(self.success(self.invoke(fixture))["commit_count"], 4)

    def test_forbidden_path_in_merge_commit_itself_and_non_utf8_names_refuse(self):
        with self.fixture() as fixture:
            fixture.git("checkout", "-b", "side")
            fixture.write("side-only.txt")
            fixture.commit("Synthetic side")
            fixture.git("checkout", "fixture-main")
            fixture.write("main-only.txt")
            fixture.commit("Synthetic main")
            fixture.git("merge", "--no-ff", "--no-commit", "side")
            fixture.write("local-secrets/merge-only.txt")
            fixture.git("add", "--force", "local-secrets/merge-only.txt")
            fixture.commit("Synthetic forbidden merge tree")
            fixture.git("rm", "--", "local-secrets/merge-only.txt")
            fixture.commit("Clean merge result later")
            self.refusal(self.invoke(fixture))
        with self.fixture() as fixture:
            raw = os.fsencode(fixture.root) + b"/bad-\xff-name"
            with open(raw, "wb") as stream:
                stream.write(b"synthetic\n")
            fixture.commit("Synthetic non UTF-8 Git path")
            self.refusal(self.invoke(fixture))

    def test_dirty_untracked_conflicted_and_invalid_repository_refuse(self):
        for kind in ("tracked", "staged", "untracked", "conflicted", "not-git"):
            with self.subTest(kind=kind), self.fixture() as fixture:
                if kind in {"tracked", "staged"}:
                    fixture.write("docs/contracts/context-publication-v1.md", b"dirty\n")
                    if kind == "staged":
                        fixture.git("add", "docs/contracts/context-publication-v1.md")
                elif kind == "untracked":
                    fixture.write("untracked.txt")
                elif kind == "conflicted":
                    fixture.write("conflict.txt", b"base\n")
                    fixture.commit("Synthetic conflict base")
                    fixture.git("checkout", "-b", "side")
                    fixture.write("conflict.txt", b"side\n")
                    fixture.commit("Synthetic conflict side")
                    fixture.git("checkout", "fixture-main")
                    fixture.write("conflict.txt", b"main\n")
                    fixture.commit("Synthetic conflict main")
                    self.assertNotEqual(fixture.git("merge", "side", check=False).returncode, 0)
                else:
                    before = fixture.snapshot()
                    git_path, disabled = fixture.root / ".git", fixture.root / "git-disabled"
                    git_path.rename(disabled)
                    out, err = StringIO(), StringIO()
                    try:
                        with patch.object(publication, "ROOT", fixture.root), redirect_stdout(out), redirect_stderr(err):
                            code = publication.main(fixture.arguments())
                    finally:
                        disabled.rename(git_path)
                    self.assertEqual(fixture.snapshot(), before)
                    self.assertEqual((code, out.getvalue()), (1, ""))
                    self.assertTrue(err.getvalue())
                    self.assertNotIn("PUBLICATION_CHECK_NOT_IMPLEMENTED", err.getvalue())
                    continue
                self.refusal(self.invoke(fixture))
        with self.fixture() as fixture:
            fixture.write("local-evidence/ignored.txt")
            fixture.write(".venv/ignored.txt")
            self.success(self.invoke(fixture))

    def test_refs_require_commits_ancestry_checkout_head_and_no_option_injection(self):
        with self.fixture() as fixture:
            blob = fixture.git("rev-parse", "HEAD:tools/progress.py").stdout.decode().strip()
            for base, head in (("missing-ref", "HEAD"), ("--all", "HEAD"), (blob, "HEAD"),
                               (fixture.head, fixture.base), (fixture.base, "missing-ref"),
                               (fixture.base, "--help"), (fixture.base, blob)):
                with self.subTest(base=base, head=head):
                    arguments = fixture.arguments(base=base, head=head)
                    # Supply leading-dash refs as VALUES, so argparse does not consume
                    # them as usage errors before the ref-safety check under test.
                    for option in ("--base", "--head"):
                        index = arguments.index(option)
                        if arguments[index + 1].startswith("-"):
                            arguments[index:index + 2] = [option + "=" + arguments[index + 1]]
                    self.refusal(self.invoke(fixture, arguments))
            fixture.git("checkout", "-b", "side", fixture.base)
            fixture.write("side-only.txt")
            side = fixture.commit("Synthetic nonancestor")
            fixture.git("checkout", "fixture-main")
            self.refusal(self.invoke(fixture, fixture.arguments(base=side)))

    def test_required_arguments_unknown_duplicate_unstarted_stages_and_invalid_state(self):
        with self.fixture() as fixture:
            arguments = fixture.arguments()
            for option in ("--base", "--head", "--task", "--stage", "--schema-python"):
                with self.subTest(missing=option):
                    index = arguments.index(option)
                    result = self.invoke(fixture, arguments[:index] + arguments[index + 2:])
                    self.assertEqual((result[0], result[1]), (2, ""))
                    self.assertIn("usage:", result[2])
            for extra in (["--root", str(fixture.root)], ["--skip-schema"], ["--command", "true"]):
                result = self.invoke(fixture, arguments + extra)
                self.assertEqual((result[0], result[1]), (2, ""))
            for stages in (("contract", "contract"), ("unknown",), ("implementation",)):
                self.refusal(self.invoke(fixture, fixture.arguments(stages=stages)))
            unknown = list(arguments)
            unknown[unknown.index("--task") + 1] = "WFIX-404"
            self.refusal(self.invoke(fixture, unknown))
            fixture.task["depends_on"] = ["MISSING-999"]
            (fixture.root / "progress/state.json").write_text(json.dumps(fixture.data), encoding="utf-8")
            fixture.commit("Synthetic semantic-invalid state")
            self.refusal(self.invoke(fixture))

    def test_host_then_inline_schema_order_and_relative_interpreter_preserve_venv_path(self):
        with self.fixture() as fixture, tempfile.TemporaryDirectory() as caller:
            old_cwd = Path.cwd()
            try:
                os.chdir(caller)
                result = self.invoke(fixture, fixture.arguments(schema_python="venv/bin/python"))
            finally:
                os.chdir(old_cwd)
            self.success(result)
            children = result[3]
            for argv, script, tail in zip(children[:3],
                    ("tools/progress.py", "tools/progress.py", "tools/check_project.py"),
                    (["validate"], ["render", "--check"], [])):
                self.assertEqual(argv[:4], [os.path.abspath(sys.executable), "-E", "-s", "-B"])
                self.assertEqual((fixture.root / argv[4]).resolve(), fixture.root / script)
                self.assertEqual(argv[5:], tail)
            argv = children[3]
            self.assertEqual(argv[:5], [str(Path(caller) / "venv/bin/python"), "-I", "-S", "-B", "-c"])
            self.assertEqual(len(argv), 7)
            self.assertTrue(argv[5].strip(), "The schema driver is an inline stdlib program")
            self.assertEqual(argv[6], "tests/schema_validation/test_project_schemas.py")
        with self.fixture() as fixture:
            link = fixture.root / "local-evidence/schema-python"
            link.parent.mkdir(exist_ok=True)
            link.symlink_to(sys.executable)
            result = self.invoke(fixture, fixture.arguments(schema_python=link))
            self.success(result)
            self.assertEqual(result[3][3][0], str(link), "Do not resolve the selected venv executable symlink")

    def test_each_host_failure_stops_later_checks_and_schema_witness_is_exact(self):
        with self.fixture() as fixture:
            for index in range(4):
                with self.subTest(failed_step=index):
                    self.refusal(self.invoke(fixture, responses={index: (1, b"", b"synthetic failure")}),
                                 children=index + 1)
            for stdout in (b"", WITNESS.rstrip(), WITNESS + b"extra\n", b" " + WITNESS,
                           b'{"tests":0,"failures":0,"errors":0,"skipped":0}\n',
                           b'{"tests":1,"failures":0,"errors":0,"skipped":1}\n',
                           b'{"tests":1,"failures":1,"errors":0,"skipped":0}\n',
                           b'{"tests":1,"failures":0,"errors":1,"skipped":0}\n'):
                with self.subTest(witness=stdout):
                    self.refusal(self.invoke(fixture, responses={3: (0, stdout, b"unittest diagnostics")}), children=4)
            for index in (0, 3):
                self.refusal(self.invoke(fixture, responses={index: ValueError("synthetic unavailable/timeout/overflow")}),
                             children=index + 1)

    def test_patched_root_is_passed_to_semantic_validation(self):
        with self.fixture() as fixture:
            with patch.object(progress, "validate", wraps=progress.validate) as validate:
                self.success(self.invoke(fixture))
            self.assertTrue(any(call.kwargs.get("root") == fixture.root for call in validate.call_args_list),
                            "Default import-time root must not validate fixture bindings")


class PublicationRunnerTests(unittest.TestCase):
    def test_runner_uses_exact_scrubbed_env_root_binary_capture_and_deadline(self):
        poison = {"GIT_DIR": "/wrong/repo", "GIT_WORK_TREE": "/wrong/tree", "GIT_INDEX_FILE": "/wrong/index",
                  "GIT_OBJECT_DIRECTORY": "/wrong/objects", "GIT_CONFIG_COUNT": "1", "GIT_CONFIG_KEY_0": "x.y",
                  "GIT_CONFIG_VALUE_0": "z", "PYTHONPATH": "/wrong/python", "PYTHONHOME": "/wrong/home",
                  "LD_PRELOAD": "/wrong/loader", "PATH": "/wrong/bin", "SECRET_SENTINEL": "synthetic-only"}
        with tempfile.TemporaryDirectory() as directory, patch.dict(os.environ, poison):
            root = Path(directory)
            for git, argv in ((True, ["git", "status", "--porcelain=v1", "-z"]),
                              (False, [os.path.abspath(sys.executable), "-E", "-s", "-B", "tools/progress.py", "validate"]),
                              (False, ["/selected/venv/bin/python", "-I", "-S", "-B", "-c", "pass"])):
                with self.subTest(argv=argv):
                    actual = subprocess.CompletedProcess(argv, 7, b"raw\xffstdout", b"raw\xfestderr")
                    with patch.object(publication, "ROOT", root), patch.object(subprocess, "run", return_value=actual) as run:
                        observed = publication.run_step(argv, git=git)
                    self.assertEqual((observed.returncode, observed.stdout, observed.stderr),
                                     (7, b"raw\xffstdout", b"raw\xfestderr"))
                    args, kwargs = run.call_args
                    self.assertEqual(args[0] if args else kwargs["args"], argv)
                    self.assertEqual(kwargs["cwd"], root)
                    expected_env = dict(ENV, GIT_OPTIONAL_LOCKS="0", GIT_LITERAL_PATHSPECS="1") if git else ENV
                    self.assertEqual(kwargs["env"], expected_env)
                    self.assertEqual(kwargs["timeout"], 120)
                    self.assertFalse(kwargs.get("shell", False))
                    self.assertFalse(kwargs.get("text", False))
                    self.assertFalse(kwargs.get("check", False))
                    self.assertTrue(kwargs.get("capture_output") or
                                    (kwargs.get("stdout") == subprocess.PIPE and kwargs.get("stderr") == subprocess.PIPE))
                    self.assertIsNone(kwargs.get("encoding"))

    def test_runner_bounds_both_binary_streams_including_git_and_refuses_setup_failures(self):
        for git in (False, True):
            argv = ["git", "ls-tree", "-rz", "HEAD"] if git else ["/selected/python", "-V"]
            for stdout, stderr in ((b"x" * 1048576, b"y" * 1048576),
                                   (b"x" * 1048577, b""), (b"", b"y" * 1048577)):
                with self.subTest(git=git, lengths=(len(stdout), len(stderr))):
                    result = subprocess.CompletedProcess(argv, 0, stdout, stderr)
                    with patch.object(subprocess, "run", return_value=result):
                        if max(len(stdout), len(stderr)) == 1048576:
                            observed = publication.run_step(argv, git=git)
                            self.assertEqual((observed.returncode, observed.stdout, observed.stderr),
                                             (0, stdout, stderr))
                        else:
                            with self.assertRaises(ValueError):
                                publication.run_step(argv, git=git)
            for error in (FileNotFoundError("synthetic missing"), PermissionError("synthetic unavailable"),
                          subprocess.TimeoutExpired(argv, 120, output=b"partial")):
                with self.subTest(git=git, failure=type(error).__name__), \
                     patch.object(subprocess, "run", side_effect=error) as run:
                    with self.assertRaises(ValueError):
                        publication.run_step(argv, git=git)
                    self.assertEqual(run.call_count, 1, "Fail without retry")


if __name__ == "__main__":
    unittest.main()
