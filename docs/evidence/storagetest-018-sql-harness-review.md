# STORAGETEST-018 independent SQL baseline harness review

5 November 2026, reviewer `/root/capture_reviewer`, different from root harness/
metadata author and SQL test author `/root/capture_codec_tests`. Isolated branch
`task/STORAGETEST-018-sql-harness-review`, worktree
`/tmp/subathon-storagetest018-sql-harness-review`, based on actual
`573790bba8c6be393fdf9416a331dd176cb6ce21`.

**APPROVED for actual573 CMake/CI configuration, and corrected metadata text
blobs below.** This is separate from the SQL test-source review assigned to
`capture_fixture_author`; it does not approve that source on another reviewer's
behalf or authorize feature execution before that gate and selected-driver proof.

| Actual inspected artifact | Exact blob |
| --- | --- |
| tests/qt/CMakeLists.txt at573 | cb0d843bde49e95a563f1d1a6400def70716957b |
| .github/workflows/sqlite-writer.yml at573 | f9cc19274ce25df13e42830f3e999dcb44ffcc4d |
| corrected root checkpoint text | 521d43429fe7cd14d547b8f60544cc4b5f785910 |
| corrected root native evidence text | e59d799e89b1e4aa21a580b96cb93ac254bc3a54 |

Compared configuration against previously reviewed21c candidate and preserved
fixture/codec approvals11e07c9/1c6ac77. New target links reviewed capture library
and QtTest, uses AUTOMOC and exact independent fixture paths with old-prefix
definitions. All30 source slot names are registered separately with enabled
acceptance/draft-red/sqlite-reference-capture labels and30s timeouts. The new
ordinary CI guard requires exactly30 registered cases, exactly one executable,
complete equality to compiled `-functions` names, no duplicate/disabled/inverted
cases and explicit intentional-red labels. It invokes no SQL feature body. The
four-slot old-only foundation remains unconditional. Existing tests/pins/check
names/workflow permissions remain unchanged. Later slot additions require a
fresh actual source-count/configuration comparison; this approval is not a blank
approval of future tests.

Independent clean configure/build of the three new executables succeeded:

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug --target server_sqlite_reference_capture_acceptance server_reference_capture_values_acceptance server_sqlite_reference_capture_environment
ctest --preset sqlite-reference-capture-debug -L '^sqlite-reference-capture$' --show-only=json-v1
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0. Fresh51-step targeted build; exact30 compiled/registered/enabled SQL
slots, with all required labels and no feature body invocation. Workflow YAML
and all24 embedded Python blocks parse. All419 task IDs/1605 edges and every
other task record are preserved;018 remains in_progress and STORAGE018 planned.
Tooling validates current render/links/presets. Host GNU16.2.1/Qt6.12.0, selected
Boost1.83.0, CMake4.4.5/Python3.14.9. This targeted build does not claim an
independent execution of the old309 suite or any new acceptance body.

Independently read actual root raw/XML evidence. Corrected foundation log SHA256
`b5e6c97fff3654cbbe0566ebe92d5083c502d74cdaaa9fe248c6041f8a7a425e`
records all4 diagnostic slots/6Qt passes/0skips192ms; stable309 XML has309tests/
0failures/disabled/skipped, with raw SHA256
`6576818daeaf058fc824c741c68b19f3356c7fdecb5128ae956cd39999867d58`.
These corroborate root observations, not reviewer-run runtime checks. The earlier
5PASS/1FAIL diagnostic defect is preserved separately and is not feature red.

Requested two wording corrections before metadata approval: old336 registration
count must be explicitly historical versus current366 after SQL authoring; codec
review must be completed rather than a remaining gate. Actual corrected text
blobs above resolve both and distinguish current source-review/selected-CI/
FIRST-red gates. Root also removes an unobserved SQLite-version claim from only
018's note/activity; that correction preserves honest Qt-only observed evidence.

No new feature body/marker or selected official-driver success is claimed. SQL
source review, selected CI mandatory foundation/discovery, actual fixture freeze,
genuine intended codec/SQL red, final baseline review/publication/merge remain.
Production, authority/provider/recovery/platform/UI and full release are outside
this bounded configuration/metadata approval.
