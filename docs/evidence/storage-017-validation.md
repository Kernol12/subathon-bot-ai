# STORAGE-017 independent native validation

Validator: bundle_validation, 2026-11-03, task/STORAGE-017-validation, isolated
local-evidence/worktrees/storage017-validation. Base is actual integrated main
`a40b42fc6d6e2a419f90f3abfd246c29dfd8173c` (normal PR45 merge). Root task claim455258c.
Production author is a different agent, bundle_implementer; separate code/config
review is another role. This note records actual independent native acceptance,
not prospective approval or a replacement for that code review/latest-head CI.

Imported ONLY the actual authored runtime CPP from
`d675eb9e16ce87a8848fd46700f11d78b4661f0b` (root34f7dd5), then copied the exact five
root-owned promotion/configuration files for validation. No test, fixture, schema,
public API or domain value was edited. The local verification copies remain
uncommitted deliberately; this branch commits ONLY this evidence note.

## Exact validated artifacts

| Artifact | Git blob |
| --- | --- |
| Runtime CPP |834336d34f532508cf35d9eabb58f8ed8f0e71bf |
| Header (one obsolete stub comment removed only) |8614bcef00ecbcbe2f95d24751ece82b6db537ce |
| Root CMake |a713c09c2a87164d342698774cc16ba11c9a2a2f |
| Presets |0acd89ba7fc005737c208f202681b36c459a77e2 |
| Qt CMake |687a9d3e5ec12afebb2c29e05f2e2637c14fa8b6 |
| SQL workflow |c5d152f6fcbd78c414cdfefd542e29db4152a420 |
| Unchanged concrete contract |bad19c47ed397547a552e8f64e2141e87116f0d7 |
| Unchanged reviewed feature CPP |0e4f4f006d2a0e161857e38255cac487f7de83a9 |
| Unchanged independent JSON/helper |bea1273cc6858c3998066e26e11e7d870a02c1bc /67d4fbd28a4c92e24c7b9b10e063638a9ba90541 |

Original test reviewf5c6dc1/final attributiona729+eb563 and first SAME-clean-c24
18-case/35-marker red remain in the integrated baseline evidence. Original raw
SHA25690f3c739a735c13ca4eddecf0055ce7322cf1247e6dd52bc030e0956b1ee19b0 was
not replaced, replayed or relabeled. These runs are implementation acceptance.

## Actual commands and results

Linux host: CMake/CTest4.4.5, Ninja1.13.2, GNU16.2.1, systemQtCore/Sql/Test6.12.0,
Boost1.92 (compiler macro109200), actual QSQLITE3.54.0. No dependency download or
live/provider operation. Root's selected officialQt6.12/SQLite3.53.4 CI is separate.

1. `cmake --preset sqlite-original-claim-debug`: exit0.
2. `cmake --build --preset sqlite-original-claim-debug`: fresh100 work steps,
   exit0, ZERO compiler warnings/errors against actual CPP834 and original header.
3. Copied exact root promotion files above; configure/build with SAME commands:
   exit0,26 work steps, ZERO warnings/errors. Changes are option/display text,
   removed draft label/comment, one obsolete header comment, and mandatory CI18.
   No runtime CPP/test byte changed; final acceptance below uses these exact files.
4. `build/sqlite-original-claim-debug/tests/qt/server_sqlite_original_claim_acceptance -functions`:
   exit0, EXACT18 compiled cases. No `_data` helper counted as a case.
5. `ctest --preset sqlite-original-claim-debug --show-only=json-v1` and same with
   `-L '^sqlite-original-claim$'`: EXACT228 global/18 selected enabled registrations;
   selected names equal compiled slots, no DISABLED/WILL_FAIL, no draft label.
6. `ctest --preset sqlite-original-claim-debug -L '^sqlite-original-claim$'`:
   **18/18PASS,0.79s,exit0**, UTC08:46:21.468354–08:46:22.266945.
   Parsed all18 actual Qt summaries:71 passed,0failed,0skipped,0blacklisted.
7. `ctest --preset sqlite-original-claim-debug`: **228/228PASS,7.97s,exit0**,
   UTC08:46:22.267418–08:46:30.248022. Parsed all225 actual Qt summaries:
   715 passed,0failed,0skipped,0blacklisted. This includes existing component/
   infrastructure and required fixture suites plus all18 new-reader cases.
8. Programmatic source/metadata checks and `git diff --check`: exit0. Extracted
   old complete-record byte-walk and new factored record_bytes body have IDENTICAL
   tokens/order. Removing only the obsolete stub comment restores original header
   cf721 byte-exact. Frozen contract/tests/JSON/helper hashes match baseline above.

Actual18/full228 logs, UTC/exit/Qt-total metadata and independent LastTest copies
are preserved in ignored local-evidence/storage017-validation in this worktree.
The targeted LastTest was copied BEFORE full228 overwrote CTest's current log.

| Local raw artifact | SHA256 |
| --- | --- |
| Fresh build |509963013a72933972ef0aaf08a390540b7c2cda847df0c4a74f2c2823bb3fb3 |
| Promoted build |89faa2ebed084d3be5c2442723ce8fce88b82dd61fee5f8133b441dc5b4cad71 |
| New18 output |54524ceb74ee40cd18a52a91645d520e72f50b0648d68e795f10023cb1f7afb2 |
| New18 LastTest |e261364853bf22c096f1c25ab4d73810609b3b55c29e206a3c4991560a9f1119 |
| Full228 output |688ba7c8b4e634209ba2b356adba6cbd74bdf0c276f0be59a2cb29a9ea4e8a51 |
| Full228 LastTest |a6e75ad0364b20ecf21d253b717a95e3252ab730a69bb0d12f497f750645265d |
| Final global discovery |13db991486b5bdcadce5a1f6c6e7e0052fded596c5f64264197476ea24c0b8bb |

## Boundary assessment and limits

Actual source keeps pure options/explicit role/positive receipt/CID-presence checks
before ticket access. Selection uses global tagged subject first, ORIGINAL-kind
receipt fallback only on absence, strict actual selected identity and the existing
full lookup closure. Corrupt selected subject cannot fall through to a different
source; absence does not admit candidates/audit unrelated current state. The
existing borrowed session owns all SQL/budgets; no own connection/BEGIN/commit/
destructor cleanup/permission or effect evaluation was added.

The reused full closure and one-byte wrapper budget enforce owned complete results;
no selector input quota substitutes for result_items. The exact inherited byte-walk
is preserved. Local selected failures use actual whole caller rollback; writer's
already-aborted budget failures and real capability/protocol/unknown outcomes stay
truthful. Accepted tests exercise independently derived global priority/roles/MAX/
CID0/signed-original/absence/history/complete corruption/provisional read/owned
bytes/quotas/foreign-expired-terminal/budget/acknowledgement-loss outcomes, with no
skips or weakened expectations. No blocking native-validation finding remains at
these exact artifacts.

Promotion keeps all prior suites, exact18 registration/compiled guard, dependency/
action pins, permissions and check name; new18 now executes unconditionally. There
is no manual switch, skipped case, failure inversion or blanket continue-on-error.
Actual GitHub selected-environment run and latest revision checks have NOT been
observed by this validator and remain coordinator/code-review gates before merge.

This is real synthetic/disposable SQLite component acceptance. Durable holds016,
complete service004/current access/policy/clock/action meanings, all providers/FX,
recovery/delivery/UI/renderers/platform/package/security/full release remain separate
mandatory gates. Matching a historical record remains provisional data until THIS
caller's actual COMMIT and normal completion; no authorization or provider proof is
inferred. Root owns task progress/HTML/checkpoint/PR integration; no source/config
verification copy from this reviewer branch should overwrite another author's work.
