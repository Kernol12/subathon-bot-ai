# SPDX-License-Identifier: GPL-3.0-or-later
"""Reviewed-contract feature candidates; import/discovery never invokes a feature.

Expected text is independently hand-authored. Qt success uses the real supplied
compiler and independent native raw-QM verifier. Disposable child programs below
exercise ONLY failure paths; they are never alternative successful compilers.
"""
from __future__ import annotations

import copy
import dataclasses
import hashlib
import io
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "tests/fixtures/translation_catalog"
sys.path.insert(0, str(ROOT))
from tools import translation_catalog as catalog  # noqa: E402; inert declarations only

TEXT_PATHS = ("qt/messages_de.ts", "qt/messages_en.ts", "web/de.json", "web/en.json")
BINARY_PATHS = ("messages_de.qm", "messages_en.qm")
HELP = (
    "usage: translation_catalog.py {validate,generate,check} ...\n"
    "validate --source PATH\n"
    "generate --source PATH --text-output DIR --binary-output DIR --lrelease PATH\n"
    "check --source PATH --text-output DIR --binary-output DIR --lrelease PATH\n"
)


def document() -> dict:
    return json.loads((FIXTURES / "source.json").read_bytes())


def minimal() -> dict:
    return {
        "schema_version": 1, "license": "GPL-3.0-or-later", "source_locale": "en",
        "locales": ["en", "de"], "messages": [{
            "id": "fixture.single", "description": "Synthetic text.", "parameters": {},
            "plural": None, "translations": {"en": {"text": "x"}, "de": {"text": "y"}},
        }],
    }


def encoded(value: object) -> bytes:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


class Assertions(unittest.TestCase):
    def failure(self, expected: str, call, *, stage=None, reason=None, tool_exit=None):
        # The distinct TranslationCatalogNotImplemented marker cannot satisfy this.
        with self.assertRaises(catalog.CatalogFailure) as caught:
            call()
        error = caught.exception
        self.assertEqual(error.code.value, expected)
        self.assertEqual(str(error), f"Translation catalog failure: {expected}")
        if stage is not None:
            self.assertEqual(error.stage, stage)
        if reason is not None:
            self.assertEqual(error.tool_reason, reason)
        if tool_exit is not None:
            self.assertEqual(error.tool_exit, tool_exit)
        return error


class CatalogValues(Assertions):
    def test_complete_owned_sorted_dto(self):
        result = catalog.validate_catalog((FIXTURES / "source.json").read_bytes())
        self.assertIsInstance(result, catalog.Catalog)
        self.assertIsInstance(result.messages, tuple)
        self.assertEqual(tuple(m.message_id for m in result.messages), (
            "fixture.greeting", "fixture.items", "fixture.literal", "fixture.literal_plural", "fixture.reorder",
            "fixture.scalars", "fixture.selector_only"))
        expected = (
            ("Fixture greeting; name is untrusted plain text.", (("name", "text"),), None,
             "Hello, {name}!", "Hallo, {name}!"),
            ("Fixture count with reordered named values.", (("count", "integer"), ("name", "text")), "count",
             ("{count} item for {name}", "{count} items for {name}"),
             ("{name}: {count} Eintrag", "{name}: {count} Einträge")),
            ("Literal braces and Qt-looking percent sequences.", (("name", "text"),), None,
             "{{x}} %n %1 %L1 %% {name}", "{{x}} %n %1 %L1 %% {name}"),
            ("Literal percents with an invisible integer selector.", (("count", "integer"), ("name", "text")), "count",
             ("{{x}} %n %1 %L1 %% {name}", "{{x}} %n %1 %L1 %% {name}"),
             ("{{x}} %n %1 %L1 %% {name}", "{{x}} %n %1 %L1 %% {name}")),
            ("Named values reorder and repeat across languages.", (("first", "text"), ("second", "text")), None,
             "{first} then {second}; {first}", "{second}, danach {first}; {first}"),
            ("Exact XML scalars and line controls.", (), None,
             "A & B < C > D \"q\" 's'\r\n\tGrüße e\u0301 😀",
             "A & B < C > D \"q\" 's'\r\n\tGrüße e\u0301 😀"),
            ("Selector is declared even when absent from visible text.", (("count", "integer"), ("name", "text")), "count",
             ("One item for {name}", "Items for {name}"),
             ("Ein Eintrag für {name}", "Einträge für {name}")),
        )
        for message, (description, params, plural, en, de) in zip(result.messages, expected):
            self.assertEqual(message.description, description)
            self.assertEqual(tuple((p.name, p.kind.value) for p in message.parameters), params)
            self.assertIsInstance(message.parameters, tuple)
            self.assertEqual(message.plural, plural)
            for forms, value in ((message.english, en), (message.german, de)):
                if isinstance(value, tuple):
                    self.assertEqual(forms, catalog.PluralForms(*value))
                else:
                    self.assertEqual(forms, catalog.TextForms(value))
            with self.assertRaises(dataclasses.FrozenInstanceError):
                message.description = "mutated"
        with self.assertRaises(dataclasses.FrozenInstanceError):
            result.messages = ()

    def test_four_complete_frozen_text_outputs(self):
        actual = catalog.render_text((FIXTURES / "source.json").read_bytes())
        self.assertIsInstance(actual, tuple)
        self.assertEqual(tuple(a.relative_path for a in actual), TEXT_PATHS)
        for artifact in actual:
            self.assertIsInstance(artifact.data, bytes)
            self.assertEqual(artifact.data, (FIXTURES / "goldens" / artifact.relative_path).read_bytes())
            with self.assertRaises(dataclasses.FrozenInstanceError):
                artifact.data = b"mutated"

    def test_reordered_input_is_byte_identical(self):
        value = document()
        value["messages"].reverse()
        for message in value["messages"]:
            message["parameters"] = dict(reversed(tuple(message["parameters"].items())))
            message["translations"] = dict(reversed(tuple(message["translations"].items())))
        value = dict(reversed(tuple(value.items())))
        actual = catalog.render_text(json.dumps(value, ensure_ascii=True, indent=3).encode())
        self.assertEqual(tuple(a.relative_path for a in actual), TEXT_PATHS)
        for artifact in actual:
            self.assertEqual(artifact.data, (FIXTURES / "goldens" / artifact.relative_path).read_bytes())

    def test_rejects_nonbytes_before_parsing(self):
        for value in (None, "{}", bytearray(b"{}"), memoryview(b"{}"), 1):
            with self.subTest(kind=type(value).__name__):
                self.failure("invalid_schema", lambda: catalog.validate_catalog(value))

    def test_encoding_and_surrogate_failures(self):
        for value in (b"\xff", b"\xef\xbb\xbf{}", b'"\\ud800"', b'{"a":"\\udfff"}'):
            with self.subTest(data=value):
                self.failure("invalid_encoding", lambda: catalog.validate_catalog(value))

    def test_json_syntax_constants_and_duplicate_keys(self):
        cases = ((b"{", "invalid_json"), (b"{} trailing", "invalid_json"),
                 (b'{"v":NaN}', "invalid_json"), (b'{"v":Infinity}', "invalid_json"),
                 (b'{"v":-Infinity}', "invalid_json"), (b'{"v":1,"v":1}', "duplicate_key"),
                 (b'{"a":{"v":1,"v":2}}', "duplicate_key"))
        for value, code in cases:
            with self.subTest(data=value):
                self.failure(code, lambda: catalog.validate_catalog(value))

    def test_integer_token_kind_and_large_numeric_token(self):
        source = encoded(minimal())
        for spelling in (b"true", b"1.0", b'"1"', b"1e0", b"9" * 5000):
            value = source.replace(b'"schema_version":1', b'"schema_version":' + spelling, 1)
            with self.subTest(token=spelling[:8]):
                self.failure("invalid_schema", lambda: catalog.validate_catalog(value))

    def test_source_and_depth_bounds_precede_schema(self):
        self.failure("resource_limit", lambda: catalog.validate_catalog(b"\xff" * (4194304 + 1)))
        self.failure("resource_limit", lambda: catalog.validate_catalog(b"[" * 9 + b"0" + b"]" * 9))
        self.failure("invalid_schema", lambda: catalog.validate_catalog(b"[" * 8 + b"0" + b"]" * 8))

    def test_root_and_message_shape_are_strict(self):
        values = []
        for key, replacement in (("schema_version", 2), ("license", "MIT"), ("source_locale", "de"),
                                 ("locales", ["de", "en"]), ("messages", [])):
            value = minimal(); value[key] = replacement; values.append(value)
        value = minimal(); value["unknown"] = 1; values.append(value)
        value = minimal(); del value["license"]; values.append(value)
        value = minimal(); value["messages"][0]["unknown"] = 1; values.append(value)
        for value in values:
            with self.subTest(value=value):
                self.failure("invalid_schema", lambda: catalog.validate_catalog(encoded(value)))

    def test_ids_and_duplicate_identity(self):
        for bad in ("", "A.upper", "a..b", "../escape", "a-b", "a_", "ümlaut"):
            value = minimal(); value["messages"][0]["id"] = bad
            with self.subTest(identity=bad):
                self.failure("invalid_schema", lambda: catalog.validate_catalog(encoded(value)))
        value = minimal(); value["messages"] *= 2
        self.failure("duplicate_id", lambda: catalog.validate_catalog(encoded(value)))

    def test_declared_byte_and_collection_ceilings(self):
        cases = []
        value = minimal(); value["messages"][0]["id"] = "a" * 161; cases.append(value)
        value = minimal(); value["messages"][0]["description"] = "ü" * 2049; cases.append(value)
        value = minimal(); value["messages"][0]["translations"]["en"]["text"] = "ü" * 32769; cases.append(value)
        value = minimal(); value["messages"][0]["parameters"] = {f"p{n}": "text" for n in range(33)}; cases.append(value)
        value = minimal(); value["messages"] = [dict(value["messages"][0], id=f"a.x{n}") for n in range(8193)]; cases.append(value)
        for value in cases:
            with self.subTest(size=len(encoded(value))):
                self.failure("resource_limit", lambda: catalog.validate_catalog(encoded(value)))

    def test_exact_valid_field_boundaries(self):
        value = minimal(); message = value["messages"][0]
        message["id"] = "a" * 160; message["description"] = "ü" * 2048
        message["translations"]["en"]["text"] = "ü" * 32768
        actual = catalog.validate_catalog(encoded(value))
        self.assertEqual(actual.messages[0].message_id, "a" * 160)
        self.assertEqual(actual.messages[0].english.text, "ü" * 32768)
        value = minimal(); value["messages"][0]["parameters"] = {f"p{n}": "text" for n in range(32)}
        body = " ".join("{" + f"p{n}" + "}" for n in range(32))
        value["messages"][0]["translations"] = {"en": {"text": body}, "de": {"text": body}}
        self.assertEqual(len(catalog.validate_catalog(encoded(value)).messages[0].parameters), 32)

    def test_parameter_and_selector_declarations(self):
        for params, plural, code in (({"Bad": "text"}, None, "invalid_schema"),
                                    ({"a": "number"}, None, "invalid_schema"),
                                    ({"a": True}, None, "invalid_schema"),
                                    ({"a": "text"}, "a", "invalid_plural"),
                                    ({}, "absent", "invalid_plural")):
            value = minimal(); value["messages"][0].update(parameters=params, plural=plural)
            with self.subTest(params=params, plural=plural):
                self.failure(code, lambda: catalog.validate_catalog(encoded(value)))

    def test_coverage_and_form_shape_categories(self):
        cases = (({}, "coverage"), ({"en": {"text": "x"}, "de": None}, "coverage"),
                 ({"en": {"text": "x"}, "de": {"text": "y"}, "fr": {"text": "z"}}, "coverage"),
                 ({"en": {"text": "x"}, "de": {"text": ""}}, "coverage"),
                 ({"en": {"text": "x"}, "de": {"one": "y", "other": "z"}}, "invalid_plural"),
                 ({"en": {"text": "x"}, "de": {"text": 7}}, "invalid_schema"))
        for translations, code in cases:
            value = minimal(); value["messages"][0]["translations"] = translations
            with self.subTest(code=code, translations=translations):
                self.failure(code, lambda: catalog.validate_catalog(encoded(value)))
        value = document(); value["messages"] = [next(m for m in value["messages"] if m["id"] == "fixture.items")]
        for forms in ({"one": "{count} for {name}"}, {"one": "{count} for {name}", "other": "{count} for {name}", "few": "x"}):
            value["messages"][0]["translations"]["de"] = forms
            self.failure("invalid_plural", lambda: catalog.validate_catalog(encoded(value)))

    def test_template_grammar_and_declared_name_agreement(self):
        for body, code in (("{", "invalid_template"), ("}", "invalid_template"), ("{}", "invalid_template"),
                           ("{ name}", "invalid_template"), ("{name!r}", "invalid_template"),
                           ("{name:03}", "invalid_template"), ("{{{", "invalid_template"),
                           ("{missing}", "parameter_mismatch"), ("none", "parameter_mismatch")):
            value = minimal(); message = value["messages"][0]
            message["parameters"] = {"name": "text"}; message["translations"]["de"]["text"] = body
            with self.subTest(body=body):
                self.failure(code, lambda: catalog.validate_catalog(encoded(value)))

    def test_illegal_xml_scalars_are_not_replaced(self):
        for scalar in ("\0", "\x01", "\x0b", "\ufffe", "\uffff"):
            value = minimal(); value["messages"][0]["translations"]["en"]["text"] = "x" + scalar
            with self.subTest(scalar=ord(scalar)):
                self.failure("invalid_template", lambda: catalog.validate_catalog(encoded(value)))

    def test_validation_priority_and_sorted_error_identity(self):
        value = minimal(); message = value["messages"][0]
        message["id"] = "../invalid"; message["translations"] = {}
        self.failure("invalid_schema", lambda: catalog.validate_catalog(encoded(value)))
        value = minimal(); value["messages"][0]["parameters"] = {"bad": "wrong"}; value["messages"][0]["translations"] = {}
        self.failure("invalid_schema", lambda: catalog.validate_catalog(encoded(value)))
        value = minimal(); value["messages"][0]["translations"] = {"en": {"text": "{"}}
        self.failure("coverage", lambda: catalog.validate_catalog(encoded(value)))
        value = minimal(); a = value["messages"][0]; a["id"] = "z.last"; a["translations"]["de"]["text"] = "{"
        b = copy.deepcopy(a); b["id"] = "a.first"; value["messages"].append(b)
        error = self.failure("invalid_template", lambda: catalog.validate_catalog(encoded(value)))
        self.assertEqual(error.message_id, "a.first")
        self.assertEqual(error.locale, "de")


class FileFixture(Assertions):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="translation-contract-")
        self.addCleanup(self.temporary.cleanup)
        self.home = Path(self.temporary.name)
        self.source = self.home / "private-source.json"
        self.source.write_bytes((FIXTURES / "source.json").read_bytes())
        self.text = self.home / "text output"
        self.binary = self.home / "binary output"
        self.tool = os.environ.get("SUBATHON_TRANSLATION_LRELEASE", "")
        self.probe = os.environ.get("SUBATHON_TRANSLATION_PROBE", "")
        self.assertTrue(Path(self.tool).is_absolute() and Path(self.tool).is_file(), "actual Qt tool env required")
        self.assertTrue(Path(self.probe).is_absolute() and Path(self.probe).is_file(), "actual Qt-only probe env required")

    def request(self, **changes):
        value = catalog.BuildRequest(str(self.source), str(self.text), str(self.binary), self.tool)
        return dataclasses.replace(value, **changes)

    def tree(self):
        result = []
        for path in sorted(self.home.rglob("*")):
            info = path.lstat()
            body = path.read_bytes() if stat.S_ISREG(info.st_mode) and info.st_size <= 1048576 else None
            result.append((str(path.relative_to(self.home)), info.st_mode, info.st_ino, info.st_mtime_ns, info.st_size, body))
        return result

    def existing(self):
        for name in TEXT_PATHS:
            path = self.text / name; path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes((FIXTURES / "goldens" / name).read_bytes())
        self.binary.mkdir()
        for locale in ("en", "de"):
            run = subprocess.run([self.tool, "-fail-on-unfinished", str(self.text / f"qt/messages_{locale}.ts"),
                                  "-qm", str(self.binary / f"messages_{locale}.qm")], capture_output=True, timeout=15)
            self.assertEqual(run.returncode, 0, "old-only real Qt fixture arrangement must succeed")

    def verify_result(self, actual):
        self.assertIsInstance(actual, catalog.BuildResult)
        self.assertEqual((actual.message_count, actual.qt_version), (7, "6.12.0"))
        self.assertEqual(tuple(d.relative_path for d in actual.text), TEXT_PATHS)
        self.assertEqual(tuple(d.relative_path for d in actual.binary), BINARY_PATHS)
        for collection, root in ((actual.text, self.text), (actual.binary, self.binary)):
            self.assertIsInstance(collection, tuple)
            for summary in collection:
                data = (root / summary.relative_path).read_bytes()
                self.assertEqual((summary.byte_count, summary.sha256), (len(data), hashlib.sha256(data).hexdigest()))
        for name in TEXT_PATHS:
            self.assertEqual((self.text / name).read_bytes(), (FIXTURES / "goldens" / name).read_bytes())
        run = subprocess.run([self.probe, "verify", str(self.binary)], capture_output=True, timeout=15)
        self.assertEqual(run.returncode, 0, "actual generated QMs must pass independent literal verifier")

    def child(self, mode: str):
        # Real error-only executable. Unaffected tool calls exec the actual Qt binary.
        path = self.home / f"tool-{mode}.py"
        log = self.home / "child-log.jsonl"
        script = (
            f"#!{sys.executable}\nimport os,sys,time,json\nfrom pathlib import Path\n"
            f"actual={self.tool!r}; mode={mode!r}; log=Path({str(log)!r})\n"
            f"source=Path({str(self.source)!r}); managed=Path({str(self.text / 'web')!r}); parent=Path({str(self.home)!r})\n"
            "with log.open('a') as f: f.write(json.dumps({'pid':os.getpid(),'args':sys.argv[1:]})+'\\n')\n"
            "if sys.argv[1:]==['-version']:\n"
            " if mode=='badversion': print('lrelease version 6.12.1');sys.exit(0)\n"
            " if mode=='malformed': print('Qt 6.12.0');sys.exit(0)\n"
            " if mode=='versionexit': print('PRIVATE CHILD BODY',file=sys.stderr);sys.exit(19)\n"
            " os.execv(actual,[actual]+sys.argv[1:])\n"
            "if mode=='deexit' and not sys.argv[2].endswith('messages_de.ts'):\n"
            " os.execv(actual,[actual]+sys.argv[1:])\n"
            "if mode in ('sourcechange','layoutchange'):\n"
            " if sys.argv[2].endswith('messages_en.ts'):\n"
            "  if mode=='sourcechange':\n"
            "   with source.open('ab') as f: f.write(b' ')\n"
            "  else: managed.parent.mkdir(parents=True,exist_ok=True);managed.symlink_to(parent,target_is_directory=True)\n"
            " os.execv(actual,[actual]+sys.argv[1:])\n"
            "if mode in ('exit','deexit'): print('PRIVATE CHILD BODY',file=sys.stderr);sys.exit(23)\n"
            "if mode=='sleep': time.sleep(60);sys.exit(24)\n"
            "if mode=='output':\n"
            " sys.stdout.write('a'*40000);sys.stdout.flush();sys.stderr.write('b'*40000);sys.stderr.flush();time.sleep(60);sys.exit(25)\n"
            "out=Path(sys.argv[sys.argv.index('-qm')+1])\n"
            "if mode=='missing': sys.exit(0)\n"
            "if mode=='empty': out.touch();sys.exit(0)\n"
            "if mode=='directory': out.mkdir();sys.exit(0)\n"
            "if mode=='fifo': os.mkfifo(out);sys.exit(0)\n"
            "if mode=='symlink': out.symlink_to(__file__);sys.exit(0)\n"
            "if mode=='oversize':\n"
            " with out.open('wb') as f: f.truncate(67108865)\n"
            " sys.exit(0)\n"
            "raise AssertionError('no alternative successful compiler')\n"
        )
        path.write_text(script); path.chmod(0o700)
        return path, log

class CatalogFiles(FileFixture):
    def test_generate_six_artifacts_for_seven_messages(self):
        original = self.source.read_bytes()
        result = catalog.generate_catalog(self.request())
        self.verify_result(result)
        self.assertEqual(self.source.read_bytes(), original)
        with self.assertRaises(dataclasses.FrozenInstanceError):
            result.message_count = 8

    def test_two_fresh_generations_reproduce_real_qm_bytes(self):
        catalog.generate_catalog(self.request())
        first = tuple((self.binary / n).read_bytes() for n in BINARY_PATHS)
        other = self.request(text_output=str(self.home / "second-text"), binary_output=str(self.home / "second-bin"))
        catalog.generate_catalog(other)
        self.assertEqual(tuple((Path(other.binary_output) / n).read_bytes() for n in BINARY_PATHS), first)

    def test_check_success_is_read_only_and_checks_actual_qm(self):
        self.existing(); before = self.tree()
        result = catalog.check_catalog(self.request())
        self.assertEqual(self.tree(), before)
        self.verify_result(result)

    def test_check_byte_stale_missing_and_binary_corruption(self):
        self.existing()
        for name, root in ((TEXT_PATHS[0], self.text), (TEXT_PATHS[3], self.text), (BINARY_PATHS[0], self.binary)):
            path = root / name; original = path.read_bytes()
            path.write_bytes(bytes([original[0] ^ 1]) + original[1:])
            before = self.tree()
            self.failure("generated_mismatch", lambda: catalog.check_catalog(self.request()), stage="comparison")
            self.assertEqual(self.tree(), before)
            path.write_bytes(original)
        (self.binary / BINARY_PATHS[1]).unlink(); before = self.tree()
        self.failure("generated_mismatch", lambda: catalog.check_catalog(self.request()))
        self.assertEqual(self.tree(), before)

    def test_check_absent_roots_are_not_created(self):
        before = self.tree()
        self.failure("generated_mismatch", lambda: catalog.check_catalog(self.request()))
        self.assertEqual(self.tree(), before)

    def test_extra_owned_files_refuse_both_modes_without_deletion(self):
        for name in ("qt/messages_fr.ts", "web/fr.json"):
            path = self.text / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(b"extra")
            before = self.tree()
            for operation in (catalog.generate_catalog, catalog.check_catalog):
                self.failure("generated_mismatch", lambda: operation(self.request()))
                self.assertEqual(self.tree(), before)
            path.unlink()
        self.binary.mkdir(); (self.binary / "messages_old.qm").write_bytes(b"extra"); before = self.tree()
        self.failure("generated_mismatch", lambda: catalog.generate_catalog(self.request()))
        self.assertEqual(self.tree(), before)

    def test_unrelated_files_and_subtrees_are_preserved(self):
        path = self.text / "qt/notes.txt"; path.parent.mkdir(parents=True); path.write_bytes(b"untouched")
        nested = self.text / "notes/messages_old.qm"; nested.parent.mkdir(); nested.write_bytes(b"nested")
        catalog.generate_catalog(self.request())
        self.assertEqual(path.read_bytes(), b"untouched"); self.assertEqual(nested.read_bytes(), b"nested")
        self.verify_result(catalog.check_catalog(self.request()))

    def test_generate_repairs_stale_expected_files(self):
        self.existing()
        (self.text / TEXT_PATHS[0]).write_bytes(b"old"); (self.binary / BINARY_PATHS[0]).write_bytes(b"old")
        self.verify_result(catalog.generate_catalog(self.request()))

    def test_request_types_and_timeout_before_paths(self):
        self.failure("invalid_schema", lambda: catalog.generate_catalog(None), stage="input")
        for timeout in (0, 120001, True, 1.5, "1"):
            request = self.request(source="", tool_timeout_ms=timeout)
            self.failure("invalid_schema", lambda: catalog.generate_catalog(request), stage="input")
        for value in (None, b"path", Path("path"), "", "bad\0path", "bad\ud800"):
            request = self.request(source=value)
            self.failure("io_failure", lambda: catalog.generate_catalog(request), stage="paths")

    def test_source_validation_precedes_tool_and_output_creation(self):
        self.source.write_bytes(b'{"x":NaN}')
        before = self.tree()
        self.failure("invalid_json", lambda: catalog.generate_catalog(self.request(lrelease=str(self.home / "missing"))))
        self.assertEqual(self.tree(), before)

    def test_nonregular_or_missing_source_is_not_opened(self):
        fifo = self.home / "source-fifo"; os.mkfifo(fifo)
        for source in (fifo, self.home, self.home / "absent"):
            self.failure("io_failure", lambda: catalog.generate_catalog(self.request(source=str(source))), stage="input")
        self.assertFalse(self.text.exists()); self.assertFalse(self.binary.exists())

    def test_disjoint_roots_source_and_tool_publication_safety(self):
        requests = (self.request(binary_output=str(self.text)),
                    self.request(binary_output=str(self.text / "nested")),
                    self.request(text_output=str(self.home)),
                    self.request(lrelease=str(self.text / "compiler")))
        before = self.tree()
        for request in requests:
            self.failure("io_failure", lambda: catalog.generate_catalog(request))
            self.assertEqual(self.tree(), before)

    def test_output_source_hardlink_and_managed_symlink_rejected(self):
        (self.text / "qt").mkdir(parents=True)
        target = self.text / "qt/messages_en.ts"; os.link(self.source, target)
        before = self.tree()
        self.failure("io_failure", lambda: catalog.generate_catalog(self.request()))
        self.assertEqual(self.tree(), before); target.unlink()
        target.symlink_to(self.source); before = self.tree()
        self.failure("io_failure", lambda: catalog.generate_catalog(self.request()))
        self.assertEqual(self.tree(), before); target.unlink()
        (self.text / "web").symlink_to(self.home, target_is_directory=True)
        before = self.tree()
        self.failure("io_failure", lambda: catalog.generate_catalog(self.request()))
        self.assertEqual(self.tree(), before)

    def test_matching_nonregular_outputs_reject_before_tool(self):
        (self.text / "qt").mkdir(parents=True)
        for name, kind in (("messages_en.ts", "fifo"), ("messages_fr.ts", "directory")):
            target = self.text / "qt" / name
            if kind == "fifo": os.mkfifo(target)
            else: target.mkdir()
            before = self.tree()
            self.failure("io_failure", lambda: catalog.generate_catalog(self.request()))
            self.assertEqual(self.tree(), before)
            if kind == "fifo": target.unlink()
            else: target.rmdir()

    def test_symbolic_source_resolving_to_regular_file_is_supported(self):
        alias = self.home / "source-link.json"; alias.symlink_to(self.source)
        result = catalog.generate_catalog(self.request(source=str(alias)))
        self.verify_result(result)
        self.assertTrue(alias.is_symlink())

    def test_tool_unavailable_and_version_failures_are_truthful(self):
        self.failure("tool_unavailable", lambda: catalog.generate_catalog(self.request(lrelease=str(self.home / "absent"))), stage="version")
        for mode, reason, exit_code in (("badversion", "version_mismatch", 0), ("malformed", "malformed_version", 0), ("versionexit", "exit", 19)):
            tool, _ = self.child(mode)
            self.failure("tool_failed" if mode == "versionexit" else "tool_version",
                         lambda: catalog.generate_catalog(self.request(lrelease=str(tool))),
                         stage="version", reason=reason, tool_exit=exit_code)
        self.assertFalse(self.text.exists()); self.assertFalse(self.binary.exists())

    def test_compile_nonzero_and_zero_exit_bad_outputs_never_publish(self):
        cases = (("exit", "exit", 23), ("deexit", "exit", 23), ("missing", "missing_output", 0),
                 ("empty", "missing_output", 0), ("directory", "invalid_output", 0),
                 ("fifo", "invalid_output", 0), ("symlink", "invalid_output", 0),
                 ("oversize", "output_file_limit", 0))
        for mode, reason, exit_code in cases:
            tool, _ = self.child(mode)
            with self.subTest(mode=mode):
                self.failure("tool_failed", lambda: catalog.generate_catalog(self.request(lrelease=str(tool))),
                             stage="de" if mode == "deexit" else "en", reason=reason, tool_exit=exit_code)
                self.assertFalse(self.text.exists()); self.assertFalse(self.binary.exists())

    def test_process_timeout_and_combined_capture_limit_reap_child(self):
        for mode, reason in (("sleep", "timeout"), ("output", "output_limit")):
            tool, log = self.child(mode)
            with self.subTest(mode=mode):
                error = self.failure("tool_failed", lambda: catalog.generate_catalog(self.request(
                    lrelease=str(tool), tool_timeout_ms=2000)), stage="en", reason=reason)
                self.assertIn(error.tool_exit, (-15, -9))
                child_pid = json.loads(log.read_text().splitlines()[-1])["pid"]
                with self.assertRaises(ProcessLookupError): os.kill(child_pid, 0)
                self.assertFalse(self.text.exists()); self.assertFalse(self.binary.exists())

    def test_source_and_layout_rechecked_after_real_compilation(self):
        original = self.source.read_bytes()
        tool, _ = self.child("sourcechange")
        self.failure("io_failure", lambda: catalog.generate_catalog(self.request(lrelease=str(tool))))
        self.assertEqual(self.source.read_bytes(), original + b" ")
        self.assertFalse(self.text.exists()); self.assertFalse(self.binary.exists())
        self.source.write_bytes(original)
        tool, _ = self.child("layoutchange")
        self.failure("io_failure", lambda: catalog.generate_catalog(self.request(lrelease=str(tool))))
        self.assertTrue((self.text / "web").is_symlink())
        self.assertEqual(self.source.read_bytes(), original)
        self.assertFalse(self.binary.exists())
        self.assertFalse((self.text / "qt").exists())

    def test_oversized_existing_target_is_mismatch_without_mutation(self):
        self.existing()
        target = self.binary / "messages_en.qm"
        with target.open("wb") as file: file.truncate(67108865)
        before = self.tree()
        self.failure("generated_mismatch", lambda: catalog.check_catalog(self.request()), stage="comparison")
        self.assertEqual(self.tree(), before)


class CatalogCLI(FileFixture):
    def cli(self, *arguments):
        return subprocess.run([sys.executable, str(ROOT / "tools/translation_catalog.py"), *arguments],
                              capture_output=True, timeout=20)

    def record(self, result, expected_exit, *, code=None):
        self.assertEqual(result.returncode, expected_exit)
        stream = result.stderr if code else result.stdout
        self.assertEqual(result.stdout if code else result.stderr, b"")
        self.assertTrue(stream.endswith(b"\n")); self.assertEqual(stream.count(b"\n"), 1)
        value = json.loads(stream)
        self.assertEqual(stream, (json.dumps(value, ensure_ascii=True, separators=(",", ":")) + "\n").encode())
        if code:
            self.assertEqual(tuple(value), ("status", "code", "message_id", "locale", "parameter", "stage", "tool_exit", "tool_reason"))
            self.assertEqual((value["status"], value["code"]), ("error", code)); self.assertLessEqual(len(stream), 768)
        else:
            self.assertEqual(tuple(value), ("status", "operation", "message_count", "qt_version", "text", "binary"))
            self.assertEqual((value["status"], value["message_count"]), ("ok", 7))
        self.assertNotIn(str(self.home).encode(), stream)
        self.assertNotIn(b"PRIVATE CHILD BODY", stream)
        self.assertNotIn(b"Hallo", stream)
        return value

    def test_cli_help_version_and_direct_main_metadata(self):
        for args, expected in ((("--help",), HELP), (("--version",), "translation-catalog 1\n")):
            result = self.cli(*args)
            self.assertEqual((result.returncode, result.stdout, result.stderr), (0, expected.encode(), b""))
            out, err = io.StringIO(), io.StringIO()
            with redirect_stdout(out), redirect_stderr(err): status = catalog.main(list(args))
            self.assertEqual((status, out.getvalue(), err.getvalue()), (0, expected, ""))

    def test_cli_validate_exact_record_and_no_translation_body(self):
        value = self.record(self.cli("validate", "--source", str(self.source)), 0)
        self.assertEqual(value, {"status": "ok", "operation": "validate", "message_count": 7,
                                 "qt_version": None, "text": [], "binary": []})

    def test_cli_generate_check_real_binaries_and_digest_records(self):
        flags = ("--source", str(self.source), "--text-output", str(self.text),
                 "--binary-output", str(self.binary), "--lrelease", self.tool)
        value = self.record(self.cli("generate", *flags), 0)
        self.assertEqual((value["operation"], value["qt_version"]), ("generate", "6.12.0"))
        for summaries, paths, root in ((value["text"], TEXT_PATHS, self.text), (value["binary"], BINARY_PATHS, self.binary)):
            self.assertEqual(tuple(d["path"] for d in summaries), paths)
            for summary in summaries:
                self.assertEqual(tuple(summary), ("path", "bytes", "sha256"))
                body = (root / summary["path"]).read_bytes()
                self.assertEqual((summary["bytes"], summary["sha256"]), (len(body), hashlib.sha256(body).hexdigest()))
        native = subprocess.run([self.probe, "verify", str(self.binary)], capture_output=True, timeout=15)
        self.assertEqual(native.returncode, 0)
        before = self.tree(); result = self.record(self.cli("check", *flags), 0)
        self.assertEqual(result["operation"], "check"); self.assertEqual(self.tree(), before)

    def test_cli_usage_errors_precede_missing_input(self):
        cases = ((), ("unknown",), ("--help", "validate"), ("validate", "--source"),
                 ("validate", "--source", "absent", "--source", "absent"),
                 ("validate", "--source", "absent", "--lrelease", "absent"),
                 ("generate", "--source", "absent"), ("validate", "--unknown"),
                 ("generate", "--tool-timeout-ms", "1"))
        before = self.tree()
        for args in cases:
            with self.subTest(arguments=args):
                self.record(self.cli(*args), 64, code="invalid_cli_arguments")
                self.assertEqual(self.tree(), before)

    def test_cli_validation_io_and_tool_failures_are_bounded(self):
        self.source.write_bytes(b'{"secret":NaN}')
        self.record(self.cli("validate", "--source", str(self.source)), 65, code="invalid_json")
        self.record(self.cli("validate", "--source", str(self.home / "private-absent")), 74, code="io_failure")
        self.source.write_bytes((FIXTURES / "source.json").read_bytes())
        for mode, expected_exit, code in (("badversion", 69, "tool_version"), ("exit", 70, "tool_failed")):
            tool, _ = self.child(mode)
            result = self.cli("generate", "--source", str(self.source), "--text-output", str(self.text),
                              "--binary-output", str(self.binary), "--lrelease", str(tool))
            self.record(result, expected_exit, code=code)
        self.assertFalse(self.text.exists()); self.assertFalse(self.binary.exists())

    def test_cli_check_mismatch_has_exit_one_and_no_target_changes(self):
        self.existing(); (self.text / TEXT_PATHS[0]).write_bytes(b"stale")
        before = self.tree()
        self.record(self.cli("check", "--source", str(self.source), "--text-output", str(self.text),
                             "--binary-output", str(self.binary), "--lrelease", self.tool),
                    1, code="generated_mismatch")
        self.assertEqual(self.tree(), before)


if __name__ == "__main__":
    unittest.main()
