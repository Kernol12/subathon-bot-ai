# Schema tool public API and process admission — document proposal v1

SCHEMATEST-001, author base0dfa0de931a42523169418f8792126cbd435a82e.
Refines the accepted [logical tool contract](schema-validation-tool-v1.md), blob
`abb426a91173726a703cba70d6a68225de483bdd`, WITHOUT replacing its requirements,
SV01–20 or later stage gates. Authority: specification v0.45 §§17/18,
API-01/02/03 and TEST-04. This is a DOCUMENT, no actual module/interface/stub,
worker, parser, schema, fixture/test, dependency or workflow implementation.

This first concrete stage freezes public calls, package representation, process
ownership and admission outcomes. The SECOND context-adapter contract must still
freeze the exact OpenAPI role table, schema-context/reference/evaluation graph,
complete example inventory and ordered native calls after additional reviewed SDK
facts. This document does not approve an unspecified walker or standards engine.
No stub/test baseline or production behavior is released until BOTH concrete
stages and their applicable reviews/foundations pass.

## 1. Exact module, enum and two raw calls

Future file `tools/schema_validate.py`, importable without candidate dependencies,
package-file reads, dependency scans, environment edits, process creation or
feature invocation.
Imports/constants/enum/exception definitions use only the Python>=3.10 standard
library. The public declarations are exactly:

```python
class SchemaValidationOutcome(str, Enum):
    ok = "ok"
    usage_error = "usage_error"
    invalid_path = "invalid_path"
    io_unavailable = "io_unavailable"
    environment_unavailable = "environment_unavailable"
    resource_limit = "resource_limit"
    invalid_json = "invalid_json"
    invalid_manifest = "invalid_manifest"
    unsupported_dialect = "unsupported_dialect"
    unsupported_format = "unsupported_format"
    dependency_unavailable = "dependency_unavailable"
    invalid_schema = "invalid_schema"
    unresolved_reference = "unresolved_reference"
    reference_cycle = "reference_cycle"
    invalid_inventory = "invalid_inventory"
    unsupported_feature = "unsupported_feature"
    invalid_instance = "invalid_instance"
    validation_unavailable = "validation_unavailable"

def validate_package(manifest_path: str) -> SchemaValidationOutcome: ...
def main(argv: list[str] | tuple[str, ...] | None = None) -> int: ...
```

No additional success factory pretending to validate a package, mutable result,
injectable successful validator/worker,
permission certificate, caller-provided graph or optional limit/backend arguments.
Outcome is only an observation of this invocation's owned input snapshot. It is
not a certificate for later modified files, a supported endpoint or release proof.
The enum/string values have immutable ordinary Python lifetime; no input borrow,
parsed tree, worker handle, exception object or retained file leaks through return.

validate_package accepts ONLY exact built-in str, not a subclass/path-like object;
reject other types as invalid_path without repr/conversion/callback. It never
prints, logs or mutates input. The caller owns the immutable original path. After
type/size validation, retain only its bounded copy for the synchronous operation.
Normal controlled failures return the exact enum; no third-party exception text
is exposed. A process killed externally cannot produce a fabricated result.

main with None reads the current sys.argv range after the executable once; do not
copy an unbounded argv before checking its length. Explicit argv must be exact
list/tuple of exact str; no arbitrary iterable/conversion callback. Supported
forms are ONLY `["--help"]` and `["--manifest", PATH]`. Every other shape, empty
argument list, duplicate/unknown option, `--manifest=...`, extra argument or wrong
element/container type returns64 with usage_error. After --manifest the second
string is literal path data, not another option. No URL/stdin/glob expansion.

Implemented --help prints EXACT ASCII
`Usage: schema_validate.py --manifest PATH\n`, returns0 and performs no package,
dependency or worker operation. Otherwise main calls validate_package ONCE and
emits the fixed line/status below. Direct script execution uses main and its
returned integer; importing the module never calls main.

## 2. Closed output and lifetime protocol

For each normal non-help CLI result stdout is EXACT compact ASCII
`{"code":"<enum value>"}\n`, one line, no stderr. No additional keys/whitespace,
counts, paths, pointer, URI, invalid value, input-derived label, exception repr or
traceback. The longest line is35 bytes; the logical256-byte ceiling still applies
to transport/protocol admission and does not authorize extra data.

| Outcomes | CLI exit |
| --- | ---: |
| ok | 0 |
| usage_error | 64 |
| invalid_path, invalid_json, invalid_manifest, unsupported_dialect, unsupported_format, invalid_schema, unresolved_reference, reference_cycle, invalid_inventory, unsupported_feature, invalid_instance | 2 |
| io_unavailable, environment_unavailable, dependency_unavailable, validation_unavailable | 3 |
| resource_limit | 4 |

The parent owns at most one worker, bounded argv/path/control bytes and256+1
diagnostic bytes. It retains no raw package/parsed tree/standards-library error.
The worker owns package handles, snapshots, parse/context graphs and candidate
imports; none outlive its exit. Each invocation uses a FRESH worker, so library
caches cannot retain package data across calls. Module-level immutable dependency
metadata is not a successful prior-validation cache.

## 3. Exact manifest representation and limits

The logical package profile remains `subathon.schema-package.v1`.
Manifest object has EXACT required profile/documents/examples. No optional or
extension members, null substitutions, negative-fixture flags or defaults.
Decoded strings are Unicode scalar strings; duplicate object keys after escape
decoding are invalid_json. Field schema:

| Location | Required value |
| --- | --- |
| profile | exact string `subathon.schema-package.v1` |
| documents | ordered list,1..32 entries; at most one openapi31 |
| documents entry | closed object `{path: RelativePath, kind: "openapi31" or "jsonschema202012"}` |
| examples | ordered list,0..256 bindings |
| binding | closed object `{path: ExampleSource, schema_document: RelativePath, schema_pointer: Pointer}` |
| ExampleSource | RelativePath string OR closed object `{document: RelativePath, pointer: Pointer}` |
| schema_document / inline document | exact name of a declared document |
| Pointer | empty root string OR RFC6901 slash pointer, at most4096UTF8 bytes |

RelativePath is1..256ASCII bytes: slash-separated nonempty segments using
`[A-Za-z0-9_.-]+`, no segment . or .., leading slash, backslash, percent, colon or
NUL. Manifest path argument is1..4096UTF8 bytes, no NUL/lone surrogate/URI scheme;
absolute or cwd-relative native paths may select the package root. The path is not
shell-expanded or converted from a URL. Package paths are always relative to the
ACTUALLY opened manifest parent, never cwd after root selection.

Pointer validates ~0/~1 escapes and Unicode without normalization. A literal
percent sequence is pointer data, not a filesystem escape. When converting a raw
pointer to a URI-fragment lookup the concrete adapter must encode literal percent
bytes correctly; native Resource.pointer performs URI decoding. This document
does not certify that future adapter; context-stage SDK cases must establish its
exact escaped-pointer/scope behavior. Targets must be actual schema/value contexts,
not arbitrary mappings that happen to resemble them.

Distinct document paths cannot repeat or equal the manifest file's actual relative
identity. External example paths cannot alias a declared document/manifest file;
use the explicit inline-source form for a document value. Several bindings may
share one external example file/inline value with DIFFERENT schema locations.
Reject duplicate EXACT decoded tuples (source form/document/path/pointer,
schema_document, schema_pointer) as invalid_manifest. Do not Unicode-normalize or
URI-decode these tuples. Later resolved-context aliases/wrong associated schemas
are invalid_inventory under the separately reviewed context stage. One external
file is read once even if several bindings use it.

Illustrative COMPLETE inline binding (the associated contexts still need validation):

```json
{"path":{"document":"api.json","pointer":"/paths/~1timer/get/responses/200/content/application~1json/example"},"schema_document":"api.json","schema_pointer":"/components/schemas/Timer"}
```

All logical ceilings remain EXACT: manifest65536bytes; unique files289 including
manifest; each document/example1048576bytes; total raw files8388608bytes; depth64
(root container depth1);65536complete JSON values PER FILE; quoted token65536raw
bytes; numeric512bytes/256mantissa digits/absolute explicit exponent4096 and exact
numerator/denominator16384bits; resources256/context locations4096/ref edges2048;
one URI/pointer key4096UTF8bytes/total materialized graph-key bytes1048576;256
bindings;256diagnostic bytes; one worker/10seconds/536870912address-space bytes.
Limits are trusted fixed profile data, not manifest/argv/env controlled.

Raw quoted-token bytes include quotes and escapes. JSON value count includes
containers and every array element/object MEMBER VALUE, not member-name strings
as extra values; those names remain string tokens under the token ceiling. Total
raw bytes count distinct file payloads once, including manifest. Context/resource/
edge/graph-key accounting remains the accepted logical requirement and is frozen
in the second concrete stage; smaller resource counts cannot substitute for the
distinct-context or total-key budget. Exact limits refuse complete inputs rather
than accept fitting prefixes or truncate inventory.

## 4. Worker/control admission interface

The future private worker is `tools/schema_validation/worker.py`; it is not another
public package validator or alternate success path. Parent creates it with an
absolute trusted interpreter/script argv, shell=False, isolated startup and no
package-selected executable/environment. Its bounded control inputs are only the
admitted manifest path, trusted selected SDK location and two private pipe
descriptors: activation and outcome. These descriptors identify owned output
channels, not caller-supplied validation results or additional package inputs. No caller graphs/proofs/validation result are serialized in.
Neither parent nor worker imports the project source as a success fixture.
Each trusted interpreter/script/SDK-directory path is absolute,1..4096UTF8 bytes,
without NUL or lone surrogate; invalid/unrepresentable trusted configuration is
environment_unavailable. These limits do not enlarge the manifest-path ceiling.

Use isolated/no-site startup (`-I -S`) so worker setup does not execute .pth/site
imports before admission. The fixed standard-library worker prologue establishes
and verifies the536870912 soft/hard address-space ceiling before ANY package read
or candidate import. Before startup, the parent obtains and validates the absolute
SDK-directory/interpreter binding from the reviewed trusted installation descriptor,
including the selected interpreter's actual version/platform/ABI. Neither package
data nor an environment variable can supply that binding. Pre3.14 `-S` startup
does NOT provide reliable venv SDK roots through sys.prefix/sys.exec_prefix:
[Python documents the3.14 change](https://docs.python.org/3.14/library/sys_path_init.html).
Do not derive the SDK directory from those prefixes. After the resource prologue
and verification against the actual running interpreter's ABI, append ONLY that
validated hash-admitted directory directly to sys.path. Do not call site.main,
site.addsitedir or process any .pth file. Exact trusted-descriptor discovery and
minimum/selected-host `-I -S` activation require reviewed concrete harness evidence;
this document does not assert an implemented descriptor or successful activation.
If these controls cannot be established, return environment_unavailable; no
uncontrolled fallback. This is Linux developer-tool admission, not Windows/Linux
product support certification or a new runtime dependency.

Parent retains the10-second monotonic deadline from spawn through process exit,
including startup/imports/IO. A separate private activation pipe carries EXACT
one ASCII byte `A` only after limits are active; no package/source text. Missing,
extra or malformed activation makes any apparent ok result validation_unavailable.
Close unrelated descriptors and unused pipe ends. Worker has no inherited stdin;
candidate-library stdout/stderr are suppressed before import. Only the dedicated
outcome pipe may publish a constant result. No input is sent through stdout/stderr.

Read at most257 outcome bytes; do not use unbounded capture_output/communicate or
read-all of arbitrary stderr. Parent admits EXACT one listed outcome line plus EOF
and matching child exit status. usage_error/help are never valid worker responses.
Wrong/extra/truncated/unknown output, mismatched status or unexpected child death
is validation_unavailable, never ok. A parent-enforced deadline/known memory refusal
is resource_limit; a signal alone does not prove memory exhaustion or confirmed
domain failure. Kill and reap on timeout/protocol excess; release owned descriptors
on every path. No surviving worker or cache is permitted after synchronous return.
Cancellation while Python can unwind follows the same cleanup and returns no
fabricated successful observation.

Dependency imports use the admitted exact20-package environment and explicit3.1/
Python-jsonschema backend. Parent/worker may not inherit arbitrary backend, proxy,
PYTHONPATH/site customization or project package hooks from the user environment.
The precise native SDK/context calls and complete offline policy remain the SECOND
contract's gate. In particular handlers={} is not global no-network proof; unknown
SchemaRetriever schemes still fall back. Exact hash-admitted bundled dependency
metaschema reads are distinct from package-controlled file/URI resolution. The
existing seven SDK methods do not prove this proposed worker/API implementation.

## 5. Public first-failure order

This orders observable outcomes, not library exception text. Resource/process
failures can interrupt any later phase as specified by the logical contract.

1. main argv shape/type, then help or path dispatch. Invalid argv→usage_error;
   no worker/candidate/package operation. validate_package type→invalid_path;
   path encoding/length/NUL/URI shape→invalid_path. No repr/conversion callback.
2. Establish controlled worker and secure root/manifest opening. Missing required
   process/file facilities→environment_unavailable; invalid path/symlink/nonregular
   selection→invalid_path; missing/inaccessible file or unclassified OS inability
   →io_unavailable. Do not recategorize OS failure as malformed JSON.
3. Manifest byte ceiling, UTF8/BOM, lexical-resource preflight, strict complete JSON,
   then closed shape/member/type/count checks in profile→documents→examples order.
   Byte/resource excess→resource_limit; bad encoding/BOM/JSON/duplicate decoded keys
   →invalid_json; unknown profile/shape/duplicate declarations/bindings→invalid_manifest;
   invalid declared paths→invalid_path. All declaration counts and checked aggregate
   file-byte projections precede file payload reads.
4. Visit declared documents then each external example's first manifest occurrence.
   For each: secure actual handle/type and size/read checks, UTF8/BOM→resource scan
   →strict JSON→bounded exact numeric construction. Invalid JSON→invalid_json,
   resource excess→resource_limit; file errors retain step2 distinctions. Reject
   changed-read evidence as io_unavailable; no whole-filesystem snapshot claim.
5. Complete dialect then format admission, in document/pointer order:
   unsupported_dialect before unsupported_format for the same context. Other format
   data outside Schema Object context is not guessed to be a schema keyword.
   Actual context locations remain gated by the second contract, not a key-name scan.
6. Exact dependency/backend availability→dependency_unavailable. Candidate imports
   never occur before worker admission or package preflight. No guessed version,
   optional backend or successful skip can replace the selected environment.
7. Logical phase5 CONTEXT PIPELINE remains a required separate concrete contract:
   unsupported features→metaschema/structural checks→complete typed reference
   closure→same-instance cycle analysis→complete inventory equality→native semantic
   checks. No public ok/stub/test-baseline completion is authorized without that
   reviewed pipeline and its actual source-supported SDK facts.
8. Evaluate EVERY manifest-bound instance in binding order; any invalid instance
   →invalid_instance. Only full completion of every required stage returns ok.

Within each completed phase use manifest document/binding order, then unsigned
UTF8 JSON-pointer order; do not select arbitrary library traceback ordering as the
oracle. Deadline/known resource failures→resource_limit; unexpected worker/protocol/
internal inability→validation_unavailable. Source-backed context limitations must
surface through the reviewed closed refusal/gate, never remove example/ref checks.

## 6. Future inert stubs and baseline gate

Both future raw feature bodies have EXACT first statement:

```python
raise SchemaValidationNotImplemented()
```

SchemaValidationNotImplemented directly derives from RuntimeError, zero-argument
constructor sets EXACT text
`Schema package validation is intentionally unimplemented; review tests first`.
It is distinct from the outcome enum and is never mapped into an ordinary outcome
by a minimal stub. Neither body reads argv/options, validates, prints help, opens a
file, starts a worker or initializes/imports a candidate BEFORE that statement.
No default decorator/argument/factory performs feature behavior. Raw invalid input
and --help therefore also reach the genuine missing-feature marker at that stage.
The ordinary production protocol is intentionally unavailable until implementation.

Inert enum/exception/declarations are allowed only AFTER both actual concrete
contracts/dependency/SDK requirements have separate review. Root then owns minimal
source/build/CI harness, independent complete fixtures/tests and actual test review;
green selected foundations precede the FIRST intended missing-wrapper red. A child
crash/dependency failure/unknown network resolution is not that red. Production
SCHEMATOOL001 still waits for the integrated complete reviewed/red baseline.

This document supplies no caller-facing parser, source, worker, fixture, API010
implementation or broader schema support. Every original logical requirement,
CONTRACT013 validation gate and full runtime/platform/provider/release requirement
remains mandatory. The pending context contract is an explicit remaining stage,
not a claimed compatible backend or permanently narrowed product scope.
