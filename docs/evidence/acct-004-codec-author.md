# ACCT-004 command codec implementation

Author: codec_author, 2026-11-04, task/ACCT-004-codec from main1124532 and
coordinator claim603a62a. Production work is authorized by the new continuation
instruction and the completed [ACCTTEST-004 baseline](accttest-004-handoff.md).
The historical first red was preserved and was not replayed.

## Bounded change

New server/src/application_service_command.cpp implements the frozen public SAC1
encoder/decoder. Authority is [concrete API](../contracts/application-service-api-v1.md)
sections A/B/F, blobbbd6cd834b8a959ccff16d63a638e0b77d9c6e4c, logical6d0cd67f,
specification v0.45 sections3–5. No header, baseline test, fixture, schema or
existing repository behavior changes. Coordinator owns build registration and
service author removes the two old command stub definitions from the other TU.

The decoder first scans complete framing through borrowed byte views, with every
length/count checked before advancing and no string/vector materialization. It
then applies typed key/source, all count, body, conditional and output checks.
The encoder validates all nested options and typed input before counting exact
wire/output size and allocating. Public envelope-versus-command B/I is distinct;
encode output has no choice-vector items. Copying decoded QByteArrays explicitly
owns raw bytes, including empty, BOM/NUL/FF, independently of the incoming buffer.
Existing internal bundle/hold option validators are reused and service scalar
representability is additionally checked. No unknown format fallback is accepted.

## Actual checks

Native GNU16.2.1 / Qt6.12 / Boost1.92 host. This command completed exit0 with no
warnings, using the repository header and existing private option validators:

```sh
c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Icore/include -Iserver/include $(pkg-config --cflags Qt6Core Qt6Sql) -c server/src/application_service_command.cpp -o /tmp/acct004-codec.o
```

For immediate bounded validation, linked that object plus a scratch copy of the
unchanged view/service stubs (only the two replaced command definitions removed)
against the coordinator's already compiled unchanged Qt test object and existing
component libraries. No tracked build file was changed and no other stub was
invoked. The resulting scratch binary executed exactly the five reviewed codec
slots:

```sh
/tmp/acct004-codec-tests commandEncodingMatchesFrozenFrames commandDecodingMatchesFrozenMembers codecBoundsUseActualOwnedDTOs codecMalformedAndPurePriority codecOwnsBinaryBuffers
```

Observed exit0: 99PASS including init/cleanup, 0failed/skipped/blacklisted.
This is 97codec scenarios (38encode,38decode,8bounds,12malformed,1ownership),
not full service acceptance. The final source was recompiled exit0 with the same strict warning flags,
relinked and the identical five-slot command passed99/0/0 again. Full-width C++
enums are checked before wire narrowing, retaining key-before-source priority.
Integration should also run the slots through the actual CMake target.

## Remaining gates

Different source review, coordinator's actual build integration and full35/full300
acceptance, ordinary selectedQt CI promotion and current-head merge checks remain.
No independent approval, running server, real authority/policy/action/provider,
platform or full release proof is claimed. Admission must use input-only command
preflight, not inherit public encode's envelope output budget; internal retained
decode must use storage/intermediate limits instead of public caller-input limits.
