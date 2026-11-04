# SESSIONTEST-001 selected CI preparation

Coordinator /root, 2026-11-04. This is authored local workflow evidence only;
publication, different actual workflow review and selected Ubuntu execution are
UNPERFORMED. Existing PR59 checks do not cover this new option or native library.

The password-hashing-build job uses ubuntu-24.04 and the repository's existing
exact checkout commit. Permissions are contents:read; no credentials persist.
It downloads libsodium1.0.22 archive2008529bytes with exact SHA256
adbdd8f16149e81ac6078a03aca6fc03b592b89ef7b5ed83841c086191be3349 before extraction.
This is the same artifact independently authenticated by the isolated official
GPG path in [source evidence](sessiontest-001-dependency-preparation.md); the
workflow's byte pin relies on that recorded authentication, not a moving tag or
an unauthenticated new digest. No alternative host sodium fallback is allowed.

Out-of-tree portable GCC/-O2 upstream configure, static build/install use only
RUNNER_TEMP/prefix, no global install or vendor makecheck. Configure does execute
native compiler probes; child core limit0 is explicit, not a promise about external
system crash handlers. The explicit importer checks Linuxx86-64/header1.0.22/ABI26/4;
actual foundation separately verifies runtime identity, real vectors and guarded
memory. Project configure uses the existing exact GoogleTest fetch opt-in when
required; no new dependency is introduced for workflow parsing.

CI builds the default target plus explicit compile-only generator target, verifies
mandatory enabled noninverted green foundation discovery and no generator CTest,
then runs that exact foundation with no-tests=error. Existing infrastructure runs
separately. Generator never executes in CI. New password feature tests, once
reviewed, need exact enabled inventory/intentional-red guard before publication;
the first feature red remains an explicit plain command with raw/JUnit evidence,
never a failed green job or suppression. This initial workflow has no feature run.

Actual local validation: PyYAML parse,8 steps/read-only permission/no failure
suppression, all7 run blocks passed bash -n without execution; project consistency
and whitespace PASS. No CI/platform/native password proof follows. Workflow scope
is still absent and the existing owner access request remains pending; do not
attempt a bypass or assume local authoring can satisfy the publication gate.
Actual full workflow review and current successful selected CI are required before
baseline merge or first feature red. Later tests may update the build/discovery
portion, requiring review of that actual revision.
