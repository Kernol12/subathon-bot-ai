# Private SDK installation admission — proposal v1

SCHEMATEST-001. Refines the [public API](schema-validation-tool-api-v1.md) and
[private process plan](schema-validation-process-test-v1.md); logical/context/public
outcomes, limits and package phases remain unchanged. This fixes the previously
unspecified installation discovery needed for independent admission tests. No
installation, worker, validator or public behavior is implemented by this document.

## Bound parent record and one SDK root

The private immutable parent record contains its absolute sys.executable spelling,
normalized absolute SDK directory, implementation name, major/minor, sys.platform,
os.uname().machine and compile-time SOABI/Py_DEBUG/Py_GIL_DISABLED values. These
come ONLY from the actual running interpreter and standard-library sysconfig, not
environment variables, manifest/package data, an arbitrary executable option or a
serialized caller record. Preserve the executable spelling when launching: resolving
a venv interpreter symlink to the base binary can lose the selected environment.

This selected installation profile is Linux x86-64, ordinary non-debug CPython3.10,
3.12 or3.14 with the GIL. SOABI must be respectively
cpython-310-x86_64-linux-gnu, cpython-312-x86_64-linux-gnu or
cpython-314-x86_64-linux-gnu. Those are the three hash-admitted/tested targets,
not a claim that every Python>=3.10 ABI/platform is verified. Other configurations
return environment_unavailable; public declarations remain importable on Python>=3.10.
No Windows/product runtime support is removed by this developer installation profile.

The parent must be in a dedicated stdlib venv: its actual prefix differs from its
base prefix, and the normalized prefix equals the parent of the executable's bin
directory. That root contains a regular pyvenv.cfg of at most4096bytes. Discover
purelib and platlib with the parent's stdlib sysconfig, normalize their absolute
existing directory identities and require they name ONE directory. Do not append
an unreviewed second native-extension root. Retain the public4096-byte bounds for
all trusted paths. A nonexistent/ambiguous/unbound root or unsupported layout is
environment_unavailable, not missing-package proof. This does not claim a system
Python/global installation is admitted.

A pre3.14 -S parent whose prefixes cannot prove that venv binding returns
environment_unavailable; it must never guess a base-interpreter SDK root. Normal
isolated venv startup supplies the parent record BEFORE the worker starts -I -S.
The child cannot derive SDK roots from its pre3.14 -S prefixes. Parent descriptor
admission checks configuration and binding, not the presence of all selected
packages; a missing selected distribution belongs to the later dependency phase.

## Child recomputation and metadata admission

Only the SDK directory is transmitted in the existing private argv; no untransmitted
parent tuple is assumed to be available. After establishing/verifying the fixed
RLIMIT and BEFORE package reads or candidate imports, the worker recomputes the
actual implementation/minor/platform/machine/compile-time ABI against the exact
profile above. It retains its absolute executable spelling and derives the venv
root from that executable's bin parent, checks the bounded regular pyvenv.cfg, and
uses stdlib sysconfig.get_paths(scheme="posix_prefix", vars={"base": venv_root,
"platbase": venv_root}) to project purelib/platlib WITHOUT relying on its -S prefix.
Their normalized existing identities must equal the one received SDK directory.
Any inability to establish this controlled configuration is environment_unavailable.
No candidate path is appended or imported during this prologue.

The actual selected-distribution check occurs ONLY at API dependency phase6, AFTER
complete package parse/dialect/format preflight. Read the trusted repository's
[wheel manifest](../../tools/schema_validation/wheels.json) beside the worker, then
check each of the exact20 selected distributions in the admitted SDK directory.
For each, select an artifact whose candidate_targets contains the actual cp310,
cp312 or cp314 target. Its exact dist_info_directory names the expected installed
metadata; no arbitrary directory scan/import supplies a guessed alternate version.

Read WHEEL at most4097bytes (ceiling4096) and METADATA at most262145bytes
(ceiling262144), reject incomplete/changed/nonregular metadata reads, and compare
the WHOLE raw SHA256 against that artifact's wheel_metadata_sha256 and
distribution_metadata_sha256. Both must match THE SAME admitted artifact. These
identities were extracted from the already whole-wheel-hash-admitted archives;
METADATA binds the exact version and WHEEL binds the selected ABI/platform tags.
Missing, wrong-version, malformed, mismatched or unusable selected distributions
return dependency_unavailable at this phase, never early environment_unavailable.
Read only these20 fixed pairs; the metadata working set is bounded separately from
package-file budgets and remains under the worker's existing memory/deadline limit.

Only after all metadata matches may the worker append the ONE bound directory
directly to sys.path and import the explicit real selected backends. Never call
site.main/addsitedir, process .pth files, add cwd/user-site/system-site fallbacks or
resolve package URIs with dependency search paths. Import failure/unusable selected
backend is dependency_unavailable. No skipped backend or alternate version is ok.
The exact hash-admitted bundled metaschema resource allowance remains unchanged.

This metadata check binds distribution/ABI identities for an already trusted
installation. It is not a claim that metadata hashes authenticate every installed
code byte against a malicious local administrator. Package-controlled data cannot
write the trusted installation; no new hostile-installation success guarantee is
introduced. Original archive hashes remain mandatory for installation.

## Independent negative fixtures and parent facility seam

A dependency fixture creates a FRESH disposable stdlib venv with with_pip=False
and system_site_packages=False. It copies the actual admitted single site-packages
root into the fresh root; do not relocate a whole venv with stale scripts/prefixes.
No pip/network/install operation is used. First the untouched clone must validate a
literal valid package using the REAL public script/SDK. Then remove the real
openapi_spec_validator package directory AND its matching .dist-info directory;
the correctly bound clone must return dependency_unavailable, exact CLI line/exit3,
with no secret/package/exception text. This is a real missing distribution, never
a fake validating worker. The raw direct public call remains FIRST before clone
construction in this authored behavior case, preserving named-marker attribution.

For one unsupported-parent-facility case, tests may patch ONLY the stdlib module
attribute os.pipe to raise OSError(errno.ENOSYS). Expect environment_unavailable,
no Popen call and no public output. This explicitly tests an injected missing
PARENT pipe capability; it does not claim the host lacks pipes, simulate child
RLIMIT failure, or prove child file/ABI/resource protection. The existing actual
Popen/time fault seam is unchanged. ENOSYS is not EMFILE/resource exhaustion.

Real worker prologue/ABI/root/no-site enforcement, exact trusted-resource integrity,
secure open/read-race handling, bounded allocation and every SDK no-fetch path
remain integration and source-review obligations. Parent refusal mocks cannot prove
them. Keep these gates explicit through implementation; never label all controls
verified from a public outcome code or use a fabricated selected-library success.

A DIFFERENT reviewer must approve the ACTUAL document and metadata fields before
these additive tests depend on them. Actual test/oracle/harness review and genuine
missing-wrapper red still precede implementation. These are private developer-tool
admission refinements, not new public signatures/options or reduced release scope.
