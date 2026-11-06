# Wrapper acceptance source authoring — SCHEMATEST-001

Author `/root/session_supplier_author`, 6 November2026. **AUTHORED, NOT REVIEWED
OR EXECUTED.** New isolated branch `task/SCHEMATEST-001-wrapper-tests-author`,
worktree `/tmp/subathon-schematest001-wrapper-tests-author`, from actual root
`2af1b8945e081ca83e0f3a03c5ea8ee154149538`. Only this note and
[wrapper_acceptance.py](../../tests/schema_validation/wrapper_acceptance.py) change.
No production, shared state/HTML/CI, dependency, install or GitHub write.

Authority read: root/tests AGENTS, architecture/workflow/test strategy and paired
[SCHEMATOOL-001](../tasks/SCHEMATOOL-001.md), specificationv0.45 §§17/18 API01/02/03
TEST04; logical contract blob `abb426a91173726a703cba70d6a68225de483bdd`,
API691 and clarified API blob `a526dbc1114de7f79268dd37c51b97988e627c80`,
final context blob `32ec87a25dca2d39227a7fe994df3a2d908a26e6`. The private
process document at actual e6fe6bd/blob `7f30d99d2977a38105ce8618f827da804565c8f1`
was separately APPROVED DOCUMENT ONLY by different reviewer C at3a2003a BEFORE
fault-helper authoring. Public signatures/outcomes/limits remain unchanged.

## Actual layout and oracle inventory

One stdlib-only module, one ordinary `WrapperAcceptance(unittest.TestCase)` class,
**60 named test methods**, no decorators on tests, custom constructors, load_tests,
SDK imports, successful validating callbacks or dynamically generated registrations.
Module import defines helpers/classes/constants only; it creates no fixture, reads no
candidate, imports no target and starts no process/thread. setUpClass explicitly loads
the trusted local target file; script-directory/PYTHONPATH discovery is unnecessary
under root's isolated `-I -B` runner. Temporary packages are constructed within cases.
The local changed_directory helper supports Python3.10 (no contextlib.chdir).

There are **300 authored direct public-call oracles** when all preceding assertions
complete, plus ONE extra real-script parity invocation AFTER its direct main call.
This is a source-level manual count, not an execution/discovery receipt, and not300
independent unittest registrations. Many methods contain literal subTest tables or
several sequential observations. An immediate raw marker terminates that observation;
later sequential assertions in the same method are not claimed reached in red.
Each of60 registered methods first reaches a raw public validate_package/main call,
including malformed input/help/fault cases. The helpers do not catch the missing-
feature marker or map it into success. No expectedFailure, skips, inversion or
successful worker stand-in exists. Root owns separate inert import/enum/static checks.

| Actual named methods (prefix test_ omitted) | Contract/oracle purpose |
| --- | --- |
| minimal_draft_package_and_openapi_patches | SV01, explicit3.1.0/.1/.2, real positive package |
| api_path_types_never_convert; api_path_encoding_and_shape | API§1 exact builtin type, encoding/size/NUL/URI; poison callbacks |
| main_closed_argv_never_converts; help_static_without_worker_or_package_io; main_tuple_none_and_option_looking_path | API§§1/2 exact argv/help, no conversion, literal option-looking path |
| main_closed_codes_and_real_script_redaction | actual invalid_instance line/exit/no stderr and direct-before-script parity |
| fresh_calls_do_not_cache_prior_success; package_root_is_manifest_parent_not_cwd | fresh snapshot/root ownership |
| missing_manifest_and_declared_member; symlink_manifest_parent_and_members; nonregular_files_are_refused_without_blocking | SV18, real missing/symlink/directory/FIFO, no parent OS-error fake |
| manifest_closed_shape_and_types; manifest_path_grammar_and_exact_256_boundary; manifest_pointer_and_inline_source_shape | API§3 closed JSON representation, exact path grammar/length, pointer escapes |
| duplicate_declarations_bindings_and_file_aliases | duplicate tuples and actual hardlink identity |
| document_and_binding_count_limits_precede_missing_reads; all_289_unique_files_are_admitted | 32/33 documents,256/257 bindings,289 actual distinct files, count-before-missing-read priority |
| manifest_and_individual_file_byte_boundaries; total_raw_file_byte_boundary_includes_manifest | SV19 exact65536/1MiB/8MiB and+1; whitespace pads VALID JSON, distinct bytes include manifest |
| strict_json_rejects_encoding_duplicates_and_grammar | SV13, BOM/UTF8/surrogates/escaped duplicate key/NaN/comments/trailing/number grammar |
| json_depth_values_and_raw_string_boundaries | depth64/65;65536/65537 values; names are tokens not extra values; raw/escaped65536/65537-byte strings |
| lexical_numeric_bounds_before_exact_construction | SV19 mantissa256/257, exponent±4096/4097, lexeme512/513 isolated with leading exponent zeroes |
| first_failure_json_preflight_and_complete_file_order | BOM before scan, scan before grammar, ALL payload JSON before format/shape |
| exact_numeric_integer_boolean_and_keyword_semantics; exact_fraction_multipleof_and_neighbor | SV02–06, normalized1.0/1e0, bool distinct, enum/uniqueItems/negative zero/schema-count1.0, exact40-place neighbor |
| dialect_admission_is_explicit_and_precedes_format; schema_format_only_not_literal_or_metadata | SV17, explicit3.1 not3.0/3.2, closed dialect/vocabulary/format, literal format and bearerFormat |
| native_invalid_schema_in_unused_children_and_ref_siblings | SV14, all raw children and siblings before closure, wrong child types retained |
| literal_data_ref_id_format_and_reserved_property_names; profile_features_are_context_specific | SV07/09, atomic default/const/enum, reserved property names; actual dynamic/discriminator/structural profile gates |
| complete_unused_reference_closure_and_wrong_target_role; duplicate_resource_ids_anchors_and_outside_ids | SV08/10, every unused ref, data/response/schema type distinction, unique scoped identities, no outside IDs |
| manifest_pointer_escaping_percent_hash_and_unicode; uri_reference_fragments_are_not_reencoded_as_raw_pointers; unicode_pointer_identity_is_not_normalized | independent literal~0/~1/%25/hash/UTF8 lookup strings and distinct composed/decomposed names |
| cross_document_scope_nested_ids_anchors_and_ref_siblings; oas_schema_scope_with_and_without_root_id | native whole-fragment scope, relative nested IDs/anchors, cross document, raw $ref siblings, no-id reset |
| same_instance_cycles_even_unused_or_conditional; productive_recursive_schema_and_inventory_only_edges | SV11/12, same-instance and OAS alias cycles; productive finite instances; $defs/contentSchema not application edges |
| native_applicators_booleans_unevaluated_and_property_names | native bool/array/unevaluated/property-name facet behavior |
| oas_default_literal_ref_preserved_and_raw_sibling_checked | unchanged DATA $ref positive/negative default, unused sibling default, standalone annotation distinction |
| schema_example_inventory_missing_then_invalid_instance; inventory_null_example_and_no_associated_schema | SV15/16, complete examples not just first; null present, response invalid instance, missing schema refusal |
| reference_example_physical_identity_complete_forced_schema_pairs; free_component_example_requires_explicit_actual_schema_binding; extra_misdirected_and_resolved_alias_inventory | physical identities, all forced associations, no arbitrary extra, free example, alias duplicates |
| external_example_shared_across_actual_schemas; parameter_header_encoding_and_schema_annotation_inventory; standalone_inline_examples_are_complete_literal_data | external reuse, Parameter/Header/Encoding, raw inline annotation data and complete association |
| oas_closed_roles_include_unused_components_trace_and_content_forms | role table including PathItem/TRACE/ref containers/content forms/OAuth metadata, unused structural ref closure |
| all_documents_finish_dialect_admission_before_native_checks; context_phase_priority_across_independent_defects | complete ordered phases: dialect, supported profile, structure, closure, cycle, inventory, semantics, instances |
| required_inventory_growth_refuses_instead_of_truncating; context_resource_and_reference_edge_boundaries | complete inventory257; context4096/4097, resources256/257, refs2048/2049 separately |
| graph_keys_include_encoded_controls_and_complete_owned_spellings | exact4096/4097 encoded URI controls and larger expansion despite fitting raw pointer; aggregate retained key lower bound>1MiB |
| worker_activation_missing_extra_and_malformed_refuse_forged_ok; worker_outcome_bounded_exact_closed_protocol; worker_status_mismatch_and_unclassified_signal; worker_startup_deadline_kills_and_reaps | private process v1 actual children, malformed protocol refusal, bounded257 read, unexpected death, deterministic complete deadline, real parent reap |

The exact60 names are statically enumerable from the actual class source; no name
or expected result comes from the implementation. Fixtures use JSON encoding only
for constructing ordinary valid synthetic files; invalid lexical fixtures/numeric
lexemes/protocol lines and expected outcomes are literal and independent. Encoded
Unicode pointers are explicitly spelled, never computed by a future role walker.
A poison object's repr/conversion/iteration raises if touched; subTest labels use
fixed integers so the test harness itself does not convert that supplied object.

## Primitive fault harness and source limits

The reviewed private plan is enforced at the intercepted normal Popen call:
absolute interpreter/worker/SDK paths, -I -S, exact ordered options, canonical distinct
descriptors>=3, exactly two pass_fds, DEVNULLs/shell=False/close_fds/no preexec/text.
Only then launch a REAL fixed stdlib-only child, retain real Popen/PID, and write
malformed bytes to the actual parent pipes. No child can establish domain validity.
Forged ok appears ONLY in explicitly malformed cases expecting refusal. Valid
package/scope/schema/instance cases use the unpatched real parent/worker/library path.

The deadline child acknowledges a separate test readiness pipe then blocks. The
Popen interposer uses kernel select with3-second readiness bound; the parent's
module time.monotonic observes start then start+11 only after readiness. A separate
5-second real threading.Timer kills the child without waiting/reaping; if it fires,
the case FAILS. It cannot provide the expected resource_limit. Save/retain the real
clock before patching. After public return, waitpid(WNOHANG) must raise
ChildProcessError BEFORE any poll/wait/cleanup; only afterward finally may kill/wait
or release synchronization descriptors. No real memory stress, sleeps or network.

This authored source still needs independent audit of every timer/descriptor/error
path and actual API behavior. Deadline testing proves that fixed parent accounting
case, not all scheduler behavior. Parent spawn interception does not prove worker
resource prologue, no-site/trusted ABI admission, secure-read races or real no-fetch
semantics. No fixture is labeled live/provider/platform verification.

## Honest limits and remaining additive admission work

The v1 16384-bit rational boundary cannot be independently crossed by an otherwise
admitted number. A256-digit mantissa times at most10^4096 is<10^4352<2^16384;
negative exponent/fractional-place denominators have the same conservative bound.
Tests independently isolate the reachable mantissa/exponent/lexeme limits instead
of fabricating a16385-bit instance. A290th distinct package file likewise requires
more than32 documents or256 external sources first; the actual289-file positive
and each separate count+1 refusal cover reachable boundaries.

Graph-key bytes count every actually retained spelling/copy, so exact aggregate
1MiB equality is representation-sensitive and cannot be asserted by guessing a
future implementation's ownership layout. The aggregate negative fixture has250
physical schema-pointer strings and250 distinct absolute resource URI strings,
each at least3400bytes, across two<1MiB documents: those mandatory spellings alone
exceed1MiB; resources252/contexts252 still fit and every individual key<4096. This
is an independently sufficient lower bound, not proof of one exact allocator peak.
Exact reservation/copy accounting, transient bounded reads and one-snapshot
ownership remain implementation/source review gates. The other key fixture has a
2007-byte raw pointer but a>6000-byte required encoded control URI, isolating the
expanded-key ceiling. No raw-pointer prefix is treated as complete validation.

Observable environment_unavailable/dependency_unavailable/trusted SDK descriptor
and ABI failures, fresh-worker child read-race/changed-read admission and exact
RLIMIT/no-site activation need additional deterministic real-OS/private admission
fixtures or reviewed integration/source evidence. The current contract has no exact
trusted-descriptor representation/discovery seam; this source does not invent one
or label parent mocks as child proof. Root assigned different C to assess the
smallest exact additive test contract. **This first candidate is NOT a complete
reviewed baseline and does NOT release production.** Do not silently omit SV20 or
claim a selected-dependency version failure was tested by forged outcome bytes.

Also retain source/integration audit for ZERO network across every SDK fallback,
openat/no-follow descriptor ownership and serialized owner snapshot constraints.
No synthetic .invalid URI causes a real server/listener/live check by these tests.
The library foundation's tripwires and later implementation audit are distinct
from these public expected outcomes; no absence of network is inferred from a
refusal code alone.

## Actual verification and next gates

Host Python3.14.9. Commands actually executed:

- stdlib `ast.parse(source, feature_version=(3,10))` and `compile(...,'exec')`
  WITHOUT exec/import: PASS,60 test definitions,949 lines before SPDX addition.
- Independent stdlib Fraction literals:0.3/0.1=3;40-place neighbor modulo0.1=1/10^40;
  exact512/513 lexeme and65536 escaped-token lengths, conservative bit bound and
  aggregate key lower bound: PASS. These commands do NOT execute fixture helpers.
- `python3 tools/check_project.py`: PASS426 tasks/122 decisions/links/presets/HTML.
- `python3 tools/progress.py render --check`: PASS, current; no shared render edited.
- `git diff --check`: PASS for tracked diff (new paths also checked when staged).

No test/candidate/SDK import, unittest discovery, test/feature body, process probe,
subprocess child, installation or intended red was executed. Different actual
source/oracle review (including additive admission families), root's inert harness
review, selected green foundations and FIRST genuine missing-wrapper marker run
remain mandatory. Root owns exact task/CI/publication status and later PR gates.


## Actual pre-execution source-review corrections

First committed candidate73f33c7/source7fa72d4 had60 names/272 authored calls.
Different C's actual audit found one malformed positive fixture: multipart
encoding.part lacked the required media-schema property. Independently checked
[OAS3.1.1 §4.8.14.1](https://spec.openapis.org/oas/v3.1.1.html#media-type-object)
through the primary web tool: an encoding key must be a schema property. Add
properties.part with type string, preserving the intended positive Header inventory
case and its later missing-binding refusal. No expected outcome was weakened.

Different A found two fault cases masked by an independent exit mismatch. The
extra-key invalid_json line now exits2 and forbidden worker usage_error exits64;
only malformed-line/worker-code admission can justify refusal there. The separate
status-mismatch case remains unchanged. Both reviewers/self-audit identified help's
bool/int ambiguity and exceptional readiness descriptor ownership: require exact
int0, add test-only builtins.open/io.open/os.open read tripwires, and retain ALL
four test synchronization descriptors in one outer ExitStack from creation through
spawn/readiness/public return. Idempotent close_owned removes closed descriptors
before early release so later cleanup cannot close a reused descriptor. No cleanup
or poll/wait may reap a child before the actual parent-reap assertion.

C's requested focused additions retain the same60 method registrations: with AND
without OAS Schema root$id, raw1 must violate the sibling minimum2; a recursive
propertyNames->$ref-root cycle is productive through the finite name-string facet;
missing anchor is unresolved_reference. Literal control prefix
https://schemas.subathon.invalid/package/s.json#/%24defs/ is57ASCII bytes. Names
of4039/4040 ASCII a characters yield exactly4096/4097 encoded controls while raw
pointers are4046/4047 bytes and fit. Expect ok/resource_limit independently of
implementation calculations. A's requested actual main observation on the65537-byte
manifest verifies resource_limit exit4 and its closed stdout; no fake result.

These add SEVEN raw observations (six context/reference/boundary and one main),
bringing the authored source count to279. Python3.10-feature AST parse, compile
WITHOUT exec/import and independent literal URI/pointer lengths PASS. No test or
candidate was imported/executed before or after these corrections. Actual revised
source still requires both different reviewers' exact-candidate verdicts; additive
admission families remain pending as described above.


Further pre-execution different review of actual328e3b0/source609f9e8 kept all prior
oracles. C requested closure coverage through every CLOSED Schema child location:
retain the existing $defs unused reference, add the other19 literal map/array/single
keyword locations, all expecting unresolved_reference even when instance1 would
never evaluate that child. This fixture table constructs inputs only; it implements
no role scanner, resolver or assertion checker. Add separate Operation.callbacks
and Response.links profile-refusal locations; webhooks alone could not establish
those roles. The same60 registrations now contain300 authored raw-call observations.

A's re-review found timer.start may raise before a timer is joinable. Register a
timer for later cancel/join ONLY after successful start; nested finally protects
all descriptor closing and real child kill/wait even if timer cleanup raises.
Child kill's own finally attempts wait. Neither cleanup path manufactures any
accepted result or parent-reap proof. Static3.10 AST/compile-without-exec PASS on
this1004-line source; no test/candidate import, native child or feature run occurred.
These findings are actual source/oracle review corrections BEFORE red, not weakened
assertions, observed runtime failures or production workaround. Admission-family
completeness still awaits root's separately reviewed installation contract.

## Released additive admission source — current63-case candidate

NEW isolated author branch `task/SCHEMATEST-001-wrapper-admission-author`, worktree
`/tmp/subathon-schematest001-wrapper-admission-author`, from actual root
`fab7d825c02d4d2085fe820d9a61acc8e87db373`. The earlier author branches/reviews
remain intact. Actual installation document0bcdc4a/blob
`86d6d482e767af3d57d4da58d7fd644a9383c34e` and admitted manifest metadata blob
`6f4ffec82e73274bebeac2ea1aa0ad31e28f4539` received DIFFERENT C document/data
approval52229d7 BEFORE this additive authoring. Read the complete actual document
and review; no prospective document gate is assumed. Source remains unexecuted.

Current actual inventory is **63 named tests**, **315 authored direct public-call
observations**, and **3 additional real public-script observations** after direct
calls. The original60-case/300-call evidence above remains historical source
provenance, not an overwritten approval of new behavior. Added registrations:

- test_parent_pipe_enosys_refuses_without_worker_or_logs
- test_sysconfig_environment_presence_including_empty_refuses_unchanged
- test_real_sdk_clone_control_then_missing_distribution_is_dependency_unavailable

The new script_result helper invokes the REAL absolute public script with the
REAL requested interpreter, isolated startup, no shell/inherited input and a fixed
minimal PATH/locale environment. Separate temporary stdout/stderr files are read
at most257bytes after bounded wait/cleanup; fixed expected line/exit and no stderr
are asserted. This is no injectable successful worker or validating callback.

The clone test's FIRST operation under test is actual raw validate_package on the
literal valid package. Only after that completes may it import stdlib shutil/
sysconfig/venv, discover the ACTUAL single admitted SDK root, create a NEW disposable
with_pip=False/no-system-site venv and copy site-packages. It does not relocate a
whole venv or run pip/download/install. The untouched copied SDK must first produce
REAL CLI ok/exit0. Remove exact openapi_spec_validator code AND the pinned
openapi_spec_validator-0.9.0.dist-info directory; the same real script then must
produce dependency_unavailable/exit3/no stderr. Missing metadata cannot be relabeled
as early environment_unavailable. No clone/helper/child has actually run yet, and
this authored recipe does not establish that runtime result in advance.

The parent-only ENOSYS test patches the exact approved module os.pipe seam, verifies
it was reached, and forbids ANY Popen call. It requires environment_unavailable and
no public logs. No pipe2/prebound fallback is admitted by the private document.
This proves only the injected parent refusal when later executed; it never claims
that actual host pipes, child RLIMIT, ABI or secure-open support are unavailable.

For EACH of the three exact sysconfig override keys, both empty and nonempty values
require environment_unavailable/no worker/no public logs; the complete caller
environment must remain equal before/after. Each subtest isolates one key and the
outer patch restores test-owned state even when the raw missing-feature marker
raises. No untrusted value becomes a module/SDK location. Outcome/no-spawn checks
do not replace implementation audit of the required BEFORE-sysconfig-import/access
ordering or the minimal child's exact environment.

C's separately retained dialect-selection gap is closed in the EXISTING dialect
method only: explicit OAS root jsonSchemaDialect base with Schema.$schema base OR
Draft2020-12 accepts1.0 and rejects true; a Draft or unknown ROOT jsonSchemaDialect,
and an unknown Schema.$schema, refuse unsupported_dialect. These7 added direct
observations plus8 admission observations yield315. No default-latest class is
selected by fixtures; expected codes stay independent literal oracles.

Static preservation check compared parsed ASTs against actual rootfab7d82/source
`ebb23a2542cde9466d20fdea38c52cc5b0901560`: **59 original test-method ASTs and ALL5
Wrapper helpers are EXACT**, as are every external fixture helper/class. The sole
existing-method delta is the explicitly agreed dialect method; all earlier
assertions remain and7 observations append. Only errno is added to module imports;
stdlib clone imports occur AFTER its raw first call. Additive script_result and3
new test methods are the only new class definitions. No module-load fixture,
process, SDK discovery, candidate import or feature call is introduced.

Actual stdlib Python3.10-feature AST parse and compile WITHOUT exec/import PASS;
63 distinct registered method definitions,1090 source lines. Independent AST
preservation assertions PASS. Neither unittest discovery nor module/target/helper/
venv/SDK/feature import or execution occurred. Different actual additive source/
oracle review and root's matching63-case inert harness/config review remain gates
before FIRST intended red. Prior A28/C32 reviews remain valid only for their actual
unchanged scope, not advance approval of this delta or a complete baseline.

Worker prologue/real no-site activation, ABI/root recomputation, exact trusted
metadata/resource integrity and bounded reads/ownership, changed-read races,
secure descriptor traversal, complete ZERO-network SDK integration and every
remaining implementation/resource/source gate remain explicit. Public refusal
codes and parent mocks do not verify all those controls. This adds no public
option/API, runtime capability, provider/platform/live or release claim.

## Final public admission coverage correction before red

Root authorized a focused correction of actualfc5180b/sourcea80b16c after different
C's final read-only audit found TWO observable gaps. Preserve that source's valid
A4347cd admission and C525d dialect reviews and root's observed OLD-a80 green
metadata/discovery harness receipt. Those are not advance approval/observation of
this corrected candidate; no feature/test/red body ran in that green harness.

The REAL clone test keeps its untouched positive and actual code+dist-info removal.
Before removal, keep code AND Version/Tag headers intact; append one newline to
WHEEL, require real dependency_unavailable/closed CLI/exit3, then restore exact raw
bytes in finally. Repeat separately for METADATA. These isolate required SAME-
artifact WHOLE raw hashes: import-only or version/tag-only admission cannot pass.
No expected hash/output comes from the wrapper; no fake validation or install.
Only the disposable copied metadata is modified. Appending bytes never touches the
existing Version/Tag header contents. A wrong Version is necessarily already a
whole-metadata mismatch; no extra redundant wrong-version fixture was needed.

Public main requires EXACT builtin list/tuple before callbacks. Add separate list
and tuple subclasses whose iteration/length/indexing/repr raise. Their construction
uses only ordinary builtin list/tuple arguments, and each test row's subTest label
is a fixed integer. Require literal usage_error/exit64. This catches isinstance or
premature container access while retaining ordinary Poison/StringSubclass rows.
Poison constructors execute only in the test body, never at module import.

Inventory remains63 names. Two added direct argv observations bring the current
authored total to **317 raw public calls**; two extra REAL clone-script observations
bring the current extra-script count to **5**. All6 existing Wrapper helper ASTs,
all prior external helper/class ASTs and61 existing test ASTs remain EXACT versus
fc5180b. Only the two authorized test methods change; two inert poison classes
are added. AST3.10/compile-without-exec and preservation assertions PASS. No import,
test/helper, clone/venv, candidate/SDK, child, feature or intended-red run occurred.
Different actual correction review and refreshed matching green harness observation
remain required; final red attribution must inspect actual final stub frames.
All previously listed real worker/source/integration/offline/ownership gates remain.

## Persistent poison evidence — actual pre-red correction

Different A's actual090e1a3/source6969 review found a concrete false-green path:
an incorrect main can call a poison callback, catch its ordinary AssertionError,
then return expected usage_error. Root authorized the same correction for the
older Poison and StringSubclass callbacks, which had that identical gap. Public
API§1 requires rejection WITHOUT conversion/iteration/representation callbacks;
the expected refusal code alone cannot prove that property.

Add ONE test-module POISON_TOUCHES ledger, never reset during the process. Every
one of the13 callbacks in Poison/StringSubclass/ListSubclass/TupleSubclass appends
a fixed literal label BEFORE its original raise. Add ordinary unittest tearDown
requiring the ledger equal the empty builtin list. It inspects no poison object,
and cannot erase a touched callback even when the implementation catches its
exception. All integer subTest labels and original public observations stay exact.
No test-derived input/exception text is added to product diagnostics.

A correct immediate raw feature stub invokes no poison callback, so this teardown
passes during genuine marker red; it neither catches/maps the marker nor adds a
new test registration. No ledger clearing, callback reset, successful fake result,
fixture weakening or source behavior appears. Module import merely creates an
empty test-only list; callbacks and teardown run only during actual later tests.

Static comparison to090e1a3: **ALL63 existing test-method ASTs and ALL6 existing
Wrapper helper ASTs are EXACT**. Add tearDown; prepend only the literal ledger
append to each of13 existing poison callbacks, retaining its prior full body.
Other fixture helpers, old metadata hash mutations and restores, names/counts
(63 registrations/317 raw calls/5 extra scripts) remain unchanged. Python3.10-
feature AST parse/compile WITHOUT exec and exact preservation assertions PASS.
No source/helper/test import, feature/red body, clone/venv, SDK or child ran.
Actual different correction review and refreshed green harness observation still
precede final intended red; preserve all prior receipts without extending them.
