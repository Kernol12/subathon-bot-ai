# SESSIONTEST-008 coherent baseline candidate

Author: fresh `/root/session_supplier_baseline_author`, 7 November 2026.
Dedicated branch `task/SESSIONTEST-008-author`, isolated worktree
`/tmp/subathon-sessiontest008-author`; base
`e8b7e4c741f8b0dcef5e4d5092a0c927f65738f1`, coordinator claim from integrated
main `b8f3b141801087ab425ce74afee352bbe8a3678a`.
[Current task handoff](sessiontest-008-handoff.md) owns integration/status.
This is one authored packet awaiting actual different review, not reviewed tests,
observed feature red, native readiness or implemented behavior.

## Scope and frozen authority

The [accepted supplier v1](../contracts/session-resolution-v1.md), Git blob
`c5f5a6e71664042e18475068d3caf577b0c8d009`, remains unchanged. Specification
v0.45 §10, AUTH-05/12 and TEST-04/05 define this bounded prepare/resolve baseline.
The public header reproduces its aliases, enum widths/order, fields, variant order,
ports and two signatures. Both production bodies immediately throw
`std::logic_error("Account session resolution is intentionally unimplemented; review tests first")`.
There is no parsing, validation, hash call, readiness, authority or constructor
behavior in those stubs and no additional public exception/crypto interface.

Owned artifacts are the new server header/source, focused feature and SDK-only
test translation units, literal fixture and this evidence. The coordinator
explicitly delegated only the option/target/preset/closed-discovery harness edits
in root/server/tests CMake and new `tests/cmake/SessionResolutionLabels.cmake`.
Existing targets, assertions, SDK pins, workflows, progress, generated HTML,
schemas and prior receipts are unchanged. Root owns their later integration.

## Independent oracle and coverage

The fixture transcribes all three accepted hashlib-derived digest literals and
the exact 28-byte prefix, including its one NUL. Public synthetic bytes are not
production tokens. Expected values never come from project preparation/resolution
or a successful fake crypto provider. The SDK-only foundation independently
contains the same three published literals and links only `Sodium::Sodium`.
It includes no project header, fixture, GTest or private wrapper.

| Accepted supplier requirement | Candidate coverage |
| --- | --- |
| §1/3 exact 64 lowercase hexadecimal, whole input, length before reading, lexical before crypto | Three digest vectors; every position against twelve disallowed byte classes; prefix/whitespace/separator/NUL/non-ASCII cases; empty/short/oversized/suffix priority; inaccessible mapped bytes at lengths63/65 |
| §1/2 exact unkeyed single-part BLAKE2b-256 frame and selected native failure | Exact60-byte native observation against literal prefix/raw; real hash success with literal output; unavailable after real native hash success; fresh-process initialization failure after real initialization success |
| §2 raw/framed scratch erasure | Selected real sodium_memzero, matching live public raw32/frame60 content before erasure and all-zero bytes immediately afterward, both success and post-native-error paths |
| §2/3 limits and ownership | Zero/overflow/string-max/peak-invalid limits before producers; inclusive peak quotient; request reference/aggregate refusal; all12 fact reference bounds; repeated equal occurrences counted individually; facts11 refusal/facts12 acceptance; request1+facts12+result5 accepted with separate per-value12/peak36; owned result survives all producer state |
| §3 steps2–4 current source and complete facts | One source and ordered clock; all four failure variants; unclassified exceptions including bad_alloc become unavailable; every empty ref, all installation/account bindings, all32 digest mismatch positions, invalid state/time order; size before shape before revocation; disabled/revoked/either epoch before clock |
| §3 steps5–6 time and re-resolution | Clock unavailable/diagnostic/bad_alloc/unknown exception; rollback refusal; authentication after lifetime start; age0/299999/300000/300001; expiry minus1/exact/plus1; full signed extremes/unsigned age18446744073709551614; fresh boundaries near both extremes; each execution reloads current facts and samples current clock when eligible |
| §2/3 byte-exact opaque references and internal failure data | NUL/non-ASCII references accepted and owned; epoch case mismatch revoked; failure alternatives contain only the enum, and source diagnostics never escape as an exception/success |

There are39 ordinary TEST definitions and39 exact enabled feature registrations.
The closed inventory fails discovery on any missing/extra name and assigns
`session-resolution;acceptance;intentional-red;server`, timeout10, without
WILL_FAIL, DISABLED, skips, inversion or blanket failure suppression. Foundation
is separately named `scaffold.session_resolution_environment`, labels
`scaffold;session-resolution-environment`, timeout10. Total discovery43 consists
of39 features, the new foundation and the unchanged three infrastructure tests.

## Private native observer constraints and interpretation

The feature TU supplies exactly four GNU link wrappers: sodium_init,
crypto_generichash, sodium_memzero and sodium_memcmp. Fixed trivial constinit
thread-local observations are inactive outside a test-local scope. Native wrappers
delegate real SDK behavior; initialization/hash errors are injected only AFTER
actual successful corresponding native calls. No success value, digest, readiness
or zeroization is fabricated. Unsafe observed hash lengths/pointers and comparison
widths are refused as errors rather than passed to an unsafe native oracle.
The observation itself still fails the relevant profile/width assertions.

The initialization-error case is an exact individual CTest filter in a fresh
process, before any project prepare operation or cached readiness. Successful
preparation tests impose no per-call initialization count or caching policy.
Directly running that one case in a process already used for preparation does not
meet its stated precondition. Default CTest's individual process commands do.

Erasure checks borrow only live buffers during the real clearing call and never
read dead stack storage. They recognize raw32/frame60 content, not chosen stack
addresses or a particular RAII class. The accepted fixed buffers justify those
sizes. They do not prove register/swap/dump/framework-copy erasure or a process-wide
memory bound. No global allocator override or project readiness hook executes
during passive registration/discovery.

The source/clock fakes are explicit synchronous simulations. Their exception
mapping follows §3's unclassified failure→unavailable rule, including thrown
bad_alloc; an explicit source resource-refusal VARIANT remains resource_limit.
No new allocation-error classification is introduced. A result-only string-budget
refusal cannot be reached after valid complete facts: all five result occurrences
are a subset of the already admitted twelve fact occurrences under the same
per-value budget. Tests therefore assert the exact separate budgets and owned
projection instead of fabricating an unreachable result-only limit fixture.
Actual source preflight before materializing stored facts, serialized Qt scope,
native fixed workspace and production allocation/copy handling require their
separate source/adapter reviews; simulation does not establish those capabilities.
Root is obtaining the selected-native source/static-object scratch audit separately.

## Actual allowed author checks

Host Linux x86-64, GCC16.2.1, CMake4.4.5, Ninja1.13.2, Python3.14.9,
clang-format23.1.4 and installed GoogleTest. The coordinator separately prepared
the existing selected libsodium1.0.22 static prefix
`/tmp/subathon-session-sdk/prefix`; archive/source admission and SDK build receipt
are root-owned in `/tmp/subathon-session-sdk/sdk-preparation.json`.
Static archive SHA256 is
`f8e295ad9a6d8c6d3c657e4d2f33d590da161f6ed5a81dfae09d1067fab3d017`.
Runtime identity of this newly prepared archive has NOT yet been observed.

Actual commands in the dedicated author worktree:

```sh
clang-format -i server/include/subathon/server/session_resolution.hpp server/src/session_resolution.cpp tests/acceptance/session_resolution_tests.cpp tests/acceptance/session_resolution_environment.cpp tests/fixtures/session_resolution_vectors.hpp
cmake --preset session-resolution-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-session-sdk/prefix
cmake --build --preset session-resolution-debug
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -Iserver/include -I/tmp/subathon-session-sdk/prefix/include -fsyntax-only tests/acceptance/session_resolution_tests.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -I/tmp/subathon-session-sdk/prefix/include -fsyntax-only tests/acceptance/session_resolution_environment.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -Iserver/include -fsyntax-only server/src/session_resolution.cpp
build/session-resolution-debug/tests/server_session_resolution_acceptance --gtest_list_tests > /tmp/subathon-sessiontest008-author-audit/gtest-list.txt
ctest --preset session-resolution-debug --show-only=json-v1 > /tmp/subathon-sessiontest008-author-audit/discovery.json
clang-format --dry-run --Werror server/include/subathon/server/session_resolution.hpp server/src/session_resolution.cpp tests/acceptance/session_resolution_tests.cpp tests/acceptance/session_resolution_environment.cpp tests/fixtures/session_resolution_vectors.hpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Configure/build PASS (initial12 steps and final two-step test rebuild), no project
warnings. All three strict syntax checks PASS. Passive listing/discovery PASS.
An inline standard-library Python inventory audit checked exact source/list/CTest
name sets, all labels/filters/timeouts/enabled states, the43 total entries and
unchanged accepted contract blob. Native nm inspection found no project/wrapper
or C++ global constructor symbol in the SDK-only executable. Feature source/object
inspection found only inert GTest registration before listing, with no SUT, SDK,
fake-source/clock or private control constructor called there. Project check PASS
(443 task records/122 decisions/current links/presets/HTML); current progress and
whitespace/format checks PASS.

Passive artifacts under `/tmp/subathon-sessiontest008-author-audit`:

| Artifact | SHA256 |
| --- | --- |
| gtest-list.txt | `9f681a3d84f0d0484dcf33ccfb191e1bc770b02c3b73769dd6dc9b890e7356cd` |
| discovery.json | `d32cf74ebaef994a71b0d7e5c7f3205d2d53815bb85d8991f9bd39b37c69351f` |
| feature binary | `85fd82584fabfa15e558120ae70ceeef92ace8845908edfccd7d027739f78055` |
| SDK-only binary | `f582eac4b72b2fa4d410ccea01dd329387a282711900d18169e708c842440924` |

One exploratory ls of the historical SESSIONTEST-001 temporary SDK path failed
because that old directory is absent; no download/install or behavioral run
followed it. Object inspection with an over-broad sed range emitted excess static
registration disassembly; this is inspection output, not test execution or red.
The complete compile used only root's explicit new prefix. No feature test body,
stub marker, SDK foundation, selected native initialization/hash/comparison/erasure,
new fake/control constructor or provider operation has executed in this author
stage. Only native test LIST mode/passive CTest discovery executed.

## Next gate and CI assessment

Review this actual complete packet with a different agent/session before any new
test/function/native-foundation body runs. After actual approval, root owns the
separate selected-SDK green foundation and one ordinary observed marker-red run,
retained raw receipts/attribution and current selected CI integration. Existing
scaffold CI does not verify this newly executable native/component lane; root owns
the workflow addition and selected-host execution before baseline integration.
This packet neither finishes SESSIONTEST-008 nor authorizes SESSION-008 production.

Issuance/random generation, SQL/current-source implementation, lifecycle mutations,
real trusted clock/recovery, login/permissions/transport, queue admission, supported
platform security and all seven integrations/three rate-provider/full release
requirements remain open under their owning tasks.
