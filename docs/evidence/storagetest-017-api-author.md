# STORAGETEST-017 minimal additive API/stub author

Author: bundle_implementer, 2026-11-03, existing isolated
 task/STORAGETEST-017-contract-author. Root explicitly authorized this MINIMAL
API/STUB phase after actual different document approval39a50970 (rootbb27a06)
for concrete contractbad19c47/author02394078. The earlier worktree's two contract
Markdown files are preserved; this phase changes only the bundle header, bundle
source and this own note. Root owns shared build/CI/state integration.

## Actual additions

- Fixed `ApplicationOriginalClaimSelector`: existing SubjectKind, ReceiptId,
  optional CID. No convenience/validation/authorization constructor.
- `SqliteOriginalClaimKind` with subject0/source1 and owned
  `ApplicationOriginalClaimMatch { kind, RecordedApplicationBundle existing }`.
- One const optional-return `lookup_original_claim` declaration and its four-line
  source definition: immediate unconditional `throw SqliteOriginalClaimNotImplemented()`.
- Unique explicit logic_error marker for this new feature. No old marker changes.
- Scoped comments pin fixed-selector no input aggregate quota, full selected
  closure/global subject priority, result enum1/zero new vector items, provisional
  ownership and actual error/rollback truth as the future contract. They explicitly
  identify the current implementation as a named stub pending its own reviewed red.

No validation, helper access, SQL, lookup/matching, mapping, cleanup, callback,
transaction or production query behavior is supplied. The method was NOT invoked.
No tests/fixtures/new native suite/red were authored or run. Existing constructor/
destructor/old seven functions/header fields/enums/schema/data are unchanged.

## Exact artifact preservation

New header Git blob `cf721e7f61f10dacfb7d078e1545a2d403e0a4df`;
new source `130e3db62b421479ec3d0a25ce2e712480482f0e`.
An independent removal check strips ONLY the exact added marker/type/declaration
blocks and the new four-line definition, then computes Git blob framing/SHA1:

| Restored existing artifact | Exact Git blob |
| --- | --- |
| Header | `8e24f0624673bf88f86405651f9aabee1026a226` |
| Source including all existing implementation | `a9056be460d6f0536d66341d09e70f177a6638e3` |

Both match the actual integrated21c base byte-for-byte. There is no old-file-wide
reformat, declaration reorder or hidden validation edit. New marker/types/body
snippets independently compare equal to `clang-format --style=file` output,
ignoring only their insertion-boundary trailing blank line.

## Actual checks

- First direct `c++ -fsyntax-only` invocation on the header/source compiled with
  exit0 but emitted the expected main-file `#pragma once` warning and a Qt system
  header conversion warning because pkg-config supplied normal include flags.
  Those were syntax diagnostics, not a feature red or zero-warning proof.
- Corrected check compiles a header-include TU from stdin and the actual source,
  using `c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -fPIC
  -fsyntax-only -Iserver/include -Icore/include`, pkg-config Qt6Core/Qt6Sql flags
  with each Qt `-I` treated as `-isystem` just as existing CMake does. Header TU
  and source each exit0 with ZERO diagnostic bytes. No link/run/new entrypoint.
- Actual tools: GNU c++16.2.1 (20260810); pkg-config Qt6Core/Qt6Sql6.12.0.
- `python3 tools/check_project.py`: PASS396 records/122 decisions/current HTML/
  local links/presets. `git diff --check`: PASS.

Root's real existing-profile configure/build/old209 and published own selected CI
remain separate next checks. Actual different API/config review is required before
independent fixture/test authoring. Only those later reviewed tests/compile/
discovery/environment gate and intended new-stub red authorize STORAGE-017
implementation. Old native passes/old red are not this reader's baseline.
All service/access/current-policy/hold/recovery/provider/platform release gates
remain explicit; no owner decision/access or live operation was needed.
