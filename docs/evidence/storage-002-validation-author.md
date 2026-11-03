# STORAGE-002 pure value helper authoring

Author: bundle_validation, 2026-11-03, task/STORAGE-002-validation, based on
integrated main995e0fa. Root owns the task claim/progress/build/workflow; implementation
and independent source review are separate agents/worktrees.

Owned paths: server/src/sqlite_application_bundle_values.hpp and this note.
The internal header supplies pure options/key/count/state/contribution/effect/job/
reversal preflight, exact local and aggregate conservation, logical owned-byte
accounting, complete state/intent equality, and the literal reviewed physical step5
manifest atop the existing prefix1–4 factory. It neither accesses a session nor
mutates input, starts/commits/rolls back a transaction or changes a public API.

Authority: concrete sqlite-application-bundle-v1 and physical schema revision1;
reviewed header8e24/featureCPP4b60/fixturee8b/helperb0 and original historical red90fa
remain unchanged. Literal DDL was transcribed from the physical contract (not from
observed SQLite output or generated test expectations). Prior prefix factories are
reused without edits. Signed arithmetic conservation uses Exact, including MIN.
Logical byte weights preserve optional absence and present-empty values. A generic
logical_bytes helper returns checked totals for selected returned DTOs; overflow
throws length_error for the owning runtime adapter to map after actual rollback.

Actual checks:

- clang-format -i server/src/sqlite_application_bundle_values.hpp; completed.
- c++ -std=c++20 -fPIC -Wall -Wextra -Wshadow -I. -Icore/include -Iserver/include
  $(pkg-config --cflags Qt6Core) -fsyntax-only /tmp/bundle_values_compile.cpp;
  exit0, no diagnostic. The temporary translation unit includes this header.
- git diff --check; exit0.

Earlier direct-header diagnostic also passed compilation but produced a pragma-once
main-file warning plus one Qt system-header conversion warning under -Wconversion;
it was not a warning-free production build claim. The normal include translation
unit above is the warning-free syntax check. No feature case, SQL transaction,
old suite or red baseline was executed by these syntax checks. Integration must run
the unchanged32 and full206 cases, obtain actual different source/config review,
and observe promoted ordinary CI before task completion. This helper authoring
alone establishes no passing application-bundle acceptance or full release support.
