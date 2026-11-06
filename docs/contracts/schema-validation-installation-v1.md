# Private SDK installation admission — v1, revision2

SCHEMATEST-001 / SCHEMATOOL-001. Revision2 has actual different document approval
C48c07527; native source/build/test/execution gates remain. Refines the [public API](schema-validation-tool-api-v1.md) and
[private process plan](schema-validation-process-test-v1.md); logical/context/public
outcomes, limits and package phases remain unchanged. This fixes the previously
unspecified installation discovery needed for independent admission tests. No
installation, worker, validator or public behavior is implemented by this document.

## Explicit local native build prerequisite

[ADR-0009](../decisions/0009-schema-native-process-owner.md) and the
[native process contract](schema-validation-native-process-v1.md) select one
first-party C++20 process library using existing CMake/Ninja, with no Qt or new
third-party runtime. After actual contract/build/test review, build explicitly
with preset schema-transport-debug and target subathon_schema_transport_v1.
The sole artifact is
`<repository>/build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so`,
derived from the trusted script's repository, never cwd/package/environment search.
Record exact source/configuration/compiler/libc/architecture/artifact SHA256 and
reviewed symbol/call-graph provenance. No automatic build, download or alternate
search path is admitted; no machine-specific toolchain setting is committed.

The exact ABI1 pointer/length types, closed integer mapping and sole versioned
production symbol are frozen in that contract. Use stdlib ctypes.CDLL only after
public/path/ABI/sysconfig-override admission, with builtin types and no callbacks,
custom converters or errcheck. Import/help neither load the artifact nor inspect
the SDK/build files. Missing/unloadable/wrong-architecture/unbound-symbol native
installation is environment_unavailable; known Python MemoryError is resource_limit.
This prerequisite is separate from the phase6 dependency check: a missing native
artifact never proves a selected SDK distribution is missing. Trusted loading is
not an authentication scheme against an administrator replacing local binaries.

The positive profile additionally needs actual reviewed clone3 atomic pidfd,
close_range, raw signal/clock/wait/rollback and loader/ABI foundations on BOTH
selected CI hosts. Kernel version alone or a green Python-only receipt is
insufficient. Unsupported local facilities fail closed; selected positive CI
must actually establish and exercise them without skip/fallback. No broader
Python or product-platform claim follows. Python3.14 remains separately observed.

## Bound parent record and one SDK root

The private immutable parent record contains its absolute sys.executable spelling,
normalized absolute SDK directory, implementation name, major/minor, sys.platform,
os.uname().machine and compile-time SOABI/Py_DEBUG/Py_GIL_DISABLED values.
Normalize both configuration flags with bool(...); a missing/None value is false,
including older CPython without a free-threading flag. These values come ONLY from the actual running interpreter and standard-library sysconfig, not
environment variables, manifest/package data, an arbitrary executable option or a
serialized caller record. BEFORE importing sysconfig or accessing its configuration,
reject the PRESENCE of _PYTHON_SYSCONFIGDATA_NAME, _PYTHON_SYSCONFIGDATA_PATH or
_PYTHON_PROJECT_BASE in the parent environment as environment_unavailable, including
empty values. CPython sysconfig reads these overrides directly even with -I; isolated
startup alone is not proof of neutralization. Do not silently mutate the caller's
environment. The exact minimal child environment excludes all three keys.
Preserve the executable spelling when launching: resolving
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
directory. That root contains a regular pyvenv.cfg of at most4096bytes. The Python
parent derives prefix/sysconfig/path identities without opening that file. Its
actual bounded no-follow/nonblocking open, regular-file/snapshot checks, at most
4097-byte read and close now occur INSIDE the single native resource interval,
before child birth, under the native contract. Thus Python allocation/unwinding
cannot separate a configuration FD acquisition from its cleanup. Failure remains
environment_unavailable (known resource refusal remains resource_limit); no package
file is read or candidate imported here. Child checks below remain independent.
Discover
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

Revision1's exact os.pipe ENOSYS interposition is now HISTORICAL: revision2
explicitly ports it to one real native pipe2 capability-refusal phase through
the reviewed negative-only SAME-core adapter. Preserve the exact
environment_unavailable/no birth/no public output assertion and before-fallback
resource evidence; require actual port-source review and intended red. An injected
parent ENOSYS still does not claim absent host pipes, child RLIMIT/ABI/file proof
or resource exhaustion. EMFILE/ENFILE/ENOMEM retain resource_limit. Other fault
rows and real SDK-clone controls remain as defined in process-test revision4;
an unreachable Python os.pipe/Popen/time patch must never yield vacuous green.

Real worker prologue/ABI/root/no-site enforcement, exact trusted-resource integrity,
secure open/read-race handling, bounded allocation and every SDK no-fetch path
remain integration and source-review obligations. Parent refusal mocks cannot prove
them. Keep these gates explicit through implementation; never label all controls
verified from a public outcome code or use a fabricated selected-library success.

A DIFFERENT reviewer must approve the ACTUAL document and metadata fields before
these additive tests depend on them. Actual test/oracle/harness review and genuine
missing-wrapper red still precede implementation. These are private developer-tool
admission refinements, not new public signatures/options or reduced release scope.

Revision2 reason: actual selected Python resource/PID result-allocation and cleanup
source intervals prevent complete parent ownership proof. This explicit native
admission/configuration-owner split and negative-seam port address that private
architecture change without changing the one SDK root, exact interpreter spelling,
three pre-sysconfig presence guards, all three ABI profiles, child recomputation,
no-site behavior, phase6, paired20 metadata checks, hashes or selected versions.
Original revision1 source86d6d482 and its valid tests/reviews/red artifacts remain
historical evidence. Actual different revision review is a prerequisite, not a
claim that a native installation or foundation already works.
