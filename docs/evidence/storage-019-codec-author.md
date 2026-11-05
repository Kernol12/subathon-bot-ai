# STORAGE-019 — bounded structural codec implementation author

Author `/root/capture_codec_tests`,5 November2026. Dedicated support branch
`task/STORAGE-019-codec-author`, worktree
`/tmp/subathon-storage019-codec-author`, based on coordinator claim36605ee after
actual main8435fb0 integrated PR68. Explicit production assignment followed the
completed reviewed27-codec FIRST red; that historical red was not replayed.

Scope: `server/src/reference_capture_values.cpp` and source-private
`server/src/reference_capture_codec_p.hpp` only, plus this note. Public ABI,
SQL store/factory stub, contracts, assertions/fixtures, build/CI/state/pins remain
unchanged. Root separately owns stale public-comment update and CI promotion.
Authority: [task](../tasks/EPIC-009/STORAGE-019.md), concrete
[codec§3](../contracts/sqlite-initial-captures-v1.md), consumer§2 and integrated
[baseline](storagetest-018-handoff.md). This is structural DATA, not admission,
rule evaluation, current authority, provider proof or durable capture behavior.

## Implementation and source-audit map

All eight functions share borrowed typed views, complete validation and final
ownership boundaries. Four fixed format grammars remain explicit. No generated
or implementation-derived expected bytes are introduced.

- `validate_limits` checks every positive limit against size/ptrdiff/Qt signed
  ceilings before narrowing/addition, with reserved n+1 for actual std::string and
  QByteArray per-container profiles. Fixed I0 still requires valid positive item
  and clock-journal ceilings. `Size` checks subtraction before addition.
- `borrow` captures references as std::string_view and scalar DATA only. Full-width
  TimerEndMode/GraceReuse/ApplicationPrincipalKind values stay signed64 until tag
  validation, so typed256/-1 cannot wrap to an accepted U8. Fact tags remain their
  actual uint8 width. No source clone, container sort/cache or normalization occurs.
- Complete input B uses every copied reference and fixed scalar/optional/tag byte,
  plus actual payload, before any semantic check. Configuration base26+refs,
  arrival base35+refs+payload, execution base33+refs, initial base24+refs preserve
  the independent fixture60/73/70/55 counts. Every actual individual B payload
  fits U32; no arbitrary global whole-document U32 restriction is added.
- `validate` stages bounds→all tags→all required references→shape→numeric values→
  bindings. Known account0 is invalid_binding; unknown principal is invalid_tag.
  SRC unknown0 fact tags are structural DATA. Null original payload is rejected,
  signed timestamps/deadlines preserve the full width, CID0 is real, SRX revision
  positive throughMAX versus SRI exact1.
- `Reader` scans fixed framing, exact magic and every field/trailing byte before
  semantic validation. Optional0/1 framing failures are invalid_document and no
  unreachable later field error is invented. SRA scans its embedded SRC directly
  into its nested view; no temporary owned configuration document/envelope exists.
- Static assertions bound each complete borrowed-view payload to512 bytes.
  Largest record count is SRA:18actual B fields+one embedded-frame span=19<=64;
  parsing depthSRA→SRC→rule/config is at most3. No variable collection/index tree
  or recursion exists. Fixed Reader/sizing stack is separate from view payload.
- `Sizing` walks the same canonical grammar before `Writer` reserves output.
  Full format+document B, document ceiling, per-field limits/U32 framing and
  destination capacity are checked before ownership. SRA writes embedded SRC
  directly into final output after exact sizing, not through an owned envelope.
- `own` runs only after the entire decoded borrowed DTO fits and is semantically
  valid. Present-empty payload explicitly materializes a non-null QByteArray.
  `allocation_boundary` catches own bad_alloc/length_error into the allocation-free
  fixed ReferenceInputFailure(allocation_failure) around every public codec and
  private body path; partial RAII values clean themselves. No SQL/mutation exists.

The private header exposes four typed body decoders and a two-value BodyBounds
selector, not a generic parser/SQL bypass. Known function role supplies exact
magic; it never sniffs a document to choose a role. Public-input path preserves
public input_limit semantics; retained mode maps configured body/decoded-reference/
owned overages to result_limit while preserving malformed categories. Every actual
nested reference is preflighted under the supplied original ceiling before owned
materialization. No widened public profile then postcheck occurs. Fixed supplied
adapter role metadata is absent from a stored body, so it creates no artificial
selected reference35/37 under a SQL reference26 profile. Public codecs STILL
bound their actual supplied format_ref and count it in the complete envelope B.
Future SQL integration must keep actual selected metadata/preownership guards and
separate source review; no SQL capability or retained native pass is claimed here.

## Actual author checks

Environment: GNU16.2.1(20260810), CMake4.4.5, QtCore/Sql/Test6.12.0, selected
Boost1.83 SDK under/tmp, host Linux. Disposable synthetic state only.

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug -j 4
ctest --preset sqlite-reference-capture-debug -L '^reference-capture-values$' --no-tests=error --output-on-failure --output-junit /tmp/storage019-author-codec27.xml
ctest --preset sqlite-reference-capture-debug -LE '^draft-red$' --no-tests=error --output-on-failure --output-junit /tmp/storage019-author-stable309.xml
```

Configure exit0. Initial build had an ordinary compilation defect: private helper
name `emit` collided with Qt's macro. Renamed to `write_document`; no macro/API/test
change. Original build log remains/tmp/storage019-author-build.log. Corrected
incremental111-step build exit0, zero compiler warnings/errors, log
/tmp/storage019-author-build-corrected.log. This is not a missing-feature red claim.

Unchanged27 codec acceptance PASS exit0/0fail/disabled/skipped in0.10s.
Raw text SHA256a7f72b356bedec6e723644095b0369ad656717884184854e7192ea53090a10a2;
JUnit SHA2561cad9b748ab68b7b595ae4ff403c7424ea983e5f657529ee9f86d854d84c6679.
Unchanged stable309 PASS exit0/0fail/disabled/skipped in25.42s, including mandatory
old-primitive capture diagnostic. Raw text SHA256
022a002ca1ae4e46dbba6760eb5e51e90a8801a4c172d3b82265f7a7f36c69b2.
Actual LastTest.log has0QWARN/0SKIP. SQL31 remains registered intentionally red
and was NOT invoked. Original valid27 red receipt remains historical untouched.

`clang-format --dry-run --Werror` on both authored source paths and
`git diff --check`: exit0. Frozen codec CPPc207e423/header53da620d and SQL stub
3372f37d remain exact. No test, fixture, schema, public header or shared file change.

Different source/private-interface/config audit, actual root integration checks,
ordinary selected CI27 promotion/current-head results and PR69 publication/merge
remain required. Passing27 plus309 does not establish allocation/scratch correctness
without source audit or complete SQL/API/provider/recovery/UI/platform release.

## Independently reviewed decoder-priority regression and bounded correction

Different source reviewer `/root/capture_reviewer` found originalc2db's outer
helper checked empty format/null body semantics before unsupported format,
contrary to the explicit consumer§2/concrete§3 decoder sequence. Different
regression author `/root/capture_fixture_author` added one slot covering all four
decoders with empty-format/valid-body and unknown-format/null-body values,
plus existing bounds/known-format semantics. Original27 assertions stay intact.
Actual regression0deb60d received separate review before root execution.

Root observed actual targeted behavior red at dbe8fa3,19:45:12Z: exit8,
8 category mismatches,2init/cleanupPASS/0skip/setup failure. Empty format returned
actual invalid_reference3 instead of unsupported_format7; unknown format/null
returned invalid_shape4 instead of unsupported_format7. This is reached behavior,
not a missing marker or a preparation error. Root preserves raw
/tmp/storage019-priority-red.txt SHA256
56911fd705b32b7e0f7a7d8d94c086f82d13b06c8d5ec628ece1bc0db6de4afe.
The source author did not claim that root command as their own.

Only AFTER that valid reviewed regression red did root authorize this bounded
source correction. Fresh support branch `task/STORAGE-019-codec-priority-fix` at
`/tmp/subathon-storage019-codec-priority-fix` starts from actualdbe8fa3, including
root's reviewed regression/harness. `outer` retains ALL option/field/whole-envelope
bounds first, then rejects a nonmatching supplied format as unsupported_format,
then checks known-format null body as invalid_shape. The empty-format-specific
invalid_reference check becomes redundant and is removed. No other codec/body
logic, private interface, baseline assertion, ABI or shared configuration changes.

Actual author validation on that exact bounded source:

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug -j 4
ctest --preset sqlite-reference-capture-debug -LE '^sqlite-reference-capture$' --no-tests=error --output-on-failure --output-junit /tmp/storage019-priority-author-combined337.xml
```

Fresh configure/full145-step build exit0, zero warnings/errors. Combined337
(28 codecs including the additive regression+old stable309) PASS exit0 in24.74s;
0fail/disabled/skipped, LastTest.log0QWARN/0SKIP. JUnit contains exactly28 codec
cases and ZERO SQL capture cases. Raw text SHA256
340b0c1ef7c2d0defa8a3b851cfccab943afaf397959ac01cc2dcb1679b129fd;
JUnit SHA256ffd78b935f2f7bd4eb5bdaed8945ff47e7cb4cbaabe42346a1cc3e32c83012e5.
Formatting/diff checks exit0. Environment remains the selected author host above.
SQL31 is untouched and uninvoked; historical27 marker red is not replayed.
The fix commit owns ONLY codec CPP and this author note; regression/harness
ancestors are already root-owned and should not be duplicated on import.
Different actual revised source/config review and selected ordinary CI28 remain
root's publication gates before task completion/merge.
