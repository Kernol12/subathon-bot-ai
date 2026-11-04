# LOCALETEST-001 independent concrete API document review

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
