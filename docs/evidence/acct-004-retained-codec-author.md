# ACCT-004 retained command decoder correction

Author: codec_author, 2026-11-04, task/ACCT-004-retained-codec from coordinator
candidateba2e90f. This is implementation of the bounded retained-command finding,
not an approval of my earlier public command codec or of this correction.

Authority: concrete application-service-api-v1.md blobbbd6cd83, sectionsB/F;
[bounded other-author service review](acct-004-service-codec-review.md), finding4.
The coordinator released this correction only after the separately reviewed
retainedCommandBoundIsNotMalformedData regression (CPP8ab5dba0) reached actual
behavior red on unchanged service722c25be: real SQL setup passed, zero skips,
expected result_limit disagreed with actual malformed classification. The root's
original per-invocation red evidence remains preserved; it was not replayed here.

## Owned changes and seam

Only server/src/application_service_command.cpp, NEWprivate
server/src/application_service_command_internal.hpp and this note are authored.
Public headers/contracts/tests/fixtures/schema/build files remain unchanged.
The other service author confirmed this private seam before implementation:

```cpp
namespace subathon::server::service_detail {
OriginalApplicationCommand decode_retained_command(
    const ApplicationEnvelope&, const ApplicationServiceLimits&);
}
```

Its caller replaces the service's public decoder invocation and preserves existing
typed failure mapping; that separate service-source change is coordinator-owned
integration of the other author's work.

The internal decoder reuses the borrowed framing scan, typed validation and deep
materializer. It checks retained envelope bounds against selected storage value/
document limits and intermediate bytes. It checks decoded command ownership and
actual choice items against intermediate limits before allocating any command
string/vector/QByteArray. Retained configured field/count overruns use result_limit;
malformed empty keys, role/presence/enums, option tags, framing and conditional
groups retain their structural categories. Healthy unsupported format keeps its
existing exception after the bounded envelope check. No widening followed by a
public allocating decode occurs; public input/result ceilings do not control
retained DTO allocation. Public codec behavior and its validation order remain.

## Actual checks

GNU16.2.1 / Qt6.12 / Boost1.92 host. Strict compilation completed exit0 without
warnings:

```sh
c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Icore/include -Iserver/include $(pkg-config --cflags Qt6Core Qt6Sql) -c server/src/application_service_command.cpp -o /tmp/acct004-retained-codec.o
```

For bounded immediate validation, a scratch copy of the actual old service TU had
ONLY its retained-command call replaced with the agreed private seam/header. It
compiled exit0 with the same strict flags, then linked with the new codec object,
unchanged coordinator-compiled test object and existing component libraries.
No tracked source/test/build file outside owned paths was changed. Executed:

```sh
/tmp/acct004-retained-codec-tests commandEncodingMatchesFrozenFrames commandDecodingMatchesFrozenMembers codecBoundsUseActualOwnedDTOs codecMalformedAndPurePriority codecOwnsBinaryBuffers retainedCommandBoundIsNotMalformedData
```

Observed exit0,100PASS including init/cleanup,0fail/skipped/blacklisted,662ms:
97 unchanged codec scenarios plus the single reviewed real-SQL retained-command
regression and two init/cleanup cases. This is bounded scratch-link verification,
not full service acceptance or a separate review verdict.

The complete integrated service correction still needs all applicable reviewed
regressions/full native checks, independent code review and selected CI. The
other internal service codec findings remain assigned to the other service author;
this note does not close them. Actual authority/provider/adapter/platform/release
gates remain unperformed.
