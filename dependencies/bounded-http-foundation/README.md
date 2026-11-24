<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Bounded HTTP successor (API-016)

This additive artifact implements the bounded HTTP foundation over byte-identical
API-012 inputs. All 34 frozen foundation methods and 44 unchanged native methods
pass selected Linux CI against one successor behind technical consumer gates;
native44 also passes against the original candidate4 SDK. The source, allocation
inventory, R4 interface, recipe/gate and CI review record is in
[PR #664](https://github.com/Kernol12/subathon-bot-ai/pull/664).

Revision R4 comprises corrections 1–19; 1–18 were reviewed in PR #664 and 19 (API-024,
interface only) in #685. Its numbered
**Contract corrections** and **Final R4 interface** sections identify their scope.
Corrections 1–18 changed no first-party public header or reviewed oracle. These results establish
no release, platform or security verification; the ABI marker alone admits no binary.

The [source allocation inventory](RESOURCE_ACCOUNTING.md) records the physical
storage, metadata guards, selected standard-library assumptions and exclusions.
Its hash and installed copy are bound by the manifest and complete SDK seal.

`manifest.json` binds the parent manifest, exact upstreams, all three ordered
patches, original and derived native source and the unchanged sixteen build options.
The recipe verifies these before building. Original and derived first-party native
sources are never compiled together; the consumer compiles only the sealed copy.

Use `tools/native_websocket_ci.py prepare --profile bounded-http-foundation --root
<fresh-root>` to clone, patch, build Release/shared/C++20/OpenSSL, copy provenance
and notices and seal every installed SDK file. Set RUNNER_TEMP to the session build
parent. `--source-seed <candidate4-source>` clones pinned Git objects without
hardlinks; it neither copies uncommitted source bytes nor writes to that seed.
`--local-development` records actual Arch packages and installed custom notices
instead of asserting the pinned Ubuntu package profile. It supplies development
results only and is not redistribution clearance. Hosted CI retains its exact
Ubuntu package and notice checks.

Configure the consumer with `SUBATHON_DROGON_ROOT=<root>/sdk`, then run `gate` with
that same profile/root, `--build-dir <consumer-build>` and `--consumer native` or
`foundation`. Local development also needs `--local-development`. The gate reads
ELF/ABI closure before first load, runs controlled identity with a scrubbed
environment, checks actual loaded maps and verifies unchanged seals. The foundation
receipt's frozen `review_receipt` field points to the coordinator's development
execution ruling; it does not claim any binary was admitted by review. Subsequent
foundation children consume the exact closure/source hashes and check reached maps.

Only a passing technical gate permits local consumer execution. Such results are
development evidence. Acceptance remains gated CI on the reviewed head, plus the
full tier-A review of source, artifact, recipe, resource/ABI accounting and gate.
See the [contract](../../docs/contracts/bounded-http-foundation-v1.md) and
[coordinator ruling](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6908247970).

Drogon remains MIT, Trantor BSD, and the correction patches/first-party code
GPL-3.0-or-later. The recipe retains upstream LICENSE, Trantor License,
orm_lib/COPYING, project COPYING/LICENSE.md/THIRD_PARTY_NOTICES.md, parent correction
provenance and actual selected dependency notices under the installed SDK prefix.

Patch 0003 selects C++20 for all Trantor compilation in this successor, including
unselected listeners. CI receipts bind the actual checked-out merge-ref commit;
PR descriptions also record the reviewed branch head. The private
BoundedIoInternal.h is source-only and is never installed for first-party use.
