# STORAGETEST-018 — minimal interface and immediate-stub authorship

Status: **authored minimal candidate awaiting different source/harness review**.
Only declarations, owned DTOs, fixed errors, inert facade storage and immediate
missing-behavior markers were added. No018 entrypoint, marker, constructor, codec,
SQL/schema diagnostic, fixture setup or native program was invoked. Object-only
compilation below does not establish intended red or implemented behavior.

## Scope and authority

Author `/root/access_reviewer`, separate from reviewer `/root/selected_boost`.
Isolated `/tmp/subathon-storagetest018-minimal-author`, branch
`task/STORAGETEST-018-minimal-author`, created by root at
`365be94942e155ea2f066415108642ea1c689ee7`. Actual different document/API/physical
approval `0411b7d47e174f9561cb003db958b84fb52252d0` was integrated before this
assignment. Read root/server AGENTS, workflow/test strategy, task018, exact
[minimal release](storagetest-018-minimal-release.md), reviewed
[supplier contract](../contracts/sqlite-initial-captures-v1.md),
[physical proposal](../contracts/sqlite-initial-captures-schema-v1.md) and
CONTRACT014 consumer blob `5a12ebea1ecce7192c57db3462427bc3cfb9bfd6`.

Own ONLY these FOUR new code files plus this NEW note:

| Path | Actual candidate blob |
| --- | --- |
| server/include/subathon/server/reference_capture_values.hpp | e68bf4830eed617a938e620def63d322389fcda8 |
| server/src/reference_capture_values.cpp | 5445276db1897d7edb8ca24dedb69e8e31f5093c |
| server/include/subathon/server/sqlite_reference_capture.hpp | 86ad17dc2417915c05b8dfb7aac67aee6064c652 |
| server/src/sqlite_reference_capture.cpp | 3372f37d2013196a535510db9a9e2e13d61038bd |

All old sources/headers/contracts/fixtures/assertions and immutable migration1–6
remain unchanged. Root retains CMake/presets/CI/state/map/HTML/checkpoint/schema/
manifest integration and publication. Root has accepted coordinate7/ID for this
gated baseline; this candidate constructs no migration, prefix or SQL and installs
no schema. Predictive document framing is not native projection evidence.

## Authored surface

Shared header owns exactly the reviewed fixed raw DTOs, uint8 fact/input enums,
six uint64 limits and allocation-free ReferenceInputFailure(code)/code()/what().
Real ApplicationPrincipalBinding, ApplicationPlanKey/ApplicationEnvelope,
receipt/CID/TimerConfig types are imported, never redefined. All raw aggregate
member order/defaults are preserved. In particular ReferenceRule.automation keeps
the actual imported default account0; no default automation authority is invented.
No admission/configuration source/evaluator/current authority/clock/ref producer
is declared or implemented here. Future API011 imports these same shared values.

Eight encode/decode bodies each consist SOLELY of
`throw ReferenceCaptureValuesNotImplemented{};`, with exact fixed message
`Reference capture values are intentionally unimplemented; review tests first`.
Manifest/find/retain bodies each consist SOLELY of
`throw SqliteReferenceCaptureNotImplemented{};`, with exact fixed message
`SQLite initial captures are intentionally unimplemented; review tests first`.
The facade constructor only moves/copies its revocable session and limits; no
validation, thread/session phase probe, SQL, connection/adoption or cleanup.
Fixed storage error text is `Reference initial capture did not commit`.
No successful return, prefix construction, field parsing/formatting or other
behavior precedes a marker, including for the manifest factory.

## Actual compile-only checks

Linux host GCC16.2.1 (20260810), clang-format23.1.4. Actual
`pkg-config --modversion Qt6Core Qt6Sql Qt6Test` reports6.12.0 for all three.
Formatted only the four owned new code files using repository .clang-format.
Executed these exact object-only commands from the isolated worktree:

```sh
c++ -std=c++20 -fPIC -Wall -Wextra -Wpedantic -Werror $(pkg-config --cflags Qt6Core Qt6Sql) -Icore/include -Iserver/include -c server/src/reference_capture_values.cpp -o /tmp/storagetest018-reference-capture-values.o
c++ -std=c++20 -fPIC -Wall -Wextra -Wpedantic -Werror $(pkg-config --cflags Qt6Core Qt6Sql) -Icore/include -Iserver/include -c server/src/sqlite_reference_capture.cpp -o /tmp/storagetest018-sqlite-reference-capture.o
```

Both exit0 with NO diagnostics. No executable was linked or invoked. This checks
the actual type imports/declarations/body compilation, not marker runtime behavior.
No CMake configure/build/list/CTest or Qt/SQLite foundation/probe/setup was run by
this author; root owns the separately reviewed build-only harness.

Additional source-text-only Python check compared all nine DTO member/default
lists to actual frozen consumer/supplier declarations after stripping comments/
whitespace, and checked all8+3 function bodies contain only their correct throw;
PASS. It read text only, imported no project module and invoked no code entrypoint.
`clang-format --dry-run --Werror` on all four code files PASS. Project consistency,
`python3 tools/check_project.py` PASS419 task records/122 decisions/local links/
presets/current HTML, retaining its draft19 note. `python3 tools/progress.py render
--check` PASS current snapshot without rewriting root-owned HTML/state. Staged
`git diff --cached --check` PASS; exactly the five assigned NEW paths, no old file
or shared-file changes. These are author checks, not independent review.

Remaining gates: DIFFERENT actual minimal source plus root harness review →
separately released real-driver schema diagnostics/independent literal fixtures →
authored tests/harness and separate actual review → successful full compile and
genuine intended feature marker red → integrated STORAGETEST-018. STORAGE-018
implementation waits for that baseline. Manifest stub must never be called as
fixture setup and its failure misreported as feature red. No018/storage/readiness,
source authority, API011, provider or recovery capability is claimed. No PR was
published or merged by this author.
