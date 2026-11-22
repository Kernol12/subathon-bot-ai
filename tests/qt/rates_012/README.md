# RATES-012 Stage 1 fixture handoff

[#775's frozen contract r2](https://github.com/Kernol12/subathon-bot-ai/issues/775#issuecomment-7068200880) supplies the expectations under 129B and 135A. Its SHA-256 is `0d8d91b3b55b14c3f23f6bf404ab6f53cca3c180b438e9b70b690393f1f52fa3`. The Stage 1 base is `7399a266da7d95c040ced8a20bcdf580352c0556`, tree `7453ce76ddcead79858c8d0219f2b1d73304a02c`.

The fixture declares the new private operations in `boundary.hpp`. `missing.cpp` supplies explicit missing behavior for the baseline. The work marker is the integer 1. Mutations and load results travel through distinct private state and the writer's actual completion. Catalog bootstrap uses the shipped v15 and v16 manifests in disposable storage, then reopens under the default writer limits. The runtime key fixture requires a non-root service UID and a protected home, as in #270.

F8 uses the shipped #400 handoff and sanitizer with fixed synthetic DTOs and exercises guarded attempt completion separately. The legacy sanitizer rejects a duplicate echo as an invalid document before projection. The guarded boundary receives a modeled invalid/multiple input and must return an unknown fact for that exact attempt. F10 models #272 codec compatibility using the shipped #270 open and reseal primitives. F15 checks a guarded early refusal and the shipped #398 accounting with a synthetic cap and port.

## Selectors and expected checkpoints

These are independent expectations and **predicted** first missing checkpoints. For scenarios requiring a seed, the first mutation admission is itself a #775 behavior assertion. Each successful seed must then stage and acknowledge through the writer before subsequent witnesses are meaningful.

| Qt Test selector | Independent witness | First missing checkpoint |
| --- | --- | --- |
| `F1_exactRecordReopenAndConnector` | Literal 11-byte BLOB, 95 = 48 + 28 + 3 + 16, exact revision then nonce, independently opened V and unchanged shipped 71-byte connector and v16 catalog | Admit the FX insert after connector and catalog controls |
| `F2_exactScopeIsolation` | Independently framed J, B, U and 09 scopes are missing until the copied record fails AEAD under their AAD. Opaque connector FXR1 stays a connector value | Admit the default FX insert |
| `F3_authorityPrecedesRowAuthentication` | Denied, denied, stale, then authentication failure for the same corrupted row. No private slot or handoff | Admit the FX seed before the corruption stimulus |
| `F4_revisionAndNoDrawOnIdenticalPut` | Zero draws and identical ciphertext for V again. W draws 22 then A2. Repeated 11 refuses after one draw. Fresh r2 delivers W | Admit the FX seed |
| `F5_revocationRebindingAndLateSettlement` | Revoked queued permit gives zero accepts. Accepted obsolete attempt settles accounting only. Mismatched fields and reused permits give zero accepts | Admit the FX seed |
| `F6_exactEraseAndAuthorizedAbsence` | Invalid empty and stale wrong expectations. Exact erasure and authorized management absence. Recreate at 22/A2, leave connector bytes intact and deliver V under fresh r2 | Admit the FX seed after the connector control |
| `F7_lostCommitAndFailedPreparation` | Actual COMMIT with lost acknowledgement holds use. Healthy private readback confirms r2 and fresh handoff observes W. Known commit with work failure suppresses permit | Admit the FX seed |
| `F8_boundEchoAndShippedSanitizer` | 124 = 48 + 28 + 32 + 16. Fixed lowercase, uppercase, absent and invalid JSON give match, mismatch or unknown. Foreign and duplicate finishes produce no private fact. Sanitizer drops the echoed bytes and unretained header | Admit the tag 09 seed after real sanitizer controls |
| `F9_boundsAndDefaultWriterBudget` | 0 invalid, 1 gives 93 bytes, 65,508 gives 65,600, 65,509 has size precedence. 4,097-byte ref refused, 12,296-byte scope accepted. 31/invalid 32 App IDs invalid, 33 exceeds limit | Return invalid input for empty put |
| `F10_rotationCodecCompatibility` | Old and new records of both families verify exact payload equality only at exact scopes and key material. Corrupted tag/ciphertext, wrong key material and tag 0A fail qualification | Admit the FX seed after connector control |
| `F11_restoreRequiresFreshRecoveryAuthority` | Authentic older r1 is not dispatch authority. Fresh selection expecting r2 is stale. Explicit Recovery-approved r1 hands off V | Admit the initial snapshot seed |
| `F12_keylessMalformedAuthenticatedPayloadAndRedaction` | Open route uses no SDK credential work. Missing key and authenticated wrong magic/empty payload refuse. SQLite index error rolls back and messages contain no canaries | Admit the synthetic canary seed |
| `F13_mutationFenceSpansCommitDeliveryAndRollback` | Held fence blocks fresh preparation and prior permits during delayed completion. Both outcomes invalidate prior permits. Known rollback permits fresh r1/V and committed change needs fresh r2/W | Admit the FX seed |
| `F14_uncertainInsertAbsenceCandidateOrDisagreement` | Real rollback/commit with missing acknowledgement yield unknown. Empty-expectation absence resolves without mutation success. Candidate reconciles, unrelated rX stays held | Admit the empty-expectation insert |
| `F15_transportCapAndShippedNotSentAccounting` | 157 = 48 + 28 + 65 + 16. Encoded bytes alone exceed target cap 64. Zero GET, zero quota, local retry, exact stored revision | Admit the 65-byte seed |

## Compile and passive inspection

From the repository root on Linux x86-64, supply the already prepared libsodium 1.0.22 static SDK prefix and Qt 6.12 or later. The opt-in CMake target registers one intentional-red Qt suite outside the default presets and CI.

```sh
rates_sdk=/path/to/libsodium-apitest001/sdk
rates_build=/tmp/rates-012-compile
cmake -S . -B "$rates_build" -G Ninja -DSUBATHON_BUILD_RATES_012_STAGE1=ON \
    -DSUBATHON_SODIUM_ROOT="$rates_sdk"
cmake --build "$rates_build" --target rates_012_stage1 --parallel 3
rates_runner="$rates_build/tests/qt/rates_012/rates_012_stage1"
readelf -h -l -d -V -W "$rates_runner"
readelf --dyn-syms --wide "$rates_runner"
readelf --syms --wide "$rates_runner"
readelf -h -d -V -W /usr/lib/qt6/plugins/sqldrivers/libqsqlite.so
sha256sum "$rates_runner" "$rates_runner.map" \
    "$rates_sdk/lib/libsodium.a" "$rates_sdk/include/sodium/version.h" \
    /usr/lib/qt6/plugins/sqldrivers/libqsqlite.so
```

Use the link map and ELF dynamic and version sections to resolve both the binary and QSQLITE closure against the selected Qt tree. Hash the resolved files including the interpreter. Verify the static sodium SDK path and wrapper symbols in the map and symbol table. This is compile and passive inspection only. Candidate execution, discovery, loader and plugin reconciliation, and intended-red evidence require the coordinator's admission and review workflow.
