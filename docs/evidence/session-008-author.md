# SESSION-008 — supplier implementation author evidence

Author `/root/session_supplier_implementer`, 7 November 2026. Isolated worktree
`/tmp/subathon-session008-author`, branch `task/SESSION-008-implementation`, from
coordinator claim `e606fe369739bb74c4f107569933cfa2f724b34a` on normally integrated
main `37f098d99b306830e43398e55071286a689a95b2` (PR82).
[Current handoff](session-008-handoff.md), [task](../tasks/EPIC-007/SESSION-008.md),
[accepted supplier](../contracts/session-resolution-v1.md), unchanged contract blob
`c5f5a6e71664042e18475068d3caf577b0c8d009`; specification v0.45 §10,
AUTH-05/12 and TEST-04/05.

## Scope and preserved gates

Own only `server/src/session_resolution.cpp` and this note. Implement the two
accepted prepare/resolve entrypoints. The public header, all39 baseline methods,
literal fixture, native foundation, build harness, labels and original receipts
are unchanged. Their frozen blobs match the coordinator handoff. The prior
independent test review2014c0c and first-red attribution6bef738 remain the baseline;
the historical red was not replayed. No new interface, dependency, test expectation,
configuration, shared progress file or architecture was introduced.

Final implementation source blob: `0525d75537d890a1dc00bb940b5cefd709c8d6c0`.
This is author evidence, not independent code approval or task completion.

## Code and resource decisions

Preparation rejects a wrong view length without accessing its characters. Its
single lexical pass reads at most64 input characters, accepting only ASCII0–9/a–f.
To satisfy both whole-input validation before decoding and the one-read ceiling,
the existing raw32 buffer temporarily holds the first32 text bytes and frame60
holds the last32. Only after full lexical admission does forward in-place decoding
produce the first16 raw bytes, then decoding from frame produces the other16.
The frame is overwritten with the static28-byte prefix including its one NUL,
followed by raw32. No additional secret-text array is needed.

Every constructed scratch object clears both entire arrays through sodium_memzero
on success and every early return, including partial lexical admission and native
failure. No scratch is created for wrong lengths. Initialization is checked per
canonical preparation call, accepting only0/1; no fallback or fake successful
crypto exists. The actual selected single-part crypto_generichash receives60 input
bytes,32 output bytes and no key. A nonzero return produces unavailable.

The one32-byte digest output lives directly in the result variant. A nested scope
ends the raw32/frame60 lifetimes before returning that variant, so a non-elided
array handoff cannot overlap another digest with the92-byte scratch. The peak
active application byte buffers are124; the post-scratch handoff can have two32-byte
digest objects, below that peak. The domain prefix is static read-only nonsecret
data. Fixed scalars/discriminants/object overhead, native hashing scratch and the
external runtime are separate from this byte-buffer count. The existing baseline
review bounds the selected native fixed-profile hash separately; this note does
not turn124 into a total process/stack/init/platform bound.

Resolution admits positive limits and the representable three-value peak before
any producer. Individual reference limits fit string max_size and terminator
arithmetic; aggregate owned_bytes remains a separate per-value ceiling rather than
being treated as one string or one shared call budget. Checked addition uses
remaining budget before adding each separately counted occurrence. Request, all12
fact occurrences and all5 result occurrences are preflighted before result growth.
The source remains responsible for preflighting its stored data before allocating
its owned snapshot; this consumer cannot retroactively establish that obligation.

The source returns one owned variant directly into the local snapshot. Validation
borrows at most12 simultaneous string views; iteration/lambda parameters borrow by
reference, and request/fact/result view arrays have disjoint scopes. The output is
constructed inside its variant, then its five strings are copied once from the
admitted snapshot. Returning the variant uses elision or owned string moves, with
no additional full owning copy. Request/facts/result payloads may coexist under
their separate owned_bytes ceilings, preserving the3× aggregate bound.

The resolver calls the source once, handles its explicit variants, then checks all
fact sizes before shape, bindings, valid state and time order. Digest binding uses
the selected fixed32-byte sodium_memcmp. Revoked/disabled/epoch failures precede
the sole clock sample. Clock refusal or timestamps before authentication/start
produce unavailable; expiry is exclusive. After ordered signed comparisons,
unsigned64 subtraction gives the exact nonnegative age over the entire signed64
domain, without overflow or domain narrowing. Freshness includes exactly300000ms.
All unclassified source/clock/allocation exceptions map to unavailable without
diagnostic disclosure. Facts are reloaded for each execution; no authority cache
or clock-trust inference was added.

## Actual checks

Linux7.2.14-arch1-1 x86-64; GNU C++16.2.1 (20260810), CMake4.4.5,
clang-format23.1.4, Python3.14.9. Existing selected libsodium1.0.22 static prefix
`/tmp/subathon-session-sdk/prefix`, archive SHA256
`f8e295ad9a6d8c6d3c657e4d2f33d590da161f6ed5a81dfae09d1067fab3d017`.
All commands below ran from this author worktree and returned0.

```sh
cmake --preset session-resolution-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-session-sdk/prefix
cmake --build --preset session-resolution-debug
ctest --preset session-resolution-debug --show-only=json-v1 > /tmp/subathon-session008-author-discovery.json
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wsign-conversion -Werror -Iserver/include -I/tmp/subathon-session-sdk/prefix/include -fsyntax-only server/src/session_resolution.cpp
clang-format --dry-run --Werror server/src/session_resolution.cpp
ctest --preset session-resolution-debug -L '^session-resolution$' --no-tests=error --output-junit /tmp/subathon-session008-author-feature.xml
ctest --preset session-resolution-debug --no-tests=error --output-junit /tmp/subathon-session008-author-full.xml
cmake --build --preset session-resolution-debug
ctest --preset session-resolution-debug --no-tests=error --output-junit /tmp/subathon-session008-author-final-full.xml > /tmp/subathon-session008-author-final-full.stdout
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Initial build12 steps, final rebuild3 steps, no project warnings. Passive JSON
contains exactly43 registrations including39 features. Initial selected39 PASS
in0.05s and full43 PASS in0.72s, no failures/disabled/skips. After the source audit
changed the validation iteration to borrow its view and made the constant prefix
explicitly static, the final rebuilt source passed all43 again in0.68s, including
all39 feature cases and the native foundation. The full preset also executes the
unchanged Python tooling and project-consistency checks. Strict syntax and format
checks pass; progress HTML remains current. JUnit/discovery files named above are
local run artifacts, not new tracked evidence or replacements for baseline receipts.
The final standalone project check passes443 task records,122 decisions, local
links and presets.

## Handoff and remaining gates

No genuine test defect or missing requirement was reached; no baseline assertion
was changed and no synthetic regression was added merely to mirror the code.
Different-agent source/resource/secret-lifetime review is required at this exact
revision. The coordinator owns stable label/ordinary CI promotion, actual selected
hosted results, shared evidence/state, final acceptance and PR publication/merge.
Existing intentional-red labels are still unchanged even though these local bodies
now pass; local green alone does not claim that CI promotion has happened.

This component supplies only synchronous preparation and resolution over injected
current facts. Real Qt/SQL snapshot production and preallocation, trustworthy
clock/recovery ownership, accepted login, permissions, issuance/RNG, lifecycle,
transport, protected client storage, platform/security and release verification
remain their owning tasks. It provides no full SESSION-002/003 or provider/live
capability. All seven integrations and three exchange-rate providers remain in
release scope. No live supporter data or credentials were used.
