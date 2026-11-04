# Translation catalog — concrete Python/tool API proposal v1

LOCALETEST-001 API DOCUMENT stage only. Authority: specification v0.45 §12/I18N-04
and [catalog contract](translation-catalog-v1.md), including the actual supported
explicit EMPTY context/name correction273c6e7, separately re-approved947eba4
(root dfd53cd, source blob51383296). The earlier approved top-level
message serialization was contradicted by actual Qt6.12 and is NOT usable.
Different actual review of the corrected contract AND this API precedes Python
source, schemas, stubs, fixtures/tests, new native harness or production behavior.
This document defines no implemented generator or accepted baseline.

## 1. Small public surface and immutable owned data

Proposed module tools/translation_catalog.py, Python >=3.10 standard library only.
No Qt Python binding, package installation, public fake-process adapter, application
server or UI dependency is required by this module. All source/output data below
is immutable: frozen dataclasses with slots, tuples, bytes, str and finite enums;
no nested list/dict or caller-owned mutable view escapes. Proposed declarations:

~~~python
class ParameterKind(str, Enum):
    TEXT = "text"
    INTEGER = "integer"

@dataclass(frozen=True, slots=True)
class Parameter:
    name: str
    kind: ParameterKind

@dataclass(frozen=True, slots=True)
class TextForms:
    text: str

@dataclass(frozen=True, slots=True)
class PluralForms:
    one: str
    other: str

@dataclass(frozen=True, slots=True)
class Message:
    message_id: str
    description: str
    parameters: tuple[Parameter, ...]  # Sorted by ASCII name.
    plural: str | None
    english: TextForms | PluralForms
    german: TextForms | PluralForms

@dataclass(frozen=True, slots=True)
class Catalog:
    messages: tuple[Message, ...]  # Nonempty, sorted by ASCII ID.

@dataclass(frozen=True, slots=True)
class TextArtifact:
    relative_path: str  # One of the four fixed text paths, forward slashes.
    data: bytes

@dataclass(frozen=True, slots=True)
class ArtifactDigest:
    relative_path: str
    byte_count: int
    sha256: str         # Lower-case 64 hexadecimal characters.

@dataclass(frozen=True, slots=True)
class BuildRequest:
    source: str
    text_output: str
    binary_output: str
    lrelease: str

@dataclass(frozen=True, slots=True)
class BuildResult:
    message_count: int
    text: tuple[ArtifactDigest, ...]    # Exactly four, sorted by relative_path.
    binary: tuple[ArtifactDigest, ...]  # Exactly two, sorted by relative_path.
    qt_version: str                    # Exactly "6.12.0", actually checked.

def validate_catalog(source: bytes) -> Catalog: ...
def render_text(source: bytes) -> tuple[TextArtifact, ...]: ...
def generate_catalog(request: BuildRequest) -> BuildResult: ...
def check_catalog(request: BuildRequest) -> BuildResult: ...
def main(argv: Sequence[str] | None = None) -> int: ...
~~~

These dataclasses are DATA, not validation/durability capabilities. In particular
render_text deliberately accepts source bytes and validates them; it does not trust
a caller-constructed Catalog as a certificate. Private render helpers may consume
the freshly validated internal value once. validate/render never read files,
resolve paths, start a tool, mutate an output tree or infer a process locale.
A supplied bytearray/memoryview/nonbytes source is invalid_schema before any work;
bytes input is immutable and parsed strings/artifact bytes remain owned. BuildRequest
contains plain immutable path text; callers may explicitly convert pathlib.Path
before constructing it. There is no retained custom PathLike/fspath callback.

The Catalog shape omits fixed schema_version/license/source_locale/locales because
those are validated v1 constants, not user-selectable return variants. Its exact
named-token grammar, selector-only allowance, two en/de forms and Unicode/integer
meaning are the reviewed logical contract. No formatter, argument substitution,
QML preference object or JavaScript runtime renderer is implemented by these APIs.

## 2. Exact error data and diagnostic bounds

CatalogErrorCode is a str Enum with EXACT values invalid_encoding, invalid_json,
duplicate_key, invalid_schema, duplicate_id, coverage, invalid_template,
parameter_mismatch, invalid_plural, resource_limit, tool_unavailable, tool_version,
tool_failed, generated_mismatch, io_failure. CatalogFailure(Exception) owns:

~~~text
code: CatalogErrorCode
message_id: str | None = None
locale: str | None = None
parameter: str | None = None
stage: str | None = None
tool_exit: int | None = None
tool_reason: str | None = None
~~~

message_id is retained ONLY when valid under the160-byte ID rule; locale is en/de;
parameter is retained ONLY when valid under its32-byte name rule. stage is one of
input, schema, parameters, coverage, template, paths, inventory, version, en, de,
publication, comparison, or None. tool_exit is the actual subprocess return code
when a process exited, including a negative signal code; never a guessed success.
tool_reason is one of exit, timeout, output_limit, missing_output, malformed_version,
version_mismatch, or None. The sanitized exception text is exactly
`Translation catalog failure: <code>`; it contains no source body, path, child
output or traceback. This fixed metadata keeps the serialized error below768 bytes.
Do not turn unknown exceptions/assertion failures into fake catalog validation.

Pure/direct functions raise CatalogFailure for the specified expected failures.
They never print. A malformed public BuildRequest/nonpath field is io_failure,
stage paths, before file/tool work. main maps expected failures to the CLI record
below. Unexpected internal exceptions propagate in direct APIs and produce the
fixed CLI internal_error category/exit70, without a false normal BuildResult.

Validation priority after a valid bytes argument:

1. source byte ceiling4MiB, then strict UTF-8/no BOM; invalid UTF-8/BOM is
   invalid_encoding. No Unicode normalization occurs.
2. bounded JSON syntax/depth and duplicate object keys BEFORE constructing maps
   that could discard a duplicate. Invalid JSON/NaN/Infinity is invalid_json;
   nesting beyond8 is resource_limit. After syntactic decoding, an unpaired
   surrogate in any decoded JSON string is invalid_encoding before schema work.
   Parse JSON number token kinds losslessly:
   only the INTEGER token1 satisfies schema_version. Never rely on host integer-
   digit limits or a floating conversion to accept1.0/true/string1.
3. root shape/fixed constants, nonempty messages array and8192 ceiling; message
   object shapes/ID/description domain and duplicate IDs. Unknown root/message
   fields or invalid IDs/description are invalid_schema; equal duplicates are
   duplicate_id. Enforce declared byte/count ceilings as resource_limit.
4. per-message parameters/name/type/32-count, then plural declaration. Invalid
   declarations are invalid_schema; unknown/noninteger selector is invalid_plural.
5. exact en/de coverage, locale object/form keys and required string presence.
   Missing/extra locale, null locale entry or absent/null/empty required text is
   coverage. A wrong ordinary/plural form-key set is invalid_plural; nonstring
   nonnull text is invalid_schema. Selector presence is not inferred from text.
6. each template's UTF-8 byte ceiling/XML scalar profile, brace grammar and declared
   names. XML-illegal controls such as escaped NUL are invalid_template; malformed brace tokens are invalid_template, while a well-formed undeclared name
   or missing nonselector declared name is parameter_mismatch. Invalid unpaired
   surrogates already have invalid_encoding priority after JSON syntax decoding.

Within each group use ASCII-sorted valid IDs/locales/parameter names. Malformed
JSON/duplicate-key detection reports its bounded category before semantic ordering;
no malformed identifier is echoed just to describe a location. Conflicting errors
inside the SAME indivisible JSON syntax/duplicate-key parse group need not expose
a field/order beyond that category. Source text is never partially accepted.

## 3. Deterministic pure rendering

render_text first performs the COMPLETE validation above and returns exactly:

~~~text
qt/messages_de.ts
qt/messages_en.ts
web/de.json
web/en.json
~~~

The tuple is sorted by these relative paths. Bytes follow the actual corrected
catalog contract's canonical serialization EXACTLY: ONE empty context/name, sorted
ID messages inside it, six/eight-space text/form indentation, all structural tag/
attribute ordering, raw text scalar preservation and one final LF. Browser JSON
has fixed root/message ordering, sorted IDs/parameter names, named templates and
its specified escapes. No timestamp, source pathname, tool fingerprint, host locale,
absolute location or generated business-logic map is added. Reordering valid source
objects/messages produces the same four artifact bytes.

Pure success does NOT establish .qm generation. render_text has no lrelease argument
and must not fake compiled bytes or a qt_version. The generated web/en.json remains
the SAME required native metadata descriptor; no separate manually authored map is
returned. Its strings do not replace actual QM native lookup.

## 4. File/tool operation order and ownership

Both generate_catalog and check_catalog perform this sequence. BuildRequest fields
must be immutable str paths; reject bytes/nonstring/empty/NUL/unpaired-surrogate
paths as io_failure before file/tool work. Path strings are retained as immutable
owned data; no callback/path conversion can change the request during execution.
The host serializes source/output edits for a build/check; this is a local build
tool, not a file service or a concurrent-write capability.

1. Open source READ-ONLY and read at most4MiB+1 bytes; overage is resource_limit,
   open/read/type errors are io_failure. Validate the complete source before any
   Qt tool or publication. Symlinked source may resolve to a real ordinary file;
   the tool never opens it for writing. Record its identity/content snapshot.
2. Canonicalize/validate root disjointness, source exclusion/aliases and the exact
   direct-child owned inventory in the catalog contract. Match names before file
   type; reject recognized symlink/nonregular output entries and symlinked managed
   qt/web directories as io_failure. Extra recognized owned names give
   generated_mismatch for BOTH modes; no deletion/migration. Missing expected
   outputs are allowed for generate. For check, remember missing expected files
   without creating them, complete private staging and report mismatch at step8.
   The lrelease executable must also lie outside both output roots and must not
   alias a publication target; never replace the compiler used by this operation.
   Do not recurse
   into unrelated trees or derive paths from IDs. No output directory/file is
   created merely to validate a failing source, path or tool setup.
3. Produce the four pure text artifacts privately; create isolated temporary stage
   directories and write canonical TS/browser bytes there. Staging is not target
   publication. No project source path is embedded in generated TS.
4. Resolve the explicit lrelease path from the request as a trusted build tool.
   The coordinator/CMake/CI supplies the verified Qt6.12.0 artifact/prefix closure;
   a version string alone is not an archive checksum or provider capability.
   Run `<tool> -version` with no shell. Expected stdout, after stripping ASCII
   whitespace, is exactly `lrelease version 6.12.0`. A different/malformed version
   at exit0 gives tool_version. Failure to start gives tool_unavailable; a started
   nonzero process gives tool_failed with actual exit metadata.
5. Run exact argv `<tool> -fail-on-unfinished <staged en.ts> -qm <staged en.qm>`,
   then corresponding de argv. Never invoke a shell, PATH-selected alias, -idbased,
   -fail-on-invalid, lupdate, network or a provider. Use fresh isolated stage paths.
   Both child processes must exit0 and create their requested REGULAR NONEMPTY
   QM outputs; absent/nonregular/empty output is tool_failed/missing_output. Never
   copy a preexisting QM to simulate success. Actual generated-QM semantics still
   require the independent real Qt Core probe/acceptance below.
6. Recheck source identity/content and publication root/entry safety before target
   work; a changed source/layout is io_failure. Source bytes are never rewritten.
   Construct immutable digest summaries from the ACTUAL staged artifact bytes.
7. generate ONLY: after all prior success, create needed target directories and
   publish the six expected files. Use newly staged per-file temporary files and
   os.replace-style replacement, not in-place truncation of an existing hard link.
   Publish text paths in tuple order above, then binary messages_de.qm/en.qm.
   Unrelated and extra-owned entries are never removed. Source aliases are refused.
   I/O failure can leave a mixture of old/new expected outputs: return io_failure,
   never a complete BuildResult or a claim of a two-directory transaction. A later
   valid generate/check repairs/detects that mixture. All private temporary files
   owned by this invocation are cleaned on normal/error exit; unrelated files are
   untouched. Filesystem crash durability/hostile concurrent actor isolation is not
   supplied by this build utility.
8. check ONLY: do not create/replace/remove target files or directories. Compare
   exact owned inventory and all six expected existing bytes with the newly staged
   bytes. Compare sizes then bounded chunks byte-for-byte; hashes alone are not
   the consistency oracle. Missing/extra/different gives generated_mismatch.
   Return BuildResult only after ALL comparisons pass. No mtime-based shortcut,
   stale cached QM, partial success or text-only success is allowed.

Permission, OS and subprocess failures are expected bounded categories, not ignored
exceptions. Actual source/target contents/inventory are preserved on input/path/
tool failure; a publication I/O failure is separately honest as above. check may
read filesystem metadata/data but never mutates target contents/inventory.

Tool execution has a120-second MONOTONIC deadline PER process and65536 bytes
COMBINED stdout+stderr capture maximum. Drain both pipes without deadlock into
bounded buffers. On deadline/output cap, terminate/kill and reap only this invoked
child, report tool_failed with timeout/output_limit and actual exit when available;
no buffer growth beyond the cap or passing truncated-success result. Child output
is never forwarded into normal CLI stdout/stderr or exposed as translation text.
Only the fixed safe diagnostic metadata is public. The selected actual Qt tool
can have its raw preflight logs recorded separately by the coordinator; this API
does not accept an injected fake successful compiler/reader as production authority.

## 5. CLI records and exit mapping

The three command lines are exactly the catalog contract's validate/generate/check
forms. Flags may be reordered; repeated/unknown flags, missing flag values, unknown
commands or flags inappropriate for validate are CLI usage errors BEFORE reads.
No implicit default source/output/tool location, stdin catalog, environment language
selection, print-body mode or silent overwrite mode is proposed. Sole-argument --help and --version are read-only CLI metadata operations, not
generator execution. They are not combinable with a command or other flags.

With valid command syntax, success emits ONE compact JSON line to stdout and
nothing to stderr. Object key order is status, operation, message_count, qt_version,
text, binary. validate has qt_version null and text/binary empty arrays. generate/
check have qt_version "6.12.0" and digest arrays in tuple order; each digest object's
keys are path, bytes, sha256. status is "ok". No source/output absolute paths or
translation bodies occur. JSON uses compact separators, ASCII escaped metadata and
one LF; no tool chatter appears beside it. Success exit is0.

Expected failures emit NOTHING to stdout and ONE compact JSON line to stderr,
keys status("error"), code, message_id, locale, parameter, stage, tool_exit,
tool_reason. Null optional fields remain null, not omitted. Record length <=768
UTF-8 bytes including LF. main returns these stable exits:

| Category | Exit |
| --- | ---: |
| malformed CLI arguments, code invalid_cli_arguments (CLI-only, no CatalogFailure enum) |64|
| invalid_encoding/json/duplicate_key/schema/id/coverage/template/parameter/plural/resource_limit |65|
| tool_unavailable or tool_version |69|
| tool_failed; unexpected internal exception code internal_error |70|
| io_failure |74|
| generated_mismatch |1|

No traceback/raw child text or private path is a required/public diagnostic. A
normal failure is never exit0. --version prints exactly `translation-catalog 1` plus LF and exits0. --help prints
EXACTLY these four lines with one final LF, and exits0:

~~~text
usage: translation_catalog.py {validate,generate,check} ...
validate --source PATH
generate --source PATH --text-output DIR --binary-output DIR --lrelease PATH
check --source PATH --text-output DIR --binary-output DIR --lrelease PATH
~~~

Initial main remains an immediate marker stub even for these arguments; providing
help/version success is later implementation behavior under the reviewed CLI tests,
never build/discovery's way of probing whether the stub is ready.

## 6. Minimal future stub and valid-red/build boundary

Proposed explicit marker class TranslationCatalogNotImplemented(RuntimeError),
text exactly `Translation catalog generation is intentionally unimplemented; review tests first`.
It is DISTINCT from CatalogFailure. Every callable validate_catalog, render_text,
generate_catalog, check_catalog and main initially raises that marker IMMEDIATELY,
with no argument validation, parsing, file/path access, tool run, output or canned
success. Dataclass/enum definitions and exception construction remain inert.

The guarded future module-script entry catches ONLY this marker and emits to
stderr exactly one compact line:

~~~json
{"status":"unimplemented","marker":"TranslationCatalogNotImplemented"}
~~~

with final LF and exit86. This sentinel exit is not a skip/known failing-test
inversion and is not any production success/error mapping above. Direct Python
calls expose the actual named exception, never an expected validation exception.
Other CLI behavior remains unimplemented until its own reviewed feature tests.
Importing the module is inert; the script guard runs only as __main__.

Build/discovery may py_compile the module, import inert DTO/stub definitions and
list test methods, but must NEVER call these feature functions or run main during
configure/build/discovery. No CMake execute_process(generator), import-time catalog
validation, data-provider invocation, check target or generated-file dependency
may consume the stub before the reviewed first-red gate. A missing executable,
compile/import error, setup failure or skipped case is not a valid intended red.

## 7. Qt and test proof partition / handoff

The coordinator's authorized SCRATCH existing-Qt probe discovered/repaired the
empty-context requirement and proved56 raw lookup checks/two fresh-dir same-tool
comparisons. It is feasibility, not a project test baseline or permission to copy
implementation-generated expectations. Future independently authored literal JSON/
TS/raw-string/count fixtures and tests require actual separate review.

A later minimal Qt Core probe/harness uses the actual selected official prefix,
compiles independent frozen literal TS using real lrelease, LOADS actual en/de QM
and directly queries QTranslator with empty context and declared witnesses. It
requires exact raw percent/braces/CR/Unicode/form/English-base metadata observations.
That old-third-party-only foundation must be GREEN in the same reviewed environment
before the first project stub-red. It must NOT call the new project generator,
source catalog, API stub, Python renderer or named formatter to construct its oracles.
Runtime product preference/formatter behavior is not hidden in that foundation.

Then project feature tests call the reviewed Python functions/CLI and compare ALL
four text bytes against independent complete goldens, actual produced QM raw forms
against independent literals, exact CLI failures/ownership/source preservation and
two fresh-directory binary reproducibility. Real QM/raw Qt validation and browser
JSON scalar/metadata parity are mandatory; TS/JSON-only success cannot finish
LOCALE-001. Expected values never come from render_text/generate_catalog under test.
CI must discover the complete reviewed suite and keep intended stub-red separate
until implementation passes, with no WILL_FAIL/skip/continue-on-error masking.

This API contains no native/browser named formatter, detected/saved language change,
Qt/QML live retranslation signal, theme, independent locale preference, German
layout or package/platform implementation. LOCALE-002/003/004/090 and all other
release requirements remain. Next stages are actual API document review, minimal
stub/harness review, independent test authorship/review and own selected foundation
plus FIRST intended marker red. Production LOCALE-001 is not released by this text.
