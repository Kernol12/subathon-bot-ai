# LOCALETEST-001 independent concrete API document review

**Current verdict: APPROVED actual revised API document8a31d9d.**
The final section pins the accepted artifacts; earlier changes-required findings
remain preserved. This is not approval of future Python/stub/harness/test artifacts.

Reviewer service_reviewer, different from author service_author,2026-11-04.
Initial actual author895b500/rootec662b1 imported into reviewer066b1da. Owns only
this new API review note; the supported catalog review947eba4 remains valid and
its earlier failed-mapping history is untouched.

## Actual initial API candidate: changes required

| Inspected artifact | Git blob |
| --- | --- |
| [translation-catalog-api-v1.md](../contracts/translation-catalog-api-v1.md) | 7f1738b6da682f136895bbb2f2ce4b5bdfe95fb5 |
| [API author handoff](localetest-001-api-author.md) | c247d91a76aae7021976135a6d8834c593c6f8c6 |

Read the full document against specification v0.45 section12/I18N-04 and the actual
supported catalog contract51383296. The small two-pure/two-file/main surface is
appropriate. Catalog/artifact/result returns own frozen tuple/str/bytes data;
rendering does not accept a caller-made Catalog as a validation certificate.
Named template/Unicode/count metadata and corrected empty-context serialization
remain explicit. No runtime formatter or application preference is hidden here.

The proposed marker remains distinct from expected validation errors, immediately
throws from all five functions, and has an explicit CLI86 sentinel. Import/build/
discovery must stay inert: no configured build runs the unimplemented generator.
Actual literal third-party-only Qt foundation must compile/load real QM and be
green before a valid project stub-red; the previous scratch56-check probe does not
replace independent project fixture/test review or its own environment gate.

Three bounded document clarifications are requested before approval:

1. **A1 — practical real timeout verification.** A fixed120-second deadline has no
   inexpensive actual timeout case. Add a small validated lower deadline, such as
   a direct BuildRequest integer tool_timeout_ms1..120000 with default120000 and
   unchanged CLI default. This lets focused tests exercise a real failed child
   without a fake successful compiler, injected process authority, sleeps lasting
   minutes or a complex fake-clock framework. Invalid limits must reject before
   work; the maximum process/capture promises remain bounded.
2. **A2 — artifact byte bounds.** Child stdout/stderr is bounded, but actual staged
   QM file sizes/digest reads are not. Specify a finite engineering per-file QM
   ceiling with size/type checks and bounded chunked digest/compare reads, plus
   exact overage error attribution. A conservative ceiling such as64MiB is well
   above the bounded4MiB source profile; it must not truncate bytes or accept a
   partially read successful output. Existing target mismatch checks must not
   load unbounded target bytes simply to compare them.
3. **A3 — source file type before blocking open.** Require the source to resolve
   to an ordinary regular file before attempting a potentially blocking open,
   then verify the opened identity/type before reading. A FIFO/device is not a
   source catalog; opening a FIFO first can hang before the promised bounded read
   or io_failure. Keep permitted source symlinks to regular files and the existing
   serialized local-host/no-hostile-race boundary; no new file-service guarantee is
   requested.

These are routine API/resource/testability choices, sent to author/coordinator as
one document-only batch. Existing source contract, canonical TS, Qt mapping,
metadata packaging, output ownership and accepted review are not reopened.

## Remaining review/implementation gates

CLI safe JSON/768-byte errors and stable exits must preserve exact failure truth;
no child text/path/body or traceback becomes a normal public diagnostic. Per-file
replacement may honestly leave old/new output mixture on publication I/O failure;
no two-directory transaction is claimed. check remains read-only, compares actual
bytes rather than hashes alone, and validates source/inventory/tool aliases before
process/publication. The selected tool is real and pinned by the host, not proven
by a version string alone. Initial feature methods do none of this behavior.

Actual initial-document checks: check_project PASS396 tasks/122 decisions/local
links/presets/current HTML; progress render --check PASS; diff --check PASS.
No Python interface/stub, schema/catalog, generator, fixture/test, native harness,
lrelease compilation, Qt lookup or project feature/red invocation was authored or
executed for this API review. Approval of the actual revised document, minimal
source/harness review, independent fixture/test review and selected green foundation
remain required before first intended red and production implementation.

## Actual revised API document: approved

**APPROVED**, independent reviewer service_reviewer,2026-11-04. Actual author
8a31d9d/rootce1e069 imported into reviewer3fdaf31. Accepted API document blob
acb90189c371ab4aa8979cbb2ac8d117b2e8ab0e and API author note
1cfe205bad45324023bcbb8496f8ddccaf52d432. The separately approved source contract
5138329642d112423431d0314a6f9f9a78c7795d and its author note
a203ad4bbba2978bcdcea276de603ee8a75d7d04 remain byte-identical.

Read the complete revised API and actual correction diff. A1–A3 are resolved:

- A1 supplies one ordinary direct API deadline setting: exact non-bool integer
  tool_timeout_ms1..120000/default120000. The eventual file methods reject malformed
  request/limit before paths or work. CLI retains its existing fixed default and
  gains no flag/environment override. Focused failure tests can exercise a real
  sleeping child with a short deadline; no fake-success executor or clock authority
  is added. Process stdout/stderr stays bounded and both pipes must drain without
  deadlock; timeout/cap handling terminates/reaps only the invoked child.
- A2 gives each staged QM/returned text artifact a64MiB engineering ceiling, with
  pure text sizing before oversized materialization. Staged QM lstat/type/size and
  opened fstat identity/size precede bounded digest/copy reads. Chunks<=65536 and
  exact counted EOF prevent unbounded or partial successful digests. Compiler exit0
  is retained honestly when its file is missing/invalid/oversized; output_file_limit
  does not fabricate a nonzero process exit. Existing oversized target mismatch
  rejects from size before reading its body. A child may have written beyond the
  ceiling before exit; this API promises rejection/read/publication bounds, not a
  filesystem quota. Negative-only sparse output helpers do not prove Qt success.
- A3 resolves/stat-checks an ordinary regular source before any potentially blocking
  open, preserves allowed symlinks to regular files, and verifies opened identity/
  type before its4MiB+1 bounded read. FIFO/device/socket/directory/invalid targets
  are rejected without opening that type. The existing serialized local-host
  boundary remains; no hostile concurrent actor isolation is invented.

Other reviewed boundaries remain clear and consistent: frozen tuple/str/bytes
outputs and fresh source validation; deterministic JSON token/schema/template
priority without host big-integer/float assumptions; actual canonical empty-context
Qt/browser bytes; six owned artifacts and source/tool alias protection; actual
selected Qt version/process/file outcomes; per-file publication with honest mixed
old/new state after I/O failure; read-only full-byte check; safe fixed metadata and
CLI exits/help/version. A version line alone is not an archive trust claim.

The five initial entry stubs must still throw TranslationCatalogNotImplemented
IMMEDIATELY, including main/help/version, with no argument/source/path/tool work.
Only the guarded script marker wrapper emits its explicit exit86 JSON sentinel.
No configure/build/discovery path may invoke a new project feature; compiled/import
failures or skips cannot become intended red. The future independently authored
Qt foundation uses actual pinned tools with literal independent TS/QM/raw outputs
and must pass before first project marker-red. The scratch56-check investigation
is still only third-party feasibility, not the project baseline.

### Independent document checks and remaining gates

A stdlib-only audit checked exactly five declared entrypoints and the explicit
4MiB/64MiB constants. A maximum-sized valid safe diagnostic metadata projection
(message ID160,parameter32,all optional fixed fields present,large actual OS exit)
serialized to364 bytes including LF, below the768-byte ceiling. No source paths,
translation bodies or child output appear. This is a document bound check, not
execution of a project CLI or its future error formatter.

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0;396 task records/122 decisions/local links/presets/current HTML pass.
No Python interface/stub, generator, schema/catalog, fixture/test, native harness,
lrelease compilation, QM lookup, formatter or intended project red was authored
or executed for this API review.

This approval covers the exact two revised API DOCUMENT files only. Coordinator
may release the next bounded minimal source/interface/stub and harness stage,
which requires a different actual artifact review before independent fixture/test
authoring and valid-red gates. It does not approve future code, permit generator
execution at build, finish LOCALETEST-001/LOCALE-001, or establish product runtime/
preference/packaging/platform/full-release support. All remaining scope is retained.
