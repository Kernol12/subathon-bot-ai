# Independent installation-admission document/data review — SCHEMATEST-001

Reviewer `/root/session_plan_review`, different from document/data author and
integrator `/root`,6 November2026. **APPROVED exact DOCUMENT + metadata identities
ONLY**, final candidate `0bcdc4a117bcd137206feaa291e57994862f06c2`, installation
document blob `86d6d482e767af3d57d4da58d7fd644a9383c34e`, wheel manifest blob
`6f4ffec82e73274bebeac2ea1aa0ad31e28f4539`.

Read actual initial3fbf2e4, os.pipe precisioncb74ecd, boolean precision774db1a and
source-backed override correction0bcdc4a. NEW isolated review branch
`task/SCHEMATEST-001-installation-review`, worktree
`/tmp/subathon-schematest001-installation-review`, starts atcb74ecd; later exact
document diffs/blobs read with git. ONLY this review note changes. Intervening root
test-source integration is not attributed to either installation-document commit.

## Scope, binding and actual source finding

Reviewed root/tools instructions, workflow, [paired brief](../tasks/SCHEMATOOL-001.md),
accepted logicalabb426/context32ec87/publica526dbc/private-process7f30d99 and original
[dependency admission](schematest-001-dependency-review.md). Public calls, enum,
manifest, package budgets, first-failure priority and immediate named stubs remain.
This is a bounded isolated developer installation profile, not global/server/client
dependency permission or a reduction of Windows/Linux product scope.

The parent record preserves actual sys.executable spelling and binds the actual
venv prefix/bin root, single normalized existing purelib==platlib directory and
ordinary Linux x86-64 CPython3.10/3.12/3.14 ABI. Resolving an executable symlink to
the base binary cannot silently replace that binding. Unsupported layouts/ABIs or
unprovable pre3.14 -S parent prefixes are environment_unavailable. Public module
declarations remain importable on Python>=3.10; no broader runtime proof is inferred.

Actual774 clarifies bool-normalized debug/free-threaded configuration flags: absent/
None is false, including older CPython with no free-threading config key. It does
not mistake an absent older flag for an unsupported nonzero ABI.

I found a real gap in relying on isolated startup alone for environment-independent
sysconfig. Versioned CPython3.10/3.12 source reads _PYTHON_SYSCONFIGDATA_NAME directly;
3.14 also reads _PYTHON_SYSCONFIGDATA_PATH, and _PYTHON_PROJECT_BASE affects source/
configuration discovery. -I does not erase these explicit os.environ reads. Reported
before final approval. Actual0bcdc4a now rejects PRESENCE, even empty values, of all
three BEFORE parent sysconfig import/configuration access, returning
environment_unavailable without mutating the caller environment. Exact child
environment excludes them. This closes the written source finding; future actual
test source and implementation must enforce it, not merely repeat the prose.

Only the existing SDK path is transmitted; the worker does not pretend to possess
an untransmitted parent tuple. After verified RLIMIT admission and before package/
candidate work, it independently checks its runtime and executable-root binding,
bounded regular pyvenv.cfg and explicit posix_prefix base/platbase projections.
It does not obtain the SDK root from pre3.14 -S sys.prefix. Single-root identity
must match the received SDK path. These are configuration checks; distribution
absence remains a later dependency_unavailable, preserving complete package parse/
dialect/format precedence over selected-library availability.

Primary [CPython3.10.12 sysconfig](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/sysconfig.py),
[3.12.3 sysconfig](https://raw.githubusercontent.com/python/cpython/v3.12.3/Lib/sysconfig.py)
and [3.14.9 sysconfig](https://raw.githubusercontent.com/python/cpython/v3.14.9/Lib/sysconfig/__init__.py)
were independently opened/read. Their explicit schemes and variable expansion
support the proposed root projections;3.14 includes ordinary/free-threaded suffix
handling. [Python path initialization](https://docs.python.org/3.14/library/sys_path_init.html)
documents the prefix/-S version difference. This is source feasibility, not a new
observed3.10/3.12/3.14 worker execution or proof all distributor layouts match.

## Independent archive and installed-metadata audit

Standard-library json/pathlib/hashlib/zipfile inspection ONLY; no SDK imports,
installs, validators, children, feature calls or tests. Compared the new manifest
after stripping precisely its three added identity fields/artifact and restoring
the old status with the prior manifest: EXACT equality. All20 packages,28 original
artifact filenames, versions, Requires-Python/Dist, targets, whole-wheel hashes/
sizes and declared license/material hashes remain. Lock blob
`859d7cfaa04a207a3052839e9622287566770259` and THIRD_PARTY_NOTICES blob
`2427eaee13c21d60553ade6fc21c11bade491907` remain unchanged. The updated status
describes already observed fixed isolated SDK facts, not working wrapper behavior.

For EVERY target copy in `/tmp/subathon-schema-pinned-cp310`, `-cp312`, `-cp314`:

- Exactly one selected artifact per package/target;20 packages,28 unique filenames,
  60 actual target copies. Recomputed whole-file SHA256/size BEFORE trusting ZIP data.
- Its exact dist_info_directory contains exactly one WHEEL and METADATA member.
  Whole raw member hashes equal wheel_metadata_sha256 and
  distribution_metadata_sha256 from that SAME admitted artifact. No cross-artifact
  pairing or version-name guess satisfies the check.
- All original bundled LICENSE/NOTICE hashes rechecked without modification.
- Maximum observed WHEEL is224bytes; maximum METADATA141528bytes, inside the new
  4096/262144 ceilings. Bounded +1 reads/nonregular/changed-read refusal remain
  implementation requirements, not inferred from these small actual archive sizes.

All60 pairs PASS. Actual installed CPython3.14 SDK's20 WHEEL/METADATA pairs also
byte-equal their admitted archive members. No installed3.10/3.12 observation is
claimed by this reviewer. Metadata establishes selected distribution/ABI identity
in an already trusted installation; it does not authenticate every code byte against
a malicious local administrator. Original hash-only installation and pinned bundled
resource provenance remain mandatory. Native load/backend failure is still a real
dependency failure, never skipped success or fallback to an unreviewed version.

## Fixture feasibility, allowed next step and remaining gates

The negative dependency recipe uses a FRESH with_pip=False/no-system-site stdlib
venv and actual copied admitted single SDK root, with a REAL untouched positive
control followed by removal of openapi_spec_validator code AND dist-info. It does
not relocate a whole environment or supply a fake validator. Python's [venv docs](https://docs.python.org/3.10/library/venv.html#how-venvs-work)
explain why recreating the environment avoids stale absolute-path relocation.
The actual raw direct public marker call precedes clone setup in the later test,
so an environment construction failure cannot manufacture FIRST intended red.

The explicit module os.pipe/no-alias/pipe2 constraint permits the one parent ENOSYS
fault injection with no Popen/public output. Its scope is honestly parent refusal,
not an unavailable real host facility, EMFILE exhaustion, worker RLIMIT/ABI failure
or successful domain validation. Actual test source must verify no spawn, cleanup,
phase attribution and unchanged ordinary real-SDK paths. No such test is approved
merely from this document.

The test author may now AUTHOR these exact additive admission cases. Different
actual test/oracle review and root's concrete minimal metadata/projection/static
discovery harness/config review still precede execution. Actual selected minimum/
CI -I-S projection and metadata checks remain observations to obtain through that
reviewed harness, preserving the14 SDK source/guards. Complete independent baseline
review and genuine missing-wrapper red still precede production. Real worker
prologue/no-site/ABI/root/resource/secure-read/no-fetch/ownership guarantees need
integration and later implementation/source review. No complete baseline, worker,
offline/security certificate, product/platform/live or release completion is claimed.
CONTRACT013, all7 integrations and3 FX providers remain in scope.

Actual independent archive/metadata/license/installed-pair/old-field assertions PASS;
`python3 tools/check_project.py` PASS426tasks/122decisions/links/presets/current HTML;
`python3 tools/progress.py render --check` PASS; exact3fb/cb74/774/0bcdc document/data
diff checks PASS. No candidate source, SDK, fixture/helper/test/feature/marker body,
native child, install, environment mutation or GitHub operation ran here.
